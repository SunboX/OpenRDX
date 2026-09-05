# Building on modern macOS

This is the detailed macOS companion to the common
[OpenRDX build and test guide](building.md). It describes the setup verified on
an Apple Silicon Mac with a current macOS release: the official **Windows** TI
ARM Code Generation Tools (CGT) 5.2.9 run through Wine.

The native TI ARM CGT 5.2.9 macOS package is not usable on modern Macs. Its
compiler programs are 32-bit `i386` Mach-O executables, while current macOS and
Rosetta 2 do not run 32-bit Mac applications. The verified route instead uses
Wine 11 to run the 32-bit Windows compiler. Rosetta 2 is installed for the
Intel-based Wine application used by this setup; it does not make the native
32-bit macOS TI tools runnable.

The project requires TI's `ti_arm9_abi`. Do not replace TI CGT with
GCC, Clang, or a newer TI compiler.

## Verified layout

The default project configuration expects this layout:

```text
~/Applications/Wine Stable.app
~/ti/ti-cgt-arm_5.2.9/
    bin/armcl             macOS launcher
    bin/armcl.exe         TI Windows compiler
    bin/armhex            macOS launcher
    bin/armhex.exe        TI Windows HEX converter
    include/              TI headers
~/ti/wine-ti-cgt-5.2.9/   dedicated Wine prefix
```

The verified versions are:

- Wine Stable 11.0
- TI ARM CGT 5.2.9 for Windows
- PlatformIO Core 6.x

The Wine application and TI installation can live elsewhere, but the launcher
constants and either `TI_CGT_ROOT` or `platformio.ini` must then be adjusted.

## 1. Install prerequisites

Install PlatformIO Core and verify the project-local command path used below:

```bash
~/.platformio/penv/bin/pio --version
```

On Apple Silicon, install Rosetta 2 if it is not already present:

```bash
softwareupdate --install-rosetta --agree-to-license
```

Install Wine Stable 11.0, which is the version tested here. A newer Wine release
may also work but has not been validated for this toolchain. The tested
application is located at `~/Applications/Wine Stable.app`. A package manager
may instead install it in `/Applications`; if so, change `WINE` in the launcher
shown below.

Verify Wine:

```bash
WINE="$HOME/Applications/Wine Stable.app/Contents/Resources/wine/bin/wine"
"$WINE" --version
```

## 2. Download and install TI ARM CGT 5.2.9

Download the official **Windows** installer
`ti_cgt_tms470_5.2.9_windows_installer.exe` from the
[TI ARM CGT 5.2.9 download page](https://www.ti.com/tool/download/ARM-CGT/5.2.9).
The macOS installer from the same page contains the incompatible 32-bit native
executables and is not used by this setup.

The installer used for the verified build had this SHA-256 digest:

```text
d79040f1d588ca01bd924a92005b4a09fd73797ede9bdb6b263ea1b1a2e34640
```

Check the downloaded installer if it has the same file name and release:

```bash
shasum -a 256 "$HOME/Downloads/ti_cgt_tms470_5.2.9_windows_installer.exe"
```

Create a dedicated Wine prefix and run the TI installer unattended:

```bash
WINE_BIN="$HOME/Applications/Wine Stable.app/Contents/Resources/wine/bin"
WINE_PREFIX="$HOME/ti/wine-ti-cgt-5.2.9"
TI_CGT_ROOT="$HOME/ti/ti-cgt-arm_5.2.9"

mkdir -p "$WINE_PREFIX" "$HOME/ti"

TI_CGT_WINDOWS_ROOT="$(
  WINEPREFIX="$WINE_PREFIX" WINEDEBUG=-all MVK_CONFIG_LOG_LEVEL=0 \
    "$WINE_BIN/winepath" -w "$TI_CGT_ROOT"
)"

WINEPREFIX="$WINE_PREFIX" WINEDEBUG=-all MVK_CONFIG_LOG_LEVEL=0 \
  "$WINE_BIN/wine" \
  "$HOME/Downloads/ti_cgt_tms470_5.2.9_windows_installer.exe" \
  --mode unattended \
  --prefix "$TI_CGT_WINDOWS_ROOT"
```

Confirm that the required Windows programs and headers were installed:

```bash
file "$TI_CGT_ROOT/bin/armcl.exe" "$TI_CGT_ROOT/bin/armhex.exe"
test -f "$TI_CGT_ROOT/include/stdint.h"
```

Both programs should be reported as `PE32` Windows executables.

## 3. Add the macOS launchers

PlatformIO deliberately resolves extensionless tool names on macOS. Add one
launcher script and copy it under both required names. The script selects
`armcl.exe` or `armhex.exe` from its own file name, invokes it through Wine,
and translates macOS absolute paths to Wine paths.

Save the following as `~/ti/ti-cgt-arm_5.2.9/bin/armcl`:

```python
#!/usr/bin/env python3
"""Run the Windows TI CGT 5.2.9 tools through Wine on macOS."""

from pathlib import Path
import os
import subprocess
import sys
import tempfile


BIN_DIR = Path(__file__).resolve().parent
TOOL = BIN_DIR / (Path(sys.argv[0]).name + ".exe")
WINE = (
    Path.home()
    / "Applications"
    / "Wine Stable.app"
    / "Contents"
    / "Resources"
    / "wine"
    / "bin"
    / "wine"
)
WINE_PREFIX = BIN_DIR.parent.parent / "wine-ti-cgt-5.2.9"
PROJECT_DRIVE = WINE_PREFIX / "dosdevices" / "p:"
PROJECT_ROOT = PROJECT_DRIVE.resolve() if PROJECT_DRIVE.exists() else None


def windows_path(value: str) -> str:
    """Translate an absolute macOS path to a Wine path."""
    if PROJECT_ROOT is not None:
        try:
            relative = Path(value).relative_to(PROJECT_ROOT)
            suffix = str(relative).replace("/", "\\")
            return "P:\\" + suffix
        except ValueError:
            pass
    return "Z:" + value.replace("/", "\\")


def translate_argument(argument: str, temporary_files: list[Path]) -> str:
    """Translate paths in a TI command-line argument or response file."""
    if argument.startswith("-@="):
        response_path = argument[3:]
        if response_path.startswith("/"):
            source = Path(response_path)
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="ascii",
                suffix=".opt",
                prefix="ti-cgt-",
                delete=False,
            ) as translated:
                for line in source.read_text(encoding="ascii").splitlines():
                    value = line.strip()
                    quoted = value.startswith('"') and value.endswith('"')
                    if quoted:
                        value = value[1:-1]
                    if value.startswith("/"):
                        value = windows_path(value)
                    translated.write(f'"{value}"\n' if quoted else f"{value}\n")
                translated_path = Path(translated.name)
            temporary_files.append(translated_path)
            return "-@=" + windows_path(str(translated_path))

    if argument.startswith("/"):
        return windows_path(argument)

    if "=" in argument:
        option, value = argument.split("=", 1)
        if value.startswith("/"):
            return option + "=" + windows_path(value)

    return argument


def main() -> int:
    """Run the selected TI program and return its exit status."""
    if not WINE.is_file():
        print(f"Wine executable not found: {WINE}", file=sys.stderr)
        return 126
    if not TOOL.is_file():
        print(f"TI CGT executable not found: {TOOL}", file=sys.stderr)
        return 127

    temporary_files: list[Path] = []
    try:
        arguments = [
            translate_argument(argument, temporary_files)
            for argument in sys.argv[1:]
        ]
        environment = os.environ.copy()
        environment["WINEPREFIX"] = str(WINE_PREFIX)
        environment.setdefault("WINEDEBUG", "-all")
        environment.setdefault("MVK_CONFIG_LOG_LEVEL", "0")
        return subprocess.run(
            [str(WINE), str(TOOL), *arguments],
            env=environment,
            check=False,
        ).returncode
    finally:
        for temporary_file in temporary_files:
            temporary_file.unlink(missing_ok=True)


if __name__ == "__main__":
    raise SystemExit(main())
```

Copy it to the second tool name and make both launchers executable:

```bash
cp "$HOME/ti/ti-cgt-arm_5.2.9/bin/armcl" \
  "$HOME/ti/ti-cgt-arm_5.2.9/bin/armhex"
chmod +x \
  "$HOME/ti/ti-cgt-arm_5.2.9/bin/armcl" \
  "$HOME/ti/ti-cgt-arm_5.2.9/bin/armhex"
```

Python 3.9 or newer is required by the launchers.

## 4. Map the project to an ASCII Wine drive

The old compiler can crash when the Windows path contains the decomposed accent
in the `Andrés_Werkstatt` directory name. Map the checkout to Wine drive `P:`
so the launchers can pass short, ASCII-only project paths to the compiler.

Run this command from the project root:

```bash
WINE_PREFIX="$HOME/ti/wine-ti-cgt-5.2.9"

if [ ! -e "$WINE_PREFIX/dosdevices/p:" ]; then
  ln -s "$PWD" "$WINE_PREFIX/dosdevices/p:"
fi

readlink "$WINE_PREFIX/dosdevices/p:"
```

The final command must print the current project root. If the checkout is moved,
delete only this `p:` symlink after inspecting it, then recreate it from the new
project root.

The repository also writes `.pio/...` object names to the TI linker response
file instead of absolute Unicode paths. Keep that behavior in
`scripts/ti_cgt_tools.py`.

## 5. Verify the compiler and build

Verify that the launcher reaches the expected compiler and that the required ABI
is accepted:

```bash
"$HOME/ti/ti-cgt-arm_5.2.9/bin/armcl" -version
"$HOME/ti/ti-cgt-arm_5.2.9/bin/armcl" --abi=ti_arm9_abi --help >/dev/null
```

Run the tests, clean the previous output, and build:

```bash
~/.platformio/penv/bin/python -m unittest discover -s tests -v
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt -t clean
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

A successful clean build compiles 37 C sources and two TI assembly sources,
creating 39 object files and these direct artifacts:

```text
.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.out
.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.map
.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex
.pio/build/tusb9261_ti_cgt/TUSB9261_RDX_flash.hex
```

The sparse HEX is the release-container input. The continuous `_flash.hex`
image is generated for the advanced FlashBurner route. Update containers and
release manifests are created separately by the
[release process](release-process.md), not by the PlatformIO build adapter.

The project has no validated upload target. Building does not authorize
flashing these artifacts.

## Alternate installation paths

The configured macOS root is `~/ti/ti-cgt-arm_5.2.9`. Override it for one build
without editing the project:

```bash
TI_CGT_ROOT=/alternate/path/to/ti-cgt-arm_5.2.9 \
  ~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

An alternate root still needs extensionless `bin/armcl` and `bin/armhex`
launchers. If Wine or its prefix is elsewhere, update `WINE` and `WINE_PREFIX`
inside both launchers.

## Windows build

The macOS setup does not modify the Windows compiler route:

- Windows uses `C:\ti\ti-cgt-arm_5.2.5`.
- Windows invokes `armcl.exe` and `armhex.exe` directly.
- macOS uses `~/ti/ti-cgt-arm_5.2.9` and the extensionless Wine launchers.
- `TI_CGT_ROOT` remains an explicit override on either host.

The two compiler versions may produce different output bytes. Do not treat a
successful macOS 5.2.9 build as byte-identical to the validated Windows 5.2.5
build without comparing the actual artifacts.

## Troubleshooting

### `bad CPU type in executable`

The native macOS TI tool was selected. It is a 32-bit `i386` executable and
cannot run on current macOS. Install the Windows package and use the
extensionless Wine launchers.

### `Wine executable not found`

Change the launcher's `WINE` constant to the actual Wine binary. For a typical
system-wide installation, it may be under `/Applications/Wine Stable.app`.

### `TI ARM compiler not found`

PlatformIO prints the resolved path. Confirm that `bin/armcl` exists and is
executable, or set `TI_CGT_ROOT` to the correct wrapper-enabled installation.

### Compiler exception while opening a source or object file

Verify that `~/ti/wine-ti-cgt-5.2.9/dosdevices/p:` points to the current
checkout. The compiler must see the project through this ASCII drive mapping,
not only through Wine's `Z:` drive and the accented macOS path.

### Linker response-file encoding error

Run the current repository version and its unit tests. The response file must
contain ASCII project-relative paths; absolute paths under `Andrés_Werkstatt`
are not safe for this compiler release.

[Back to the common build guide](building.md) ·
[Documentation index](../README.md)
