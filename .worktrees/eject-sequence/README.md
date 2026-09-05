# TUSB9261 RDX Eject Test Firmware

This branch contains standalone mechanism test firmware. It does **not**
provide USB mass storage, SATA/AHCI, SCSI, flash management, cartridge checks,
or the normal RDX runtime.

Each new active-low GPIO1 eject-button press applies the motor pattern recovered
from the original RDX v2.83 payload:

1. Drive GPIO3 low.
2. Wait 50 microseconds.
3. Run PWM1 at 100 percent with a 50-microsecond period.
4. Keep the motor active for 2,000 milliseconds.
5. Disable PWM1.
6. Wait 50 microseconds.
7. Restore GPIO3 high.

The firmware then waits for the button to be released before another press can
trigger the motor. There is no debounce, host coordination, cartridge check,
mechanism-position check, retry, or extended timeout.

PWM1 is held at configuration `0x10` while idle and changed to `0x12` while
running. The phase-1 output-level bit `0x10` is required in addition to the
continuous-mode bit `0x02`; omitting it produced no physical motor movement in
the first hardware test even though programming succeeded.

## Hardware validation

The corrected firmware was built with Windows TI ARM CGT 5.2.5, programmed to
the Tandberg RDX through the TUSB9261 ROM bootloader, and tested on the physical
device on 2026-08-24. After a USB power cycle with the J7 recovery strap open,
pressing the eject button successfully operated the motor. This validates the
GPIO1 button input and corrected GPIO3/PWM1 motor-output path on the tested
unit. Full cartridge travel and the omitted production safety/state checks are
outside this focused test.

## Build

The default PlatformIO environment remains `tusb9261_ti_cgt`. PlatformIO
orchestrates the build, while TI ARM CGT performs compilation, assembly,
linking, and HEX conversion with the legacy `ti_arm9_abi` ABI.

macOS:

```bash
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

The default macOS compiler root is `~/ti/ti-cgt-arm_5.2.9`. Apple Silicon needs
Rosetta for the Intel compiler binaries.

Windows:

```bat
C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
```

The Windows default remains `C:\ti\ti-cgt-arm_5.2.5` with `armcl.exe` and
`armhex.exe`.

Set `TI_CGT_ROOT` for a temporary toolchain-root override. Do not substitute
GCC, Clang, or a newer TI CGT release; the firmware still requires
`ti_arm9_abi` COFF support.

Generated artifacts are written below `.pio/build/tusb9261_ti_cgt/`:

- `TUSB9261_RDX.out`
- `TUSB9261_RDX.map`
- `TUSB9261_RDX.hex`

## Tests

```bash
~/.platformio/penv/bin/python -m unittest discover -s tests -v
```

The source contract verifies the button and motor mapping, exact timing,
start/stop ordering, release-to-rearm behavior, and minimal build allowlist.
Tool-resolution tests preserve the Windows, macOS, and `TI_CGT_ROOT` behavior.

## Scope and safety

Only `main.c`, `startup.c`, the two TI vector assembly sources, and the minimal
register header are part of the firmware. Production USB, SATA/AHCI, SCSI,
flash, thermal, LED-controller, watchdog, cartridge-state, and recovered-core
code has been removed from this branch.

The tested programming and J7 ROM-bootloader recovery procedure is documented
in [TUSB9261 flashing and recovery](docs/TUSB9261_FLASHING_PROCEDURE.md). See
[Eject test validation](docs/EJECT_TEST_VALIDATION.md) for the binary evidence,
artifact hashes, physical result, and remaining validation boundary.
