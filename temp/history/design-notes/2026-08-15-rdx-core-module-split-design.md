# RDX Core Module Split Design

## Goal

Split the 19,554-line `src/rdx_core.c` into understandable implementation
fragments below 1,000 lines without changing function order, generated code,
fixed addresses, the TI ARM9 ABI, or the firmware HEX image.

## Chosen architecture

`src/rdx_core.c` remains the only translation unit. It retains the file header,
includes `rdx_firmware.h`, and then includes 24 ordered implementation
fragments from `src/rdx_core/`. This gives maintainers small responsibility-
named files while preserving the exact compiler view of the source.

The fragments use `.inc` rather than `.c` because they are not independently
compiled modules. Numeric prefixes make the existing function order explicit;
descriptive suffixes identify each fragment's dominant responsibility.

## Fragment layout

1. `01_image_validation.inc`
2. `02_management_descriptors.inc`
3. `03_identity_and_mechanism.inc`
4. `04_hash_flash_usb_thermal.inc`
5. `05_device_descriptor_dispatch.inc`
6. `06_record_transfer_helpers.inc`
7. `07_eject_reconnect_smart.inc`
8. `08_thermal_and_record_validation.inc`
9. `09_command_phase_and_ata.inc`
10. `10_spi_flash_and_response_dispatch.inc`
11. `11_usb_ata_payload_transfer.inc`
12. `12_ahci_smart_spi_control.inc`
13. `13_runtime_transfer_helpers.inc`
14. `14_mww_usb_command_control.inc`
15. `15_control_transfer_crc_records.inc`
16. `16_record_builders_and_capacity.inc`
17. `17_cartridge_state_and_validation.inc`
18. `18_firmware_transfer_and_persistence.inc`
19. `19_identity_sense_and_mww.inc`
20. `20_command_state_accessors.inc`
21. `21_persistence_and_state_setters.inc`
22. `22_state_queries_and_hardware_helpers.inc`
23. `23_low_level_accessors.inc`
24. `24_reconnect_state_machine.inc`

Each boundary falls between complete function definitions. No function, local
label, declaration, or comment is rewritten during the move.

## Build integration

`scripts/ti_cgt_build.py` discovers `src/rdx_core/*.inc` and adds them to the
PlatformIO dependency list. The fragments are not added to `C_SOURCES`, so TI
CGT still emits one `rdx_core.obj` at the same position in the link.

## Regression protection

The test suite will verify that:

- `rdx_core.c` is a wrapper rather than another implementation file;
- every ordered fragment exists and is included exactly once;
- every fragment and the wrapper contain fewer than 1,000 lines;
- concatenating the fragments reproduces the pre-split implementation text;
- semantic-name and comment scans include `.inc` files;
- the build adapter tracks all fragments as source dependencies.

Final verification requires all Python tests, Clang parsing of the wrapper and
all other C sources, a clean TI CGT build, an unchanged `rdx_core.obj` placement
and size, and an Intel HEX SHA-256 equal to the pre-split baseline.

## Safety boundaries

The split does not alter function declarations, function order, absolute
symbols, linker files, compiler flags, artifact names, or hardware state. No
firmware upload or flash operation is part of this work.
