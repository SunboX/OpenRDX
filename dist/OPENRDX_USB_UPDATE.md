# Install or update OpenRDX

This is the canonical installation procedure. `build-dist.ps1` copies it into
release bundles as `OPENRDX_USB_UPDATE.md`; use that packaged copy with the
matching release files when installing.

The procedure applies only to the supported Tandberg Data RDX QuikStor external
USB 3.0 receiver. Select the route by its current firmware field:

| Current receiver revision | Procedure | Updater switch |
| --- | --- | --- |
| Receiver `0283` | [Inspect, then establish record preservation](#inspect-then-install) | Compatibility installation is disabled |
| OpenRDX `0109`, `0108`, `0107`, `0106`, or legacy `0001` | [Update existing OpenRDX](#update-existing-openrdx) through its running firmware | `-InstallOpenRDX` |

The bundled updater uses Windows SPTI for existing OpenRDX receivers. It checks
the product model, revision, selected serial number, physical USB location, and
the selected firmware inputs described below. Do not use this procedure with
another product or image.

> [!WARNING]
> Firmware installation changes the receiver. Read the complete procedure before
> starting, use stable USB power, and stop whenever identity or release validation
> is ambiguous.

## Before starting

- Put one complete OpenRDX release in its own directory. Keep the versioned
  `.bin`, `-FlashBurner.hex`, `.json`, and `.sha256` files together with
  `rdx_manager_firmware_update.ps1` and this procedure. Do not mix releases.
- For first installation on `0283`, establish an installation/recovery workflow
  that backs up, restores, and verifies this receiver's manufacturing and state
  records before any staging or erasure. For an existing OpenRDX receiver, keep
  the matching continuous HEX and the repository's ROM-loader recovery procedure
  available in case recovery is needed.
- Connect the intended RDX adapter to stable USB power. The updater must select
  one unambiguous target. If more than one compatible adapter is connected, use
  its exact serial number so the intended unit is the sole match.
- Eject and physically remove the cartridge. Close RDX Manager and every other
  program that could access the device.
- Use an elevated Windows PowerShell window for the installation.

## Verify the release

From the release directory, verify every file listed in the checksum file. The
list covers the five non-checksum bundle files:

```powershell
$checksumFile = @(Get-ChildItem -File .\OpenRDX-v*.sha256)
if ($checksumFile.Count -ne 1) { throw 'Expected exactly one release checksum file.' }
foreach ($line in Get-Content -LiteralPath $checksumFile[0].FullName) {
  if ($line -notmatch '^([0-9a-fA-F]{64}) \*(.+)$') { throw "Invalid checksum line: $line" }
  $expected = $Matches[1]
  $artifact = Join-Path $checksumFile[0].DirectoryName $Matches[2]
  $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $artifact).Hash
  if (-not $actual.Equals($expected, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Checksum mismatch: $artifact"
  }
}
```

The JSON manifest records the Manager container, continuous FlashBurner HEX,
procedure, and updater. The updater reads that manifest and hashes the selected
firmware container for an existing OpenRDX receiver. Compatibility installation
is blocked before any artifact staging or device command. The updater does not
independently hash itself or this procedure, and it does not authenticate the manifest or
checksum file. The manual whole-bundle checksum step remains mandatory before
running the updater.

## Inspect, then install

This section is for receiver revision `0283` only. For OpenRDX `0109`, `0108`, `0107`, `0106`, or `0001`, follow
[Update existing OpenRDX](#update-existing-openrdx).

First list compatibility receivers without sending a SCSI command:

```powershell
$devices = @(Get-CimInstance Win32_DiskDrive | Where-Object {
  $_.Model -eq 'TANDBERG RDX USB Device' -and
  $_.FirmwareRevision -eq '0283' -and
  ([string]$_.PNPDeviceID).StartsWith(
    'USBSTOR\DISK&VEN_TANDBERG&PROD_RDX&REV_0283\',
    [StringComparison]::OrdinalIgnoreCase)
})
$devices | Select-Object Index, Model, SerialNumber, FirmwareRevision, Size, PNPDeviceID |
  Format-List
```

Identify the intended adapter's reported `SerialNumber`. Validation requires
zero disk size and no mounted cartridge
volume. If WMI retains its media-loaded flag after removal, the updater accepts
only an identity-matched Windows storage-disk result that explicitly reports
`No Media` and zero size. Run its read-only validation, supplying
the serial whenever the list contains more than one compatible adapter:

```powershell
$targetSerial = (Read-Host 'Enter the exact SerialNumber shown above').Trim()
if ([string]::IsNullOrWhiteSpace($targetSerial)) { throw 'A target serial is required.' }
.\rdx_manager_firmware_update.ps1 -ValidateOnly -ValidateFirmwareKind CompatibilityReceiver `
  -TargetSerialNumber $targetSerial
```

Validation reports the combined WMI, storage-disk, size, and volume result. Any
inconsistent state other than the documented stale WMI flag is rejected.

The bundled `-InstallOpenRDXOnCompatibilityReceiver` route and its
`-RestoreOpenRDX` alias are disabled before staging or any device command. Their
generic TI FlashBurner step does not back up or restore receiver-specific
manufacturing and state records. Programming an application HEX or receiving
a FlashBurner success message does not establish that those records survived.

Use an OpenRDX Manager installation/recovery workflow that backs up, restores, and
verifies the records for this same receiver. If that workflow is unavailable,
stop after read-only inspection. Keep J7 open during inspection; do not enter
the ROM loader or bypass the script's guard to continue first installation.

## If an earlier first installation was interrupted

Retain the complete log and any receiver-specific backup. Do not restart the
legacy installer, force mode `05h`, or program a generic application HEX as a
substitute for record restoration. Establish a recovery plan that restores and
verifies this receiver's own records before additional writes. Consult
`docs/development/rom-loader-recovery.md` in the matching source repository;
that guide is not included in the six-file bundle.

## Update existing OpenRDX

Use this route for a supported receiver already running OpenRDX revision `0109`, `0108`, `0107`,
`0106`, or the legacy marker `0001`.
It programs the receiver's firmware; it does not copy files to a cartridge.
Complete **Before starting** and **Verify the release** above first. Keep the
bay physically empty, leave J7 open, and maintain power and the same USB port
throughout transfer and automatic reset. Close RDX Manager and other device
access programs before starting.

This procedure is derived from the current updater and firmware source, with
automated host-workflow tests using simulated devices. It is not a recorded
hardware qualification of updating every earlier OpenRDX build. Revision `0109`
represents version `1.09`; legacy `0001` does not identify the installed release.
Neither revision verifies an image hash.
Confirm the installed build supports the `OPENRDX1` container protocol before
programming; discovery and empty-bay validation alone cannot establish that.
The evidence and remaining device checks are recorded in
`docs/development/openrdx-update-validation.md` in the source repository.

### Select and validate the receiver

Run in Windows PowerShell from the verified release directory:

```powershell
$devices = @(Get-CimInstance Win32_DiskDrive | Where-Object {
  $_.Model -eq 'TANDBERG RDX USB Device' -and
  $_.FirmwareRevision -cin @('0001', '0106', '0107', '0108', '0109') -and
  ([string]$_.PNPDeviceID).StartsWith(
    ('USBSTOR\DISK&VEN_TANDBERG&PROD_RDX&REV_' + $_.FirmwareRevision + '\'),
    [StringComparison]::OrdinalIgnoreCase)
})
$devices | Select-Object Index, Model, SerialNumber, FirmwareRevision, Size, PNPDeviceID |
  Format-List
$targetSerial = (Read-Host 'Enter the exact SerialNumber shown above').Trim()
if ([string]::IsNullOrWhiteSpace($targetSerial)) { throw 'A target serial is required.' }
.\rdx_manager_firmware_update.ps1 -ValidateOnly -ValidateFirmwareKind OpenRDX `
  -TargetSerialNumber $targetSerial
```

Use the complete reported serial, including leading zeros. The updater requires
exactly one matching model, revision and
serial, zero size, and no mounted cartridge volume. A stale WMI `MediaLoaded`
flag is accepted only with an identity-matched storage result reporting `No Media`
and zero size. `-ValidateOnly` sends no SCSI commands and does not validate an
image or test the installed receiver's update implementation.

### Transfer and activate

For release 1.09, run the following in an elevated Windows PowerShell session
from that release directory, with `$targetSerial` set as above. For another
release, substitute its matching `.bin` and `.json` names together.

```powershell
.\rdx_manager_firmware_update.ps1 -InstallOpenRDX `
  -ImagePath .\OpenRDX-v1-09.bin `
  -ManifestPath .\OpenRDX-v1-09.json `
  -TargetSerialNumber $targetSerial
```

`-ImagePath` is mandatory for this mode. **Do not substitute `-Update`: that
switch writes the pinned `0283` compatibility image to an OpenRDX receiver.**
`-RestoreOpenRDX` is an alias for the compatibility-installation route and also
does not select the in-place workflow.

Before the first write, the script checks the container length and SHA-256,
manifest format and template digest, then rechecks the selected PnP identity,
serial, physical USB location and empty bay. The manifest field
`installation_requires_rom_loader: true` describes compatibility installation;
the existing-OpenRDX branch still performs direct mode-05 activation. The
release field `required_receiver_kind` likewise describes first installation.
Do not edit these fields to select a different route. In-place installation
does not separately hash or program the continuous HEX; the mandatory bundle
checksum verification covers that recovery artifact.

Expect 16 sequential mode-04 WRITE BUFFER commands: fifteen 4096-byte chunks
and one 670-byte chunk, totaling 62,110 bytes. All must return SCSI GOOD.
`07/74/08` is an error on this route. The script may retry the complete transfer once from
offset zero after an error; do not launch another updater alongside it.

After successful transfer, mode `05h` at offset `0x00F29E` and length zero
commits the withheld boot vector. The firmware schedules an automatic USB
disconnect and reset. **Do not unplug the receiver during this sequence.**
Normal completion needs neither J7 nor FlashBurner nor a manual power cycle.
The running firmware erases only the boot-image sectors from `0x00000` through
`0x0FFFF`; the manufacturing and state records in high flash remain outside the
update range. Legacy TI flash-unlock and full-chip erase commands are rejected.

### Check completion and handle failures

The updater waits for USB identity `1A5A:0005` and an accepted OpenRDX disk LUN at
the selected physical USB location (up to 30 seconds for each wait). Its final
message is `Custom firmware adapter and no-media RDX disk LUN re-enumerated:`.
An activation IOCTL disconnect can produce a warning; the script then checks
enumeration. A returned device alone does not prove which firmware bytes booted.
The script does not require observing disappearance, verify a release version,
read flash back, or repeat its empty-bay assertion after reset.

Repeat the read-only selection and validation step using the serial reported
after reset. Save the transfer log, manifest, checksums and post-reset identity.
Before calling the update hardware-validated, independently confirm the running
release version and boot behavior. If the serial changes, bind the observation
to the selected physical USB location rather than assuming a reused disk number
identifies the same receiver.

If validation fails before transfer, correct the reported prerequisite without
sending a command. If transfer, authentication, activation or re-enumeration
fails, retain the complete log and stop additional writes. Do not force a
standalone mode-05 command, switch updater modes, or treat a timeout as success.
After flash erasure begins, interruption may leave the boot vector invalid and
require ROM-loader recovery. Use the matching continuous HEX and
`docs/development/rom-loader-recovery.md` from the source repository; establish
the recovery target before programming. Do not bridge J7 on a powered board.
