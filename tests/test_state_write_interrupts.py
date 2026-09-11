# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Exercise persistent state writes against register and SPI transaction models."""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from test_flash_preservation import production_function


ROOT = Path(__file__).resolve().parents[1]
CC = shutil.which("cc")


@unittest.skipUnless(CC, "Host C logic probe requires cc; firmware still uses TI CGT")
class StateWriteInterruptTests(unittest.TestCase):
    """State writes must exclude USB SPI access without changing other IRQs."""

    @classmethod
    def setUpClass(cls):
        """Compile actual control functions with observable SPI/register boundaries."""
        cls.temporary = tempfile.TemporaryDirectory(prefix="openrdx-state-irq-")
        cls.addClassCleanup(cls.temporary.cleanup)
        directory = Path(cls.temporary.name)
        control = (ROOT / "src/rdx_mount/rdx_manager_control.c").read_text()
        vim_header = (ROOT / "include/rdx_mount/vim_nvic.h").read_text()
        spi_header = (ROOT / "include/rdx_mount/spi.h").read_text()
        defines = "\n".join(
            line for source in (control, vim_header, spi_header)
            for line in source.splitlines()
            if re.match(r"#define (?:RDX_STATE_|RDX_CONTROL_USB_|VIM_REQMASK|Opcode)", line)
        )
        functions = "\n".join(production_function(control, name) for name in (
            "rdx_control_store_le32", "rdx_control_calculate_checksum",
            "rdx_control_record_is_valid", "rdx_control_save_operation_mode",
            "rdx_control_save_drive_load_count",
        ))
        source = (ROOT / "tests/fixtures/state_write_interrupt_probe.c").read_text()
        source = source.replace("/* PRODUCTION_DEFINES */", defines)
        source = source.replace("/* PRODUCTION_FUNCTIONS */", functions)
        path = directory / "probe.c"
        path.write_text(source)
        cls.probe = directory / "probe"
        result = subprocess.run(
            [CC, "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
             str(path), "-o", str(cls.probe)], capture_output=True, text=True,
        )
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def probe_transaction(self, operation, failure_at):
        """Check enabled/disabled callers and an unrelated IRQ change mid-transaction."""
        for enabled in (0, 1):
            with self.subTest(operation=operation, enabled=enabled, failure_at=failure_at):
                result = subprocess.run(
                    [str(self.probe), operation, str(enabled), str(failure_at)],
                    capture_output=True, text=True, timeout=10,
                )
                self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_mode_save_masks_usb_through_complete_transaction(self):
        """Mode persistence must exclude USB reads and restore only its saved bit."""
        self.probe_transaction("mode", 0)

    def test_count_save_masks_usb_through_complete_transaction(self):
        """Count persistence must keep its five SPI operations indivisible to USB."""
        self.probe_transaction("count", 0)

    def test_mode_save_restores_prior_mask_on_each_spi_failure(self):
        """Read, both write-enables, erase, and program errors must all restore IRQ state."""
        for failure_at in range(1, 6):
            self.probe_transaction("mode", failure_at)

    def test_count_save_restores_prior_mask_on_each_spi_failure(self):
        """A failed count write must never leave USB disabled or enable it spuriously."""
        for failure_at in range(1, 6):
            self.probe_transaction("count", failure_at)


if __name__ == "__main__":
    unittest.main()
