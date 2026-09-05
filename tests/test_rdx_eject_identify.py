# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for cached MODE SENSE replies during mechanical eject."""

from pathlib import Path
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"


def function_body(source, signature):
    """Return one C function or conditional block using balanced braces."""
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


class RdxEjectIdentifyTests(unittest.TestCase):
    """Keep a seven-second ATA refresh out of unavailable-media MODE SENSE."""

    @classmethod
    def setUpClass(cls):
        """Read production handlers and the hardware ownership getter."""
        cls.scsi = (SOURCE_DIR / "scsi.c").read_text(encoding="utf-8")
        cls.hardware = (SOURCE_DIR / "rdx_hardware.c").read_text(encoding="utf-8")
        cls.mode_sense = function_body(
            cls.scsi, "inline STATUS_T scsi_handle_mode_sense_cmd(void)"
        )
        cls.cache_pages = function_body(
            cls.mode_sense,
            "else if ((page_code == PAGE_CODE_ALL_PAGES) || "
            "(page_code == PAGE_CODE_CACHING_MODE))",
        )
        cls.refresh_gate = (
            "if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete &&\n"
            "            !rdx_hardware_is_logically_unloaded() &&\n"
            "            !rdx_hardware_eject_in_progress())"
        )

    def test_identify_refresh_requires_ready_loaded_idle_media(self):
        """Empty, logically unloaded, or eject-owned media uses cached fields."""
        self.assertIn(self.refresh_gate, self.cache_pages)
        refresh = function_body(self.cache_pages, self.refresh_gate)
        self.assertIn("ahci_identify_device(scsi_cmd.pCmdInput->bLUN);", refresh)
        self.assertEqual(1, self.mode_sense.count("ahci_identify_device("))
        self.assertEqual(1, refresh.count("ahci_identify_device("))
        self.assertNotIn("return", refresh)

    def test_pending_eject_blocks_refresh_before_the_motor_starts(self):
        """An accepted request owns the gate before WAIT_FOR_IO is published."""
        busy = function_body(
            self.hardware, "BOOLEAN_T rdx_hardware_eject_in_progress(void)"
        )
        self.assertIn("rdx_hardware.eject_requested ||", busy)
        self.assertIn("rdx_hardware.eject_state != RDX_EJECT_IDLE", busy)
        self.assertIn("!rdx_hardware_eject_in_progress()", self.cache_pages)

    def test_skipping_refresh_still_builds_both_caching_headers(self):
        """MODE SENSE6/10 retain their cache response, FUA, and write protection."""
        self.assertIn(self.refresh_gate, self.cache_pages)
        refresh = function_body(self.cache_pages, self.refresh_gate)
        after_refresh = self.cache_pages[self.cache_pages.index(refresh) + len(refresh):]
        self.assertIn("== SCSI_MODE_SENSE6", after_refresh)
        self.assertEqual(2, after_refresh.count(".bFUA ? DPOFUA_BIT : 0U"))
        self.assertEqual(
            2, after_refresh.count("rdx_hardware_is_write_protected() ? WP_BIT : 0U")
        )
        self.assertIn("CACHING_MODE_PAGE_LENGTH + 3", after_refresh)
        self.assertIn("CACHING_MODE_PAGE_LENGTH + 6", after_refresh)
        self.assertIn("data_length = index + CACHING_MODE_PAGE_LENGTH;", after_refresh)
        self.assertIn("scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;", self.mode_sense)
        self.assertIn("status = STATUS_SCSI_RESPONSE_READY;", self.mode_sense)

    def test_cache_capability_and_current_words_remain_available(self):
        """Changeable/current pages retain cached IDENTIFY words82/85."""
        self.assertIn(self.refresh_gate, self.cache_pages)
        refresh = function_body(self.cache_pages, self.refresh_gate)
        after_refresh = self.cache_pages[self.cache_pages.index(refresh) + len(refresh):]
        for word in (82, 85):
            for bit in ("0x20", "0x40"):
                with self.subTest(word=word, bit=bit):
                    self.assertIn("wIdentifyDeviceInfo[{}] & {}".format(word, bit), after_refresh)
        self.assertNotIn("ti_memset", refresh)
        self.assertNotIn("ahci_save_device_info", refresh)

    def test_unavailable_media_can_still_query_mode_pages(self):
        """Keep the command-level exception needed for empty-bay mode replies."""
        handler = function_body(self.scsi, "inline STATUS_T scsi_ata_cmd_handler(void)")
        readiness_gate = handler[:handler.index("// Call appropriate command handler.")]
        self.assertIn("!= SCSI_MODE_SENSE6", readiness_gate)
        self.assertIn("!= SCSI_MODE_SENSE10", readiness_gate)
        self.assertIn("status = scsi_handle_mode_sense_cmd();", handler)


if __name__ == "__main__":
    unittest.main()
