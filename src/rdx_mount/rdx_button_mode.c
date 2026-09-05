/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_button_mode.c
//
// Description : OpenRDX no-cartridge eject-button mode gesture.
//=======================================================================================

/*! @file
 * @brief Hidden OpenRDX eject-button mode-selection state machine.
 */

#include "rdx_button_mode.h"

#include "rdx_led.h"
#include "rdx_manager_protocol.h"
#include "reg_io.h"
#include "rti.h"
#include "scsi.h"
#include "system.h"
#include "usb_hal.h"

#define RDX_BUTTON_ENTRY_HOLD_MS                 5000UL
#define RDX_BUTTON_CLICK_WINDOW_MS               1000UL
#define RDX_BUTTON_SELECTION_HOLD_MS             5000UL
#define RDX_BUTTON_SESSION_TIMEOUT_MS           60000UL
#define RDX_BUTTON_RECONNECT_DETACH_MS            1000UL
#define RDX_BUTTON_RECONNECT_SETTLE_MS            2000UL

typedef enum _RDX_BUTTON_GESTURE_STATE_T
{
    RDX_BUTTON_GESTURE_IDLE = 0,
    RDX_BUTTON_GESTURE_ENTRY_HOLD,
    RDX_BUTTON_GESTURE_ENTRY_RELEASE,
    RDX_BUTTON_GESTURE_READY,
    RDX_BUTTON_GESTURE_FIRST_PRESS,
    RDX_BUTTON_GESTURE_CLICK_WINDOW,
    RDX_BUTTON_GESTURE_CONFIRMING,
    RDX_BUTTON_GESTURE_WAIT_RELEASE
} RDX_BUTTON_GESTURE_STATE_T;

typedef enum _RDX_BUTTON_RECONNECT_STATE_T
{
    RDX_BUTTON_RECONNECT_IDLE = 0,
    RDX_BUTTON_RECONNECT_RECHECK_ABSENT = 1,
    RDX_BUTTON_RECONNECT_WAIT_ABSENT = 2,
    RDX_BUTTON_RECONNECT_DISCONNECT = 3,
    RDX_BUTTON_RECONNECT_CONNECT = 4
} RDX_BUTTON_RECONNECT_STATE_T;

typedef struct _RDX_BUTTON_MODE_STATE_T
{
    RDX_BUTTON_GESTURE_STATE_T gesture_state;
    RDX_BUTTON_RECONNECT_STATE_T reconnect_state;
    UINT8_T selection;
    UINT32_T click_deadline;
    UINT32_T hold_deadline;
    UINT32_T session_deadline;
    UINT32_T reconnect_deadline;
    UINT32_T usb_settle_deadline;
    BOOLEAN_T usb_settling;
} RDX_BUTTON_MODE_STATE_T;

static RDX_BUTTON_MODE_STATE_T rdx_button_mode;

/** Return TRUE when one wrap-safe absolute millisecond deadline has passed. */
static BOOLEAN_T rdx_button_mode_deadline_reached(UINT32_T now,
                                                  UINT32_T deadline)
{
    return ((INT32_T)(now - deadline) >= 0) ? TRUE : FALSE;
}

/** Restore the ordinary dock-controller display and wait for release. */
static void rdx_button_mode_cancel(BOOLEAN_T button_pressed)
{
    rdx_led_cancel_gesture(RDX_LED_DOCK);
    rdx_button_mode.gesture_state = button_pressed ?
        RDX_BUTTON_GESTURE_WAIT_RELEASE : RDX_BUTTON_GESTURE_IDLE;
}

/** Start the zero-based selection display for one menu entry. */
static void rdx_button_mode_show_selection(UINT8_T selection)
{
    rdx_button_mode.selection = selection;
    rdx_led_show_gesture_selection(RDX_LED_DOCK, selection);
}

/**
 * @brief Dispatch one confirmed entry from the seven-entry menu table.
 *
 * Entries zero and one persist operation modes one and two. Entries two
 * through five intentionally perform no action. Entry six requests a system
 * reset. The selected action is deferred until the LED confirmation finishes.
 */
static void rdx_button_mode_dispatch_selection(UINT32_T now,
                                               BOOLEAN_T cartridge_present)
{
    if (rdx_button_mode.selection == 0U)
    {
        (void)rdx_manager_set_button_operation_mode(1U);
    }
    else if (rdx_button_mode.selection == 1U)
    {
        (void)rdx_manager_set_button_operation_mode(2U);
        /* Mode two clears the same scoped PREVENT interlock consulted by a
         * host eject request; it does not synthesize sense data. */
        scsi_clear_medium_removal_prevented();
    }
    else if (rdx_button_mode.selection == 6U)
    {
        system_reset();
        return;
    }

    if (rdx_button_mode.selection <= 1U)
    {
        /* Persistence schedules reconnect state three for an empty bay or
         * state two while media remains present; it never detaches USB in the
         * callback itself. Mode two clears only its defined SCSI interlock,
         * leaving every unrelated command feature untouched. */
        rdx_button_mode.reconnect_state = cartridge_present ?
            RDX_BUTTON_RECONNECT_WAIT_ABSENT :
            RDX_BUTTON_RECONNECT_DISCONNECT;
        /* The reconnect timer starts expired. State three therefore detaches
         * on the next foreground service instead of adding an entry delay. */
        rdx_button_mode.reconnect_deadline = now;
    }
}

/**
 * @brief Advance asynchronous reconnect states two, three, and four.
 *
 * State three detaches and arms 1000 ms. State four reconnects and then arms
 * a separate 2000-ms transfer-settle interval. State two waits until media is
 * absent before entering the one-millisecond recheck stages.
 */
static void rdx_button_mode_service_reconnect(UINT32_T now,
                                              BOOLEAN_T cartridge_present)
{
    if (rdx_button_mode.usb_settling &&
        rdx_button_mode_deadline_reached(
            now, rdx_button_mode.usb_settle_deadline))
    {
        /* The separate transfer-settle interval expires while reconnect state
         * itself is already idle. No unrelated command state consumes it. */
        rdx_button_mode.usb_settling = FALSE;
    }

    if (rdx_button_mode.reconnect_state == RDX_BUTTON_RECONNECT_IDLE)
    {
        return;
    }

    if (rdx_button_mode.reconnect_state == RDX_BUTTON_RECONNECT_WAIT_ABSENT)
    {
        if (cartridge_present)
        {
            return;
        }
        /* State two never jumps directly to detach. It enters a one-millisecond
         * absence recheck, then gives state three another millisecond before
         * the disconnect operation can run. */
        rdx_button_mode.reconnect_state =
            RDX_BUTTON_RECONNECT_RECHECK_ABSENT;
        rdx_button_mode.reconnect_deadline = now + 1U;
        return;
    }

    if (!rdx_button_mode_deadline_reached(
            now, rdx_button_mode.reconnect_deadline))
    {
        return;
    }

    if (rdx_button_mode.reconnect_state ==
        RDX_BUTTON_RECONNECT_RECHECK_ABSENT)
    {
        if (cartridge_present)
        {
            rdx_button_mode.reconnect_state =
                RDX_BUTTON_RECONNECT_WAIT_ABSENT;
            return;
        }
        rdx_button_mode.reconnect_state = RDX_BUTTON_RECONNECT_DISCONNECT;
        rdx_button_mode.reconnect_deadline = now + 1U;
    }
    else if (rdx_button_mode.reconnect_state ==
             RDX_BUTTON_RECONNECT_DISCONNECT)
    {
        if (cartridge_present)
        {
            rdx_button_mode.reconnect_state =
                RDX_BUTTON_RECONNECT_WAIT_ABSENT;
            return;
        }
        usb_hal_disconnect();
        rdx_button_mode.reconnect_deadline =
            READ_REG32(RTIFRC1_REG_OFF) +
            RDX_BUTTON_RECONNECT_DETACH_MS;
        rdx_button_mode.reconnect_state = RDX_BUTTON_RECONNECT_CONNECT;
    }
    else if (rdx_button_mode.reconnect_state == RDX_BUTTON_RECONNECT_CONNECT)
    {
        /* Clear reconnect state before reconnecting USB. */
        rdx_button_mode.reconnect_state = RDX_BUTTON_RECONNECT_IDLE;
        usb_hal_connect();
        rdx_button_mode.usb_settle_deadline =
            now + RDX_BUTTON_RECONNECT_SETTLE_MS;
        rdx_button_mode.usb_settling = TRUE;
    }
}

/**
 * @brief Advance the hidden-menu root and button substates.
 *
 * The initial debounced press must remain held for all 5000 ms; an early
 * release cancels. Selection presses arm independent 1000-ms, 5000-ms, and
 * 60000-ms timers. A release followed by a second press before the one-second
 * timer expires confirms; otherwise the seven-entry index advances and its
 * LED pattern restarts.
 */
static void rdx_button_mode_service_gesture(UINT32_T now,
                                            BOOLEAN_T button_pressed,
                                            BOOLEAN_T cartridge_present)
{
    if (cartridge_present)
    {
        if ((rdx_button_mode.gesture_state != RDX_BUTTON_GESTURE_IDLE) &&
            (rdx_button_mode.gesture_state !=
             RDX_BUTTON_GESTURE_WAIT_RELEASE))
        {
            rdx_button_mode_cancel(button_pressed);
        }
        return;
    }

    if ((rdx_button_mode.gesture_state != RDX_BUTTON_GESTURE_IDLE) &&
        (rdx_button_mode.gesture_state !=
         RDX_BUTTON_GESTURE_WAIT_RELEASE) &&
        rdx_button_mode_deadline_reached(
            now, rdx_button_mode.session_deadline))
    {
        /* The 60-second menu timer is refreshed by every stable press, never
         * by a release. It cancels entry, selection, or confirmation state. */
        rdx_button_mode_cancel(button_pressed);
        return;
    }

    switch (rdx_button_mode.gesture_state)
    {
        case RDX_BUTTON_GESTURE_IDLE:
            if (button_pressed)
            {
                rdx_button_mode.hold_deadline =
                    now + RDX_BUTTON_ENTRY_HOLD_MS;
                rdx_button_mode.session_deadline =
                    now + RDX_BUTTON_SESSION_TIMEOUT_MS;
                rdx_button_mode.gesture_state =
                    RDX_BUTTON_GESTURE_ENTRY_HOLD;
            }
            break;

        case RDX_BUTTON_GESTURE_ENTRY_HOLD:
            if (!button_pressed)
            {
                /* Entry requires one uninterrupted no-cartridge hold. A
                 * release before the five-second deadline cancels it. */
                rdx_button_mode_cancel(FALSE);
            }
            else if (rdx_button_mode_deadline_reached(
                         now, rdx_button_mode.hold_deadline))
            {
                rdx_button_mode_show_selection(0U);
                rdx_button_mode.gesture_state =
                    RDX_BUTTON_GESTURE_ENTRY_RELEASE;
            }
            break;

        case RDX_BUTTON_GESTURE_ENTRY_RELEASE:
            if (!button_pressed)
            {
                rdx_button_mode.gesture_state = RDX_BUTTON_GESTURE_READY;
            }
            break;

        case RDX_BUTTON_GESTURE_READY:
            if (button_pressed)
            {
                rdx_button_mode.click_deadline =
                    now + RDX_BUTTON_CLICK_WINDOW_MS;
                rdx_button_mode.hold_deadline =
                    now + RDX_BUTTON_SELECTION_HOLD_MS;
                rdx_button_mode.session_deadline =
                    now + RDX_BUTTON_SESSION_TIMEOUT_MS;
                rdx_button_mode.gesture_state =
                    RDX_BUTTON_GESTURE_FIRST_PRESS;
            }
            break;

        case RDX_BUTTON_GESTURE_FIRST_PRESS:
            if (!button_pressed)
            {
                rdx_button_mode.gesture_state =
                    RDX_BUTTON_GESTURE_CLICK_WINDOW;
            }
            else if (rdx_button_mode_deadline_reached(
                         now, rdx_button_mode.hold_deadline))
            {
                rdx_button_mode_cancel(TRUE);
            }
            break;

        case RDX_BUTTON_GESTURE_CLICK_WINDOW:
            if (button_pressed &&
                !rdx_button_mode_deadline_reached(
                    now, rdx_button_mode.click_deadline))
            {
                /* The second stable press is P4 -> P5. It refreshes T5/T60
                 * and queues root state 4, whose LED conversion is deferred
                 * to the next ordinary controller service. */
                rdx_button_mode.hold_deadline =
                    now + RDX_BUTTON_SELECTION_HOLD_MS;
                rdx_button_mode.session_deadline =
                    now + RDX_BUTTON_SESSION_TIMEOUT_MS;
                rdx_led_confirm_gesture(RDX_LED_DOCK);
                rdx_button_mode.gesture_state =
                    RDX_BUTTON_GESTURE_CONFIRMING;
            }
            else if (rdx_button_mode_deadline_reached(
                         now, rdx_button_mode.click_deadline))
            {
                rdx_button_mode_show_selection(
                    (rdx_button_mode.selection < 6U) ?
                    (UINT8_T)(rdx_button_mode.selection + 1U) : 0U);
                rdx_button_mode.gesture_state = RDX_BUTTON_GESTURE_READY;
            }
            break;

        case RDX_BUTTON_GESTURE_CONFIRMING:
            if (rdx_led_take_gesture_confirmation(RDX_LED_DOCK))
            {
                rdx_button_mode_dispatch_selection(now, cartridge_present);
                rdx_button_mode.gesture_state = button_pressed ?
                    RDX_BUTTON_GESTURE_WAIT_RELEASE :
                    RDX_BUTTON_GESTURE_IDLE;
            }
            break;

        default:
            if (!button_pressed)
            {
                rdx_button_mode.gesture_state = RDX_BUTTON_GESTURE_IDLE;
            }
            break;
    }
}

/** Initialize the hidden no-cartridge gesture and deferred reconnect path. */
void rdx_button_mode_init(void)
{
    rdx_button_mode.gesture_state = RDX_BUTTON_GESTURE_IDLE;
    rdx_button_mode.reconnect_state = RDX_BUTTON_RECONNECT_IDLE;
    rdx_button_mode.selection = 0U;
    rdx_button_mode.click_deadline = 0U;
    rdx_button_mode.hold_deadline = 0U;
    rdx_button_mode.session_deadline = 0U;
    rdx_button_mode.reconnect_deadline = 0U;
    rdx_button_mode.usb_settle_deadline = 0U;
    rdx_button_mode.usb_settling = FALSE;
}

/** Advance the hidden button gesture and its asynchronous USB reconnect. */
void rdx_button_mode_service(UINT32_T now, BOOLEAN_T button_pressed,
                             BOOLEAN_T cartridge_present)
{
    rdx_button_mode_service_gesture(now, button_pressed, cartridge_present);
    rdx_button_mode_service_reconnect(now, cartridge_present);
}
