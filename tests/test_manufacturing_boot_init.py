# SPDX-FileCopyrightText: 2026 André Fiedler
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Guard the boot-only latch initializer required by TI ARM9 COFF startup."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/rdx_mount/rdx_manufacturing.c"


class ManufacturingBootInitializationTests(unittest.TestCase):
    """Host C zero initialization must not conceal the production ABI contract."""

    def test_latch_has_explicit_boot_initializer_and_no_runtime_clear(self):
        """Require a COFF initializer while preserving the post-mutation latch."""
        source = SOURCE.read_text(encoding="utf-8")
        self.assertRegex(source, r"static\s+BOOLEAN_T\s+rdx_mfg_mutation_started\s*=\s*FALSE\s*;")
        assignments = re.findall(r"\brdx_mfg_mutation_started\s*=\s*(\w+)\s*;", source)
        self.assertEqual(["FALSE", "TRUE"], assignments)
        self.assertNotIn("rdx_manufacturing_init", source)
        protocol = (ROOT / "src/rdx_mount/rdx_manager_protocol.c").read_text(encoding="utf-8")
        self.assertIn("if (rdx_manufacturing_mutation_started())", protocol)

    def test_ti_compiler_emits_zero_cinit_record_for_the_real_receiver(self):
        """Compile production C with TI 5.2, never substitute a host compiler."""
        configured = os.environ.get("TI_CGT_ROOT")
        if not configured:
            self.skipTest("Set TI_CGT_ROOT to verify the production TI COFF initializer")
        tool_root = Path(configured).expanduser()
        compiler = tool_root / "bin" / ("armcl.exe" if os.name == "nt" else "armcl")
        self.assertTrue(compiler.is_file(), str(compiler))
        revision = subprocess.run([str(compiler), "--compiler_revision"],
                                  capture_output=True, text=True, timeout=30)
        self.assertEqual(0, revision.returncode, revision.stderr)
        self.assertRegex(revision.stdout.strip(), r"^5\.2\.(5|9)$")
        with tempfile.TemporaryDirectory(prefix="rdx-boot-init-") as temporary:
            directory = Path(temporary)
            # TI's legacy Windows driver cannot reliably open non-ASCII paths.
            # Copy unchanged inputs to an isolated ASCII staging directory.
            shutil.copytree(ROOT / "include/rdx_mount", directory / "include")
            shutil.copyfile(SOURCE, directory / "rdx_manufacturing.c")
            command = [str(compiler), "-c", "-mv7m3", "--abi=ti_arm9_abi", "-me",
                       "--gcc", "--emit_warnings_as_errors", "--symdebug:none", "-O3",
                       "--keep_asm", f"--asm_directory={directory}",
                       f"--obj_directory={directory}",
                       f"--include_path={directory / 'include'}",
                       f"--include_path={tool_root / 'include'}", "rdx_manufacturing.c"]
            result = subprocess.run(command, cwd=directory, capture_output=True,
                                    text=True, errors="replace", timeout=60)
            self.assertEqual(0, result.returncode, result.stdout + result.stderr)
            assembly = (directory / "rdx_manufacturing.asm").read_text(encoding="utf-8")
            self.assertRegex(
                assembly,
                r'(?s)\.sect\s+"\.cinit"\s+\.align\s+4\s+'
                r'\.field\s+4,32\s+\.field\s+_rdx_mfg_mutation_started\+0,32\s+'
                r'\.bits\s+0,32',
            )


if __name__ == "__main__":
    unittest.main()
