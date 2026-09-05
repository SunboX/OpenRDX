"""Behavior and scope checks for the standalone RDX lock-slider test."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
SWITCH_HEADER = PROJECT_DIR / "include" / "switch_test.h"
SWITCH_SOURCE = PROJECT_DIR / "src" / "main.c"
STARTUP_SOURCE = PROJECT_DIR / "src" / "startup.c"
BUILD_ADAPTER = PROJECT_DIR / "scripts" / "ti_cgt_build.py"


class SwitchTestFirmwareTests(unittest.TestCase):
    """Lock the confirmed MCP3008-to-green-LED test behavior."""

    def test_lock_slider_uses_confirmed_adc_mapping(self):
        """Bind the lock slider to the hardware-confirmed ADC channel."""
        self.assertTrue(
            SWITCH_HEADER.is_file(), "include/switch_test.h is missing"
        )
        header = SWITCH_HEADER.read_text(encoding="utf-8")
        expected_definitions = {
            "FRONT_GREEN_LED_MASK": "(1UL << 7)",
            "FRONT_AMBER_LED_MASK": "(1UL << 6)",
            "LOCK_SLIDER_ADC_CHANNEL": "4UL",
            "LOCK_SLIDER_ADC_THRESHOLD": "650UL",
            "MCP3008_START_WORD": "0x11050001UL",
            "MCP3008_CHANNEL_WORD": "0x11050080UL",
            "MCP3008_FINISH_WORD": "0x01050000UL",
        }

        for name, value in expected_definitions.items():
            with self.subTest(name=name):
                self.assertRegex(
                    header,
                    rf"(?m)^#define {name}\s+{re.escape(value)}$",
                )

    def test_leds_start_off_before_output_enable(self):
        """Prevent either active-low front LED channel flashing at startup."""
        source = SWITCH_SOURCE.read_text(encoding="utf-8")
        preload = (
            "GIOOUT0 |= FRONT_GREEN_LED_MASK | FRONT_AMBER_LED_MASK;"
        )
        output_direction = (
            "GIODIR0 |= FRONT_GREEN_LED_MASK | FRONT_AMBER_LED_MASK;"
        )

        self.assertIn(preload, source)
        self.assertIn(output_direction, source)
        self.assertLess(source.index(preload), source.index(output_direction))

    def test_mcp3008_uses_recovered_three_word_transaction(self):
        """Preserve the production MCP3008 command ordering and bit assembly."""
        source = SWITCH_SOURCE.read_text(encoding="utf-8")

        self.assertIn(
            "spi_transfer_word(MCP3008_START_WORD, &receive_word)",
            source,
        )
        self.assertIn(
            "MCP3008_CHANNEL_WORD | ((channel & 7UL) << 4)",
            source,
        )
        self.assertIn(
            "spi_transfer_word(MCP3008_FINISH_WORD, &receive_word)",
            source,
        )
        self.assertIn("upper_bits = (receive_word & 3UL) << 8;", source)
        self.assertIn(
            "*sample = upper_bits | (receive_word & 0xFFUL);",
            source,
        )

    def test_main_displays_confirmed_lock_slider_on_green(self):
        """Show the confirmed channel-four lock input on green only."""
        source = SWITCH_SOURCE.read_text(encoding="utf-8")
        expected_display = (
            "front_green_led_write(\n"
            "                channel_four > LOCK_SLIDER_ADC_THRESHOLD);"
        )

        self.assertIn("LOCK_SLIDER_ADC_CHANNEL, &channel_four", source)
        self.assertNotIn("mcp3008_read_channel(2UL", source)
        self.assertNotIn("channel_two", source)
        self.assertIn(expected_display, source)
        self.assertIn("for (;;) {", source)

    def test_main_has_no_production_or_mechanism_behavior(self):
        """Exclude production services and physical-output actuation."""
        source = SWITCH_SOURCE.read_text(encoding="utf-8").lower()
        forbidden_terms = (
            "motor",
            "pwm",
            "eject",
            "usb_",
            "sata",
            "cartridge",
            "watchdog",
        )

        for term in forbidden_terms:
            with self.subTest(term=term):
                self.assertNotIn(term, source)

    def test_build_contains_only_the_diagnostic_runtime_sources(self):
        """Compile only the two minimal C and two vector assembly sources."""
        adapter = BUILD_ADAPTER.read_text(encoding="utf-8")
        self.assertIn('C_SOURCE_NAMES = ["main.c", "startup.c"]', adapter)
        self.assertIn(
            'ASM_SOURCE_NAMES = ["exceptions_isr.asm", "intvecs.asm"]',
            adapter,
        )
        self.assertTrue(STARTUP_SOURCE.is_file(), "src/startup.c is missing")

        actual_sources = {
            path.relative_to(PROJECT_DIR).as_posix()
            for path in (PROJECT_DIR / "src").rglob("*")
            if path.is_file()
        }
        self.assertEqual(
            {
                "src/main.c",
                "src/startup.c",
                "src/exceptions_isr.asm",
                "src/intvecs.asm",
            },
            actual_sources,
        )
        actual_headers = {
            path.relative_to(PROJECT_DIR).as_posix()
            for path in (PROJECT_DIR / "include").rglob("*")
            if path.is_file()
        }
        self.assertEqual({"include/switch_test.h"}, actual_headers)

        removed_paths = (
            "src/rdx_core.c",
            "src/usb_stack.c",
            "src/ahci.c",
            "src/spi.c",
            "linker/firmware_absolute_symbols.cmd",
        )
        for relative_path in removed_paths:
            with self.subTest(path=relative_path):
                self.assertFalse(
                    (PROJECT_DIR / relative_path).exists(),
                    f"production path still exists: {relative_path}",
                )


if __name__ == "__main__":
    unittest.main()
