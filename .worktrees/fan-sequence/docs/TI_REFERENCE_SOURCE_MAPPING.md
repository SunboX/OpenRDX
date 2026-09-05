# TI reference source mapping

The external `TUSB9261FW_SourceCode.zip` release was used as a read-only naming
reference. Its source was built with the configured TI ARM CGT 5.2.9 compiler to
confirm that the archive is internally coherent and to obtain its linker map.
The reference firmware is not assumed to be byte-identical to the recovered RDX
image; names were accepted only where source behavior or fixed-layout offsets
also matched the recovered code.

## Confirmed datapath layout

The recovered fixed addresses match the structures in the TI headers:

| Address | Recovered role | TI structure evidence |
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

## Confirmed routine names

Matching control flow and register operations restored TI names for the USB HAL
initializer and event handlers, AHCI port initialization/reset/start/stop paths,
RTI compare configuration and delay routines, MWW window initialization helpers,
and low-level SPI polling/transfer helpers. RDX-only behavior retains descriptive
names derived from its own call sites rather than borrowing an unrelated TI name.

## Confidence boundary

The full decision record is
[`tests/fixtures/firmware_symbol_manifest.json`](../tests/fixtures/firmware_symbol_manifest.json).
Every renamed absolute symbol keeps its original address. Symbols without a
unique match remain listed in
[`UNRESOLVED_FIRMWARE_SYMBOLS.md`](UNRESOLVED_FIRMWARE_SYMBOLS.md).
