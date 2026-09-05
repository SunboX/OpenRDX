# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source-contract checks for the RDX PWM and GPIO port."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
INCLUDE_DIR = PROJECT_DIR / "include" / "rdx_mount"
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"


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
    """Require each supplied statement to appear after the preceding one."""
    cursor = 0
    for statement in statements:
        position = body.find(statement, cursor)
        test_case.assertNotEqual(
            -1,
            position,
            "missing or out-of-order statement: {}".format(statement),
        )
        cursor = position + len(statement)


class RdxPwmGpioTests(unittest.TestCase):
    """Lock the required PWM arithmetic and validated RDX pin roles."""

    @classmethod
    def setUpClass(cls):
        """Load the four hardware-abstraction files once for this suite."""
        cls.pwm_header = (INCLUDE_DIR / "pwm.h").read_text(encoding="utf-8")
        cls.pwm_source = (SOURCE_DIR / "pwm.c").read_text(encoding="utf-8")
        cls.gio_header = (INCLUDE_DIR / "gio.h").read_text(encoding="utf-8")
        cls.gio_source = (SOURCE_DIR / "gio.c").read_text(encoding="utf-8")

    def test_pwm_uses_period_and_duty_arithmetic(self):
        """Use raw clock ticks and divide-before-multiply duty math."""
        body = function_body(
            self.pwm_source,
            "void pwm_run(UINT32_T pwm_num, UINT32_T duty_cycle_percentage, "
            "UINT32_T period_us)",
        )
        assert_statements_in_order(
            self,
            body,
            (
                "period_ticks = rti_clock_mhz * period_us;",
                "WRITE32(PWM_PCR_REG_OFF(pwm_num), PWM_PCR_FREE_RUN_BIT);",
                "WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_RUNNING);",
                "WRITE32(PWM_START_REG_OFF(pwm_num), PWM_START_BIT);",
                "WRITE32(PWM_PER_REG_OFF(pwm_num), period_ticks);",
                "(period_ticks / 100U) * duty_cycle_percentage",
            ),
        )
        self.assertNotIn("period_ticks - 1", body)
        self.assertIn("#define PWM_CFG_RDX_IDLE", self.pwm_header)
        self.assertIn("#define PWM_CFG_RDX_RUNNING", self.pwm_header)

    def test_pwm_disable_restores_idle_level(self):
        """Clear duty before restoring the PWM channel to idle CFG 0x10."""
        body = function_body(self.pwm_source, "void pwm_disable(UINT32_T pwm_num)")
        assert_statements_in_order(
            self,
            body,
            (
                "WRITE32(PWM_PH1D_REG_OFF(pwm_num), 0U);",
                "WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_IDLE);",
            ),
        )

    def test_header_exposes_validated_rdx_board_mapping(self):
        """Bind every required standard and SCI GPIO to its physical number."""
        expected = {
            "RDX_EJECT_BUTTON_GIO_NUM": "1",
            "RDX_MECHANISM_INPUT_GIO_NUM": "2",
            "RDX_MOTOR_CONTROL_GIO_NUM": "3",
            "RDX_CARTRIDGE_PRESENT_GIO_NUM": "5",
            "RDX_CARTRIDGE_AMBER_GIO_NUM": "6",
            "RDX_CARTRIDGE_GREEN_GIO_NUM": "7",
            "RDX_DOCK_AMBER_GIO_NUM": "8",
            "RDX_DOCK_GREEN_GIO_NUM": "9",
        }
        for name, value in expected.items():
            with self.subTest(name=name):
                self.assertRegex(
                    self.gio_header,
                    rf"(?m)^\s*{re.escape(name)}\s*=\s*{value},?$",
                )
        self.assertIn(
            "RDX_MOTOR_GATE_GIO_NUM                = "
            "RDX_MOTOR_CONTROL_GIO_NUM",
            self.gio_header,
        )
        self.assertIn("RDX_MECHANISM_AUXILIARY_GIO_NUM       = 11", self.gio_header)

    def test_gpio_init_preloads_safe_outputs_and_keeps_button_polled(self):
        """Latch motor/LED off levels before direction and enable no eject IRQ."""
        body = function_body(self.gio_source, "void gio_init(void)")
        assert_statements_in_order(
            self,
            body,
            (
                "WRITE32(GIOENACLR_REG_OFF, 0xFFU);",
                "MODIFY32(GIOOUT0_REG_OFF, RDX_STD_OUTPUT_MASK,",
                "RDX_STD_HIGH_PRELOAD_MASK);",
                "WRITE32(GIODIR0_REG_OFF, RDX_STD_OUTPUT_MASK);",
                "MODIFY_REG32(SCI_PIO3, RDX_SCI_LED_MASK, "
                "RDX_SCI_LED_MASK);",
                "MODIFY_REG32(SCI_PIO1, RDX_SCI_LED_MASK, "
                "RDX_SCI_LED_MASK);",
                "MODIFY_REG32(SCI_PIO0, RDX_SCI_LED_MASK, 0U);",
                "MODIFY_REG32(SPI_PC0, RDX_MECHANISM_AUXILIARY_MASK, 0U);",
                "MODIFY_REG32(SPI_PC3, RDX_MECHANISM_AUXILIARY_MASK, 0U);",
                "MODIFY_REG32(SPI_PC1, RDX_MECHANISM_AUXILIARY_MASK,",
            ),
        )
        self.assertNotIn("gio_input_setup(INPUT_BUTTON_GIO_NUM, TRUE)", body)
        self.assertNotIn("one_touch_button_pressed", self.gio_source)

    def test_unassigned_profile_outputs_do_not_claim_gpio0(self):
        """Keep logical-output sentinels from becoming a physical GPIO0 drive."""
        self.assertNotIn("RDX_PROFILE_AUXILIARY_GIO_NUM", self.gio_header)
        self.assertNotIn("RDX_PROFILE_AUXILIARY_GIO_NUM", self.gio_source)
        self.assertNotIn("gio_rdx_profile_mechanism_auxiliary_set", self.gio_header)
        self.assertNotIn("gio_rdx_profile_mechanism_auxiliary_set", self.gio_source)
        self.assertEqual(
            "RDX_STD_HIGH_PRELOAD_MASK",
            re.search(
                r"(?m)^#define RDX_STD_OUTPUT_MASK\s+(.+)$", self.gio_source
            ).group(1),
        )

    def test_rdx_inputs_and_mechanism_control_use_explicit_logic(self):
        """Expose active-low polling and explicit GPIO3 control levels."""
        self.assertIn(
            "gio_get_state(RDX_EJECT_BUTTON_GIO_NUM) == 0U",
            function_body(
                self.gio_source, "BOOLEAN_T gio_rdx_eject_button_pressed(void)"
            ),
        )
        self.assertIn(
            "gio_get_state(RDX_CARTRIDGE_PRESENT_GIO_NUM) == 0U",
            function_body(
                self.gio_source, "BOOLEAN_T gio_rdx_cartridge_present(void)"
            ),
        )
        motor_body = function_body(
            self.gio_source,
            "void gio_rdx_motor_control_set_high(BOOLEAN_T high)",
        )
        assert_statements_in_order(
            self,
            motor_body,
            (
                "gio_high(RDX_MOTOR_CONTROL_GIO_NUM);",
                "gio_low(RDX_MOTOR_CONTROL_GIO_NUM);",
            ),
        )
        wrapper = function_body(
            self.gio_source, "void gio_rdx_motor_gate_enable(BOOLEAN_T enable)"
        )
        self.assertIn(
            "gio_rdx_motor_control_set_high(enable ? FALSE : TRUE);", wrapper
        )


if __name__ == "__main__":
    unittest.main()
