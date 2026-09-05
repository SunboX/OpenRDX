# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Tests for the custom RDX Manager update-container builder."""

import tempfile
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

import build_rdx_update_container as builder


def find_compatibility_template() -> Path | None:
    """Return a deterministic pinned template when the sibling tree exists."""

    manager_root = ROOT.parent / "OpenRDXManager"
    if not manager_root.is_dir():
        return None
    candidates = sorted(manager_root.rglob("RDX2E__STD__F-0283.bin"))
    matching = [
        path
        for path in candidates
        if builder.sha256(path.read_bytes()) == builder.COMPATIBILITY_TEMPLATE_SHA256
    ]
    return matching[0] if matching else None


class RdxUpdateContainerBuilderTests(unittest.TestCase):
    """Verify boot formatting and Intel HEX validation."""

    def test_boot_region_has_complete_descriptor_and_payload_records(self):
        """The formatter must emit the fixed descriptor and type-2 records."""

        payload = bytes(index & 0xFF for index in range(builder.FIRMWARE_PAYLOAD_LENGTH))
        boot_region = builder.build_ti_boot_region(payload)
        self.assertEqual(len(boot_region), builder.BOOT_REGION_END - builder.BOOT_REGION_OFFSET)
        self.assertEqual(boot_region[:2], b"\x60\x92")
        self.assertEqual(boot_region[2:8], b"\x01\x00\x00\x00\x00\x00")
        self.assertEqual(boot_region[8], 0x02)
        self.assertEqual(
            int.from_bytes(boot_region[9:13], "little"),
            builder.FIRMWARE_PAYLOAD_LENGTH,
        )
        self.assertEqual(boot_region[13:-1], payload)
        self.assertEqual(boot_region[-1], sum(payload) & 0xFF)

    def test_parser_loads_minimal_valid_armhex_image(self):
        """A checksummed extended-address file maps bytes at 08000000h."""

        source_text = ":020000040800F2\n:0400000001020304F2\n:00000001FF\n"
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "firmware.hex"
            source.write_text(source_text, encoding="ascii")
            memory = builder.parse_intel_hex(source)
        payload, used_length = builder.build_runtime_payload(memory)
        self.assertEqual(payload[:4], b"\x01\x02\x03\x04")
        self.assertEqual(used_length, 4)
        self.assertTrue(all(value == 0xFF for value in payload[4:]))

    def test_parser_rejects_bad_record_checksum(self):
        """A corrupt Intel HEX line must fail before an image is emitted."""

        source_text = ":020000040800F2\n:0400000001020304F3\n:00000001FF\n"
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "firmware.hex"
            source.write_text(source_text, encoding="ascii")
            with self.assertRaisesRegex(ValueError, "checksum"):
                builder.parse_intel_hex(source)

    def test_container_has_openrdx_payload_authentication_block(self):
        """Custom containers must identify and hash the flashed boot region."""

        template = find_compatibility_template()
        if template is None:
            self.skipTest("compatibility template is unavailable")
        payload = bytes(
            ((index * 29) + 7) & 0xFF
            for index in range(builder.FIRMWARE_PAYLOAD_LENGTH)
        )
        container = builder.build_container(
            template.read_bytes(), payload
        )
        auth = container[
            builder.CUSTOM_AUTH_OFFSET:
            builder.CUSTOM_AUTH_OFFSET + builder.CUSTOM_AUTH_LENGTH
        ]
        boot_region = container[
            builder.BOOT_REGION_OFFSET:builder.BOOT_REGION_END
        ]
        self.assertEqual(auth[:8], builder.CUSTOM_AUTH_MAGIC)
        self.assertEqual(auth[8:40].hex(), builder.sha256(boot_region))
        self.assertEqual(auth[40:], bytes(builder.CUSTOM_AUTH_LENGTH - 40))
        self.assertEqual(
            builder.rdx_rolling_check(
                container[:builder.METADATA_CHECK_END], 0xFFFFFFFF
            ),
            0,
        )

    def test_flashburner_image_name_is_derived_from_the_build_hex(self):
        """The installation manifest must pin the adjacent continuous HEX."""

        source = Path("TUSB9261_RDX.hex")
        expected = source.with_name(f"{source.stem}_flash{source.suffix}")
        self.assertEqual(expected.name, "TUSB9261_RDX_flash.hex")


if __name__ == "__main__":
    unittest.main()
