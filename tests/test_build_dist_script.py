# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Validate the versioned distribution build contract."""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UPDATER = ROOT / "scripts" / "rdx_manager_firmware_update.ps1"
UPDATE_PROCEDURE = ROOT / "docs" / "getting-started" / "installation.md"
RELEASE_PROCESS = ROOT / "docs" / "development" / "release-process.md"


class BuildDistScriptTests(unittest.TestCase):
    """Keep release naming, firmware version, and safety checks synchronized."""

    def test_release_version_matches_compiled_firmware_version(self) -> None:
        """Require VERSION to describe the version compiled into the firmware."""

        release_version = (ROOT / "VERSION").read_text(encoding="ascii").strip()
        header = (ROOT / "include" / "rdx_mount" / "tusb9260.h").read_text(
            encoding="utf-8"
        )
        major = re.search(r"^#define\s+FIRMWARE_MAJOR_VERSION\s+(\d+)\s*$", header, re.M)
        minor = re.search(r"^#define\s+FIRMWARE_MINOR_VERSION\s+(\d+)\s*$", header, re.M)
        self.assertIsNotNone(major)
        self.assertIsNotNone(minor)
        compiled_version = f"{major.group(1)}.{minor.group(1).zfill(2)}"
        self.assertEqual(release_version, compiled_version)

    def test_script_builds_the_named_manager_container(self) -> None:
        """Require the distribution script to emit the documented release set."""

        script = (ROOT / "build-dist.ps1").read_text(encoding="utf-8")
        self.assertIn("$Version.Replace('.', '-')", script)
        self.assertIn('"OpenRDX-v$versionFileName"', script)
        self.assertIn('"$releaseBaseName.bin"', script)
        self.assertIn('"$releaseBaseName-FlashBurner.hex"', script)
        self.assertIn("build_rdx_update_container.py", script)
        self.assertIn("TUSB9261_RDX_flash.hex", script)
        self.assertIn("$manifest.container = Split-Path -Leaf $distBinary", script)
        self.assertIn(
            "$manifest.flashburner_hex = Split-Path -Leaf $distRecoveryHex",
            script,
        )
        self.assertIn("does not have the required 62,110-byte length", script)
        self.assertIn("does not match its generated manifest", script)

    def test_release_gate_runs_the_complete_test_suite(self) -> None:
        """Do not package a release after only focused protocol checks."""

        script = (ROOT / "build-dist.ps1").read_text(encoding="utf-8")
        self.assertIn(
            "'-m', 'unittest', 'discover', '-s', 'tests', '-v'", script
        )
        self.assertNotIn("test_build_rdx_update_container.py', '-v'", script)
        self.assertNotIn("test_rdx_manager_protocol.py', '-v'", script)

    def test_release_includes_the_portable_compatibility_installation_bundle(self) -> None:
        """Package the guarded updater and record its procedure digests."""

        script = (ROOT / "build-dist.ps1").read_text(encoding="utf-8")
        self.assertIn("scripts\\rdx_manager_firmware_update.ps1", script)
        self.assertIn("docs\\getting-started\\installation.md", script)
        self.assertIn("-Destination $distUpdater", script)
        self.assertIn("-Destination $distInstallationProcedure", script)
        self.assertIn("required_receiver_kind", script)
        self.assertIn("installation_updater", script)
        self.assertIn("installation_updater_sha256", script)
        self.assertIn("installation_procedure", script)
        self.assertIn("installation_procedure_sha256", script)
        self.assertIn(
            '"$($updaterHash.ToLowerInvariant()) *$(Split-Path -Leaf $distUpdater)"',
            script,
        )

    def test_release_directory_is_recreated_after_build_and_tests(self) -> None:
        """Prevent retired artifacts from surviving into a later release."""

        script = (ROOT / "build-dist.ps1").read_text(encoding="utf-8")
        self.assertIn("function Reset-DistributionDirectory", script)
        self.assertIn(
            "Remove-Item -LiteralPath $requestedDirectory -Recurse -Force",
            script,
        )
        self.assertIn(
            "$requestedDirectory.Equals(\n"
            "            $expectedDirectory, [StringComparison]::OrdinalIgnoreCase)",
            script,
        )
        self.assertLess(
            script.index("Running the complete automated firmware and release suite"),
            script.rindex("Reset-DistributionDirectory `"),
        )

    def test_release_manifest_contains_only_relocatable_artifact_paths(self) -> None:
        """Remove the development-only compiler input from release metadata."""

        script = (ROOT / "build-dist.ps1").read_text(encoding="utf-8")
        self.assertIn(
            "$manifest.PSObject.Properties.Remove('firmware_hex')", script
        )
        self.assertNotIn("$manifest.firmware_hex =", script)
        self.assertIn("$manifest.container = Split-Path -Leaf $distBinary", script)
        self.assertIn(
            "$manifest.flashburner_hex = Split-Path -Leaf $distRecoveryHex",
            script,
        )
        self.assertIn(
            '"$($installationProcedureHash.ToLowerInvariant()) '
            '*$(Split-Path -Leaf $distInstallationProcedure)"',
            script,
        )

    def test_bundled_updater_resolves_one_local_release_without_dev_paths(self) -> None:
        """Allow a copied release directory to install without checkout paths."""

        updater = UPDATER.read_text(encoding="utf-8")
        self.assertNotIn(r"D:\Projekte\OpenRDX", updater)
        self.assertIn("function Get-RdxCustomManifestPath", updater)
        self.assertIn("-Filter 'OpenRDX-v*.json'", updater)
        self.assertIn("$bundledManifests.Count -eq 1", updater)
        self.assertIn("function Get-RdxManifestContainerPath", updater)
        self.assertIn("[System.IO.Path]::GetFileName($containerName)", updater)
        self.assertIn(
            "$ImagePath = Get-RdxManifestContainerPath "
            "-CustomManifestPath $ManifestPath",
            updater,
        )

    def test_template_lookup_accepts_only_pinned_bytes_deterministically(self) -> None:
        """Permit duplicate byte-identical templates without path-specific rules."""

        script = (ROOT / "build-dist.ps1").read_text(encoding="utf-8")
        updater = UPDATER.read_text(encoding="utf-8")
        updater_lookup = updater[
            updater.index("function Find-RdxCompatibilityImage"):
            updater.index("function Get-RdxCustomManifestPath")
        ]
        for content in (script, updater_lookup):
            self.assertIn("Get-FileHash -Algorithm SHA256", content)
            self.assertIn("Sort-Object -Property FullName", content)
            self.assertIn("RDX2E__STD__F-0283.bin", content)
            self.assertNotIn("$templateCandidates.Count -ne 1", content)
            self.assertNotIn("$candidates.Count -ne 1", content)

    def test_installation_procedure_uses_the_guarded_bundle_entrypoint(self) -> None:
        """Keep release instructions aligned with the safe updater mode."""

        procedure = UPDATE_PROCEDURE.read_text(encoding="utf-8")
        self.assertIn("Get-CimInstance Win32_DiskDrive", procedure)
        self.assertIn(
            ".\\rdx_manager_firmware_update.ps1 -InstallOpenRDXOnCompatibilityReceiver `",
            procedure,
        )
        self.assertIn("-TargetSerialNumber $targetSerial", procedure)
        self.assertIn("Read-Host 'Enter the exact SerialNumber", procedure)
        self.assertIn(
            "-ValidateOnly -ValidateFirmwareKind CompatibilityReceiver", procedure
        )
        self.assertIn("physically remove the cartridge", procedure)
        self.assertIn("exact late `07/74/08` authentication result", procedure)
        self.assertIn("with **J7 open**", procedure)
        self.assertIn("at that same USB location", procedure)
        self.assertIn("manifest-pinned continuous HEX", procedure)

    def test_packaged_installation_procedure_has_no_checkout_relative_links(self) -> None:
        """Keep the copied operator guide usable outside the source checkout."""

        procedure = UPDATE_PROCEDURE.read_text(encoding="utf-8")
        local_targets = []
        for target in re.findall(r"(?<!!)\[[^\]]+\]\(([^)]+)\)", procedure):
            target = target.strip().removeprefix("<").removesuffix(">")
            path, _, _ = target.partition("#")
            if path and not re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:", target):
                local_targets.append(target)

        self.assertEqual(
            [],
            local_targets,
            "The packaged installation guide cannot resolve checkout-relative links",
        )

    def test_operator_guides_distinguish_manual_and_updater_digest_coverage(self) -> None:
        """Prevent updater checks from being presented as whole-bundle validation."""

        procedure = UPDATE_PROCEDURE.read_text(encoding="utf-8")
        release_process = RELEASE_PROCESS.read_text(encoding="utf-8")
        procedure_prose = " ".join(procedure.split())
        release_process_prose = " ".join(release_process.split())

        self.assertIn("five non-checksum bundle files", procedure_prose)
        self.assertIn(
            "manual whole-bundle checksum step remains mandatory", procedure_prose
        )
        self.assertIn("hashes the selected firmware container", procedure_prose)
        self.assertIn("hashes the continuous FlashBurner HEX", procedure_prose)
        self.assertIn(
            "does not independently hash itself or this procedure", procedure_prose
        )
        self.assertIn(
            "does not authenticate the manifest or checksum file", procedure_prose
        )
        self.assertIn(
            "manual whole-bundle checksum step remains mandatory",
            release_process_prose,
        )

        for guide in (procedure_prose, release_process_prose):
            self.assertNotIn("every release digest", guide)
            self.assertNotIn("validates every artifact", guide)
            self.assertNotRegex(
                guide,
                r"(?is)updater.{0,120}recheck.{0,120}(?:updater|procedure|manifest)",
            )


if __name__ == "__main__":
    unittest.main()
