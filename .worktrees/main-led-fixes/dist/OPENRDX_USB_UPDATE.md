# OpenRDX USB update

This procedure applies to the supported Tandberg RDX USB compatibility receiver,
identified by the required firmware field `0283`. It uses Windows SPTI and TI
Command Line FlashBurner. The updater checks
the product model, revision, selected serial number, physical USB location, and
every release digest; do not use this procedure with another product or image.

## Before starting

- Put one complete OpenRDX release in its own directory. Keep the versioned
  `.bin`, `-FlashBurner.hex`, `.json`, and `.sha256` files together with
  `rdx_manager_firmware_update.ps1` and this procedure. Do not mix releases.
- Install TI Command Line FlashBurner in its normal location, or pass its exact
  executable path with `-FlashBurnerPath`.
- Connect the intended RDX adapter to stable USB power. If more than one
  compatible adapter is connected, select the intended unit by serial number.
- Eject and physically remove the cartridge. Close RDX Manager and every other
  program that could access the device.
- Use an elevated Windows PowerShell window for the installation.

## Verify the release

From the release directory, verify every file listed in the checksum file:

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

The JSON manifest pins the Manager container, continuous FlashBurner HEX,
procedure, and updater. The updater repeats the firmware digest checks before
it sends or programs anything.

## Inspect, then install

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

Identify the intended adapter's reported `SerialNumber`. An installation requires a
ten-character printable ASCII serial, zero disk size, and no mounted cartridge
volume. If WMI retains its media-loaded flag after removal, the updater accepts
only an identity-matched Windows storage-disk result that explicitly reports
`No Media` and zero size. The updater repeats the identity and empty-bay checks
immediately before WRITE BUFFER. Run its read-only validation first, supplying
the serial whenever
the list contains more than one compatible adapter:

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
container automatically. It validates every artifact, authorizes the selected
compatibility receiver from that adapter's serial, stages the OpenRDX container,
and requires the exact late `07/74/08` authentication result. Any other status
aborts. When prompted, disconnect and reconnect USB with **J7 open**. The adapter
must enumerate as the single `0451:926B` ROM loader in the system and at the same
physical USB location selected before transfer. The updater then programs only
the manifest-pinned continuous HEX.

Do not disconnect during WRITE BUFFER transfer or FlashBurner programming.
Accept only the power cycles requested by the script. After FlashBurner
succeeds, reconnect once more with J7 open; the updater must find the OpenRDX
revision `0001` empty-bay device at that same USB location before reporting
success. Afterwards, `.\rdx_manager_firmware_update.ps1 -ValidateOnly` performs
a read-only check of the installed OpenRDX identity.

## If the sequence is interrupted

- Before the expected `07/74/08` result, stop and diagnose the reported error;
  do not force activation or substitute another image.
- After that result, do not send mode `05h` or restart installation against the
  staged state. Perform the requested USB power cycle and continue only with
  the manifest-pinned HEX.
- If exactly one ROM loader does not appear, or FlashBurner fails, leave the
  adapter in its recoverable state and follow the J7 procedure in the project
  README. Do not guess at a device instance or HEX file.
