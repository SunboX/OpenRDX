# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Exercise automatic distribution packaging with real container inputs."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

from scripts.package_github_release import validate_bundle
from test_build_rdx_update_container import find_compatibility_template


ROOT = Path(__file__).resolve().parents[1]
HEX = ":020000040800F2\n:0400000001020304F2\n:00000001FF\n"


class BuildDistTests(unittest.TestCase):
    """Prevent stale bundles, mismatched versions, and partial replacements."""

    def setUp(self):
        """Create a minimal source tree with real, checksummed HEX input."""
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name) / "project with spaces"
        self.build = self.root / ".pio/build/tusb9261_ti_cgt"
        self.build.mkdir(parents=True)
        self.environment = dict(os.environ)
        self.environment.pop("OPENRDX_TEMPLATE_PATH", None)
        self.set_version("1.07")
        for name in ("TUSB9261_RDX.hex", "TUSB9261_RDX_flash.hex"):
            (self.build / name).write_text(HEX, encoding="ascii")
        for name, text in (
            ("scripts/rdx_manager_firmware_update.ps1", "guarded updater\n"),
            ("docs/getting-started/installation.md", "installation guide\n"),
        ):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
        self.dist = self.root / "dist"
        self.dist.mkdir()
        (self.dist / "OpenRDX-v1-06.bin").write_bytes(b"old bundle")

    def set_version(self, version):
        """Change both release inputs to model an actual version bump."""
        major, minor = version.split(".")
        (self.root / "VERSION").write_text(version + "\n", encoding="ascii")
        header = self.root / "include/rdx_mount/tusb9260.h"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text(
            f"#define FIRMWARE_MAJOR_VERSION {int(major)}\n"
            f"#define FIRMWARE_MINOR_VERSION {int(minor)}\n", encoding="ascii",
        )

    def use_template(self):
        """Use the real pinned input when the build environment provides it."""
        template = find_compatibility_template()
        if template is None:
            self.skipTest("compatibility template is unavailable")
        self.environment["OPENRDX_TEMPLATE_PATH"] = str(template)

    def run_packager(self):
        """Run the public packaging command and retain failure diagnostics."""
        return subprocess.run(
            [sys.executable, str(ROOT / "scripts/build_dist.py"),
             "--root", str(self.root), "--build-dir", str(self.build)],
            env=self.environment, capture_output=True, text=True, timeout=30,
        )

    def test_refreshes_complete_bundle_and_removes_retired_version(self):
        """A version bump must replace every bundle member with current input."""
        self.use_template()
        result = self.run_packager()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        bundle = validate_bundle(self.dist, "1.07")
        self.assertEqual(set(bundle), {p.name for p in self.dist.iterdir()})
        self.assertEqual(b"guarded updater\n", bundle["rdx_manager_firmware_update.ps1"])
        manifest = json.loads(bundle["OpenRDX-v1-07.json"])
        self.assertNotIn("firmware_hex", manifest)
        self.assertEqual("CompatibilityReceiver", manifest["required_receiver_kind"])
        self.assertEqual(b"\x01\x02\x03\x04", bundle["OpenRDX-v1-07.bin"][0x199:0x19D])

        # Repeated builds must repair missing files and use current VERSION.
        (self.dist / "OPENRDX_USB_UPDATE.md").unlink()
        self.set_version("1.08")
        self.environment.pop("OPENRDX_TEMPLATE_PATH")
        result = self.run_packager()
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        bundle = validate_bundle(self.dist, "1.08")
        self.assertEqual(set(bundle), {p.name for p in self.dist.iterdir()})

    def test_missing_template_fails_without_replacing_previous_bundle(self):
        """Packaging must fail explicitly when the required template is absent."""
        self.environment["OPENRDX_TEMPLATE_PATH"] = str(self.root / "missing.bin")
        result = self.run_packager()
        self.assertNotEqual(0, result.returncode)
        self.assertIn("template", result.stderr.lower())
        self.assertEqual(["OpenRDX-v1-06.bin"], [p.name for p in self.dist.iterdir()])

    def test_invalid_inputs_preserve_previous_bundle(self):
        """Do not expose a new bundle after version or HEX validation fails."""
        self.use_template()
        for invalid in ("version", "hex", "template"):
            with self.subTest(invalid=invalid):
                self.set_version("1.07")
                (self.build / "TUSB9261_RDX.hex").write_text(HEX, encoding="ascii")
                self.use_template()
                if invalid == "version":
                    (self.root / "VERSION").write_text("1.08\n", encoding="ascii")
                elif invalid == "hex":
                    (self.build / "TUSB9261_RDX.hex").write_text("corrupt\n", encoding="ascii")
                else:
                    path = self.root / "bad-template.bin"
                    path.write_bytes(bytes(62110))
                    self.environment["OPENRDX_TEMPLATE_PATH"] = str(path)
                result = self.run_packager()
                self.assertNotEqual(0, result.returncode)
                self.assertIn({"version": "does not match", "hex": "HEX", "template": "SHA-256 mismatch"}[invalid], result.stderr)
                self.assertEqual(["OpenRDX-v1-06.bin"], [p.name for p in self.dist.iterdir()])

    def test_default_build_refreshes_dist_even_when_firmware_is_current(self):
        """Exercise the actual SCons adapter; only the TI tool action is replaced."""
        self.use_template()
        scons = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio")) / "packages/tool-scons/scons.py"
        if not scons.is_file():
            self.skipTest("PlatformIO SCons is unavailable")
        shutil.copytree(ROOT / "scripts", self.root / "scripts", dirs_exist_ok=True)
        (self.root / "src/rdx_mount").mkdir(parents=True)
        (self.root / "linker").mkdir()
        (self.root / "linker/tusb9260_link.cmd").write_text("fixture", encoding="ascii")
        (self.root / "SConstruct").write_text('''from pathlib import Path
env = Environment(tools=[])
env.Replace(PROJECT_DIR=Dir('.').abspath, BUILD_DIR=Dir('.pio/build/tusb9261_ti_cgt').abspath)
env.AddMethod(lambda self, name, default='': default, 'GetProjectOption')
base_command = env.Command
def command(self, target, source, action):
    if getattr(action, '__name__', '') == 'build_with_ti_cgt':
        def fixture_firmware(target, source, env):
            Path('compile-count').open('a').write('compiled\\n')
            for path in target:
                output = Path(str(path))
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_text(':020000040800F2\\n:0400000001020304F2\\n:00000001FF\\n')
        action = fixture_firmware
    return base_command(target, source, action)
env.AddMethod(command, 'Command')
SConscript('scripts/ti_cgt_build.py', exports='env')
Default(env.BuildProgram())
''', encoding="utf-8")
        for attempt in range(2):
            result = subprocess.run(
                [sys.executable, str(scons), "-Q"], cwd=self.root,
                env=self.environment, capture_output=True, text=True, timeout=30,
            )
            self.assertEqual(0, result.returncode, result.stdout + result.stderr)
            self.assertTrue((self.dist / "OpenRDX-v1-07.bin").is_file(), result.stdout)
            bundle = validate_bundle(self.dist, "1.07")
            self.assertEqual(set(bundle), {p.name for p in self.dist.iterdir()})
            if attempt == 0:
                (self.dist / "OpenRDX-v1-07.bin").unlink()
                (self.dist / "OpenRDX-v1-06.bin").write_bytes(b"stale")
        self.assertEqual("compiled\n", (self.root / "compile-count").read_text())

        # A packaging failure must also fail an otherwise up-to-date build.
        self.environment["OPENRDX_TEMPLATE_PATH"] = str(self.root / "missing.bin")
        result = subprocess.run(
            [sys.executable, str(scons), "-Q"], cwd=self.root,
            env=self.environment, capture_output=True, text=True, timeout=30,
        )
        self.assertNotEqual(0, result.returncode)
        self.assertIn("template", result.stderr.lower())
        validate_bundle(self.dist, "1.07")

    def test_release_gate_stops_before_build_when_tests_fail(self):
        """The PowerShell wrapper must keep failed tests from updating dist."""
        pwsh = shutil.which("pwsh") or shutil.which("powershell")
        if not pwsh:
            self.skipTest("PowerShell is unavailable")
        shutil.copyfile(ROOT / "build-dist.ps1", self.root / "build-dist.ps1")
        test_runner = self.root / "test-runner.ps1"
        test_runner.write_text(
            "Add-Content -LiteralPath 'calls.txt' -Value ('tests ' + ($args -join ' '))\n"
            "exit 1\n", encoding="utf-8",
        )
        pio = self.root / "pio.ps1"
        pio.write_text(
            "Add-Content -LiteralPath 'calls.txt' -Value ('build ' + ($args -join ' '))\n"
            "exit 0\n", encoding="utf-8",
        )
        command = [pwsh, "-NoProfile", "-NonInteractive", "-File",
                   str(self.root / "build-dist.ps1"), "-PlatformIoPath", str(pio),
                   "-PythonPath", str(test_runner)]
        failed = subprocess.run(command, cwd=self.root, capture_output=True, text=True, timeout=30)
        self.assertNotEqual(0, failed.returncode)
        calls = self.root / "calls.txt"
        self.assertEqual("tests -m unittest discover -s tests -v\n", calls.read_text(encoding="utf-8-sig"))
        self.assertEqual(["OpenRDX-v1-06.bin"], [p.name for p in self.dist.iterdir()])
        calls.unlink()
        test_runner.write_text(test_runner.read_text().replace("exit 1", "exit 0"))
        passed = subprocess.run(command, cwd=self.root, capture_output=True, text=True, timeout=30)
        self.assertEqual(0, passed.returncode, passed.stdout + passed.stderr)
        self.assertEqual(
            ["tests -m unittest discover -s tests -v", "build run -e tusb9261_ti_cgt"],
            calls.read_text(encoding="utf-8-sig").splitlines(),
        )


if __name__ == "__main__":
    unittest.main()
