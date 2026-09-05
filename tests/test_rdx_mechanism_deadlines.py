# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Regression contracts for motor endpoint sampling and deadline priority."""

from pathlib import Path
import re
import unittest

from test_rdx_hardware_runtime import function_body, assert_statements_in_order


SOURCE = (
    Path(__file__).resolve().parents[1] / "src/rdx_mount/rdx_mechanism.c"
).read_text(encoding="utf-8")


class RdxMechanismDeadlineTests(unittest.TestCase):
    """Keep sensor history and elapsed travel ahead of optional bus work."""

    def test_recovery_gpio0_assertion_uses_vendor_profile_classes(self):
        """Only vendor class 2 (35h/37h) asserts mapped logical output 11."""
        enter = function_body(
            SOURCE,
            "static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,",
        )
        recovery = enter[
            enter.index("case RDX_MECHANISM_RECOVERY_DRIVE_WAIT_ENDPOINT:"):
            enter.index("case RDX_MECHANISM_RETURN_SETTLE:")
        ]
        pwm_branch = function_body(
            recovery, "if (rdx_mechanism.hardware_profile != 0x36U)"
        )
        auxiliary_branch = function_body(
            pwm_branch,
            "if ((rdx_mechanism.hardware_profile == 0x35U) ||",
        )
        self.assertIn(
            "(rdx_mechanism.hardware_profile == 0x37U))", pwm_branch
        )
        self.assertEqual(
            "gio_rdx_profile_mechanism_auxiliary_set(TRUE);",
            auxiliary_branch[1:-1].strip(),
        )
        assert_statements_in_order(
            self, pwm_branch,
            ("gio_rdx_profile_mechanism_auxiliary_set(TRUE);", "pwm_run("),
        )
        self.assertEqual(
            1, SOURCE.count("gio_rdx_profile_mechanism_auxiliary_set(TRUE);")
        )

    def test_recovery_phase_drives_gpio3_low_as_in_vendor_binary(self):
        """State 6 passes logical zero, including the translated missing argument."""
        enter = function_body(
            SOURCE,
            "static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,",
        )
        recovery_state = re.search(
            r"(RDX_MECHANISM_[A-Z_]+)\s*=\s*6\b", SOURCE
        ).group(1)
        recovery = enter[
            enter.index("case " + recovery_state + ":"):
            enter.index("case RDX_MECHANISM_RETURN_SETTLE:")
        ]
        # Vendor 0283 at 08003BD8 sets r1=0; 08003BDC sets r0=10;
        # the BL at 08003BE2 passes those values to spi_set_logical_output.
        # The translated C omitted r1 and previously led to a wrong high level.
        assert_statements_in_order(
            self,
            recovery,
            (
                "gio_rdx_motor_control_set_high(FALSE);",
                "usleep(RDX_MECHANISM_DEAD_TIME_US);",
                "pwm_run(RDX_MOTOR_PWM_NUM, 100U,",
            ),
        )
        self.assertNotIn("gio_rdx_motor_control_set_high(TRUE)", recovery)

    def test_each_drive_observes_initial_endpoint_level_before_power(self):
        """Retain a released switch even if it closes before the first poll."""
        reset = function_body(
            SOURCE, "static void rdx_mechanism_reset_transition(void)"
        )
        self.assertIn("!gio_rdx_mechanism_input_asserted()", reset)
        self.assertNotIn("mechanism_deasserted_seen = FALSE;", reset)
        enter = function_body(
            SOURCE,
            "static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,",
        )
        forward = enter[
            enter.index("case RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA:"):
            enter.index("case RDX_MECHANISM_NO_MEDIA_GRACE:")
        ]
        reverse = enter[
            enter.index("case RDX_MECHANISM_RECOVERY_DRIVE_WAIT_ENDPOINT:"):
            enter.index("case RDX_MECHANISM_RETURN_SETTLE:")
        ]
        for phase, power in (
            (forward, "rdx_mechanism_run_motor(FALSE);"),
            (reverse, "pwm_run(RDX_MOTOR_PWM_NUM"),
        ):
            with self.subTest(power=power):
                assert_statements_in_order(
                    self, phase, ("rdx_mechanism_reset_transition();", power)
                )

    def test_interrupt_owner_is_published_before_motor_output(self):
        """SATA error interrupts must defer recovery as soon as travel begins."""
        enter = function_body(
            SOURCE,
            "static void rdx_mechanism_enter_state(RDX_MECHANISM_STATE_T state,",
        )
        self.assertIn("volatile RDX_MECHANISM_STATE_T state;", SOURCE)
        assert_statements_in_order(
            self,
            enter,
            ("rdx_mechanism.state = state;", "switch (state)",
             "rdx_mechanism_run_motor(FALSE);"),
        )

    def test_expired_travel_stops_before_sampling_adc_or_rearming_grace(self):
        """An elapsed motor deadline cannot be extended by media disappearance."""
        service = function_body(
            SOURCE, "RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now)"
        )
        forward = service[
            service.index("case RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA:"):
            service.index("case RDX_MECHANISM_NO_MEDIA_GRACE:")
        ]
        assert_statements_in_order(
            self,
            forward,
            (
                "rdx_mechanism_transition_complete()",
                "rdx_mechanism_deadline_reached(now,",
                "rdx_mechanism_enter_state(RDX_MECHANISM_RETURN_PAUSE, now);",
                "rdx_mechanism_media_context_absent()",
                "rdx_mechanism_enter_state(RDX_MECHANISM_NO_MEDIA_GRACE, now);",
            ),
        )

    def test_present_digital_media_context_needs_no_adc_transaction(self):
        """Either digital presence input makes the absence predicate false."""
        context = function_body(
            SOURCE, "static BOOLEAN_T rdx_mechanism_media_context_absent(void)"
        )
        assert_statements_in_order(
            self,
            context,
            (
                "cartridge_present = gio_rdx_cartridge_present();",
                "if (cartridge_present || sata_link_active)",
                "return FALSE;",
                "adc_status = rdx_mcp3008_read_channel(",
            ),
        )


if __name__ == "__main__":
    unittest.main()
