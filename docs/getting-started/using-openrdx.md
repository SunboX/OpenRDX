# Using OpenRDX

This guide covers everyday operation of OpenRDX on the supported Tandberg Data
RDX QuikStor external USB 3.0 compatibility receiver built around TUSB9261 and
identified before installation by preinstalled vendor firmware revision `0283`.
OpenRDX reports revision `0107` for version `1.07`; earlier builds used the
fixed marker `0001`. Other docks are outside the
current support scope.

For installation, follow the separate [guarded installation guide](installation.md).
For implementation detail and test limits, see
[firmware behavior](../reference/firmware-behavior.md).

## RDX cartridges and standard SATA drives

OpenRDX supports two distinct media paths:

| Media | Admission | Host-visible capacity |
| --- | --- | --- |
| RDX cartridge | Must retain recognizable RDX identity and pass the cartridge access and metadata checks. | The cartridge's validated user-data area. |
| Standard SATA drive | Must be a compatible, unlocked drive and pass guarded access checks. | The admitted drive's native capacity. |

The checks fail closed. They reduce the chance of exposing ambiguous or unsafe
media, but they do not promise compatibility with every SATA HDD or SSD.

> [!WARNING]
> Do not place a damaged RDX cartridge on the direct-disk path. If all of its
> identifying metadata and its ATA security state have been erased, OpenRDX may
> be unable to distinguish it from a generic disk. Restore the cartridge's
> identifying records before using it again.

## One stable removable drive

The receiver publishes one removable USB logical unit whether media is present
or not. This keeps the receiver discoverable and avoids creating a different USB
device for each inserted disk.

- With accepted media, normal capacity and data access become available.
- With an empty bay, media-dependent requests report that no medium is present.
- With rejected media, the USB drive remains present but the medium does not
  become ready.

If the operating system offers to initialize, format, or repair a medium that
OpenRDX has not admitted, cancel the operation. Remove the medium, confirm that
it meets the requirements above, and check the
[detailed validation boundary](../reference/firmware-behavior.md#validation-boundary).

## Write protection

The physical write-protection slider or tab belongs to an RDX cartridge; it is
not a control on the dock. Before publishing either admitted media path,
OpenRDX samples the same receiver hardware input, caches its state, and uses it
to gate writes. A failed reading defaults to write-protected. When protected,
OpenRDX rejects normal writes and trim/UNMAP before disk submission.

Set an RDX cartridge's slider before insertion. If you change it while the
cartridge is available, safely eject and reinsert it before relying on the new
state. A bare standard SATA drive has no RDX cartridge slider, and this
repository does not establish that every mechanical SATA carrier exposes a
user-adjustable equivalent. Confirm the host-reported read-only state before
relying on physical protection for standard SATA media.

## Eject safely

Use the same cautious workflow for RDX cartridges and standard SATA media:

1. Finish file copies and close applications using the drive.
2. Use the operating system's eject or safely-remove action and wait for it to
   complete.
3. If the medium remains in the bay, press the dock's eject button once and wait
   for the mechanism to release it.
4. Remove the medium only after host activity has stopped and the dock has
   completed the eject action.

OpenRDX coordinates supported host-eject and physical-button requests with
pending storage work and host removal locks. A host may prevent physical removal
temporarily; clear the application's removal lock or complete the host eject
rather than forcing the medium out. Do not disconnect USB power during an active
write or firmware update.

## When media is rejected

Rejected media looks like an empty or not-ready drive to the host. OpenRDX does
not silently reinterpret recognizable but invalid RDX media as a standard SATA
drive.

When this happens:

1. Do not format, partition, repair, or test-write the medium.
2. Safely remove it and reconnect the receiver if the host has cached the prior
   state.
3. For a standard SATA drive, verify that it is unlocked and that its reported
   capacity and sector information are valid.
4. For an RDX cartridge, treat damaged identity or access metadata as a
   cartridge-repair problem, not as permission to use the direct-disk path.

## RDX Manager support

OpenRDX supports a defined RDX Manager subset for receiver discovery, identity
and status pages, selected saved controls, diagnostics, eject policy, and the
guarded firmware-update transport. It does not claim complete compatibility
with every RDX Manager feature or every credential-changing operation.

The exact implemented commands and limitations are documented in the
[RDX Manager protocol reference](../reference/rdx-manager-protocol.md).

## Current validation boundary

The standard-SATA implementation is integrated on `main`, while physical
standard-SATA evidence currently covers one 3 TB Seagate ST3000LM024 on the
feature branch. Repeated cold starts, I/O above the 32-bit LBA boundary, a full
device regression with both media types, and the corrected eject/return cycle
still require device validation. See
[Firmware behavior](../reference/firmware-behavior.md#validation-boundary) for
the maintained technical statement.

[Back to the documentation index](../README.md)
