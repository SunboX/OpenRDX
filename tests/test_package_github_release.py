# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Exercise release packaging with real temporary files and corrupt inputs."""

import hashlib
import importlib.util
import json
import os
import tempfile
import unittest
import zipfile
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/package_github_release.py"
VERSION = "1.06"
BASE = "OpenRDX-v1-06"
COMMIT = "0123456789abcdef0123456789abcdef01234567"
RAW_FILES = (
    "TUSB9261_RDX.out", "TUSB9261_RDX.map", "TUSB9261_RDX.hex",
    "TUSB9261_RDX_flash.hex",
)


def digest(data: bytes) -> str:
    """Return a digest for independent fixture and output verification."""

    return hashlib.sha256(data).hexdigest()


class GithubReleasePackageTests(unittest.TestCase):
    """Keep release assets complete, portable, and tied to validated inputs."""

    def setUp(self) -> None:
        """Create a minimal complete repository and a valid six-file bundle."""

        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.dist = self.root / "dist"
        self.build = self.root / "build"
        self.output = self.root / "release"
        self.dist.mkdir()
        self.build.mkdir()
        for name in ("LICENSE", "NOTICE", "COMMERCIAL-LICENSE.md", "README.md"):
            (self.root / name).write_text(f"{name}: André Fiedler\n", encoding="utf-8")
        (self.root / "LICENSES").mkdir()
        (self.root / "LICENSES/AGPL-3.0-or-later.txt").write_text("license\n")
        (self.root / "VERSION").write_text(VERSION + "\n")
        header = self.root / "include/rdx_mount/tusb9260.h"
        header.parent.mkdir(parents=True)
        header.write_text("#define FIRMWARE_MAJOR_VERSION 1\n#define FIRMWARE_MINOR_VERSION 06\n")
        for name, data in {
            "docs/README.md": b"[Recovery](development/rom-loader-recovery.md)\n",
            "docs/getting-started/installation.md": b"Installation\n",
            "docs/development/rom-loader-recovery.md": b"![J7](../assets/rdx-board-j7-location.jpg)\n",
            "docs/assets/rdx-board-j7-location.jpg": b"fixture image one",
            "docs/assets/rdx-board-j7-close-up.jpg": b"fixture image two",
        }.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        for name in RAW_FILES:
            (self.build / name).write_bytes((name + "\n").encode())
        self.payloads = {
            BASE + ".bin": b"B" * 62110,
            BASE + "-FlashBurner.hex": (self.build / RAW_FILES[-1]).read_bytes(),
            "rdx_manager_firmware_update.ps1": b"Write-Host 'updater'\n",
            "OPENRDX_USB_UPDATE.md": b"Installation\n",
        }
        self.manifest = {
            "release_version": VERSION,
            "release_name": BASE,
            "container": BASE + ".bin",
            "container_length": 62110,
            "flashburner_hex": BASE + "-FlashBurner.hex",
            "installation_updater": "rdx_manager_firmware_update.ps1",
            "installation_procedure": "OPENRDX_USB_UPDATE.md",
        }
        self.write_bundle()

    def write_bundle(self) -> None:
        """Refresh all valid bundle hashes after an intentional fixture change."""

        for field in ("container", "flashburner_hex", "installation_updater", "installation_procedure"):
            name = self.manifest[field]
            self.manifest[field + "_sha256"] = digest(self.payloads[name])
        for name, data in self.payloads.items():
            (self.dist / name).write_bytes(data)
        self.write_manifest_and_checksums()

    def write_manifest_and_checksums(self) -> None:
        """Recompute the checksum listing without normalizing manifest fields."""

        # Windows PowerShell writes a UTF-8 BOM in some supported versions.
        (self.dist / (BASE + ".json")).write_text(
            json.dumps(self.manifest, indent=2), encoding="utf-8-sig"
        )
        names = list(self.payloads) + [BASE + ".json"]
        (self.dist / (BASE + ".sha256")).write_text("".join(
            f"{digest((self.dist / name).read_bytes())} *{name}\n" for name in names
        ), encoding="ascii")

    def package(self, **overrides) -> dict:
        """Load and invoke the actual packager against the temporary inputs."""

        self.assertTrue(SCRIPT.is_file(), "The release packager must exist")
        spec = importlib.util.spec_from_file_location("package_github_release", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        arguments = dict(
            root=self.root, dist=self.dist, build_dir=self.build,
            output_dir=self.output, version=VERSION, commit=COMMIT,
            repository="SunboX/OpenRDX",
        )
        arguments.update(overrides)
        return module.package_release(**arguments)

    def assert_rejected_without_output(self, **overrides) -> None:
        """Require a clear validation error and no partial publication assets."""

        with self.assertRaises(ValueError):
            self.package(**overrides)
        self.assertFalse(self.output.exists())

    def test_archives_and_direct_assets_include_integrity_and_source_information(self) -> None:
        """A valid package exposes install files, recovery docs, licenses, and builds."""

        self.package()
        bundle_names = set(self.payloads) | {BASE + ".json", BASE + ".sha256"}
        expected_output = bundle_names | {BASE + ".zip", BASE + "-build.zip", "SHA256SUMS"}
        self.assertEqual({path.name for path in self.output.iterdir()}, expected_output)
        for name in bundle_names:
            self.assertEqual((self.output / name).read_bytes(), (self.dist / name).read_bytes())
        legal_names = {"LICENSE", "NOTICE", "COMMERCIAL-LICENSE.md", "LICENSES/AGPL-3.0-or-later.txt"}
        for name, expected in (
            (BASE + ".zip", bundle_names | legal_names | {
                "README.md", "VERSION", "BUILD-INFO.json", "docs/README.md",
                "docs/getting-started/installation.md", "docs/development/rom-loader-recovery.md",
                "docs/assets/rdx-board-j7-location.jpg", "docs/assets/rdx-board-j7-close-up.jpg",
            }),
            (BASE + "-build.zip", set(RAW_FILES) | legal_names | {"BUILD-INFO.json"}),
        ):
            with zipfile.ZipFile(self.output / name) as archive:
                self.assertEqual(set(archive.namelist()), expected)
                self.assertIsNone(archive.testzip())
                info = json.loads(archive.read("BUILD-INFO.json"))
                self.assertEqual(info["release_version"], VERSION)
                self.assertEqual(info["source_commit"], COMMIT)
                self.assertEqual(info["source_commit_url"], f"https://github.com/SunboX/OpenRDX/commit/{COMMIT}")
                self.assertEqual(info["source_archive_url"], f"https://github.com/SunboX/OpenRDX/archive/{COMMIT}.zip")
                self.assertEqual(info["compiler"]["version"], "5.2.5")
                self.assertEqual(info["compiler"]["host"], "Windows")
                self.assertEqual(info["build_artifacts_sha256"][RAW_FILES[0]], digest((self.build / RAW_FILES[0]).read_bytes()))
        checksums = (self.output / "SHA256SUMS").read_text().splitlines()
        self.assertEqual(len(checksums), 8)
        self.assertEqual({line.split(" *")[1] for line in checksums}, expected_output - {"SHA256SUMS"})
        for line in checksums:
            expected_digest, name = line.split(" *")
            self.assertEqual(expected_digest, digest((self.output / name).read_bytes()))

    def test_tampered_bundle_member_is_rejected(self) -> None:
        """Byte corruption must never reach output, even with the right size."""

        (self.dist / (BASE + ".bin")).write_bytes(b"X" * 62110)
        self.assert_rejected_without_output()

    def test_manifest_hash_mismatch_is_rejected_even_with_valid_checksum_listing(self) -> None:
        """The independent manifest hash contract is enforced for every payload."""

        for field in ("container", "flashburner_hex", "installation_updater", "installation_procedure"):
            with self.subTest(field=field):
                baseline = self.manifest[field + "_sha256"]
                self.manifest[field + "_sha256"] = "0" * 64
                self.write_manifest_and_checksums()
                self.assert_rejected_without_output()
                self.manifest[field + "_sha256"] = baseline

    def test_missing_bundle_build_license_or_recovery_asset_is_rejected(self) -> None:
        """Never silently omit a declared release prerequisite."""

        for path in (
            self.dist / (BASE + ".sha256"), self.dist / "rdx_manager_firmware_update.ps1",
            self.build / RAW_FILES[0], self.root / "NOTICE",
            self.root / "docs/assets/rdx-board-j7-location.jpg",
        ):
            with self.subTest(path=path):
                data = path.read_bytes()
                path.unlink()
                self.assert_rejected_without_output()
                path.write_bytes(data)

    def test_unsafe_manifest_paths_are_rejected(self) -> None:
        """Manifest-selected paths cannot escape or rename fixed release assets."""

        for value in ("../outside.bin", "/tmp/outside.bin", "C:\\outside.bin", "sub\\file.bin"):
            with self.subTest(value=value):
                self.manifest["container"] = value
                self.write_manifest_and_checksums()
                self.assert_rejected_without_output()

    def test_checksum_listing_requires_exactly_the_five_expected_members(self) -> None:
        """Missing, duplicate, malformed, and traversal entries fail closed."""

        path = self.dist / (BASE + ".sha256")
        baseline = path.read_text()
        for changed in (
            "\n".join(baseline.splitlines()[:-1]),
            baseline + baseline.splitlines()[0] + "\n",
            baseline + "0" * 64 + " *../outside.bin\n",
            baseline.replace(" *", " ?", 1),
            baseline.replace(baseline[:64], "0" * 64, 1),
        ):
            with self.subTest(changed=changed[:80]):
                path.write_text(changed)
                self.assert_rejected_without_output()

    def test_valid_hashes_cannot_hide_wrong_container_length(self) -> None:
        """The receiver's fixed container size remains a packaging prerequisite."""

        self.payloads[BASE + ".bin"] = b"short"
        self.manifest["container_length"] = 5
        self.write_bundle()
        self.assert_rejected_without_output()

    def test_release_version_must_agree_with_source_and_manifest(self) -> None:
        """Labels cannot claim a different compiled version or manifest release."""

        self.assert_rejected_without_output(version="1.07")
        self.manifest["release_version"] = "1.07"
        self.write_manifest_and_checksums()
        self.assert_rejected_without_output()
        self.manifest["release_version"] = VERSION
        self.write_manifest_and_checksums()
        (self.root / "VERSION").write_text("1.07\n")
        self.assert_rejected_without_output()
        (self.root / "VERSION").write_text(VERSION + "\n")
        (self.root / "include/rdx_mount/tusb9260.h").write_text(
            "#define FIRMWARE_MAJOR_VERSION 1\n#define FIRMWARE_MINOR_VERSION 07\n"
        )
        self.assert_rejected_without_output()

    def test_raw_continuous_hex_must_match_installation_artifact(self) -> None:
        """Prevent build and installer archives from containing different images."""

        (self.build / RAW_FILES[-1]).write_bytes(b"different release\n")
        self.assert_rejected_without_output()

    def test_archives_are_reproducible_despite_input_timestamps(self) -> None:
        """ZIP metadata and provenance must not encode build time or local paths."""

        self.package()
        baseline = {path.name: path.read_bytes() for path in self.output.iterdir()}
        for path in self.root.rglob("*"):
            if path.is_file():
                os.utime(path, (1700000000, 1700000000))
        second = self.root / "second-output"
        self.package(output_dir=second)
        self.assertEqual(baseline, {path.name: path.read_bytes() for path in second.iterdir()})

    def test_source_identifiers_cannot_inject_unrelated_urls(self) -> None:
        """Provenance accepts an immutable SHA and GitHub repository identity."""

        self.assert_rejected_without_output(commit="main")
        self.assert_rejected_without_output(repository="SunboX/OpenRDX/../../other")

    def test_local_planning_documents_are_not_distributed(self) -> None:
        """Exclude the ignored docs/superpowers tree from manual release runs."""

        local_plan = self.root / "docs/superpowers/plans/local.md"
        local_plan.parent.mkdir(parents=True)
        local_plan.write_text("Local planning only\n")
        self.package()
        with zipfile.ZipFile(self.output / (BASE + ".zip")) as archive:
            self.assertFalse(any(name.startswith("docs/superpowers/") for name in archive.namelist()))


if __name__ == "__main__":
    unittest.main()
