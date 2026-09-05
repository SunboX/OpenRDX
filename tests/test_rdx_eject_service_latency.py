# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Source contracts that keep GPIO2 polling responsive during motor motion."""

from pathlib import Path
import re
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT_DIR / "src" / "rdx_mount"
INCLUDE_DIR = PROJECT_DIR / "include" / "rdx_mount"
MAIN = (SOURCE_DIR / "main.c").read_text(encoding="utf-8")
SPI = (SOURCE_DIR / "spi.c").read_text(encoding="utf-8")
SPI_HEADER = (INCLUDE_DIR / "spi.h").read_text(encoding="utf-8")
SOC_HEADER = (INCLUDE_DIR / "tusb9260.h").read_text(encoding="utf-8")


def function_body(source, signature):
    """Return one C function body using balanced braces."""
    signature_start = source.index(signature)
    body_start = source.index("{", signature_start)
    depth = 0
    for index in range(body_start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[body_start : index + 1]
    raise AssertionError("unterminated function: {}".format(signature))


def integer_define(source, name):
    """Read one decimal or hexadecimal integer C preprocessor constant."""
    match = re.search(
        r"^#define\s+" + re.escape(name) + r"\s+(0x[0-9A-Fa-f]+|[0-9]+)",
        source,
        re.MULTILINE,
    )
    if match is None:
        raise AssertionError("missing integer definition: {}".format(name))
    return int(match.group(1), 0)


class RdxEjectServiceLatencyTests(unittest.TestCase):
    """Prevent sleep and shared-SPI waits from hiding motor endpoint changes."""

    def test_foreground_cannot_sleep_while_eject_is_active(self):
        """Polled GPIO2 must keep service even when the RTI tick requests WFI."""
        sleep = re.search(
            r"if\s*\(wfi_enable\s*&&\s*!rdx_hardware_eject_in_progress\(\)\)"
            r"\s*\{\s*wfi_enable\s*=\s*FALSE;\s*asm\(\" wfi\"\);",
            MAIN,
        )
        self.assertIsNotNone(sleep)
        self.assertEqual(1, MAIN.count('asm(" wfi")'))
        self.assertLess(MAIN.index("rdx_hardware_service();"), sleep.start())

    def test_stalled_adc_uses_elapsed_time_instead_of_poll_count(self):
        """A stuck SPI flag must release motor service within one millisecond."""
        wait = function_body(SPI, "static STATUS_T spi_wait_for_flag_bounded(")
        compact = re.sub(r"\s+", "", wait)
        self.assertIn(
            "if((UINT32_T)(READ_REG32(RTIFRC0_REG_OFF)-frame_started_us)"
            ">=MCP3008_FRAME_TIMEOUT_US){returnSTATUS_TIMEOUT;}",
            compact,
        )
        self.assertNotIn("iterations", wait)
        self.assertNotIn("usleep(", wait)
        self.assertEqual(1000, integer_define(SPI, "MCP3008_FRAME_TIMEOUT_US"))

    def test_frame_budget_has_margin_for_configured_spi_clock(self):
        """The finite motor-service budget exceeds the frame's wire time."""
        spi_format = integer_define(SPI_HEADER, "SPI_FORMAT1_MCP3008")
        bits_per_word = spi_format & 0x1F
        clock_divisor = ((spi_format >> 8) & 0xFF) + 1
        slowest_clock_mhz = min(
            integer_define(SOC_HEADER, "CPU_CLOCK_MHZ_FPGA"),
            integer_define(SOC_HEADER, "CPU_CLOCK_MHZ_ASIC"),
        )
        frame_wire_time_us = 3 * bits_per_word * clock_divisor / slowest_clock_mhz
        self.assertGreater(
            integer_define(SPI, "MCP3008_FRAME_TIMEOUT_US"),
            frame_wire_time_us * 10,
        )

    def test_all_three_adc_words_share_one_start_time(self):
        """A slow word cannot restart the frame budget for later words."""
        frame = function_body(SPI, "STATUS_T rdx_mcp3008_read_channel(")
        self.assertEqual(1, frame.count("READ_REG32(RTIFRC0_REG_OFF)"))
        self.assertIn("frame_started_us = READ_REG32(RTIFRC0_REG_OFF);", frame)
        calls = re.findall(
            r"status = spi_transfer_word_bounded\((.*?)\);", frame, re.DOTALL
        )
        self.assertEqual(3, len(calls))
        for call in calls:
            self.assertRegex(call, r",\s*&receive_word,\s*frame_started_us\s*$")
        self.assertLess(frame.index("*sample = 0U;"), frame.index(calls[0]))
        self.assertEqual(3, frame.count("if (status != STATUS_OK)"))

    def test_tx_and_rx_waits_cannot_restart_the_frame_budget(self):
        """Both completion flags inherit the caller's same elapsed-time bound."""
        transfer = function_body(SPI, "static STATUS_T spi_transfer_word_bounded(")
        for flag in ("SPI_FLAG_TX_AVAIL_BIT", "SPI_FLAG_RX_VALID_BIT"):
            self.assertIn(
                "spi_wait_for_flag_bounded({}, frame_started_us);".format(flag),
                transfer,
            )
        self.assertNotIn("READ_REG32(RTIFRC0_REG_OFF)", transfer)
        self.assertEqual(2, transfer.count("if (status != STATUS_OK)"))


if __name__ == "__main__":
    unittest.main()
