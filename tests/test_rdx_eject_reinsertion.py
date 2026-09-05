# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts preserving insertion edges until mechanism work finishes."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
HARDWARE = (PROJECT_DIR / "src" / "rdx_mount" / "rdx_hardware.c").read_text(
    encoding="utf-8"
)


def function_body(source, signature):
    """Return a C function or conditional block using balanced braces."""
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
    raise AssertionError("unterminated C block: {}".format(signature))


class RdxEjectReinsertionTests(unittest.TestCase):
    """Cover insertion, removal, and completion around the debounce commit."""

    @classmethod
    def setUpClass(cls):
        """Extract the production cartridge polling function once."""
        cls.poll = function_body(
            HARDWARE,
            "static void rdx_hardware_service_cartridge_input(UINT32_T now)",
        )
        cls.busy_guard = (
            "if (raw_present &&\n"
            "            (rdx_hardware.eject_state != RDX_EJECT_IDLE))"
        )

    def test_busy_motion_still_tracks_each_raw_candidate(self):
        """Insertion or removal during motion still starts its debounce timer."""
        self.assertIn(self.busy_guard, self.poll)
        candidate = function_body(
            self.poll, "if (raw_present != rdx_hardware.cartridge_candidate)"
        )
        self.assertIn("rdx_hardware.cartridge_candidate = raw_present;", candidate)
        self.assertIn("rdx_hardware.cartridge_candidate_since = now;", candidate)
        self.assertNotIn("eject_state", candidate)
        self.assertLess(self.poll.index(candidate), self.poll.index(self.busy_guard))
        self.assertLess(
            self.poll.index("rdx_hardware_interval_elapsed("),
            self.poll.index(self.busy_guard),
        )

    def test_present_edge_is_not_consumed_while_waiting_or_settling(self):
        """Every non-idle eject phase defers committing a present transition."""
        self.assertIn(self.busy_guard, self.poll)
        guard = function_body(self.poll, self.busy_guard)
        self.assertRegex(guard, r"^\{\s*return;\s*\}$")
        for commit in (
            "rdx_hardware.cartridge_stable = raw_present;",
            "rdx_hardware.ejected_latched = FALSE;",
            "rdx_hardware.logical_unloaded = FALSE;",
            "rdx_hardware.eject_fault_latched = FALSE;",
            "rdx_hardware.thermal_phase = RDX_THERMAL_INITIAL;",
            "gio_rdx_mechanism_auxiliary_set(TRUE);",
        ):
            with self.subTest(commit=commit):
                self.assertLess(self.poll.index(guard), self.poll.index(commit))

    def test_absent_edge_is_committed_even_during_motion(self):
        """Only a present sample can enter the early-return guard."""
        self.assertIn(self.busy_guard, self.poll)
        self.assertEqual(1, len(re.findall(r"\breturn\s*;", self.poll)))
        removal = function_body(self.poll, "else\n        {")
        self.assertIn("rdx_manager_clear_host_eject_policy();", removal)
        self.assertIn("gio_rdx_mechanism_auxiliary_set(FALSE);", removal)
        self.assertIn("RDX_THERMAL_MEDIA_ABSENT", removal)
        self.assertNotIn("eject_state", removal)
        self.assertLess(
            self.poll.index("rdx_hardware.cartridge_stable = raw_present;"),
            self.poll.index(removal),
        )

    def test_first_idle_pass_reuses_the_completed_debounce(self):
        """Completion leaves the pending candidate and its elapsed time intact."""
        self.assertIn(self.busy_guard, self.poll)
        guard = function_body(self.poll, self.busy_guard)
        self.assertNotIn("cartridge_candidate", guard)
        self.assertNotIn("cartridge_stable", guard)
        self.assertIn("raw_present != rdx_hardware.cartridge_stable", self.poll)
        self.assertEqual(1, self.poll.count("cartridge_candidate_since = now;"))
        for signature in (
            "static void rdx_hardware_finish_eject(void)",
            "static void rdx_hardware_fail_eject(void)",
        ):
            with self.subTest(signature=signature):
                completion = function_body(HARDWARE, signature)
                self.assertIn("rdx_hardware.eject_state = RDX_EJECT_IDLE;", completion)
                self.assertNotIn("cartridge_candidate", completion)
                self.assertNotIn("cartridge_stable", completion)

    def test_deferred_insertion_publishes_a_fresh_media_session(self):
        """The accepted edge reenables the auxiliary output and clears stale state."""
        insertion = function_body(self.poll, "if (raw_present)\n")
        self.assertIn("gio_rdx_mechanism_auxiliary_set(TRUE);", insertion)
        self.assertIn("rdx_hardware.observed_init_failed = FALSE;", insertion)
        self.assertIn("rdx_hardware.thermal_phase = RDX_THERMAL_INITIAL;", insertion)
        self.assertIn("rdx_led_set_steady(RDX_LED_CARTRIDGE, TRUE);", insertion)


if __name__ == "__main__":
    unittest.main()
