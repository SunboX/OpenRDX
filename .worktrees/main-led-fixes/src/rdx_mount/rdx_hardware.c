/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_hardware.c
//
// Description : RDX mechanism, slider, fan, and thermal runtime.
//=======================================================================================

/*! @file
 * @brief Foreground coordinator for the RDX dock hardware controls.
 */

#include "rdx_hardware.h"

#include "ahci.h"
#include "gio.h"
#include "pwm.h"
#include "rdx_button_mode.h"
#include "rdx_led.h"
#include "rdx_manager_protocol.h"
#include "rdx_mechanism.h"
#include "rdx_runtime_ata.h"
#include "sata_media.h"
#include "reg_io.h"
#include "rti.h"
#include "scsi.h"
#include "spi.h"
#include "vim_nvic.h"

#define RDX_USB_INTERRUPT_MASK                  0x00200000UL
#define RDX_INPUT_DEBOUNCE_MS                   100UL
#define RDX_SLIDER_CHANNEL                      4U
#define RDX_SLIDER_UNLOCKED_THRESHOLD           650U
#define RDX_FAN_SERVICE_MS                      800UL
#define RDX_FAN_START_DELAY_MS                  100UL
#define RDX_FAN_RAMP_DELAY_MS                   60000UL
#define RDX_FAN_INITIAL_DUTY                    10U
#define RDX_FAN_NORMAL_DUTY                     50U
#define RDX_FAN_RAMP_STEP                       13U
#define RDX_TEMPERATURE_NORMAL_SAMPLE_MS        300000UL
#define RDX_TEMPERATURE_HOT_SAMPLE_MS           10000UL
#define RDX_TEMPERATURE_FAILURE_RETRY_MS       10000UL
#define RDX_TEMPERATURE_COOL_MAX                40U
#define RDX_TEMPERATURE_HOT_MIN                 45U
#define RDX_TEMPERATURE_HOT_CONFIRMATIONS       3U
#define RDX_TEMPERATURE_UNAVAILABLE             0xFFU

typedef enum _RDX_EJECT_STATE_T
{
    RDX_EJECT_IDLE = 0,
    RDX_EJECT_WAIT_FOR_IO,
    RDX_EJECT_MECHANISM_ACTIVE
} RDX_EJECT_STATE_T;

typedef enum _RDX_FAN_STATE_T
{
    RDX_FAN_STATE_IDLE = 0,
    RDX_FAN_STATE_NORMAL = 1,
    RDX_FAN_STATE_FULL = 2,
    RDX_FAN_STATE_RAMPING = 3,
    /* State 4 is an immediate in-call desired-zero transition.  The
     * implementation performs that off -> IDLE transition directly, while
     * the delayed startup phase retains its required numeric value 5. */
    RDX_FAN_STATE_STARTING = 5
} RDX_FAN_STATE_T;

typedef enum _RDX_THERMAL_PHASE_T
{
    RDX_THERMAL_MEDIA_ABSENT = 0,
    RDX_THERMAL_COOL = 1,
    RDX_THERMAL_HOT = 2,
    RDX_THERMAL_INITIAL = 3
} RDX_THERMAL_PHASE_T;

typedef struct _RDX_HARDWARE_STATE_T
{
    volatile BOOLEAN_T eject_requested;
    volatile BOOLEAN_T activity_requested;
    BOOLEAN_T write_protected;
    BOOLEAN_T button_candidate;
    BOOLEAN_T button_stable;
    BOOLEAN_T button_armed;
    UINT32_T button_candidate_since;
    BOOLEAN_T cartridge_candidate;
    BOOLEAN_T cartridge_stable;
    BOOLEAN_T ejected_latched;
    BOOLEAN_T logical_unloaded;
    BOOLEAN_T eject_fault_latched;
    UINT32_T cartridge_candidate_since;
    BOOLEAN_T observed_ready;
    BOOLEAN_T observed_init_failed;
    RDX_EJECT_STATE_T eject_state;
    UINT8_T temperature_celsius;
    UINT8_T hot_sample_count;
    UINT32_T temperature_deadline;
    RDX_FAN_STATE_T fan_state;
    BOOLEAN_T fan_disabled;
    RDX_THERMAL_PHASE_T thermal_phase;
    UINT8_T fan_duty;
    UINT32_T fan_service_deadline;
    UINT32_T fan_action_deadline;
} RDX_HARDWARE_STATE_T;

static RDX_HARDWARE_STATE_T rdx_hardware;

/** Return the current millisecond counter used by the timer services. */
static UINT32_T rdx_hardware_now_ms(void)
{
    return READ_REG32(RTIFRC1_REG_OFF);
}

/** Return TRUE once a wrap-safe millisecond interval has elapsed. */
static BOOLEAN_T rdx_hardware_interval_elapsed(UINT32_T now,
                                               UINT32_T start,
                                               UINT32_T duration)
{
    return ((UINT32_T)(now - start) >= duration) ? TRUE : FALSE;
}

/** Return TRUE when a wrap-safe absolute deadline has been reached. */
static BOOLEAN_T rdx_hardware_deadline_reached(UINT32_T now,
                                               UINT32_T deadline)
{
    return ((INT32_T)(now - deadline) >= 0) ? TRUE : FALSE;
}

/** Return TRUE when any deferred ATA callback still owns shared state. */
static BOOLEAN_T rdx_hardware_callback_is_pending(void)
{
    UINT32_T index;

    for (index = 0U; index < ATA_CALLBACK_QUEUE_DEPTH; index++)
    {
        if (ata_dev[0].callback_pending[index])
        {
            return TRUE;
        }
    }
    return FALSE;
}

/** Return TRUE when the SATA port and callback queue are both quiescent. */
static BOOLEAN_T rdx_hardware_ata_is_idle(void)
{
    return (((READ32(PxCI(0)) | READ32(PxSACT(0))) == 0U) &&
            !rdx_hardware_callback_is_pending()) ? TRUE : FALSE;
}

/** Return TRUE while a debounced cartridge has not physically terminated. */
static BOOLEAN_T rdx_hardware_cartridge_is_available(void)
{
    return (rdx_hardware.cartridge_stable &&
            !rdx_hardware.ejected_latched) ? TRUE : FALSE;
}

/** Program PWM0 only when the requested fan duty changes. */
static void rdx_hardware_set_fan_duty(UINT8_T duty)
{
    if (duty > 100U)
    {
        duty = 100U;
    }
    if (rdx_hardware.fan_duty == duty)
    {
        return;
    }

    if (duty == 0U)
    {
        pwm_disable(RDX_FAN_PWM_NUM);
    }
    else
    {
        pwm_run(RDX_FAN_PWM_NUM, duty, RDX_FAN_PWM_PERIOD_US);
    }
    rdx_hardware.fan_duty = duty;
}

/**
 * @brief Publish common eject teardown as a no-media state.
 *
 * Accepted host and physical requests share this teardown before the
 * mechanism starts.  Both terminal states 4 and 8 apply the same media,
 * temperature, fan, and cartridge-LED invalidation.  Applying the subset
 * idempotently before motor motion prevents host access or cooling work from
 * surviving an accepted eject, including terminal retry exhaustion.
 */
static void rdx_hardware_teardown_eject_media(void)
{
    ata_dev[0].bDeviceInitComplete = FALSE;
    ata_dev[0].bDeviceInitTimedOut = TRUE;
    rdx_hardware.write_protected = TRUE;
    rdx_hardware.temperature_celsius = RDX_TEMPERATURE_UNAVAILABLE;
    rdx_hardware.thermal_phase = RDX_THERMAL_MEDIA_ABSENT;
    rdx_hardware.ejected_latched = TRUE;
    rdx_hardware.logical_unloaded = FALSE;
    /* A physical mechanism teardown terminates the page 31h policy session;
     * the next cartridge must not inherit logical-unload or auto-reload bits. */
    rdx_manager_clear_host_eject_policy();
    rdx_hardware_set_fan_duty(0U);
    rdx_hardware.fan_state = RDX_FAN_STATE_IDLE;
    rdx_led_select_second(RDX_LED_CARTRIDGE, FALSE);
    rdx_led_set_steady(RDX_LED_CARTRIDGE, FALSE);
}

/** Finish a successful mechanism operation in the safe no-media state. */
static void rdx_hardware_finish_eject(void)
{
    rdx_hardware_teardown_eject_media();
    rdx_hardware.eject_fault_latched = FALSE;
    rdx_hardware.eject_requested = FALSE;
    rdx_hardware.eject_state = RDX_EJECT_IDLE;
    /* A successful eject restores the dock's steady amber indication. */
    rdx_led_select_second(RDX_LED_DOCK, FALSE);
    rdx_led_set_steady(RDX_LED_DOCK, TRUE);
}

/**
 * @brief Complete terminal state 8 without restoring cartridge access.
 *
 * Retry exhaustion uses the same media teardown as successful state 4; only
 * the dock selector/failure indication differs.  Restoring the saved ready
 * flag here would expose media after a terminal mechanism result.
 */
static void rdx_hardware_fail_eject(void)
{
    rdx_hardware_teardown_eject_media();
    rdx_hardware.eject_requested = FALSE;
    rdx_hardware.eject_state = RDX_EJECT_IDLE;
    rdx_hardware.eject_fault_latched = TRUE;
    /* Retry exhaustion selects the dock's green output. */
    rdx_led_select_second(RDX_LED_DOCK, TRUE);
}

/** Debounce the active-low cartridge-present input for exactly 100 ms. */
static void rdx_hardware_service_cartridge_input(UINT32_T now)
{
    BOOLEAN_T raw_present;

    raw_present = gio_rdx_cartridge_present();
    if (raw_present != rdx_hardware.cartridge_candidate)
    {
        rdx_hardware.cartridge_candidate = raw_present;
        rdx_hardware.cartridge_candidate_since = now;
    }
    else if ((raw_present != rdx_hardware.cartridge_stable) &&
             rdx_hardware_interval_elapsed(
                 now, rdx_hardware.cartridge_candidate_since,
                 RDX_INPUT_DEBOUNCE_MS))
    {
        rdx_hardware.cartridge_stable = raw_present;
        rdx_hardware.ejected_latched = FALSE;
        rdx_hardware.logical_unloaded = FALSE;
        rdx_hardware.eject_fault_latched = FALSE;
        rdx_hardware.write_protected = TRUE;
        rdx_hardware.temperature_celsius = RDX_TEMPERATURE_UNAVAILABLE;
        rdx_hardware.hot_sample_count = 0U;

        if (raw_present)
        {
            /* Assert logical output 5 only after the GPIO5 insertion has
             * survived its 100-ms timer and the
             * mechanism dispatcher is idle. */
            if (rdx_hardware.eject_state == RDX_EJECT_IDLE)
            {
                gio_rdx_mechanism_auxiliary_set(TRUE);
            }
            rdx_hardware.thermal_phase = RDX_THERMAL_INITIAL;
            rdx_hardware.observed_init_failed = FALSE;
            /* Enable the cartridge/activity controller when the
             * debounced cartridge monitor enters insertion state one. */
            rdx_led_select_second(RDX_LED_CARTRIDGE, FALSE);
            rdx_led_set_steady(RDX_LED_CARTRIDGE, TRUE);
        }
        else
        {
            /* Debounced physical removal also ends any retained page 31h
             * logical-unload policy, including removal while already hidden. */
            rdx_manager_clear_host_eject_policy();
            gio_rdx_mechanism_auxiliary_set(FALSE);
            rdx_hardware.thermal_phase = RDX_THERMAL_MEDIA_ABSENT;
            /* Removal darkens the cartridge/activity LED. The independent
             * dock/eject-button controller remains steady amber. */
            rdx_led_select_second(RDX_LED_CARTRIDGE, FALSE);
            rdx_led_set_steady(RDX_LED_CARTRIDGE, FALSE);
        }
    }
}

/** Debounce one button press, issue one request, and rearm on release. */
static void rdx_hardware_service_eject_button(UINT32_T now)
{
    BOOLEAN_T raw_pressed;

    raw_pressed = gio_rdx_eject_button_pressed();
    if (raw_pressed != rdx_hardware.button_candidate)
    {
        rdx_hardware.button_candidate = raw_pressed;
        rdx_hardware.button_candidate_since = now;
    }
    else if ((raw_pressed != rdx_hardware.button_stable) &&
             rdx_hardware_interval_elapsed(
                 now, rdx_hardware.button_candidate_since,
                 RDX_INPUT_DEBOUNCE_MS))
    {
        rdx_hardware.button_stable = raw_pressed;
        if (raw_pressed)
        {
            if (rdx_hardware.button_armed &&
                rdx_hardware_cartridge_is_available())
            {
                /* Page 33h inhibits only physical-button eject.  Consume the
                 * debounced press while inhibited so clearing the setting
                 * cannot turn an already-held button into a delayed request;
                 * release must rearm the button first. */
                if (!rdx_manager_physical_eject_is_enabled())
                {
                    rdx_hardware.button_armed = FALSE;
                }
                /* Check PREVENT at the remaining physical-request boundary.
                 * Denial updates only the dock indication and does not install
                 * SCSI sense.  Host LOEJ performs its check synchronously, so
                 * the common asynchronous coordinator does not distinguish
                 * the source or recheck PREVENT after acceptance. */
                else if (scsi_medium_removal_is_prevented())
                {
                    rdx_led_start_second_blink(RDX_LED_DOCK);
                    rdx_hardware.button_armed = FALSE;
                }
                else
                {
                    rdx_hardware.eject_requested = TRUE;
                    rdx_hardware.button_armed = FALSE;
                }
            }
        }
        else
        {
            rdx_hardware.button_armed = TRUE;
        }
    }
}

/** Update normal, hot, and no-media fan phases every 800 ms. */
static void rdx_hardware_service_fan(UINT32_T now)
{
    UINT8_T next_duty;

    /* Hardware profile 36h selects a terminal fan-disabled state with no
     * service transition.  It remains disabled for the whole boot rather
     * than merely skipping the initial 10-percent command. */
    if (rdx_hardware.fan_disabled)
    {
        return;
    }

    if (!rdx_hardware_deadline_reached(
            now, rdx_hardware.fan_service_deadline))
    {
        return;
    }
    rdx_hardware.fan_service_deadline = now + RDX_FAN_SERVICE_MS;

    if (rdx_hardware.thermal_phase == RDX_THERMAL_MEDIA_ABSENT)
    {
        rdx_hardware_set_fan_duty(0U);
        rdx_hardware.fan_state = RDX_FAN_STATE_IDLE;
        return;
    }

    switch (rdx_hardware.fan_state)
    {
        case RDX_FAN_STATE_IDLE:
            if ((rdx_hardware.thermal_phase == RDX_THERMAL_COOL) ||
                (rdx_hardware.thermal_phase == RDX_THERMAL_HOT))
            {
                rdx_hardware.fan_action_deadline =
                    now + RDX_FAN_START_DELAY_MS;
                rdx_hardware.fan_state = RDX_FAN_STATE_STARTING;
            }
            break;

        case RDX_FAN_STATE_STARTING:
            if (rdx_hardware_deadline_reached(
                    now, rdx_hardware.fan_action_deadline))
            {
                rdx_hardware_set_fan_duty(RDX_FAN_NORMAL_DUTY);
                rdx_hardware.fan_state = RDX_FAN_STATE_NORMAL;
            }
            break;

        case RDX_FAN_STATE_NORMAL:
            if (rdx_hardware.thermal_phase == RDX_THERMAL_HOT)
            {
                rdx_hardware.fan_action_deadline = now;
                rdx_hardware.fan_state = RDX_FAN_STATE_RAMPING;
            }
            break;

        case RDX_FAN_STATE_RAMPING:
            /* The ramping state completes its +13-percent-per-minute ramp even
             * when a cool sample arrives.  Only state 2 (full) consumes the
             * cool phase and returns the output to 50 percent. */
            if (rdx_hardware_deadline_reached(
                    now, rdx_hardware.fan_action_deadline))
            {
                if (rdx_hardware.fan_duty > 99U)
                {
                    rdx_hardware.fan_state = RDX_FAN_STATE_FULL;
                }
                else
                {
                    next_duty = rdx_hardware.fan_duty + RDX_FAN_RAMP_STEP;
                    if (next_duty > 100U)
                    {
                        next_duty = 100U;
                    }
                    rdx_hardware_set_fan_duty(next_duty);
                    rdx_hardware.fan_action_deadline =
                        now + RDX_FAN_RAMP_DELAY_MS;
                }
            }
            break;

        case RDX_FAN_STATE_FULL:
            if (rdx_hardware.thermal_phase == RDX_THERMAL_COOL)
            {
                rdx_hardware_set_fan_duty(RDX_FAN_NORMAL_DUTY);
                rdx_hardware.fan_state = RDX_FAN_STATE_NORMAL;
            }
            break;

        default:
            rdx_hardware_set_fan_duty(0U);
            rdx_hardware.fan_state = RDX_FAN_STATE_IDLE;
            break;
    }
}

/** Poll SMART temperature without racing host or callback-owned ATA work. */
static void rdx_hardware_service_temperature(UINT32_T now)
{
    UINT8_T temperature;
    BOOLEAN_T successful;

    if (!rdx_hardware.observed_ready ||
        !rdx_hardware_deadline_reached(
            now, rdx_hardware.temperature_deadline))
    {
        return;
    }

    if (!sata_media_smart_is_available(0U))
    {
        rdx_hardware.temperature_celsius = RDX_TEMPERATURE_UNAVAILABLE;
        rdx_hardware.temperature_deadline =
            now + RDX_TEMPERATURE_NORMAL_SAMPLE_MS;
        return;
    }
    if (!rdx_hardware_ata_is_idle())
    {
        return;
    }

    WRITE_REG32(VIM_REQMASKCLR0, RDX_USB_INTERRUPT_MASK);
    if (!rdx_hardware_ata_is_idle())
    {
        WRITE_REG32(VIM_REQMASKSET0, RDX_USB_INTERRUPT_MASK);
        return;
    }
    successful = rdx_read_smart_temperature(0U, &temperature);
    WRITE_REG32(VIM_REQMASKSET0, RDX_USB_INTERRUPT_MASK);

    if (!successful)
    {
        rdx_hardware.temperature_celsius = RDX_TEMPERATURE_UNAVAILABLE;
        /* Unsupported or temporarily unavailable SMART data must not consume
         * every idle foreground pass. Retry soon without blocking USB or
         * delaying storage traffic. */
        rdx_hardware.temperature_deadline =
            rdx_hardware_now_ms() + RDX_TEMPERATURE_FAILURE_RETRY_MS;
        return;
    }

    rdx_hardware.temperature_celsius = temperature;
    if (temperature >= RDX_TEMPERATURE_HOT_MIN)
    {
        if (rdx_hardware.hot_sample_count <
            RDX_TEMPERATURE_HOT_CONFIRMATIONS)
        {
            rdx_hardware.hot_sample_count++;
        }
        else
        {
            rdx_hardware.thermal_phase = RDX_THERMAL_HOT;
        }
        rdx_hardware.temperature_deadline =
            rdx_hardware_now_ms() + RDX_TEMPERATURE_HOT_SAMPLE_MS;
    }
    else
    {
        /* Prime the counter to three below 45 C.  Starting from zero,
         * four consecutive >=45 C readings are therefore required; after a
         * cooler reading, the next >=45 C reading applies the hot phase. */
        rdx_hardware.hot_sample_count =
            RDX_TEMPERATURE_HOT_CONFIRMATIONS;
        if (temperature <= RDX_TEMPERATURE_COOL_MAX)
        {
            rdx_hardware.thermal_phase = RDX_THERMAL_COOL;
        }
        /* 41..44 C deliberately retains the current hysteresis band. */
        rdx_hardware.temperature_deadline =
            rdx_hardware_now_ms() + RDX_TEMPERATURE_NORMAL_SAMPLE_MS;
    }
}

/** Follow media readiness and storage activity with RDX LED semantics. */
static void rdx_hardware_service_media_state(UINT32_T now)
{
    BOOLEAN_T ready;
    BOOLEAN_T init_failed;

    ready = ata_dev[0].bDeviceInitComplete;
    init_failed = ata_dev[0].bDeviceInitTimedOut;

    if (ready != rdx_hardware.observed_ready)
    {
        rdx_hardware.observed_ready = ready;
        if (ready)
        {
            rdx_hardware.observed_init_failed = FALSE;
            rdx_hardware.thermal_phase = RDX_THERMAL_COOL;
            rdx_hardware.temperature_celsius =
                RDX_TEMPERATURE_UNAVAILABLE;
            rdx_hardware.hot_sample_count = 0U;
            rdx_hardware.temperature_deadline =
                now + RDX_TEMPERATURE_NORMAL_SAMPLE_MS;
            /* Successful discovery restores the cartridge/activity LED's
             * normal green selector. */
            rdx_led_select_second(RDX_LED_CARTRIDGE, FALSE);
        }
        else
        {
            /* A removal or reinitialization invalidates the cached sample.
             * Remain fail-closed until the next ready transition samples it. */
            rdx_hardware.write_protected = TRUE;
        }
    }

    if ((init_failed != rdx_hardware.observed_init_failed) &&
        !ready)
    {
        rdx_hardware.observed_init_failed = init_failed;
        if (rdx_hardware_cartridge_is_available() &&
            (rdx_hardware.eject_state == RDX_EJECT_IDLE))
        {
            /* AHCI discovery and initialization failures feed the
             * cartridge/activity controller's aggregate condition flag. */
            rdx_led_select_second(RDX_LED_CARTRIDGE, init_failed);
        }
    }

    if (rdx_hardware.activity_requested)
    {
        rdx_hardware.activity_requested = FALSE;
        if (ready)
        {
            /* READ, WRITE, WRITE AND VERIFY, and VERIFY all queue the normal
             * six-phase cartridge sequence. */
            rdx_led_start_blink(RDX_LED_CARTRIDGE);
        }
    }
}

/**
 * @brief Coordinate quiescence around the mechanism engine.
 *
 * Physical-button and host LOEJ requests converge here after their source
 * interlocks. This wrapper owns callback/PxCI/PxSACT quiescence, the
 * standby-only mechanism preparation, media publication, and one persistent
 * load-count update. The separate mechanism module owns the eight timed
 * actuator states. Ordinary SCSI STOP is a separate flush-plus-standby path.
 *
 * @param[in] now current free-running millisecond counter.
 */
static void rdx_hardware_service_eject(UINT32_T now)
{
    RDX_MECHANISM_EVENT_T event;

    if ((rdx_hardware.eject_state == RDX_EJECT_IDLE) &&
        rdx_hardware.eject_requested)
    {
        if (!rdx_hardware_cartridge_is_available())
        {
            rdx_hardware.eject_requested = FALSE;
            return;
        }

        /* Every accepted host or physical request updates and persists the
         * load count exactly once, before media teardown and mechanism work. */
        (void)rdx_manager_increment_drive_load_count();
        rdx_hardware.eject_fault_latched = FALSE;
        rdx_hardware.eject_state = RDX_EJECT_WAIT_FOR_IO;
        rdx_led_start_blink(RDX_LED_DOCK);
    }

    if (rdx_hardware.eject_state == RDX_EJECT_WAIT_FOR_IO)
    {
        /* PREVENT has already been checked at the request source.  Once
         * accepted, the count, media teardown, and mechanism sequence proceed
         * without a second interlock that could cancel an accepted request. */
        if (!rdx_hardware_ata_is_idle())
        {
            return;
        }

        if (ata_dev[0].bDeviceInitComplete &&
            gio_rdx_cartridge_present())
        {
            WRITE_REG32(VIM_REQMASKCLR0, RDX_USB_INTERRUPT_MASK);
            if (rdx_hardware_ata_is_idle())
            {
                /* Mechanical eject issues STANDBY IMMEDIATE only.  The
                 * ordinary SCSI STOP handler owns its separate ordered
                 * FLUSH+STANDBY transaction.  ATA standby failure is advisory:
                 * once an eject is accepted, mechanism work still proceeds. */
                (void)rdx_prepare_mechanism_eject(0U);
            }
            else
            {
                WRITE_REG32(VIM_REQMASKSET0, RDX_USB_INTERRUPT_MASK);
                return;
            }
            WRITE_REG32(VIM_REQMASKSET0, RDX_USB_INTERRUPT_MASK);
        }

        rdx_hardware_teardown_eject_media();
        rdx_mechanism_start(rdx_hardware_now_ms(),
                            rdx_manager_get_hardware_profile());
        rdx_hardware.eject_state = RDX_EJECT_MECHANISM_ACTIVE;
        return;
    }

    if (rdx_hardware.eject_state != RDX_EJECT_MECHANISM_ACTIVE)
    {
        return;
    }
    event = rdx_mechanism_service(now);
    if (event == RDX_MECHANISM_EVENT_SUCCEEDED)
    {
        /* Success is published only after state 4's 500-ms settle. */
        rdx_hardware_finish_eject();
    }
    else if (event == RDX_MECHANISM_EVENT_FAILED)
    {
        rdx_hardware_fail_eject();
    }
}

/** Initialize every hardware-facing state with fail-safe outputs and values. */
void rdx_hardware_init(void)
{
    UINT32_T now;
    BOOLEAN_T button_pressed;
    BOOLEAN_T cartridge_present;
    UINT16_T hardware_profile;

    now = rdx_hardware_now_ms();
    button_pressed = gio_rdx_eject_button_pressed();
    cartridge_present = gio_rdx_cartridge_present();
    hardware_profile = rdx_manager_get_hardware_profile();

    rdx_hardware.eject_requested = FALSE;
    rdx_hardware.activity_requested = FALSE;
    rdx_hardware.write_protected = TRUE;
    rdx_hardware.button_candidate = button_pressed;
    rdx_hardware.button_stable = button_pressed;
    rdx_hardware.button_armed = button_pressed ? FALSE : TRUE;
    rdx_hardware.button_candidate_since = now;
    rdx_hardware.cartridge_candidate = cartridge_present;
    /* The cartridge-present debounce/controller starts empty.  A
     * cartridge already present at boot must pass the same 100-ms debounce;
     * the independent dock controller is initialized steady amber below. */
    rdx_hardware.cartridge_stable = FALSE;
    rdx_hardware.ejected_latched = FALSE;
    rdx_hardware.logical_unloaded = FALSE;
    rdx_hardware.eject_fault_latched = FALSE;
    rdx_hardware.cartridge_candidate_since = now;
    rdx_hardware.observed_ready = FALSE;
    rdx_hardware.observed_init_failed = FALSE;
    rdx_hardware.eject_state = RDX_EJECT_IDLE;
    rdx_hardware.temperature_celsius = RDX_TEMPERATURE_UNAVAILABLE;
    rdx_hardware.hot_sample_count = 0U;
    rdx_hardware.temperature_deadline =
        now + RDX_TEMPERATURE_NORMAL_SAMPLE_MS;
    rdx_hardware.fan_state = RDX_FAN_STATE_IDLE;
    rdx_hardware.fan_disabled =
        (hardware_profile == 0x36U) ? TRUE : FALSE;
    rdx_hardware.thermal_phase = cartridge_present ?
        RDX_THERMAL_INITIAL : RDX_THERMAL_MEDIA_ABSENT;
    rdx_hardware.fan_duty = 0xFFU;
    rdx_hardware.fan_service_deadline = now;
    rdx_hardware.fan_action_deadline = now;

    /* Initialize PWM1 and GPIO3 at idle. Timed boot homing is started only
     * after synchronous SATA discovery returns to foreground control. */
    rdx_mechanism_init(hardware_profile);
    /* Logical output 5 starts low.  The cartridge-present monitor is its sole
     * normal assertion owner and waits for the GPIO5 insertion debounce; do
     * not assign an unrelated boot-time SATA-power role to this output. */
    gio_rdx_mechanism_auxiliary_set(FALSE);
    /* PWM0 starts at duty 10 unless the effective hardware profile is 36h.
     * Profile values 0, FFFFh, and 1 normalize to 37h; a missing or invalid
     * Manager record uses 38h.  This keeps the fan gate independent of guessed
     * cartridge-presence or board-revision inputs. */
    if (!rdx_hardware.fan_disabled)
    {
        rdx_hardware_set_fan_duty(RDX_FAN_INITIAL_DUTY);
    }
    else
    {
        rdx_hardware_set_fan_duty(0U);
    }

    rdx_led_select_second(RDX_LED_CARTRIDGE, FALSE);
    rdx_led_set_steady(RDX_LED_CARTRIDGE, FALSE);
    /* The eject-button/dock LED starts steady orange with no cartridge. */
    rdx_led_select_second(RDX_LED_DOCK, FALSE);
    rdx_led_set_steady(RDX_LED_DOCK, TRUE);
    rdx_button_mode_init();
}

/** Start conditional boot homing after blocking startup discovery is done. */
void rdx_hardware_start(void)
{
    if (rdx_mechanism_start_boot_homing(rdx_hardware_now_ms()))
    {
        rdx_hardware.eject_state = RDX_EJECT_MECHANISM_ACTIVE;
    }
}

/** Service all hardware behavior from the foreground loop. */
void rdx_hardware_service(void)
{
    rdx_hardware_service_cartridge_input(rdx_hardware_now_ms());
    rdx_hardware_service_eject_button(rdx_hardware_now_ms());
    rdx_button_mode_service(rdx_hardware_now_ms(),
                            rdx_hardware.button_stable,
                            rdx_hardware.cartridge_stable);
    rdx_hardware_service_media_state(rdx_hardware_now_ms());
    rdx_hardware_service_temperature(rdx_hardware_now_ms());
    rdx_hardware_service_fan(rdx_hardware_now_ms());
    rdx_hardware_service_eject(rdx_hardware_now_ms());
}

/** Invalidate cached media hardware state before one ATA initialization. */
void rdx_hardware_media_reinitializing(UINT32_T port_num)
{
    if (port_num != 0U)
    {
        return;
    }
    rdx_hardware.write_protected = TRUE;
}

/** Sample and cache the lock slider before ATA readiness is published. */
void rdx_hardware_prepare_media_ready(UINT32_T port_num)
{
    UINT16_T sample;
    STATUS_T status;

    if (port_num != 0U)
    {
        return;
    }

    /* Fail closed before entering the potentially blocking SPI transaction.
     * The caller has already cleared bDeviceInitComplete, so USB cannot issue
     * media writes until this one-shot sample has completed. */
    rdx_hardware.write_protected = TRUE;
    WRITE_REG32(VIM_REQMASKCLR0, RDX_USB_INTERRUPT_MASK);
    status = rdx_mcp3008_read_channel(RDX_SLIDER_CHANNEL, &sample);
    WRITE_REG32(VIM_REQMASKSET0, RDX_USB_INTERRUPT_MASK);

    if ((status == STATUS_OK) &&
        (sample > RDX_SLIDER_UNLOCKED_THRESHOLD))
    {
        rdx_hardware.write_protected = FALSE;
    }
}

/** Return the fail-closed cartridge write-protection state. */
BOOLEAN_T rdx_hardware_is_write_protected(void)
{
    return rdx_hardware.write_protected;
}

/** Queue one asynchronous safe-eject request. */
void rdx_hardware_request_eject(void)
{
    rdx_hardware.eject_requested = TRUE;
}

/** Hide an online cartridge from SCSI without changing physical hardware. */
BOOLEAN_T rdx_hardware_logical_unload(void)
{
    if (rdx_hardware.logical_unloaded ||
        !rdx_hardware_cartridge_is_available() ||
        !ata_dev[0].bDeviceInitComplete ||
        (rdx_hardware.eject_state != RDX_EJECT_IDLE))
    {
        return FALSE;
    }

    /* This latch is deliberately the only mutation. Logical unload preserves
     * ATA readiness and every physical controller so a later logical reload
     * does not require disk discovery, motor work, or thermal recovery. */
    rdx_hardware.logical_unloaded = TRUE;
    return TRUE;
}

/** Restore SCSI visibility only after a matching logical unload. */
BOOLEAN_T rdx_hardware_logical_reload(void)
{
    if (!rdx_hardware.logical_unloaded)
    {
        return FALSE;
    }

    rdx_hardware.logical_unloaded = FALSE;
    return TRUE;
}

/** Return the logical-unload gate used by SCSI readiness checks. */
BOOLEAN_T rdx_hardware_is_logically_unloaded(void)
{
    return rdx_hardware.logical_unloaded;
}

/** Record storage activity for the foreground LED coordinator. */
void rdx_hardware_note_activity(void)
{
    rdx_hardware.activity_requested = TRUE;
}

/** Return the latest successful SMART temperature or 0xFF. */
UINT8_T rdx_hardware_get_temperature_celsius(void)
{
    return rdx_hardware.temperature_celsius;
}

/** Return TRUE while safe eject owns the mechanism state machine. */
BOOLEAN_T rdx_hardware_eject_in_progress(void)
{
    return (rdx_hardware.eject_state != RDX_EJECT_IDLE) ? TRUE : FALSE;
}
