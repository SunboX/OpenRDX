# OpenRDX firmware for the TUSB9261 RDX platform

OpenRDX is a source-built PlatformIO firmware for the Tandberg TUSB9261 RDX
adapter. PlatformIO coordinates the build while TI ARM Code Generation Tools
compile, assemble, link, and convert the image with the required ARM9 COFF ABI.

The `main` branch is the integrated dual-media image. It supports authenticated
RDX cartridges and validated generic ATA/SATA disks, publishes one removable
USB mass-storage LUN, exposes the RDX Manager command surface, and controls the
complete front panel and cartridge mechanism.

## Implemented behavior

### Front-panel LEDs

The firmware owns two independent active-low bicolor controllers. Their color
order is intentionally different:

| Controller | First output | Second output | Physical pins |
|---|---|---|---|
| Eject button / dock | Amber | Green | SCI GPIO8 / GPIO9 |
| Cartridge / activity | Green | Amber | GPIO7 / GPIO6 |

Normal visible behavior is:

- With no cartridge, the eject-button LED is steady amber and the cartridge
  LED is off.
- Insertion first lights the cartridge LED steady green. Cartridge discovery
  and storage traffic then use the green blink sequence while the eject-button
  LED remains amber.
- An accepted eject-button request blinks the eject-button LED amber.
- Media or initialization failure can select the cartridge amber channel.
- The RDX Manager LED diagnostic alternates both color pairs every 500 ms.

Both controllers share one scheduler. Normal indications advance every 500 ms;
the eject-button confirmation gesture temporarily selects a 100-ms cadence for
both controllers. Detailed pin ownership, active levels, and state transitions
are documented in [`docs/LED_OUTPUT_MAPPING.md`](docs/LED_OUTPUT_MAPPING.md).

### Eject button and mechanism

GPIO1 is an active-low button input with a 100-ms stable-state filter. With a
cartridge present, one accepted press:

1. checks the SCSI PREVENT/ALLOW interlock;
2. waits for ATA commands and deferred callbacks to become idle;
3. places the drive in standby;
4. withdraws media readiness from the host;
5. runs the non-blocking GPIO3/PWM1 mechanism sequence;
6. follows the directional GPIO2 endpoint transition and bounded return path; and
7. publishes either the ejected state or the terminal fault indication.

GPIO2 completion requires a deasserted sample followed by assertion during the
powered phase. Forward motion uses the full two-second travel window so the cam
can complete its cycle. If that phase expires, the controller stops for two
seconds, drives the return phase for up to two seconds, and retries through the
defined eight-state route. Reaching the return endpoint enters state 7 without
restarting the timer, preserving the same return window before forward
travel resumes. Three expired return attempts select the terminal fault state.

Host `START STOP UNIT` with `LOEJ=1, START=0` enters the same coordinator. Each
accepted host or physical eject request increments the checksum-protected load
counter once before mechanism completion. An ordinary `LOEJ=0, START=0` command
uses its separate FLUSH plus STANDBY path and never starts the mechanism.

With no cartridge, holding the button continuously for five seconds enters the
seven-entry service menu. The eject-button LED shows the selection as one to
seven pulses. A short press/release followed by a second press inside one second
confirms; a five-second selection hold or the 60-second session timeout cancels.
Selections zero and one persist operation modes 1 and 2, entries two through
five are inert, and entry six resets the controller. Mode 2 also clears the
medium-removal interlock. Mode changes use a timed USB disconnect/reconnect.

### Lock slider and write protection

The lock slider is MCP3008 channel 4 on SPI chip select 1. It is sampled once
before a newly initialized cartridge becomes host-visible:

- sample greater than `650`: writes enabled;
- sample at or below `650`: writes protected;
- ADC failure: writes protected.

MODE SENSE reports the cached write-protect state. WRITE(6/10/12/16) and WRITE
AND VERIFY(10/12/16) are rejected locally while locked, before an AHCI slot is
allocated.

### Fan and temperature control

PWM0 drives the fan with a 16,000-microsecond period. Hardware profile `36h`
keeps the controller disabled; other supported profiles start at 10 percent
and enter the normal thermal state machine. Temperature comes from SMART:

- normal sampling interval: five minutes;
- hot retry interval: ten seconds;
- hot threshold: 45 degrees Celsius;
- cool threshold: 40 degrees Celsius;
- normal running duty: 50 percent;
- hot ramp: 13 percentage points per minute, capped at 100 percent.

The 41-44 degree band retains the current phase. Failed SMART reads use a
bounded retry interval instead of retrying on every foreground-loop pass or
hiding the failure behind a new five-minute delay.

### Cartridge and USB mass storage

OpenRDX selects one of two media paths after each SATA initialization:

- RDX cartridges keep their authenticated path. The firmware validates the
  redundant media metadata, derives the user-data extent, reports that reduced
  capacity, and translates host LBAs for READ, WRITE, and VERIFY.
- A generic ATA/SATA disk is admitted only when it is a non-packet device, has
  a valid ATA security-status word with both ENABLED and LOCKED clear, reports
  nonzero native capacity and logical-sector size, and completes a direct READ
  VERIFY of sector zero. The disk uses direct LBAs and exposes its full native
  capacity through the 64-bit SCSI capacity path. RDX Manager displays the
  disk's standardized ATA model text as its media label because ATA IDENTIFY
  does not provide a separate manufacturer property.

Media that identifies as RDX but fails its metadata or authentication checks is
never reclassified as generic. Structurally recognizable RDX metadata also
fails closed when its checksum is damaged. Already accessible media with valid
RDX metadata enters the RDX path without issuing a redundant cartridge-access
command. Locked media must pass RDX authentication. If an RDX controller omits
the conventional lock indication, the firmware permits a bounded access
fallback for an unreadable non-packet device. A completed access rejection is
not repeated against the same disk until physical disconnect; transport and
metadata failures remain retryable through the normal recovery path.
Drives that advertise Power-Up In Standby are started with ATA SET FEATURES
before transfer configuration and are identified again when required.

If every identifying metadata record on an RDX cartridge has been erased and
the ATA security feature is also disabled, the device cannot distinguish that
damaged cartridge from a generic SATA disk. Do not use a cartridge in that
condition with the direct-disk path; restore its identifying records first.

Both media paths share the same front-panel LEDs, eject coordination, lock-slider
write protection, SMART thermal service, and deferred hot-plug handling. An
empty bay keeps one stable removable LUN available for RDX Manager discovery
while media commands return NOT READY / medium not present.

Each SCSI command is bound to the media generation present when its CDB is
accepted. The final ATA submission and ready-state publication gates also check
the live SATA link while the relevant interrupt sources are masked. A timed-out
local command is retired with a port reset before discovery can admit media
again, preventing a late completion from being assigned to a replacement disk.

The reported SCSI block limits are derived from the AHCI scatter/gather window
and the identified logical-sector size, including 4 KiB logical sectors.
Oversized transfers are rejected before command construction instead of being
truncated. Multi-command operations validate ATA ERR/DF status and the current
media generation between commands. UNMAP follows the same write-protect slider
policy as WRITE commands.

ATA PASS-THROUGH preserves the same separation: RDX media accepts only bounded
identity, SMART-read, and power-state telemetry; generic disks with an active
write-protect slider accept only an explicit read-only command set. A writable
generic disk retains the supported SAT command surface.

The validated generic-SATA feature branch mounted a 3 TB Seagate ST3000LM024
and passed format, write, read, and delete checks. The integrated dual-media
build still requires device-level validation with both RDX and generic media;
repeated cold starts and I/O above the 32-bit LBA boundary remain explicit test
items.

The RDX Manager surface includes:

- 64-byte INQUIRY and T10 VPD identity;
- cartridge identity VPD page `C0h`;
- ATA IDENTIFY VPD page `C2h`;
- LOG SENSE pages `20h`, `21h`, and `0Dh`;
- MODE SENSE pages `31h`, `33h`, and `34h`;
- matching MODE SELECT controls and saved operation mode;
- SECURITY PROTOCOL IN discovery responses;
- LED diagnostic, host logical-eject policy, physical-button inhibit, validated
  eject-control framing, and unit reset; and
- WRITE BUFFER modes `04h` and `05h` for OpenRDX updates.

The firmware-update receiver enforces sequential chunks no larger than 4 KiB,
validates the complete 62,110-byte container, preserves manufacturing/state
sectors, and withholds the first boot vector until validation completes. Mode
`05h` delays reset long enough for the final BOT status to reach the host.
Protocol layouts and failure behavior are documented in
[`docs/RDX_MANAGER_PROTOCOL_IMPLEMENTATION.md`](docs/RDX_MANAGER_PROTOCOL_IMPLEMENTATION.md).

## Build and test

Required Windows compiler:

```text
C:\ti\ti-cgt-arm_5.2.5
```

Build with warnings promoted to errors:

```powershell
C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
```

Run the complete automated suite:

```powershell
C:\Users\andre\.platformio\penv\Scripts\python.exe -m unittest discover -s tests -v
```

Set `TI_CGT_ROOT` to select another compatible TI ARM CGT installation. Do not
substitute GCC, Clang, or a TI compiler that lacks `ti_arm9_abi` COFF support.
The macOS configuration uses `~/ti/ti-cgt-arm_5.2.9`; Apple Silicon requires
Rosetta for those Intel compiler executables.

Generated build artifacts are under `.pio/build/tusb9261_ti_cgt/`:

- `TUSB9261_RDX.out`
- `TUSB9261_RDX.map`
- `TUSB9261_RDX.hex`
- `TUSB9261_RDX_flash.hex`
- `TUSB9261_RDX_update.bin`
- `TUSB9261_RDX_update.json`

Only `TUSB9261_RDX_update.bin` is an RDX Manager container. The continuous
`_flash.hex` file is for TI FlashBurner.

## Build the release bundle

Run:

```powershell
.\build-dist.ps1
```

The script builds with TI ARM CGT, runs the complete test suite, generates the
Manager container, validates its length and digest, and creates `dist/` with:

- `OpenRDX-v1-06.bin`
- `OpenRDX-v1-06-FlashBurner.hex`
- `OpenRDX-v1-06.json`
- `OpenRDX-v1-06.sha256`
- `rdx_manager_firmware_update.ps1`
- `OPENRDX_USB_UPDATE.md`

Keep all six files together in an otherwise empty release directory. The
updater refuses ambiguous directories containing more than one versioned
manifest. The JSON and SHA-256 files are authoritative for each build; hashes
are intentionally not copied into this README because firmware changes produce
new values.

`-SkipTests` is available only when the complete suite already passed for the
exact source tree. `-Version` must match `VERSION` and the compiled major/minor
defines.

## Firmware update workflows

Firmware operations require Windows, an elevated PowerShell session, stable USB
power, exactly one pinned RDX device, and an empty bay. The script performs a
read-only target check by default. When an OpenRDX device retains a stale WMI
media-loaded flag after cartridge removal, the updater accepts only an
identity-matched Windows storage-disk result that explicitly reports `No Media`,
zero size, and no mounted volumes:

```powershell
.\dist\rdx_manager_firmware_update.ps1 -ValidateOnly
```

OpenRDX self-update:

```powershell
.\dist\rdx_manager_firmware_update.ps1 `
  -InstallOpenRDX `
  -ImagePath .\dist\OpenRDX-v1-06.bin `
  -ManifestPath .\dist\OpenRDX-v1-06.json
```

Install the pinned compatibility image while OpenRDX is running:

```powershell
.\dist\rdx_manager_firmware_update.ps1 `
  -Update `
  -ImagePath C:\firmware\RDX2E__STD__F-0283.bin
```

Reinstall that image on a matching compatibility receiver:

```powershell
.\dist\rdx_manager_firmware_update.ps1 `
  -RepairCompatibilityReceiver `
  -ImagePath C:\firmware\RDX2E__STD__F-0283.bin
```

Install OpenRDX on a matching compatibility receiver:

```powershell
.\dist\rdx_manager_firmware_update.ps1 -InstallOpenRDXOnCompatibilityReceiver
```

The installation validates the expected `07/74/08` authentication result, asks for
one USB power cycle, requires exactly one `0451:926B` ROM loader, programs the
manifest-pinned continuous HEX, and verifies OpenRDX re-enumeration. Follow
[`docs/OPENRDX_USB_UPDATE.md`](docs/OPENRDX_USB_UPDATE.md) exactly.

The script never selects an arbitrary disk or bootloader. It verifies model,
firmware revision, full PnP identity, the media-loaded flag, disk size, mounted
volumes, container digest,
authentication scheme, template digest, and FlashBurner HEX digest before any
write.

## Safety and validation boundary

- No upload target is configured. A normal PlatformIO build never flashes the
  adapter.
- Never remove USB power during WRITE BUFFER transfer or FlashBurner
  programming.
- Never format, partition, or test-write a cartridge during a firmware update.
- The GPIO1 button, GPIO3/PWM1 mechanism drive, PWM0 fan output, LED pins,
  MCP3008 slider channel, RDX mount path, and Manager update transport have
  focused hardware-test evidence. The integrated image must still complete a
  full device-level regression before it is described as hardware-qualified.
  The corrected full-cycle eject/return sequence still requires device-level
  verification before flashing for ordinary use.

## Cartridge insertion and startup ordering

Cold-start SATA discovery completes before USB is connected. This keeps a
slow disk from blocking the foreground while the host is waiting for its first
USB descriptor; an empty or rejected cartridge still results in the same
published removable LUN with not-ready status after the bounded probe returns.

SATA connect-change interrupts are reduced to a fail-closed pending event. The
foreground loop first isolates interrupted DMA and callback state, then keeps
polling the link until `PxSSTS.DET` reaches PHY-ready before running cartridge
discovery and authentication. This preserves early insertion notifications
that arrive while the link is still training without repeatedly entering the
blocking discovery path for an empty bay.

Mechanism initialization places GPIO3 and PWM1 in their idle state but cannot
start timed motion. Conditional boot homing begins only after synchronous AHCI
startup returns, immediately before the watchdog-backed foreground loop starts
servicing mechanism deadlines. This guarantees that the two-second motor
deadline remains enforceable during startup.
Deferred SATA discovery also yields while the mechanism state machine is
active, so no later hot-plug operation can block an already-running motor
deadline.

## Project layout

- `src/rdx_mount/`: active firmware sources.
- `include/rdx_mount/`: active firmware headers.
- `linker/tusb9260_link.cmd`: TI linker command file.
- `scripts/ti_cgt_build.py`: PlatformIO/TI build adapter.
- `scripts/build_rdx_update_container.py`: Manager container builder.
- `scripts/rdx_manager_firmware_update.ps1`: guarded Windows updater.
- `tests/`: source, image, protocol, and release checks.
- `docs/`: hardware mappings, protocol details, and update procedures.

## License

Repository-owned OpenRDX software is available under the GNU Affero General
Public License v3.0 or later (`AGPL-3.0-or-later`). Documentation and non-code
media authored for OpenRDX are available under Creative Commons
Attribution-ShareAlike 4.0 (`CC-BY-SA-4.0`) where marked in `.reuse/dep5`.

Closed-source, proprietary, or otherwise AGPL-incompatible use of the
repository-owned software requires a separate commercial license. Texas
Instruments material and third-party documentation retain their existing
copyright notices and terms. See [`LICENSE`](LICENSE),
[`COMMERCIAL-LICENSE.md`](COMMERCIAL-LICENSE.md), and [`NOTICE`](NOTICE).
