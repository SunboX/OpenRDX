# TUSB9261 RDX flashing and ROM-bootloader recovery investigation

## Scope and status

This document records the investigation and host changes performed on
2026-08-24 to build the `test/led-sequence` branch and determine a safe,
repeatable way to program an empty SPI flash on a Tandberg RDX USB device.

The firmware built successfully, the official Texas Instruments FlashBurner
and driver were installed, and the attached device was programmed successfully.
The bootloader USB interfaces disappeared after programming, as expected for
the standalone LED firmware. The operator then confirmed that the physical LEDs
work as expected through the complete nine-state sequence.

It also records the subsequent board-level investigation needed to return the
same device to its ROM bootloader after the LED-only firmware had been written.
That firmware intentionally has no USB stack, so the normal bootloader USB
interfaces were no longer available. Measurements on the Tandberg PCB strongly
indicated that unpopulated jumper `J7` was a practical flash-disable strap for
recovery. This was subsequently confirmed on the physical device: booting with
J7 bridged exposed the ROM bootloader, and the bootloader remained available
after the bridge was removed without cycling power. FlashBurner then detected
the SPI flash and successfully programmed the eject-button motor-pulse build.

## Authoritative references

- Texas Instruments TUSB9261 product page:
  <https://www.ti.com/product/TUSB9261>
- Texas Instruments TUSB9261 data sheet:
  <https://www.ti.com/lit/ds/symlink/tusb9261.pdf>
- Texas Instruments TUSB9260 Flash Burner User Guide, revision D (SLLU125D):
  <https://www.ti.com/lit/pdf/sllu125>
- Texas Instruments TUSB9261 implementation guide (SLLA315E):
  <https://www.ti.com/lit/an/slla315e/slla315e.pdf>
- Texas Instruments FlashBurner download (SLLC414, version 2.10.0):
  <https://www.ti.com/tool/download/SLLC414/01.00.00.0E>
- Direct TI-hosted SLLC414 archive used during this investigation:
  <https://dr-download.ti.com/software-development/software-programming-tool/MD-E9x5D6YrUj/01.00.00.0E/sllc414e.zip>
- TI E2E support thread containing the command-line FlashBurner 1.3.1 package
  and usage instructions supplied by a TI employee:
  <https://e2e.ti.com/support/interface-group/interface/f/interface-forum/713036/tusb9261-flash-burner-source-code-or-example-code-for-production>
- TI support answer confirming that the TUSB9261 can program its external SPI
  flash by translating USB commands to SPI commands:
  <https://e2e.ti.com/support/interface-group/interface/f/interface-forum/1508826/tusb9261-flashing-procedure-for-spi-flash>

## Findings

The TUSB9261 contains a ROM bootloader. Application firmware is stored in an
external SPI flash and is loaded by the TUSB9261 at startup. TI documents USB
firmware update using its FlashBurner application.

The attached device enumerated with USB VID `0x0451` and PID `0x926B`. Section
3.1 of SLLU125D states that this VID/PID identifies an unprogrammed device that
is ready for programming. This agrees with the reported empty SPI flash.

Before installing the TI package, Windows exposed these interfaces:

| Interface | State | Detail |
| --- | --- | --- |
| USB composite device | OK | `USB\\VID_0451&PID_926B\\TUSB9260BL01` |
| HID interface | OK | `USB\\VID_0451&PID_926B&MI_00...` |
| TUSB9260 Boot Loader | Error | Code 28, no driver, `MI_01` |

FlashBurner accepts an application `.bin` or Intel HEX `.hex` file through its
normal **Program** operation. It adds the USB descriptors, checksums, and SPI
image formatting required by the ROM bootloader. **Program Full Binary Image**
is a different operation intended only for an already formatted full SPI image
previously exported by FlashBurner. The PlatformIO-generated HEX file must
therefore be supplied to **Program**, not **Program Full Binary Image**.

The command-line FlashBurner has the same normal firmware path through its
`/f <Firmware File>` option. Its `/s <Instance Number>` option limits a write to
one enumerated bootloader instance, and `/p` performs a read-only listing of
available bootloader instances.

FlashBurner 1.3.1 rejects Intel HEX input containing discontinuous address
records. The default `armhex` artifact has a linker gap from `0x08000040` through
`0x080000FF`, so the first programming command was rejected with:

```text
Invalid File. Memory addresses are not continuous. (Line 4).
```

This happened during host-side file parsing; no flash write occurred. TI's HEX
conversion documentation identifies `--image` plus a `ROMS` directive as the
supported way to produce a continuous image for programmers that do not accept
address discontinuities. A second HEX file was therefore generated from the
same linked `.out`, covering `0x08000000` through `0x0800038F` and filling only
the gaps with `0xFF`.

SLLU125D also establishes these important limitations:

- Administrator rights are required on Windows Vista and later.
- A successful GUI message means the device acknowledged receipt of the data;
  the FlashBurner bootloader path does not implement read-back verification.
- On an evaluation board with corrupt firmware, TI instructs the user to boot
  with SPI disabled, enumerate the ROM bootloader, re-enable SPI, and then
  program. The Tandberg RDX board already enumerated in blank-flash bootloader
  mode, so no board jumper was changed during this investigation.
- The SPI flash must support Write Enable `0x06`, Read Data `0x03`, Page Program
  `0x02`, and Chip Erase `0xC7`. A stalled write-protected flash is recovered by
  resetting or power-cycling the TUSB9261.
- After programming, TI recommends a reboot. For this standalone LED build,
  loss of USB enumeration after reboot is expected because USB, SATA, SCSI,
  HID, and flash runtime code are deliberately omitted.

## Tandberg PCB bootloader-recovery investigation

### Components and candidate jumpers

Inspection of the board photographs identified `U10` as the external
STMicroelectronics M25PE20 SPI flash. The white triangle beside `U10` marks pin
1, `S#`/chip enable (`CE#`). The relevant M25PE20 pins are:

| U10 pin | Signal | Function |
| ---: | --- | --- |
| 1 | `S#` / `CE#` | Active-low SPI flash select |
| 3 | `W#` / `WP#` | Active-low write protect |
| 4 | `VSS` | Ground |
| 7 | `HOLD#` | Active-low hold input |
| 8 | `VCC` | Flash supply |

The close-up also showed two unpopulated two-pad jumpers, `J7` and `J14`.
Resistance and continuity measurements were made only with the board
unpowered. The first set of in-circuit measurements was:

| Pad | Resistance to U10 pin 1 | Resistance to board ground |
| --- | ---: | ---: |
| J7 square | 4.087 kOhm | 1.0495 kOhm |
| J7 round | 0.26 Ohm | 6.578 kOhm |
| J14 square | 35.87 kOhm | 28.36 kOhm |
| J14 round | 6.602 kOhm | 0.23 Ohm |

These are in-circuit readings, so parallel semiconductor and resistor paths
can make resistance values differ depending on probe direction and meter test
voltage. The roughly 1 kOhm reading from `J7` square to ground did not mean that
the flash uses a separate ground. U10 pin 4 is the common board ground; direct
continuity, rather than a resistance through the unpowered circuit, is the
appropriate ground test.

Follow-up continuity measurements established the useful J7 connections:

| Measurement | Result | Conclusion |
| --- | ---: | --- |
| J7 round to U10 pin 1 (`CE#`) | 0.26 Ohm | Same net |
| J7 square to U10 pin 8 (`VCC`) | 0.19 Ohm | Same 3.3 V supply net |
| J7 square to U10 pin 7 (`HOLD#`) | 0.20 Ohm | `HOLD#` is tied high |
| J7 square to U10 pin 3 (`WP#`) | 0.26 Ohm | `WP#` is tied high |
| U4 `SPI_CS0` to J7 round | 0.9983 kOhm | Deliberate approximately 1 kOhm series path between the controller and flash `CE#` |

Consequently, the `J7` round pad is the flash-select net and the `J7` square
pad is the flash 3.3 V/high net. Shorting the two pads holds `CE#` high, keeps
the external flash deselected during reset, and prevents the TUSB9261 from
loading the installed application. The controller can then remain in its ROM
bootloader and enumerate as USB VID `0451`, PID `926B`. The approximately 1
kOhm series path limits contention current if the controller drives `SPI_CS0`
low while the strap holds the flash end high; at 3.3 V the upper-bound estimate
is approximately 3.3 mA.

This conclusion is based on direct continuity measurements, the measured
series resistance to the TUSB9261 `SPI_CS0` pin, the documented active-low
flash-select behavior, and successful ROM-bootloader enumeration on the
physical device. The exact factory name or intended production use of `J7` has
not been confirmed from a Tandberg schematic, which was not available.

`J14` round is ground, but the function of `J14` square remains unidentified.
It is not needed for SPI-disable recovery and must not be shorted merely to test
bootloader entry.

### R21 exclusion

The nearby resistor is labelled `R21`, not `R210`. Measurements were:

| Measurement | Resistance |
| --- | ---: |
| J7 round to left end of R21 | 6.633 kOhm |
| J7 round to right end of R21 | 24.26 kOhm |
| Directly across R21 | 10.012 kOhm |

These readings show that `R21` is not the approximately 1 kOhm series element
between U4 `SPI_CS0` and the J7-round/U10-`CE#` net. No modification to `R21`
is required for bootloader recovery.

### Validated J7 bootloader-entry procedure

1. Disconnect every source of power from the RDX, including USB.
2. Confirm with the unpowered board that `J7` round still has direct continuity
   to U10 pin 1 and `J7` square still has direct continuity to U10 pin 8. This
   guards against pad-orientation mistakes.
3. Securely bridge the two `J7` pads. Use a controlled jumper or soldered
   temporary lead that cannot slip onto adjacent components; do not rely on
   loose tweezers while connecting USB.
4. Connect USB with the J7 bridge already present. The forced-high flash
   `CE#` should prevent application loading.
5. Wait for Windows to enumerate `TUSB9260 Boot Loader`, VID `0451`, PID
   `926B`. Do not start programming unless that identity is present.
6. Leave USB connected so the TUSB9261 remains in the ROM bootloader, then
   carefully remove the J7 bridge without resetting or power-cycling the
   board. Removing the bridge makes the SPI flash selectable again for the
   programming transaction.
7. Run FlashBurner's read-only `/p` probe and require exactly one healthy
   bootloader instance with `Flash Burner driver: Yes` and
   `Flash Present: True`.
8. Program only that instance with the independently verified continuous HEX
   image. Do not touch J7, disconnect USB, or power-cycle while programming is
   in progress.
9. After FlashBurner reports success, remove power, ensure J7 is open, and then
   reconnect the device to boot the newly programmed firmware.

Never make resistance or continuity measurements on the powered board. Avoid
shorting the 3.3 V J7 square pad to ground, including J14 round. If the
bootloader does not enumerate, disconnect power before moving probes or
changing the bridge.

## Repository and device preparation performed

1. Fetched all Git remotes and checked out the local tracking branch
   `test/led-sequence` from `origin/test/led-sequence`.
2. Confirmed that the working tree was clean.
3. Detected the attached TUSB926x ROM bootloader as `VID_0451&PID_926B`.
4. Built with the repository-required TI ARM Code Generation Tools 5.2.5:

   ```powershell
   C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt
   ```

5. Ran the Python unit suite:

   ```powershell
   C:\Users\andre\.platformio\penv\Scripts\python.exe -m unittest discover -s tests -v
   ```

   All LED mapping, active-low initialization, sequence, timing, source-scope,
   and toolchain-selection tests passed. One unrelated Windows-only path
   separator assertion failed because the implementation returned backslashes
   while the test expected forward slashes.

6. Downloaded `sllc414e.zip` from the direct TI-hosted URL above to
   `%TEMP%\codex-tusb926x-flashburner`.
7. Verified the downloaded archive SHA-256:
   `FEE594A2DBC32320DDFB0770D844B33AEE9092BEA6DBE691C47B69E9C1EBB208`.
8. Extracted `TUSB926x_Burner_2.10.0.exe`; its SHA-256 was
   `EF86B608D0ECE9D3EB790D1784FC8A0B338A0B6868BD9D680FEB41AFDD9BE047`.
9. Verified the installer Authenticode signature as valid and signed by
   `Texas Instruments, Inc.` with a trusted timestamp.
10. Installed FlashBurner 2.10.0 silently from the signed package. The installer
    returned exit code zero.
11. Confirmed that the previously missing `MI_01` interface then appeared as
    **TUSB9260 Flash Burner Driver**, status OK, error code zero, service
    `wdfapldr`.
12. Launched the signed GUI. Its manifest requires administrator elevation, so
    the elevated window could not be safely controlled by the unelevated app
    automation runtime. The GUI was left unused and the investigation switched
    to TI's recordable command-line tool.
13. Downloaded the command-line FlashBurner 1.3.1 attachment published by a TI
    employee in the TI E2E support thread above. The archive SHA-256 was
    `CE9652B517C4BB506325FFB40700F1C085B0C24455D053726DCD419777C7D86C`.
    Unlike the SLLC414 GUI installer, this older E2E attachment is not
    Authenticode-signed; its provenance is the TI-hosted support attachment.
14. Downloaded the Linux FlashBurner 0.1.2 source attachment from the same TI
    thread for protocol/reference fallback. Its archive SHA-256 was
    `F8D829FEFC594086EE055C740F5D6E572BBCA163E0DF0330139C8DE04BC87D02`.
    It was inspected but not built or used because the supported Windows
    command-line tool and installed driver detected the target successfully.
15. Installed the command-line package under
    `C:\Program Files (x86)\Texas Instruments Inc\Command Line FlashBurner`.
    The installed `TUSB926x_CL_Burner.exe` SHA-256 was
    `4BEA39462D697550497EDFCF1804640739C350A0927619A66E444E60116755F3`.
16. Ran its read-only `/p` probe with elevation. It returned exactly one target:

    ```text
    Boot Loader Instance 0
    Flash Burner driver: Yes
    Flash Present: True
    ```

17. Tried to program the default `TUSB9261_RDX.hex`. FlashBurner rejected it as
    discontinuous before initiating any device write. The device remained in
    bootloader mode with all three interfaces healthy.
18. Generated a continuous image with TI `armhex` image mode and independently
    parsed both HEX files. The check proved that all 720 original bytes were
    identical, the output covered every byte from `0x08000000` through
    `0x0800038F`, and the 192 added bytes were all `0xFF`.
19. Programmed only bootloader instance zero with the continuous HEX file. The
    TI utility reported `Programming Succeeded` and instructed the operator to
    unplug and reconnect the device. The TUSB926x bootloader interfaces then
    disappeared from Windows, consistent with the LED-only firmware running
    without a USB stack.

FlashBurner was installed under:

```text
C:\Program Files (x86)\Texas Instruments Inc\TUSB926x_Burner
```

## Built artifacts

The successful build produced:

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.out` | 3,877 bytes | `57C0D7D3298E2E3588D86D8B7CC625346E0D2B6BDCDEEE05C523C4F6AC9EAF0D` |
| `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex` | 1,782 bytes | `26A7AB7D4CC127591685D10F93642BA66453494C2EAAA92A518D7A79EBB96AC7` |
| `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX_flash.hex` | 2,231 bytes | `D0457B1FA6A399C2B7471A1238D87C0283A03E000452744E85597046BB1522BD` |

The HEX file uses Intel HEX extended address `0x0800`, corresponding to the
linked Cortex-M3 application address range beginning at `0x08000000`.

The continuous programming image was generated with this TI `armhex` command
file from inside `.pio/build/tusb9261_ti_cgt`:

```text
TUSB9261_RDX.out
--intel
--byte
--image

ROMS
{
    FLASH: origin = 0x08000000,
           length = 0x390,
           romwidth = 8,
           memwidth = 8,
           fill = 0xFFFFFFFF,
           files = { TUSB9261_RDX_flash.hex }
}
```

The 32-bit `0xFFFFFFFF` fill value is intentional: TI ARM CGT 5.2.5 otherwise
expands `0xFF` as the repeating four-byte pattern `FF 00 00 00` for this ARM
output. The wider value produces `FF FF FF FF` throughout every linker gap.

## Validated programming procedure

1. Keep the RDX connected and confirm that Device Manager shows both its HID
   interface and **TUSB9260 Flash Burner Driver** without warnings.
2. Start `TUSB926x_Burner.exe` with administrator rights.
3. Select the compatible `VID_0451&PID_926B` bootloader device.
4. Leave **Program Full Binary Image** disabled; use the normal **Program**
   workflow so FlashBurner supplies the required SPI-image header and
   descriptors.
5. Generate and independently verify the continuous programming image as
   described above, then select
   `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX_flash.hex` as the firmware image.
6. Use descriptors compatible with the minimal LED firmware. Because the
   firmware intentionally implements no USB interfaces after boot, descriptor
   values affect only image formatting and do not provide a runtime USB
   interface.
7. Click **Program** once and wait for the terminal success or failure message.
   Do not disconnect or power-cycle while programming is in progress.
8. Record the exact FlashBurner result.
9. Power-cycle the RDX if FlashBurner has not already reset it. Expected
   standalone behavior is the documented repeating
   nine-state LED sequence with each state held for ten seconds. USB
   enumeration is not expected after the application starts.
10. If the device does not run, use the validated J7 flash-disable startup
    sequence documented above to force ROM bootloader enumeration. Remove the
    J7 bridge after the bootloader has enumerated but before probing or
    programming the SPI flash.

## Programming result

The successful command-line programming log was:

```text
TUSB9260 Command Line Flash Burner version 1.3.1.0
2010 <C> Texas Instruments Inc. All Rights Reserved.

/s - Selected Device = 0
/f - Firmware File = HEX - D:\Projekte\OpenRDX\TUSB9261_RDX_PlatformIO\.pio\build\tusb9261_ti_cgt\TUSB9261_RDX_flash.hex

Programming is in progress... (HID\VID_0451&PID_926B&MI_00\8&2C186235&0&0000)
Programming Succeeded - (HID\VID_0451&PID_926B&MI_00\8&2C186235&0&0000)

Note: Please unplug and plug your device back in, so it can be properly enumerated.
```

The elevated wrapper process returned exit code zero. After the write, no
TUSB926x bootloader PnP instance remained visible to Windows. This validates
host detection, the bootloader driver, SPI-flash presence, image parsing, and
the programming transaction.

The operator subsequently confirmed that both physical LEDs work as expected
through the complete repeating nine-state sequence, including the intended
green and amber steady and blinking states. This closes the hardware-validation
step for the tested Tandberg RDX unit.

## J7 recovery and motor-pulse programming result

After the LED-only image had removed normal USB enumeration, the operator
performed the validated J7 sequence:

1. The device was unpowered and the J7 square and round pads were bridged.
2. USB power was connected with the bridge installed.
3. Windows enumerated the TUSB9261 ROM bootloader as VID `0451`, PID `926B`.
4. J7 was opened while USB remained connected. The bootloader remained
   visible, proving that the strap is needed only during startup.
5. An elevated read-only FlashBurner `/p` probe reported exactly one target:

   ```text
   Boot Loader Instance 0
   Flash Burner driver: Yes
   Flash Present: True
   ```

This validates both the J7 boot-selection function and the requirement to open
J7 after enumeration so FlashBurner can select and program the SPI flash.

The checked-out branch was `test/eject-button-motor-pulse`. Its successful
TI ARM CGT 5.2.5 build produced:

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.out` | 3,521 bytes | `865987918BDF724A679AF601606512D460BF41128E6B72ADB148E2C495F461B8` |
| `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex` | 1,416 bytes | `C384A3E90CCD7DADBF3DFE274E42CB0EEF7E999BC78DFA70C7C81F99D90ACBBF` |
| `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX_flash.hex` | 1,878 bytes | `B9967AE5A067CEECC68DBB672E5AFF178CCF28C068376FC73D1E2F9E9A7136C1` |

The continuous image covers `0x08000000` through `0x080002FF`. Independent
comparison confirmed that all 576 firmware bytes from the normal linker output
were preserved and that the 192 added gap bytes were `0xFF`.

All five motor/eject behavior and safety tests passed. The same unrelated
Windows path-separator assertion noted for the LED branch failed because the
test expected forward slashes while the implementation returned backslashes.

With J7 open and the bootloader still active, command-line FlashBurner 1.3.1
programmed only bootloader instance zero. The exact result was:

```text
/s - Selected Device = 0
/f - Firmware File = HEX - D:\Projekte\OpenRDX\TUSB9261_RDX_PlatformIO\.pio\build\tusb9261_ti_cgt\TUSB9261_RDX_flash.hex

Programming is in progress... (HID\VID_0451&PID_926B&MI_00\8&2C186235&0&0000)
Programming Succeeded - (HID\VID_0451&PID_926B&MI_00\8&2C186235&0&0000)
```

The elevated wrapper returned exit code zero. FlashBurner instructed the
operator to unplug and reconnect the device. The final physical validation for
this branch is to power-cycle with J7 open and confirm that each new active-low
eject-button press drives the recovered motor pattern for two seconds, that a
held button does not retrigger it, and that releasing and pressing the button
again starts one new pulse. USB enumeration is not expected from this minimal
test firmware.
