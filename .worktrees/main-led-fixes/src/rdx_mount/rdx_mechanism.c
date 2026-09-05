/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
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
#define RDX_MECHANISM_STATE2_TIME_MS            50UL
#define RDX_MECHANISM_SUCCESS_SETTLE_MS        500UL
#define RDX_MECHANISM_MAX_REVERSE_EXPIRIES       3U
#define RDX_MECHANISM_CONTEXT_ADC_CHANNEL         4U
#define RDX_MECHANISM_CONTEXT_ADC_THRESHOLD     650U

typedef enum _RDX_MECHANISM_STATE_T
{
    RDX_MECHANISM_IDLE = 0,
    RDX_MECHANISM_STATE_1 = 1,
    RDX_MECHANISM_STATE_2 = 2,
    RDX_MECHANISM_STATE_3 = 3,
    RDX_MECHANISM_STATE_4 = 4,
    RDX_MECHANISM_STATE_5 = 5,
    RDX_MECHANISM_STATE_6 = 6,
    RDX_MECHANISM_STATE_7 = 7,
    RDX_MECHANISM_STATE_8 = 8
} RDX_MECHANISM_STATE_T;

typedef struct _RDX_MECHANISM_CONTEXT_T
{
    RDX_MECHANISM_STATE_T state;
    UINT8_T reverse_expiries;
    BOOLEAN_T mechanism_deasserted_seen;
    UINT16_T hardware_profile;
    UINT32_T deadline;
} RDX_MECHANISM_CONTEXT_T;

static RDX_MECHANISM_CONTEXT_T rdx_mechanism;

/** Return TRUE when one wrap-safe absolute millisecond deadline has passed. */
static BOOLEAN_T rdx_mechanism_deadline_reached(UINT32_T now,
                                                UINT32_T deadline)
{
    return ((INT32_T)(now - deadline) >= 0) ? TRUE : FALSE;
}

/**
 * @brief Run PWM1 after selecting the required GPIO3 control level.
 *
 * States 1/3 drive the GPIO3 control low and state 6 drives it high.  The
 * mandatory 50-microsecond dead time separates that transition from the
 * 100-percent, 50-microsecond-period PWM1 command.
 * Both GPIO3 levels are powered phases, so no simple active-low gate model is
 * valid for this state machine.
 *
 * @param[in] control_high TRUE for state 6, FALSE for states 1/3.
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
 * PWM1 is disabled first.  A 50-microsecond dead time then precedes the
 * GPIO3-high idle level.  Logical outputs 11 and 12 are unassigned by the
 * supported board map, so they do not claim GPIO0 or any substitute output.
 */
static void rdx_mechanism_stop_motor(void)
{
    pwm_disable(RDX_MOTOR_PWM_NUM);
    usleep(RDX_MECHANISM_DEAD_TIME_US);
    gio_rdx_motor_control_set_high(TRUE);
}

/** Reset the directional GPIO2 completion detector. */
static void rdx_mechanism_reset_transition(void)
{
    rdx_mechanism.mechanism_deasserted_seen = FALSE;
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
 * ADC input is intentionally sampled each time and is distinct from the
 * insertion-time cached write-protect value.  A failed ADC transfer behaves
 * as logical false; the call remains in foreground context because SPI may
 * wait for the controller.
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
    if (sata_link_active)
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
 * State 1 deasserts logical output 5 before sharing the GPIO3-low/PWM1
 * sequence with state 3.  State 6 is the distinct GPIO3-high/PWM1 phase.
 * States 4/5/7/8 share the PWM-off/GPIO3-high reset pattern.
 *
 * @param[in] state mechanism state 1 through 8.
 * @param[in] now current free-running millisecond counter.
 */
static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,
                                      UINT32_T now)
{
    switch (state)
    {
        case RDX_MECHANISM_STATE_1:
            rdx_mechanism_reset_transition();
            gio_rdx_mechanism_auxiliary_set(FALSE);
            rdx_mechanism_run_motor(FALSE);
            rdx_mechanism.deadline = now + RDX_MECHANISM_DRIVE_TIME_MS;
            break;

        case RDX_MECHANISM_STATE_2:
            /* State 2 leaves state 1's drive active for its 50-ms timer. */
            rdx_mechanism.deadline = now + RDX_MECHANISM_STATE2_TIME_MS;
            break;

        case RDX_MECHANISM_STATE_3:
            rdx_mechanism_run_motor(FALSE);
            rdx_mechanism.deadline = now + RDX_MECHANISM_DRIVE_TIME_MS;
            break;

        case RDX_MECHANISM_STATE_4:
            rdx_mechanism_stop_motor();
            rdx_mechanism.reverse_expiries = 0U;
            rdx_mechanism.deadline = now + RDX_MECHANISM_SUCCESS_SETTLE_MS;
            /* Successful travel selects the dock's amber output. */
            rdx_led_select_second(RDX_LED_DOCK, FALSE);
            break;

        case RDX_MECHANISM_STATE_5:
            rdx_mechanism_stop_motor();
            rdx_mechanism.deadline =
                now + RDX_MECHANISM_INTERPHASE_TIME_MS;
            break;

        case RDX_MECHANISM_STATE_6:
            rdx_mechanism_reset_transition();
            gio_rdx_motor_control_set_high(TRUE);
            usleep(RDX_MECHANISM_DEAD_TIME_US);
            /*
             * Profile 36h selects logical output 12, which is unassigned by
             * the board map and therefore produces no driven phase here.
             * Every other supported profile selects PWM1.  Logical output 11
             * is also unassigned; it must not be interpreted as GPIO0 merely
             * because its selector-table entry uses the zero sentinel.
             */
            if (rdx_mechanism.hardware_profile != 0x36U)
            {
                pwm_run(RDX_MOTOR_PWM_NUM, 100U,
                        RDX_MOTOR_PWM_PERIOD_US);
            }
            rdx_mechanism.deadline = now + RDX_MECHANISM_DRIVE_TIME_MS;
            break;

        case RDX_MECHANISM_STATE_7:
            rdx_mechanism_stop_motor();
            /* Preserve state 6's deadline so the return window remains one
             * continuous two-second interval after its endpoint is reached. */
            break;

        case RDX_MECHANISM_STATE_8:
            rdx_mechanism_stop_motor();
            rdx_mechanism.reverse_expiries = 0U;
            /* Exhausted retries select the dock's green output. */
            rdx_led_select_second(RDX_LED_DOCK, TRUE);
            break;

        default:
            rdx_mechanism_stop_motor();
            state = RDX_MECHANISM_IDLE;
            break;
    }
    rdx_mechanism.state = state;
}

/** Initialize state and place every implemented motor output at idle. */
void rdx_mechanism_init(UINT16_T hardware_profile)
{
    rdx_mechanism.hardware_profile = hardware_profile;
    rdx_mechanism_stop_motor();
    rdx_mechanism.state = RDX_MECHANISM_IDLE;
    rdx_mechanism.reverse_expiries = 0U;
    rdx_mechanism.mechanism_deasserted_seen = FALSE;
    rdx_mechanism.deadline = 0U;
}

/** Start boot homing only after foreground servicing is available. */
BOOLEAN_T rdx_mechanism_start_boot_homing(UINT32_T now)
{
    if (!gio_rdx_mechanism_input_asserted())
    {
        rdx_mechanism_enter_state(RDX_MECHANISM_STATE_1, now);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Enter state 1 for one accepted, prepared eject request.
 *
 * Media-context ADC input is deliberately sampled live later by the state-1
 * service instead of reusing the cached write-protect sample.
 *
 * @param[in] now current free-running millisecond counter.
 * @param[in] hardware_profile validated hardware profile code.
 */
void rdx_mechanism_start(UINT32_T now,
                         UINT16_T hardware_profile)
{
    rdx_mechanism.reverse_expiries = 0U;
    rdx_mechanism.hardware_profile = hardware_profile;
    rdx_mechanism_enter_state(RDX_MECHANISM_STATE_1, now);
}

/** Stop all implemented drives and return the mechanism state to idle. */
void rdx_mechanism_cancel(void)
{
    rdx_mechanism_stop_motor();
    rdx_mechanism.state = RDX_MECHANISM_IDLE;
    rdx_mechanism.reverse_expiries = 0U;
    rdx_mechanism.mechanism_deasserted_seen = FALSE;
}

/** Return TRUE while the mechanism state machine is active. */
BOOLEAN_T rdx_mechanism_is_active(void)
{
    return (rdx_mechanism.state != RDX_MECHANISM_IDLE) ? TRUE : FALSE;
}

/** Advance the mechanism state routing by one foreground pass. */
RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now)
{
    switch (rdx_mechanism.state)
    {
        case RDX_MECHANISM_STATE_1:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_4, now);
            }
            else if (rdx_mechanism_media_context_absent())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_2, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_5, now);
            }
            break;

        case RDX_MECHANISM_STATE_2:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_4, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_3, now);
            }
            break;

        case RDX_MECHANISM_STATE_3:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_4, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_5, now);
            }
            break;

        case RDX_MECHANISM_STATE_4:
            if (rdx_mechanism_deadline_reached(now,
                                                rdx_mechanism.deadline))
            {
                rdx_mechanism.state = RDX_MECHANISM_IDLE;
                return RDX_MECHANISM_EVENT_SUCCEEDED;
            }
            break;

        case RDX_MECHANISM_STATE_5:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_4, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_6, now);
            }
            break;

        case RDX_MECHANISM_STATE_6:
            if (rdx_mechanism_transition_complete())
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_7, now);
            }
            else if (rdx_mechanism_deadline_reached(now,
                                                     rdx_mechanism.deadline))
            {
                rdx_mechanism.reverse_expiries++;
                if (rdx_mechanism.reverse_expiries >=
                    RDX_MECHANISM_MAX_REVERSE_EXPIRIES)
                {
                    rdx_mechanism_enter_state(RDX_MECHANISM_STATE_8,
                                              now);
                    rdx_mechanism.state = RDX_MECHANISM_IDLE;
                    return RDX_MECHANISM_EVENT_FAILED;
                }
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_5, now);
            }
            break;

        case RDX_MECHANISM_STATE_7:
            if (rdx_mechanism_deadline_reached(now,
                                                rdx_mechanism.deadline))
            {
                rdx_mechanism_enter_state(RDX_MECHANISM_STATE_1, now);
            }
            break;

        case RDX_MECHANISM_STATE_8:
            rdx_mechanism.state = RDX_MECHANISM_IDLE;
            return RDX_MECHANISM_EVENT_FAILED;

        default:
            break;
    }
    return RDX_MECHANISM_EVENT_NONE;
}
