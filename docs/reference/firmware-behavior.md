# OpenRDX device functionality

OpenRDX retains the RDX cartridge workflow and adds a guarded direct-media path
for qualifying **standard SATA drives**. Both paths use the TUSB9261 USB-to-SATA
data path together with the dock's mechanism, front panel, thermal service, and
supported management behavior.

This is a maintainer-facing technical reference for the maintained
implementation and its external contracts. Owners should begin with
[Using OpenRDX](../getting-started/using-openrdx.md); developers should use the
[common build guide](../development/building.md).

## Device presentation

The dock exposes a removable direct-access SCSI device rather than forwarding
the identity of the inserted SATA disk. The stable identity is:

- USB VID:PID `1A5A:0005`;
- manufacturer `TANDBERG`;
- product `RDX`, padded to the protocol field width;
- one SCSI-transparent Bulk-Only Transport interface;
- endpoint `0x83` for IN and `0x03` for OUT; and
- SCSI firmware revision `0108` and USB `bcdDevice` `0108h` for release `1.08`,
  both derived from the compiled major/minor version. Earlier OpenRDX builds
  reported the fixed SCSI marker `0001` and USB revision `0283h`.

At startup OpenRDX reads the checksum-protected manufacturing identity and
publishes the USB serial as `00` followed by the receiver's ten-character unit
serial. An unreadable or checksum-invalid record uses the documented fallback
identity.

The USB device-release bytes remain `83 02` as a host-compatibility field. The
active configuration has no HID interface, and the generic HID source is not
initialized.

Implementation: [`usb_chap9.c`](../../src/rdx_mount/usb_chap9.c),
[`scsi.c`](../../src/rdx_mount/scsi.c), and
[`rdx_manager_identity.c`](../../src/rdx_mount/rdx_manager_identity.c).

## Front-panel LEDs

The two physical light pipes have distinct jobs:

| Physical indicator | Color outputs | Normal no-cartridge state |
| --- | --- | --- |
| Eject button | amber and green | steady amber |
| Cartridge position | green and amber | off |

The controller's active-low outputs are named by physical indicator and color
in [`rdx_led.h`](../../include/rdx_mount/rdx_led.h). Normal state rules are:

- no cartridge: eject-button amber steady, cartridge indicator off;
- cartridge detected: cartridge green steady during initialization;
- media activity: cartridge green blinks on the shared 500 ms cadence;
- accepted eject request: eject-button amber blinks while removal progresses;
- terminal mechanism success: eject-button amber steady; and
- terminal mechanism fault: eject-button green steady.

The hidden button-mode confirmation sequence uses the same logical ownership
layer, so diagnostic output cannot fight normal media or mechanism state.

Implementation: [`rdx_led.c`](../../src/rdx_mount/rdx_led.c) and
[`led-output-map.md`](led-output-map.md).

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

Implementation: [`rdx_button_mode.c`](../../src/rdx_mount/rdx_button_mode.c).

## Mechanism control

The foreground mechanism controller is an eight-state, non-blocking sequence.
It coordinates:

- PWM1 motor enable;
- GPIO3 motor control level;
- GPIO2 directional endpoint completion sensing;
- GPIO11 auxiliary output control;
- profile-dependent GPIO0 control and recovery-drive output selection;
- per-state timeouts and terminal cleanup; and
- LED state and USB reconnect scheduling.

Every exit path stops PWM1 and returns GPIO3 high after a 50-microsecond dead
time. Both powered phases select GPIO3 low. The hardware profile mapping
assigns GPIO0 to logical output 11 for non-38h profiles: it is low at
idle and asserted during recovery only for profiles 35h/37h. On profile 38h,
GPIO0 is logical output 14 and remains high through boot, connected USB
operation, and mechanism travel; motor cleanup preserves it. Logical output 12
remains unassigned. An accepted eject request is not cancelled
by a later command-prevention change; that decision belongs to request
acceptance.

GPIO2 completes a powered phase only after the controller has observed it
deasserted and then asserted. The initial level is sampled before applying
power so a deasserted starting position is retained if the switch closes before
the first service pass. An already-asserted input cannot prematurely end the
cycle. Initial drive keeps its two-second travel window. On expiry, the
controller enters the neutral state for two seconds and then drives the return
phase for up to two seconds. A return endpoint enters state 7 while preserving
that phase's existing deadline; once the remaining window closes, state 1
resumes the initial cycle. The return phase is retried twice before the third
expiry selects terminal state 8. Successful GPIO2 completion selects state 4
and its 500-ms settling interval.

During mechanism activity, the foreground loop skips CPU sleep because the
polled GPIO2 input cannot wake it. Live media-context ADC reads are needed only
when both cartridge presence and SATA link are absent; the complete MCP3008
frame shares a one-millisecond timeout. An expired initial travel deadline is
handled before sampling the ADC or starting a no-media grace interval. The
SATA receiver-error interrupt defers blocking stop/reset operations while the
mechanism owns travel, retaining discovery and callback work for foreground
processing after settling. Ownership is published before motor power is applied.
The first timing changes alone did not resolve the reported roughly 31-second
run. After the additional MODE SENSE guard, the user confirmed normal eject and
mechanical reinsertion. Subsequent testing reported inaccessible inserted media
and continuous fan operation; see the
[eject mechanism validation](../development/eject-reference-audit.md).

Implementation: [`rdx_mechanism.c`](../../src/rdx_mount/rdx_mechanism.c).

## Hardware profile

The hardware profile is loaded from the validated unit record in external SPI
flash. Raw profile values `0`, `1`, and `0xFFFF` normalize to profile 37; an
invalid record selects profile 38. Profile 36 disables fan output and selects
the unassigned recovery-drive output; other supported profiles use PWM1 for that
phase.

MODE SENSE caching/all-pages queries use cached device information while media
is unavailable, logically unloaded, or eject is queued/active. Refreshing ATA
IDENTIFY in that interval could block USB interrupt handling for seven seconds
and delay the motor's foreground endpoint and deadline checks.

This profile is consumed by GPIO, fan, mechanism, and management paths through
one runtime API, preventing compile-time profile assumptions from diverging.

Implementation: [`rdx_hardware.c`](../../src/rdx_mount/rdx_hardware.c).

## Cartridge write-protection slider

The physical write-protection slider or tab belongs to an RDX cartridge, not
the dock. The firmware samples the receiver input used by that cartridge slider
before publishing either admitted media path and caches the result. A bare SATA
drive has no RDX cartridge slider, and the repository does not establish that
every mechanical SATA carrier exposes a user-adjustable equivalent.

The input is sampled from MCP3008 channel 4 through SPI chip select 1 and data
format 1. A value greater than 650 is writable; all other values are protected.
An ADC timeout or receive error fails closed and therefore reports
write-protected.

SCSI MODE SENSE and write-command admission use the latched state. Host write
commands are rejected before ATA submission when protection is active.

Implementation: [`spi.c`](../../src/rdx_mount/spi.c),
[`rdx_hardware.c`](../../src/rdx_mount/rdx_hardware.c), and
[`scsi.c`](../../src/rdx_mount/scsi.c).

## Fan and temperature control

PWM0 is reserved for fan control with a 16,000 microsecond period. The service
loop evaluates thermal state every 800 ms without blocking USB or ATA work.
SMART temperature attribute `0xC2` is preferred, with `0xBE` as fallback.

The controller starts cooling on a valid temperature of at least 45 degrees
Celsius and stops at 40 degrees or below. Temperatures between those limits
retain the current demand. Cooling starts at 50 percent duty and can rise by
13 percentage points per minute while the hot demand remains active. Drive
insertion alone does not request fan power. Temperature polling begins at the
first idle opportunity after media becomes ready and repeats every 10 seconds.
An unavailable reading preserves existing cooling demand; it cannot start the
fan without a prior valid hot reading. Profile 36 always keeps PWM0 disabled.
Eject and outstanding ATA work defer temperature polling, and a failed SMART
read uses a bounded retry interval. The user confirmed the combined cool-drive,
access, and eject/reinsertion check after installation. Operation at the actual
45/40-degree thresholds has not been measured on the device.

Implementation: [`rdx_runtime_ata.c`](../../src/rdx_mount/rdx_runtime_ata.c) and
[`rdx_hardware.c`](../../src/rdx_mount/rdx_hardware.c).

## SATA media admission and data extents

After SATA initialization, OpenRDX classifies the inserted non-packet device
and keeps the RDX and standard-SATA paths separate. The implementation calls
the latter `SATA_MEDIA_KIND_GENERIC`:

- RDX media follows the cartridge-access F2 sequence. The firmware validates
  redundant metadata sectors, derives the host-visible user-data extent,
  translates host LBAs through that extent, and reports its reduced capacity.
- Standard SATA media must have a valid ATA security-status word with both
  ENABLED and LOCKED clear, report a nonzero native capacity and logical-sector
  size, and complete a direct READ VERIFY of sector zero. The firmware then maps
  host LBAs directly and reports the full native capacity through the 64-bit
  SCSI capacity path.

Recognized RDX media with invalid authentication, checksum, or metadata fails
closed and cannot fall through to the standard-SATA path. Already accessible media
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
mechanism, cached write-protection input, SMART thermal service, and deferred
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
each command. UNMAP is governed by the same cached write-protection state as
WRITE commands.

ATA PASS-THROUGH cannot bypass these policies. RDX media exposes only bounded
identity, SMART-read, and power-state telemetry. A protected generic disk uses
an explicit read-only allowlist, while a writable generic disk retains the
supported SAT command surface.

Implementation: [`sata_media.c`](../../src/rdx_mount/sata_media.c),
[`rdx_unlock.c`](../../src/rdx_mount/rdx_unlock.c),
[`ahci.c`](../../src/rdx_mount/ahci.c), and
[`scsi.c`](../../src/rdx_mount/scsi.c).

## Validation boundary

The standard-SATA feature branch mounted one 3 TB Seagate ST3000LM024 and
passed format, write, read, and delete checks. The dual-media behavior is now
integrated on `main`, but the integrated image is not fully hardware-qualified.
Repeated cold starts, I/O above the 32-bit LBA boundary, complete device
regression with both RDX and standard SATA media, and the corrected full-cycle
eject/return sequence remain explicit device-validation items.

Device testing found excessive motor travel and a short repeat eject on first
reinsertion. A live trace identified a seven-second motor-service stall when a
MODE SENSE query issued ATA IDENTIFY during removal. Those queries now use cached
information while eject is pending/active or the medium is unavailable. PWM
startup ordering, profile outputs, interrupt deferral, and insertion handling
have also been corrected. The user confirmed normal eject and mechanical
reinsertion after the MODE SENSE fix, then reported continuous fan operation
with inserted media and loss of Explorer access after insertion. Follow-up
changes make fan demand temperature-dependent at 45°C on / 40°C off, explicitly
schedule discovery on insertion, and preserve USB command state when SATA
discovery completes. After flashing the follow-up, the user reported success
with Explorer access, Manager information, cool-drive fan behavior, and another
eject/reinsertion check; Windows also showed the 320 GB cartridge online. This
does not establish operation at the thermal thresholds or across other media.
See the [binary and timing audit](../development/eject-reference-audit.md) for
the detailed evidence and remaining device checks.

Source-level tests and TI warnings-as-errors builds cover the guarded admission,
capacity, transfer, hot-plug, and ejection contracts. Those results do not
substitute for the outstanding device work above.

## Startup and hot-plug ordering

Cold-start SATA discovery completes before USB is connected. A slow disk can
therefore finish bounded discovery before the host requests its first USB
descriptor. An empty bay or rejected medium still produces the same published
removable logical unit in the not-ready state after probing returns.

A SATA connect-change interrupt records and isolates a pending event. The
foreground loop drains already queued completions, stops the affected port, and
waits for the link to become PHY-ready before it repeats the same dual-media
admission pipeline. The pending event remains latched while an inserted disk is
still training, rather than repeatedly running blocking discovery for an empty
bay. The removable USB device remains published during this process.

Mechanism initialization establishes idle outputs only. Conditional boot homing
starts after synchronous SATA discovery returns and immediately before the
watchdog-backed foreground loop begins servicing deadlines. Deferred media
discovery yields while mechanism motion is active, so later hot-plug work cannot
block an already-running eject/return deadline.

Implementation: [`main.c`](../../src/rdx_mount/main.c),
[`ahci.c`](../../src/rdx_mount/ahci.c), and
[`rdx_mechanism.c`](../../src/rdx_mount/rdx_mechanism.c).

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

Implementation: [`scsi.c`](../../src/rdx_mount/scsi.c),
[`rdx_hardware.c`](../../src/rdx_mount/rdx_hardware.c), and
[`rdx_mechanism.c`](../../src/rdx_mount/rdx_mechanism.c).

## Supported RDX Manager subset

OpenRDX locally implements its documented Manager discovery, identity, status,
control, diagnostics, and update subset. That includes selected SECURITY
PROTOCOL IN responses, VPD page `C0`, vendor mode pages `31h`, `33h`, and `34h`,
validated diagnostic-page framing, drive statistics, command prevention, host
logical unload/reload policy, physical-button inhibition, and integrity-checked
firmware transfer. Transfer bounds, rolling checks, an unkeyed SHA-256 integrity
digest, and container markers are validated before an update is accepted.

Complete RDX Manager compatibility is not claimed. Non-intercepted SECURITY
PROTOCOL commands follow the underlying ATA trusted-command path and are not
advertised as supported customer controls.

Normal mode-04 firmware transfer erases sixteen 4 KiB sectors, limited to
`0x00000` through `0x0FFFF`. The manufacturing record at `0x3E000` and state
records at `0x3F000`/`0x3FC00` remain outside that update range. Legacy TI SCSI
unlock `E1h` and chip erase `E2h` both return `STATUS_SCSI_INVALID_CMD` without
SPI operations; an unlock request cannot enable full-chip erasure. The earlier
legacy handler issued SPI opcode `C7h` after unlock, which erased persistent
records along with the application. This establishes a destructive source path;
it does not identify which operation erased a particular receiver without a
matching transfer trace.

Persistent operation-mode and eject-count writes mask the USB interrupt across
their complete SPI read/modify/write sequence. Both success and failure restore
only the USB bit that was enabled before entry, leaving an already masked caller
masked and preserving other interrupt-mask changes. USB flash reads and update
requests therefore cannot interrupt those foreground state transactions.

Implementation: [`rdx_manager_protocol.c`](../../src/rdx_mount/rdx_manager_protocol.c),
[`rdx_manager_control.c`](../../src/rdx_mount/rdx_manager_control.c), and
[`rdx-manager-protocol.md`](rdx-manager-protocol.md).

## Build and hardware boundary

The production environment and artifact roles are defined in
[Build and test OpenRDX](../development/building.md). Unit tests and a build do
not flash a device. Normal installation requires a newly generated, complete
release bundle and the
[guarded installation procedure](../getting-started/installation.md). Direct
programming is limited to the
[advanced ROM-loader recovery procedure](../development/rom-loader-recovery.md)
with an independently verified image and a recoverable test unit.

[Back to the documentation index](../README.md)
