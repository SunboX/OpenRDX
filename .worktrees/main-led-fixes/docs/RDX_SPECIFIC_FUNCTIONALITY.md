# OpenRDX device functionality

OpenRDX combines the TUSB9261 USB-to-SATA data path with the cartridge,
mechanism, front-panel, thermal, and management behavior required by the RDX
dock. This document describes the maintained implementation and its external
contracts.

## Device presentation

The dock exposes a removable direct-access SCSI device rather than forwarding
the identity of the inserted SATA disk. The stable identity is:

- USB VID:PID `1A5A:0005`;
- manufacturer `TANDBERG`;
- product `RDX`, padded to the protocol field width;
- one SCSI-transparent Bulk-Only Transport interface;
- endpoint `0x83` for IN and `0x03` for OUT; and
- SCSI firmware revision `0001` for OpenRDX update-policy checks.

The USB device-release bytes remain `83 02` as a host-compatibility field. The
active configuration has no HID interface, and the generic HID source is not
initialized.

Implementation: [`usb_chap9.c`](../src/rdx_mount/usb_chap9.c),
[`scsi.c`](../src/rdx_mount/scsi.c), and
[`rdx_manager_identity.c`](../src/rdx_mount/rdx_manager_identity.c).

## Front-panel LEDs

The two physical light pipes have distinct jobs:

| Physical indicator | Color outputs | Normal no-cartridge state |
| --- | --- | --- |
| Eject button | amber and green | steady amber |
| Cartridge position | green and amber | off |

The controller's active-low outputs are named by physical indicator and color
in [`rdx_led.h`](../include/rdx_mount/rdx_led.h). Normal state rules are:

- no cartridge: eject-button amber steady, cartridge indicator off;
- cartridge detected: cartridge green steady during initialization;
- media activity: cartridge green blinks on the shared 500 ms cadence;
- accepted eject request: eject-button amber blinks while removal progresses;
- terminal mechanism success: eject-button amber steady; and
- terminal mechanism fault: eject-button green steady.

The hidden button-mode confirmation sequence uses the same logical ownership
layer, so diagnostic output cannot fight normal media or mechanism state.

Implementation: [`rdx_led.c`](../src/rdx_mount/rdx_led.c) and
[`LED_OUTPUT_MAPPING.md`](LED_OUTPUT_MAPPING.md).

## Eject button and hidden mode selector

A normal accepted press increments the eject counter exactly once, requests
ATA standby, starts the mechanism state machine, and switches the eject-button
indicator to amber blink. A held button cannot retrigger the edge-accepted
request.

Holding the button for five seconds with no cartridge enters the seven-choice
mode selector. Successive presses advance the choice inside a one-second
window. The confirmation sequence uses twenty 100 ms phases. Choices zero and
one select the two supported behavior modes; choice one also clears command
prevention. Choices two through five are defined no-ops, and choice six resets
the device. USB reconnect is deferred until the confirmation sequence ends.

Implementation: [`rdx_button_mode.c`](../src/rdx_mount/rdx_button_mode.c).

## Mechanism control

The foreground mechanism controller is an eight-state, non-blocking sequence.
It coordinates:

- PWM1 motor enable;
- GPIO3 motor direction;
- GPIO2 directional endpoint completion sensing;
- GPIO11 auxiliary output control;
- profile-dependent selection of the reverse-drive output;
- per-state timeouts and terminal cleanup; and
- LED state and USB reconnect scheduling.

Every exit path stops PWM1 and returns direction and auxiliary outputs to their
safe idle levels. Logical outputs 11 and 12 are unassigned by this board map,
so the mechanism never treats their zero selector sentinel as GPIO0 and never
substitutes another output for them. An accepted eject request is not cancelled
by a later command-prevention change; that decision belongs to request
acceptance.

GPIO2 completes a powered phase only after the controller has observed it
deasserted and then asserted. An already-asserted input cannot prematurely end
the cycle. Forward drive keeps its two-second travel window. On expiry, the
controller enters the neutral state for two seconds and then drives the return
phase for up to two seconds. A return endpoint enters state 7 while preserving
that phase's existing deadline; once the remaining window closes, state 1
resumes the forward cycle. The return phase is retried twice before the third
expiry selects terminal state 8. Successful GPIO2 completion selects state 4
and its 500-ms settling interval.

Implementation: [`rdx_mechanism.c`](../src/rdx_mount/rdx_mechanism.c).

## Hardware profile

The hardware profile is loaded from the validated unit record in external SPI
flash. Raw profile values `0`, `1`, and `0xFFFF` normalize to profile 37; an
invalid record selects profile 38. Profile 36 disables fan output and selects
the unassigned reverse-drive output; other supported profiles use PWM1 for that
phase.

This profile is consumed by GPIO, fan, mechanism, and management paths through
one runtime API, preventing compile-time profile assumptions from diverging.

Implementation: [`rdx_hardware.c`](../src/rdx_mount/rdx_hardware.c).

## Cartridge write-protection slider

The slider is sampled from MCP3008 channel 4 through SPI chip select 1 and data
format 1. A value greater than 650 is writable; all other values are protected.
The sample is taken once when media initialization reaches ready state. An ADC
timeout or receive error fails closed and therefore reports write-protected.

SCSI MODE SENSE and write-command admission use the latched state. Host write
commands are rejected before ATA submission when protection is active.

Implementation: [`spi.c`](../src/rdx_mount/spi.c),
[`rdx_hardware.c`](../src/rdx_mount/rdx_hardware.c), and
[`scsi.c`](../src/rdx_mount/scsi.c).

## Fan and temperature control

PWM0 is reserved for fan control with a 16,000 microsecond period. The service
loop evaluates thermal state every 800 ms without blocking USB or ATA work.
SMART temperature attribute `0xC2` is preferred, with `0xBE` as fallback.

The controller uses 45 degrees Celsius as the hot threshold and 40 degrees as
the cool threshold. Four consecutive hot samples are required before entering
active cooling. Cooling starts at 50 percent duty and can rise by 13 percentage
points per minute while the drive remains hot. The hysteresis and confirmation
counter prevent rapid fan cycling near a threshold. Profile 36 always keeps
PWM0 disabled. A failed SMART read is deferred by a bounded retry interval, so
unsupported or temporarily unavailable SMART data cannot produce an unbounded
command loop.

Implementation: [`rdx_runtime_ata.c`](../src/rdx_mount/rdx_runtime_ata.c) and
[`rdx_hardware.c`](../src/rdx_mount/rdx_hardware.c).

## SATA media admission and data extents

After SATA initialization, OpenRDX classifies the inserted non-packet device
and keeps the RDX and generic ATA/SATA paths separate:

- RDX media follows the cartridge-access F2 sequence. The firmware validates
  redundant metadata sectors, derives the host-visible user-data extent,
  translates host LBAs through that extent, and reports its reduced capacity.
- Generic ATA/SATA media must have a valid ATA security-status word with both
  ENABLED and LOCKED clear, report a nonzero native capacity and logical-sector
  size, and complete a direct READ VERIFY of sector zero. The firmware then maps
  host LBAs directly and reports the full native capacity through the 64-bit
  SCSI capacity path.

Recognized RDX media with invalid authentication, checksum, or metadata fails
closed and cannot fall through to the generic path. Already accessible media
with valid RDX metadata enters the RDX path without a redundant
cartridge-access command. Locked media must pass RDX authentication. An
unreadable RDX controller that omits the conventional lock indication can use
a bounded access fallback. A completed access rejection is throttled for that
disk until physical disconnect; transport and metadata failures remain
retryable through normal recovery. Every link reinitialization clears the prior
media classification and repeats admission, so extent and identity state are
never reused for another disk.

If every identifying metadata record on an RDX cartridge has been erased and
the ATA security feature is also disabled, the device cannot distinguish that
damaged cartridge from a generic SATA disk. Such a cartridge must not be used
with the direct-disk path until its identifying records have been restored.

The initialization path also supports ATA Power-Up In Standby. When IDENTIFY
reports an incomplete spin-up response, OpenRDX issues SET FEATURES with the
spin-up subcommand, waits for completion, and repeats IDENTIFY before transfer
configuration.

Both media classes use the same USB removable-LUN presentation, LEDs, eject
mechanism, lock-slider write protection, SMART thermal service, and deferred
hot-plug coordinator. USB remains published while readiness is reported through
standard SCSI status and sense data.

Every accepted CDB captures the current media generation. Readiness publication
and final ATA submission verify that generation together with the live link and
latched connect-change state while the relevant interrupt sources are masked.
If a locally polled command times out with its slot still active, the firmware
resets the port, invalidates the generation, and requires fresh admission before
restoring normal command handling.

SCSI block limits are derived from the AHCI scatter/gather window and the
identified logical-sector size, including 4 KiB logical sectors. Oversized
requests fail before command construction rather than being truncated.
Multi-command paths validate ATA ERR/DF and the current media generation after
each command. UNMAP is governed by the same write-protect slider policy as
WRITE commands.

ATA PASS-THROUGH cannot bypass these policies. RDX media exposes only bounded
identity, SMART-read, and power-state telemetry. A protected generic disk uses
an explicit read-only allowlist, while a writable generic disk retains the
supported SAT command surface.

The validated generic-SATA feature branch mounted a 3 TB Seagate ST3000LM024
and passed format, write, read, and delete checks. The integrated dual-media
build still requires device-level validation with both media classes. Repeated
cold starts and I/O above the 32-bit LBA boundary remain explicit validation
items.

Implementation: [`sata_media.c`](../src/rdx_mount/sata_media.c),
[`rdx_unlock.c`](../src/rdx_mount/rdx_unlock.c),
[`ahci.c`](../src/rdx_mount/ahci.c), and
[`scsi.c`](../src/rdx_mount/scsi.c).

## SCSI media and removal controls

The removable LUN remains present while the bay is empty. TEST UNIT READY and
other media-dependent commands translate cartridge and mechanism state into
precise SCSI status and sense fields.

START STOP UNIT separates three operations:

- `LOEJ=0, START=0`: flush cached data and place the disk in standby;
- `LOEJ=0, START=1`: verify current media readiness; and
- `LOEJ=1, START=0`: apply prevention rules, issue standby, and begin eject.

`LOEJ=1, START=1` reports the current media state without treating the field
combination as an invalid request. PREVENT MEDIUM REMOVAL accepts ALLOW even
when media is not ready; PREVENT requires ready media. The physical button and
host command converge only after their distinct admission checks.

Implementation: [`scsi.c`](../src/rdx_mount/scsi.c),
[`rdx_hardware.c`](../src/rdx_mount/rdx_hardware.c), and
[`rdx_mechanism.c`](../src/rdx_mount/rdx_mechanism.c).

## RDX Manager protocol

OpenRDX implements the supported SECURITY PROTOCOL IN/OUT commands, VPD page
`C0`, vendor mode pages `31h`, `33h`, and `34h`, validated diagnostic-page
framing, device identity, drive statistics, command prevention, host logical
unload/reload policy, physical-button inhibition, and authenticated firmware
transfer. Transfer bounds, rolling checks, SHA-256 authentication, and
container markers are validated before an update is accepted.

Implementation: [`rdx_manager_protocol.c`](../src/rdx_mount/rdx_manager_protocol.c),
[`rdx_manager_control.c`](../src/rdx_mount/rdx_manager_control.c), and
[`RDX_MANAGER_PROTOCOL_IMPLEMENTATION.md`](RDX_MANAGER_PROTOCOL_IMPLEMENTATION.md).

## Build and hardware boundary

The production environment is `tusb9261_ti_cgt` and requires TI ARM CGT 5.2.5
on Windows. Unit tests and the build do not flash a device. Hardware programming
must follow [`TUSB9261_FLASHING_PROCEDURE.md`](TUSB9261_FLASHING_PROCEDURE.md)
or the packaged installation procedure, with an independently verified image and
a recoverable test unit.
