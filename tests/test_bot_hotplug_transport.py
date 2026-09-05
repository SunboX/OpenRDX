# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Keep removable-media discovery from restarting an active USB command."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
BOT = (ROOT / "src/rdx_mount/ums_bot.c").read_text(encoding="utf-8")


def reset_for_media_type(removable):
    """Select the simple removable/fixed branches of the BOT reset function."""
    body = BOT.split("void ums_bot_reset(void)", 1)[1].split(
        "inline void ums_bot_process_CBW(void)", 1
    )[0]
    return re.sub(
        r"#if REMOVABLE_MEDIA_DEVICE\s*\n(.*?)#else\s*\n(.*?)#endif",
        lambda match: match.group(1 if removable else 2),
        body,
        flags=re.S,
    )


class BotHotplugTransportTests(unittest.TestCase):
    """Check USB command ownership across empty-bay and insertion discovery."""

    def test_removable_discovery_cannot_rearm_cbw_during_data_or_status(self):
        """A port completion must preserve any pending data/CSW and its flags."""
        reset = reset_for_media_type(True)
        self.assertIn("ahci_register_port_init_complete_callback(0, NULL);", reset)
        self.assertNotIn("ahci_register_port_init_complete_callback(0, ums_bot_idle);", reset)

    def test_empty_lun_accepts_commands_even_while_discovery_is_pending(self):
        """A BOT reset rearms once without waiting for SATA ready or failure."""
        reset = reset_for_media_type(True)
        self.assertNotIn("bDeviceInitComplete", reset)
        self.assertNotIn("bDeviceInitTimedOut", reset)
        self.assertEqual(1, reset.count("ums_bot_idle(0);"))
        self.assertLess(reset.index("ums_bot_xfer_cleanup(FALSE);"),
                        reset.index("ums_bot_idle(0);"))

    def test_fixed_disk_keeps_its_initial_discovery_gate(self):
        """The fixed-device configuration retains its existing callback route."""
        reset = reset_for_media_type(False)
        self.assertIn("ahci_register_port_init_complete_callback(0, ums_bot_idle);", reset)
        self.assertIn("ata_dev[0].bDeviceInitComplete || ata_dev[0].bDeviceInitTimedOut", reset)


if __name__ == "__main__":
    unittest.main()
