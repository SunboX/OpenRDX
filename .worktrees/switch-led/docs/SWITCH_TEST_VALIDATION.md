# RDX Lock-Slider Front LED Test Validation

## Outcome

The `lock_unlock-slider-switch` branch replaces the production RDX runtime
with a standalone polling test. The current image reads the confirmed
lock-slider signal from MCP3008 channel 4 and displays it on the green LED.

The initial GPIO1 build was flashed on 2026-08-24. The green LED responded to
the eject button but not to the cartridge lock slider. That result physically
identifies GPIO1 as the eject-button input and disproves the original lock-
slider assumption. A second build monitored static logical input 3/GPIO2, but
neither switch affected the LED. Production startup subsequently remaps
logical input 4—the debounced cartridge-present input—to active-low GPIO5.
A third build monitored GPIO5, but the lock slider still did not affect the
LED. GPIO1, GPIO2, and GPIO5 are therefore all ruled out for the operated
switch. The dual-channel ADC image was then flashed and tested successfully:
pressing the cartridge lock slider turns the green LED on. This confirms
MCP3008 channel 4 with threshold 650 as the operated switch signal. The amber
channel-2 indicator did not identify the switch. The final firmware removes
the channel-2 read and leaves the amber LED off.

## Original payload evidence

- Source payload:
  `TUSB9261_RDX_firmware_payload_v2.83_0x08000000.bin`
- SHA-256:
  `4854e7c7b61c286c2a969adb9670151a627c6c079aa71f98073bfb1bf8ff9f21`
- Logical-I/O table: `0x0800E408`, eight bytes per entry.
- Eject-button entry: `0x0800E410` (logical input 1, GPIO1).
- Disproven second candidate: `0x0800E420` (logical input 3, GPIO2).
- Runtime source record: `0x0800E3D8` (selector GPIO5, polarity 0).
- Runtime destination: `0x0800E428` (logical input 4).
- ADC threshold table: `0x0800E36C`.
- ADC-backed logical input 0: selector 18, MCP3008 channel 4, threshold 650.
- ADC-backed logical input 16: selector 16, MCP3008 channel 2, threshold 124.
- Logical-input reader: `0x08006174`.

The payload and the confirmed LED mapping establish:

| Test signal | Recovered binding | Standalone behavior |
| --- | --- | --- |
| Eject button, physically confirmed | Logical input 1, physical GPIO1, polarity 0 | `GIOIN0` bit 1 low is asserted |
| Disproven second candidate | Logical input 3, physical GPIO2, polarity 0 | Neither tested switch changed the LED |
| Cartridge-input candidate | Runtime logical input 4, physical GPIO5, polarity 0 | `GIOIN0` bit 5 low is asserted |
| ADC-backed input 0 | Runtime selector 18, MCP3008 channel 4, threshold 650 | Green LED shows `sample > 650` |
| ADC-backed input 16 | Runtime selector 16, MCP3008 channel 2, threshold 124 | Amber LED shows `sample > 124` |
| Front green LED | Physical GPIO7, active-low | `GIOOUT0` bit 7 low turns green on |
| Front amber LED | Physical GPIO6, active-low | `GIOOUT0` bit 6 low turns amber on |

The payload proves the selector and polarity of both entries. The first
hardware run now establishes the GPIO1/eject-button association, and the
second run disproves GPIO2 for both tested switches. The original startup code
copies the GPIO5 record into logical input 4, and the cartridge state machine
repeatedly reads that input with a 100 ms debounce. The GPIO5 hardware result
rules out that path for the operated switch. The two remaining recovered input
paths use the MCP3008, so the diagnostic now exposes both threshold results.

## Implemented behavior

1. Enable GIO and preload GPIO7 and GPIO6 high before enabling their output
   directions, so both active-low LED channels start off.
2. Initialize SPI with the production register values.
3. Read MCP3008 channel 4 using the recovered three-word transaction.
4. Drive green from `channel 4 > 650`.
5. Leave amber off.

There is intentionally no eject meaning, motor/PWM action, host coordination,
USB, SATA/AHCI, cartridge state machine, or production error handling.

## Minimal link scope

The build adapter compiles only:

- `src/main.c`
- `src/startup.c`
- `src/exceptions_isr.asm`
- `src/intvecs.asm`

The branch has no production C modules, reconstructed-core fragments,
production headers, absolute-symbol command file, fixtures, or inapplicable
production semantic tests. It retains the focused slider source contract and TI
toolchain resolver tests.

## Verification commands

```bash
~/.platformio/penv/bin/python -m unittest discover -s tests -v
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

The build must use TI ARM CGT, `ti_arm9_abi`, warnings-as-errors, the existing
linker memory map, and the existing artifact names.

## macOS TI CGT build result

The complete 9-test Python suite and PlatformIO build passed on 2026-08-16
with TI ARM C/C++ Compiler 5.2.9. The emitted build action listed exactly these
inputs:

- `src/main.c`
- `src/startup.c`
- `src/exceptions_isr.asm`
- `src/intvecs.asm`
- `include/switch_test.h`
- `linker/tusb9260_link.cmd`

The resulting artifacts were:

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 2,689 bytes | `0cf761fc90960244fb1b6ff3393df84a785bef0d33d78770a23aa5a91a277a84` |
| `TUSB9261_RDX.hex` | 784 bytes | `4f8cf07390e24eada9cf0dbee22537daae9ddffa1fd3a99e0a80f5b2403903ff` |
| `TUSB9261_RDX.map` | 4,296 bytes | `8fec52d591bd5759502eb43abbdf6995720bbe23ac6de89ccb91d63651c69e48` |

The object directory contains exactly `main.obj`, `startup.obj`,
`exceptions_isr.obj`, and `intvecs.obj`. The map allocates `.intvecs` from
`intvecs.obj` and `.text` from the other three objects. No production RDX core,
USB, SATA/AHCI, SCSI, UMS, motor/PWM, or watchdog object is present. The
diagnostic's minimal SPI implementation remains inside `main.obj`.

## Windows ADC diagnostic build and flash

The focused six-test source contract and a clean PlatformIO build passed on
2026-08-24 with TI ARM CGT 5.2.5. After channel 4 was confirmed on hardware,
the channel-2 diagnostic was removed. The final linked flash span ends at
`___etext__ = 0x0800032C`. A Flash Burner image was generated as one continuous
range from `0x08000000` through `0x0800032B`; it preserves all 620 emitted bytes
and fills the 192-byte vector-to-text gap with `0xFF`.

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 3,534 bytes | `e1c1099e2bbc3f65a79195d8d7643b1a317ceab2668b1271cd516f26520d6e08` |
| `TUSB9261_RDX.hex` | 1,530 bytes | `822639b27325568f4c07c5c9bdc2ba647e32cb90af4ec81777669c2b245927c6` |
| `TUSB9261_RDX_flash.hex` | 1,992 bytes | `0b694cc2d7d95b7f851f9a192a42b0f25e10a008ef11c8c472fc1b0144a0cec0` |
| `TUSB9261_RDX.map` | 4,315 bytes | `83afdeab0974127f68be24bbd5ef008ad0ca31adcf031ba4fc854aa845b7f71e` |

The TI command-line Flash Burner 1.3.1.0 detected boot-loader instance 0,
reported its driver and flash present, and successfully programmed both the
dual-channel discovery image and the final channel-4-only image on 2026-08-24.
After reconnecting the discovery image with J7 open, pressing the cartridge
lock slider illuminated the green LED. After the final reduced image was
programmed and reconnected with J7 open, the operator confirmed that pressing
the cartridge lock slider still turns the green LED on correctly.

## Validation boundary

- Confirmed: original payload hash and the recovered addresses/mappings listed
  above.
- Confirmed: the focused source contract, toolchain resolver suite, exact
  four-object build scope, and recorded artifact hashes.
- Confirmed on hardware: the image was flashed successfully, the green LED
  responds to the eject button when GPIO1 is monitored, and GPIO1 is therefore
  not the cartridge lock-slider signal.
- Confirmed on hardware: monitoring GPIO2 produced no response from either the
  cartridge lock slider or eject button, so GPIO2 is not either tested switch.
- Confirmed on hardware: monitoring GPIO5 produced no response from the
  cartridge lock slider, so GPIO5 is not the operated switch.
- Confirmed: the dual-channel ADC diagnostic built and was programmed
  successfully with the recorded Windows artifact hash.
- Confirmed on hardware: pressing the cartridge lock slider asserts the
  MCP3008 channel-4 comparison and turns the green LED on. Channel 4 with
  threshold 650 is the operated switch signal.
- Confirmed on hardware: the final channel-4-only image preserves this correct
  behavior after removing the channel-2 discovery path.
- Not validated: the role of MCP3008 channel 2 or any lock/unlock mechanism
  behavior beyond reading the switch.
