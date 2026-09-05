# TUSB9261 OpenRDX flashing procedure

## Purpose

This procedure programs an OpenRDX application through the TUSB9261 ROM
bootloader and an external M25PE20-compatible SPI flash. It covers both normal
bootloader enumeration and the validated `J7` boot-selection method used when
the installed application does not expose the bootloader.

For an in-service RDX device that still responds to RDX Manager commands, use
the packaged updater and [USB update procedure](OPENRDX_USB_UPDATE.md)
from `dist` instead. The direct method below is intended for development,
blank-flash programming, and repair by an operator who can safely access the
PCB.

No build or test command in this repository writes to hardware. Programming is
always a separate, deliberate operator action.

## Safety requirements

- Disconnect USB and every other power source before fitting or moving a PCB
  bridge, or before making continuity measurements.
- Never make resistance or continuity measurements on a powered board.
- Confirm the `J7` pad connections on the unpowered board before using the
  bridge. Board revisions may differ.
- Do not short the 3.3 V pad to ground. The round pad of nearby `J14` is ground
  and must not be used for this procedure.
- Use a secured bridge that cannot slip onto adjacent components. Do not hold
  loose tweezers on the pads while connecting USB.
- Remove the `J7` bridge after the ROM bootloader enumerates and before probing
  or programming the SPI flash.
- Require exactly one selected bootloader instance. Do not program when target
  identity or flash presence is ambiguous.
- Do not disconnect power, move the bridge, or disturb USB while a programming
  transaction is running.

## Required software

- Windows with administrator access.
- Texas Instruments TUSB926x FlashBurner and its device driver.
- TI ARM Code Generation Tools 5.2.5, as configured by `platformio.ini`.
- PlatformIO in the repository's configured Python environment.

The TUSB9261 ROM bootloader enumerates as USB VID `0451`, PID `926B`. A usable
target must expose both its HID interface and the **TUSB9260 Flash Burner
Driver** without Device Manager warnings.

FlashBurner accepts an application `.bin` or Intel HEX `.hex` file through its
normal **Program** operation. **Program Full Binary Image** expects an already
formatted complete SPI image and must not be used with the PlatformIO
application HEX file.

## Build and validate OpenRDX

From the repository root, run:

```powershell
C:\Users\andre\.platformio\penv\Scripts\python.exe -m unittest discover -s tests -v
C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
```

The build produces these files under `.pio/build/tusb9261_ti_cgt`:

| File | Purpose |
| --- | --- |
| `TUSB9261_RDX.out` | TI linker output and symbol information |
| `TUSB9261_RDX.hex` | Sparse Intel HEX application image |
| `TUSB9261_RDX_flash.hex` | Continuous Intel HEX image for FlashBurner |

FlashBurner 1.3.1 rejects discontinuous Intel HEX input. The build adapter
therefore asks TI `armhex` to create `TUSB9261_RDX_flash.hex`, filling linker
gaps with `0xFF`. Use that continuous file for direct programming.

Before proceeding, record a SHA-256 digest for the exact file to be written:

```powershell
Get-FileHash -Algorithm SHA256 .pio\build\tusb9261_ti_cgt\TUSB9261_RDX_flash.hex
```

## Enter the ROM bootloader

If Device Manager already shows VID `0451`, PID `926B` and the Flash Burner
driver is healthy, leave the board unchanged and continue with the read-only
probe.

If an installed application starts instead, use this board-level sequence:

1. Disconnect every source of power, including USB.
2. On the unpowered board, verify that the `J7` round pad has direct continuity
   to U10 pin 1 (`CE#`) and that the `J7` square pad has direct continuity to
   U10 pin 8 (`VCC`). Stop if either measurement differs.
3. Securely bridge the two `J7` pads.
4. Connect USB with the bridge already present. Holding flash `CE#` high during
   reset prevents application loading.
5. Wait for the TUSB9261 ROM bootloader to enumerate as VID `0451`, PID `926B`.
6. Keep USB connected and carefully remove the `J7` bridge. The controller
   remains in its ROM bootloader while the SPI flash becomes selectable again.
7. Verify once more that the bridge is fully open and cannot touch nearby
   components.

The validated `J7` nets are:

| Pad | Net | Function |
| --- | --- | --- |
| Round | U10 pin 1, `CE#` | Active-low external-flash select |
| Square | U10 pin 8, `VCC` | 3.3 V high rail, also tied to `HOLD#` and `WP#` |

There is an approximately 1 kOhm series path between controller `SPI_CS0` and
the flash-select net. Nearby `R21` is a separate 10 kOhm component and requires
no modification.

## Read-only target probe

Run the command-line tool from an elevated PowerShell session:

```powershell
& 'C:\Program Files (x86)\Texas Instruments Inc\Command Line FlashBurner\TUSB926x_CL_Burner.exe' /p
```

Proceed only when the output identifies exactly one intended instance and
reports:

```text
Flash Burner driver: Yes
Flash Present: True
```

If either field differs, disconnect power and correct the driver, bridge, or
hardware condition before attempting a write.

## Program one selected instance

Recheck the SHA-256 digest and the absolute image path. Then run the elevated
command with the selected instance number and continuous HEX file:

```powershell
& 'C:\Program Files (x86)\Texas Instruments Inc\Command Line FlashBurner\TUSB926x_CL_Burner.exe' `
    /s 0 `
    /f 'D:\Projekte\OpenRDX\TUSB9261_RDX_PlatformIO\.pio\build\tusb9261_ti_cgt\TUSB9261_RDX_flash.hex'
```

Alternatively, use the GUI's normal **Program** operation with the same
continuous HEX file. Leave **Program Full Binary Image** disabled.

Wait for an explicit `Programming Succeeded` or failure result. FlashBurner's
ROM path acknowledges the transfer but does not provide a complete read-back
verification, so a successful message is not a substitute for post-boot
behavioral checks.

## Reboot and verify

1. After programming finishes, disconnect USB.
2. Confirm that `J7` is open.
3. Reconnect USB and allow OpenRDX to start.
4. Confirm the expected USB identity and removable-media interface.
5. With no cartridge inserted, confirm that the eject-button LED is steady
   amber and the cartridge LED is off.
6. Insert a cartridge and confirm that the cartridge LED becomes green before
   entering its activity blink state while the eject-button LED remains amber.
7. Confirm media discovery and read-only commands before attempting writes.
8. Record the programmed image digest, device identity, and verification
   results.

If OpenRDX does not start, disconnect power and repeat the `J7` bootloader-entry
sequence. Do not change bridges or probes while the board is powered.
