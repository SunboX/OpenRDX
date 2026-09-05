/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_led.c
//
// Description : OpenRDX bicolor LED controllers.
//=======================================================================================

#include "rdx_led.h"

#include "gio.h"
#include "reg_io.h"
#include "sci.h"

#define RDX_LED_MODE_NORMAL                      0U
#define RDX_LED_MODE_DIAGNOSTIC                  1U
#define RDX_LED_MODE_FAST                        2U

#define RDX_LED_STATE_FIRST                      0U
#define RDX_LED_STATE_SECOND                     1U
#define RDX_LED_DIAGNOSTIC_SECOND_PENDING_STATE  3U
#define RDX_LED_DIAGNOSTIC_FIRST_PENDING_STATE   2U

#define RDX_LED_NORMAL_SERVICE_TICKS             5U
#define RDX_LED_NORMAL_BLINK_PHASES               6U
#define RDX_LED_FORCED_SECOND_PHASES              8U
#define RDX_LED_FAST_BLINK_PHASES                20U

#define RDX_LED_GESTURE_NONE                       0U
#define RDX_LED_GESTURE_SECOND_OFF                 1U
#define RDX_LED_GESTURE_SECOND_ON                  2U
#define RDX_LED_GESTURE_SECOND_OFF_AFTER           3U
#define RDX_LED_GESTURE_FIRST_SETUP                4U
#define RDX_LED_GESTURE_FIRST_BLINK                5U

/* All four front-panel channels are active-low. The board wiring defines
 * the otherwise-generic pair order: SCI GPIO8/9 are dock amber/green, while
 * standard GPIO7/6 are cartridge green/amber. Keeping color identity at this
 * boundary lets the state machine remain a generic first/second controller. */
#define RDX_LED_DOCK_FIRST_MASK                   SCI_PIO_RX_GPIO8
#define RDX_LED_DOCK_SECOND_MASK                  SCI_PIO_TX_GPIO9
#define RDX_LED_DOCK_MASK \
    (RDX_LED_DOCK_FIRST_MASK | RDX_LED_DOCK_SECOND_MASK)
#define RDX_LED_CARTRIDGE_FIRST_MASK \
    (1UL << RDX_CARTRIDGE_GREEN_GIO_NUM)
#define RDX_LED_CARTRIDGE_SECOND_MASK \
    (1UL << RDX_CARTRIDGE_AMBER_GIO_NUM)
#define RDX_LED_CARTRIDGE_MASK \
    (RDX_LED_CARTRIDGE_FIRST_MASK | RDX_LED_CARTRIDGE_SECOND_MASK)

typedef struct _RDX_LED_CONTROLLER_T
{
    UINT8_T state;
    BOOLEAN_T phase;
    BOOLEAN_T second_selected;
    UINT8_T mode;
    BOOLEAN_T steady;
    UINT8_T blink_phases;
    UINT8_T forced_second_phases;
    UINT8_T gesture_state;
    UINT8_T gesture_count;
    BOOLEAN_T gesture_confirmation_requested;
    BOOLEAN_T gesture_fast_active;
    BOOLEAN_T gesture_callback_waiting;
    BOOLEAN_T gesture_callback_ready;
} RDX_LED_CONTROLLER_T;

static RDX_LED_CONTROLLER_T rdx_led_controllers[RDX_LED_TARGET_COUNT];
static UINT8_T rdx_led_shared_cadence_ticks;

/** Return TRUE when a caller supplied a valid controller selector. */
static BOOLEAN_T rdx_led_target_is_valid(RDX_LED_TARGET_T target)
{
    return (UINT32_T)target < (UINT32_T)RDX_LED_TARGET_COUNT;
}

/**
 * @brief Claim all four active-low LED pins after driving their safe level.
 *
 * An asserted front-panel channel drives low. Preloading high therefore
 * prevents a visible flash while GPIO direction and SCI ownership change.
 */
static void rdx_led_claim_pins(void)
{
    /* Preload off before changing direction or peripheral ownership. */
    MODIFY_REG32(GIOOUT0_REG_OFF, RDX_LED_CARTRIDGE_MASK,
                 RDX_LED_CARTRIDGE_MASK);
    MODIFY_REG32(SCI_PIO3, RDX_LED_DOCK_MASK, RDX_LED_DOCK_MASK);

    /* LED ownership is permanent after initialization; neither SCI channel
     * returns to its alternate debug function during normal operation. */
    MODIFY_REG32(GIOPULDIS0_REG_OFF, RDX_LED_CARTRIDGE_MASK,
                 RDX_LED_CARTRIDGE_MASK);
    MODIFY_REG32(SCI_PIO7, RDX_LED_DOCK_MASK, RDX_LED_DOCK_MASK);
    MODIFY_REG32(GIODIR0_REG_OFF, RDX_LED_CARTRIDGE_MASK,
                 RDX_LED_CARTRIDGE_MASK);
    MODIFY_REG32(SCI_PIO1, RDX_LED_DOCK_MASK, RDX_LED_DOCK_MASK);
    MODIFY_REG32(SCI_PIO0, RDX_LED_DOCK_MASK, 0U);
}

/**
 * @brief Drive one controller's phase through the configured physical mapping.
 *
 * State zero asserts the controller's first output and state one asserts its
 * second output. Board wiring is used only to name those generic
 * outputs: dock first/second are amber/green, while cartridge first/second
 * are green/amber.
 */
static void rdx_led_apply_output(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;
    UINT32_T output;

    controller = &rdx_led_controllers[(UINT32_T)target];
    if (target == RDX_LED_DOCK)
    {
        output = RDX_LED_DOCK_MASK;
        if (controller->phase)
        {
            if (controller->state == RDX_LED_STATE_FIRST)
            {
                output &= ~RDX_LED_DOCK_FIRST_MASK;
            }
            else
            {
                output &= ~RDX_LED_DOCK_SECOND_MASK;
            }
        }
        MODIFY_REG32(SCI_PIO3, RDX_LED_DOCK_MASK, output);
    }
    else
    {
        output = RDX_LED_CARTRIDGE_MASK;
        if (controller->phase)
        {
            if (controller->state == RDX_LED_STATE_FIRST)
            {
                output &= ~RDX_LED_CARTRIDGE_FIRST_MASK;
            }
            else
            {
                output &= ~RDX_LED_CARTRIDGE_SECOND_MASK;
            }
        }
        MODIFY_REG32(GIOOUT0_REG_OFF, RDX_LED_CARTRIDGE_MASK, output);
    }
}

/**
 * @brief Initialize one controller to its first-output/off state.
 *
 * Every controller begins with state zero, phase zero, mode zero, no pending
 * transient, and no stable illumination. This explicit initialization keeps
 * reset behavior independent of static-storage assumptions.
 */
static void rdx_led_initialize_controller(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    controller = &rdx_led_controllers[(UINT32_T)target];
    controller->state = RDX_LED_STATE_FIRST;
    controller->phase = FALSE;
    controller->second_selected = FALSE;
    controller->mode = RDX_LED_MODE_NORMAL;
    controller->steady = FALSE;
    controller->blink_phases = 0U;
    controller->forced_second_phases = 0U;
    controller->gesture_state = RDX_LED_GESTURE_NONE;
    controller->gesture_count = 0U;
    controller->gesture_confirmation_requested = FALSE;
    controller->gesture_fast_active = FALSE;
    controller->gesture_callback_waiting = FALSE;
    controller->gesture_callback_ready = FALSE;
    rdx_led_apply_output(target);
}

/**
 * @brief Render one menu-selection cycle for the no-cartridge button gesture.
 *
 * The sequence is a second-output off/on/off preamble, a first-output-off
 * setup, then `(selection + 1) * 2` half-phases. The cycle state advances
 * while its final pulse is rendered, so the following service must draw the
 * next second-output-off preamble without another first-output-off frame.
 */
static void rdx_led_advance_gesture(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    controller = &rdx_led_controllers[(UINT32_T)target];
    switch (controller->gesture_state)
    {
        case RDX_LED_GESTURE_SECOND_OFF:
            controller->state = RDX_LED_STATE_SECOND;
            controller->phase = FALSE;
            controller->gesture_state = RDX_LED_GESTURE_SECOND_ON;
            break;

        case RDX_LED_GESTURE_SECOND_ON:
            controller->state = RDX_LED_STATE_SECOND;
            controller->phase = TRUE;
            controller->gesture_state =
                RDX_LED_GESTURE_SECOND_OFF_AFTER;
            break;

        case RDX_LED_GESTURE_SECOND_OFF_AFTER:
            controller->state = RDX_LED_STATE_SECOND;
            controller->phase = FALSE;
            controller->gesture_state = RDX_LED_GESTURE_FIRST_SETUP;
            break;

        case RDX_LED_GESTURE_FIRST_SETUP:
            controller->state = RDX_LED_STATE_FIRST;
            controller->phase = FALSE;
            controller->blink_phases =
                (UINT8_T)((controller->gesture_count + 1U) * 2U);
            controller->gesture_state = RDX_LED_GESTURE_FIRST_BLINK;
            break;

        case RDX_LED_GESTURE_FIRST_BLINK:
            if (controller->blink_phases != 0U)
            {
                controller->blink_phases--;
                controller->phase = !controller->phase;
                controller->state = RDX_LED_STATE_FIRST;
            }
            else
            {
                /* The final count transition already advanced the cycle.
                 * Render the next second-output-off preamble now and continue
                 * with its second-output-on successor on the next service. */
                controller->state = RDX_LED_STATE_SECOND;
                controller->phase = FALSE;
                controller->gesture_state = RDX_LED_GESTURE_SECOND_ON;
            }
            break;

        default:
            /* Corrupt private state fails safely into the start of a cycle. */
            controller->state = RDX_LED_STATE_FIRST;
            controller->phase = FALSE;
            controller->gesture_state = RDX_LED_GESTURE_SECOND_OFF;
            break;
    }
    rdx_led_apply_output(target);
}

/**
 * @brief Start the delayed 20-half-phase confirmation sequence.
 *
 * Confirmation does not enter fast mode immediately. The next ordinary
 * controller service loads twenty phases and mode two without changing the
 * current output. Deferring the request preserves that boundary and whichever
 * menu channel was visible when the user confirmed the selection.
 */
static BOOLEAN_T rdx_led_begin_gesture_confirmation(
    RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    controller = &rdx_led_controllers[(UINT32_T)target];
    if (!controller->gesture_confirmation_requested ||
        (controller->mode == RDX_LED_MODE_DIAGNOSTIC))
    {
        return FALSE;
    }

    controller->gesture_confirmation_requested = FALSE;
    controller->gesture_state = RDX_LED_GESTURE_NONE;
    controller->forced_second_phases = 0U;
    controller->blink_phases = RDX_LED_FAST_BLINK_PHASES;
    controller->mode = RDX_LED_MODE_FAST;
    controller->gesture_fast_active = TRUE;
    controller->gesture_callback_waiting = FALSE;
    controller->gesture_callback_ready = FALSE;
    return TRUE;
}

/** Advance one normal-mode controller phase. */
static void rdx_led_advance_normal(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    controller = &rdx_led_controllers[(UINT32_T)target];
    if (controller->forced_second_phases != 0U)
    {
        controller->forced_second_phases--;
        controller->phase = !controller->phase;
        controller->state = RDX_LED_STATE_SECOND;
    }
    else if (controller->blink_phases != 0U)
    {
        controller->blink_phases--;
        controller->phase = !controller->phase;
        controller->state = controller->second_selected ?
            RDX_LED_STATE_SECOND : RDX_LED_STATE_FIRST;
    }
    else
    {
        controller->phase = controller->steady;
        controller->state = controller->second_selected ?
            RDX_LED_STATE_SECOND : RDX_LED_STATE_FIRST;
    }

    rdx_led_apply_output(target);
}

/** Advance diagnostic mode one through off, second, off, and first output. */
static void rdx_led_advance_diagnostic(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    controller = &rdx_led_controllers[(UINT32_T)target];
    if (controller->state == RDX_LED_STATE_FIRST)
    {
        controller->state = RDX_LED_DIAGNOSTIC_SECOND_PENDING_STATE;
        controller->phase = FALSE;
    }
    else if (controller->state == RDX_LED_DIAGNOSTIC_SECOND_PENDING_STATE)
    {
        controller->state = RDX_LED_STATE_SECOND;
        controller->phase = TRUE;
    }
    else if (controller->state == RDX_LED_STATE_SECOND)
    {
        controller->state = RDX_LED_DIAGNOSTIC_FIRST_PENDING_STATE;
        controller->phase = FALSE;
    }
    else
    {
        controller->state = RDX_LED_STATE_FIRST;
        controller->phase = TRUE;
    }
    rdx_led_apply_output(target);
}

/**
 * @brief Advance one controller at the already-selected shared cadence.
 *
 * Mode two remains set through the twentieth half-phase. The following
 * 100-ms service restores mode zero without redrawing the output. Callback
 * readiness is published only on the next normal controller service, so
 * `gesture_callback_waiting` deliberately survives another 500-ms interval.
 */
static void rdx_led_service_controller(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    controller = &rdx_led_controllers[(UINT32_T)target];
    if (rdx_led_begin_gesture_confirmation(target))
    {
        return;
    }

    if (controller->mode == RDX_LED_MODE_FAST)
    {
        if (controller->blink_phases != 0U)
        {
            controller->blink_phases--;
            controller->phase = !controller->phase;
            rdx_led_apply_output(target);
        }
        else
        {
            controller->mode = RDX_LED_MODE_NORMAL;
            if (controller->gesture_fast_active)
            {
                controller->gesture_fast_active = FALSE;
                controller->gesture_callback_waiting = TRUE;
            }
        }
        return;
    }

    if (controller->gesture_callback_waiting)
    {
        controller->gesture_callback_waiting = FALSE;
        controller->gesture_callback_ready = TRUE;
    }

    if (controller->mode == RDX_LED_MODE_DIAGNOSTIC)
    {
        rdx_led_advance_diagnostic(target);
    }
    else if (controller->gesture_state != RDX_LED_GESTURE_NONE)
    {
        rdx_led_advance_gesture(target);
    }
    else
    {
        rdx_led_advance_normal(target);
    }
}

/** Initialize and safely claim the two OpenRDX LED controllers. */
void rdx_led_init(void)
{
    RDX_LED_TARGET_T target;

    rdx_led_claim_pins();
    for (target = RDX_LED_DOCK; target < RDX_LED_TARGET_COUNT;
         target = (RDX_LED_TARGET_T)((UINT32_T)target + 1U))
    {
        rdx_led_initialize_controller(target);
    }
    rdx_led_shared_cadence_ticks = 0U;
}

/**
 * @brief Advance both LED controllers from the 100-ms firmware service.
 *
 * One timer services both controllers. It reloads for 100 ms only while the
 * dock controller is in mode two; otherwise it reloads for 500 ms. A dock
 * confirmation therefore advances the cartridge controller at 100 ms for the
 * same interval, even if the cartridge controller remains in normal mode.
 */
void rdx_led_tick(void)
{
    if (rdx_led_controllers[RDX_LED_DOCK].mode == RDX_LED_MODE_FAST)
    {
        rdx_led_shared_cadence_ticks = 0U;
        rdx_led_service_controller(RDX_LED_DOCK);
        rdx_led_service_controller(RDX_LED_CARTRIDGE);
        return;
    }

    rdx_led_shared_cadence_ticks++;
    if (rdx_led_shared_cadence_ticks < RDX_LED_NORMAL_SERVICE_TICKS)
    {
        return;
    }
    rdx_led_shared_cadence_ticks = 0U;
    rdx_led_service_controller(RDX_LED_DOCK);
    rdx_led_service_controller(RDX_LED_CARTRIDGE);
}

/** Select the first or second output for one controller. */
void rdx_led_select_second(RDX_LED_TARGET_T target, BOOLEAN_T selected)
{
    if (!rdx_led_target_is_valid(target))
    {
        return;
    }
    if (selected)
    {
        rdx_led_controllers[(UINT32_T)target].steady = TRUE;
    }
    rdx_led_controllers[(UINT32_T)target].second_selected = selected != FALSE;
}

/** Enable or disable the stable indication for one controller. */
void rdx_led_set_steady(RDX_LED_TARGET_T target, BOOLEAN_T steady)
{
    if (!rdx_led_target_is_valid(target))
    {
        return;
    }
    rdx_led_controllers[(UINT32_T)target].steady = steady != FALSE;
}

/** Start the six-phase normal-color blink sequence for one controller. */
void rdx_led_start_blink(RDX_LED_TARGET_T target)
{
    if (!rdx_led_target_is_valid(target))
    {
        return;
    }
    rdx_led_controllers[(UINT32_T)target].steady = TRUE;
    rdx_led_controllers[(UINT32_T)target].blink_phases =
        RDX_LED_NORMAL_BLINK_PHASES;
}

/** Start the eight-phase forced-second blink sequence for one controller. */
void rdx_led_start_second_blink(RDX_LED_TARGET_T target)
{
    if (!rdx_led_target_is_valid(target))
    {
        return;
    }
    rdx_led_controllers[(UINT32_T)target].steady = TRUE;
    rdx_led_controllers[(UINT32_T)target].forced_second_phases =
        RDX_LED_FORCED_SECOND_PHASES;
}

/** Start a 20-phase mode-two blink at the dock-driven shared cadence. */
void rdx_led_start_fast_blink(RDX_LED_TARGET_T target)
{
    if (!rdx_led_target_is_valid(target))
    {
        return;
    }
    rdx_led_controllers[(UINT32_T)target].steady = TRUE;
    rdx_led_controllers[(UINT32_T)target].blink_phases =
        RDX_LED_FAST_BLINK_PHASES;
    rdx_led_controllers[(UINT32_T)target].mode = RDX_LED_MODE_FAST;
    rdx_led_controllers[(UINT32_T)target].gesture_fast_active = FALSE;
}

/** Return TRUE while one controller runs its mode-two confirmation blink. */
BOOLEAN_T rdx_led_fast_blink_is_active(RDX_LED_TARGET_T target)
{
    if (!rdx_led_target_is_valid(target))
    {
        return FALSE;
    }
    return ((rdx_led_controllers[(UINT32_T)target].mode ==
             RDX_LED_MODE_FAST) ||
            rdx_led_controllers[(UINT32_T)target].
                gesture_confirmation_requested ||
            rdx_led_controllers[(UINT32_T)target].
                gesture_callback_waiting) ? TRUE : FALSE;
}

/** Start or restart the no-cartridge selection display. */
void rdx_led_show_gesture_selection(RDX_LED_TARGET_T target,
                                    UINT8_T selection)
{
    if (!rdx_led_target_is_valid(target) || (selection > 6U))
    {
        return;
    }

    rdx_led_controllers[(UINT32_T)target].gesture_count = selection;
    rdx_led_controllers[(UINT32_T)target].gesture_state =
        RDX_LED_GESTURE_SECOND_OFF;
    rdx_led_controllers[(UINT32_T)target].gesture_confirmation_requested =
        FALSE;
    rdx_led_controllers[(UINT32_T)target].gesture_callback_waiting = FALSE;
    rdx_led_controllers[(UINT32_T)target].gesture_callback_ready = FALSE;
}

/** Queue selection confirmation for the next normal service. */
void rdx_led_confirm_gesture(RDX_LED_TARGET_T target)
{
    if (!rdx_led_target_is_valid(target))
    {
        return;
    }

    rdx_led_controllers[(UINT32_T)target].gesture_confirmation_requested =
        TRUE;
    rdx_led_controllers[(UINT32_T)target].gesture_callback_ready = FALSE;
}

/** Cancel an unconfirmed gesture display and restore normal control. */
void rdx_led_cancel_gesture(RDX_LED_TARGET_T target)
{
    RDX_LED_CONTROLLER_T *controller;

    if (!rdx_led_target_is_valid(target))
    {
        return;
    }

    controller = &rdx_led_controllers[(UINT32_T)target];
    controller->gesture_state = RDX_LED_GESTURE_NONE;
    controller->gesture_confirmation_requested = FALSE;
    if (controller->gesture_fast_active)
    {
        controller->mode = RDX_LED_MODE_NORMAL;
        controller->blink_phases = 0U;
    }
    controller->gesture_fast_active = FALSE;
    controller->gesture_callback_waiting = FALSE;
    controller->gesture_callback_ready = FALSE;
}

/** Consume the callback point reached after gesture confirmation. */
BOOLEAN_T rdx_led_take_gesture_confirmation(RDX_LED_TARGET_T target)
{
    BOOLEAN_T ready;

    if (!rdx_led_target_is_valid(target))
    {
        return FALSE;
    }

    ready = rdx_led_controllers[(UINT32_T)target].gesture_callback_ready;
    rdx_led_controllers[(UINT32_T)target].gesture_callback_ready = FALSE;
    return ready;
}

/** Select or clear diagnostic mode one for both paired LED controllers. */
void rdx_led_set_diagnostic(BOOLEAN_T active)
{
    UINT8_T mode;

    mode = active ? RDX_LED_MODE_DIAGNOSTIC : RDX_LED_MODE_NORMAL;
    rdx_led_controllers[RDX_LED_DOCK].mode = mode;
    rdx_led_controllers[RDX_LED_CARTRIDGE].mode = mode;
}

/** Return TRUE while both paired controllers are in diagnostic mode one. */
BOOLEAN_T rdx_led_diagnostic_is_active(void)
{
    return (rdx_led_controllers[RDX_LED_DOCK].mode ==
            RDX_LED_MODE_DIAGNOSTIC) &&
           (rdx_led_controllers[RDX_LED_CARTRIDGE].mode ==
            RDX_LED_MODE_DIAGNOSTIC);
}
