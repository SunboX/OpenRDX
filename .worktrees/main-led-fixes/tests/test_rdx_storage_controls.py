# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts for RDX ADC, write-protect, and eject integration."""

import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SPI_HEADER = (ROOT / "include" / "rdx_mount" / "spi.h").read_text(
    encoding="utf-8"
)
SPI_SOURCE = (ROOT / "src" / "rdx_mount" / "spi.c").read_text(
    encoding="utf-8"
)
SCSI_HEADER = (ROOT / "include" / "rdx_mount" / "scsi.h").read_text(
    encoding="utf-8"
)
SCSI_SOURCE = (ROOT / "src" / "rdx_mount" / "scsi.c").read_text(
    encoding="utf-8"
)
BOT_SOURCE = (ROOT / "src" / "rdx_mount" / "ums_bot.c").read_text(
    encoding="utf-8"
)
TUSB_HEADER = (ROOT / "include" / "rdx_mount" / "tusb9260.h").read_text(
    encoding="utf-8"
)


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


class RdxStorageControlTests(unittest.TestCase):
    """Preserve the bounded RDX hardware and SCSI contracts."""

    def test_mcp3008_uses_required_bus_and_frame(self):
        """Keep CS1, data-format one, and all three required DAT1 words."""
        for token in (
            "SPI_FUNCTION_MASK         0x00000E03UL",
            "SPI_DIRECTION_MASK        0x00000601UL",
            "SPI_FORMAT1_MCP3008       0x01002408UL",
            "MCP3008_START_WORD     0x11050001UL",
            "MCP3008_CHANNEL_WORD   0x11050080UL",
            "MCP3008_FINISH_WORD    0x01050000UL",
            "SPI_WAIT_LIMIT         1000000UL",
        ):
            self.assertIn(token, SPI_HEADER + SPI_SOURCE)
        self.assertIn(
            "STATUS_T rdx_mcp3008_read_channel(UINT8_T channel, UINT16_T *sample)",
            SPI_HEADER,
        )
        self.assertRegex(SPI_SOURCE, r"channel > 7U")
        self.assertIn("return STATUS_TIMEOUT;", SPI_SOURCE)

    def test_mode_sense_and_write_dispatch_honor_physical_lock(self):
        """Expose WP and reject only the validated write-opcode class."""
        self.assertGreaterEqual(
            SCSI_SOURCE.count("rdx_hardware_is_write_protected() ? WP_BIT : 0U"),
            4,
        )
        classifier = function_body(
            SCSI_SOURCE,
            "static BOOLEAN_T scsi_is_media_write_class_cmd(",
        )
        # The physical lock covers block writes and the destructive UNMAP
        # operation, without broadening into unrelated command policies.
        for opcode in (
            "SCSI_WRITE6",
            "SCSI_WRITE10",
            "SCSI_WRITE12",
            "SCSI_WRITE16",
            "RDX_SCSI_WRITE_AND_VERIFY10",
            "RDX_SCSI_WRITE_AND_VERIFY12",
            "RDX_SCSI_WRITE_AND_VERIFY16",
            "SCSI_UNMAP",
        ):
            self.assertIn(opcode, classifier)
        for invented_extension in (
            "SCSI_FORMAT_UNIT",
            "ATA_PASS_THROUGH12",
            "ATA_PASS_THROUGH16",
        ):
            self.assertNotIn(invented_extension, classifier)
        write_gate = re.search(
            r"if \(scsi_is_media_write_class_cmd.*?\n\s*\}",
            SCSI_SOURCE,
            re.DOTALL,
        )
        self.assertIsNotNone(write_gate)
        self.assertIn("rdx_hardware_is_write_protected()", write_gate.group(0))
        self.assertIn("return STATUS_SCSI_WRITE_PROTECTED;", write_gate.group(0))
        self.assertNotIn("STATUS_SCSI_ATA_ERROR", write_gate.group(0))
        self.assertRegex(
            SCSI_SOURCE,
            re.compile(
                r"case STATUS_SCSI_WRITE_PROTECTED:.*?"
                r"scsi_set_sense_data\(DATA_PROTECT, WRITE_PROTECTED, NO_ASCQ\)",
                re.DOTALL,
            ),
        )

    def test_prevent_state_and_scsi_eject_share_runtime_contract(self):
        """Keep distinct STOP and LOEJ paths with one coordinator."""
        self.assertIn("scsi_medium_removal_is_prevented(void);", SCSI_HEADER)
        self.assertIn("static BOOLEAN_T scsi_medium_removal_prevented", SCSI_SOURCE)
        start_stop = SCSI_SOURCE[
            SCSI_SOURCE.index("inline STATUS_T scsi_handle_start_stop_unit_cmd(void)") :
            SCSI_SOURCE.index("inline STATUS_T scsi_handle_start_stop_unit_cmd(void)")
            + 10000
        ]
        self.assertEqual(1, start_stop.count("rdx_hardware_request_eject();"))
        removable_loeject = start_stop.index("#if REMOVABLE_MEDIA_DEVICE")
        policy = start_stop.index(
            "if (!rdx_manager_apply_host_eject_policy())",
            removable_loeject,
        )
        queued = start_stop.index("rdx_hardware_request_eject();", policy)
        accepted = start_stop.index("return STATUS_OK;", queued)
        generic_flush = start_stop.index(
            "// Send ATA FLUSH CACHE and then send ATA STANDBY IMMEDIATE.",
            accepted,
        )
        self.assertLess(removable_loeject, policy)
        self.assertLess(policy, queued)
        self.assertLess(queued, accepted)
        self.assertLess(accepted, generic_flush)
        self.assertIn("MEDIUM_REMOVAL_PREVENTED", SCSI_SOURCE)
        self.assertIn(
            "return STATUS_SCSI_MEDIUM_REMOVAL_PREVENTED;",
            start_stop,
        )

    def test_empty_bay_prevent_and_start_stop_are_asymmetric(self):
        """Allow release/no-op eject while rejecting media-dependent forms."""
        prevent = function_body(
            SCSI_SOURCE,
            "inline STATUS_T scsi_handle_prevent_allow_medium_removal_cmd(void)",
        )
        start_stop = function_body(
            SCSI_SOURCE, "inline STATUS_T scsi_handle_start_stop_unit_cmd(void)"
        )
        ata_dispatch = function_body(
            SCSI_SOURCE, "inline STATUS_T scsi_ata_cmd_handler(void)"
        )

        self.assertIn("prevent & MEDIUM_REMOVAL_PROHIBITED", prevent)
        self.assertIn("BOOLEAN_T removal_prohibited;", prevent)
        self.assertIn("!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete", prevent)
        self.assertIn("rdx_hardware_is_logically_unloaded()", prevent)
        self.assertIn("return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;", prevent)
        self.assertIn("scsi_medium_removal_prevented = removal_prohibited;", prevent)
        self.assertIn("return STATUS_OK;", prevent)
        self.assertNotIn("return STATUS_ERROR;", prevent)
        loeject = start_stop.index(
            "((scsi_cmd.pCmdInput->pCommandBlock[4] & LOEJ_BIT) != 0U)"
        )
        removable_dispatch = start_stop.index(
            "pwr_condition = PWR_CONDITION_START_VALID;"
        )
        readiness = start_stop.index(
            "if (!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete ||"
        )
        self.assertLess(removable_dispatch, loeject)
        self.assertLess(loeject, readiness)
        self.assertIn("rdx_hardware_is_logically_unloaded()", start_stop)
        self.assertIn("return STATUS_OK;", start_stop)
        self.assertIn("return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;", start_stop)
        self.assertIn("SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL", ata_dispatch)
        self.assertIn("SCSI_START_STOP_UNIT", ata_dispatch)
        self.assertIn("rdx_hardware_is_logically_unloaded()", ata_dispatch)
        self.assertNotIn("return STATUS_SCSI_ATA_ERROR;", start_stop)
        self.assertRegex(
            SCSI_SOURCE,
            re.compile(
                r"case STATUS_SCSI_MEDIUM_REMOVAL_PREVENTED:.*?"
                r"scsi_set_sense_data\(ILLEGAL_REQUEST,.*?"
                r"MEDIUM_REMOVAL_PREVENTED,.*?"
                r"ASCQ_MEDIUM_REMOVAL_PREVENTED\)",
                re.DOTALL,
            ),
        )

    def test_local_rejections_complete_bot_instead_of_waiting_for_ata(self):
        """Send FAILED CSW for local sense; reserve ATA_ERROR for callbacks."""
        for status in (
            "STATUS_SCSI_MEDIUM_REMOVAL_PREVENTED",
            "STATUS_SCSI_WRITE_PROTECTED",
        ):
            self.assertIn(status, TUSB_HEADER)
        self.assertIn(
            "if (status != STATUS_SCSI_ATA_ERROR)",
            BOT_SOURCE,
        )
        self.assertIn(
            "ums_bot_send_CSW((status == STATUS_OK) ? "
            "CSW_CMD_PASSED : CSW_CMD_FAILED);",
            BOT_SOURCE,
        )


if __name__ == "__main__":
    unittest.main()
