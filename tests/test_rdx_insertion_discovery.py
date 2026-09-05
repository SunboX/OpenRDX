# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Contracts for GPIO-confirmed insertion after an empty SATA discovery."""

from pathlib import Path
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"


def function_body(source, signature):
    """Return a C function or conditional block using balanced braces."""
    body_start = source.index("{", source.index(signature))
    depth = 0
    for index in range(body_start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[body_start : index + 1]
    raise AssertionError("unterminated C block: {}".format(signature))


class RdxInsertionDiscoveryTests(unittest.TestCase):
    """Keep physical insertion independent of a second SATA connect interrupt."""

    @classmethod
    def setUpClass(cls):
        """Read the insertion, deferred discovery, and media admission sources."""
        cls.ahci = (SOURCE_DIR / "ahci.c").read_text(encoding="utf-8")
        cls.hardware = (SOURCE_DIR / "rdx_hardware.c").read_text(encoding="utf-8")
        cls.poll = function_body(
            cls.hardware,
            "static void rdx_hardware_service_cartridge_input(UINT32_T now)",
        )

    def test_insertion_rearms_discovery_after_a_consumed_empty_attempt(self):
        """A DET=3 empty-bay failure must not require another connect-change IRQ."""
        service = function_body(self.ahci, "void ahci_service(void)")
        self.assertIn("if (!ahci_hotplug_pending[port_num])", service)
        self.assertLess(
            service.index("ahci_hotplug_pending[port_num] = FALSE;"),
            service.index("status = ahci_init_port(port_num);"),
        )
        insertion = function_body(self.poll, "if (raw_present)\n")
        self.assertIn("ahci_media_inserted(0U);", insertion)
        self.assertLess(
            insertion.index("gio_rdx_mechanism_auxiliary_set(TRUE);"),
            insertion.index("ahci_media_inserted(0U);"),
        )
        self.assertNotIn("PxSSTS", insertion)
        self.assertNotIn("PORT_CONNECT_CHANGE_STATUS", insertion)

    def test_rearming_checks_readiness_with_both_sata_handlers_masked(self):
        """An already-admitted cartridge is never invalidated by late debounce."""
        self.assertIn("void ahci_media_inserted(UINT32_T port_num)", self.ahci)
        notify = function_body(self.ahci, "void ahci_media_inserted(UINT32_T port_num)")
        absent = function_body(notify, "if (!ata_dev[port_num].bDeviceInitComplete)")
        self.assertIn("ahci_schedule_media_discovery(port_num, TRUE);", absent)
        self.assertLess(
            notify.index("AHCI_MEDIA_PUBLICATION_INTERRUPT_MASK);"),
            notify.index("if (!ata_dev[port_num].bDeviceInitComplete)"),
        )
        self.assertIn("AHCI_CONTROLLER_INTERRUPT_MASK);", absent)
        self.assertNotIn("AHCI_SATA_INTERRUPT_MASK);", absent)
        ready = function_body(notify, "else")
        self.assertIn("AHCI_SATA_INTERRUPT_MASK);", ready)
        self.assertNotIn("ahci_schedule_media_discovery", ready)
        self.assertLess(
            notify.index("AHCI_SATA_INTERRUPT_MASK);"),
            notify.index("AHCI_USB_INTERRUPT_MASK);"),
        )

    def test_insertion_reuses_epoch_and_callback_isolation_without_waiting(self):
        """The notifier only schedules; normal foreground admission owns ATA work."""
        self.assertIn("void ahci_media_inserted(UINT32_T port_num)", self.ahci)
        notify = function_body(self.ahci, "void ahci_media_inserted(UINT32_T port_num)")
        self.assertIn("ahci_schedule_media_discovery(port_num, TRUE);", notify)
        for blocking in ("ahci_init_port(", "ahci_stop(", "ahci_port_reset(", "msleep("):
            with self.subTest(blocking=blocking):
                self.assertNotIn(blocking, notify)
        schedule = function_body(self.ahci, "static void ahci_schedule_media_discovery(")
        self.assertIn("sata_media_reset(port_num);", schedule)
        self.assertIn("ahci_reinit_wait_for_callbacks[port_num] = TRUE;", schedule)
        self.assertIn("ahci_hotplug_pending[port_num] = TRUE;", schedule)

    def test_physical_removal_clears_failed_access_even_with_the_phy_ready(self):
        """A later cartridge does not inherit an earlier access-attempt throttle."""
        removal = function_body(self.poll, "else\n        {")
        self.assertIn("sata_media_link_disconnected(0U);", removal)
        self.assertNotIn("PxSSTS", removal)
        self.assertNotIn("eject_state", removal)

    def test_late_insertion_debounce_preserves_the_ready_slider_sample(self):
        """AHCI can publish readiness before GPIO5 reaches its 100-ms commit."""
        guard = (
            "if (!raw_present || !ata_dev[0].bDeviceInitComplete)"
        )
        self.assertIn(guard, self.poll)
        unavailable = function_body(self.poll, guard)
        self.assertIn("rdx_hardware.write_protected = TRUE;", unavailable)
        self.assertEqual(1, self.poll.count("rdx_hardware.write_protected = TRUE;"))


if __name__ == "__main__":
    unittest.main()
