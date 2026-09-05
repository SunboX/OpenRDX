# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
CHECKER = PROJECT_ROOT / "scripts" / "check_license_metadata.py"
CHECKER_SPEC = importlib.util.spec_from_file_location("license_checker", CHECKER)
assert CHECKER_SPEC is not None and CHECKER_SPEC.loader is not None
license_checker = importlib.util.module_from_spec(CHECKER_SPEC)
CHECKER_SPEC.loader.exec_module(license_checker)
COPYRIGHT_TAG = "SPDX-FileCopyright" "Text:"
LICENSE_TAG = "SPDX-License-" "Identifier:"


class LicenseMetadataTests(unittest.TestCase):
    """Keep source attribution and license declarations independently checked."""

    def test_ti_package_files_accept_the_supplied_bsd_source_license(self):
        """Use the package's source grant for the five verified matching files."""
        expected_licenses = {
            "src/rdx_mount/exceptions_isr.asm": "BSD-3-Clause",
            "src/rdx_mount/intvecs.asm": "BSD-3-Clause",
            "include/rdx_mount/dox.h": "BSD-3-Clause",
            "src/exceptions_isr.asm": "BSD-3-Clause AND AGPL-3.0-or-later",
            "src/intvecs.asm": "BSD-3-Clause AND AGPL-3.0-or-later",
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name, license_expression in expected_licenses.items():
                with self.subTest(path=name):
                    path = root / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    marker = ";" if path.suffix == ".asm" else "//"
                    header = (
                        f"{marker} {COPYRIGHT_TAG} 2019 Texas Instruments Incorporated\n"
                        f"{marker} All rights reserved.\n"
                        f"{marker} {LICENSE_TAG} {license_expression}\n"
                    )
                    if " AND " in license_expression:
                        header += f"{marker} {COPYRIGHT_TAG} 2026 André Fiedler\n"
                    path.write_text(header)

                    defects = license_checker.validate_header(root, Path(name))

                    self.assertEqual([], defects)

    def test_ti_package_source_notice_is_required(self):
        """Require the full supplied BSD conditions alongside source headers."""
        with tempfile.TemporaryDirectory() as directory:
            defects = license_checker.validate_repository(Path(directory))

        self.assertIn(
            "LICENSES/BSD-3-Clause.txt: missing required license file", defects
        )

    def test_ti_package_files_retain_the_supplied_copyright_year(self):
        """Reject substituting inferred dates for the source manifest notice."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            name = "src/rdx_mount/intvecs.asm"
            path = root / name
            path.parent.mkdir(parents=True)
            path.write_text(
                f"; {COPYRIGHT_TAG} 2009-2013 Texas Instruments Incorporated\n"
                f"; {LICENSE_TAG} BSD-3-Clause\n"
                "; All rights reserved.\n"
            )

            defects = license_checker.validate_header(root, Path(name))

            self.assertIn(f"{name}: missing supplied TI copyright", defects)

    def test_ti_package_files_cannot_claim_project_only_ownership(self):
        """Preserve TI attribution on package assembly and specification files."""
        expected_defects = {
            "src/rdx_mount/exceptions_isr.asm": "incorrect TI reference license",
            "src/rdx_mount/intvecs.asm": "incorrect TI reference license",
            "include/rdx_mount/dox.h": "incorrect TI reference license",
            "src/exceptions_isr.asm": "incorrect mixed-origin license",
            "src/intvecs.asm": "incorrect mixed-origin license",
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name, defect in expected_defects.items():
                with self.subTest(path=name):
                    path = root / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    marker = ";" if path.suffix == ".asm" else "//"
                    path.write_text(
                        f"{marker} {COPYRIGHT_TAG} 2026 André Fiedler\n"
                        f"{marker} {LICENSE_TAG} AGPL-3.0-or-later\n"
                    )

                    defects = license_checker.validate_header(root, Path(name))

                    self.assertIn(f"{name}: missing TI SPDX copyright", defects)
                    self.assertIn(f"{name}: {defect}", defects)

    def test_ti_support_file_cannot_drop_all_ti_notices(self):
        """Reject AGPL-only headers for an established TI support module."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            relative_path = Path("src/rdx_mount/ahci.c")
            path = root / relative_path
            path.parent.mkdir(parents=True)
            path.write_text(
                f"/*\n * {COPYRIGHT_TAG} 2026 André Fiedler\n"
                f" * {LICENSE_TAG} AGPL-3.0-or-later\n */\n"
            )

            defects = license_checker.validate_header(root, relative_path)

            self.assertIn(
                "src/rdx_mount/ahci.c: missing TI SPDX copyright", defects
            )
            self.assertIn(
                "src/rdx_mount/ahci.c: incorrect mixed-origin license", defects
            )

    def test_license_expression_must_match_the_applicable_terms(self):
        """Reject an extra alternative license appended to the project grant."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "example.py"
            path.write_text(
                f"# {COPYRIGHT_TAG} 2026 André Fiedler\n"
                f"# {LICENSE_TAG} AGPL-3.0-or-later OR MIT\n"
            )

            defects = license_checker.validate_header(root, Path("example.py"))

            self.assertIn("example.py: incorrect OpenRDX license", defects)

    def test_ti_support_file_retains_the_supplied_rights_notice(self):
        """Keep the TI rights notice alongside the machine-readable metadata."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            relative_path = Path("src/rdx_mount/ahci.c")
            path = root / relative_path
            path.parent.mkdir(parents=True)
            path.write_text(
                f"/*\n * {COPYRIGHT_TAG} 2009 Texas Instruments Incorporated\n"
                f" * {COPYRIGHT_TAG} 2026 André Fiedler\n"
                f" * {LICENSE_TAG} LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later\n */\n"
            )

            defects = license_checker.validate_header(root, relative_path)

            self.assertIn("src/rdx_mount/ahci.c: missing TI rights notice", defects)

    def test_repository_license_metadata_is_complete(self):
        """Reject missing or misleading license metadata in tracked files."""
        result = subprocess.run(
            [sys.executable, str(CHECKER), str(PROJECT_ROOT)],
            capture_output=True,
            text=True,
            check=False,
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("License metadata check passed", result.stdout)


if __name__ == "__main__":
    unittest.main()
