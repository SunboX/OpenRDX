# Install or update OpenRDX

This is the canonical installation procedure. `build-dist.ps1` copies it into
release bundles as `OPENRDX_USB_UPDATE.md`; use that packaged copy with the
matching release files when installing.

The procedure applies only to the supported Tandberg Data RDX QuikStor external
USB 3.0 receiver. Select the route by its current firmware field:

| Current receiver revision | Procedure | Updater switch |
| --- | --- | --- |
| Vendor `0283` | [First installation](#inspect-then-install) through the ROM loader | `-InstallOpenRDXOnCompatibilityReceiver` |
| OpenRDX `0001` | [Update existing OpenRDX](#update-existing-openrdx) through its running firmware | `-InstallOpenRDX` |

Both routes use Windows SPTI. First installation also uses TI Command Line
FlashBurner. The updater checks
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
- For first installation on `0283`, install TI Command Line FlashBurner in its
  normal location, or pass its exact executable path with `-FlashBurnerPath`.
  For an existing OpenRDX receiver, keep the matching continuous HEX and the
  repository's ROM-loader recovery procedure available in case recovery is needed.
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
firmware container. For compatibility installation, it hashes the continuous
FlashBurner HEX against the manifest value. It does not independently
hash itself or this procedure, and it does not authenticate the manifest or
checksum file. The manual whole-bundle checksum step remains mandatory before
running the updater.

## Inspect, then install

This section is for vendor revision `0283` only. For revision `0001`, follow
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

Identify the intended adapter's reported `SerialNumber`. An installation requires
a ten-character printable ASCII serial, zero disk size, and no mounted cartridge
volume. If WMI retains its media-loaded flag after removal, the updater accepts
only an identity-matched Windows storage-disk result that explicitly reports
`No Media` and zero size. The updater repeats the identity and empty-bay checks
immediately before WRITE BUFFER. Run its read-only validation first, supplying
the serial whenever the list contains more than one compatible adapter:

```powershell
$targetSerial = (Read-Host 'Enter the exact SerialNumber shown above').Trim()
if ($targetSerial -notmatch '^[\x20-\x7E]{10}$') {
  throw 'The selected serial must contain exactly ten printable ASCII characters.'
}
.\rdx_manager_firmware_update.ps1 -ValidateOnly -ValidateFirmwareKind CompatibilityReceiver `
  -TargetSerialNumber $targetSerial
```

Validation reports the combined WMI, storage-disk, size, and volume result. Any
inconsistent state other than the documented stale WMI flag is rejected.

Start the guarded installation from the same directory:

```powershell
.\rdx_manager_firmware_update.ps1 -InstallOpenRDXOnCompatibilityReceiver `
  -TargetSerialNumber $targetSerial
```

When only one compatible adapter is connected, `-TargetSerialNumber` is optional.

With exactly one release manifest beside it, the updater resolves the matching
container automatically. It validates target identity and state, reads the
manifest, hashes the selected firmware container, authorizes the selected
compatibility receiver from that adapter's serial, stages the OpenRDX container,
and requires the exact late `07/74/08` authentication result. Any other status
aborts.

The script then leads the operator through two distinct power cycles:

1. When prompted after staging, disconnect and reconnect USB **without bridging
   J7**. The adapter must enumerate as the single `0451:926B` ROM loader in the
   system and at the same physical USB location selected before transfer.
2. The updater programs only the manifest-pinned continuous HEX through that ROM
   loader. After FlashBurner returns success, disconnect and reconnect once more
   with **J7 open** so the programmed OpenRDX application can boot.

Do not disconnect during WRITE BUFFER transfer or FlashBurner programming.
Accept only the power cycles requested by the script. After the second
reconnection, the updater must find the OpenRDX revision `0001` empty-bay device
at that same USB location before reporting success. Afterwards,
`.\rdx_manager_firmware_update.ps1 -ValidateOnly` performs a read-only check of
the installed OpenRDX identity.

## If first installation is interrupted

- Before the expected `07/74/08` result, stop and diagnose the reported error;
  do not force activation or substitute another image.
- After that result, do not send mode `05h` or restart installation against the
  staged state. Perform the requested USB power cycle and continue only with
  the manifest-pinned HEX.
- If exactly one ROM loader does not appear, or FlashBurner fails, leave the
  adapter in its recoverable state and follow the advanced recovery guide at
  `docs/development/rom-loader-recovery.md` in the matching source repository.
  That guide is not included in the six-file bundle. Do not guess at a device
  instance or HEX file.

## Update existing OpenRDX

Use this route for a supported receiver already running OpenRDX revision `0001`.
It programs the receiver's firmware; it does not copy files to a cartridge.
Complete **Before starting** and **Verify the release** above first. Keep the
bay physically empty, leave J7 open, and maintain power and the same USB port
throughout transfer and automatic reset. Close RDX Manager and other device
access programs before starting.

This procedure is derived from the current updater and firmware source, with
automated host-workflow tests using simulated devices. It is not a recorded
hardware qualification of updating every earlier OpenRDX build. Revision `0001`
identifies the firmware family, not the installed release version or image hash.
Confirm the installed build supports the `OPENRDX1` container protocol before
programming; discovery and empty-bay validation alone cannot establish that.
The evidence and remaining device checks are recorded in
`docs/development/openrdx-update-validation.md` in the source repository.

### Select and validate the receiver

Run in Windows PowerShell from the verified release directory:

```powershell
$devices = @(Get-CimInstance Win32_DiskDrive | Where-Object {
  $_.Model -eq 'TANDBERG RDX USB Device' -and
  $_.FirmwareRevision -eq '0001' -and
  ([string]$_.PNPDeviceID).StartsWith(
    'USBSTOR\DISK&VEN_TANDBERG&PROD_RDX&REV_0001\',
    [StringComparison]::OrdinalIgnoreCase)
})
$devices | Select-Object Index, Model, SerialNumber, FirmwareRevision, Size, PNPDeviceID |
  Format-List
$targetSerial = (Read-Host 'Enter the exact SerialNumber shown above').Trim()
if ([string]::IsNullOrWhiteSpace($targetSerial)) { throw 'A target serial is required.' }
.\rdx_manager_firmware_update.ps1 -ValidateOnly -ValidateFirmwareKind OpenRDX `
  -TargetSerialNumber $targetSerial
```

Use the complete reported serial, including leading zeros. The ten-character
serial requirement for the vendor authorization step does not apply to this
route. The updater still requires exactly one matching model, revision and
serial, zero size, and no mounted cartridge volume. A stale WMI `MediaLoaded`
flag is accepted only with an identity-matched storage result reporting `No Media`
and zero size. `-ValidateOnly` sends no SCSI commands and does not validate an
image or test the installed receiver's update implementation.

### Transfer and activate

For release 1.06, run the following in an elevated Windows PowerShell session
from that release directory, with `$targetSerial` set as above. For another
release, substitute its matching `.bin` and `.json` names together.

```powershell
.\rdx_manager_firmware_update.ps1 -InstallOpenRDX `
  -ImagePath .\OpenRDX-v1-06.bin `
  -ManifestPath .\OpenRDX-v1-06.json `
  -TargetSerialNumber $targetSerial
```

`-ImagePath` is mandatory for this mode. **Do not substitute `-Update`: that
switch writes the pinned vendor `0283` image to an OpenRDX receiver.**
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
`07/74/08` is an error on this route, not the expected staging result used for
vendor installation. The script may retry the complete transfer once from
offset zero after an error; do not launch another updater alongside it.

After successful transfer, mode `05h` at offset `0x00F29E` and length zero
commits the withheld boot vector. The firmware schedules an automatic USB
disconnect and reset. **Do not unplug the receiver during this sequence.**
Normal completion needs neither J7 nor FlashBurner nor a manual power cycle.

### Check completion and handle failures

The updater waits for USB identity `1A5A:0005` and an OpenRDX `0001` disk LUN at
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
