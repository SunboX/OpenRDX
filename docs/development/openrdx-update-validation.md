# Existing OpenRDX update: evidence and validation limits

The operator procedure is
[Update existing OpenRDX](../getting-started/installation.md#update-existing-openrdx).
This record separates implementation evidence, simulated host behavior, and
observations from the connected receiver. It does not certify an untested image
or convert a build result into a successful installation.

## Source trace

Reviewed on 2026-09-05 against firmware and updater at commit `229e5a0`, with
the documentation and workflow tests added in this working tree. No firmware,
updater, toolchain, ABI, compiler flags, or linker inputs were changed.

| Step | Deciding source | Meaning for the operator |
| --- | --- | --- |
| Select operation | `scripts/rdx_manager_firmware_update.ps1` parameter sets and `$targetKind` | `-InstallOpenRDX` targets `0001` and requires `-ImagePath`; `-Update` targets `0001` but writes the pinned compatibility image. |
| Resolve inputs | `Get-RdxCustomManifestPath` and explicit `-ManifestPath` | Supplying both matching paths removes dependence on checkout defaults. |
| Validate target | `Get-RdxTarget`, `Confirm-RdxSelectedTarget`, `Assert-RdxEmptyBay` | Match model, revision, serial, PnP identity and physical port; recheck empty bay before writing. |
| Validate container | `if ($InstallOpenRDX -or $InstallOpenRDXOnCompatibilityReceiver)` | Check 62,110-byte length, manifest digest, authentication scheme, required flags and template hash. |
| Transfer | Main mode-04 loop and `New-WriteBufferCdb` | Send 16 sequential chunks; permit one full retry. The mode-02 authorization path is skipped. |
| Validate received image | `rdx_manager_handle_write_buffer` in `src/rdx_mount/rdx_manager_protocol.c` | Require sequential offsets and either the pinned compatibility-image hash or the `OPENRDX1` format and payload digest. |
| Preserve boot boundary | `rdx_program_container_payload`, `rdx_begin_update`, `rdx_program_flash` | Erase/program the application region while withholding its first four bytes; do not erase manufacturing/state sectors. |
| Activate | Mode-05 branch and `rdx_manager_protocol_tick` | Write withheld vector only after validation, then reset after five 100-ms ticks. |
| Observe return | `Wait-RdxPnpPrefix`, `Wait-RdxTarget` | Require OpenRDX USB identity and disk family at the same physical port. This is not flash readback or a release-version check. |

The compatibility route's ten-character serial requirement occurs in
`Enable-RdxCompatibilityHeaderCheckBypass`. It is not an in-place requirement.
The manifest's ROM-loader flag is validated by both branches, but only the
compatibility branch runs FlashBurner. This explains why changing manifest
flags or substituting the similarly named restore switches is incorrect.

## Automated evidence

`tests/test_openrdx_update_workflow.py` runs
`tests/openrdx_update_simulation.ps1` in a separate Windows PowerShell process.
The harness parses the actual updater, imports constant assignments and function
definitions, and executes its unchanged main block. It excludes native SPTI and
privilege initialization. Windows device queries and transport are replaced by
simulated implementations; there are no device handles or hardware writes.

The checks cover:

- sixteen exact mode-04 commands and the final mode-05 command, with buffer zero,
  correct offsets, lengths and selected device path;
- acceptance of a stale WMI media flag only when the simulated matching storage
  disk reports `No Media`;
- rejection before any command when the bay is reported occupied, the selected
  identity changes, image bytes differ from the manifest hash, or the manifest
  authentication scheme is unsupported;
- final-chunk authentication failure, the one permitted complete retry, and no
  activation after either attempt fails; and
- explicit image, manifest and serial arguments in the maintained procedure,
  together with its warning about the compatibility-image switch.

The host fixture is deliberately not a bootable firmware image. These tests do
not emulate SPI flash, validate the running device's implementation, test real
Windows SPTI, or prove behavior after power loss. The existing source-contract
and container tests cover additional firmware and artifact invariants.

Windows test command:

```powershell
C:\Users\andre\.platformio\penv\Scripts\python.exe -X utf8 -m unittest discover -s tests -v
```

UTF-8 mode is needed on this Windows setup because license tests write temporary
files containing `André Fiedler` using Python's default encoding and the checker
reads them as UTF-8. The initial default-encoding run had ten decoding errors;
all 191 pre-existing tests passed after enabling UTF-8. This changes the test
process encoding, not firmware sources or the TI build.

The final `build-dist.ps1` release check with `$env:PYTHONUTF8 = '1'` passed
all **195 tests**, including the four new workflow/documentation tests, on
2026-09-05. The local build and test log is
`.pio/openrdx-update-validation.log`; it records the TI build success and the
suite result `Ran 195 tests in 19.804s` / `OK`.

## Build and connected-device observations

The Windows TI ARM CGT 5.2.5 build completed successfully on 2026-09-05 with
warnings promoted to errors. The generated release was version `1.06`.

| Artifact | SHA-256 |
| --- | --- |
| `OpenRDX-v1-06.bin` | `411a454b47d0cae86a2b984e4ff7112d1581a2d53c67bddeafb450bc5cad78c7` |
| `OpenRDX-v1-06-FlashBurner.hex` | `5c59199c8f57260a97211abd548680f7dbe1f2160e9152308fedb4911582350d` |

The bundle was regenerated with the revised installation guide after the full
release check. All five non-checksum bundle files passed checksum verification,
and the packaged `OPENRDX_USB_UPDATE.md` matched the canonical guide's hash.
The firmware hashes above remained unchanged by this documentation work.

The connected device was observed as `TANDBERG RDX USB Device`, revision `0001`,
serial `009876543210`, then assigned `PhysicalDrive2`. Read-only updater
validation passed: WMI retained `MediaLoaded=True`, while identity-matched
Windows storage reported `No Media`, zero size and no mounted volume. No SCSI
command was sent. Disk numbers can be reassigned; this observation is not a
permanent target selection.

The maintained protocol reference reports earlier physical WRITE BUFFER,
Manager-completion, ROM-loader and re-enumeration exercises. It does not bind an
in-place transfer transcript, starting-build identity and resulting flash digest
to this connected receiver and this new container. The examined earlier
installation guide also covers receiver revision `0283`, not this `0001` case.
That initial investigation performed no transfer or activation. The subsequent
explicitly requested hardware run is recorded below.

## Hardware run on 2026-09-05

After the operator explicitly requested flashing, the guarded in-place updater
programmed the version 1.06 container identified above. Windows elevation was
used, all five bundle checksums were rechecked, and the RDX monitor was stopped
before transfer. One earlier launch stopped at the monitor-termination prompt
before sending any firmware commands; the next launch completed the update.

- Transfer ran from 19:51:37 to 19:51:55 Europe/Berlin. All sixteen mode-04
  chunks and mode-05 activation returned SCSI GOOD, with no transfer retry.
- A separate WMI query during automatic reset returned no TANDBERG RDX disk.
  The updater then found the application and disk LUN at the selected physical
  USB location and completed successfully.
- The pre-update serial was `009876543210`; the returned serial was
  `007360242889`, with revision `0001`. The present USB node was
  `USB\VID_1A5A&PID_0005\007360242889` with status `OK`.
- A fresh `-ValidateOnly -ValidateFirmwareKind OpenRDX` run, selecting the
  returned serial, passed with an empty bay and storage status `No Media`.
  No manual power cycle, J7 bridge or FlashBurner invocation was needed.

Local evidence is retained under `.pio/flash-20260905-195131/`:
`transcript.txt`, `result.json`, `post-validation.txt`, and copies of the release
manifest and checksum list. The first launch's pre-transfer failure is retained
under `.pio/flash-20260905-195057/`. These local logs are not shipped in the bundle.

This establishes a successful real transfer, activation and return for this
receiver and input container. The starting release version was not measured,
and no post-update release-version query, flash readback, cold-start test or
cartridge I/O regression was performed. It does not establish byte identity of
flash or compatibility with every earlier OpenRDX build.

## Remaining physical validation

The run above covers transfer and return. For broader hardware qualification,
retain the following evidence; starting-build identification, independent
version/readback, and media regression remain outstanding:

1. Evidence identifying the installed starting build and its support for the
   `OPENRDX1` update protocol. Family revision `0001` alone is insufficient.
2. The selected USB location, serial, empty-bay result, exact input hashes and
   complete real transfer/activation transcript.
3. Observed reset and return at that same physical location, followed by a
   fresh read-only empty-bay check. The updater currently does not require seeing
   disappearance and can accept a device that remains enumerated.
4. Independent confirmation of the running release and, for an exact-byte claim,
   a separately established readback method and matching programmed-region hash.
   The source's `SCSI_TI_GET_FW_VERSION` handler returns minor then major in two
   bytes; the current updater has no read-data probe for that command. A version
   response alone would still not prove byte identity.
5. Boot and receiver regression results appropriate to the intended media use.
   The broader limits in [Firmware behavior](../reference/firmware-behavior.md#validation-boundary)
   continue to apply after a successful firmware transfer.

The project instruction requiring a separately validated flashing safety
procedure remains in force for future operations. The recorded hardware run
adds physical transfer evidence to the source trace and simulation, within the
explicit limits above.
