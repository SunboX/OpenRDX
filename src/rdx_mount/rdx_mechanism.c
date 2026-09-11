/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_mechanism.c
//
// Description : Non-blocking RDX mechanism state machine.
//=======================================================================================

/*! @file
 * @brief Timed, non-blocking control of the cartridge-eject mechanism.
 */

#include "rdx_mechanism.h"

#include "ahci.h"
#include "gio.h"
#include "pwm.h"
#include "rdx_led.h"
#include "reg_io.h"
#include "rti.h"
#include "spi.h"

#define RDX_MECHANISM_DEAD_TIME_US              50UL
#define RDX_MECHANISM_DRIVE_TIME_MS           2000UL
#define RDX_MECHANISM_INTERPHASE_TIME_MS      2000UL
#define RDX_MECHANISM_NO_MEDIA_GRACE_MS        50UL
#define RDX_MECHANISM_SUCCESS_SETTLE_MS        500UL
#define RDX_MECHANISM_MAX_RETURN_TIMEOUTS       3U
#define RDX_MECHANISM_CONTEXT_ADC_CHANNEL         4U
#define RDX_MECHANISM_CONTEXT_ADC_THRESHOLD     650U
#define RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT      8U

typedef enum _RDX_MECHANISM_STATE_T
{
    RDX_MECHANISM_IDLE = 0,
    /** Run GPIO3-low/PWM1 while checking endpoint and live media context. */
    RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA = 1,
    /** Keep the low-control drive active during the no-media grace timer. */
    RDX_MECHANISM_NO_MEDIA_GRACE = 2,
    /** Continue the low-control drive until endpoint or timeout. */
    RDX_MECHANISM_LOW_DRIVE_WAIT_ENDPOINT = 3,
    /** Stop the motor and settle before publishing successful completion. */
    RDX_MECHANISM_SUCCESS_SETTLE = 4,
    /** Stop the motor before attempting the recovery drive. */
    RDX_MECHANISM_RETURN_PAUSE = 5,
    /** Wait for the endpoint during the profile-dependent recovery drive. */
    RDX_MECHANISM_RECOVERY_DRIVE_WAIT_ENDPOINT = 6,
    /** Stop at the return endpoint until the existing drive deadline. */
    RDX_MECHANISM_RETURN_SETTLE = 7,
    /** Stop the motor and publish failure after the return timeout limit. */
    RDX_MECHANISM_RETRIES_EXHAUSTED = 8
} RDX_MECHANISM_STATE_T;

typedef struct _RDX_MECHANISM_CONTEXT_T
{
    volatile RDX_MECHANISM_STATE_T state;
    volatile UINT8_T return_drive_timeouts;
    BOOLEAN_T mechanism_deasserted_seen;
    volatile UINT16_T hardware_profile;
    volatile UINT32_T deadline;
} RDX_MECHANISM_CONTEXT_T;

static RDX_MECHANISM_CONTEXT_T rdx_mechanism;

typedef struct _RDX_MECHANISM_DIAGNOSTIC_EVENT_T
{
    volatile UINT32_T sequence;
    volatile UINT32_T timestamp;
    volatile UINT32_T payload;
} RDX_MECHANISM_DIAGNOSTIC_EVENT_T;

typedef struct _RDX_MECHANISM_DIAGNOSTICS_T
{
    volatile UINT32_T last_service;
    volatile UINT32_T max_service_gap;
    volatile UINT32_T phase_entered;
    volatile UINT32_T software_start_count;
    volatile UINT32_T boot_homing_count;
    volatile UINT32_T success_count;
    volatile UINT32_T failure_count;
    UINT32_T next_event;
    UINT32_T last_sequence;
    RDX_MECHANISM_DIAGNOSTIC_EVENT_T events[
        RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT];
} RDX_MECHANISM_DIAGNOSTICS_T;

static RDX_MECHANISM_DIAGNOSTICS_T rdx_mechanism_diagnostics;

/** Serialize one 16-bit value without depending on CPU byte order. */
static void rdx_mechanism_diagnostic_put_u16(UINT8_T *data, UINT16_T value)
{
    data[0] = (UINT8_T)value;
    data[1] = (UINT8_T)(value >> 8);
}

/** Serialize one 32-bit value without exposing structure padding. */
static void rdx_mechanism_diagnostic_put_u32(UINT8_T *data, UINT32_T value)
{
    data[0] = (UINT8_T)value;
    data[1] = (UINT8_T)(value >> 8);
    data[2] = (UINT8_T)(value >> 16);
    data[3] = (UINT8_T)(value >> 24);
}

/** Commit the phase and pin levels after a foreground output transition. */
static void rdx_mechanism_record_diagnostic_phase(UINT32_T now)
{
    RDX_MECHANISM_DIAGNOSTIC_EVENT_T *event;
    UINT32_T sequence;
    UINT32_T payload;

    event = &rdx_mechanism_diagnostics.events[
        rdx_mechanism_diagnostics.next_event];
    sequence = rdx_mechanism_diagnostics.last_sequence + 1U;
    if (sequence == 0U)
    {
        sequence = 1U;
    }
    payload = (UINT32_T)rdx_mechanism.state |
        ((READ32(GIOIN0_REG_OFF) & 0xFFU) << 8) |
        ((READ32(GIOOUT0_REG_OFF) & 0xFFU) << 16) |
        ((UINT32_T)rdx_mechanism.return_drive_timeouts << 24);

    /* A USB interrupt can preempt any store. Zero invalidates the slot until
     * both data words are ready; the reader never depends on next_event. */
    event->sequence = 0U;
    event->timestamp = now;
    event->payload = payload;
    event->sequence = sequence;
    rdx_mechanism_diagnostics.last_sequence = sequence;
    rdx_mechanism_diagnostics.next_event =
        (rdx_mechanism_diagnostics.next_event + 1U) %
        RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT;
}

/** Clear counters and phase history when the mechanism is initialized. */
static void rdx_mechanism_reset_diagnostics(UINT32_T now)
{
    UINT32_T index;

    rdx_mechanism_diagnostics.last_service = now;
    rdx_mechanism_diagnostics.max_service_gap = 0U;
    rdx_mechanism_diagnostics.phase_entered = now;
    rdx_mechanism_diagnostics.software_start_count = 0U;
    rdx_mechanism_diagnostics.boot_homing_count = 0U;
    rdx_mechanism_diagnostics.success_count = 0U;
    rdx_mechanism_diagnostics.failure_count = 0U;
    rdx_mechanism_diagnostics.next_event = 0U;
    rdx_mechanism_diagnostics.last_sequence = 0U;
    for (index = 0U; index < RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT; index++)
    {
        rdx_mechanism_diagnostics.events[index].sequence = 0U;
    }
    rdx_mechanism_record_diagnostic_phase(now);
}

/** Reset the service-gap measurement and count one admitted motion request. */
static void rdx_mechanism_begin_diagnostics(UINT32_T now, BOOLEAN_T boot_homing)
{
    rdx_mechanism_diagnostics.last_service = now;
    rdx_mechanism_diagnostics.max_service_gap = 0U;
    if (boot_homing)
    {
        rdx_mechanism_diagnostics.boot_homing_count++;
    }
    else
    {
        rdx_mechanism_diagnostics.software_start_count++;
    }
}

/** Retain the largest gap between foreground passes during one active cycle. */
static void rdx_mechanism_record_diagnostic_service(UINT32_T now)
{
    UINT32_T gap;

    if (rdx_mechanism.state == RDX_MECHANISM_IDLE)
    {
        return;
    }
    gap = now - rdx_mechanism_diagnostics.last_service;
    if (gap > rdx_mechanism_diagnostics.max_service_gap)
    {
        rdx_mechanism_diagnostics.max_service_gap = gap;
    }
    rdx_mechanism_diagnostics.last_service = now;
}

/** Serialize only complete ring entries, ordered by their commit sequence. */
static UINT8_T rdx_mechanism_read_diagnostic_events(UINT8_T *data)
{
    UINT32_T sequences[RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT];
    UINT32_T timestamps[RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT];
    UINT32_T payloads[RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT];
    UINT32_T count = 0U;
    UINT32_T index;
    UINT32_T position;
    UINT32_T sequence;
    UINT32_T timestamp;
    UINT32_T payload;
    RDX_MECHANISM_DIAGNOSTIC_EVENT_T *event;

    for (index = 0U; index < RDX_MECHANISM_DIAGNOSTIC_EVENT_COUNT; index++)
    {
        event = &rdx_mechanism_diagnostics.events[index];
        sequence = event->sequence;
        if (sequence == 0U)
        {
            continue;
        }
        timestamp = event->timestamp;
        payload = event->payload;
        if (event->sequence != sequence)
        {
            continue;
        }
        position = count;
        /* Live entries differ by at most eight commits, including wrap. */
        while ((position != 0U) &&
               ((INT32_T)(sequence - sequences[position - 1U]) < 0))
        {
            sequences[position] = sequences[position - 1U];
            timestamps[position] = timestamps[position - 1U];
            payloads[position] = payloads[position - 1U];
            position--;
        }
        sequences[position] = sequence;
        timestamps[position] = timestamp;
        payloads[position] = payload;
        count++;
    }
    for (index = 0U; index < count; index++)
    {
        rdx_mechanism_diagnostic_put_u32(&data[index * 8U], timestamps[index]);
        rdx_mechanism_diagnostic_put_u32(&data[index * 8U + 4U], payloads[index]);
    }
    return (UINT8_T)count;
}

/** Read fixed RAM diagnostics and non-destructive GPIO, RTI, and PWM registers. */
BOOLEAN_T rdx_mechanism_read_diagnostics(UINT8_T *data, UINT32_T length)
{
    UINT32_T index;

    if ((data == NULL) || (length != RDX_MECHANISM_DIAGNOSTIC_LENGTH))
    {
        return FALSE;
    }
    for (index = 0U; index < RDX_MECHANISM_DIAGNOSTIC_LENGTH; index++)
    {
        data[index] = 0U;
    }
    data[0] = 'R';
    data[1] = 'D';
    data[2] = 'X';
    data[3] = 'M';
    data[4] = 1U;
    data[5] = (UINT8_T)rdx_mechanism.state;
    data[6] = rdx_mechanism.return_drive_timeouts;
    rdx_mechanism_diagnostic_put_u16(&data[8], rdx_mechanism.hardware_profile);
    data[10] = (UINT8_T)READ32(GIOIN0_REG_OFF);
    data[11] = (UINT8_T)READ32(GIOOUT0_REG_OFF);
    rdx_mechanism_diagnostic_put_u32(&data[12], READ32(RTIFRC0_REG_OFF));
    rdx_mechanism_diagnostic_put_u32(&data[16], READ32(RTIFRC1_REG_OFF));
    rdx_mechanism_diagnostic_put_u32(&data[20], rdx_mechanism.deadline);
    rdx_mechanism_diagnostic_put_u32(&data[24],
        rdx_mechanism_diagnostics.last_service);
    rdx_mechanism_diagnostic_put_u32(&data[28],
        rdx_mechanism_diagnostics.max_service_gap);
    rdx_mechanism_diagnostic_put_u32(&data[32],
        rdx_mechanism_diagnostics.phase_entered);
    rdx_mechanism_diagnostic_put_u32(&data[36],
        rdx_mechanism_diagnostics.software_start_count);
    rdx_mechanism_diagnostic_put_u32(&data[40],
        rdx_mechanism_diagnostics.boot_homing_count);
    rdx_mechanism_diagnostic_put_u32(&data[44],
        rdx_mechanism_diagnostics.success_count);
    rdx_mechanism_diagnostic_put_u32(&data[48],
        rdx_mechanism_diagnostics.failure_count);
    rdx_mechanism_diagnostic_put_u32(&data[52],
        READ32(PWM_CFG_REG_OFF(RDX_MOTOR_PWM_NUM)));
    rdx_mechanism_diagnostic_put_u32(&data[56],
        READ32(PWM_PER_REG_OFF(RDX_MOTOR_PWM_NUM)));
    rdx_mechanism_diagnostic_put_u32(&data[60],
        READ32(PWM_PH1D_REG_OFF(RDX_MOTOR_PWM_NUM)));
    data[7] = rdx_mechanism_read_diagnostic_events(&data[64]);
    return TRUE;
}

/** Return TRUE when one wrap-safe absolute millisecond deadline has passed. */
static BOOLEAN_T rdx_mechanism_deadline_reached(UINT32_T now,
                                                UINT32_T deadline)
{
    return ((INT32_T)(now - deadline) >= 0) ? TRUE : FALSE;
}

/**
 * @brief Run PWM1 after selecting the required GPIO3 control level.
 *
 * Both initial and recovery drives use GPIO3 low. The
 * mandatory 50-microsecond dead time separates that transition from the
 * 100-percent, 50-microsecond-period PWM1 command.
 *
 * @param[in] control_high Requested GPIO3 level; powered phases select FALSE.
 */
static void rdx_mechanism_run_motor(BOOLEAN_T control_high)
{
    gio_rdx_motor_control_set_high(control_high);
    usleep(RDX_MECHANISM_DEAD_TIME_US);
    pwm_run(RDX_MOTOR_PWM_NUM, 100U, RDX_MOTOR_PWM_PERIOD_US);
}

/**
 * @brief Reset implemented mechanism drives to their idle pattern.
 *
 * Disable PWM1 and the mapped logical output 11 before the 50-microsecond
 * dead time and GPIO3-high idle level. Profile 38h assigns GPIO0 to logical
 * output 14 instead, so the profile helper preserves its asserted level.
 */
static void rdx_mechanism_stop_motor(void)
{
    pwm_disable(RDX_MOTOR_PWM_NUM);
    gio_rdx_profile_mechanism_auxiliary_set(FALSE);
    usleep(RDX_MECHANISM_DEAD_TIME_US);
    gio_rdx_motor_control_set_high(TRUE);
}

/**
 * @brief Seed the directional detector with GPIO2 before starting travel.
 *
 * Preserve an already-deasserted starting position if the switch asserts
 * before the first foreground service. An asserted starting position still
 * requires a later deassertion and assertion before it can complete travel.
 */
static void rdx_mechanism_reset_transition(void)
{
    rdx_mechanism.mechanism_deasserted_seen =
        !gio_rdx_mechanism_input_asserted() ? TRUE : FALSE;
}

/**
 * @brief Detect a complete directional transition on active-low GPIO2.
 *
 * A deasserted input must be observed before a later asserted input reports
 * completion. This prevents a powered phase from succeeding merely because
 * GPIO2 was already asserted when the phase began.
 *
 * @return TRUE only after the required deasserted-to-asserted transition.
 */
static BOOLEAN_T rdx_mechanism_transition_complete(void)
{
    BOOLEAN_T asserted;

    asserted = gio_rdx_mechanism_input_asserted();
    if (!asserted)
    {
        rdx_mechanism.mechanism_deasserted_seen = TRUE;
    }
    else if (rdx_mechanism.mechanism_deasserted_seen)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Evaluate the three independent media-context inputs.
 *
 * Media context is present when active-low GPIO5 reports a cartridge, the
 * SATA link has DET=3, or a live MCP3008 channel-4 sample exceeds 650.  This
 * ADC input is sampled live only when both digital inputs are absent and is
 * distinct from the insertion-time cached write-protect value. A failed ADC
 * transfer behaves as logical false; the shared frame wait is bounded so
 * sensor polling and motor deadlines can continue in foreground context.
 *
 * @return TRUE only when all three media-context inputs are absent.
 */
static BOOLEAN_T rdx_mechanism_media_context_absent(void)
{
    UINT16_T adc_sample;
    STATUS_T adc_status;
    BOOLEAN_T cartridge_present;
    BOOLEAN_T sata_link_active;

    cartridge_present = gio_rdx_cartridge_present();
    sata_link_active = ((READ32(PxSSTS(0)) & PSSTS_DET_MASK) ==
                        PSSTS_DET_PHY_READY) ? TRUE : FALSE;
    if (cartridge_present || sata_link_active)
    {
        return FALSE;
    }

    adc_status = rdx_mcp3008_read_channel(
        RDX_MECHANISM_CONTEXT_ADC_CHANNEL, &adc_sample);
    return (!cartridge_present &&
            ((adc_status != STATUS_OK) ||
             (adc_sample <= RDX_MECHANISM_CONTEXT_ADC_THRESHOLD))) ?
           TRUE : FALSE;
}

/**
 * @brief Apply one mechanism output/timer state.
 *
 * The initial low drive deasserts logical output 5 before running PWM1.
 * Its no-media continuation uses the same GPIO3-low/PWM1 sequence. The
 * recovery drive also selects GPIO3 low before its profile-specific output.
 * Pauses, settling, and terminal failure share the PWM-off/GPIO3-high
 * reset pattern, including deassertion of mapped logical output 11.
 *
 * @param[in] state mechanism output/timer phase to enter.
 * @param[in] now current free-running millisecond counter.
 */
static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,
                                      UINT32_T now)
{
    /* The RX-error ISR must see ownership before any output can power the
     * motor, so it defers blocking link recovery throughout motor travel. */
    rdx_mechanism.state = state;
    rdx_mechanism_diagnostics.phase_entered = now;
    switch (state)
    {
        case RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA:
            rdx_mechanism_reset_transition();
            gio_rdx_mechanism_auxiliary_set(FALSE);
            rdx_mechanism_run_motor(FALSE);
            rdx_mechanism.deadline = now + RDX_MECHANISM_DRIVE_TIME_MS;
            break;

        case RDX_MECHANISM_NO_MEDIA_GRACE:
            /* Keep the initial low drive active for the 50-ms grace timer. */
            rdx_mechanism.deadline = now + RDX_MECHANISM_NO_MEDIA_GRACE_MS;
            break;

        case RDX_MECHANISM_LOW_DRIVE_WAIT_ENDPOINT:
            rdx_mechanism_run_motor(FALSE);
            rdx_mechanism.deadline = now + RDX_MECHANISM_DRIVE_TIME_MS;
            break;

        case RDX_MECHANISM_SUCCESS_SETTLE:
            rdx_mechanism_stop_motor();
            rdx_mechanism.return_drive_timeouts = 0U;
            rdx_mechanism.deadline = now + RDX_MECHANISM_SUCCESS_SETTLE_MS;
            /* Successful travel selects the dock's amber output. */
            rdx_led_select_second(RDX_LED_DOCK, FALSE);
            break;

        case RDX_MECHANISM_RETURN_PAUSE:
            rdx_mechanism_stop_motor();
            rdx_mechanism.deadline =
                now + RDX_MECHANISM_INTERPHASE_TIME_MS;
            break;

        case RDX_MECHANISM_RECOVERY_DRIVE_WAIT_ENDPOINT:
            rdx_mechanism_reset_transition();
            /* Powered travel requires logical output 10 low. High is the
             * stopped control level, not a return-direction selector. */
            gio_rdx_motor_control_set_high(FALSE);
            usleep(RDX_MECHANISM_DEAD_TIME_US);
            /*
             * Profile 36h selects logical output 12, which is unassigned by
             * the board map and therefore produces no driven phase here.
             * Every other profile selects PWM1. Profiles 35h/37h additionally
             * assert logical output 11, mapped to GPIO0.
             */
            if (rdx_mechanism.hardware_profile != 0x36U)
            {
                if ((rdx_mechanism.hardware_profile == 0x35U) ||
                    (rdx_mechanism.hardware_profile == 0x37U))
                {
                    gio_rdx_profile_mechanism_auxiliary_set(TRUE);
                }
                pwm_run(RDX_MOTOR_PWM_NUM, 100U,
                        RDX_MOTOR_PWM_PERIOD_US);
            }
            rdx_mechanism.deadline = now + RDX_MECHANISM_DRIVE_TIME_MS;
            break;

        case RDX_MECHANISM_RETURN_SETTLE:
            rdx_mechanism_stop_motor();
            /* Preserve the recovery-drive deadline so the return window
             * remains one interval measured from the recovery-drive start. */
            break;

        case RDX_MECHANISM_RETRIES_EXHAUSTED:
            rdx_mechanism_stop_motor();
            rdx_mechanism.return_drive_timeouts = 0U;
            /* Exhausted retries select the dock's green output. */
            rdx_led_select_second(RDX_LED_DOCK, TRUE);
            break;

        default:
            rdx_mechanism_stop_motor();
            rdx_mechanism.state = RDX_MECHANISM_IDLE;
            break;
    }
    rdx_mechanism_record_diagnostic_phase(now);
}

/** Initialize state and place every implemented motor output at idle. */
void rdx_mechanism_init(UINT16_T hardware_profile)
{
    rdx_mechanism.hardware_profile = hardware_profile;
    rdx_mechanism_stop_motor();
    rdx_mechanism.state = RDX_MECHANISM_IDLE;
    rdx_mechanism.return_drive_timeouts = 0U;
    rdx_mechanism.mechanism_deasserted_seen = FALSE;
    rdx_mechanism.deadline = 0U;
    rdx_mechanism_reset_diagnostics(READ32(RTIFRC1_REG_OFF));
}

/** Start boot homing only after foreground servicing is available. */
BOOLEAN_T rdx_mechanism_start_boot_homing(UINT32_T now)
{
    if (!gio_rdx_mechanism_input_asserted())
    {
        rdx_mechanism_begin_diagnostics(now, TRUE);
        rdx_mechanism_enter_state(RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA, now);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Start the low-control drive for one accepted, prepared eject request.
 *
 * Media-context ADC input is deliberately sampled live by the initial drive
 * service instead of reusing the cached write-protect sample.
 *
 * @param[in] now current free-running millisecond counter.
 * @param[in] hardware_profile validated hardware profile code.
 */
void rdx_mechanism_start(UINT32_T now,
                         UINT16_T hardware_profile)
{
    rdx_mechanism.return_drive_timeouts = 0U;
    rdx_mechanism.hardware_profile = hardware_profile;
    rdx_mechanism_begin_diagnostics(now, FALSE);
    rdx_mechanism_enter_state(RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA, now);
}

/** Stop all implemented drives and return the mechanism state to idle. */
void rdx_mechanism_cancel(void)
{
    UINT32_T now;

    rdx_mechanism_stop_motor();
    rdx_mechanism.state = RDX_MECHANISM_IDLE;
    rdx_mechanism.return_drive_timeouts = 0U;
    rdx_mechanism.mechanism_deasserted_seen = FALSE;
    now = READ32(RTIFRC1_REG_OFF);
    rdx_mechanism_diagnostics.phase_entered = now;
    rdx_mechanism_record_diagnostic_phase(now);
}

/** Return TRUE while the mechanism state machine is active. */
BOOLEAN_T rdx_mechanism_is_active(void)
{
    return (rdx_mechanism.state != RDX_MECHANISM_IDLE) ? TRUE : FALSE;
}

/** Advance the mechanism state routing by one foreground pass. */
RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now)
{
    rdx_mechanism_record_diagnostic_service(now);

    switch (rdx_mechanism.state)
    {
        case RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_SUCCESS_SETTLE, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                /* Stop expired travel before any optional ADC wait or a
                 * newly absent medium can rearm the low-drive sequence. */
                rdx_mechanism_enter_state(RDX_MECHANISM_RETURN_PAUSE, now);
            }
            else if (rdx_mechanism_media_context_absent())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_NO_MEDIA_GRACE, now);
            }
            break;

        case RDX_MECHANISM_NO_MEDIA_GRACE:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_SUCCESS_SETTLE, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_LOW_DRIVE_WAIT_ENDPOINT, now);
            }
            break;

        case RDX_MECHANISM_LOW_DRIVE_WAIT_ENDPOINT:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_SUCCESS_SETTLE, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_RETURN_PAUSE, now);
            }
            break;

        case RDX_MECHANISM_SUCCESS_SETTLE:
            if (rdx_mechanism_deadline_reached(now,
                                                rdx_mechanism.deadline))
            {
                rdx_mechanism.state = RDX_MECHANISM_IDLE;
                rdx_mechanism_diagnostics.phase_entered = now;
                rdx_mechanism_diagnostics.success_count++;
                rdx_mechanism_record_diagnostic_phase(now);
                return RDX_MECHANISM_EVENT_SUCCEEDED;
            }
            break;

        case RDX_MECHANISM_RETURN_PAUSE:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_SUCCESS_SETTLE, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_RECOVERY_DRIVE_WAIT_ENDPOINT, now);
            }
            break;

        case RDX_MECHANISM_RECOVERY_DRIVE_WAIT_ENDPOINT:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_RETURN_SETTLE, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism.return_drive_timeouts++;
                if (rdx_mechanism.return_drive_timeouts >=
                    RDX_MECHANISM_MAX_RETURN_TIMEOUTS)
                {
                    rdx_mechanism_enter_state(RDX_MECHANISM_RETRIES_EXHAUSTED,
                                              now);
                    rdx_mechanism.state = RDX_MECHANISM_IDLE;
                    rdx_mechanism_diagnostics.phase_entered = now;
                    rdx_mechanism_diagnostics.failure_count++;
                    rdx_mechanism_record_diagnostic_phase(now);
                    return RDX_MECHANISM_EVENT_FAILED;
                }
                rdx_mechanism_enter_state(RDX_MECHANISM_RETURN_PAUSE, now);
            }
            break;

        case RDX_MECHANISM_RETURN_SETTLE:
            if (rdx_mechanism_deadline_reached(now,
                                                rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA, now);
            }
            break;

        case RDX_MECHANISM_RETRIES_EXHAUSTED:
            rdx_mechanism.state = RDX_MECHANISM_IDLE;
            rdx_mechanism_diagnostics.phase_entered = now;
            rdx_mechanism_diagnostics.failure_count++;
            rdx_mechanism_record_diagnostic_phase(now);
            return RDX_MECHANISM_EVENT_FAILED;

        default:
            break;
    }
    return RDX_MECHANISM_EVENT_NONE;
}
