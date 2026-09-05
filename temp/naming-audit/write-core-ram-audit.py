"""Write the RAM-slice symbol audit without touching firmware inputs."""
import json
import pathlib
import re

root = pathlib.Path('.')
rows = [row for row in json.load(open('tests/fixtures/firmware_symbol_manifest.json'))['symbols']
        if row['placeholder_name'].startswith('core_') and 0x0800EE00 <= int(row['address'], 16) <= 0x0800F6FF]
by_address = {int(row['address'], 16): row for row in rows}
proposals = []

def add(address, new, explanation, *evidence, confidence='structural'):
    """Record one address-preserving symbol correction with direct use evidence."""
    row = by_address[address]
    proposals.append({'old': row['semantic_name'], 'new': new, 'address': row['address'],
                      'evidence': [explanation, *evidence], 'confidence': confidence})

add(0x0800EE18, 'core_smart_temperature_celsius',
    'Stores the temperature extracted from SMART attributes 0xC2 or 0xBE; 0xFF marks absent temperature, and 41/45 degree thresholds drive thermal control.',
    'src/rdx_core/07_eject_reconnect_smart.inc:844-852 extracts attributes C2/BE into this address.',
    'src/rdx_core/08_thermal_and_record_validation.inc:261-279 compares this value against 0x29 and 0x2D.', confidence='confirmed')
add(0x0800EE1A, 'core_smart_temperature_sample_timer',
    'Software-timer handle controlling SMART temperature sampling, armed for 300000 ticks normally or 10000 after a hot sample.',
    'src/rdx_core/08_thermal_and_record_validation.inc:255-275 polls this timer before reading SMART and rearms it.',
    'src/rdx_core/12_ahci_smart_spi_control.inc:54 initializes its timeout to zero.', confidence='confirmed')
add(0x0800EE1E, 'core_ata_standby_active',
    'Flag becomes one only after ATA STANDBY IMMEDIATE succeeds; it suppresses active thermal/standby monitoring until cleared by activity or resumed status handling.',
    'src/rdx_core/14_mww_usb_command_control.inc:186-190 sets it after core_submit_ata_standby_immediate(1) succeeds.',
    'src/rdx_core/07_eject_reconnect_smart.inc:124-128 exits monitoring while the flag is nonzero.',
    'src/rdx_core/14_mww_usb_command_control.inc:708-711 clears it before processing active device status.')
add(0x0800EE20, 'core_standby_activity_check_timer',
    'Software-timer handle polled in the ATA standby monitor before accumulating transfer activity; it is armed to five minutes and may be rearmed to one second.',
    'src/rdx_core/07_eject_reconnect_smart.inc:130-164 selects this timer, checks standby/activity fields, and rearms the selected timer.',
    'src/rdx_core/20_command_state_accessors.inc:710 arms it for 300000.')
add(0x0800EE21, 'core_thermal_pwm_override_active',
    'Boolean probe result for descriptor field 0x40 low-byte value 1. When asserted it suppresses automatic temperature phase selection and forces PWM state 7/output zero.',
    'src/rdx_core/03_identity_and_mechanism.inc:428 stores the core_get_0x40 boolean result.',
    'src/rdx_core/19_identity_sense_and_mww.inc:102-108 forces PWM off for the asserted flag; :566-577 defines the probe.',
    'src/rdx_core/08_thermal_and_record_validation.inc:277 guards automatic temperature selection.')
add(0x0800EE23, 'core_ata_standby_request_pending',
    'Flag records an ATA standby request that must wait for command arbitration; the monitor clears it immediately before submitting deferred standby work.',
    'src/rdx_core/19_identity_sense_and_mww.inc:84-89 sets it when core_forward_mww_transfer_value returns zero.',
    'src/rdx_core/07_eject_reconnect_smart.inc:120-122 consumes the flag and invokes the standby handler.')
add(0x0800EE24, 'core_secondary_descriptor_state',
    'Scalar secondary-descriptor state, passed as an output state pointer during secondary metadata configuration and normalized from state 2 to 1 when descriptor masks are cleared.',
    'src/rdx_core/09_command_phase_and_ata.inc:432 passes its address to secondary descriptor configuration.',
    'src/rdx_core/14_mww_usb_command_control.inc:294-298 reads state 2 and writes state 1.')
add(0x0800EE28, 'core_primary_descriptor_state',
    'Scalar primary-descriptor state. Values 2/3 select special primary-descriptor response behavior; reset normalizes both to 1 alongside clearing the primary descriptor mask.',
    'src/rdx_core/02_management_descriptors.inc:455-460 selects primary descriptor response mode from states 2/3.',
    'src/rdx_core/14_mww_usb_command_control.inc:299-302 clears the primary mask and normalizes this state.')
add(0x0800EE40, 'core_command_sense_table_index',
    'Indexes three-byte sense key/ASC/ASCQ entries: multiplied by three and applied to the three sense columns.',
    'src/rdx_core/14_mww_usb_command_control.inc:321-329 computes the triplet index and writes fixed-format SCSI sense fields.', confidence='confirmed')
add(0x0800EE47, 'core_command_write_blocked_flag',
    'Byte retained when entering command phase 3; a nonzero value causes the write-request validation branch to reject rather than use the descriptor availability check.',
    'src/rdx_core/16_record_builders_and_capacity.inc:74-77 stores this byte on phase 3 entry.',
    'src/rdx_core/19_identity_sense_and_mww.inc:414-422 returns blocked unless this byte is zero and the descriptor availability check succeeds.',
    'src/rdx_core/06_record_transfer_helpers.inc:353-357 maps that blocked result to command status 0x15 on the write branch.')
add(0x0800EE4C, 'core_selected_sense_status_byte',
    'Leading status byte in the four-byte selected-sense tuple. It is cleared with the tuple and written separately from sense key and ASC/ASCQ.',
    'src/rdx_core/19_identity_sense_and_mww.inc:830-838 writes status, sense key, and ASC/ASCQ through the three setters.',
    'src/rdx_core/22_state_queries_and_hardware_helpers.inc:471-475 stores the uint8_t status byte.')
add(0x0800EE4D, 'core_selected_sense_key',
    'Selected-sense tuple byte receives the sense-key argument, including 4 for hardware error paired with ASC/ASCQ 0x4400.',
    'src/rdx_core/03_identity_and_mechanism.inc:592-598 publishes the hardware-error sense tuple.',
    'src/rdx_core/19_identity_sense_and_mww.inc:830-838 passes the middle argument to this byte setter.')
add(0x0800EE4E, 'core_selected_sense_asc_ascq',
    'Selected-sense tuple halfword receives packed ASC/ASCQ values such as 0x4400 and 0x4080-0x4082.',
    'src/rdx_core/03_identity_and_mechanism.inc:594-638 selects the 16-bit sense detail.',
    'src/rdx_core/22_state_queries_and_hardware_helpers.inc:457-461 stores a uint16_t argument.')
for address, name, direction, low, high in [
    (0x0800EE50, 'core_command_written_bytes_low', 'write', 713, 715),
    (0x0800EE54, 'core_command_written_bytes_high', 'write', 713, 715),
    (0x0800EE58, 'core_command_read_bytes_low', 'read', 707, 709),
    (0x0800EE5C, 'core_command_read_bytes_high', 'read', 707, 709),
]:
    add(address, name,
        f'One half of the 64-bit {direction}-byte accumulator. Successful block transfers multiply block count by logical block size before addition, with carry into the high half.',
        f'src/rdx_core/07_eject_reconnect_smart.inc:703-{high} adds block_count * block_size to the {direction} pair.',
        'src/rdx_core/17_cartridge_state_and_validation.inc:339-348 identifies writes by SCSI opcodes 0x0A/0x2A/0x2E/0x8A/0x8E/0xAA/0xAE.',
        'src/rdx_core/06_record_transfer_helpers.inc:323-325 stores that read/write classification in descriptor byte 10; the ATA builder uses zero for read and nonzero for write.', confidence='confirmed')
add(0x0800EE71, 'core_mode_page_31_flags',
    'Flag byte from mode page 0x31, byte 4. Bits 3/4 are updated by MODE SELECT and returned by the matching MODE SENSE builder.',
    'src/rdx_core/03_identity_and_mechanism.inc:173-203 parses page character 1 (0x31), byte 4, and stores bits 3/4.',
    'src/rdx_core/01_image_validation.inc:403 writes this byte to response page offset 4.', confidence='confirmed')
add(0x0800EE78, 'core_notification_mode',
    'Notification mode scalar, loaded from persistent identity byte 0x0D and updated by the notification-mode setter.',
    'src/rdx_core/16_record_builders_and_capacity.inc:649 stores the mode argument.',
    'src/rdx_core/22_state_queries_and_hardware_helpers.inc:684-691 retrieves persistent identity byte 0x0D.')
add(0x0800F0B0, 'core_ahci_port_initialization_state',
    'State passed directly to the AHCI port initialization state machine; state 0x10 permits export of the initialized ATA device information.',
    'src/ahci.c:518 passes this state address to ahci_run_port_initialization_state_machine.',
    'src/rdx_core/09_command_phase_and_ata.inc:316-326 exposes device information only in state 0x10.', confidence='confirmed')
add(0x0800F0B4, 'core_usb_reconnect_settle_timer',
    'Software-timer handle started for 2000 ticks immediately after USB reconnect.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:452-454 connects USB then arms this timer.', confidence='confirmed')
add(0x0800F0B5, 'core_usb_disconnect_requested',
    'Boolean set by USB disconnect and cleared after USB reconnect.',
    'src/usb_stack.c:420 sets this flag at the start of usb_hal_disconnect.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:452-454 clears it after usb_hal_connect.', confidence='confirmed')
add(0x0800F0B6, 'core_logical_output_fourteen_hold_timer',
    'Software-timer handle holding logical output 0xE active for 30000 ticks before normal state-dependent output control may lower it.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:769-772 sets output 0xE, allocates this timer, and arms 30000 ticks.',
    'src/rdx_core/12_ahci_smart_spi_control.inc:622-629 polls it when selecting the logical output 0xE value.')
add(0x0800F0B8, 'core_device_command_busy',
    'Shared device-command arbitration busy flag. MWW supported commands set it; MWW completion, ATA cleanup, and SMART completion clear it. Internal acquisition helpers test for zero.',
    'src/rdx_core/21_persistence_and_state_setters.inc:614-615 sets the MWW ownership flag and this busy flag.',
    'src/mww.c:658 clears it before completing device transfer state.',
    'src/rdx_core/23_low_level_accessors.inc:43-45 clears it after SMART handling; :501-502 checks it during internal command arbitration.')
add(0x0800F0B9, 'core_mww_device_command_active',
    'MWW ownership flag set before forwarding supported commands and used to release the shared device-command busy state at MWW completion.',
    'src/mww.c:54-57 sets ownership before forwarding a supported command.',
    'src/rdx_core/21_persistence_and_state_setters.inc:614-615 sets this byte and device busy.',
    'src/rdx_core/19_identity_sense_and_mww.inc:339-345 clears device busy when this ownership byte is nonzero.')
add(0x0800F0BA, 'core_mww_command_received_flag',
    'Flag set after an incoming MWW descriptor passes the ATAPI/command gate; reconnect state machines observe the flag while waiting for host traffic.',
    'src/mww.c:44-49 sets it after the command gate and before extracting the command payload.',
    'src/rdx_core/24_reconnect_state_machine.inc:25-27 and :49-51 poll it to retain reconnect state 2.')
add(0x0800F0BB, 'core_mww_dispatch_blocked_flag',
    'Flag tested at MWW handler entry; nonzero causes dispatch to be deferred. Internal command helpers set it and the deferred-dispatch worker clears it before retrying the MWW handler.',
    'src/mww.c:37-40 checks the accessor and defers transfer handling when nonzero.',
    'src/rdx_core/20_command_state_accessors.inc:285-288 clears it before retrying MWW dispatch.',
    'src/rdx_core/23_low_level_accessors.inc:501 sets it when requesting device-command arbitration.')
add(0x0800F0BC, 'core_mww_dispatch_deferred',
    'Pending deferred-MWW-dispatch flag. It is set when the MWW handler finds dispatch blocked, and consumed by the retry worker.',
    'src/mww.c:37-40 routes blocked dispatch to the setter.',
    'src/rdx_core/21_persistence_and_state_setters.inc:587 sets this byte and disables the transfer interrupt.',
    'src/rdx_core/20_command_state_accessors.inc:285-288 clears it and retries the MWW handler.')
add(0x0800F0E0, 'core_firmware_image_validation_active',
    'Flag marks an in-progress structured firmware-image validation, set at stream initialization and cleared on validator completion or successful flash commit.',
    'src/rdx_core/14_mww_usb_command_control.inc:247-253 initializes validation state and manages this flag around core_validate_structured_flash_image.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:112-115 clears it after the first-word flash commit.', confidence='confirmed')
add(0x0800F0E4, 'core_firmware_image_commit_word',
    'Saved first four bytes of a firmware image, held until the end and then programmed at SPI flash address zero.',
    'src/rdx_core/14_mww_usb_command_control.inc:757-759 saves the first four bytes.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:112 programs exactly four bytes from this address to flash offset zero.', confidence='confirmed')
add(0x0800F0E8, 'core_firmware_image_validation_context',
    'Persistent structured-image validator context; cleared over 0x198 bytes and passed to the image validator during streamed firmware processing.',
    'src/rdx_core/20_command_state_accessors.inc:195-196 clears this 0x198-byte context.',
    'src/rdx_core/14_mww_usb_command_control.inc:248-251 initializes and submits it to core_validate_structured_flash_image.', confidence='confirmed')
add(0x0800F284, 'core_thermal_pwm_state',
    'State of the thermal PWM sequencer; values select settling, ramping, idle, override, or disabled state 8.',
    'src/rdx_core/04_hash_flash_usb_thermal.inc:717-817 is the PWM ramp state machine.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:367-370 gates PWM output 0xF on state !=8.', confidence='confirmed')
add(0x0800F288, 'core_thermal_pwm_control_phase',
    'Control phase consumed and advanced by the thermal PWM sequencer. Temperature thresholds select phases 1/2 and standby selects phase 0.',
    'src/rdx_core/08_thermal_and_record_validation.inc:277-288 selects phases from temperature thresholds.',
    'src/rdx_core/04_hash_flash_usb_thermal.inc:721-812 reads and advances the phase during PWM control.', confidence='confirmed')
add(0x0800F290, 'core_led_controller_count',
    'Count of registered two-output LED controllers, bounded to two and used by the periodic LED state updater.',
    'src/rdx_core/14_mww_usb_command_control.inc:794-801 increments the count and rejects more than two.',
    'src/rdx_core/13_runtime_transfer_helpers.inc:205 registers one LED pair; src/ums_bot.c:42 registers the other.', confidence='confirmed')
add(0x0800F294, 'core_led_controller_table',
    'Two-entry pointer table of registered LED controllers, each holding two logical output numbers and its current pattern state.',
    'src/rdx_core/14_mww_usb_command_control.inc:794-799 writes controller pointers with their two logical outputs.',
    'src/rdx_core/12_ahci_smart_spi_control.inc:594-597 iterates controllers and drives their state/output routines.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:420 clears the eight-byte table.', confidence='confirmed')
add(0x0800F2D4, 'core_software_timer_count',
    'Allocation count for the 19 software timers; a new timer receives this count as its byte handle before the count increments.',
    'src/rdx_core/16_record_builders_and_capacity.inc:699-706 bounds the count to 0x12, assigns the handle, initializes the slot, and increments the count.', confidence='confirmed')
add(0x0800F2D8, 'core_software_timer_remaining_ticks',
    'Table of 19 software-timer remaining tick values. Timer handles index the table for arming/expiry checks, while tick processing decrements each entry with saturation at zero.',
    'src/rti.c:203 stores the timeout at the handle-indexed slot.',
    'src/rdx_core/21_persistence_and_state_setters.inc:42 checks for zero at the handle-indexed slot.',
    'src/rdx_core/15_control_transfer_crc_records.inc:337-353 decrements all 19 entries.', confidence='confirmed')
add(0x0800F418, 'core_ahci_dma_setup_callback',
    'Callback pointer queued after receiving a DMA Setup FIS, registered alongside command-completion and error callbacks.',
    'src/rdx_core/14_mww_usb_command_control.inc:96-98 queues this pointer after reading the received DMA Setup tag.',
    'src/rdx_core/21_persistence_and_state_setters.inc:301-303 registers it between completion and error callbacks.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/include/ahci.h:365-367 names the corresponding DMA Setup callback.', confidence='confirmed')
add(0x0800F487, 'core_ata_dma_setup_auto_activate_supported',
    'IDENTIFY DEVICE word 78 bit 2 capability for DMA Setup Auto-Activation.',
    'src/rdx_core/01_image_validation.inc:578-582 extracts bit 2 of ata_identify_sata_features_supported.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/ahci.c:1192-1193 names the exact matching bit.', confidence='confirmed')
for address, name, half, offset in [(0x0800F494, 'core_ata_true_max_lba_low', 'low', '0x18'),
                                    (0x0800F498, 'core_ata_true_max_lba_high', 'high', '0x1C')]:
    add(address, name,
        f'The {half} half of the preserved true maximum-LBA value, copied from the device capacity words before any logical-sector emulation.',
        f'src/rdx_core/01_image_validation.inc:618-619 reads capacity halves and :667-668 writes them at ATAPI flag base 0x0800F47C plus {offset}.',
        '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/ahci.c:1284-1286 matches the true-sector-size/true-MaxLBA/true-alignment copy sequence.', confidence='confirmed')
add(0x0800F4C9, 'core_ata_device_fault_flag',
    'ATA device fault flag copied from bit 5 of the received D2H FIS status byte.',
    'src/rdx_core/09_command_phase_and_ata.inc:684-686 extracts the D2H status bit.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/ahci.c:2205 assigns the corresponding bDeviceFault field.', confidence='confirmed')
add(0x0800F5E0, 'core_ata_callback_write_index',
    'Write index for a sixteen-entry ATA callback ring. It indexes callback pointers/pending bytes and twelve-byte callback data records, then increments and wraps at sixteen.',
    'src/rdx_core/13_runtime_transfer_helpers.inc:759-763 enqueues at this index and wraps it.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/ahci.c:118-124 implements the corresponding callback_index operation.', confidence='confirmed')
add(0x0800F610, 'core_active_command_descriptor',
    'Pointer to the current command descriptor, retained from the command-dispatch buffer and used for returned payload/length fields and ATA submission.',
    'src/rdx_core/10_spi_flash_and_response_dispatch.inc:32-44 stores the input buffer pointer and updates descriptor result fields.',
    'src/rdx_core/12_ahci_smart_spi_control.inc:686-700 reads fields for device-command submission.')
add(0x0800F65C, 'core_descriptor_response_status_byte',
    'Byte copied from response initialization descriptor offset 0x11 and emitted by the descriptor response builder. No independent persistent-state meaning is established.',
    'src/rdx_core/16_record_builders_and_capacity.inc:742 loads byte offset 0x11.',
    'src/rdx_core/05_device_descriptor_dispatch.inc:753 emits that byte through the shared response buffer.')
add(0x0800F670, 'core_usb_request_handler_count',
    'Count for a maximum of four registered USB request handlers paired with masked request-type bytes.',
    'src/rdx_core/16_record_builders_and_capacity.inc:163-170 appends the callback at F6D0 and request type at F6E0 before incrementing this count.',
    'src/rdx_core/09_command_phase_and_ata.inc:113-121 invokes the registered callback with the active request context.', confidence='confirmed')

# The verified USB_STACK_FXN member layout disambiguates the endpoint callback anchors.
usb_layout = '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/include/usb_stack.h:152-177 defines four IN and four OUT active callbacks, two callbacks per BOT/UAS direction, four PM callbacks, and reset/EP0 callbacks in that order.'
add(0x0800F671, 'core_usb_power_management_callback_count',
    'Registration count for the four-entry USB power-management callback array.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:236-240 appends a callback at F6B4 and increments this count.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:399-408 matches usb_stack_register_PM_callback.', usb_layout, confidence='confirmed')
add(0x0800F67C, 'core_usb_in_endpoint_two_callback',
    'Active IN endpoint 2 transfer callback at USB_STACK_FXN +8. It anchors a two-entry IN callback copy; the accompanying OUT pair is copied at +0x10.',
    'src/rdx_core/15_control_transfer_crc_records.inc:395-405 and :423-433 copy BOT/UAS endpoint callback pairs into the active IN/OUT slots.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:555-556 and :592-593 are the corresponding UAS/BOT callback copies.', usb_layout, confidence='confirmed')
add(0x0800F694, 'core_usb_bot_in_transfer_callbacks',
    'Base of the two BOT IN transfer callback slots. Endpoint direction bit 7 selects this base and endpoint number minus two selects the element.',
    'src/rdx_core/19_identity_sense_and_mww.inc:533-538 selects F694 when bit 7 is set and stores at endpoint*4-8.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:490-494 matches the BOT OUT/IN registration branches.', usb_layout, confidence='confirmed')
add(0x0800F69C, 'core_usb_bot_out_transfer_callbacks',
    'Base of the two BOT OUT transfer callback slots. A clear endpoint direction bit selects this base and endpoint number minus two selects the element.',
    'src/rdx_core/19_identity_sense_and_mww.inc:533-538 selects F69C when bit 7 is clear and stores at endpoint*4-8.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:490-494 matches the BOT OUT/IN registration branches.', usb_layout, confidence='confirmed')
add(0x0800F6B0, 'core_usb_uas_out_endpoint_three_callback',
    'UAS OUT endpoint 3 callback slot at USB_STACK_FXN +0x3C. The PM callback dispatcher also uses its address as the origin of a preincrement cursor into the following array.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:587-595 initializes the cursor here then preincrements to F6B4 before each PM callback invocation.',
    'src/rdx_core/15_control_transfer_crc_records.inc:427-429 copies the UAS OUT pair from F6AC/F6B0 into active OUT endpoint 2/3 callbacks.', usb_layout, confidence='confirmed')
add(0x0800F6B4, 'core_usb_power_management_callbacks',
    'Four-entry USB power-management callback array following the BOT/UAS endpoint transfer callbacks.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:236-240 registers up to four callbacks.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:587-599 walks four slots and invokes each with the caller-supplied state.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:304-306 invokes the matching pPwrMngmtCallback array with pm_state.', usb_layout, confidence='confirmed')
add(0x0800F6C4, 'core_usb_bot_reset_callback',
    'BOT reset callback invoked after selecting BOT endpoint callbacks and mass-storage class zero.',
    'src/rdx_core/15_control_transfer_crc_records.inc:404-407 selects class zero, configures endpoints, then invokes this callback.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:592-603 selects BOT callbacks and invokes pBOTResetCallback.', usb_layout, confidence='confirmed')
add(0x0800F6C8, 'core_usb_uas_reset_callback',
    'UAS reset callback invoked after selecting UAS endpoint callbacks and mass-storage class one.',
    'src/rdx_core/15_control_transfer_crc_records.inc:432-435 selects class one, configures endpoints, then invokes this callback.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:555-566 selects UAS callbacks and invokes pUASResetCallback.', usb_layout, confidence='confirmed')
add(0x0800F6CC, 'core_usb_ep0_out_transfer_callback',
    'Endpoint-zero OUT data-transfer completion callback. The control-transfer state machine invokes it on transfer completion or short transfer.',
    'src/rdx_core/12_ahci_smart_spi_control.inc:796-802 invokes this callback at control-transfer completion.',
    '../TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/source/usb_stack.c:208-210 invokes pEP0DataXferCallback_OUT; :250-253 registers it.', usb_layout, confidence='confirmed')

# A reviewed-unchanged entry is not a claim of stronger semantics than its use sites support.
changed = {item['old'] for item in proposals}
unchanged = []
notes = {
    'core_cartridge_monitor_context': 'A real 0x14-byte monitor state structure with three embedded software-timer handles; context is appropriate.',
    'core_command_feature_state': 'Byte used to gate a feature request and set/get externally; no stronger feature meaning established.',
    'core_command_buffer_state': 'A phase-three command-state byte with set/get paths; specific contract beyond the state flag remains unclear.',
    'core_device_status_value': 'Only set/get accessors establish a byte status value; retain the conservative name.',
    'core_response_type': 'Boolean-like response state gates subsequent response handling; source does not establish a stronger protocol label.',
    'core_flash_update_context': 'Only cleared when the dispatch-blocked flag is zero and during reset; no nonzero writer establishes a replacement meaning.',
    'core_device_request_update': 'Stores a callback-like constant via one setter; insufficient evidence to distinguish port-init callback from another request callback.',
    'core_response_status_value': 'Only reset and queried by the response/USB paths; no nonzero writer establishes a sharper meaning.',
    'core_usb_endpoint_transfer': 'Two-word active endpoint configuration anchor copied with a second pair at +0x10; transport-specific record meaning needs a separate layout audit.',
    'core_firmware_record_update_alternate_table': 'Biased pointer used by endpoint-dependent registration; renaming only one anchor would overstate the exact table layout.',
    'core_firmware_record_update': 'Biased pointer used with endpoint index *4 -8 in callback registration; exact table boundaries remain uncertain.',
    'core_primary_firmware_callback': 'Installed callback invoked after endpoint configuration selects descriptor queue state zero; exact transport contract remains conservative.',
    'core_secondary_firmware_callback': 'Callback invoked when the current control transfer completes or short-transfers; existing name remains conservative pending handler audit.',
}
for row in rows:
    if row['semantic_name'] in changed:
        continue
    note = notes.get(row['semantic_name'], 'All current source uses in core-ram-uses.txt match the named role; no conflicting meaning or stronger correction established.')
    unchanged.append({'name': row['semantic_name'], 'address': row['address'], 'review': note})

proposals.sort(key=lambda item: int(item['address'], 16))
assert len({item['old'] for item in proposals}) == len(proposals)
assert len({item['new'] for item in proposals}) == len(proposals)
all_names = {row['semantic_name'] for row in json.load(open('tests/fixtures/firmware_symbol_manifest.json'))['symbols']}
assert not ({item['new'] for item in proposals} & all_names)
assert len(proposals) + len(unchanged) == len(rows)
pathlib.Path('temp/naming-audit/core-ram-renames.json').write_text(json.dumps(proposals, indent=2) + '\n')
pathlib.Path('temp/naming-audit/core-ram-reviewed-unchanged.json').write_text(json.dumps(unchanged, indent=2) + '\n')
summary = {'address_start': '0x0800EE00', 'address_end': '0x0800F6FF', 'core_rows_reviewed': len(rows),
           'rename_proposals': len(proposals), 'reviewed_unchanged': len(unchanged),
           'scope': 'core-prefixed manifest state/storage rows only; peripheral rows are owned by the peripheral audit',
           'shared_files_modified': False,
           'source_evidence': 'Direct current source uses captured in core-ram-uses.txt, with TI reference comparisons for ATA fields.',
           'limits': 'No behavior changes, compilation, binary rewriting, or hardware operations performed.'}
pathlib.Path('temp/naming-audit/core-ram-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
