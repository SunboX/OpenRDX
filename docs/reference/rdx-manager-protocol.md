# RDX Manager protocol implementation

## Scope

This maintainer reference documents the Manager-compatible subset implemented
by OpenRDX. It is not a claim of complete RDX Manager compatibility. For
customer operation, start with [Using OpenRDX](../getting-started/using-openrdx.md).

OpenRDX keeps one removable direct-access LUN available with or without a
cartridge. This lets RDX Manager discover the adapter, inspect dock and media
state, change supported settings, run diagnostics, and transfer firmware. Media
commands return NOT READY / medium not present while the bay is empty.

All multi-byte fields, allocation limits, page lengths, and checksum rules are
handled locally for the documented subset. Other security-protocol commands are
translated through the existing ATA trusted-command path; they are not
advertised as supported OpenRDX customer controls.

## Discovery sequence

RDX Manager begins with standard INQUIRY, VPD page `83h`, and LOG SENSE page
`20h`. The 64-byte INQUIRY response provides:

| Offset | Value | Meaning |
|---:|---|---|
| 8..15 | `TANDBERG` | T10 vendor |
| 16..31 | `RDX` padded with spaces | product |
| 32..35 | `0108` | OpenRDX release `1.08`, encoded as two major and two minor decimal digits |
| 36 | `38h` | RDX product type |
| 37..39 | `RDX` | RDX capability signature |
| 40 | `02h` | OEM type |
| 41 | `57h` | feature byte 0 |
| 42 | `50h` | feature byte 1 |

When a cartridge is ready, Manager can additionally request:

- VPD `C0h`: cartridge label, physical block count, disk model/serial,
  cartridge vendor/model/serial, and barcode. For admitted generic SATA media,
  the first cartridge-property field carries the ATA model as the Manager media
  label while the three metadata-only properties remain empty;
- VPD `C2h`: 512-byte ATA IDENTIFY data with each 16-bit word in SCSI byte
  order;
- LOG SENSE `21h`: cartridge state and counters;
- LOG SENSE `0Dh`: SMART temperature;
- READ CAPACITY; and
- MODE SENSE pages `31h`, `33h`, `34h`, and the caching page.

Drive LOG page `20h` is 56 bytes with parameters zero through seven. Cartridge
page `21h` is up to 116 bytes with parameters zero through thirteen. Missing
cartridge counters are omitted, with the page length adjusted accordingly.
Temperature page `0Dh` is 16 bytes: parameter zero contains the current sensor
sample and parameter one contains the reference temperature. An unavailable
reading uses `FFh`; it must not be presented as a zero-degree measurement.

### Adapter identity VPD page 83h

The response contains one fixed T10 designator. Vendor begins at byte 8,
product at byte 16, serial at byte 32, and the eight-byte manufacturing date at
byte 42. The complete response is 50 bytes. A 256-byte manufacturing record at
SPI address `0003E000h` supplies these fixed-width fields:

| Field | Offset | Length |
|---|---:|---:|
| serial | `08h` | 10 bytes |
| vendor | `12h` | 8 bytes |
| product | `1Ah` | 16 bytes |
| manufacturing date | `9Ch` | 8 ASCII bytes, record version 5 and later |
| hardware profile | `A4h` | 2 bytes, little-endian |

Bytes zero through three are independent modulo-256 checksum lanes over bytes
four through 255. An invalid record uses the compiled ten-byte numeric fallback
identifier, vendor `TANDBERG`, product `RDX` padded with spaces, and hardware
profile `38h`, and default date `10102010`. A valid record older than version 5
has no date field and reports an empty date. Profile values zero, one, and
`FFFFh` normalize to `37h`.

### Drive and cartridge status

Drive page `20h` parameter 3 reports the active-low mechanism input on GPIO2.
Parameter 5 reports bus power for hardware profile `38h` when GPIO4 indicates
that the device is not self-powered. Other profiles retain the adapter-power
encoding. The drive load count remains the persistent dock counter.

Cartridge page `21h` reads stored context-4 metadata during cartridge admission:

| Metadata record | Stored value | LOG SENSE parameter |
| --- | --- | --- |
| `0Ah` | Cartridge load count | 1 |
| `0Bh` | Read MiB | 6 |
| `0Ch` | Written MiB | 5 |

Each scalar has a four-byte typed header and one to four little-endian payload
bytes. The page emits valid values as four-byte big-endian integers. Zero is a
valid stored value; missing, malformed, or duplicate counters do not become
zero samples. Only checksum-validated sectors are decoded. A bounded metadata
walk continues after identity is found, and cached values are cleared when the
medium is replaced. Generic SATA media have no cartridge metadata counters.

These are stored counters sampled during admission. OpenRDX does not currently
add session I/O totals to them or persist updated cartridge usage records.
Parameter 8 uses the cartridge write-protection input. Filesystem free space is
not available to this firmware page and must be queried by the host.

Implementation: `src/rdx_mount/rdx_manager_status.c`,
`src/rdx_mount/rdx_media_metadata.c`, and `src/rdx_mount/rdx_unlock.c`.
Deterministic response checks: `tests/test_rdx_status_values.py`.

On 2026-09-11 the user reported successful local operation after the macOS
TI ARM CGT 5.2.9 build of v1.08. Its container SHA-256 was
`6e7453c672fdd495fe13dc0110b0c26da3c6d049ff0e6d3cc5e40e04ea83ffe7`.
This is user-reported acceptance, without an independent parameter capture or
a flash readback. It does not establish byte identity with the Windows release
build or validate session counter persistence.

### MODE SENSE block descriptor

Page `00h` with DBD clear returns the eight-byte MODE SENSE(10) header followed
by one eight-byte direct-access block descriptor and no mode page. The final
three descriptor bytes contain the cartridge logical block size. MODE SENSE(6)
and MODE SENSE(10) vendor-page requests also include a descriptor when DBD is
clear. The header's write-protect bit reflects the insertion-time lock-slider
sample.

### Security status

SECURITY PROTOCOL IN protocol `20h` is answered locally for selectors `0000h`,
`0001h`, `0010h`, `0011h`, `0012h`, `0020h`, and `0021h`. Selectors `0010h`
and `0021h` return 44-byte and 16-byte no-security records. Other protocols keep
the SAT pass-through behavior.

OpenRDX does not locally implement a blanket credential-changing SECURITY
PROTOCOL OUT policy. Commands not handled by the local Manager subset continue
through the ATA trusted non-data or trusted-send path, depending on transfer
length. That forwarding behavior is an implementation fact, not a supported
customer workflow; do not use OpenRDX documentation as authorization to alter or
erase cartridge credentials.

## Manager controls

These controls remain available with an empty bay:

| Operation | Command | Behavior |
|---|---|---|
| host eject policy | MODE SELECT(6/10), page `31h` | bit 3 selects logical unload; bit 4 selects one-shot auto-reload or explicitly reloads hidden media when bit 3 is clear |
| physical eject gate | MODE SELECT(6/10), page `33h` | byte 3 bit 0 inhibits physical-button eject and is returned by current/saved MODE SENSE |
| operation mode | MODE SELECT(10), page `34h` | applies operation mode and maximum SATA generation |
| saved operation mode | MODE SELECT(10), `SP=1` | rewrites the checksummed 64-byte state record at `3F000h` |
| self-test | SEND DIAGNOSTIC, `SELFTEST=1` | enters the SCSI self-test path |
| LED test | `1D 00 00 00 08 00`, page `80h` | toggles both bicolor controllers at 500-ms cadence |
| eject control frame | `1D 00 00 00 06 00`, page `85h` | validates and acknowledges the frame without changing mechanism, visibility, or page `31h`/`33h` state |
| unit reset | `1D 05 00 00 00 00` | disconnects USB and resets the controller |

The LED test changes controller mode rather than temporarily borrowing pins.
The LED driver permanently owns GPIO7/6 for cartridge green/amber and SCI
GPIO8/9 for eject-button amber/green. Disabling the diagnostic preserves the
controller state and returns both controllers to normal mode.

Page `31h` bit 3 consumes host LOEJ through a reversible visibility gate: ATA
readiness, fan control, LEDs, and the mechanism remain unchanged while ordinary
media commands report not ready. Bit 4 with bit 3 performs unload/reload as a
one-shot and clears both bits. Bit 4 without bit 3 is accepted only while media
is logically unloaded, reloads it, and clears the action bit. Physical removal
or mechanism teardown clears both policy bits. Page `33h` affects the physical
button only; host LOEJ and the empty-bay configuration gesture remain separate.

## Firmware-update transport

RDX Manager uses a ten-byte WRITE BUFFER command:

```text
3B mm 00 oo oo oo ll ll ll 00
```

- mode `04h` transfers a chunk at a 24-bit absolute container offset;
- chunks are sequential and no larger than 4096 bytes;
- the complete container is 62,110 bytes (`0000F29Eh`); and
- mode `05h`, offset `0000F29Eh`, length zero activates a validated image.

The executable interval is `[018Ch, F29Ah)`, mapped to SPI flash
`[0000h, F10Eh)`. The four trailing container bytes are metadata and are not
flashed. Application sectors `0000h` through `F000h` are erased;
manufacturing/state sectors `3E000h` and `3F000h` are preserved.

The receiver enforces these safety properties:

1. Only buffer ID zero, modes `04h`/`05h`, bounded chunks, and the exact next
   offset are accepted.
2. A compatibility image is accepted only at the pinned length and SHA-256.
3. An OpenRDX image must contain `OPENRDX1` at offset `0108h` and the unkeyed
   SHA-256 integrity digest of the complete boot region at offset `0110h`; the
   rest of that 128-byte image-validation field is zero.
4. The first four boot-vector bytes stay in RAM while the remaining image is
   written in page-boundary-safe transactions.
5. The boot vector is programmed only after complete validation and mode-05
   activation.
6. Activation returns GOOD, waits 500 ms for the BOT status to reach the host,
   then disconnects and resets.

Power must remain stable after the first mode-04 command. An interrupted
transfer leaves the boot vector invalid so the controller enters its ROM loader
instead of executing a partial image.

## Container generation

The [release process](../development/release-process.md) uses
`scripts/build_rdx_update_container.py` to combine the sparse build HEX with the
pinned compatibility template. The template must match SHA-256
`73D528801AEFC032D3A53637B035F65D72809151B2F127C6B2050E9BACC76F3B`.
The generated manifest records all build-specific hashes and declares
`installation_requires_rom_loader: true` for compatibility installation.

## Guarded updater

Read-only target validation is the default. The updater supports returning an
OpenRDX receiver to the pinned compatibility image, repairing that compatibility
image, installing OpenRDX through the ROM loader, and updating OpenRDX in place.
The [installation guide](../getting-started/installation.md) owns the guarded
operator commands. For an existing `0001` receiver, use its
[in-place update procedure](../getting-started/installation.md#update-existing-openrdx)
with `-InstallOpenRDX`. The `-Update` switch restores the pinned compatibility image.

The updater requires one unambiguously selected target with no cartridge volume.
An optional serial-number filter can select one receiver while other compatible
adapters remain connected. Re-enumeration is then bound to that receiver's
physical USB location. Compatibility installation separately requires exactly
one `0451:926B` ROM loader system-wide because FlashBurner selects it by index.

For OpenRDX images both installation branches verify the manifest, container
integrity digest, the exact manifest field `authentication_scheme`, template
digest, ROM-loader requirement field, and device identity. Compatibility
installation additionally verifies the continuous-HEX digest and accepts only
the exact `07/74/08` authentication result before requesting the first USB power
cycle and selecting the ROM loader. In-place installation requires successful
transfer and sends mode `05h` directly; `07/74/08` is a failure on that route.

## Validation boundary

The protocol, container, state-record, and updater checks are covered by the
automated suite and TI warnings-as-errors build. Focused device tests have
exercised empty-bay Manager discovery, cartridge pages, saved controls, LED
diagnostics, WRITE BUFFER transfers, 100-percent Manager completion, the
compatibility installation, ROM-loader programming, and OpenRDX re-enumeration.
The generated manifest is authoritative for the artifact hashes and runtime size
of that generated bundle. SHA-256 checks integrity against those expected bytes;
they do not authenticate a publisher or prove that firmware is signed.

[Back to the documentation index](../README.md)
