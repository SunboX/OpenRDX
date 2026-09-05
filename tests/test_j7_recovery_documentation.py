# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Keep the J7 and vendor-firmware recovery guide safe and discoverable."""

from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
GUIDE_PATH = PROJECT_ROOT / "docs" / "development" / "rom-loader-recovery.md"
DOCS_INDEX_PATH = PROJECT_ROOT / "docs" / "README.md"
ROOT_README_PATH = PROJECT_ROOT / "README.md"
PHOTO_PATHS = (
    PROJECT_ROOT / "docs" / "assets" / "rdx-board-j7-location.jpg",
    PROJECT_ROOT / "docs" / "assets" / "rdx-board-j7-close-up.jpg",
)


class J7RecoveryDocumentationTests(unittest.TestCase):
    """Protect the evidence and safety gates needed for board recovery."""

    def test_guide_identifies_the_same_unit_raw_backup(self):
        """Require the exact source path, size, and digest of the raw backup."""

        guide = GUIDE_PATH.read_text(encoding="utf-8")

        self.assertIn("TUSB9261_RDX_*_Project/Firmware/", guide)
        self.assertIn("M25PE20_RDX_*_flash_256KiB.bin", guide)
        self.assertIn("262,144 bytes", guide)
        self.assertIn(
            "058d1f0b3a22f7833106cf9e2a952ced9d8090802649e728ed5b80032c6cf5f9",
            guide,
        )

    def test_vendor_restoration_uses_ti_full_image_with_format_boundaries(self):
        """Keep TI restoration distinct from incompatible update envelopes."""

        guide = GUIDE_PATH.read_text(encoding="utf-8")
        compact_guide = " ".join(guide.split())

        self.assertIn("originating receiver", guide)
        self.assertIn("RDX2E__STD__F-0283.bin", guide)
        self.assertIn("62,110-byte", guide)
        self.assertIn(
            "73d528801aefc032d3a53637b035f65d72809151b2f127c6b2050e9bacc76f3b",
            guide,
        )
        self.assertNotIn("OpenRDXManager", guide)
        self.assertNotIn("rdx_manager_firmware_update.ps1", guide)
        self.assertIn("Restore vendor firmware with TI FlashBurner", guide)
        self.assertIn("not a validated FlashBurner input", compact_guide)
        self.assertIn("Program Full Binary Image", guide)
        self.assertIn("UNVALIDATED / EXPERIMENTAL", compact_guide)
        self.assertIn("not been hardware-tested", compact_guide)
        self.assertIn("262144", guide)
        self.assertIn("7820999743", guide)

    def test_guide_includes_j7_photos_and_is_linked_from_the_index(self):
        """Keep the physical locator evidence present and easy to find."""

        guide = GUIDE_PATH.read_text(encoding="utf-8")
        index = DOCS_INDEX_PATH.read_text(encoding="utf-8")
        root_readme = ROOT_README_PATH.read_text(encoding="utf-8")

        self.assertIn("../assets/rdx-board-j7-location.jpg", guide)
        self.assertIn("../assets/rdx-board-j7-close-up.jpg", guide)
        self.assertIn("development/rom-loader-recovery.md", index)
        self.assertIn("docs/development/rom-loader-recovery.md", root_readme)
        for photo_path in PHOTO_PATHS:
            self.assertTrue(photo_path.is_file(), str(photo_path))


if __name__ == "__main__":
    unittest.main()
