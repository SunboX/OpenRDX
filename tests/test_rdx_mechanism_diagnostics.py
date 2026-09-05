# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Contracts for a bounded, read-only mechanism snapshot over vendor BOT."""

from pathlib import Path
import unittest

from test_rdx_hardware_runtime import function_body


ROOT = Path(__file__).resolve().parents[1]
SCSI = (ROOT / "src/rdx_mount/scsi.c").read_text(encoding="utf-8")
HEADER = (ROOT / "include/rdx_mount/rdx_mechanism.h").read_text(encoding="utf-8")
MECHANISM = (ROOT / "src/rdx_mount/rdx_mechanism.c").read_text(encoding="utf-8")


class RdxMechanismDiagnosticTests(unittest.TestCase):
    """Keep diagnostic reads separate from flash, storage, and motion commands."""

    def test_snapshot_requires_exact_size_and_zero_reserved_bytes(self):
        """Malformed read commands cannot select a different region or operation."""
        vendor = function_body(SCSI, "inline STATUS_T scsi_handle_ti_defined_cmd(void)")
        snapshot = vendor[
            vendor.index("case SCSI_OPENRDX_MECHANISM_STATUS:"):
            vendor.index("case SCSI_TI_READ_FLASH:")
        ]
        for index in range(1, 5):
            self.assertIn("pCommandBlock[{}] != 0U".format(index), snapshot)
        self.assertIn("pCommandBlock[5] !=", snapshot)
        self.assertIn("RDX_MECHANISM_DIAGNOSTIC_LENGTH", snapshot)
        self.assertIn("STATUS_SCSI_INVALID_CMD", snapshot)
        self.assertIn("rdx_mechanism_read_diagnostics(", snapshot)
        self.assertIn("STATUS_SCSI_RESPONSE_READY", snapshot)
        for forbidden in ("SpiOps(", "ahci_", "system_reset", "request_eject"):
            self.assertNotIn(forbidden, snapshot)

    def test_snapshot_uses_data_in_without_media_readiness(self):
        """An empty or withdrawing cartridge must not block the diagnostic path."""
        directions = function_body(SCSI, "STATUS_T scsi_get_data_direction(")
        start = directions.index("case SCSI_OPENRDX_MECHANISM_STATUS:")
        end = directions.index("break;", start)
        self.assertIn("*direction = ENDPT_DIRECTION_IN;", directions[start:end])
        vendor = function_body(SCSI, "inline STATUS_T scsi_handle_ti_defined_cmd(void)")
        snapshot = vendor[
            vendor.index("case SCSI_OPENRDX_MECHANISM_STATUS:"):
            vendor.index("case SCSI_TI_READ_FLASH:")
        ]
        self.assertNotIn("bDeviceInitComplete", snapshot)
        self.assertNotIn("sata_media", snapshot)

    def test_snapshot_is_bounded_and_does_not_advance_the_mechanism(self):
        """Reading status cannot wait on SPI, change outputs, or finish a phase."""
        body = function_body(MECHANISM, "BOOLEAN_T rdx_mechanism_read_diagnostics(")
        self.assertIn("RDX_MECHANISM_DIAGNOSTIC_LENGTH", HEADER)
        self.assertIn("RDX_MECHANISM_DIAGNOSTIC_LENGTH", body)
        for forbidden in (
            "pwm_run(", "pwm_disable(", "gio_high(", "gio_low(",
            "rdx_mechanism_enter_state(", "rdx_mechanism_service(",
            "rdx_mechanism_start(", "rdx_mechanism_cancel(",
            "rdx_mcp3008_read_channel(", "SpiOps(", "msleep(", "usleep(",
        ):
            self.assertNotIn(forbidden, body)
        for register in (
            "RTIFRC0_REG_OFF", "RTIFRC1_REG_OFF", "PWM_CFG_REG_OFF",
            "PWM_PER_REG_OFF", "PWM_PH1D_REG_OFF",
        ):
            self.assertIn(register, body)


if __name__ == "__main__":
    unittest.main()
