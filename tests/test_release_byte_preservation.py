# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Verify Git cannot rewrite checksummed release bytes during staging."""

from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ReleaseBytePreservationTests(unittest.TestCase):
    """Exercise Git's clean filter with Windows line-ending conversion enabled."""

    def test_release_artifacts_keep_exact_bytes_with_autocrlf_enabled(self):
        """Text-shaped release files must retain CRLF and their published hashes."""
        payload = b"first line\r\nsecond line\r\n"
        raw_hash = subprocess.check_output(
            ["git", "hash-object", "--stdin", "--no-filters"],
            input=payload, cwd=ROOT,
        )
        for name in (
            "OpenRDX-v1-06.bin", "OpenRDX-v1-06-FlashBurner.hex",
            "rdx_manager_firmware_update.ps1", "OPENRDX_USB_UPDATE.md",
            "OpenRDX-v1-06.json", "OpenRDX-v1-06.sha256",
        ):
            with self.subTest(artifact=name):
                filtered_hash = subprocess.check_output(
                    ["git", "-c", "core.autocrlf=true", "hash-object",
                     "--stdin", "--path=dist/" + name],
                    input=payload, cwd=ROOT,
                )
                self.assertEqual(raw_hash, filtered_hash)


if __name__ == "__main__":
    unittest.main()
