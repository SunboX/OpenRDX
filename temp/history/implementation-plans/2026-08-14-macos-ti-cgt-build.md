# macOS TI CGT Build Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the PlatformIO wrapper select a macOS TI ARM CGT 5.2.9 installation while preserving the validated Windows TI ARM CGT 5.2.5 build.

**Architecture:** A small dependency-free Python helper resolves the toolchain root and host-native executable names. The existing SCons adapter consumes those paths while leaving all compiler, linker, and HEX arguments unchanged.

**Tech Stack:** Python standard library, `unittest`, PlatformIO Core 6.x, SCons, TI ARM CGT 5.2.x.

## Global Constraints

- Windows continues to use `C:\ti\ti-cgt-arm_5.2.5`, `armcl.exe`, and `armhex.exe` by default.
- macOS uses TI ARM CGT 5.2.9 and extensionless `armcl` and `armhex` executables.
- Do not change firmware sources, compiler flags, `ti_arm9_abi`, linker inputs, or artifact names.
- Do not fall back to GCC or TI ARM CGT 15.3 and newer.
- Treat the macOS output as an firmware build; do not claim hash identity with Windows CGT 5.2.5.
- This directory is not a Git repository, so commit steps are intentionally omitted.

---

### Task 1: Portable TI CGT tool resolution

**Files:**
- Create: `scripts/ti_cgt_tools.py`
- Create: `tests/test_ti_cgt_tools.py`
- Modify: `scripts/ti_cgt_build.py`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `custom_ti_cgt_root`, `custom_ti_cgt_root_macos`, `TI_CGT_ROOT`, and the host system name.
- Produces: `resolve_ti_cgt_tools(generic_root, macos_root=None, environ=None, host_system=None) -> TiCgtTools` with `root`, `compiler`, `hex_converter`, and `include_dir` paths.

- [ ] **Step 1: Write failing tests for Windows, macOS, and environment override selection**

```python
import unittest
from pathlib import Path

from scripts.ti_cgt_tools import resolve_ti_cgt_tools


class ResolveTiCgtToolsTests(unittest.TestCase):
    def test_windows_preserves_existing_root_and_exe_names(self):
        tools = resolve_ti_cgt_tools(
            r"C:\ti\ti-cgt-arm_5.2.5",
            macos_root="~/ti/ti-cgt-arm_5.2.9",
            environ={},
            host_system="Windows",
        )
        self.assertEqual(tools.root, Path(r"C:\ti\ti-cgt-arm_5.2.5"))
        self.assertEqual(tools.compiler.name, "armcl.exe")
        self.assertEqual(tools.hex_converter.name, "armhex.exe")

    def test_macos_uses_macos_root_and_extensionless_names(self):
        tools = resolve_ti_cgt_tools(
            r"C:\ti\ti-cgt-arm_5.2.5",
            macos_root="/opt/ti/ti-cgt-arm_5.2.9",
            environ={},
            host_system="Darwin",
        )
        self.assertEqual(tools.root, Path("/opt/ti/ti-cgt-arm_5.2.9"))
        self.assertEqual(tools.compiler.name, "armcl")
        self.assertEqual(tools.hex_converter.name, "armhex")

    def test_environment_override_has_highest_priority(self):
        tools = resolve_ti_cgt_tools(
            r"C:\ti\ti-cgt-arm_5.2.5",
            macos_root="/opt/ti/ti-cgt-arm_5.2.9",
            environ={"TI_CGT_ROOT": "/custom/ti-cgt"},
            host_system="Darwin",
        )
        self.assertEqual(tools.root, Path("/custom/ti-cgt"))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests and verify the missing helper causes RED**

Run: `/Users/afiedler/.platformio/penv/bin/python -m unittest tests/test_ti_cgt_tools.py -v`

Expected: `ModuleNotFoundError: No module named 'scripts.ti_cgt_tools'`.

- [ ] **Step 3: Implement the minimal resolver**

```python
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
```

- [ ] **Step 4: Run the resolver tests and verify GREEN**

Run: `/Users/afiedler/.platformio/penv/bin/python -m unittest tests/test_ti_cgt_tools.py -v`

Expected: three passing tests and `OK`.

- [ ] **Step 5: Connect the resolver to PlatformIO without changing build flags**

In `platformio.ini`, retain the Windows option and add:

```ini
custom_ti_cgt_root_macos = ~/ti/ti-cgt-arm_5.2.9
```

In `scripts/ti_cgt_build.py`, import the helper from the sibling scripts directory and replace the hard-coded `.exe` paths with:

```python
tools = resolve_ti_cgt_tools(
    env.GetProjectOption("custom_ti_cgt_root"),
    macos_root=env.GetProjectOption("custom_ti_cgt_root_macos", ""),
)
TI_ROOT = tools.root
ARMCL = tools.compiler
ARMHEX = tools.hex_converter
TI_INCLUDE = tools.include_dir
```

- [ ] **Step 6: Run unit tests and a missing-toolchain PlatformIO probe**

Run: `/Users/afiedler/.platformio/penv/bin/python -m unittest discover -s tests -v`

Expected: all tests pass.

Run: `/Users/afiedler/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`

Expected before installing CGT 5.2.9: failure naming `~/ti/ti-cgt-arm_5.2.9/bin/armcl`, proving macOS selection instead of the Windows path.

### Task 2: Documentation and complete macOS build

**Files:**
- Modify: `README.md`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.out`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.map`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex`

**Interfaces:**
- Consumes: the resolver and PlatformIO configuration from Task 1 plus a TI ARM CGT 5.2.9 installation at `~/ti/ti-cgt-arm_5.2.9`.
- Produces: documented Windows and macOS build commands and verified firmware artifacts.

- [ ] **Step 1: Document host-specific requirements and safety boundary**

Update `README.md` to state:

- Windows uses TI ARM CGT 5.2.5 at `C:\ti\ti-cgt-arm_5.2.5`.
- macOS requires TI ARM CGT 5.2.9 at `~/ti/ti-cgt-arm_5.2.9` and Rosetta on Apple Silicon.
- `TI_CGT_ROOT=/alternate/path` overrides either configured root.
- Both platforms use `pio run -e tusb9261_ti_cgt` with the appropriate PlatformIO executable.
- Only the Windows 5.2.5 build has the recorded reproducibility hashes.

- [ ] **Step 2: Install the official macOS TI ARM CGT 5.2.9 package**

Download `ti_cgt_tms470_5.2.9_osx_installer.app.zip` from TI after myTI authentication, then install unattended to `~/ti/ti-cgt-arm_5.2.9`. Confirm:

```bash
file ~/ti/ti-cgt-arm_5.2.9/bin/armcl
file ~/ti/ti-cgt-arm_5.2.9/bin/armhex
```

Expected: Intel Mach-O executables runnable through Rosetta.

- [ ] **Step 3: Clean and build the complete firmware on macOS**

Run: `/Users/afiedler/.platformio/penv/bin/pio run -e tusb9261_ti_cgt -t clean`

Run: `/Users/afiedler/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`

Expected: 28 C inputs, two assembly inputs, 30 TI object files, successful link, successful HEX conversion, and PlatformIO `[SUCCESS]`.

- [ ] **Step 4: Verify artifacts and Windows invariants**

Run:

```bash
find .pio/build/tusb9261_ti_cgt/ti-objects -name '*.obj' | wc -l
shasum -a 256 \
  .pio/build/tusb9261_ti_cgt/TUSB9261_RDX.out \
  .pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex
rg -n 'C:\\ti\\ti-cgt-arm_5\.2\.5|armcl\.exe|armhex\.exe|ti_arm9_abi' \
  platformio.ini scripts tests README.md
```

Expected: 30 objects, hashes recorded for the macOS build, and evidence that Windows 5.2.5 plus the legacy ABI remain preserved.

- [ ] **Step 5: Run final verification**

Run: `/Users/afiedler/.platformio/penv/bin/python -m unittest discover -s tests -v`

Run: `/Users/afiedler/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`

Expected: tests pass and the already-built PlatformIO environment reports `[SUCCESS]`.
