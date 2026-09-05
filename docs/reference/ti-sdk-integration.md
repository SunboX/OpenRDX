# TI SDK integration

This document records the maintained fixed-address and controller-structure
mapping used by OpenRDX. It is an implementation reference, not a customer
procedure and not, by itself, proof of current product behavior; use the
maintained [firmware behavior reference](./firmware-behavior.md) for that
purpose.

The TUSB9261 firmware SDK supplies controller structures, register definitions,
and conventional peripheral names. OpenRDX uses those definitions when the
associated controller behavior and fixed-layout offsets agree.

## Confirmed datapath layout

The fixed datapath addresses match the structures in the TI headers:

| Address | OpenRDX role | TI structure |
| --- | --- | --- |
| `0xC0010000` | AHCI command-list base | `AHCI_CMD_HEADER_T` array programmed into `PxCLB` |
| `0xC0010100` | AHCI received-FIS base | `AHCI_RFIS_T` area programmed into `PxFB` |
| `0xC0010200` | AHCI command-table base | Per-slot `AHCI_CMD_TABLE_T` storage |
| `0xC0010A00` | USB event buffer | Address programmed into `GEVNTADR0_LO` |
| `0xC0010C00` | USB endpoint-zero buffer | Control-transfer buffer following the event area |
| `0xC0010E00` | USB bulk endpoint buffer | 1024-byte bulk-transfer workspace |
| `0xC0011200` | Endpoint-zero setup TRB | Four `TRANSFER_REQUEST_BLOCK_T` words |
| `0xC0011290` | USB IN TRB ring | Follows setup and per-endpoint TRBs |
| `0xC0011340` | USB OUT TRB ring | Follows the eleven-entry IN ring |
| `0xC0011610` | ATA IDENTIFY DEVICE data | Word offsets match serial, firmware, model, capacity, and feature fields |

The received-FIS aliases now use the field names from `REGISTER_FIS_D2H_T`, the
PIO Setup FIS, DMA Setup FIS, and Set Device Bits FIS. The ATA IDENTIFY aliases
use the standardized word roles: serial number at word 10, firmware revision at
word 23, model number at word 27, LBA28 capacity at words 60-61, SATA capability
words, LBA48 capacity at words 100-103, sector-size information at word 106,
world-wide name at words 108-111, and alignment information at word 209.

## Runtime integration

USB HAL initialization and event handling, AHCI port lifecycle operations, RTI
compare configuration, MWW setup, and low-level SPI transfer helpers use the TI
SDK register and structure definitions. RDX-only behavior uses names describing
its current device role.

## Fixed-layout boundary

The complete binding record is
[`tests/fixtures/firmware_symbol_manifest.json`](../../tests/fixtures/firmware_symbol_manifest.json).
Every fixed-address binding has a supported semantic name and an assigned
address. The manifest and linker command file together define the maintained
address boundary.

The [firmware symbol catalog guide](firmware-symbols.md) explains the evidence
levels, source-tree boundaries, and checks for current source references.

[Back to the documentation index](../README.md)
