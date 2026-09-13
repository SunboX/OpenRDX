# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Exercise real firmware command handlers against a bounded host flash model."""

import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CC = shutil.which("cc")


def production_function(source, name):
    """Copy one complete production function, with host-compatible linkage."""
    match = re.search(r"^(?:(?:static|inline) )*\w+ " + name + r"\(", source, re.M)
    start = source.index("{", match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end].removeprefix("inline ")


@unittest.skipUnless(CC, "Host C logic probe requires cc; firmware still uses TI CGT")
class FlashPreservationTests(unittest.TestCase):
    """Catch legacy full-chip erase and updates extending into persistent records."""

    @classmethod
    def setUpClass(cls):
        """Compile unchanged SCSI/update handlers with SPI and peripheral shims."""
        cls.temporary = tempfile.TemporaryDirectory(prefix="openrdx-flash-")
        cls.addClassCleanup(cls.temporary.cleanup)
        directory = Path(cls.temporary.name)
        protocol = (ROOT / "src/rdx_mount/rdx_manager_protocol.c").read_text()
        scsi = (ROOT / "src/rdx_mount/scsi.c").read_text()
        hardware_header = (ROOT / "include/rdx_mount/tusb9260.h").read_text()
        status = re.search(r"typedef enum\s*\{[^}]+\} STATUS_T;", hardware_header).group()
        defines = "\n".join(
            line for header in ("scsi.h", "spi.h", "tusb9260.h", "rdx_manager_serial.h")
            for line in (ROOT / "include/rdx_mount" / header).read_text().splitlines()
            if re.match(r"#define (?:SCSI_TI_|SCSI_OPENRDX_|Opcode|RDX_SERIAL_|"
                        r"FIRMWARE_(?:MAJOR|MINOR)_VERSION\s+\d)", line)
        )
        helpers = protocol[protocol.index("#define RDX_MEDIA_ID_MAX_LENGTH"):
                           protocol.index("/** Reset the in-RAM state")]
        prelude = (ROOT / "tests/fixtures/flash_preservation_probe.c").read_text()
        source = prelude.replace("/* PRODUCTION_STATUS_AND_DEFINES */", status + "\n" + defines)
        source = source.replace("/* PRODUCTION_COMMAND_HANDLERS */", helpers + "\n" +
                                production_function(scsi, "scsi_handle_ti_defined_cmd") + "\n" +
                                production_function(protocol, "rdx_manager_handle_write_buffer"))
        probe_source = directory / "probe.c"
        probe_source.write_text(source)
        cls.probe = directory / "probe"
        completed = subprocess.run(
            [CC, "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
             str(probe_source), "-o", str(cls.probe)], capture_output=True, text=True,
        )
        if completed.returncode:
            raise AssertionError(completed.stdout + completed.stderr)

    def run_probe(self, scenario, image=None):
        """Run a fresh process so static unlock state cannot leak between cases."""
        arguments = [str(self.probe), scenario]
        if image is not None:
            path = Path(self.temporary.name) / "container.bin"
            path.write_bytes(image)
            arguments.append(str(path))
        completed = subprocess.run(arguments, capture_output=True, text=True, timeout=10)
        self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)

    def test_legacy_erase_rejected_before_unlock(self):
        """E2 must return INVALID CMD without issuing any SPI operation."""
        self.run_probe("erase")

    def test_legacy_erase_rejected_after_unlock(self):
        """E1 must never make E2 erase the receiver's manufacturing/state sectors."""
        self.run_probe("unlock-erase")

    def test_legacy_unlock_is_rejected(self):
        """Do not report a successful unlock for the disabled erase protocol."""
        self.run_probe("unlock")

    def test_failed_flash_read_never_publishes_stale_response(self):
        """SPI contention/fault must fail backup reads instead of replaying old bytes."""
        self.run_probe("read-failure")

    def test_successful_flash_read_returns_exact_bytes(self):
        """Preserve the existing bounded manufacturing-read response contract."""
        self.run_probe("read")

    def test_restore_waits_for_restart_after_update_or_serial_activity(self):
        """Update failure, pending activation, and prior serial writes exclude restore."""
        self.run_probe("restore-interlocks")

    def test_mode04_and_activation_preserve_every_byte_above_boot_image(self):
        """A valid streamed update writes its payload while preserving high flash."""
        image = bytearray((index * 17 + 3) & 255 for index in range(62110))
        image[0x108:0x188] = bytes(128)
        image[0x108:0x110] = b"OPENRDX1"
        image[0x110:0x130] = hashlib.sha256(image[0x18C:0xF29A]).digest()
        self.run_probe("update", image)


if __name__ == "__main__":
    unittest.main()
