# RDX Eject Test Validation

## Outcome

The `test/eject-sequence` branch replaces the production RDX runtime with a
standalone polling test. One new active-low GPIO1 button press drives the
recovered GPIO3/PWM1 motor pattern for exactly 2,000 milliseconds, stops the
motor, and then requires button release before it can trigger again.

The initial test firmware was flashed successfully on 2026-08-24, but pressing
the eject button did not move the motor. Comparison with the original PWM
initialization found that the test set continuous mode `0x02` without retaining
the required phase-1 output-level bit `0x10`. The corrected source keeps PWM1
at `0x10` while idle and explicitly writes `0x12` while running. After the
corrected image was programmed and the device was power-cycled with J7 open,
the operator confirmed that pressing the eject button operated the physical
motor. Full cartridge ejection was not separately reported.

## Original payload evidence

- Source payload:
  `TUSB9261_RDX_firmware_payload_v2.83_0x08000000.bin`
- SHA-256:
  `4854e7c7b61c286c2a969adb9670151a627c6c079aa71f98073bfb1bf8ff9f21`
- Logical-I/O table: `0x0800E408`, eight bytes per entry.
- Input reader: `0x08006174`.
- Logical-output setter: `0x0800506C`.
- Mechanism-output dispatcher: `0x08003BBC`.
- Mechanism-output reset helper: `0x08009FF8`.

The original table and code establish:

| Test signal | Original binding | Test register behavior |
| --- | --- | --- |
| Eject button | Logical input 1 at `0x0800E410`, physical GPIO1, polarity 0 | `GIOIN0` bit 1 low means pressed |
| Motor control | Logical output 10 at `0x0800E458`, physical GPIO3, polarity 1 | `GIOOUT0` bit 3 low starts the recovered pattern; high is the reset state |
| Motor PWM | Logical output 13 at `0x0800E470`, physical selector 13 | PWM1 continuous mode, 100 percent, 50-microsecond period |

At `0x08003BBC`, the applicable motor path drives logical output 10 to zero,
waits 50 microseconds, applies 100 percent to logical output 13 with a
50-microsecond period, and arms a 2,000-millisecond timer. The reset helper at
`0x08009FF8` disables the PWM-side outputs, waits 50 microseconds, and restores
logical output 10 to one.

## Implemented sequence

The standalone test performs these operations without condition checks:

1. Preload GPIO3 high before enabling output direction.
2. Configure GPIO1 as input and PWM1 disabled.
3. Wait for GPIO1 to become low.
4. Drive GPIO3 low.
5. Wait 50 microseconds.
6. Set PWM1 configuration to `0x12`, combining phase-1 output level `0x10`
   with continuous mode `0x02`, and set period and phase duration both to
   `clock_mhz * 50`, reproducing 100 percent over 50 microseconds.
7. Wait 2,000 milliseconds.
8. Set PWM1 phase duration to zero and disable its motor-related configuration
   bits.
9. Wait 50 microseconds and drive GPIO3 high.
10. Wait for GPIO1 to become high before returning to step 3.

There is intentionally no debounce, USB detach, ATA standby, cartridge
presence, cartridge identity, mechanism position, fault input, temperature,
retry, or extended timeout handling.

## Minimal link scope

The build adapter compiles only:

- `src/main.c`
- `src/startup.c`
- `src/exceptions_isr.asm`
- `src/intvecs.asm`

The branch has no production C modules, reconstructed core fragments,
production headers, absolute-symbol command file, or production semantic tests.
It retains only the focused eject source test and TI toolchain resolver test.

## Verification commands

```bash
~/.platformio/penv/bin/python -m unittest discover -s tests -v
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

The PlatformIO build uses TI ARM CGT, `ti_arm9_abi`, warnings-as-errors, and the
existing artifact names.

## Corrected Windows TI CGT build result

After the failed physical test, PWM1 initialization was corrected to preserve
the original phase-1 output-level bit. The Windows TI ARM CGT 5.2.5 build
completed successfully on 2026-08-24 and produced:

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 3,513 bytes | `69F179AFB1969224FCB309FDF0F1F8EC8AF7D14664A2F4AFA8A621A50FDC9104` |
| `TUSB9261_RDX.hex` | 1,400 bytes | `D3FE70AEDCF29FD619654483DA043FCEE7D27D6E9F33B97187217E8A863FE38B` |
| `TUSB9261_RDX_flash.hex` | 1,878 bytes | `7B08F0E07C8FFFE43221CAFFE372912FC39BB8163B79E1617131F61591F7D406` |

The continuous FlashBurner image covers `0x08000000` through `0x080002FF`.
Independent Intel HEX parsing confirmed that all 568 firmware bytes match the
normal linker output, all 200 added bytes are `0xFF`, and the output has no
address discontinuities.

The corrected image was programmed successfully to bootloader instance zero on
2026-08-24. FlashBurner returned exit code zero and `Programming Succeeded`.
After a USB power cycle with J7 open, the operator confirmed that an eject-button
press operated the motor on the physical Tandberg RDX unit.

All six eject-test source and behavior tests pass. The full ten-test suite has
one unrelated pre-existing Windows-only failure: the response-file test expects
forward slashes while `Path.relative_to()` produces backslashes on Windows.

## macOS TI CGT build result

The PlatformIO build completed successfully on 2026-08-15 with macOS TI ARM
CGT 5.2.9. The emitted build action listed exactly these inputs:

- `src/main.c`
- `src/startup.c`
- `src/exceptions_isr.asm`
- `src/intvecs.asm`
- `include/eject_test.h`
- `linker/tusb9260_link.cmd`

The resulting artifacts were:

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `TUSB9261_RDX.out` | 3,521 bytes | `a6b2357324d630dcc022713f04f36de46f7641e61f3f86f0f40ce46f08f3dc17` |
| `TUSB9261_RDX.hex` | 1,416 bytes | `c384a3e90ccd7dadbf3dfe274e42cb0eef7e999bc78dfa70c7c81f99d90acbbf` |
| `TUSB9261_RDX.map` | 4,292 bytes | `6f7be5909339bd06d0129057809ebb57d9c5b8c09d9e11b360fd8dba5e9340c3` |

The map allocates `.intvecs` from `intvecs.obj`, `.text` from `main.obj`,
`startup.obj`, and `exceptions_isr.obj`, and four bytes of `.bss` from
`main.obj`. It contains no production AHCI, USB, SCSI, RDX core, SPI, MWW, UMS,
flash, watchdog, or PWM module objects.

## Validation boundary

- Confirmed: original payload hash and the addresses/values listed above.
- Confirmed: source contract and exact build input allowlist when the automated
  suite passes.
- Confirmed: TI 5.2.9 compilation/link/HEX conversion, the artifact hashes
  above, and the four-object link-map contents.
- Confirmed: FlashBurner programming and J7 ROM-bootloader recovery on the
  physical Tandberg board.
- Failed: the initial physical test with PWM1 running at configuration `0x02`;
  pressing the eject button produced no motor movement.
- Confirmed: the corrected image was accepted by FlashBurner and programmed to
  the physical device.
- Confirmed: after a USB power cycle, pressing the eject button operated the
  motor with the corrected `0x10`/`0x12` PWM configuration.
- Not validated: full cartridge travel, successful cartridge ejection, or any
  of the production mechanism and safety checks omitted from this test build.
