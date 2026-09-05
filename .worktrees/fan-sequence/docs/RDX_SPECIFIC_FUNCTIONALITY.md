# RDX-specific firmware functionality

This document lists the TUSB9261 RDX functionality that goes beyond ordinary
USB Mass Storage to SATA translation.

## Functionality inventory

### 1. Tandberg/RDX device presentation

The firmware presents the bridge as a Tandberg RDX product instead of simply
exposing the identity of the attached SATA disk. The device identity uses:

- Manufacturer: `TANDBERG`
- Product: `RDX`
- USB VID:PID: `1A5A:0005`
- `bcdDevice`: `0x0283`, strongly suggesting firmware version 2.83
- One SCSI-transparent Bulk-Only Transport interface

The active interface uses endpoint `0x83` for IN and endpoint `0x03` for OUT.
There is no active HID interface.

Source references:

- [`usb_get_device_descriptor`](../src/usb_stack.c) returns the embedded USB
  device descriptor.
- [`core_initialize_rdx_identity_records`](../src/rdx_core.c) initializes the
  associated runtime identity/configuration data.
- The verified descriptor values are recorded in
  [`TUSB9261_RDX_TECHNICAL_REFERENCE.md`](../doc/TUSB9261_RDX_TECHNICAL_REFERENCE.md#device-identity).

### 2. RDX unit and manufacturing identity

The firmware reads and validates a 256-byte unit record at SPI flash offset
`0x3E000`. The record contains Tandberg/RDX identifiers, serial-like
values, manufacturing information, and other implementation-specific fields.

Observed contents include:

- `7820999743`
- `TANDBERG`
- `RDX`
- `306200140256B3A9L5000157`
- `201220218`

If the record cannot be read or its checksum is invalid, the firmware installs
default identity values in RAM. Code also exists to erase and rewrite the
record.

Source references:

- [`core_initialize_rdx_identity_records`](../src/rdx_core.c) reads, validates, and
  supplies defaults for the record.
- [`core_read_unit_identity_record`](../src/rdx_core.c) reads 256 bytes from
  `0x3E000`.
- [`core_rewrite_unit_identity_record`](../src/rdx_core.c) erases and rewrites the
  record.
- [`core_calculate_record_checksum`](../src/rdx_core.c) calculates the record
  checksum.

### 3. Persistent RDX state with fallback copy

The firmware maintains a 64-byte state or configuration record at SPI flash
offset `0x3F000`. It validates the primary record and tries a second copy at
`0x3FC00` if the primary record is unreadable or invalid. If both copies fail,
the state is cleared and a new primary record can be written.

This is not part of normal USB-to-SATA forwarding; it is persistent
device-specific state managed by the RDX application.

Source references:

- [`core_initialize_rdx_identity_records`](../src/rdx_core.c) implements primary
  validation, fallback loading, reset, and rewrite decisions.
- [`core_read_primary_persistent_state`](../src/rdx_core.c) reads `0x3F000`.
- [`core_read_fallback_persistent_state`](../src/rdx_core.c) reads `0x3FC00`.
- [`core_rewrite_primary_persistent_state`](../src/rdx_core.c) erases and rewrites the
  primary record.

### 4. Cartridge and mechanism state machine

The firmware contains a multi-stage state machine that monitors logical inputs,
debounces state changes, controls several digital or PWM outputs, coordinates
SATA presence, and uses timeouts and retries. The state machine acts as a
cartridge/mechanism controller rather than ordinary SATA transport.

The sequence includes:

- Debounced input monitoring
- Load/removal state transitions
- Timed output activation
- SATA-link and device-state checks
- Retry handling
- Mechanism failure paths
- Coordination with USB connection state

Source references:

- [`core_monitor_cartridge_input`](../src/rdx_core.c) monitors an input,
  debounces changes, and coordinates AHCI/device states.
- [`core_advance_mechanism_state`](../src/rdx_core.c) advances the mechanism
  through its operational states.
- [`core_drive_mechanism_outputs`](../src/rdx_core.c) drives the timed output
  sequences for state values 0 through 8.
- [`spi_set_logical_output`](../src/spi.c) controls the logical output layer.
- [`sci_read_logical_input`](../src/sci.c) reads digital and thresholded
  logical inputs.

### 5. Coordinated safe eject and unload

The removal path does not simply drop the SATA link. The firmware checks the
device state and can issue ATA `STANDBY IMMEDIATE` (`0xE0`) before removal. It
then coordinates the cartridge event with USB detach and reconnect behavior so
the host sees a controlled medium change.

The observed sequence includes:

1. Check whether the SATA device is ready for the operation.
2. Issue ATA `STANDBY IMMEDIATE` where required.
3. Update the mechanism state.
4. Disconnect or reset the USB-facing state.
5. Wait for the cartridge transition.
6. Reconnect USB and restart the host-visible device state.

Source references:

- [`core_submit_ata_standby_immediate`](../src/rdx_core.c) builds and submits the ATA
  standby command.
- [`core_coordinate_safe_cartridge_eject`](../src/rdx_core.c) coordinates the
  cartridge event with USB detach and reconnect.
- [`usb_hal_disconnect`](../src/usb_stack.c) performs the USB-side detach or
  reset operation.
- [`core_reconnect_device_and_start_timer`](../src/rdx_core.c) reconnects USB and starts
  a post-connect timer.

### 6. RDX medium-state and mechanism-error reporting over SCSI

Internal cartridge, mechanism, readiness, and recovery states are translated
into host-visible SCSI status and sense fields. This allows the host to receive
distinct results for conditions such as:

- No usable medium
- Medium transition or change
- Not-ready conditions
- Mechanism or hardware failures
- Recovery-required states
- Other RDX-specific failure flags

Source references:

- [`core_select_rdx_sense_status`](../src/rdx_core.c) selects status and
  sense values from internal RDX state flags.
- [`core_write_selected_sense_fields`](../src/rdx_core.c) writes the selected sense
  state fields.

### 7. Vendor-specific SCSI mode pages

The MODE SENSE response builder supports vendor-specific pages `0x31`, `0x33`,
and `0x34`. Page `0x3F` includes them in the response for "all pages." Both
MODE SENSE(6) and MODE SENSE(10) response layouts are handled.

These pages return internal RDX state and capability flags that are not part of
the normal SATA block-device data path. The exact public meaning of every byte
has not yet been decoded.

Source reference:

- [`core_build_scsi_mode_sense_response`](../src/rdx_core.c) recognizes and builds
  pages `0x31`, `0x33`, `0x34`, and `0x3F`.

### 8. Proprietary RDX management and record protocol

The command path parses structured records and dispatches proprietary record
types `C0`, `C2`, `C4`, `C5`, and `C6`. It distinguishes several transfer or
operation forms and returns RDX-specific result values.

The record parser verifies header fields, lengths, flags, and subtype values
before dispatch. The precise external command transport and complete meaning of
each record type remain internal implementation details.

Source reference:

- [`core_dispatch_rdx_management_record`](../src/rdx_core.c) validates and
  dispatches the `C0` through `C6` record family.

### 9. Active SMART temperature monitoring

The firmware actively communicates with the cartridge disk's ATA SMART
interface rather than merely passing host SMART commands through. It enables
SMART operation, reads SMART data, and extracts temperature from attribute
`0xC2`, falling back to attribute `0xBE`.

Source references:

- [`core_build_ata_smart_command`](../src/rdx_core.c) constructs ATA SMART
  commands, including SMART data and status operations.
- [`core_initialize_smart_monitoring`](../src/rdx_core.c) initializes SMART-related
  monitoring state.
- [`core_read_smart_temperature`](../src/rdx_core.c) reads the 512-byte
  SMART data block and searches for attributes `0xC2` and `0xBE`.

### 10. Temperature-driven PWM control

The SMART temperature value drives a timed PWM-control state machine. The code
uses thresholds around 41 to 45 degrees Celsius:

- Normal temperature polling is approximately every 300 seconds.
- Elevated temperature shortens polling to approximately every 10 seconds.
- Crossing the temperature thresholds changes the PWM-control state.
- PWM output is ramped rather than changed in one abrupt step.

The PWM output provides temperature-dependent thermal control.

Source references:

- [`core_control_temperature_monitor`](../src/rdx_core.c) evaluates the SMART
  temperature, changes polling intervals, and applies the thresholds.
- [`core_ramp_thermal_pwm`](../src/rdx_core.c) implements the
  ramped PWM-control state machine.
- [`core_apply_pwm_percentage`](../src/rdx_core.c) applies a percentage to
  a PWM-capable logical output.

### 11. Front-panel and status indication

The firmware initializes heartbeat/activity indication and applies
state-dependent PWM and digital-output patterns. The mechanism state machine
changes levels and timing during loading, ready, error, and removal sequences.

The activity and heartbeat outputs use state-dependent PWM and timing patterns.

Source references:

- [`pwm_init_heartbeat_led`](../src/pwm.c)
- [`pwm_init_activity_led`](../src/pwm.c)
- [`core_drive_mechanism_outputs`](../src/rdx_core.c) applies output levels and
  timing according to cartridge/mechanism state.
- [`core_reset_logical_outputs`](../src/rdx_core.c) resets several related
  logical outputs to a defined state.
- [`core_apply_pwm_percentage`](../src/rdx_core.c) applies PWM levels.

### 12. Integrity-checked structured-image programming

The firmware parses a structured image, validates its bounds and checksums,
maintains a SHA-256 context, and writes validated content to SPI flash in
bounded pages. This appears to be a protected update or configuration-import
path, but its exact externally visible command has not yet been conclusively
named.

The implementation includes:

- Structured header and length validation
- CRC/checksum processing
- SHA-256 initialization, streaming update, and finalization
- Validation of embedded records
- Page-aligned SPI flash programming
- Failure cleanup and status reporting

Source references:

- [`core_validate_structured_flash_image`](../src/rdx_core.c) parses and validates the
  structured input.
- [`core_sha256_compress_block`](../src/rdx_core.c) implements the SHA-256
  compression operation.
- [`core_sha256_finalize`](../src/rdx_core.c) implements SHA-256
  padding and finalization.
- [`core_sha256_initialize`](../src/rdx_core.c) initializes the
  standard SHA-256 state constants.
- [`core_calculate_crc32`](../src/rdx_core.c) implements a CRC using
  polynomial `0x04C11DB7`.
- [`core_sha256_update`](../src/rdx_core.c) streams data into the
  SHA-256 context.
- [`core_write_validated_flash_data`](../src/rdx_core.c) passes validated data to
  the SPI flash writer.
- [`core_program_spi_flash_pages`](../src/rdx_core.c) performs page-bounded SPI
  programming.

## Additional firmware components

### HMAC-based cartridge authentication

The firmware contains the strings `Jefe` and
`what do ya want for nothing?`, together with the matching HMAC-SHA256 test
digest. These values support an HMAC test or self-test path; cartridge
acceptance does not currently call a documented HMAC policy.

### MCP3008 ADC sensing

[`core_read_mcp3008_channel`](../src/rdx_core.c) performs a three-byte SPI
transaction matching a 10-bit, eight-channel ADC read. The logical-input layer
can route certain input types through this ADC and compare the result against a
threshold.

The active MCP3008 channels, their physical sensor connections, and their role
in cartridge/mechanism decisions are not documented.

### One-touch backup and HID

[`one_touch.c`](../src/one_touch.c) contains no project-specific procedure.
Generic HID initialization scaffolding remains in the project, but the active
embedded USB descriptor has no HID interface. Therefore, no one-touch
backup-button or host HID functionality should be claimed for this firmware.

### Cartridge-acceptance policy

Cartridge acceptance may depend on one or more of:

- SATA drive firmware
- Drive model or serial identity
- Cartridge electronics
- Mechanical sensors
- Cryptographic authentication

Determining the active dependencies requires controlled hardware testing and
USB/SATA traces.

## Ordinary bridge functionality excluded from this inventory

The following are normal TUSB9261 USB-to-SATA bridge responsibilities and are
not counted as RDX-specific functionality:

- USB Chapter 9 request handling
- Bulk-Only Transport framing
- Standard SCSI read and write commands
- Standard INQUIRY and capacity responses
- Ordinary SCSI-to-ATA command translation
- AHCI initialization, DMA, and interrupt handling
- Generic USB endpoint operation
- Watchdog and system initialization
- Generic SPI boot-image access
- Standard memory and interrupt helpers

## Validation boundaries

This repository has no validated upload target. Before any hardware test,
validate the persistent-record field definitions, HMAC use, MCP3008 sensor
assignments, cartridge policy, and boot behavior on a controlled test device
with a reliable restore path.
