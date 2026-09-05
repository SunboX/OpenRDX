# Fan Test Firmware Validation

## Purpose and evidence boundary

This branch contains a standalone TUSB9261 RDX fan-output test. It was derived
from the recovered v2.83 payload; the original binary is the hardware-mapping
oracle, not a claim that this branch reproduces the original runtime.

- Payload: `TUSB9261_RDX_firmware_payload_v2.83_0x08000000.bin`
- SHA-256:
  `4854e7c7b61c286c2a969adb9670151a627c6c079aa71f98073bfb1bf8ff9f21`
- Logical I/O table: `0x0800E408`
- Runtime hardware-configuration tables: `0x0800E398` and `0x0800E3D0`
- Logical-input reader: `0x08006174`
- Logical-output helper: `0x0800B0C0`
- Temperature state machine: `0x080038CC`, reaching the output helper via
  `0x0800BEC0`

Logical input 1 occupies `0x0800E410`, maps to physical GPIO1, and has
polarity byte zero. The recovered input reader reads `GIOIN0`, so a low
physical level asserts the eject button. Startup selects logical outputs
`(0, 4, 12, 15, 11, 14, 16)`; in both runtime configuration tables, logical
output 15 maps to physical selector 12 with polarity byte one. The output
helper routes selector 12 to PWM0. The production thermal path passes logical
output 15, a requested duty, and a 16,000-microsecond period to that helper.
This is the firmware evidence that the active-high PWM0 path controls the fan.

## Implemented fan-test behavior

The reduced firmware drives PWM0 directly rather than retaining the production
logical-I/O and thermal-control implementations. GPIO1 is the sole button
input and is active low. On each new press, PWM0 runs active high at
100-percent duty using the recovered 16,000-microsecond period for exactly
1,000 milliseconds. It then sets duty to zero, disables continuous output, and
waits for GPIO1 to return high before accepting another press. A held button
therefore cannot retrigger the pulse.

The TI PWM architecture counts one period as `PER + 1` peripheral clocks and
uses `PH1D` for the first-phase clock count. The firmware therefore keeps
`fan_pwm_period_ticks` as the requested clock count, writes
`PWM0_PER = fan_pwm_period_ticks - 1UL`, and writes
`PWM0_PH1D = fan_pwm_period_ticks` while the fan runs. Since `PH1D` is then
equal to `PER + 1`, the output stays high for the complete period. Writing the
unadjusted tick count to both registers would instead make the period one clock
too long and leave one low clock in each nominally 100-percent-duty cycle.

No debounce or error/retry behavior is added. The branch intentionally excludes
temperature reads and thresholds, SMART, USB/host coordination, SATA/AHCI,
SCSI, BOT/UAS, HID, flash management, cartridge presence, mechanism state,
watchdog handling, LEDs, and the normal RDX runtime.

## Automated verification

The source-contract commands for this branch are:

```sh
~/.platformio/penv/bin/python -m unittest tests.test_fan_test_firmware -v
~/.platformio/penv/bin/python -m unittest discover -s tests -v
```

The focused contract verifies the GPIO1 active-low mapping, PWM0 registers,
the `PER + 1` period arithmetic and full-period `PH1D` count,
16,000-microsecond period, 1,000-millisecond pulse, stop ordering, held-button
release gate, source allowlist, and removal of production paths. The full
surviving suite additionally verifies Windows, macOS, and `TI_CGT_ROOT` TI CGT
path resolution.

## Build and map evidence

### Windows build, programming, and hardware result

The Windows TI ARM CGT 5.2.5 build completed successfully on 2026-08-24 and
produced:

| Artifact | Size (bytes) | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 3,259 | `A92AC01ABFDDA9CDC733CF7E530F7CCF6EBB0296DD1E7251B9C69AC366DDD629` |
| `TUSB9261_RDX.hex` | 1,169 | `355EB805A340C9434144FBF8E33BE83777DB5539B176908CDC25ACCBFD6C0EE2` |
| `TUSB9261_RDX_flash.hex` | 1,631 | `49E8F330249F3F7634671AB15B2E94A2F0340EE550CE7318A7FFE90946416ED8` |

The continuous FlashBurner image contains 664 bytes covering `0x08000000`
through `0x08000297`. Independent Intel HEX parsing confirmed that all 472
linked firmware bytes were preserved, all 192 added gap bytes were `0xFF`, and
the output contained no address discontinuities.

An elevated FlashBurner `/p` probe reported one bootloader instance, the driver
present, and the SPI flash present. FlashBurner then programmed only instance
zero and returned exit code zero with `Programming Succeeded`. After a USB
power cycle with J7 open, the operator confirmed that pressing the eject button
successfully operated the physical fan. The firmware-defined one-second pulse
duration was not independently measured with test equipment.

All seven focused fan tests passed. The full eleven-test suite retained one
unrelated pre-existing Windows-only failure: the response-file test expects
forward slashes while `Path.relative_to()` produces backslashes on Windows.

### macOS build and map evidence

A clean local rebuild completed successfully on 2026-08-15 with:

```sh
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt -t clean
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

The build used the configured macOS default
`/Users/afiedler/ti/ti-cgt-arm_5.2.9` and TI ARM Linker PC v5.2.9. No
`TI_CGT_ROOT` override was supplied. This confirms that this checkout builds
with that host toolchain; it does not claim byte equivalence with the validated
Windows TI CGT 5.2.5 output.

The final verification build was linked at `Sat Aug 15 16:38:57 2026` and
generated these artifacts:

| Artifact | Size (bytes) | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 3,259 | `c4e836d60d2244846903870e348c81a7b727f6c39970f367043023f81653f381` |
| `TUSB9261_RDX.hex` | 1,169 | `355eb805a340c9434144fbf8e33be83777db5539b176908cdc25accbfd6c0ee2` |
| `TUSB9261_RDX.map` | 4,154 | `2aeec41795c8dfd1d9b56a4f53a93b579e08b6f2aace4b3f65f856cd013a9a13` |

TI CGT embeds the link time in the COFF `.out` header and prints it in the
`.map` file. Their hashes therefore change on each clean rebuild even when the
loadable firmware bytes do not. Three consecutive clean verification builds
produced the same `.hex` hash shown above; the `.out` and `.map` hashes record
the specifically timestamped build identified here.

The map's `SECTION ALLOCATION MAP` contains exactly these input objects:
`main.obj`, `startup.obj`, `exceptions_isr.obj`, and `intvecs.obj`. Its global
symbol table contains the fan-test entry points (`_main` and
`_fan_test_system_init`), reset/exception symbols, and linker/runtime symbols
only. Inspection of those two map sections found no USB, AHCI, SCSI, flash,
temperature, mechanism, recovered-core, or production-PWM object or global
symbol. The check deliberately excluded the map's output-file header: the
ordinary project filename `TUSB9261_RDX.out` contains the letters `USB` but is
not an input object or firmware symbol.

The original macOS build evidence above remains source/build validation only.
Physical fan operation and FlashBurner programming were subsequently validated
with the Windows TI ARM CGT 5.2.5 image recorded in the preceding section.
