# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for the OpenRDX button and LED menu."""

from pathlib import Path
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"
INCLUDE_DIR = PROJECT_DIR / "include" / "rdx_mount"
BUTTON = (SOURCE_DIR / "rdx_button_mode.c").read_text(encoding="utf-8")
BUTTON_HEADER = (INCLUDE_DIR / "rdx_button_mode.h").read_text(encoding="utf-8")
LED = (SOURCE_DIR / "rdx_led.c").read_text(encoding="utf-8")
MANAGER = (SOURCE_DIR / "rdx_manager_control.c").read_text(encoding="utf-8")
SCSI = (SOURCE_DIR / "scsi.c").read_text(encoding="utf-8")
SCSI_HEADER = (INCLUDE_DIR / "scsi.h").read_text(encoding="utf-8")


def function_body(source, signature):
    """Return one C function body using balanced braces."""
    signature_start = source.index(signature)
    body_start = source.index("{", signature_start)
    depth = 0
    for index in range(body_start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[body_start : index + 1]
    raise AssertionError("unterminated function: {}".format(signature))


class RdxButtonModeTests(unittest.TestCase):
    """Lock the no-media menu and shared LED scheduler details."""

    def test_initial_entry_is_one_continuous_five_second_hold(self):
        """Require release-before-T5 cancellation and release-after-T5 entry."""
        service = function_body(
            BUTTON,
            "static void rdx_button_mode_service_gesture(UINT32_T now,",
        )
        self.assertIn("RDX_BUTTON_ENTRY_HOLD_MS                 5000UL", BUTTON)
        self.assertIn("RDX_BUTTON_SESSION_TIMEOUT_MS           60000UL", BUTTON)
        entry = service[
            service.index("case RDX_BUTTON_GESTURE_ENTRY_HOLD:") :
            service.index("case RDX_BUTTON_GESTURE_ENTRY_RELEASE:")
        ]
        self.assertLess(entry.index("if (!button_pressed)"), entry.index("hold_deadline"))
        self.assertIn("rdx_button_mode_cancel(FALSE);", entry)
        self.assertIn("rdx_button_mode_show_selection(0U);", entry)
        release = service[
            service.index("case RDX_BUTTON_GESTURE_ENTRY_RELEASE:") :
            service.index("case RDX_BUTTON_GESTURE_READY:")
        ]
        self.assertIn("if (!button_pressed)", release)

    def test_click_window_cycles_seven_entries_and_double_click_confirms(self):
        """Preserve T1/T5/T60, wraparound, and the root-four confirmation."""
        for token in (
            "RDX_BUTTON_CLICK_WINDOW_MS               1000UL",
            "RDX_BUTTON_SELECTION_HOLD_MS             5000UL",
            "RDX_BUTTON_SESSION_TIMEOUT_MS           60000UL",
            "(rdx_button_mode.selection < 6U)",
            "rdx_led_confirm_gesture(RDX_LED_DOCK);",
            "rdx_led_take_gesture_confirmation(RDX_LED_DOCK)",
        ):
            self.assertIn(token, BUTTON)

    def test_callback_table_changes_only_modes_one_two_or_resets(self):
        """Keep entries 2..5 null and dispatch entry 6 through system reset."""
        dispatch = function_body(
            BUTTON,
            "static void rdx_button_mode_dispatch_selection(UINT32_T now,",
        )
        self.assertEqual(1, dispatch.count("rdx_manager_set_button_operation_mode(1U)"))
        self.assertEqual(1, dispatch.count("rdx_manager_set_button_operation_mode(2U)"))
        self.assertEqual(1, dispatch.count("scsi_clear_medium_removal_prevented();"))
        self.assertEqual(1, dispatch.count("system_reset();"))
        self.assertNotIn("usb_hal_disconnect();", dispatch)
        self.assertNotIn("usb_hal_connect();", dispatch)
        clear_prevent = function_body(
            SCSI, "void scsi_clear_medium_removal_prevented(void)"
        )
        self.assertIn("scsi_medium_removal_prevented = FALSE;", clear_prevent)
        self.assertIn(
            "void scsi_clear_medium_removal_prevented(void);", SCSI_HEADER
        )

    def test_reconnect_is_deferred_and_preserves_required_waits(self):
        """Detach on reconnect service, wait 1 s, connect, and settle 2 s."""
        reconnect = function_body(
            BUTTON,
            "static void rdx_button_mode_service_reconnect(UINT32_T now,",
        )
        self.assertIn("RDX_BUTTON_RECONNECT_DETACH_MS            1000UL", BUTTON)
        self.assertIn("RDX_BUTTON_RECONNECT_SETTLE_MS            2000UL", BUTTON)
        self.assertLess(reconnect.index("usb_hal_disconnect();"), reconnect.index("usb_hal_connect();"))
        self.assertIn("RDX_BUTTON_RECONNECT_WAIT_ABSENT", reconnect)

    def test_led_uses_one_dock_driven_shared_service_timer(self):
        """Dock mode two must make both controllers run every 100 ms."""
        tick = function_body(LED, "void rdx_led_tick(void)")
        fast = tick.index(
            "rdx_led_controllers[RDX_LED_DOCK].mode == RDX_LED_MODE_FAST"
        )
        dock = tick.index("rdx_led_service_controller(RDX_LED_DOCK);", fast)
        cartridge = tick.index(
            "rdx_led_service_controller(RDX_LED_CARTRIDGE);", dock
        )
        self.assertLess(fast, dock)
        self.assertLess(dock, cartridge)
        self.assertIn("RDX_LED_NORMAL_SERVICE_TICKS", tick)
        self.assertNotIn("cadence_ticks", LED.split("typedef struct _RDX_LED_CONTROLLER_T", 1)[1].split("}", 1)[0])

    def test_selection_cycle_has_no_duplicate_first_off_frame(self):
        """Root 3 must render root 8's second-off preamble immediately."""
        advance = function_body(
            LED, "static void rdx_led_advance_gesture(RDX_LED_TARGET_T target)"
        )
        first_blink = advance[
            advance.index("case RDX_LED_GESTURE_FIRST_BLINK:") :
            advance.index("default:")
        ]
        exhausted = first_blink[first_blink.index("else") :]
        self.assertIn("controller->state = RDX_LED_STATE_SECOND;", exhausted)
        self.assertIn("controller->phase = FALSE;", exhausted)
        self.assertIn(
            "controller->gesture_state = RDX_LED_GESTURE_SECOND_ON;",
            exhausted,
        )
        self.assertNotIn(
            "controller->gesture_state = RDX_LED_GESTURE_SECOND_OFF;",
            exhausted,
        )

    def test_accepted_eject_count_is_checksum_persisted(self):
        """Persist a RAM-first counter update with the required flash sequence."""
        increment = function_body(
            MANAGER, "STATUS_T rdx_manager_increment_drive_load_count(void)"
        )
        save = function_body(
            MANAGER,
            "static STATUS_T rdx_control_save_drive_load_count(UINT32_T",
        )
        self.assertLess(
            increment.index("rdx_control.drive_load_count++;"),
            increment.index("rdx_control_save_drive_load_count("),
        )
        for token in (
            "RDX_STATE_LOAD_COUNT_OFFSET",
            "OpcodeSectorErase",
            "OpcodePageProgram",
            "rdx_control_calculate_checksum",
        ):
            self.assertIn(token, save)
        self.assertIn("void rdx_button_mode_init(void);", BUTTON_HEADER)
        self.assertIn("void rdx_button_mode_service(", BUTTON_HEADER)


if __name__ == "__main__":
    unittest.main()
