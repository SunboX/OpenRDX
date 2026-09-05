# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for SATA receiver errors during powered eject travel."""

from pathlib import Path
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
AHCI = (PROJECT_DIR / "src" / "rdx_mount" / "ahci.c").read_text(encoding="utf-8")


def function_body(source, signature):
    """Return a function or conditional block using balanced C braces."""
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


class RdxEjectRxErrorTests(unittest.TestCase):
    """Keep noisy removal from running blocking recovery ahead of motor service."""

    def test_active_motion_defers_before_any_blocking_receiver_recovery(self):
        """The first RX error during motion bypasses multi-second stop/reset."""
        receiver = function_body(AHCI, "void ahci_rx_error_isr(void)")
        self.assertIn("if (rdx_mechanism_is_active())", receiver)
        active = function_body(receiver, "if (rdx_mechanism_is_active())")
        self.assertIn("return;", active)
        for blocking in ("ahci_stop(", "ahci_port_reset(", "msleep("):
            self.assertNotIn(blocking, active)
            self.assertLess(receiver.index(active), receiver.index(blocking))
        self.assertLess(receiver.index(active), receiver.index("counter++"))
        # WAIT_FOR_IO still needs normal recovery to complete outstanding I/O.
        self.assertNotIn("rdx_hardware_eject_in_progress()", receiver)

    def test_deferred_error_invalidates_media_and_preserves_terminal_callback(self):
        """Normal eject stays unready; boot homing preserves its transport owner."""
        receiver = function_body(AHCI, "void ahci_rx_error_isr(void)")
        self.assertIn("if (rdx_mechanism_is_active())", receiver)
        active = function_body(receiver, "if (rdx_mechanism_is_active())")
        capture = active.index("media_was_ready = ata_dev[0].bDeviceInitComplete;")
        schedule = active.index("ahci_schedule_media_discovery(0U, TRUE);")
        clear = active.index("ahci_clear_rx_error_count();")
        callback = active.index("ahci_ata_cbk_queue_add(")
        self.assertLess(capture, schedule)
        self.assertLess(schedule, clear)
        self.assertLess(clear, callback)
        self.assertIn("media_was_ready &&", active)
        self.assertIn("!ahci_callbacks_are_pending(0U)", active)
        self.assertEqual(1, active.count("ahci_ata_cbk_queue_add("))
        self.assertIn("ata_dev[0].pAtaErrorCallback", active)

    def test_clearing_receiver_errors_is_bounded_and_shared(self):
        """A handled error must not remain pending when discovery reenables IRQs."""
        self.assertIn("static void ahci_clear_rx_error_count(void)", AHCI)
        clear = function_body(AHCI, "static void ahci_clear_rx_error_count(void)")
        self.assertEqual(2, clear.count("MODIFY32(PxPHYCTRL(0)"))
        self.assertIn("PPHY_CTRL_CLR_RX_8B10B_ERR_CNT);", clear)
        self.assertIn("PPHY_CTRL_CLR_RX_8B10B_ERR_CNT, 0);", clear)
        for blocking in ("while", "msleep(", "usleep(", "ahci_stop("):
            self.assertNotIn(blocking, clear)
        receiver = function_body(AHCI, "void ahci_rx_error_isr(void)")
        self.assertEqual(2, receiver.count("ahci_clear_rx_error_count();"))

    def test_deferred_receiver_observes_link_loss_before_masking_events(self):
        """Removal already visible at the RX interrupt clears old access throttling."""
        receiver = function_body(AHCI, "void ahci_rx_error_isr(void)")
        active = function_body(receiver, "if (rdx_mechanism_is_active())")
        self.assertTrue("sata_media_link_disconnected(0U);" in active)
        absent = function_body(
            active,
            "if ((READ32(PxSSTS(0)) & PSSTS_DET_MASK) !=",
        )
        self.assertIn("sata_media_link_disconnected(0U);", absent)
        self.assertIn("PSSTS_DET_PHY_READY", active[:active.index(absent)])
        self.assertLess(
            active.index("sata_media_link_disconnected(0U);"),
            active.index("ahci_schedule_media_discovery(0U, TRUE);"),
        )

    def test_foreground_observes_removal_after_receiver_events_were_masked(self):
        """A later link loss must clear the previous cartridge's failed-access limit."""
        service = function_body(AHCI, "void ahci_service(void)")
        absent = function_body(
            service,
            "if ((READ32(PxSSTS(port_num)) & PSSTS_DET_MASK) !=",
        )
        self.assertTrue("sata_media_link_disconnected(port_num);" in absent)
        self.assertLess(
            absent.index("sata_media_link_disconnected(port_num);"),
            absent.index("continue;"),
        )
        self.assertNotIn("ahci_hotplug_pending[port_num] = FALSE;", absent)
        self.assertNotIn("sata_media_reset(", absent)
        self.assertLess(service.index(absent), service.index("ahci_init_port(port_num);"))

    def test_pending_recovery_keeps_epoch_and_callback_barriers(self):
        """Deferred RX recovery uses the existing durable hotplug drain path."""
        schedule = function_body(AHCI, "static void ahci_schedule_media_discovery(")
        self.assertIn("ata_dev[port_num].bDeviceInitComplete = FALSE;", schedule)
        self.assertIn("sata_media_reset(port_num);", schedule)
        self.assertIn("WRITE32(PxIE(port_num), 0U);", schedule)
        self.assertIn("WRITE_REG32(VIM_REQMASKCLR0, 0x00000020);", schedule)
        self.assertIn("ahci_reinit_wait_for_callbacks[port_num] = TRUE;", schedule)
        self.assertIn("ahci_hotplug_pending[port_num] = TRUE;", schedule)
        service = function_body(AHCI, "void ahci_service(void)")
        guard = service.index("rdx_hardware_eject_in_progress()")
        callbacks = service.index("ahci_callbacks_are_pending(port_num)")
        stop = service.index("ahci_stop(port_num);")
        phy_ready = service.index("PSSTS_DET_PHY_READY")
        discovery = service.index("ahci_init_port(port_num);")
        self.assertLess(guard, callbacks)
        self.assertLess(callbacks, stop)
        self.assertLess(stop, phy_ready)
        self.assertLess(phy_ready, discovery)


if __name__ == "__main__":
    unittest.main()
