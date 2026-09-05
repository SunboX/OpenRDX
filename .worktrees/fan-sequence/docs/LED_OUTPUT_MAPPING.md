# RDX LED Output Mapping Audit

## Outcome

The original RDX v2.83 payload confirms two bicolor LED controllers and all
four color channels. The firmware registers logical outputs `(6, 7)` as one
controller and `(8, 9)` as the other, and its built-in diagnostic command puts
both controllers into their alternating-color test mode.

The controller state machine selects the second member of each pair when a
fault flag is set and the first member in the normal state. The manufacturer
identifies fault indication as amber and ready indication as green. The
logical-output table and setter establish the exact physical selector,
register, and active-low polarity below.

## Source integrity

- Original RDX payload: `TUSB9261_RDX_firmware_payload_v2.83_0x08000000.bin`
- Payload SHA-256: `4854e7c7b61c286c2a969adb9670151a627c6c079aa71f98073bfb1bf8ff9f21`
- Logical table address: `0x0800E408`
- Bicolor controller initialization:
  - `0x08007CBC` registers logical outputs `(8, 9)` at controller `0x0800F29C`.
  - `0x08009750` registers logical outputs `(6, 7)` at controller `0x0800F2B4`.
- Built-in LED diagnostic: the command path at `0x0800356A` sets both
  controllers to alternating-color mode through the helper at `0x0800D0C8`.
- Color and polarity data flow:
  1. `0x0800CD9C` stores the aggregated fault state in controller byte `+5`.
  2. On the stable-state path, `0x080084F8` copies `(byte +5 != 0)` to the
     controller state word at `+0`.
  3. `0x0800AE24` asserts the second logical output at `+0x10` when that state
     word is nonzero; otherwise it asserts the first output at `+0x0C`.
  4. `0x0800506C` converts an asserted logical state to a low physical level
     for polarity byte zero. A deasserted state drives the physical level high.
  5. *Tandberg Data RDX QuikStor External USB Quick Start Guide*, P/N 433780
     Rev. F, page 2, identifies `On Amber` as `Fault` and `On Green` as `Ready`
     for both the Activity LED and the Eject Button/Power LED.
- Pair identity:
  - `(6, 7)` is initialized with the GPIO3 eject-button callback path and is
    the dock/eject-button LED controller.
  - `(8, 9)` is linked to the cartridge-state object and is the cartridge LED
    controller.

## Recovered logical-pin table

Each entry occupies eight bytes. The first word is the physical selector. In
the second word, byte zero is the polarity consumed by the logical-output
setter and byte one controls output direction.

| Logical index | Physical selector | Config word | LED role |
| ---: | ---: | ---: | --- |
| 6 | 7 | `0x00000100` | Dock green |
| 7 | 6 | `0x00000100` | Dock amber |
| 8 | 8 | `0x00000100` | Cartridge green |
| 9 | 9 | `0x00000100` | Cartridge amber |

For polarity byte zero, `spi_set_logical_output()` writes the physical bit low
when the logical state is asserted and high when it is deasserted. All four
bindings are therefore active-low; the safe all-off register value has every
one of these four bits set.

## DOCK_GREEN

- Confidence: Confirmed
- Register: `0xFFF7BC3C`
- Selector: `GPIO7 via logical output 6`
- Polarity: `active-low`

## DOCK_AMBER

- Confidence: Confirmed
- Register: `0xFFF7BC3C`
- Selector: `GPIO6 via logical output 7`
- Polarity: `active-low`

## CARTRIDGE_GREEN

- Confidence: Confirmed
- Register: `0xFFF7E548`
- Selector: `SCI GPIO8 via logical output 8`
- Polarity: `active-low`

## CARTRIDGE_AMBER

- Confidence: Confirmed
- Register: `0xFFF7E548`
- Selector: `SCI GPIO9 via logical output 9`
- Polarity: `active-low`

## Normal firmware implementation

The normal RDX firmware preserves USB, SATA/AHCI, cartridge mechanics,
SPI-flash access, HID, SCSI, watchdog, and the reconstructed RDX application.
This change names the recovered logical outputs at the two existing controller
initialization call sites; it does not alter their values, initialization
order, controller state machines, or diagnostic behavior.
