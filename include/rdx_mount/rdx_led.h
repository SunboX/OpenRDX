/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_led.h
//
// Description : OpenRDX bicolor LED controllers.
//=======================================================================================

#ifndef _RDX_LED_H_
#define _RDX_LED_H_

#include "tusb9260_types.h"

/** RDX bicolor LED controller selected by the runtime. */
typedef enum _RDX_LED_TARGET_T
{
    RDX_LED_DOCK = 0,
    RDX_LED_CARTRIDGE = 1,
    RDX_LED_TARGET_COUNT = 2
} RDX_LED_TARGET_T;

/**
 * Initialize both LED controllers and permanently claim their active-low pins.
 *
 * All four physical outputs are driven to their safe, deasserted level before
 * their output directions and GPIO functions are selected.
 */
void rdx_led_init(void);

/** Advance both LED controllers from the 100-ms firmware service. */
void rdx_led_tick(void);

/**
 * Select the controller's second physical output at the next service.
 * Selecting the second output also enables the stable indication. Selecting
 * the first leaves the steady-enable state unchanged.
 * The first/second colors are intentionally controller-specific: dock uses
 * amber/green while cartridge uses green/amber.
 *
 * @param[in] target LED controller to update.
 * @param[in] selected TRUE selects the second output; FALSE selects the first.
 */
void rdx_led_select_second(RDX_LED_TARGET_T target, BOOLEAN_T selected);

/**
 * Set whether the controller is illuminated in its normal stable state.
 *
 * @param[in] target LED controller to update.
 * @param[in] steady TRUE enables the stable indication; FALSE turns it off.
 */
void rdx_led_set_steady(RDX_LED_TARGET_T target, BOOLEAN_T steady);

/**
 * Start the six-phase normal-color blink sequence.
 *
 * @param[in] target LED controller to update.
 */
void rdx_led_start_blink(RDX_LED_TARGET_T target);

/**
 * Start the eight-phase blink forced to the second physical output.
 *
 * @param[in] target LED controller to update.
 */
void rdx_led_start_second_blink(RDX_LED_TARGET_T target);

/**
 * Start the mode-two 20-phase blink sequence.
 *
 * The shared scheduler switches to 100 ms only when the dock controller is in
 * mode two. A cartridge-only mode-two request therefore continues at the
 * shared 500-ms cadence.
 *
 * @param[in] target LED controller to update.
 */
void rdx_led_start_fast_blink(RDX_LED_TARGET_T target);

/**
 * Report whether one controller is still running mode-two confirmation.
 *
 * @param[in] target LED controller to query.
 * @return TRUE while the 20-phase fast blink is active, otherwise FALSE.
 */
BOOLEAN_T rdx_led_fast_blink_is_active(RDX_LED_TARGET_T target);

/**
 * @brief Start or restart the no-cartridge selection display.
 *
 * Every 500-ms service produces a second-output off/on/off preamble followed
 * by one through seven first-output pulses.  This is the exact visual menu
 * encoded by the menu cycle states.
 *
 * @param[in] target LED controller used for the menu; pass the dock target.
 * @param[in] selection zero-based callback-table selection from 0 through 6.
 */
void rdx_led_show_gesture_selection(RDX_LED_TARGET_T target,
                                    UINT8_T selection);

/**
 * @brief Queue a gesture confirmation on the next ordinary LED service.
 *
 * The selected output is then toggled for twenty 100-ms half-phases.  Callback
 * readiness is not published until the following 500-ms service, matching
 * the mode-two completion transition and delayed dispatch.
 *
 * @param[in] target LED controller to confirm; pass the dock target.
 */
void rdx_led_confirm_gesture(RDX_LED_TARGET_T target);

/**
 * @brief Cancel an unconfirmed gesture display.
 *
 * @param[in] target LED controller whose normal stable behavior is restored.
 */
void rdx_led_cancel_gesture(RDX_LED_TARGET_T target);

/**
 * @brief Consume the post-confirmation callback point for one controller.
 *
 * @param[in] target LED controller to query.
 * @return TRUE exactly once after fast confirmation and its final normal
 *         service have completed, otherwise FALSE.
 */
BOOLEAN_T rdx_led_take_gesture_confirmation(RDX_LED_TARGET_T target);

/**
 * Select diagnostic mode-one operation for both controllers.
 *
 * The controller state is intentionally preserved so this operation has the
 * same mode-only semantics as the Manager diagnostic command.
 *
 * @param[in] active TRUE selects diagnostic mode; FALSE selects normal mode.
 */
void rdx_led_set_diagnostic(BOOLEAN_T active);

/**
 * Report whether both LED controllers are in diagnostic mode.
 *
 * @return TRUE when the paired Manager diagnostic is active.
 */
BOOLEAN_T rdx_led_diagnostic_is_active(void);

#endif /* _RDX_LED_H_ */
