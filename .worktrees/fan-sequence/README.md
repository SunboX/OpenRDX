# TUSB9261 RDX Fan Test Firmware

This branch is standalone fan-test firmware. It does not provide USB mass
storage, SATA/AHCI, SCSI, flash management, cartridge checks, mechanism control,
temperature monitoring, or the normal RDX runtime.

Each new active-low GPIO1 eject-button press drives active-high PWM0 at
100-percent duty for one second, turns it off, and waits for button release.

## Hardware validation

The firmware was built with Windows TI ARM CGT 5.2.5, programmed through the
TUSB9261 ROM bootloader, and tested on the physical Tandberg RDX device on
2026-08-24. After a USB power cycle with the J7 recovery strap open, pressing
the eject button successfully operated the fan. This validates the GPIO1
button input and PWM0 fan-output path on the tested unit. The one-second
interval is enforced by the firmware but was not independently measured with
test equipment.

## Build

PlatformIO only orchestrates the legacy TI ARM Code Generation Tools build. The
firmware keeps the required `ti_arm9_abi`, TI assembly/linker syntax,
warnings-as-errors policy, and artifact names.

Windows, with TI ARM CGT 5.2.5 installed at `C:\ti\ti-cgt-arm_5.2.5`:

```powershell
C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
```

macOS, with TI ARM CGT 5.2.9 installed at `~/ti/ti-cgt-arm_5.2.9` and its
extensionless tool wrappers available (Apple Silicon requires Rosetta):

```bash
~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

To use another compatible local TI CGT installation without editing project
configuration:

```bash
TI_CGT_ROOT=/alternate/path/to/ti-cgt-arm_5.2.9 \
  ~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt
```

The build creates these artifacts under `.pio/build/tusb9261_ti_cgt/`:

- `TUSB9261_RDX.out`
- `TUSB9261_RDX.map`
- `TUSB9261_RDX.hex`

## Safety and validation

The tested programming and J7 ROM-bootloader recovery procedure is documented
in [TUSB9261 flashing and recovery](docs/TUSB9261_FLASHING_PROCEDURE.md).

Recovered hardware evidence, source-contract verification, and the boundary of
the build, programming, and physical evidence are documented in
[Fan Test Validation](docs/FAN_TEST_VALIDATION.md).
