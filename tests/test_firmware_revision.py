# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Check the actual revision initializers with the supported TI compiler."""

import configparser
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

from scripts.ti_cgt_tools import resolve_ti_cgt_tools


ROOT = Path(__file__).resolve().parents[1]
CONFIG = configparser.ConfigParser()
CONFIG.read(ROOT / "platformio.ini")
OPTIONS = CONFIG["env:tusb9261_ti_cgt"]
TOOLS = resolve_ti_cgt_tools(
    OPTIONS["custom_ti_cgt_root"], OPTIONS["custom_ti_cgt_root_macos"]
)


@unittest.skipUnless(TOOLS.compiler.is_file(), f"TI compiler missing: {TOOLS.compiler}")
class FirmwareRevisionTests(unittest.TestCase):
    """Compile production initializers without running or flashing firmware."""

    def compile_revisions(self, version=None):
        """Return TI-emitted SCSI ASCII and USB descriptor revision bytes."""
        scsi = (ROOT / "src/rdx_mount/scsi.c").read_text(encoding="utf-8")
        revision = re.search(
            r'ti_memcpy\(\(void\*\)&scsi_resp_buff\[32\],\s*("[^"]*"|\w+),',
            scsi.split("inline void scsi_build_std_inquiry_data(", 1)[1],
        ).group(1)
        if not revision.startswith('"'):
            revision = re.search(
                rf"static const UINT8_T {revision}\[[^]]*\]\s*=\s*(\{{.*?\}});",
                scsi, re.S,
            ).group(1)
        usb = (ROOT / "src/rdx_mount/usb_chap9.c").read_text(encoding="utf-8")
        descriptor = usb.split("/* Device Descriptor */", 1)[1].split(
            "/* Device Qualifier", 1
        )[0]
        overrides = ""
        if version is not None:
            for component, value in zip(("MAJOR", "MINOR"), version):
                overrides += (f"#undef FIRMWARE_{component}_VERSION\n"
                              f"#define FIRMWARE_{component}_VERSION {value}\n")
        with tempfile.TemporaryDirectory(prefix="openrdx-version-") as directory:
            temporary = Path(directory)
            probe = temporary / "probe.c"
            probe.write_text(
                '#include "tusb9260.h"\n#define USB_DT_DEVICE 1\n'
                + overrides +
                f"const UINT8_T inquiry_probe[4] = {revision};\n"
                f"const UINT8_T usb_probe[] = {{{descriptor}}};\n",
                encoding="utf-8",
            )
            completed = subprocess.run(
                [str(TOOLS.compiler), "-c", "-mv7m3", "--abi=ti_arm9_abi", "-me",
                 "--gcc", "--emit_warnings_as_errors", "--keep_asm",
                 f"--include_path={ROOT / 'include/rdx_mount'}",
                 f"--include_path={TOOLS.include_dir}",
                 f"--obj_directory={temporary}", f"--asm_directory={temporary}",
                 str(probe)],
                capture_output=True, text=True, timeout=60,
            )
            self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)
            # TI's Windows tools emit code-page comments even for UTF-8 input;
            # the numeric data directives below are always ASCII.
            assembly = probe.with_suffix(".asm").read_text(encoding="latin-1")
            values = {}
            for symbol in ("inquiry_probe", "usb_probe"):
                values[symbol] = bytes(int(value) for value in re.findall(
                    rf"\.bits\s+(\d+),8\s*; _{symbol}\[\d+\]", assembly
                ))
            self.assertEqual(18, len(values["usb_probe"]))
            return values["inquiry_probe"], values["usb_probe"][12:14]

    def test_reported_revision_matches_release(self):
        """The released major/minor must appear in both host-facing identities."""
        version = (ROOT / "VERSION").read_text(encoding="ascii").strip()
        major, minor = map(int, version.split("."))
        inquiry, usb = self.compile_revisions()
        self.assertEqual(f"{major:02d}{minor:02d}".encode("ascii"), inquiry)
        self.assertEqual(bytes.fromhex(f"{minor:02d} {major:02d}"), usb)

    def test_decimal_digits_and_bcd_follow_future_versions(self):
        """Prevent literal revisions, octal minors, or binary USB version bytes."""
        for version, expected_ascii, expected_usb in (
            ((1, 10), b"0110", b"\x10\x01"),
            ((10, 9), b"1009", b"\x09\x10"),
        ):
            with self.subTest(version=version):
                self.assertEqual((expected_ascii, expected_usb),
                                 self.compile_revisions(version))


if __name__ == "__main__":
    unittest.main()
