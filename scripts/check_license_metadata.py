#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 André Fiedler
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
    "LICENSES/BSD-3-Clause.txt",
    "LICENSES/CC-BY-SA-4.0.txt",
    "LICENSES/LicenseRef-ThirdParty-Documentation.txt",
    "LICENSES/LicenseRef-TI-TUSB926x.txt",
    "NOTICE",
)
SPDX_COPYRIGHT_TAG = "SPDX-FileCopyright" "Text:"
PROJECT_COPYRIGHT = f"{SPDX_COPYRIGHT_TAG} 2026 André Fiedler"
SPDX_LICENSE_TAG = "SPDX-License-" "Identifier:"
PROJECT_LICENSE = f"{SPDX_LICENSE_TAG} AGPL-3.0-or-later"
TI_COPYRIGHT_FRAGMENT = "Texas Instruments Incorporated"
TI_COPYRIGHT_PATTERN = re.compile(
    rf"(?:{SPDX_COPYRIGHT_TAG}\s*[0-9-]+\s+Texas Instruments Incorporated"
    r"|Copyright\s+[0-9-]+\s+by Texas Instruments Incorporated)"
)
TI_LICENSE = f"{SPDX_LICENSE_TAG} LicenseRef-TI-TUSB926x"
TI_MODIFIED_LICENSE = (
    f"{SPDX_LICENSE_TAG} LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later"
)
TI_BSD_LICENSE = f"{SPDX_LICENSE_TAG} BSD-3-Clause"
TI_BSD_MODIFIED_LICENSE = f"{SPDX_LICENSE_TAG} BSD-3-Clause AND AGPL-3.0-or-later"
TI_BSD_COPYRIGHT = f"{SPDX_COPYRIGHT_TAG} 2019 Texas Instruments Incorporated"
TI_MODIFIED_WITHOUT_NOTICE = {"linker/tusb9260_link.cmd"}
TI_BSD_MODIFIED_FILES = {
    "src/exceptions_isr.asm",
    "src/intvecs.asm",
}
# These bodies match the retained TI package files; no project contribution
# is asserted merely for adding machine-readable attribution. The package's
# 1.06 manifest assigns BSD-3-Clause to source and supplies its 2019 notice.
TI_REFERENCE_FILES = {
    "include/rdx_mount/dox.h",
    "src/rdx_mount/exceptions_isr.asm",
    "src/rdx_mount/intvecs.asm",
}
# Keep established TI module attribution independent of the header being
# validated, so removing every TI notice cannot select project-only terms.
TI_HEADER_MODULES = {
    "ahci", "gio", "mww", "one_touch", "pwm", "reg_io", "rti", "sci",
    "scsi", "scsi_data", "spi", "string", "system", "tusb9260",
    "tusb9260_types", "ums_bot", "ums_uas", "usb_hal", "usb_hid",
    "usb_stack", "vim_nvic", "wdt",
}
TI_SOURCE_MODULES = {
    "ahci", "c_int00", "gio", "main", "mww", "one_touch", "pwm", "reg_io",
    "rti", "sci", "scsi", "scsi_data", "spi", "string", "system",
    "system_init", "ums_bot", "ums_uas", "usb_chap9", "usb_hal",
    "usb_hal_isr", "usb_hid", "usb_stack", "usb_vendor", "vim_intvecs",
    "vim_nvic", "wdt",
}
TI_MODIFIED_FILES = (
    {f"include/rdx_mount/{name}.h" for name in TI_HEADER_MODULES}
    | {f"src/rdx_mount/{name}.c" for name in TI_SOURCE_MODULES}
    | TI_MODIFIED_WITHOUT_NOTICE
    | TI_BSD_MODIFIED_FILES
)


def repository_files(root: Path) -> list[Path]:
    """Return tracked and pending repository files without local build state."""
    if (root / ".git").exists():
        result = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z", "-co", "--exclude-standard"],
            capture_output=True,
            text=True,
            check=True,
        )
        relative_paths = [Path(name) for name in result.stdout.split("\0") if name]
    else:
        relative_paths = [path.relative_to(root) for path in root.rglob("*")]

    files = []
    for relative_path in relative_paths:
        if not (root / ".git").exists() and any(
            part in EXCLUDED_PARTS for part in relative_path.parts
        ):
            continue
        path = root / relative_path
        if path.is_file():
            files.append(relative_path)
    return sorted(set(files))


def header_text(path: Path) -> str:
    """Read the bounded leading text that must carry source metadata."""
    return "\n".join(path.read_text(encoding="utf-8-sig").splitlines()[:45])


def header_declarations(header: str, tag: str) -> list[str]:
    """Extract complete metadata declarations from source comment lines."""
    pattern = rf"^\s*(?://|[#;*])\s*({re.escape(tag)}\s*[^\r\n]+?)\s*$"
    return re.findall(pattern, header, flags=re.MULTILINE)


def validate_header(root: Path, relative_path: Path) -> list[str]:
    """Return licensing defects for one comment-capable source file."""
    path = root / relative_path
    if path.suffix.lower() not in COMMENTABLE_SUFFIXES:
        return []

    header = header_text(path)
    copyrights = header_declarations(header, SPDX_COPYRIGHT_TAG)
    licenses = header_declarations(header, SPDX_LICENSE_TAG)
    defects = []
    relative_name = relative_path.as_posix()
    is_ti_bsd = relative_name in TI_REFERENCE_FILES | TI_BSD_MODIFIED_FILES
    is_ti_reference = (
        relative_path.parts[:2] == ("include", "ti_reference")
        or relative_name in TI_REFERENCE_FILES
    )
    is_ti_modified = (
        not is_ti_reference
        and (
            TI_COPYRIGHT_PATTERN.search(header) is not None
            or relative_name in TI_MODIFIED_FILES
        )
    )

    if not copyrights:
        defects.append(f"{relative_name}: missing SPDX copyright header")
    if not licenses:
        defects.append(f"{relative_name}: missing SPDX license header")
    if is_ti_bsd and TI_BSD_COPYRIGHT not in copyrights:
        defects.append(f"{relative_name}: missing supplied TI copyright")

    if is_ti_reference:
        if not any(TI_COPYRIGHT_FRAGMENT in value for value in copyrights):
            defects.append(f"{relative_name}: missing TI SPDX copyright")
        if licenses != [TI_BSD_LICENSE if is_ti_bsd else TI_LICENSE]:
            defects.append(f"{relative_name}: incorrect TI reference license")
        if PROJECT_COPYRIGHT in copyrights:
            defects.append(f"{relative_name}: TI reference claims OpenRDX copyright")
    elif is_ti_modified:
        if not any(TI_COPYRIGHT_FRAGMENT in value for value in copyrights):
            defects.append(f"{relative_name}: missing TI SPDX copyright")
        if PROJECT_COPYRIGHT not in copyrights:
            defects.append(f"{relative_name}: missing OpenRDX modification copyright")
        if licenses != [TI_BSD_MODIFIED_LICENSE if is_ti_bsd else TI_MODIFIED_LICENSE]:
            defects.append(f"{relative_name}: incorrect mixed-origin license")
    else:
        if PROJECT_COPYRIGHT not in copyrights:
            defects.append(f"{relative_name}: missing OpenRDX copyright")
        if licenses != [PROJECT_LICENSE]:
            defects.append(f"{relative_name}: incorrect OpenRDX license")

    if (
        (is_ti_reference or relative_name in TI_MODIFIED_FILES)
        and relative_name not in TI_MODIFIED_WITHOUT_NOTICE
        and "All rights reserved." not in header
    ):
        defects.append(f"{relative_name}: missing TI rights notice")

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
