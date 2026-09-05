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


def format_ti_response_file(paths, project_dir):
    """Return ASCII-safe, project-relative object paths for TI CGT."""
    base = Path(project_dir)
    return "".join(
        '"{}"\n'.format(Path(path).relative_to(base))
        for path in paths
    )


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
