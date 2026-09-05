# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

import subprocess
import sys
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
CHECKER = PROJECT_ROOT / "scripts" / "check_license_metadata.py"


class LicenseMetadataTests(unittest.TestCase):
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
