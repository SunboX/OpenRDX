# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Protect the ordered, single-translation-unit RDX core split."""

import hashlib
import pathlib
import re
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
RDX_CORE_WRAPPER = PROJECT_ROOT / "src" / "rdx_core.c"
RDX_CORE_DIRECTORY = PROJECT_ROOT / "src" / "rdx_core"
BUILD_ADAPTER = PROJECT_ROOT / "scripts" / "ti_cgt_build.py"
EXPECTED_IMPLEMENTATION_SHA256 = (
    "0e7562e6b686393ddeb3dfe408cc0f9ec7379fce8cc2a89131f256db503dc40f"
)
EXPECTED_FUNCTION_COUNT = 617
FRAGMENT_NAMES = [
    "01_image_validation.inc",
    "02_multiword_division.inc",
    "03_identity_and_mechanism.inc",
    "04_hash_flash_usb_thermal.inc",
    "05_device_descriptor_dispatch.inc",
    "06_record_transfer_helpers.inc",
    "07_eject_reconnect_smart.inc",
    "08_thermal_and_record_validation.inc",
    "09_command_phase_and_ata.inc",
    "10_spi_flash_and_response_dispatch.inc",
    "11_usb_ata_payload_transfer.inc",
    "12_ahci_smart_spi_control.inc",
    "13_runtime_transfer_helpers.inc",
    "14_mww_usb_command_control.inc",
    "15_control_transfer_crc_records.inc",
    "16_record_builders_and_capacity.inc",
    "17_cartridge_state_and_validation.inc",
    "18_firmware_transfer_and_persistence.inc",
    "19_identity_sense_and_mww.inc",
    "20_command_state_accessors.inc",
    "21_persistence_and_state_setters.inc",
    "22_state_queries_and_hardware_helpers.inc",
    "23_low_level_accessors.inc",
    "24_reconnect_state_machine.inc",
]
FUNCTION_DEFINITION_PATTERN = re.compile(
    rb"^(?:void|bool|char|int|int16_t|uint8_t|uint16_t|uint32_t|uint64_t)"
    rb"[ \t*]+core_[A-Za-z0-9_]+\(",
    re.MULTILINE,
)
SPDX_HEADER_PATTERN = re.compile(
    rb"\A/\*\n"
    rb" \* SPDX-FileCopyrightText:[^\n]+\n"
    rb" \*\n"
    rb" \* SPDX-License-"
    rb"Identifier:[^\n]+\n"
    rb" \*/\n\n"
)


class RdxCoreFragmentTests(unittest.TestCase):
    """Verify that splitting the inactive core cannot change its code stream."""

    def fragment_paths(self):
        """Return the approved fragments in compiler include order."""
        return [RDX_CORE_DIRECTORY / name for name in FRAGMENT_NAMES]

    def test_wrapper_includes_every_fragment_in_order(self):
        """Keep the preprocessor order equal to the defined function order."""
        wrapper = RDX_CORE_WRAPPER.read_text(encoding="utf-8")
        includes = re.findall(r'^#include "rdx_core/([^"]+\.inc)"$', wrapper, re.MULTILINE)
        self.assertEqual(FRAGMENT_NAMES, includes)
        self.assertIsNone(FUNCTION_DEFINITION_PATTERN.search(wrapper.encode("utf-8")))

    def test_wrapper_and_fragments_stay_below_one_thousand_lines(self):
        """Enforce the repository's source-file size ceiling."""
        paths = [RDX_CORE_WRAPPER] + self.fragment_paths()
        violations = []
        for path in paths:
            if not path.is_file():
                violations.append(f"missing: {path.relative_to(PROJECT_ROOT)}")
                continue
            line_count = len(path.read_text(encoding="utf-8").splitlines())
            if line_count >= 1000:
                violations.append(
                    f"{path.relative_to(PROJECT_ROOT)}: {line_count} lines"
                )
        self.assertEqual([], violations, "\n".join(violations))

    def test_concatenated_fragments_preserve_the_defined_implementation(self):
        """Detect unreviewed changes or reordering in this branch's implementation."""
        fragment_paths = self.fragment_paths()
        missing = [
            str(path.relative_to(PROJECT_ROOT))
            for path in fragment_paths
            if not path.is_file()
        ]
        self.assertEqual([], missing, "missing fragments: {}".format(", ".join(missing)))
        # Git may check these inactive fragments out with CRLF on Windows.
        # Hash their canonical LF form so the guard remains independent of
        # core.autocrlf while still detecting content changes.
        implementation = b"".join(
            SPDX_HEADER_PATTERN.sub(
                b"",
                path.read_bytes().replace(b"\r\n", b"\n"),
                count=1,
            )
            for path in fragment_paths
        )
        self.assertEqual(
            EXPECTED_IMPLEMENTATION_SHA256,
            hashlib.sha256(implementation).hexdigest(),
        )
        self.assertEqual(
            EXPECTED_FUNCTION_COUNT,
            len(FUNCTION_DEFINITION_PATTERN.findall(implementation)),
        )

    def test_mount_build_excludes_inactive_core_fragments(self):
        """The OpenRDX mount image must not compile inactive fragments."""
        adapter = BUILD_ADAPTER.read_text(encoding="utf-8")
        self.assertIn(
            'SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"',
            adapter,
        )
        self.assertNotIn("RDX_CORE_FRAGMENTS", adapter)
        self.assertNotIn("firmware_absolute_symbols.cmd", adapter)


if __name__ == "__main__":
    unittest.main()
