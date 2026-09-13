# Manufacturing restore validation — 2026-09-13

This records verification of the OpenRDX 1.10 receiver integration. No firmware
was installed and no physical manufacturing data was written during this work.

## Automated checks

- The complete Python suite passed 299 tests with eight host-dependent skips
  on macOS. PowerShell-dependent checks run in the Windows release gate.
- Executable C receiver scenarios cover exact commands, malformed and stale
  data, erased/corrupt current records, occupied or busy bays, insertion races,
  neighboring-sector preservation, no-op/probe behavior, and flash failures.
- SPI register simulations cover bounded TX/RX/status waits, fault latching,
  shared bus ownership, and preserving saved interrupt enables.
- Integration regressions demonstrate protected USB receive storage despite
  ATA scratch writes, rejection of oversized/short manufacturing transfers,
  and mutual exclusion between restores, serial edits and firmware updates.
- A failed raw flash read returns SCSI failure without exposing stale response
  bytes. A separate success case verifies the exact returned sector chunk.
- Code review covered the receiver, BOT data handoff, interrupt ownership,
  SPI access and mutation interlocks; the resulting issues were corrected.

The receive-buffer, transfer-length and cross-command interlock regressions
failed before the integration changes and passed afterwards. The raw flash
failure regression likewise failed before error propagation was added.

## Local build

TI ARM CGT 5.2.9 through the existing Wine launchers compiled, linked and
converted the production image with warnings treated as errors. An ASCII-path
source snapshot avoided the toolchain's path limitation; all production source,
headers, linker files and build scripts were compared byte-for-byte with the
release checkout. The normal distribution packager then generated and validated
all six version-1.10 installation files from those outputs.

The link map reports 59,812 bytes used and 4,443 bytes free in PD_MEM. The 2 KiB
stack allocation is unchanged. The local build is not claimed byte-identical
to the Windows TI ARM CGT 5.2.5 release build. Published assets must come from
the successful workflow for the exact tagged source commit, with matching
build records, version and checksums.

## Device-validation boundary

Physical restore, manual power-cycle identity/profile activation, physical
insertion races, and interrupted NOR programming remain unverified. Successful
host simulations and the TI build do not prove physical-device behavior.
Sector replacement is not atomic across power loss and has no on-device
rollback journal. Keep the external backup and stable power throughout restore.

See [the protocol and limitations](../reference/manufacturing-restore.md) and
[the release process](release-process.md).
