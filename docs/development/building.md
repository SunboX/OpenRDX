# Build and test OpenRDX

This is the common maintainer guide for compiling and testing the firmware. The
build is intentionally tied to the legacy TI ARM COFF ABI required by the
TUSB9261 image.

## Supported toolchains

| Host | Required compiler route | Configured default |
| --- | --- | --- |
| Windows | TI ARM Code Generation Tools 5.2.5 | `C:\ti\ti-cgt-arm_5.2.5` |
| macOS | Windows TI ARM Code Generation Tools 5.2.9 invoked through Wine | `~/ti/ti-cgt-arm_5.2.9` |

The macOS route uses extensionless launcher scripts around the Windows tools.
Follow [Building on modern macOS](building-on-macos.md) for the verified setup.
Set `TI_CGT_ROOT` for a one-command override to another compatible installation.

> [!CAUTION]
> Do not replace this toolchain with GCC, Clang, or a newer TI compiler. The
> firmware requires `ti_arm9_abi` COFF support, which newer TI releases removed.

## Run the automated suite

From the repository root on macOS or another Unix-like shell:

```bash
~/.platformio/penv/bin/python -m unittest discover -s tests -v
```

On Windows:

```powershell
C:\Users\andre\.platformio\penv\Scripts\python.exe -X utf8 -m unittest discover -s tests -v
```

UTF-8 mode keeps temporary Unicode test fixtures consistent with the checker.
When running the suite through `build-dist.ps1`, set `$env:PYTHONUTF8 = '1'`
in that PowerShell session so its Python child process uses the same encoding.

## Build the firmware

The default PlatformIO environment is `tusb9261_ti_cgt`.

macOS:

```bash
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

Windows:

```powershell
C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
```

PlatformIO orchestrates the operation. The TI tools still compile, assemble,
link, and convert the image with warnings promoted to errors. The active build
scope is 39 C sources and two TI assembly sources, producing 41 object files.

Every successful normal build also refreshes the complete versioned `dist/`
bundle. This step runs even when the TI outputs are already up to date, repairing
missing bundle files and removing retired version filenames. A packaging error
fails the build and leaves the previous bundle intact.

Packaging requires the pinned `RDX2E__STD__F-0283.bin` compatibility template.
Set `OPENRDX_TEMPLATE_PATH` to its full path for the first build, or place it
below the sibling `OpenRDXManager/` directory. The packager checks its SHA-256
and caches verified bytes under ignored `.pio/rdx-template/`, so later builds
can use the ordinary command without setting the variable again. An explicit
template path with missing or incorrect bytes fails instead of using the cache.

## Build artifacts

A successful build places these direct outputs under
`.pio/build/tusb9261_ti_cgt/`:

| File | Role |
| --- | --- |
| `TUSB9261_RDX.out` | TI linked program output |
| `TUSB9261_RDX.map` | Link map |
| `TUSB9261_RDX.hex` | Sparse Intel HEX used as the release-container input |
| `TUSB9261_RDX_flash.hex` | Continuous Intel HEX prepared for the advanced FlashBurner route |

The same build creates these six files under `dist/`, using `VERSION` for the
versioned names:

- `OpenRDX-v<MAJOR>-<MINOR>.bin`
- `OpenRDX-v<MAJOR>-<MINOR>-FlashBurner.hex`
- `OpenRDX-v<MAJOR>-<MINOR>.json`
- `OpenRDX-v<MAJOR>-<MINOR>.sha256`
- `rdx_manager_firmware_update.ps1`
- `OPENRDX_USB_UPDATE.md`

The packager validates the version, container length, manifest, and checksums
before replacing `dist/`. For the complete automated test gate and publication
steps, follow the [release process](release-process.md). A normal `pio run`
does not run the full test suite.

The `.out` and `.map` files can contain tool-generated differences. Do not claim
cross-host byte identity without comparing the actual artifacts; use the
generated Intel HEX when firmware-byte comparison is required.

## Building is not flashing

This repository has no validated PlatformIO upload target. A successful build
does not write hardware and does not authorize anyone to flash an adapter.

Normal installation requires a complete verified release bundle and the
[guarded installation procedure](../getting-started/installation.md). Direct
FlashBurner programming is an advanced recovery operation covered separately in
[ROM-loader recovery](rom-loader-recovery.md).

[Back to the documentation index](../README.md)
