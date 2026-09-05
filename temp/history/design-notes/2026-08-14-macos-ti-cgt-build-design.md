# macOS TI CGT build design

## Goal

Make the PlatformIO wrapper build on macOS without changing the existing
Windows TI ARM CGT 5.2.5 build, firmware sources, compiler flags, ABI, linker
inputs, or artifact names.

## Constraints

- Windows continues to use `C:\ti\ti-cgt-arm_5.2.5` by default and invokes
  `armcl.exe` and `armhex.exe`.
- macOS uses TI ARM CGT 5.2.9, the oldest macOS release that retains the
  required legacy `ti_arm9_abi` support.
- TI ARM CGT 15.3 and newer are invalid for this firmware because they removed
  the required COFF ABI.
- A macOS build is an firmware build and is not expected to match the artifact
  hashes produced by the validated Windows 5.2.5 build.

## Design

Extract toolchain selection into a small Python helper that accepts the host
system, configured roots, and environment. The resolution order is:

1. `TI_CGT_ROOT`, when set.
2. `custom_ti_cgt_root_macos` on macOS.
3. The existing `custom_ti_cgt_root` on all other hosts.

The helper selects `.exe` tool names only on Windows and extensionless tool
names elsewhere. It expands `~` and environment variables in the selected
root. The PlatformIO adapter consumes the resolved root, compiler, converter,
and include directory without changing any build commands or flags.

`platformio.ini` retains the current Windows option and adds a macOS option
whose default is `~/ti/ti-cgt-arm_5.2.9`.

## Error handling

If the selected compiler or HEX converter does not exist, the build fails
before compilation and reports the resolved path. No fallback to GCC or a
newer incompatible TI compiler is allowed.

## Testing and verification

- Add standard-library unit tests covering Windows defaults, macOS defaults,
  and the `TI_CGT_ROOT` override.
- Observe the tests fail before implementing the helper.
- Run all unit tests after implementation.
- Run a complete PlatformIO macOS build with TI ARM CGT 5.2.9.
- Verify that the default Windows root, `.exe` names, build flags, linker
  inputs, and output names remain present.
- Record macOS artifact sizes and SHA-256 hashes without claiming equivalence
  to the validated Windows 5.2.5 artifacts.
