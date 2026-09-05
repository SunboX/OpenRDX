# SPDX-FileCopyrightText: 2026 André Fiedler
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
                "WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_IDLE);",
                "WRITE32(PWM_PER_REG_OFF(pwm_num), period_ticks);",
                "(period_ticks / 100U) * duty_cycle_percentage",
                "WRITE32(PWM_PCR_REG_OFF(pwm_num), PWM_PCR_FREE_RUN_BIT);",
                "WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_RUNNING);",
                "WRITE32(PWM_START_REG_OFF(pwm_num), PWM_START_BIT);",
            ),
        )
        self.assertNotIn("period_ticks - 1", body)
        self.assertIn("#define PWM_CFG_RDX_IDLE", self.pwm_header)
        self.assertIn("#define PWM_CFG_RDX_RUNNING", self.pwm_header)

    def test_pwm_disable_restores_idle_level(self):
        """Force the idle output before changing the buffered high duration."""
        body = function_body(self.pwm_source, "void pwm_disable(UINT32_T pwm_num)")
        assert_statements_in_order(
            self,
            body,
            (
                "WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_IDLE);",
                "WRITE32(PWM_PH1D_REG_OFF(pwm_num), 0U);",
            ),
        )

    def test_pwm_start_cannot_capture_an_unconfigured_period(self):
        """Load both timing registers before the final START register write."""
        body = function_body(
            self.pwm_source,
            "void pwm_run(UINT32_T pwm_num, UINT32_T duty_cycle_percentage, "
            "UINT32_T period_us)",
        )
        writes = re.findall(r"WRITE32\((PWM_\w+_REG_OFF)\(pwm_num\)", body)
        self.assertEqual(
            [
                "PWM_CFG_REG_OFF",
                "PWM_PER_REG_OFF",
                "PWM_PH1D_REG_OFF",
                "PWM_PCR_REG_OFF",
                "PWM_CFG_REG_OFF",
                "PWM_START_REG_OFF",
            ],
            writes,
        )

    def test_header_exposes_validated_rdx_board_mapping(self):
        """Bind every required standard and SCI GPIO to its physical number."""
        expected = {
            "RDX_PROFILE_AUXILIARY_GIO_NUM": "0",
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

    def test_gpio0_output_waits_for_profile_identity(self):
        """Keep GPIO0 undriven until its profile determines the preload level."""
        body = function_body(self.gio_source, "void gio_init(void)")
        self.assertIn("rdx_profile_mechanism_auxiliary_available = FALSE;", body)
        self.assertNotIn("rdx_manager_get_hardware_profile", body)
        self.assertNotIn("RDX_PROFILE_AUXILIARY_GIO_NUM", body)
        self.assertEqual(
            "RDX_STD_HIGH_PRELOAD_MASK",
            re.search(
                r"(?m)^#define RDX_STD_OUTPUT_MASK\s+(.+)$", self.gio_source
            ).group(1),
        )

    def test_profile_gpio0_preload_precedes_output_direction(self):
        """Map selector zero to an output: high for 38h and low otherwise."""
        body = function_body(
            self.gio_source,
            "void gio_rdx_profile_outputs_init(UINT16_T hardware_profile)",
        )
        high_branch = function_body(body, "if (hardware_profile == 0x38U)")
        low_branch = function_body(body, "else")
        self.assertIn("gio_high(RDX_PROFILE_AUXILIARY_GIO_NUM);", high_branch)
        self.assertNotIn("gio_low", high_branch)
        self.assertIn("gio_low(RDX_PROFILE_AUXILIARY_GIO_NUM);", low_branch)
        self.assertNotIn("gio_high", low_branch)
        assert_statements_in_order(
            self,
            body,
            (
                "rdx_profile_mechanism_auxiliary_available = FALSE;",
                "gio_high(RDX_PROFILE_AUXILIARY_GIO_NUM);",
                "gio_low(RDX_PROFILE_AUXILIARY_GIO_NUM);",
                "gio_set_direction(RDX_PROFILE_AUXILIARY_GIO_NUM, "
                "GIO_DIR_OUTPUT);",
                "(hardware_profile != 0x38U) ? TRUE : FALSE;",
            ),
        )

    def test_logical11_cannot_lower_profile38_gpio0(self):
        """Protect logical output 14 from mechanism logical-11 stop writes."""
        body = function_body(
            self.gio_source,
            "void gio_rdx_profile_mechanism_auxiliary_set(BOOLEAN_T asserted)",
        )
        unavailable = function_body(
            body, "if (!rdx_profile_mechanism_auxiliary_available)"
        )
        self.assertRegex(unavailable, r"^\{\s*return;\s*\}$")
        assert_statements_in_order(
            self,
            body,
            (
                "if (!rdx_profile_mechanism_auxiliary_available)",
                "return;",
                "if (asserted)",
                "gio_high(RDX_PROFILE_AUXILIARY_GIO_NUM);",
                "gio_low(RDX_PROFILE_AUXILIARY_GIO_NUM);",
            ),
        )
        self.assertIn(
            "static BOOLEAN_T rdx_profile_mechanism_auxiliary_available = FALSE;",
            self.gio_source,
        )

    def test_gpio0_is_not_an_unassigned_selector(self):
        """Allow physical selector zero through output and direction helpers."""
        for signature in (
            "void gio_high(UINT32_T gio_num)",
            "void gio_low(UINT32_T gio_num)",
            "void gio_set_direction(UINT32_T gio_num, GIO_DIR_T dir)",
        ):
            with self.subTest(signature=signature):
                body = function_body(self.gio_source, signature)
                standard_gpio = function_body(body, "if (gio_num < NUM_STD_GIOS)")
                self.assertIn("(1 << gio_num)", standard_gpio)
                self.assertNotRegex(body, r"gio_num\s*(?:>|!=)\s*0")

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
