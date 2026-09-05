# TUSB9261 RDX Lock-Slider Test Firmware

The `lock_unlock-slider-switch` branch contains standalone sensor test firmware.
It is not the production RDX mass-storage firmware.

Its complete runtime behavior is to sample the hardware-confirmed lock-slider
signal on MCP3008 channel 4 continuously. A value above the production
threshold of 650 turns the green LED on. The amber LED remains off. Sampling
uses the recovered three-word SPI transaction.

This final channel-4-only image was built, flashed, and physically tested on
the Tandberg RDX hardware on 2026-08-24. Pressing the cartridge lock slider
turns the green LED on and releasing it turns the LED off, as intended.

The first hardware build incorrectly monitored GPIO1. Testing on 2026-08-24
showed that pressing the eject button, not the cartridge lock slider, turned
the green LED on. This physically identifies GPIO1 as the eject-button input
and disproves the earlier slider assumption. A second build monitored GPIO2,
matching static logical input 3, but neither switch affected the LED. The
production startup path instead remaps logical input 4—the debounced cartridge
input—to active-low GPIO5. A third hardware build monitored GPIO5, but the lock
slider still did not affect either LED. The dual-channel ADC diagnostic was
then flashed and tested successfully on 2026-08-24: pressing the cartridge
lock slider turned the green LED on. This confirmed MCP3008 channel 4 with the
production threshold of 650 as the operated lock-slider signal. The final
test firmware therefore reads only channel 4 and drives only the green LED.

There is no eject behavior, motor action, USB, SATA/AHCI, cartridge state
machine, or other production service.

## Build

The default PlatformIO environment remains `tusb9261_ti_cgt`. PlatformIO
orchestrates the build while TI ARM CGT performs compilation, assembly,
linking, and HEX conversion with the legacy `ti_arm9_abi` ABI.

macOS:

```bash
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

The default macOS compiler root is `~/ti/ti-cgt-arm_5.2.9`. Apple Silicon
requires Rosetta for these Intel compiler binaries.

Windows:

```bat
C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
```

The Windows default remains `C:\ti\ti-cgt-arm_5.2.5`, with `armcl.exe` and
`armhex.exe`.

Set `TI_CGT_ROOT` for a temporary toolchain-root override. Do not substitute
GCC, Clang, or a newer TI CGT release; this firmware requires
`ti_arm9_abi` COFF support.

Generated artifacts are written below `.pio/build/tusb9261_ti_cgt/`:

- `TUSB9261_RDX.out`
- `TUSB9261_RDX.map`
- `TUSB9261_RDX.hex`

## Tests

```bash
~/.platformio/penv/bin/python -m unittest discover -s tests -v
```

The source contract verifies the recovered MCP3008 transaction, confirmed ADC
channel and threshold, green LED mapping, startup-off state, and exact
four-source build allowlist. Tool-resolution tests preserve Windows, macOS,
and `TI_CGT_ROOT` behavior.

The focused six-test suite and a clean Windows PlatformIO build passed with TI
ARM CGT 5.2.5 on 2026-08-24.

## Final Windows artifacts

The final image ends at `0x0800032C`. Because FlashBurner 1.3.1 requires
continuous Intel HEX addresses, `TUSB9261_RDX_flash.hex` covers
`0x08000000` through `0x0800032B`. It preserves all 620 emitted firmware bytes
and fills only the 192-byte linker gap with `0xFF`.

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 3,534 bytes | `e1c1099e2bbc3f65a79195d8d7643b1a317ceab2668b1271cd516f26520d6e08` |
| `TUSB9261_RDX.hex` | 1,530 bytes | `822639b27325568f4c07c5c9bdc2ba647e32cb90af4ec81777669c2b245927c6` |
| `TUSB9261_RDX_flash.hex` | 1,992 bytes | `0b694cc2d7d95b7f851f9a192a42b0f25e10a008ef11c8c472fc1b0144a0cec0` |
| `TUSB9261_RDX.map` | 4,315 bytes | `83afdeab0974127f68be24bbd5ef008ad0ca31adcf031ba4fc854aa845b7f71e` |

## Flashing and bootloader recovery

The board can be forced into the TUSB9261 ROM bootloader with unpopulated
jumper `J7`. Measurements established that its round pad is connected to U10
flash `CE#`, while its square pad is the flash 3.3 V/high net. With all power
removed, bridge J7, connect USB, wait for VID `0451`/PID `926B` to enumerate,
then remove the bridge while USB remains connected. This leaves the bootloader
running while making the SPI flash selectable for programming.

TI command-line FlashBurner 1.3.1.0 detected exactly one healthy bootloader
instance with its driver loaded and flash present. Programming instance zero
with the independently verified continuous HEX completed with `Programming
Succeeded`. After reconnecting with J7 open, the final channel-4-only firmware
was confirmed to respond correctly to the physical lock slider.

Do not measure resistance on a powered board or leave J7 bridged during flash
programming. Full installation details, measurements, hashes, failure history,
and the validated recovery sequence are recorded in
[the flashing procedure](docs/TUSB9261_FLASHING_PROCEDURE.md).

## Scope and safety

The firmware contains only `main.c`, `startup.c`, the two required TI vector
assembly sources, and the minimal register header. It implements only GIO LED
control and the recovered MCP3008 SPI read. Production USB, SATA/AHCI, SCSI,
flash, thermal, motor/PWM, watchdog, cartridge-state, and reconstructed core
code is absent.

See [lock-slider test validation](docs/SWITCH_TEST_VALIDATION.md) for the
firmware evidence, candidate-signal history, build results, and validation
boundary.
