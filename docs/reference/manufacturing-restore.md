# Manufacturing record restore

This OpenRDX-specific protocol restores exactly the 256-byte manufacturing
record at SPI flash `0x3E000`. It requires no arbitrary flash-write command and
does not write the boot image or the state-record sector at `0x3F000`.

Introduced in OpenRDX 1.10, this implementation has executable host simulations.
Physical manufacturing restore has not yet been validated on a dock.
Support must be discovered by the exact probe below, irrespective of a dock's
version string. Previously released firmware does not gain this capability.

## Commands

Both commands are ten-byte SCSI WRITE BUFFER CDBs. Every byte, including reserved
and control bytes, must match. A command with a different CDB length is rejected.

| Command | Exact CDB | Data OUT |
| --- | --- | --- |
| Capability | `3B 02 4D 00 00 01 00 00 00 00` | None |
| Restore | `3B 02 4D 00 00 00 00 02 10 00` | Exactly 528 bytes |

An accepted capability probe returns SCSI GOOD without reading flash, reserving
SPI, changing identity, or moving the mechanism. Firmware download/activation
in progress or an earlier serial mutation causes the dispatcher to reject
manufacturing commands. There is
no automatic reset after either command.

The restore command has this payload, using half-open byte ranges:

| Bytes | Meaning |
| --- | --- |
| `[0,8)` | Eight ASCII bytes `RDXMFG01` |
| `[8,12)` | Expected current entire 4096-byte sector CRC-32, little endian |
| `[12,16)` | Four zero reserved bytes |
| `[16,272)` | Exact expected current raw manufacturing record, 256 bytes |
| `[272,528)` | Exact saved raw manufacturing record to restore, 256 bytes |

CRC is CRC-32/ISO-HDLC: reflected polynomial `0xEDB88320`, initial value
`0xFFFFFFFF`, final XOR `0xFFFFFFFF`, processing the sector from `0x3E000`
through `0x3EFFF`. It is a stale-data guard, not authentication.

The saved desired record must be non-erased and satisfy the four-lane checksum.
Initialize lanes to `78 56 34 12`, add bytes 4 through 255 modulo 256 to
lane `(offset - 4) & 3`, and require bytes 0 through 3 to equal the four lanes.
The expected current record may be valid, erased, or corrupt: the exact-byte
and full-sector CRC comparison deliberately permit recovery of damaged records.
No serial, product, profile, padding, or other bytes are synthesized.

## Mutation boundary

`src/rdx_mount/scsi.c:scsi_handle_write_buffer_cmd` rejects wrong CDB lengths
and routes buffer ID `0x4D` to
`src/rdx_mount/rdx_manufacturing.c:rdx_manager_handle_manufacturing`.
BOT preserves the initial CBW transfer length via `ums_bot.c` and checks the
final received length for an exactly declared 528-byte restore. Oversized CBWs
and short receives are both rejected.

The receiver saves and masks the USB, AHCI, and SATA receiver-error interrupt
sources, then requires a fresh safe bay state and acquires exclusive SPI
ownership. An interrupted foreground flash operation or ADC frame causes a
restore to fail before flash access. The safe-bay predicate requires:

- GPIO5's active-low cartridge-presence signal is absent;
- the mechanism is idle and no eject is pending;
- ATA readiness is false, SATA DET is zero, and PxCI/PxSACT are zero;
- no deferred ATA callback remains queued.

GPIO2 is a mechanism endpoint and is not used as the cartridge-presence test.
Logical unload or a failed media admission alone cannot satisfy physical absence.

BOT receives the request into the 4116-byte SCSI response buffer, which ATA DMA
cannot overwrite before dispatch. After interrupt masking and ATA-idle checks,
the entire 4096-byte sector is read into the separate normal-data buffer. Both
the raw-record comparison and sector CRC must match before a write-enable is
sent. Physical state is checked again after the snapshot/CRC and immediately
before erase. An identical desired record returns GOOD without erase/program.
Immediately before the first write-enable, a mutation latch blocks further
restores, serial edits and firmware updates until a manual restart, including
when completion is uncertain.
If the final physical check fails after write-enable, write-disable is sent
before returning failure, without erasing the sector.

The receiver changes only the first 256 bytes of its sector snapshot, erases
one sector at `0x3E000`, programs sixteen aligned 256-byte pages, and reads back
all sixteen pages. GOOD requires an exact comparison of the complete sector,
including all 3840 neighboring bytes. SPI reservations and saved interrupt
enables are restored on every return; sources already disabled stay disabled.

`spi.c:SpiOpsBounded` uses the existing SPI framing with one-millisecond
controller-word flag waits and a two-second flash-status wait budget. After an
SPI timeout or receive error, further flash/ADC acquisitions fail until SPI
initialization at a manual restart: a later request must not append data to an
incomplete chip-select frame. The receiver never automatically retries an erase,
resets the dock, or updates its cached USB identity/hardware profile mid-boot.

## Failures and limitations

Malformed payloads, stale snapshots, checksum-invalid desired records, occupied
or busy docks, and SPI ownership conflicts cause a failed SCSI command before
erase. SPI/program/readback failures produce a failed command. A readback error
in a neighboring byte is a failure even when the manufacturing record matches.

NOR sector erase is not atomic. Power loss, USB power interruption, or hardware
failure after erase can leave any part of this sector damaged, including its
neighboring records. Keeping a saved backup outside the dock and stable power
is necessary. There is no second-sector journal or power-failure rollback. The
checksum and CRC provide integrity and freshness checks, not caller identity or
proof that a saved record belongs to this dock; Manager must bind the selected
device and require deliberate backup selection before sending the command.

Raw flash reads fail if SPI access fails; they never report a stale response
buffer as successful backup or restore readback.

After a successful restore, raw flash readback reflects the saved bytes. The
running device's identity and hardware-profile caches intentionally keep their
boot-time values. A deliberate manual restart is needed to activate them.

## Automated evidence

Run `~/.platformio/penv/bin/python -m unittest discover -s tests -v` and the
normal TI build `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`.

`tests/test_rdx_manufacturing_restore.py` compiles the real receiver and SPI C
implementations with a host compiler for simulation. Fixtures replace physical
registers, NOR storage, and the dispatcher's update-state query. They cover
exact CDB/payload validation, dispatch, stale CAS values, invalid targets,
erased/corrupt-current recovery, physical insertion races, mechanism/SATA work,
neighbor preservation, complete readback, no-op/probe behavior, flash failures,
bounded TX/RX/BUSY/WEL waits, fault latching, and bus/interrupt ownership.

These tests do not replace the TI firmware build, verify a distributable
release, or prove physical flash behavior on a receiver.
