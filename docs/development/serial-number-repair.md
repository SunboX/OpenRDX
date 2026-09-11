# Serial-number inspection and repair

The USB serial is `00` followed by the ten ASCII unit-serial digits stored at
offset `0x08` of the 256-byte manufacturing record at flash address `0x3E000`.
There is no numeric conversion. A missing or checksum-invalid record selects
the fallback unit serial `7360242889`, displayed over USB as `007360242889`.

Serial repair is available through a new, separately framed command in 1.09.
It does not run automatically during boot or installation. The ten serial
digits supplied by the operator must be the intended identity for that unit.
The command accepts arbitrary decimal digits, including leading zeros.

## Inspection evidence

On 2026-09-11 a read-only, target-bound macOS probe returned 256 bytes of `FF`
from the connected receiver's manufacturing record. Its USB serial was
`007360242889`; the supplied earlier USB serial was `007820746632`.
The record's SHA-256 was
`3d6876a0146de8576eb2395a858de1213d1b92c65b779df3a331cfd5a4584546`.
This proves an erased record at inspection time, not which installation erased
it. The serial calculation itself follows the expected format.

The available saved profile for this receiver also contained the fallback
serial. Restoring the known unit serial `7820746632` therefore requires explicit
fallback initialization. That restores the chosen identity, not unknown per-unit
manufacturing fields, production history, or calibration.

## Host sequence

1. Select exactly one receiver by full USB serial and physical location, require
   an empty bay, and retain the same exclusive transport lease.
2. Read the complete 4096-byte sector at `0x3E000`, with identity checks around
   the reads. Save an owner-private, durable backup and a target-bound repair plan.
   The firmware record occupies the first 256 bytes; the remaining bytes must
   also survive any erase needed to edit a valid record.
3. Validate the plan again on apply, including its backup hash, USB identity,
   location, and equality with the complete fresh sector. Require the explicit
   fallback-initialization flag if and only if all 256 record bytes are `FF`.
4. Probe the capability described below. An unsupported response must stop the
   utility before it sends a serial write. Older firmware does not support it.
5. Send the one fixed serial-write request. Firmware verifies the expected
   record and full-sector CRC, rechecks physical cartridge absence, and writes
   only the manufacturing sector. It verifies every sector byte before GOOD.
6. Read back and compare the entire resulting sector. Only serial/checksum bytes
   may change for a valid record. An erased record receives the explicitly
   selected fallback profile described below.
7. Request a device reset, then verify the new USB serial at the same physical
   location. The firmware retains its earlier in-memory identity until reset.

After an actual or uncertain serial write, another serial write or firmware
download is rejected until reset. Reads remain available. A power interruption
during a valid-record sector rewrite can require recovery from the saved backup;
this operation is not power-fail atomic. Never discard the backup after failure.
An erased-record repair programs one page without erasing any sector.

## Closed command contract

Both commands use SCSI WRITE BUFFER (`3B`), mode `02`, buffer ID `53`.
All reserved bytes must be zero. These are separate from the firmware download
commands in modes `04` and `05`, which continue to require buffer ID zero.

| Operation | Ten-byte CDB | Payload |
| --- | --- | --- |
| Capability query | `3B 02 53 00 00 01 00 00 00 00` | None; GOOD has no side effect |
| Serial write | `3B 02 53 00 00 00 00 01 18 00` | Exactly 280 bytes |

| Payload offset | Length | Meaning |
| --- | --- | --- |
| `00` | 8 | ASCII `RDXSER01` |
| `08` | 4 | Little-endian IEEE CRC32 of the expected complete 4096-byte sector |
| `0C` | 1 | `00` for a valid record; `01` to initialize an entirely erased record |
| `0D` | 1 | Reserved, zero |
| `0E` | 10 | New ASCII decimal unit serial, without the USB `00` prefix |
| `18` | 256 | Exact expected raw manufacturing record, including checksum |

CRC32 uses the reflected polynomial `EDB88320`, initial value `FFFFFFFF`, and
final XOR `FFFFFFFF`, matching Python `zlib.crc32`. It detects stale sector
contents; it is not authentication or an authorization mechanism.

The four record-checksum lanes start at `78 56 34 12`. Bytes `04..FF` are
added modulo 256 to lane `offset & 3`. The resulting four bytes are stored at
offset zero. A valid record is edited only at offsets `00..03` and `08..11`.

Fallback initialization starts with zero-filled bytes, version 6 at `04`, the
chosen serial at `08`, `TANDBERG` at `12`, space-padded `RDX` at `1A`, EC value 1
at `30`, little-endian USB identifiers `1A5A:0006` at `76`, matching vendor/product
strings at `7A` and `82`, date `10102010` at `9C`, and little-endian profile `0038`
at `A4`. These are the documented fallback values. OpenRDX's active USB PID
remains `0005`. Nonblank, checksum-invalid records are rejected, never guessed.

## Implementation and validation

- `src/rdx_mount/rdx_manager_serial.c` owns record validation, serial editing,
  bounded sector writes, and complete readback. BOT receives the serial request
  in the separate SCSI response buffer so ATA discovery DMA cannot alter the
  requested serial. The writer rejects outstanding ATA commands before borrowing
  the 4-KiB normal buffer for the sector snapshot, and rechecks before programming.
  This avoids a new 4-KiB allocation in the controller's limited program RAM.
- Saved settings mask USB command submission across their complete flash
  transaction and restore its previous interrupt state. A USB serial write
  cannot interrupt their SPI command or write-enable sequence.
- `src/rdx_mount/rdx_manager_protocol.c` owns command framing, the side-effect-free
  capability query, and exclusion against firmware downloads.
- `tests/test_rdx_serial_repair.py` executes the actual C writer and dispatcher
  against host-side flash and transport adapters. It covers invalid frames,
  stale snapshots, occupied bays, untouched opaque/neighbor bytes, erased-only
  initialization, no-op edits, and uncertain writes/readback failures.

Validation on 2026-09-11 passed all 291 firmware tests and the macOS TI ARM CGT
5.2.9 build. The complete six-file 1.09 bundle passed its manifest and checksum
checks. The container SHA-256 is
`2a207091cbdcafc60777a4a9a2c7b5137a2c2481572ff92911d6eabc3aca71f8`.
The linked runtime uses 51,868 of the 61,696-byte image allowance.

Host tests are not physical-device proof. No 1.09 installation or serial-write
hardware qualification is recorded here yet. See the
[installation guide](../getting-started/installation.md) and
[recovery guide](rom-loader-recovery.md) before changing a receiver.
