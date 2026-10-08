# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Run the actual BOT error cleanup against local endpoint accounting models."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parents[1]
CC = shutil.which("cc")
MSVC = shutil.which("cl")


def cleanup_probe_directory(temporary):
    """Allow Windows to release a just-executed test binary before temp cleanup."""
    for attempt in range(10):
        try:
            temporary.cleanup()
            return
        except PermissionError:
            if attempt == 9:
                raise
            time.sleep(0.05)


@unittest.skipUnless(CC or MSVC, "Host logic test needs cc or configured MSVC")
class BotCleanupTests(unittest.TestCase):
    """Completion callbacks must not count an OUT transfer a second time."""

    def test_actual_cleanup_handles_active_and_completed_transfers(self):
        """Compile and execute production cleanup without USB or register access."""
        source = (ROOT / "src/rdx_mount/ums_bot.c").read_text(encoding="utf-8")
        signature = "void ums_bot_xfer_cleanup(BOOLEAN_T stall_active_endpt)"
        self.assertEqual(1, source.count(signature))
        start = source.index(signature)
        end = source.index("\n}", start) + 2
        fixture = (ROOT / "tests/fixtures/bot_cleanup_probe.c").read_text(
            encoding="utf-8"
        )
        temporary = tempfile.TemporaryDirectory(prefix="openrdx-bot-cleanup-")
        self.addCleanup(cleanup_probe_directory, temporary)
        with self.subTest(scope="RAM-only host executable"):
            tmp = temporary.name
            directory = Path(tmp)
            generated = directory / "probe.c"
            generated.write_text(
                fixture.replace("/* PRODUCTION_CLEANUP */", source[start:end]),
                encoding="utf-8",
            )
            executable = directory / ("probe.exe" if MSVC and not CC else "probe")
            if CC:
                command = [CC, "-std=c99", "-Wall", "-Wextra", "-Werror",
                           str(generated), "-o", str(executable)]
            else:
                command = [MSVC, "/nologo", "/std:c11", "/W4", "/WX",
                           str(generated), f"/Fe:{executable}",
                           f"/Fo:{directory / 'probe.obj'}"]
            built = subprocess.run(command, cwd=directory, capture_output=True,
                                   text=True, timeout=30)
            self.assertEqual(0, built.returncode, built.stdout + built.stderr)
            result = subprocess.run([str(executable)], cwd=directory,
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(0, result.returncode, result.stdout + result.stderr)
            self.assertIn("PASS 7 BOT cleanup scenarios", result.stdout)


if __name__ == "__main__":
    unittest.main()
