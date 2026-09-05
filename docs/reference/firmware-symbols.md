# Firmware symbol catalog

The [symbol catalog](../../tests/fixtures/firmware_symbol_manifest.json) records
534 fixed bindings, their declaration types, current source locations, and the
evidence supporting their names. Each entry has a semantic name; no entry is
awaiting a name.

| Kind | Entries | Meaning |
| --- | ---: | --- |
| Control-flow target | 133 | A specific block reached by a branch |
| MMIO register | 55 | A controller register with a verified fixed address |
| State | 288 | A persistent field, counter, flag, or callback |
| Storage | 51 | A buffer, descriptor, signature, or constant table |
| Label data | 7 | A named data location used by a firmware procedure |

Six signature, key, sense-table, and identifier bindings are data rather than
executable branch destinations. Their addresses and types are unchanged. Datapath RAM fields, including ATA
IDENTIFY words and USB transfer descriptors, are classified as data; MMIO is
reserved for the controller and processor register ranges. The address-valued
capacity constants `0xFFFFFFFE` and `0xFFFFFFFF` are data, not registers.

## Naming evidence

Names describe the operation or data represented by the code, rather than a
nearby caller or a peripheral mentioned elsewhere in the function. Examples
include:

| Name | Supporting operation or layout |
| --- | --- |
| `usb_device_speed` | USB speed enumeration and the `USB_DEVICE_T` field layout |
| `usb_power_management_state` | Suspend, resume, reset, and disconnect callback values |
| `usb_out_endpoint_info`, `usb_in_endpoint_info` | Direction-selected `EP_INFO_T` arrays with a `0x28`-byte stride |
| `core_sha256_round_constants` | The 64 SHA-256 round words consumed by the compression function |
| `core_command_written_bytes_low`, `core_command_read_bytes_low` | Counters incremented by sector count multiplied by sector size |
| `core_allocate_software_timer`, `core_advance_software_timers` | Allocation and saturating subtraction across 19 countdown slots |
| `core_set_usb_test_mode` | `DCTL` bits 4:1, distinct from its link-state request field |
| `core_mask_usb_interrupt` | VIM bank 0, bit 21, which belongs to USB |
| `core_execute_ata_standby_or_verify` | Argument one selects `E0`; the other path selects `40` |
| `core_divide_multiword_unsigned` | Quotient estimation, subtraction, correction, and remainder handling |

The catalog distinguishes **confirmed** names, supported by controller
definitions or checked data, from **structural** names, which describe the
observed code and access pattern. Both have an evidence note. This distinction
does not imply physical-device validation.

## Source and build boundaries

The fixed-binding catalog applies to the top-level firmware modules and the
fragments in `src/rdx_core/`. The division routines are grouped in
`02_multiword_division.inc`. These files are outside the production build.

The production image uses `src/rdx_mount/` and `include/rdx_mount/`, producing
39 objects with TI ARM CGT. Its mechanism states describe the low-control
drive, no-media grace interval, endpoint waits, return drive, settling, and
exhausted retries. The state values, timeouts, register writes, and transitions
retain their existing values and operations.

## Maintenance checks

The Python suite checks both source trees, symbol/header/linker consistency,
data-versus-control classification, current catalog references, function
banners, repository language, and licensing metadata. The immutable catalog
projection protects binding identities, addresses, and declaration types;
evidence locations and classifications can be corrected without changing that
layout boundary.

After moving or editing a cataloged source, update its `uses` locations in the
catalog. The location check rejects references that no longer match the source.
Run the [documented tests and TI build](../development/building.md) before
publishing firmware changes.

TI package assembly and documentation headers retain their TI terms. Files
with OpenRDX additions identify both contributions where applicable. The
license checker also records the expected TI file ownership independently of
the headers, so removing an attribution cannot change the expected license.

[Back to the documentation index](../README.md)
