# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Execute the production manufacturing receiver against simulated flash I/O."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


PROJECT_DIR = Path(__file__).resolve().parents[1]


def extract_function(source, signature):
    """Extract a production C function for a narrow executable dispatch fixture."""
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for position in range(opening, len(source)):
        if source[position] == "{":
            depth += 1
        elif source[position] == "}":
            depth -= 1
            if depth == 0:
                return source[start:position + 1]
    raise AssertionError("Unterminated production function")


class ManufacturingRestoreTests(unittest.TestCase):
    """Catch destructive writes admitted by stale, malformed, or unsafe requests."""

    def test_receiver_preserves_flash_and_rejects_unsafe_requests(self):
        """Compile real receiver C; fake only physical registers and SPI storage."""
        source = PROJECT_DIR / "src/rdx_mount/rdx_manufacturing.c"
        self.assertTrue(source.is_file(), "Manufacturing restore receiver is missing")
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("Host C compiler unavailable for receiver simulation")
        with tempfile.TemporaryDirectory(prefix="rdx-manufacturing-") as directory:
            staging = Path(directory)
            for name in (
                "rdx_manufacturing.h", "ahci.h", "gio.h", "rdx_hardware.h",
                "rdx_mechanism.h", "reg_io.h", "spi.h", "string.h",
                "tusb9260.h", "vim_nvic.h",
            ):
                (staging / name).write_text("", encoding="ascii")
            executable = staging / "receiver-test"
            harness = PROJECT_DIR / "tests/fixtures/manufacturing_receiver_harness.c"
            dispatcher = extract_function(
                (PROJECT_DIR / "src/rdx_mount/scsi.c").read_text(encoding="utf-8"),
                "inline STATUS_T scsi_handle_write_buffer_cmd(void)")
            test_source = staging / "receiver.c"
            transfer = (PROJECT_DIR / "src/rdx_mount/rdx_manager_serial.c").read_text(encoding="utf-8")
            selector = extract_function(transfer, "UINT8_T *rdx_manager_write_buffer_data(")
            test_source.write_text(selector + "\n" + source.read_text(encoding="utf-8") + "\n" +
                                   dispatcher.replace("inline STATUS_T", "STATUS_T", 1),
                                   encoding="utf-8")
            command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                       "-I", str(staging), "-include", str(harness),
                       str(test_source), "-o", str(executable)]
            build = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            for scenario in (
                "restore", "stale-record", "stale-sector", "corrupt-desired",
                "erased-desired", "erased-current", "corrupt-current",
                "occupied", "insertion-race", "mechanism", "eject", "sata",
                "callbacks", "malformed-cdb", "length", "null", "magic",
                "reserved", "capability", "noop", "read-failure",
                "erase-failure", "program-failure", "verify-failure", "spi-busy",
                "dispatcher", "short-cdb", "long-cdb", "update-active",
                "wren-insertion-race", "wren-failure", "null-cdb", "long-payload",
            ):
                with self.subTest(scenario=scenario):
                    result = subprocess.run([str(executable), scenario],
                                            capture_output=True, text=True, timeout=15)
                    self.assertEqual(result.returncode, 0,
                                     result.stdout + result.stderr)

    def test_bounded_spi_times_out_and_preserves_bus_ownership(self):
        """Execute the real SPI implementation with finite simulated register time."""
        source = PROJECT_DIR / "src/rdx_mount/spi.c"
        self.assertIn("STATUS_T SpiOpsBounded(", source.read_text(encoding="utf-8"),
                      "Bounded flash helper is missing")
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("Host C compiler unavailable for SPI simulation")
        with tempfile.TemporaryDirectory(prefix="rdx-spi-") as directory:
            staging = Path(directory)
            for name in ("spi.h", "gio.h", "reg_io.h", "rti.h", "sci.h", "tusb9260.h",
                         "tusb9260_types.h", "wdt.h"):
                (staging / name).write_text("", encoding="ascii")
            # Reuse the checked-in register definitions, keeping their real
            # addresses/bit fields in the executable low-level SPI test.
            header = (PROJECT_DIR / "include/rdx_mount/spi.h").read_text(encoding="utf-8")
            definitions = header[header.index("/*SPI register addresses*/"):
                                 header.index("/**\n * @brief Initialize")]
            (staging / "registers.h").write_text(definitions, encoding="utf-8")
            executable = staging / "spi-test"
            harness = PROJECT_DIR / "tests/fixtures/manufacturing_spi_harness.c"
            build = subprocess.run(
                [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                 "-I", str(staging), "-include", str(harness), str(source),
                 "-o", str(executable)], capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(executable)], capture_output=True,
                                    text=True, timeout=15)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
