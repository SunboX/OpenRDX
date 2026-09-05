"""Behavior and scope checks for the standalone RDX eject-test firmware."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
EJECT_HEADER = PROJECT_DIR / "include" / "eject_test.h"
EJECT_SOURCE = PROJECT_DIR / "src" / "main.c"
STARTUP_SOURCE = PROJECT_DIR / "src" / "startup.c"
BUILD_ADAPTER = PROJECT_DIR / "scripts" / "ti_cgt_build.py"


class EjectTestFirmwareTests(unittest.TestCase):
    """Lock the recovered button mapping and direct motor-pulse behavior."""

    def test_recovered_button_motor_and_timing_constants(self):
        """Bind the test to the original payload's GPIO and timing values."""
        self.assertTrue(EJECT_HEADER.is_file(), "include/eject_test.h is missing")
        header = EJECT_HEADER.read_text(encoding="utf-8")
        expected_definitions = {
            "EJECT_BUTTON_MASK": "(1UL << 1)",
            "MOTOR_CONTROL_MASK": "(1UL << 3)",
            "MOTOR_DEAD_TIME_US": "50UL",
            "MOTOR_RUN_TIME_MS": "2000UL",
            "MOTOR_PWM_PERIOD_US": "50UL",
        }

        for name, value in expected_definitions.items():
            with self.subTest(name=name):
                self.assertRegex(
                    header,
                    rf"(?m)^#define {name}\s+{re.escape(value)}$",
                )

        self.assertIn("#define EJECT_BUTTON_ASSERTED 0UL", header)
        self.assertIn("#define PWM_CFG_PHASE1_OUTPUT_HIGH 0x10UL", header)
        self.assertIn("#define PWM_CFG_MODE_CONTINUOUS 2UL", header)
        self.assertIn("#define PWM_CFG_MOTOR_BITS 0x23UL", header)

    def test_pwm1_preserves_the_recovered_phase_one_high_level(self):
        """Keep PWM1 idle at 0x10 and run it at the recovered 0x12."""
        source = EJECT_SOURCE.read_text(encoding="utf-8")
        self.assertIn("PWM1_CFG = PWM_CFG_PHASE1_OUTPUT_HIGH;", source)
        self.assertIn(
            "PWM1_CFG = PWM_CFG_PHASE1_OUTPUT_HIGH | "
            "PWM_CFG_MODE_CONTINUOUS;",
            source,
        )
        self.assertNotIn("PWM1_CFG |= PWM_CFG_MODE_CONTINUOUS;", source)

    def test_gpio3_is_preloaded_off_before_output_enable(self):
        """Avoid a motor glitch while GPIO direction changes at startup."""
        source = EJECT_SOURCE.read_text(encoding="utf-8")
        required_statements = (
            "GIOOUT0 |= MOTOR_CONTROL_MASK;",
            "GIODIR0 &= ~EJECT_BUTTON_MASK;",
            "GIODIR0 |= MOTOR_CONTROL_MASK;",
        )
        for statement in required_statements:
            self.assertIn(statement, source)

        self.assertLess(
            source.index("GIOOUT0 |= MOTOR_CONTROL_MASK;"),
            source.index("GIODIR0 |= MOTOR_CONTROL_MASK;"),
        )

    def test_motor_pulse_retains_recovered_start_run_stop_order(self):
        """Run GPIO3/PWM1 for two seconds with recovered 50 us dead times."""
        source = EJECT_SOURCE.read_text(encoding="utf-8")
        expected_order = (
            "PWM1_PH1D = 0UL;",
            "PWM1_CFG = PWM_CFG_PHASE1_OUTPUT_HIGH;",
            "GIOOUT0 &= ~MOTOR_CONTROL_MASK;",
            "eject_delay_us(MOTOR_DEAD_TIME_US);",
            "PWM1_CFG = PWM_CFG_PHASE1_OUTPUT_HIGH | "
            "PWM_CFG_MODE_CONTINUOUS;",
            "PWM1_START = PWM_START_BIT;",
            "PWM1_PER = motor_pwm_period_ticks;",
            "PWM1_PH1D = motor_pwm_period_ticks;",
            "eject_delay_ms(MOTOR_RUN_TIME_MS);",
            "PWM1_PH1D = 0UL;",
            "PWM1_CFG &= ~PWM_CFG_MOTOR_BITS;",
            "eject_delay_us(MOTOR_DEAD_TIME_US);",
            "GIOOUT0 |= MOTOR_CONTROL_MASK;",
        )
        cursor = 0
        for statement in expected_order:
            next_position = source.find(statement, cursor)
            self.assertNotEqual(
                -1,
                next_position,
                f"missing or out-of-order statement: {statement}",
            )
            cursor = next_position + len(statement)

    def test_main_triggers_once_then_waits_for_button_release(self):
        """Prevent a held active-low button from retriggering the motor."""
        source = EJECT_SOURCE.read_text(encoding="utf-8")
        expected_loop = (
            "for (;;) {\n"
            "        while (eject_button_pressed() == 0UL) {\n"
            "        }\n"
            "        eject_run_motor_pulse();\n"
            "        while (eject_button_pressed() != 0UL) {\n"
            "        }\n"
            "    }"
        )
        self.assertIn(expected_loop, source)
        self.assertIn(
            "return (GIOIN0 & EJECT_BUTTON_MASK) == EJECT_BUTTON_ASSERTED;",
            source,
        )

    def test_build_contains_only_the_eject_runtime_sources(self):
        """Exclude production USB, SATA, cartridge, flash, and core code."""
        adapter = BUILD_ADAPTER.read_text(encoding="utf-8")
        self.assertIn('C_SOURCE_NAMES = ["main.c", "startup.c"]', adapter)
        self.assertIn(
            'ASM_SOURCE_NAMES = ["exceptions_isr.asm", "intvecs.asm"]',
            adapter,
        )
        self.assertTrue(STARTUP_SOURCE.is_file(), "src/startup.c is missing")

        removed_paths = (
            "src/rdx_core.c",
            "src/usb_stack.c",
            "src/ahci.c",
            "src/spi.c",
            "linker/firmware_absolute_symbols.cmd",
        )
        for relative_path in removed_paths:
            with self.subTest(path=relative_path):
                self.assertFalse(
                    (PROJECT_DIR / relative_path).exists(),
                    f"production path still exists: {relative_path}",
                )


if __name__ == "__main__":
    unittest.main()
