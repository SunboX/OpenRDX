# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Exercise the production cache refresh without replacing admitted geometry."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from test_flash_preservation import production_function
from test_rdx_eject_identify import function_body


ROOT = Path(__file__).resolve().parents[1]
RUNTIME = (ROOT / "src/rdx_mount/rdx_runtime_ata.c").read_text(encoding="utf-8")
CC = shutil.which("cc") or shutil.which("cl")


class CacheRefreshContractTests(unittest.TestCase):
    """Keep the initialization parser away from runtime MODE SENSE."""

    def test_refresh_changes_only_current_cache_bits(self):
        """The production helper cannot overwrite capacity, mapping, or identity."""
        body = function_body(RUNTIME, "BOOLEAN_T rdx_refresh_cache_info(")
        self.assertNotIn("ahci_identify_device", body)
        self.assertNotIn("ahci_save_device_info", body)
        self.assertNotIn("ddMaxLBA", body)
        self.assertNotIn("ddTrueMaxLBA", body)
        self.assertEqual(2, body.count("wIdentifyDeviceInfo[85]"))
        self.assertIn("current & 0x0060U", body)
        self.assertLess(body.index("if (successful)"), body.index("identify[170U]"))
        self.assertLess(body.index("rdx_runtime_lock_current_session"), body.index("identify[170U]"))
        self.assertEqual(2, body.count("WRITE32(PxIS(port_num), RDX_PIO_COMPLETION_STATUS)"))


@unittest.skipUnless(CC, "Host C logic probe requires cc or an initialized MSVC shell; firmware uses TI CGT")
class CacheRefreshRuntimeTests(unittest.TestCase):
    """Execute unchanged production C with command/epoch boundary shims."""

    @classmethod
    def setUpClass(cls):
        """Build a host-only logic fixture using production function text."""
        cls.temporary = tempfile.TemporaryDirectory(prefix="openrdx-cache-")
        cls.addClassCleanup(cls.temporary.cleanup)
        directory = Path(cls.temporary.name)
        fixture = (ROOT / "tests/fixtures/cache_refresh_probe.c").read_text(encoding="utf-8")
        source = fixture.replace("/* PRODUCTION_FUNCTION */", production_function(RUNTIME, "rdx_refresh_cache_info"))
        source_path = directory / "probe.c"
        source_path.write_text(source, encoding="utf-8")
        cls.probe = directory / ("probe.exe" if Path(CC).name.lower() == "cl.exe" else "probe")
        if Path(CC).name.lower() in ("cl", "cl.exe"):
            command = [CC, "/nologo", "/utf-8", "/W4", "/WX", str(source_path),
                       "/Fe:" + str(cls.probe), "/Fo:" + str(directory / "probe.obj")]
        else:
            command = [CC, "-std=c99", "-Wall", "-Wextra", "-Werror", str(source_path), "-o", str(cls.probe)]
        completed = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=60)
        if completed.returncode:
            raise AssertionError(completed.stdout + completed.stderr)

    def run_probe(self, scenario):
        """Run one isolated in-memory scenario, without any device access."""
        completed = subprocess.run([str(self.probe), scenario], capture_output=True, text=True, timeout=10)
        self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)

    def test_cache_changes_preserve_reduced_three_tb_geometry(self):
        """All four cache-bit combinations preserve the entire remaining device."""
        self.run_probe("success")

    def test_busy_port_keeps_cached_state_without_dispatch(self):
        """An occupied command slot must not read or update device state."""
        self.run_probe("busy")

    def test_failed_identify_cannot_publish_stale_buffer_bits(self):
        """Rejected IDENTIFY preserves all previously admitted state."""
        self.run_probe("failed")

    def test_media_change_prevents_cache_commit_and_interrupt_restore(self):
        """A new media epoch cannot receive the old command's cache flags."""
        self.run_probe("replaced")


if __name__ == "__main__":
    unittest.main()
