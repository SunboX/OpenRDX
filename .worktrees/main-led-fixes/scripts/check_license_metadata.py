#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Validate repository license files and SPDX source headers."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


COMMENTABLE_SUFFIXES = {
    ".asm",
    ".c",
    ".cmd",
    ".h",
    ".inc",
    ".ini",
    ".ps1",
    ".py",
}
EXCLUDED_PARTS = {
    ".git",
    ".pio",
    ".superpowers",
    ".worktrees",
    "__pycache__",
}
REQUIRED_LICENSE_FILES = (
    ".reuse/dep5",
    "COMMERCIAL-LICENSE.md",
    "LICENSE",
    "LICENSES/AGPL-3.0-or-later.txt",
    "LICENSES/CC-BY-SA-4.0.txt",
    "LICENSES/LicenseRef-ThirdParty-Documentation.txt",
    "LICENSES/LicenseRef-TI-TUSB926x.txt",
    "NOTICE",
)
PROJECT_COPYRIGHT = "SPDX-FileCopyrightText: 2026 Andre Fiedler"
SPDX_LICENSE_TAG = "SPDX-License-" "Identifier:"
PROJECT_LICENSE = f"{SPDX_LICENSE_TAG} AGPL-3.0-or-later"
TI_COPYRIGHT_FRAGMENT = "Texas Instruments Incorporated"
TI_COPYRIGHT_PATTERN = re.compile(
    r"(?:SPDX-FileCopyrightText:\s*[0-9-]+\s+Texas Instruments Incorporated"
    r"|Copyright\s+[0-9-]+\s+by Texas Instruments Incorporated)"
)
TI_LICENSE = f"{SPDX_LICENSE_TAG} LicenseRef-TI-TUSB926x"
TI_MODIFIED_LICENSE = (
    f"{SPDX_LICENSE_TAG} LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later"
)
TI_MODIFIED_WITHOUT_NOTICE = {"linker/tusb9260_link.cmd"}


def repository_files(root: Path) -> list[Path]:
    """Return tracked and pending repository files without local build state."""
    if (root / ".git").exists():
        result = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-co", "--exclude-standard"],
            capture_output=True,
            text=True,
            check=True,
        )
        relative_paths = [Path(line) for line in result.stdout.splitlines() if line]
    else:
        relative_paths = [path.relative_to(root) for path in root.rglob("*")]

    files = []
    for relative_path in relative_paths:
        if any(part in EXCLUDED_PARTS for part in relative_path.parts):
            continue
        if relative_path.parts[:2] == ("docs", "superpowers"):
            continue
        path = root / relative_path
        if path.is_file():
            files.append(relative_path)
    return sorted(set(files))


def header_text(path: Path) -> str:
    """Read the bounded leading text that must carry source metadata."""
    return "\n".join(path.read_text(encoding="utf-8-sig").splitlines()[:45])


def validate_header(root: Path, relative_path: Path) -> list[str]:
    """Return licensing defects for one comment-capable source file."""
    path = root / relative_path
    if path.suffix.lower() not in COMMENTABLE_SUFFIXES:
        return []

    header = header_text(path)
    defects = []
    relative_name = relative_path.as_posix()
    is_ti_reference = relative_path.parts[:2] == ("include", "ti_reference")
    is_ti_modified = (
        not is_ti_reference
        and (
            TI_COPYRIGHT_PATTERN.search(header) is not None
            or relative_name in TI_MODIFIED_WITHOUT_NOTICE
        )
    )

    if "SPDX-FileCopyrightText:" not in header:
        defects.append(f"{relative_name}: missing SPDX copyright header")
    if SPDX_LICENSE_TAG not in header:
        defects.append(f"{relative_name}: missing SPDX license header")

    if is_ti_reference:
        if TI_COPYRIGHT_FRAGMENT not in header:
            defects.append(f"{relative_name}: missing TI SPDX copyright")
        if TI_LICENSE not in header or TI_MODIFIED_LICENSE in header:
            defects.append(f"{relative_name}: incorrect TI reference license")
        if PROJECT_COPYRIGHT in header:
            defects.append(f"{relative_name}: TI reference claims OpenRDX copyright")
    elif is_ti_modified:
        if TI_COPYRIGHT_FRAGMENT not in header:
            defects.append(f"{relative_name}: missing TI SPDX copyright")
        if PROJECT_COPYRIGHT not in header:
            defects.append(f"{relative_name}: missing OpenRDX modification copyright")
        if TI_MODIFIED_LICENSE not in header:
            defects.append(f"{relative_name}: incorrect mixed-origin license")
    else:
        if PROJECT_COPYRIGHT not in header:
            defects.append(f"{relative_name}: missing OpenRDX copyright")
        if PROJECT_LICENSE not in header:
            defects.append(f"{relative_name}: incorrect OpenRDX license")

    return defects


def validate_repository(root: Path) -> list[str]:
    """Return every missing license file or invalid source header."""
    defects = []
    for relative_name in REQUIRED_LICENSE_FILES:
        if not (root / relative_name).is_file():
            defects.append(f"{relative_name}: missing required license file")

    for relative_path in repository_files(root):
        defects.extend(validate_header(root, relative_path))
    return defects


def main(argv: list[str]) -> int:
    """Run the repository validation command."""
    root = Path(argv[1] if len(argv) > 1 else ".").resolve()
    defects = validate_repository(root)
    if defects:
        print("License metadata check failed:")
        for defect in defects:
            print(f"- {defect}")
        return 1

    print("License metadata check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
