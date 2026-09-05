# SPDX-FileCopyrightText: 2026 André Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Host-portable TI ARM CGT path resolution."""

import os
import platform
from pathlib import Path
from typing import NamedTuple


class TiCgtTools(NamedTuple):
    root: Path
    compiler: Path
    hex_converter: Path
    include_dir: Path


def align_up(value, alignment):
    """Round a non-negative integer up to a positive power-of-two boundary."""
    if value < 0 or alignment <= 0 or (alignment & (alignment - 1)) != 0:
        raise ValueError("invalid alignment request")
    return (value + alignment - 1) & ~(alignment - 1)


def format_ti_response_file(paths, project_dir):
    """Return ASCII-safe, project-relative object paths for TI CGT."""
    base = Path(project_dir)
    return "".join(
        '"{}"\n'.format(Path(path).relative_to(base).as_posix())
        for path in paths
    )


def get_intel_hex_address_span(path):
    """Return the inclusive address span occupied by Intel HEX data records."""
    upper_address = 0
    first_address = None
    last_address = None

    for raw_line in Path(path).read_text(encoding="ascii").splitlines():
        if not raw_line.startswith(":"):
            continue
        record = bytes.fromhex(raw_line[1:])
        if len(record) < 5 or (sum(record) & 0xFF) != 0:
            raise ValueError("invalid Intel HEX record")
        byte_count = record[0]
        address = (record[1] << 8) | record[2]
        record_type = record[3]
        payload = record[4 : 4 + byte_count]
        if len(payload) != byte_count:
            raise ValueError("truncated Intel HEX record")

        if record_type == 0:
            absolute_start = upper_address + address
            absolute_end = absolute_start + byte_count - 1
            first_address = (
                absolute_start
                if first_address is None
                else min(first_address, absolute_start)
            )
            last_address = (
                absolute_end
                if last_address is None
                else max(last_address, absolute_end)
            )
        elif record_type == 4 and byte_count == 2:
            upper_address = int.from_bytes(payload, byteorder="big") << 16

    if first_address is None or last_address is None:
        raise ValueError("Intel HEX file contains no data records")
    return first_address, last_address


def resolve_ti_cgt_tools(
    generic_root, macos_root=None, environ=None, host_system=None
):
    environment = os.environ if environ is None else environ
    system = platform.system() if host_system is None else host_system
    configured_root = environment.get("TI_CGT_ROOT")
    if not configured_root:
        configured_root = macos_root if system == "Darwin" and macos_root else generic_root

    root = Path(os.path.expandvars(str(configured_root))).expanduser()
    suffix = ".exe" if system == "Windows" else ""
    return TiCgtTools(
        root=root,
        compiler=root / "bin" / ("armcl" + suffix),
        hex_converter=root / "bin" / ("armhex" + suffix),
        include_dir=root / "include",
    )
