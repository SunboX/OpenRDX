# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Check documented in-place commands and the updater with simulated devices."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
UPDATER = ROOT / "scripts" / "rdx_manager_firmware_update.ps1"
PROCEDURE = ROOT / "docs" / "getting-started" / "installation.md"
POWERSHELL = shutil.which("powershell.exe") or shutil.which("pwsh")


class OpenRdxUpdateDocumentationTests(unittest.TestCase):
    """Keep the operator command and its validation boundary explicit."""

    def test_existing_receiver_has_explicit_image_and_manifest(self):
        """Prevent vendor restore or vendor serial rules entering this route."""
        guide = PROCEDURE.read_text(encoding="utf-8").split(
            "## Update existing OpenRDX", 1
        )[1]
        self.assertIn("-ValidateOnly -ValidateFirmwareKind OpenRDX", guide)
        self.assertIn(".\\rdx_manager_firmware_update.ps1 -InstallOpenRDX `", guide)
        version = (ROOT / "VERSION").read_text(encoding="ascii").strip()
        release_name = "OpenRDX-v" + version.replace(".", "-")
        self.assertIn(f"-ImagePath .\\{release_name}.bin", guide)
        self.assertIn(f"-ManifestPath .\\{release_name}.json", guide)
        self.assertIn("-TargetSerialNumber $targetSerial", guide)
        self.assertNotIn("{10}", guide)
        self.assertIn("Do not substitute `-Update`", guide)
        self.assertIn("not a recorded\nhardware qualification", guide)
        self.assertIn("does not require observing disappearance", guide)


@unittest.skipUnless(POWERSHELL, "PowerShell required for simulated host workflow")
class OpenRdxUpdateWorkflowTests(unittest.TestCase):
    """Run unchanged main-flow code with fake transport and storage cmdlets."""

    def simulate(self, scenario="success", *, corrupt_image=False, bad_manifest=False,
                 initial_revision="0001", reconnected_revision="0001", pnp_revision=""):
        """Create disposable host-validation fixtures and return recorded I/O."""
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "fixture.bin"
            manifest = image.with_suffix(".json")
            # These bytes test host guards only; the receiver is not emulated.
            content = bytes(62110)
            image.write_bytes(content)
            metadata = {
                "container_length": len(content),
                "container_sha256": hashlib.sha256(content).hexdigest(),
                "authentication_scheme": "openrdx-sha256-v1",
                "requires_openrdx_receiver": True,
                "installation_requires_rom_loader": True,
                "template_sha256": (
                    "73d528801aefc032d3a53637b035f65d72809151b2f127c6b2050e9bacc76f3b"
                ),
            }
            if corrupt_image:
                image.write_bytes(b"x" + content[1:])
            if bad_manifest:
                metadata["authentication_scheme"] = "unsupported"
            manifest.write_text(json.dumps(metadata), encoding="utf-8")
            # Let Windows PowerShell locate its own modules when launched from
            # a Python process that inherited PowerShell 7's module path.
            environment = dict(os.environ)
            for key in list(environment):
                if key.lower() == "psmodulepath":
                    del environment[key]
            completed = subprocess.run(
                [
                    POWERSHELL, "-NoProfile", "-NonInteractive", "-File",
                    str(ROOT / "tests" / "openrdx_update_simulation.ps1"),
                    "-UpdaterPath", str(UPDATER), "-ImagePath", str(image),
                    "-ManifestPath", str(manifest), "-Scenario", scenario,
                    "-InitialRevision", initial_revision,
                    "-ReconnectedRevision", reconnected_revision,
                    *(["-PnpRevision", pnp_revision] if pnp_revision else []),
                ],
                capture_output=True, text=True, timeout=30, env=environment,
            )
            self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)
            results = [
                line.removeprefix("WORKFLOW_RESULT ")
                for line in completed.stdout.splitlines()
                if line.startswith("WORKFLOW_RESULT ")
            ]
            self.assertEqual(1, len(results), completed.stdout + completed.stderr)
            return json.loads(results[0])

    def test_in_place_transfer_uses_sixteen_chunks_then_activation(self):
        """Require exact offsets, sizes, buffer zero, and no vendor bypass."""
        result = self.simulate()
        self.assertIsNone(result["error"])
        expected = []
        for offset in range(0, 62110, 4096):
            length = min(4096, 62110 - offset)
            cdb = bytes([0x3B, 4, 0]) + offset.to_bytes(3, "big")
            cdb += length.to_bytes(3, "big") + b"\0"
            expected.append((cdb, length))
        expected.append((bytes.fromhex("3B 05 00 00 F2 9E 00 00 00 00"), 0))
        actual = []
        for command in result["commands"]:
            path, cdb, length = command.split("|")
            self.assertEqual(r"\\.\PhysicalDrive99", path)
            actual.append((bytes.fromhex(cdb.replace("-", " ")), int(length)))
        self.assertEqual(expected, actual)
        self.assertGreaterEqual(result["disk_queries"], 3)

    def test_invalid_prerequisites_send_no_commands(self):
        """Reject occupied bay, changed identity, modified bytes or manifest."""
        for scenario, options, message in (
            ("media", {}, "physically remove"),
            ("identity", {}, "identity changed"),
            ("success", {"corrupt_image": True}, "manifest validation failed"),
            ("success", {"bad_manifest": True}, "manifest validation failed"),
        ):
            with self.subTest(scenario=scenario, options=options):
                result = self.simulate(scenario, **options)
                self.assertIn(message, result["error"])
                self.assertEqual([], result["commands"])

    def test_versioned_receivers_and_legacy_upgrades_remain_updateable(self):
        """Accept release revisions before transfer and after a legacy reset."""
        version = (ROOT / "VERSION").read_text(encoding="ascii").strip()
        major, minor = map(int, version.split("."))
        current_revision = f"{major:02d}{minor:02d}"
        for initial, reconnected in (("0001", current_revision),
                                     ("0106", current_revision),
                                     (current_revision, current_revision)):
            with self.subTest(initial=initial, reconnected=reconnected):
                result = self.simulate(initial_revision=initial,
                                       reconnected_revision=reconnected)
                self.assertIsNone(result["error"])
                self.assertEqual(17, len(result["commands"]))

    def test_unrecognized_or_inconsistent_revision_sends_no_commands(self):
        """Do not admit vendor firmware or a mismatched Windows PnP identity."""
        for revision, pnp_revision in (("0283", "0283"), ("9999", "9999"),
                                       ("0106", "0001"), ("1.06", "0106")):
            with self.subTest(revision=revision, pnp_revision=pnp_revision):
                result = self.simulate(initial_revision=revision,
                                       pnp_revision=pnp_revision)
                self.assertIsNotNone(result["error"])
                self.assertEqual([], result["commands"])

    def test_final_authentication_error_never_activates(self):
        """In-place transfer must fail after its bounded retry, not stage ROM."""
        result = self.simulate("authentication")
        self.assertIn("07/74/08", result["error"])
        self.assertEqual(32, len(result["commands"]))
        self.assertTrue(all("|3B-04-00-" in cmd for cmd in result["commands"]))


if __name__ == "__main__":
    unittest.main()
