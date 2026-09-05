# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for the RDX hardware coordinator."""

from pathlib import Path
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"
INCLUDE_DIR = PROJECT_DIR / "include" / "rdx_mount"
HARDWARE = (SOURCE_DIR / "rdx_hardware.c").read_text(encoding="utf-8")
HARDWARE_HEADER = (INCLUDE_DIR / "rdx_hardware.h").read_text(encoding="utf-8")
MECHANISM = (SOURCE_DIR / "rdx_mechanism.c").read_text(encoding="utf-8")
MECHANISM_HEADER = (INCLUDE_DIR / "rdx_mechanism.h").read_text(encoding="utf-8")
BUTTON_MODE = (SOURCE_DIR / "rdx_button_mode.c").read_text(encoding="utf-8")
AHCI = (SOURCE_DIR / "ahci.c").read_text(encoding="utf-8")
MAIN = (SOURCE_DIR / "main.c").read_text(encoding="utf-8")
RTI = (SOURCE_DIR / "rti.c").read_text(encoding="utf-8")
MANAGER = (SOURCE_DIR / "rdx_manager_protocol.c").read_text(encoding="utf-8")


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


def assert_statements_in_order(test_case, body, statements):
    """Require each supplied statement to follow the preceding statement."""
    cursor = 0
    for statement in statements:
        position = body.find(statement, cursor)
        test_case.assertNotEqual(
            -1,
            position,
            "missing or out-of-order statement: {}".format(statement),
        )
        cursor = position + len(statement)


class RdxHardwareRuntimeTests(unittest.TestCase):
    """Lock the audited input, mechanism, thermal, and LED integration."""

    def test_foreground_runtime_is_initialized_and_serviced(self):
        """Run blocking hardware work in main and keep RTI service bounded."""
        self.assertIn("void rdx_hardware_init(void);", HARDWARE_HEADER)
        self.assertIn("void rdx_hardware_start(void);", HARDWARE_HEADER)
        self.assertIn("void rdx_hardware_service(void);", HARDWARE_HEADER)
        self.assertLess(MAIN.index("scsi_init();"), MAIN.index("rdx_hardware_init();"))
        self.assertLess(MAIN.index("rdx_hardware_init();"), MAIN.index("usb_stack_init();"))
        self.assertLess(MAIN.index("ahci_init();"), MAIN.index("rdx_hardware_start();"))
        self.assertLess(MAIN.index("rdx_hardware_start();"), MAIN.index("wdt_start();"))
        self.assertLess(
            MAIN.index("ahci_service();"),
            MAIN.index("if (ata_dev[0].callback_pending"),
        )
        self.assertIn("rdx_hardware_service();", MAIN)
        self.assertIn("rdx_hardware_note_activity();", RTI)
        self.assertNotIn("pwm_", RTI)

    def test_button_and_presence_inputs_use_required_debounce_and_dispatch(self):
        """Debounce both inputs and route media/no-media presses separately."""
        self.assertIn("RDX_INPUT_DEBOUNCE_MS                   100UL", HARDWARE)
        button = function_body(
            HARDWARE,
            "static void rdx_hardware_service_eject_button(UINT32_T now)",
        )
        self.assertIn("gio_rdx_eject_button_pressed();", button)
        self.assertIn("rdx_hardware.button_armed = FALSE;", button)
        self.assertIn("rdx_hardware.button_armed = TRUE;", button)
        self.assertIn("scsi_medium_removal_is_prevented()", button)
        self.assertIn("rdx_led_start_second_blink(RDX_LED_DOCK);", button)
        self.assertNotIn("eject_from_button", HARDWARE)
        self.assertNotIn("scsi_set_sense_data", button)
        self.assertNotIn("usb_hal_disconnect", HARDWARE)
        self.assertNotIn("RDX_BUTTON_FORCE_DISCONNECT_MS", HARDWARE)
        self.assertIn("rdx_button_mode_init();", HARDWARE)
        self.assertIn("rdx_button_mode_service(", HARDWARE)
        self.assertIn("rdx_manager_increment_drive_load_count();", HARDWARE)
        self.assertIn("RDX_BUTTON_ENTRY_HOLD_MS                 5000UL", BUTTON_MODE)
        presence = function_body(
            HARDWARE,
            "static void rdx_hardware_service_cartridge_input(UINT32_T now)",
        )
        self.assertIn("gio_rdx_cartridge_present();", presence)
        self.assertIn("RDX_INPUT_DEBOUNCE_MS", presence)

    def test_lock_slider_is_cached_at_media_ready_and_fails_closed(self):
        """Finish the one-shot channel-4 sample before publishing ATA ready."""
        prepare = function_body(
            HARDWARE,
            "void rdx_hardware_prepare_media_ready(UINT32_T port_num)",
        )
        reinitializing = function_body(
            HARDWARE, "void rdx_hardware_media_reinitializing(UINT32_T port_num)"
        )
        init_port = function_body(
            AHCI, "STATUS_T ahci_init_port(UINT32_T port_num)"
        )
        assert_statements_in_order(
            self,
            prepare,
            (
                "rdx_hardware.write_protected = TRUE;",
                "WRITE_REG32(VIM_REQMASKCLR0, RDX_USB_INTERRUPT_MASK);",
                "rdx_mcp3008_read_channel(RDX_SLIDER_CHANNEL, &sample);",
                "WRITE_REG32(VIM_REQMASKSET0, RDX_USB_INTERRUPT_MASK);",
                "(sample > RDX_SLIDER_UNLOCKED_THRESHOLD)",
                "rdx_hardware.write_protected = FALSE;",
            ),
        )
        self.assertIn("RDX_SLIDER_CHANNEL                      4U", HARDWARE)
        self.assertIn("RDX_SLIDER_UNLOCKED_THRESHOLD           650U", HARDWARE)
        self.assertNotIn("RDX_SLIDER_SAMPLE_MS", HARDWARE)
        self.assertNotIn("slider_sample_needed", HARDWARE)
        self.assertNotIn("rdx_hardware_service_slider", HARDWARE)
        self.assertIn("rdx_hardware.write_protected = TRUE;", reinitializing)
        assert_statements_in_order(
            self,
            init_port,
            (
                "ata_dev[port_num].bDeviceInitComplete = FALSE;",
                "rdx_hardware_media_reinitializing(port_num);",
                "rdx_hardware_prepare_media_ready(port_num);",
                "ata_dev[port_num].bDeviceInitComplete = TRUE;",
            ),
        )

    def test_mechanism_uses_exact_outputs_timers_and_retry_routing(self):
        """Preserve states 1..8 without collapsing their powered phases."""
        run = function_body(
            MECHANISM, "static void rdx_mechanism_run_motor(BOOLEAN_T control_high)"
        )
        stop = function_body(
            MECHANISM, "static void rdx_mechanism_stop_motor(void)"
        )
        enter = function_body(
            MECHANISM,
            "static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,",
        )
        service = function_body(
            MECHANISM, "RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now)"
        )
        assert_statements_in_order(
            self,
            run,
            (
                "gio_rdx_motor_control_set_high(control_high);",
                "usleep(RDX_MECHANISM_DEAD_TIME_US);",
                "pwm_run(RDX_MOTOR_PWM_NUM, 100U, RDX_MOTOR_PWM_PERIOD_US);",
            ),
        )
        assert_statements_in_order(
            self,
            stop,
            (
                "pwm_disable(RDX_MOTOR_PWM_NUM);",
                "usleep(RDX_MECHANISM_DEAD_TIME_US);",
                "gio_rdx_motor_control_set_high(TRUE);",
            ),
        )
        for token in (
            "RDX_MECHANISM_DRIVE_TIME_MS           2000UL",
            "RDX_MECHANISM_INTERPHASE_TIME_MS      2000UL",
            "RDX_MECHANISM_STATE2_TIME_MS            50UL",
            "RDX_MECHANISM_SUCCESS_SETTLE_MS        500UL",
            "RDX_MECHANISM_MAX_REVERSE_EXPIRIES       3U",
        ):
            self.assertIn(token, MECHANISM)
        for state in range(1, 9):
            self.assertIn("RDX_MECHANISM_STATE_{} = {}".format(state, state), MECHANISM)
        self.assertIn("gio_rdx_mechanism_auxiliary_set(FALSE);", enter)
        self.assertIn("rdx_mechanism_run_motor(FALSE);", enter)
        self.assertIn("gio_rdx_motor_control_set_high(TRUE);", enter)
        self.assertNotIn("profile_mechanism_auxiliary", MECHANISM)
        self.assertIn("rdx_mechanism.hardware_profile != 0x36U", enter)
        self.assertIn(
            "pwm_run(RDX_MOTOR_PWM_NUM, 100U,\n"
            "                        RDX_MOTOR_PWM_PERIOD_US);",
            enter,
        )
        self.assertEqual(3, enter.count("now + RDX_MECHANISM_DRIVE_TIME_MS"))
        state_seven = enter[
            enter.index("case RDX_MECHANISM_STATE_7:") :
            enter.index("case RDX_MECHANISM_STATE_8:")
        ]
        self.assertIn("rdx_mechanism_stop_motor();", state_seven)
        self.assertNotIn("rdx_mechanism.deadline =", state_seven)
        self.assertIn("rdx_mechanism.reverse_expiries++;", service)
        self.assertIn("RDX_MECHANISM_EVENT_SUCCEEDED", service)
        self.assertIn("RDX_MECHANISM_EVENT_FAILED", service)

    def test_mechanism_samples_live_media_context_and_defers_boot_homing(self):
        """Initialize idle, then home only after synchronous startup returns."""
        context = function_body(
            MECHANISM, "static BOOLEAN_T rdx_mechanism_media_context_absent(void)"
        )
        initialize = function_body(
            MECHANISM,
            "void rdx_mechanism_init(UINT16_T hardware_profile)",
        )
        start_homing = function_body(
            MECHANISM,
            "BOOLEAN_T rdx_mechanism_start_boot_homing(UINT32_T now)",
        )
        self.assertIn("rdx_mcp3008_read_channel(", context)
        self.assertIn("RDX_MECHANISM_CONTEXT_ADC_CHANNEL", context)
        self.assertIn("adc_sample <= RDX_MECHANISM_CONTEXT_ADC_THRESHOLD", context)
        self.assertIn("adc_status != STATUS_OK", context)
        self.assertNotIn("write_protected", MECHANISM)
        self.assertIn("rdx_mechanism_stop_motor();", initialize)
        self.assertNotIn("RDX_MECHANISM_STATE_1", initialize)
        self.assertIn("if (!gio_rdx_mechanism_input_asserted())", start_homing)
        self.assertIn("RDX_MECHANISM_STATE_1", start_homing)
        self.assertIn("void rdx_mechanism_init(", MECHANISM_HEADER)
        self.assertIn(
            "BOOLEAN_T rdx_mechanism_start_boot_homing(", MECHANISM_HEADER
        )
        hardware_initialize = function_body(
            HARDWARE, "void rdx_hardware_init(void)"
        )
        hardware_start = function_body(
            HARDWARE, "void rdx_hardware_start(void)"
        )
        self.assertIn("rdx_mechanism_init(hardware_profile);", hardware_initialize)
        self.assertNotIn("rdx_mechanism_start_boot_homing", hardware_initialize)
        self.assertIn("rdx_mechanism_start_boot_homing", hardware_start)

    def test_eject_uses_complete_timed_return_and_retry_path(self):
        """Keep eject on the common state sequence until GPIO2 completion."""
        start = function_body(
            MECHANISM,
            "void rdx_mechanism_start(UINT32_T now,",
        )
        service = function_body(
            MECHANISM, "RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now)"
        )
        eject = function_body(
            HARDWARE, "static void rdx_hardware_service_eject(UINT32_T now)"
        )

        self.assertIn(
            "rdx_mechanism_enter_state(RDX_MECHANISM_STATE_1, now);", start
        )
        self.assertEqual(
            3,
            service.count(
                "rdx_mechanism_enter_state(RDX_MECHANISM_STATE_5, now);"
            ),
        )
        for shortcut in (
            "RDX_MECHANISM_EJECT_DRIVE_TIME_MS",
            "RDX_MECHANISM_RELEASE_CONFIRM_MS",
            "accepted_eject",
            "eject_cutoff_pending",
            "rdx_mechanism_tick",
        ):
            self.assertNotIn(shortcut, MECHANISM)
        self.assertNotIn("sata_media_get_kind", eject)
        self.assertIn(
            "rdx_mechanism_start(rdx_hardware_now_ms(),\n"
            "                            rdx_manager_get_hardware_profile());",
            eject,
        )
        self.assertNotIn("rdx_mechanism_tick", MECHANISM_HEADER)
        self.assertNotIn("rdx_mechanism", RTI)

    def test_mechanism_requires_directional_gpio2_endpoint_transition(self):
        """Require GPIO2 deassertion before accepting endpoint assertion."""
        reset = function_body(
            MECHANISM, "static void rdx_mechanism_reset_transition(void)"
        )
        transition = function_body(
            MECHANISM,
            "static BOOLEAN_T rdx_mechanism_transition_complete(void)",
        )
        service = function_body(
            MECHANISM, "RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now)"
        )

        self.assertIn("rdx_mechanism.mechanism_deasserted_seen = FALSE;", reset)
        assert_statements_in_order(
            self,
            transition,
            (
                "asserted = gio_rdx_mechanism_input_asserted();",
                "if (!asserted)",
                "rdx_mechanism.mechanism_deasserted_seen = TRUE;",
                "else if (rdx_mechanism.mechanism_deasserted_seen)",
                "return TRUE;",
            ),
        )
        self.assertEqual(
            5,
            service.count("rdx_mechanism_transition_complete()"),
        )

    def test_eject_acceptance_is_counted_once_and_not_reinterlocked(self):
        """Check PREVENT at each source, then run the common eject sequence."""
        eject = function_body(
            HARDWARE, "static void rdx_hardware_service_eject(UINT32_T now)"
        )
        failure = function_body(
            HARDWARE, "static void rdx_hardware_fail_eject(void)"
        )
        self.assertEqual(1, eject.count("rdx_manager_increment_drive_load_count"))
        self.assertNotIn("scsi_medium_removal_is_prevented", eject)
        assert_statements_in_order(
            self,
            eject,
            (
                "rdx_manager_increment_drive_load_count();",
                "RDX_EJECT_WAIT_FOR_IO",
                "rdx_prepare_mechanism_eject(0U);",
                "rdx_hardware_teardown_eject_media();",
                "rdx_mechanism_start(",
            ),
        )
        self.assertIn("rdx_hardware_teardown_eject_media();", failure)
        self.assertNotIn("rdx_prepare_media_stop", eject)

    def test_logical_unload_is_an_isolated_reversible_visibility_gate(self):
        """Hide SCSI media without changing ATA or physical controllers."""
        unload = function_body(
            HARDWARE, "BOOLEAN_T rdx_hardware_logical_unload(void)"
        )
        reload_media = function_body(
            HARDWARE, "BOOLEAN_T rdx_hardware_logical_reload(void)"
        )
        initialize = function_body(HARDWARE, "void rdx_hardware_init(void)")
        presence = function_body(
            HARDWARE,
            "static void rdx_hardware_service_cartridge_input(UINT32_T now)",
        )
        teardown = function_body(
            HARDWARE, "static void rdx_hardware_teardown_eject_media(void)"
        )

        for declaration in (
            "BOOLEAN_T rdx_hardware_logical_unload(void);",
            "BOOLEAN_T rdx_hardware_logical_reload(void);",
            "BOOLEAN_T rdx_hardware_is_logically_unloaded(void);",
        ):
            self.assertIn(declaration, HARDWARE_HEADER)
        self.assertIn("!ata_dev[0].bDeviceInitComplete", unload)
        self.assertIn("RDX_EJECT_IDLE", unload)
        self.assertIn("rdx_hardware.logical_unloaded = TRUE;", unload)
        for forbidden in (
            "pwm_",
            "rdx_led_",
            "rdx_mechanism_",
            "rdx_manager_increment_drive_load_count",
            "rdx_prepare_",
            "bDeviceInitComplete =",
        ):
            self.assertNotIn(forbidden, unload)
            self.assertNotIn(forbidden, reload_media)
        self.assertIn("if (!rdx_hardware.logical_unloaded)", reload_media)
        self.assertIn("rdx_hardware.logical_unloaded = FALSE;", reload_media)
        self.assertIn("rdx_hardware.logical_unloaded = FALSE;", initialize)
        self.assertIn("rdx_hardware.logical_unloaded = FALSE;", presence)
        self.assertIn("rdx_hardware.logical_unloaded = FALSE;", teardown)

    def test_temperature_and_fan_preserve_required_bands_and_cadence(self):
        """Confirm hot samples and ramp PWM0 from normal duty in 13% steps."""
        for token in (
            "RDX_FAN_SERVICE_MS                      800UL",
            "RDX_FAN_START_DELAY_MS                  100UL",
            "RDX_FAN_RAMP_DELAY_MS                   60000UL",
            "RDX_FAN_INITIAL_DUTY                    10U",
            "RDX_FAN_NORMAL_DUTY                     50U",
            "RDX_FAN_RAMP_STEP                       13U",
            "RDX_TEMPERATURE_NORMAL_SAMPLE_MS        300000UL",
            "RDX_TEMPERATURE_HOT_SAMPLE_MS           10000UL",
            "RDX_TEMPERATURE_COOL_MAX                40U",
            "RDX_TEMPERATURE_HOT_MIN                 45U",
            "RDX_TEMPERATURE_HOT_CONFIRMATIONS       3U",
        ):
            self.assertIn(token, HARDWARE)
        self.assertRegex(
            HARDWARE,
            r"#define\s+RDX_TEMPERATURE_FAILURE_RETRY_MS\s+10000UL\b",
        )
        self.assertIn("rdx_read_smart_temperature(0U, &temperature);", HARDWARE)
        temperature = function_body(
            HARDWARE,
            "static void rdx_hardware_service_temperature(UINT32_T now)",
        )
        self.assertIn(
            "rdx_hardware.hot_sample_count =\n"
            "            RDX_TEMPERATURE_HOT_CONFIRMATIONS;",
            temperature,
        )
        self.assertIn(
            "else\n        {\n"
            "            rdx_hardware.thermal_phase = RDX_THERMAL_HOT;",
            temperature,
        )
        self.assertIn("rdx_hardware.fan_duty + RDX_FAN_RAMP_STEP", HARDWARE)
        fan = function_body(
            HARDWARE, "static void rdx_hardware_service_fan(UINT32_T now)"
        )
        ramp_start = fan.index("case RDX_FAN_STATE_RAMPING:")
        full_start = fan.index("case RDX_FAN_STATE_FULL:")
        ramp = fan[ramp_start:full_start]
        full = fan[full_start:]
        self.assertNotIn("thermal_phase == RDX_THERMAL_COOL", ramp)
        self.assertIn("thermal_phase == RDX_THERMAL_COOL", full)
        self.assertIn(
            "pwm_run(RDX_FAN_PWM_NUM, duty, RDX_FAN_PWM_PERIOD_US);",
            HARDWARE,
        )
        self.assertIn("RDX_FAN_STATE_STARTING = 5", HARDWARE)
        self.assertNotIn("RDX_FAN_STATE_STOPPING", HARDWARE)
        self.assertIn("if (rdx_hardware.fan_disabled)", fan)
        initialize = function_body(HARDWARE, "void rdx_hardware_init(void)")
        self.assertIn("(hardware_profile == 0x36U) ? TRUE : FALSE", initialize)
        temperature_failure = temperature[
            temperature.index("if (!successful)") :
            temperature.index("rdx_hardware.temperature_celsius = temperature;")
        ]
        self.assertRegex(
            temperature_failure,
            r"rdx_hardware\.temperature_deadline\s*=\s*"
            r"rdx_hardware_now_ms\(\)\s*\+\s*"
            r"RDX_TEMPERATURE_FAILURE_RETRY_MS;",
        )

    def test_led_policy_preserves_controller_ownership(self):
        """Preserve baseline, activity, eject, and fault ownership."""
        media = function_body(
            HARDWARE,
            "static void rdx_hardware_service_media_state(UINT32_T now)",
        )
        initialize = function_body(HARDWARE, "void rdx_hardware_init(void)")
        insertion = function_body(
            HARDWARE,
            "static void rdx_hardware_service_cartridge_input(UINT32_T now)",
        )
        eject = function_body(
            HARDWARE, "static void rdx_hardware_service_eject(UINT32_T now)"
        )
        finish = function_body(
            HARDWARE, "static void rdx_hardware_finish_eject(void)"
        )
        failure = function_body(
            HARDWARE, "static void rdx_hardware_fail_eject(void)"
        )
        teardown = function_body(
            HARDWARE, "static void rdx_hardware_teardown_eject_media(void)"
        )
        assert_statements_in_order(
            self,
            initialize,
            (
                "rdx_led_select_second(RDX_LED_CARTRIDGE, FALSE);",
                "rdx_led_set_steady(RDX_LED_CARTRIDGE, FALSE);",
                "rdx_led_select_second(RDX_LED_DOCK, FALSE);",
                "rdx_led_set_steady(RDX_LED_DOCK, TRUE);",
            ),
        )
        self.assertIn("rdx_led_set_steady(RDX_LED_CARTRIDGE, TRUE);", insertion)
        self.assertIn("rdx_led_set_steady(RDX_LED_CARTRIDGE, FALSE);", insertion)
        self.assertNotIn("RDX_LED_DOCK", insertion)
        self.assertIn(
            "rdx_led_select_second(RDX_LED_CARTRIDGE, init_failed);", media
        )
        self.assertIn("rdx_led_start_blink(RDX_LED_CARTRIDGE);", media)
        self.assertNotIn("rdx_led_start_fast_blink", HARDWARE)
        self.assertIn("rdx_led_start_blink(RDX_LED_DOCK);", eject)
        self.assertNotIn(
            "rdx_led_start_second_blink(RDX_LED_CARTRIDGE);", HARDWARE
        )
        self.assertIn("rdx_led_set_steady(RDX_LED_CARTRIDGE, FALSE);", teardown)
        self.assertIn("rdx_hardware_teardown_eject_media();", finish)
        self.assertIn("rdx_led_select_second(RDX_LED_DOCK, FALSE);", finish)
        self.assertIn("rdx_led_set_steady(RDX_LED_DOCK, TRUE);", finish)
        self.assertIn("rdx_led_select_second(RDX_LED_DOCK, TRUE);", failure)
        self.assertNotIn("rdx_hardware_cancel_eject", HARDWARE)
        self.assertEqual(1, eject.count("rdx_hardware_fail_eject();"))
        self.assertIn(
            "buffer[15] = rdx_hardware_get_temperature_celsius();",
            MANAGER,
        )


if __name__ == "__main__":
    unittest.main()
