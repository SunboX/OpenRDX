# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Lock the logical-output names for the normal OpenRDX firmware."""

import pathlib
import re
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
SPI_HEADER = PROJECT_ROOT / "include" / "spi.h"
DOCK_CONTROLLER_SOURCE = PROJECT_ROOT / "src" / "ums_bot.c"
CARTRIDGE_CONTROLLER_SOURCE = (
    PROJECT_ROOT / "src" / "rdx_core" / "13_runtime_transfer_helpers.inc"
)
LOGICAL_OUTPUT_SOURCE = PROJECT_ROOT / "src" / "spi.c"
MAPPING_DOCUMENT = PROJECT_ROOT / "docs" / "LED_OUTPUT_MAPPING.md"


class MainLedMappingTests(unittest.TestCase):
    """Verify the LED mapping remains explicit and traceable."""

    def test_header_defines_the_logical_outputs(self):
        """Expose the four LED logical-output values by their roles."""
        header = SPI_HEADER.read_text(encoding="utf-8")
        expected_constants = {
            "RDX_CARTRIDGE_GREEN_LOGICAL_OUTPUT": "6U",
            "RDX_CARTRIDGE_AMBER_LOGICAL_OUTPUT": "7U",
            "RDX_DOCK_AMBER_LOGICAL_OUTPUT": "8U",
            "RDX_DOCK_GREEN_LOGICAL_OUTPUT": "9U",
        }
        for name, value in expected_constants.items():
            self.assertRegex(header, re.compile(rf"^#define {name} {value}$", re.MULTILINE))

    def test_dock_controller_uses_amber_then_green_outputs(self):
        """Keep the dock controller paired with its first amber output."""
        source = DOCK_CONTROLLER_SOURCE.read_text(encoding="utf-8")
        self.assertIn(
            "core_process_clear_memory_spi_response_context(\n"
            "        0x800f29c,\n"
            "        RDX_DOCK_AMBER_LOGICAL_OUTPUT,\n"
            "        RDX_DOCK_GREEN_LOGICAL_OUTPUT);",
            source,
        )

    def test_cartridge_controller_uses_green_then_amber_outputs(self):
        """Keep the cartridge controller paired with cartridge green then amber."""
        source = CARTRIDGE_CONTROLLER_SOURCE.read_text(encoding="utf-8")
        self.assertIn(
            "core_process_clear_memory_spi_response_context(\n"
            "        0x800f2b4,\n"
            "        RDX_CARTRIDGE_GREEN_LOGICAL_OUTPUT,\n"
            "        RDX_CARTRIDGE_AMBER_LOGICAL_OUTPUT);",
            source,
        )

    def test_logical_output_setter_retains_active_low_polarity_branch(self):
        """Retain the polarity-zero path that drives an asserted LED low."""
        source = LOGICAL_OUTPUT_SOURCE.read_text(encoding="utf-8")
        self.assertRegex(
            source,
            re.compile(
                r"operation_status = 1;\n"
                r"    if \(condition == 0\) \{\n"
                r"        spi_get_spi_transfer_storage_result = "
                r"\(uint32_t \*\)spi_get_spi_transfer_storage\(context\);\n"
                r"        if \(\(char\)spi_get_spi_transfer_storage_result\[1\] == '\\0'\)\n"
                r"            goto spi_set_logical_output_before_updating_status_bits;\n"
                r"    \} else \{\n"
                r"        spi_get_spi_transfer_storage_result = "
                r"\(uint32_t \*\)spi_get_spi_transfer_storage\(context\);\n"
                r"        if \(\(char\)spi_get_spi_transfer_storage_result\[1\] != '\\0'\)\n"
                r"            goto spi_set_logical_output_before_updating_status_bits;\n"
                r"    \}\n"
                r"    operation_status = 0;",
            ),
        )

    def test_logical_output_setter_masks_active_low_gpio_and_sci_outputs(self):
        """Keep GPIO and SCI active-low output masks and their final write aligned."""
        source = LOGICAL_OUTPUT_SOURCE.read_text(encoding="utf-8")
        branch_patterns = (
            (
                "GPIO logical outputs below 8",
                r"if \(status_bits < 8\) \{\n"
                r"        working_result = 1 << \(status_bits & 0xff\);\n"
                r"        pin_mask = working_result;\n"
                r"        if \(operation_status == 0\) \{\n"
                r"            pin_mask = 0;\n"
                r"        \}\n"
                r"        /\* Snapshot the fixed-address register before decoding its status or capability fields\. \*/\n"
                r"        data_cursor = \(uint8_t \*\)&GIOOUT0_REG_OFF;",
            ),
            (
                "SCI logical outputs 8 and 9",
                r"else if \(\(status_bits == 8\) \|\| \(status_bits == 9\)\) \{\n"
                r"        working_result = 1 << \(status_bits - 7 & 0xff\);\n"
                r"        pin_mask = working_result;\n"
                r"        if \(operation_status == 0\) \{\n"
                r"            pin_mask = 0;\n"
                r"        \}\n"
                r"        data_cursor = \(uint8_t \*\)&SCI_PIO3;",
            ),
        )
        for branch_name, pattern in branch_patterns:
            with self.subTest(branch=branch_name):
                self.assertRegex(source, re.compile(pattern))
        self.assertIn(
            "rti_update_masked_register_bits(data_cursor, working_result, pin_mask);",
            source,
        )

    def test_mapping_document_defines_every_led_channel(self):
        """Require a physical mapping for every dock and cartridge channel."""
        self.assertTrue(MAPPING_DOCUMENT.is_file(), "LED output mapping is missing")
        mapping = MAPPING_DOCUMENT.read_text(encoding="utf-8")
        expected = {
            "DOCK_GREEN": "SCI GPIO9 via logical output 9",
            "DOCK_AMBER": "SCI GPIO8 via logical output 8",
            "CARTRIDGE_GREEN": "GPIO7 via logical output 6",
            "CARTRIDGE_AMBER": "GPIO6 via logical output 7",
        }
        for section, selector in expected.items():
            with self.subTest(section=section):
                self.assertIn("## {}\n".format(section), mapping)
                self.assertIn("- Selector: `{}`".format(selector), mapping)
        self.assertEqual(4, mapping.count("- Polarity: `active-low`"))


if __name__ == "__main__":
    unittest.main()
