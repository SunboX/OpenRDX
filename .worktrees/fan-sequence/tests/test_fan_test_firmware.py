"""Behavior and scope checks for the standalone RDX fan-test firmware."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
FAN_HEADER = PROJECT_DIR / "include" / "fan_test.h"
FAN_SOURCE = PROJECT_DIR / "src" / "main.c"
STARTUP_SOURCE = PROJECT_DIR / "src" / "startup.c"
BUILD_ADAPTER = PROJECT_DIR / "scripts" / "ti_cgt_build.py"


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
    """Require statements to appear in the supplied order."""
    cursor = 0
    for statement in statements:
        position = body.find(statement, cursor)
        test_case.assertNotEqual(
            -1,
            position,
            "missing or out-of-order statement: {}".format(statement),
        )
        cursor = position + len(statement)


class FanTestFirmwareTests(unittest.TestCase):
    """Lock the recovered button mapping and direct fan-pulse behavior."""

    def test_recovered_button_fan_and_timing_constants(self):
        """Bind the test to the original payload's GPIO, PWM, and timing values."""
        self.assertTrue(FAN_HEADER.is_file(), "include/fan_test.h is missing")
        header = FAN_HEADER.read_text(encoding="utf-8")
        expected_definitions = {
            "EJECT_BUTTON_MASK": "(1UL << 1)",
            "EJECT_BUTTON_ASSERTED": "0UL",
            "FAN_PWM_PERIOD_US": "16000UL",
            "FAN_RUN_TIME_MS": "1000UL",
            "PWM_CFG_FAN_RUNNING": "0x12UL",
            "PWM_CFG_FAN_BITS": "0x13UL",
        }

        for name, value in expected_definitions.items():
            with self.subTest(name=name):
                self.assertRegex(
                    header,
                    rf"(?m)^#define {name}\s+{re.escape(value)}$",
                )

        self.assertIn("#define PWM0_PCR MMIO32(0xFFF78804UL)", header)
        self.assertIn("#define PWM0_PH1D MMIO32(0xFFF78818UL)", header)

    def test_constant_definition_pattern_accepts_standard_whitespace(self):
        """Match normal C whitespace between a constant name and its value."""
        self.assertRegex(
            "#define FAN_RUN_TIME_MS 1000UL",
            rf"(?m)^#define FAN_RUN_TIME_MS\s+{re.escape('1000UL')}$",
        )

    def test_gpio1_is_the_only_active_low_button_input(self):
        """Poll only recovered GPIO1 and interpret its low level as pressed."""
        source = FAN_SOURCE.read_text(encoding="utf-8")
        button_body = function_body(
            source, "static uint32_t eject_button_pressed(void)"
        )
        self.assertIn(
            "return (GIOIN0 & EJECT_BUTTON_MASK) == EJECT_BUTTON_ASSERTED;",
            button_body,
        )
        self.assertEqual(1, source.count("GIOIN0"))

    def test_hardware_init_configures_only_button_timer_and_pwm0(self):
        """Initialize the recovered fan output off and one millisecond timer."""
        source = FAN_SOURCE.read_text(encoding="utf-8")
        body = function_body(source, "static void fan_hardware_init(void)")
        assert_statements_in_order(
            self,
            body,
            (
                "fan_pwm_period_ticks = clock_mhz * FAN_PWM_PERIOD_US;",
                "GIOGCR0 = 1UL;",
                "GIODIR0 &= ~EJECT_BUTTON_MASK;",
                "GIOPULDIS0 |= EJECT_BUTTON_MASK;",
                "PWM0_PCR = PWM_PCR_FREE_RUN_BIT;",
                "PWM0_PER = fan_pwm_period_ticks - 1UL;",
                "PWM0_PH1D = 0UL;",
                "PWM0_CFG = 0UL;",
                "fan_timer_init(clock_mhz);",
            ),
        )
        timer_body = function_body(source, "static void fan_timer_init(uint32_t clock_mhz)")
        assert_statements_in_order(
            self,
            timer_body,
            (
                "RTIGCTRL = 0UL;",
                "RTIFRC1 = 0UL;",
                "RTICPUC1 = (clock_mhz * 1000UL) - 1UL;",
                "RTIGCTRL = RTI_COUNTER_ONE_ENABLE;",
            ),
        )

    def test_fan_pulse_runs_pwm0_full_for_one_second_then_stops(self):
        """Retain recovered PWM0 period and direct start-run-stop order."""
        source = FAN_SOURCE.read_text(encoding="utf-8")
        start_body = function_body(source, "static void fan_start(void)")
        assert_statements_in_order(
            self,
            start_body,
            (
                "PWM0_PER = fan_pwm_period_ticks - 1UL;",
                "PWM0_PH1D = fan_pwm_period_ticks;",
                "PWM0_CFG = PWM_CFG_FAN_RUNNING;",
                "PWM0_START = PWM_START_BIT;",
            ),
        )
        pulse_body = function_body(source, "static void fan_run_pulse(void)")
        assert_statements_in_order(
            self,
            pulse_body,
            (
                "fan_start();",
                "fan_delay_ms(FAN_RUN_TIME_MS);",
                "fan_stop();",
            ),
        )
        stop_body = function_body(source, "static void fan_stop(void)")
        assert_statements_in_order(
            self,
            stop_body,
            (
                "PWM0_PH1D = 0UL;",
                "PWM0_CFG &= ~PWM_CFG_FAN_BITS;",
            ),
        )

    def test_main_triggers_once_then_waits_for_button_release(self):
        """Prevent a held active-low button from retriggering the fan."""
        source = FAN_SOURCE.read_text(encoding="utf-8")
        body = function_body(source, "void main(void)")
        expected_loop = (
            "for (;;) {\n"
            "        while (eject_button_pressed() == 0UL) {\n"
            "        }\n"
            "        fan_run_pulse();\n"
            "        while (eject_button_pressed() != 0UL) {\n"
            "        }\n"
            "    }"
        )
        self.assertIn(expected_loop, body)

    def test_build_contains_only_the_fan_runtime_sources(self):
        """Exclude all production USB, SATA, cartridge, thermal, and core code."""
        adapter = BUILD_ADAPTER.read_text(encoding="utf-8")
        self.assertIn('C_SOURCE_NAMES = ["main.c", "startup.c"]', adapter)
        self.assertIn(
            'ASM_SOURCE_NAMES = ["exceptions_isr.asm", "intvecs.asm"]',
            adapter,
        )
        self.assertTrue(STARTUP_SOURCE.is_file(), "src/startup.c is missing")
        self.assertEqual(
            ["fan_test.h"],
            sorted(path.name for path in (PROJECT_DIR / "include").rglob("*.h")),
        )
        self.assertEqual(
            ["main.c", "startup.c"],
            sorted(path.name for path in (PROJECT_DIR / "src").glob("*.c")),
        )

        removed_paths = (
            "src/rdx_core.c",
            "src/usb_stack.c",
            "src/ahci.c",
            "src/spi.c",
            "src/pwm.c",
            "linker/firmware_absolute_symbols.cmd",
        )
        for relative_path in removed_paths:
            with self.subTest(path=relative_path):
                self.assertFalse(
                    (PROJECT_DIR / relative_path).exists(),
                    "production path still exists: {}".format(relative_path),
                )


if __name__ == "__main__":
    unittest.main()
