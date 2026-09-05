"""Create supported symbol naming changes without editing firmware files."""
import json
from pathlib import Path
manifest=json.loads(Path('tests/fixtures/firmware_symbol_manifest.json').read_text())['symbols']
by_name={row['semantic_name']:row for row in manifest}
changes=[]

def add(old,new,confidence,*evidence):
    """Record one fixed-address symbol rename with source evidence."""
    row=by_name[old]
    changes.append(dict(old=old,new=new,address=row['address'],evidence=list(evidence),confidence=confidence))

binary='TUSB9261_RDX_firmware_payload_v2.83_0x08000000.bin'
add('core_response_word_context','core_encoded_password_prefix','confirmed',
    'src/rdx_core/11_usb_ata_payload_transfer.inc:core_process_response_word_clear_memory_response_word_context decodes nine bytes into the start of a 32-byte word-cipher input.',
    f'{binary}:0x0800D438 contains a4 b6 c9 ca cf c8 cc bf 96; core_transfer_x01 in 17_cartridge_state_and_validation.inc subtracts byte index and 0x54, yielding Password:.')
add('core_usb_event_context','core_encoded_fingerprint_prefix','confirmed',
    'src/rdx_core/15_control_transfer_crc_records.inc:core_process_usb_event_context decodes twelve bytes into the response then inserts ASCII 0 at offset 11 and encrypts the 32-byte block.',
    f'{binary}:0x0800D444 decodes with core_transfer_x01 to FprintTyp1- followed by a space.')
add('core_memory_copy_context','core_permuted_word_cipher_key_material','confirmed',
    'src/rdx_core/16_record_builders_and_capacity.inc:core_process_clear_memory_clear_memory_copy_context passes this fixed source through core_transfer_bounded_transfer_context to produce sixteen key bytes, then supplies them to core_transfer_remaining_bytes_word.',
    f'{binary}:0x0800D450 starts 18 d6 71 94 7b 21 56 31 75 6e 8f 12 f9 6c 29 43; 19_identity_sense_and_mww.inc uses the 16-entry permutation at 0x0800E8C4.')
add('core_availability_request','core_capacity_range_descriptor_table','structural',
    'src/rdx_core/13_runtime_transfer_helpers.inc:core_run_requested_available_availability_request examines sixteen records of stride 0x94, comparing an input 64-bit value against the low/high words of inclusive lower and upper bounds at record offsets 4/8 and 12/16.',
    f'{binary}:0x0800D4E0 contains sixteen 0x94-byte records with ordered capacity bounds, beginning 0 through 0x0BA43B73; the final record ends at 0xFFFFFFFFFFFFFFFF.')
add('core_spi_transfer_storage','core_scsi_opcode_table_cursor_origin','confirmed',
    'src/rdx_core/17_cartridge_state_and_validation.inc:core_transfer_spi_transfer_storage preincrements this cursor by 0x18 before each of 42 opcode comparisons; the cursor origin itself is not a command entry.',
    f'{binary}:the first row at 0x0800DE20 contains opcode 0x88, direction 1, function pointer 0x0800ABAF and validation metadata pointer 0x0800E210; following rows include 0x8A,0x28,0x2A,0x12,0x25,0x9E.')
add('core_device_update_context','core_mcp3008_channel_thresholds','confirmed',
    'src/rdx_core/17_cartridge_state_and_validation.inc:core_update_device_update_context reads the selected MCP3008 channel and compares its result with the selected fixed uint16 threshold.',
    f'{binary}:0x0800E36C contains eight thresholds 650,650,124,650,650,650,650,650.')
add('core_fallback_register_offset_table','core_default_spi_logical_pin_configuration','confirmed',
    'src/rdx_core/12_ahci_smart_spi_control.inc:core_transfer_register_offsets_register_offset_context selects this table for variants other than 0x38 and configures seven logical pins via core_process_first_pin_initial_pin.',
    'src/rdx_core/18_firmware_transfer_and_persistence.inc:core_process_first_pin_initial_pin accesses spi_get_spi_transfer_storage and spi_process_pin_mask_spi_pc3; these are pin configurations, not register-offset records.',
    f'{binary}:0x0800E398 contains seven 8-byte pin/polarity pairs beginning (18,1),(5,0),(23,0),(12,1).')
add('core_register_offset_context','core_variant_38_spi_logical_pin_configuration','confirmed',
    'src/rdx_core/12_ahci_smart_spi_control.inc:core_transfer_register_offsets_register_offset_context selects this table for variant code 0x38, then applies seven 8-byte SPI logical-pin configurations.',
    f'{binary}:0x0800E3D0 contains seven pin/polarity pairs beginning (18,1),(5,0),(23,0),(12,1).')
add('core_descriptor_selection_context','core_sha256_round_constants','confirmed',
    'src/rdx_core/04_hash_flash_usb_thermal.inc:core_sha256_compress_block consumes one word per each of 64 SHA-256 compression rounds.',
    f'{binary}:0x0800E490 begins the SHA-256 K table 0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5; the table contains 64 words.')
add('core_copy_context','core_copy_record_final_region_descriptor','structural',
    'src/rdx_core/08_thermal_and_record_validation.inc:core_process_remaining_descriptor_space_for_copy_copy_resource caller uses this descriptor for the 0x10-byte destination region at offset 0x50.',
    f'{binary}:0x0800E84C holds the three-byte descriptor 35 03 01, adjacent to the middle, leading and trailing region descriptors.')
add('core_buffer_compare_context','core_record_type_0d_descriptor','structural',
    'src/rdx_core/13_runtime_transfer_helpers.inc:core_process_clear_memory_buffer_compare_context passes this descriptor to core_process_remaining_descriptor_space_for_copy_copy_resource for a 0x20-byte record; 14_mww_usb_command_control.inc uses the same descriptor for the matching update path.',
    f'{binary}:0x0800E858 contains descriptor bytes 0d 04 01.')
add('core_memory_transition_context','core_memory_transition_header_descriptor','structural',
    'src/rdx_core/07_eject_reconnect_smart.inc:core_transition_clear_memory_transition_context dispatches a four-byte status/header record using this descriptor before the 0xa0-byte payload descriptor; 08_thermal_and_record_validation.inc reads the same four-byte record.',
    f'{binary}:0x0800E861 contains descriptor bytes 39 0c 01.')
add('core_target_type_context','core_record_lookup_type_order','confirmed',
    'src/rdx_core/08_thermal_and_record_validation.inc:core_run_target_type_clear_memory_target_type_context iterates four type identifiers for core_run_ready_state_handler, stopping before the requested target type.',
    f'{binary}:0x0800E864 contains uint32 values 0x17,4,6,0x0B.')
add('core_comparison_context','core_record_comparison_descriptors','structural',
    'src/rdx_core/06_record_transfer_helpers.inc:core_transfer_comparison_context traverses ten eight-byte descriptors containing type/group bytes and uint16 status and value offsets; groups successful comparisons by byte 1.',
    f'{binary}:0x0800E874 has ten rows, beginning 02 17 00 04 00 00 0c 00 and ending 37 03 00 04 08 00 30 00.')
add('core_bounded_transfer_context','core_word_cipher_key_byte_permutation','confirmed',
    'src/rdx_core/19_identity_sense_and_mww.inc:core_transfer_bounded_transfer_context copies sixteen source bytes in the index order supplied by this word table.',
    f'{binary}:0x0800E8C4 contains the permutation 9,12,5,1,10,3,13,7,6,15,11,0,4,8,2,14.')
add('core_firmware_record_context','core_firmware_record_callback_enabled_table','confirmed',
    'src/rdx_core/10_spi_flash_and_response_dispatch.inc:core_transition_firmware_context_0x14_firmware_record_context tests the enabled value before calling the corresponding entry in core_firmware_record_callback_table.',
    f'{binary}:0x0800E944 begins eight-byte enabled/callback rows (1,0x0800D31D),(1,0x0800D315), followed by disabled rows.')
add('core_primary_memory_region','core_encrypted_fingerprint_key_seed','structural',
    'src/rdx_core/16_record_builders_and_capacity.inc:core_process_clear_memory_clear_memory_primary_memory_region copies this sixteen-byte seed and decrypts it through core_process_end_remaining_for_ti_memset_ti; core_process_usb_event_context then uses the derived value to encrypt its fingerprint record.',
    f'{binary}:0x0800E97C contains sixteen fixed seed bytes c9 b2 6b cc b8 3d 25 12 48 0f f5 9b f2 cc 52 c7.')
add('core_secondary_memory_region','core_encrypted_response_key_seed','structural',
    'src/rdx_core/16_record_builders_and_capacity.inc:core_process_clear_memory_clear_memory_secondary_memory_region copies and decrypts this sixteen-byte seed, then passes the derived key to core_configure_word_two_byte_one_word_two_byte_two for response-key generation.',
    f'{binary}:0x0800E98C contains sixteen fixed seed bytes 10 18 d6 b6 52 bd 20 9b 2c 64 aa 34 76 08 6c f4.')
add('core_fixed_command_context','core_signature_verification_key_descriptor','structural',
    'src/rdx_core/17_cartridge_state_and_validation.inc:core_process_constant_0x24_fixed_command_context supplies this descriptor to core_transfer_packed_command_words_command_words; that function checks the transformed record against SHA-256 output and encoded padding.',
    f'{binary}:0x0800E9E0 contains 32,0x10001,0x0800E7CC, consistent with a word-count, public exponent and modulus pointer.')
add('core_configuration_update_context','core_cartridge_retry_delay_ms_table','confirmed',
    'src/rdx_core/16_record_builders_and_capacity.inc:core_update_configuration_update_context selects a delay by retry count, then core_monitor_cartridge_input arms core_cartridge_ready_retry_timer with that delay.',
    f'{binary}:0x0800EA00 contains uint32 delays 10000,10000,16000,5000 milliseconds; the final word is also core_configuration_default_delay_ms.')
add('core_payload_response_context','core_supported_attribute_identifiers','confirmed',
    'src/rdx_core/01_image_validation.inc:core_handle_payload_byte_type_or_response_payload_response_context serializes three two-byte attribute identifiers from this table into the supported attribute-list response.',
    f'{binary}:0x0800EA1C contains 0x0803,0x0805,0x0806.')
add('core_secondary_update_context','core_encoded_word_cipher_delta','confirmed',
    'src/rdx_core/22_state_queries_and_hardware_helpers.inc:core_update_secondary_update_context decodes four bytes via core_transfer_x01; word-cipher encryption and decryption use that decoded word as their round sum delta.',
    f'{binary}:0x0800EA48 contains 0d ce 8d f5; subtracting byte index and 0x54 yields b9 79 37 9e, the little-endian delta 0x9E3779B9.')

usb_map=[
('core_endpoint_context','usb_device_connection_state','dev_state','core_dispatch_response_buffer checks DEFAULT/ADDRESSED/CONFIGURED transitions for standard requests.'),
('core_firmware_transfer_context','usb_ep0_state','ep0_state','core_prepare_command_transfer_direction and both completion handlers implement states 0 through 5 matching eEP0_STATE_T.'),
('core_descriptor_configuration_context','usb_ep0_three_stage_transfer','ep0_three_stage_xfer','core_configure_descriptor_configuration_context selects status-TRB type 3 or 4 according to this flag.'),
('core_mww_deferred_transfer_pending','usb_ep0_zero_length_packet_pending','ep0_zlp_pending','core_prepare_command_transfer_direction sets it when a zero-length packet must complete the data stage.'),
('core_mww_transfer_restart_pending','usb_ep0_setup_configuration_pending','bConfigEP0SetupXfer','usb_hal_init sets it after EP0 configuration; the connection handler clears it after posting the first setup transfer.'),
('core_usb_transfer_processing_marker','usb_disconnect_countdown','dDisconnectCountDown','The connection handler sets this field to 0x14.'),
('core_descriptor_queue_context','usb_active_mass_storage_class','active_mass_storage_class','The BOT interface selector stores 0 and the UAS interface selector stores 1; TRB lengths and NCQ gates select by that mode.'),
('core_word_update_context','usb_serial_number_string_index','bSerialNumStringDescIndex','core_update_word_update_context copies device descriptor byte 0x10, the iSerialNumber field.'),
('core_usb_configuration_attribute_bit_six','usb_self_powered_capable','bSelfPoweredCapable','core_update_word_update_context copies configuration bmAttributes bit 6; USB_CD_ATTR_SELF_POWERED_BIT is 0x40.'),
('core_usb_configuration_attribute_bit_five','usb_remote_wakeup_capable','bRemoteWakeupCapable','core_update_word_update_context copies configuration bmAttributes bit 5; USB_CD_ATTR_REMOTE_WAKEUP_BIT is 0x20.'),
('core_usb_response_capability_active','usb_remote_wakeup_enabled','bRemoteWakeupEnabled[0]','USB GET_STATUS code exposes this as the remote-wakeup enabled bit and indexes the neighboring flags for interfaces.'),
('core_usb_reset_required','usb_test_mode','wTestMode','The EP0 status completion path enters the requested USB test mode when this field is nonzero.'),
('core_usb_dispatch_case_eight_status','usb_current_configuration','bCurrentConfigNum','core_dispatch_response_buffer request 8 returns this byte; request 9 clears it when configuration is removed.'),
('core_usb_dispatch_case_ten_status','usb_interface_zero_alternate_setting','bCurrentInterfaceAltSetting[0]','core_dispatch_response_buffer request 10 returns this byte; request 11 selects the interface alternate setting.'),
('core_usb_dispatch_reset_companion_byte','usb_interface_one_alternate_setting','bCurrentInterfaceAltSetting[1]','SET_CONFIGURATION clears both alternate-setting bytes; MAX_INTERFACE_NUM is 1.'),
('core_usb_state_three_response_bit_two_condition','usb_u1_enabled','bIsU1Enabled','GET_STATUS at SuperSpeed maps this byte to bit 2, and USB reset clears it.'),
('core_usb_state_three_response_bit_three_condition','usb_u2_enabled','bIsU2Enabled','GET_STATUS at SuperSpeed maps this byte to bit 3, and USB reset clears it.'),
('core_usb_state_three_response_bit_four_condition','usb_ltm_enabled','bIsLTMEnabled','GET_STATUS at SuperSpeed maps this byte to bit 4.'),
('core_firmware_remaining_context','usb_setup_packet','setup_packet','core_process_firmware_transfer_for_mww_transition copies the eight-byte EP0 setup packet here before setup-request callbacks.'),
('core_descriptor_component_context','usb_ep0_out_max_packet_size','ep_info_OUT[0].wMaxPktSize','core_update_usb_transfer_state_for_usb_state sets the speed-selected packet size; endpoint configuration uses the same field with 0x28-byte endpoint stride.'),
('core_mww_out_deferred_transfer_gate','usb_ep0_out_transfer_active','ep_info_OUT[0].bXferActive','core_prepare_command_transfer_direction waits on this OUT endpoint activity flag before restarting a control stage.'),
('core_mww_saved_transfer_completion_word','usb_ep3_out_transfer_active','ep_info_OUT[3].bXferActive','core_process_mww_saved_request_for_mww_handle_saved clears this endpoint-3 activity flag after its remaining length reaches zero.'),
('core_mww_window_transfer_state','usb_ep0_in_stalled','ep_info_IN[0].bStalled','core_get_mww_active_flag selects this IN stall flag or the OUT stall flag according to endpoint direction; src/rdx_mount/usb_hal.c:usb_hal_is_endpt_stalled is the matching implementation.'),
('core_mww_descriptor_component_context','usb_ep0_in_max_packet_size','ep_info_IN[0].wMaxPktSize','core_update_usb_transfer_state_for_usb_state sets this alongside the OUT packet size, and direction-specific endpoint configuration updates it.'),
('core_mww_in_deferred_transfer_gate','usb_ep0_in_transfer_active','ep_info_IN[0].bXferActive','core_prepare_command_transfer_direction waits on this IN endpoint activity flag before restarting a control stage.'),
('core_mww_descriptor_ring_state','usb_in_trb_bytes_remaining','trb_ring_info_IN.bytes_remaining','core_transition_queue_capacity_command_queue_capacity_context decrements this byte count by each published IN TRB length.'),
('core_queue_capacity_context','usb_out_trb_bytes_remaining','trb_ring_info_OUT.bytes_remaining','core_transition_queue_capacity_command_queue_capacity_context decrements this byte count by each published OUT TRB length; it is not ring capacity.'),
]
for old,new,field,detail in usb_map:
    add(old,new,'confirmed','include/ti_reference/usb_hal.h:USB_DEVICE_T and related enums define '+field+'.',
        'src/rdx_core and src/usb_stack.c: '+detail,
        'src/rdx_mount/usb_hal.c contains the corresponding named TI-structure access for endpoint, control-transfer and ring operations.')
add('core_identity_persistence_context','core_persistent_mechanism_completion_count','structural',
    'src/rdx_core/21_persistence_and_state_setters.inc:core_update_identity_persistence_context increments this word and rewrites the persistent state record; its caller in 13_runtime_transfer_helpers.inc does so once the mechanism completion check succeeds.',
    'The field is the word at persistent state record offset 4, immediately after the checksum at 0x0800EC64.')
add('core_primary_response_buffer','core_identity_record_offset_24_word','structural',
    'src/rdx_core/07_eject_reconnect_smart.inc:core_process_clear_memory_primary_response_buffer serializes this scalar uint32 at identity record offset 0x24; 04_hash_flash_usb_thermal.inc reads the same field and 03_identity_and_mechanism.inc defaults it to 1.')
add('core_secondary_response_buffer','core_compact_identity_vendor_identifier','confirmed',
    'src/rdx_core/03_identity_and_mechanism.inc:core_initialize_rdx_identity_records copies the eight-byte core_default_identity_vendor_identifier into this field; 13_runtime_transfer_helpers.inc serializes it at compact identity offset 8 ahead of the 16-byte product identifier.')
add('core_flash_validation_context','core_identity_record_variant_code','structural',
    'src/rdx_core/07_eject_reconnect_smart.inc:core_process_clear_memory_primary_response_buffer serializes this halfword at identity record offset 0x78; 19_identity_sense_and_mww.inc normalizes 0,1,0xFFFF to 0x37.',
    'src/rdx_core/12_ahci_smart_spi_control.inc:core_transfer_register_offsets_register_offset_context selects different SPI pin configurations for code 0x38; no transient validation state is stored here.')
add('core_response_end_context','core_ata_device_discovered','confirmed',
    'src/rdx_core/03_identity_and_mechanism.inc:core_handle_clear_memory_lookup_ata_command_buffer stores whether core_configure_ata_devices_address_ata_device_table succeeded; zero selects the discovery-failed sense path. The symbol begins the broader cartridge runtime record.')
add('core_ata_command_context','core_ata_security_unlock_succeeded','confirmed',
    'src/rdx_core/09_command_phase_and_ata.inc:core_handle_ata_command_opcode_ata_command_context stores 1 only after successful execution of ATA opcode 0xF2 (SECURITY UNLOCK), immediately before rdx_sata_usb_authenticate.')
add('core_ata_standby_context','core_ata_configuration_loaded','structural',
    'src/rdx_core/03_identity_and_mechanism.inc:core_handle_clear_memory_lookup_ata_command_buffer stores the result of core_process_ata_command_buffer_for_clear_memory_lookup; that function populates runtime configuration using core_configure_initialize_output_record and core_process_configuration_block_payload.',
    'src/rdx_core/15_control_transfer_crc_records.inc:core_handle_ata_standby_context checks this flag before releasing runtime cartridge data, then clears it.')
add('core_command_state_context','core_cartridge_monitor_enabled','confirmed',
    'src/rdx_core/03_identity_and_mechanism.inc:core_handle_clear_memory_lookup_ata_command_buffer stores 1 after successful device setup and authentication, starts SMART monitoring and arms the monitor timer.',
    'src/rdx_core/07_eject_reconnect_smart.inc:core_handle_cartridge_monitor_context_for_thermal_control_context returns before monitoring when this flag is zero; 14_mww_usb_command_control.inc uses it to decide whether to rearm that monitor.')
add('core_command_processing_context','core_cartridge_startup_delay_timer','confirmed',
    'src/rdx_core/14_mww_usb_command_control.inc:core_process_clear_memory_response_end_context registers this software timer and arms it for 5000 milliseconds.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:core_process_command_processing_context polls it for variant 0x38; 16_record_builders_and_capacity.inc adds 5000 ms to the first retry delay while it is active.')
add('core_command_completion_context','core_mechanism_transition_timer','confirmed',
    'src/rdx_core/05_device_descriptor_dispatch.inc:core_drive_mechanism_outputs arms this timer for each mechanism state transition, including the explicit 500 ms settling delay.',
    'src/rdx_core/03_identity_and_mechanism.inc:core_advance_mechanism_state polls this timer to advance mechanism states 1 through 7; 13_runtime_transfer_helpers.inc registers it.')
add('core_scsi_logical_address_context','core_mechanism_input_cycle_state','structural',
    'src/rdx_core/14_mww_usb_command_control.inc:core_update_scsi_logical_address_context samples the mechanism input, stores state 1 after input assertion and state 2 after release; it returns whether state 2 has been reached.',
    'src/rdx_core/05_device_descriptor_dispatch.inc:core_drive_mechanism_outputs resets the state to zero in mechanism states 1 and 6 alongside the input-release latch.')
add('core_sense_update_context','core_mechanism_output_selection','structural',
    'src/rdx_core/20_command_state_accessors.inc:core_update_sense_update_context stores the caller selection when the response-type condition permits it.',
    'src/rdx_core/13_runtime_transfer_helpers.inc:core_update_drive_mechanism_outputs receives this stored selection after mechanism completion; it does not store a SCSI sense record.')
add('core_sense_record_context','core_mechanism_completion_inhibited','structural',
    'src/rdx_core/05_device_descriptor_dispatch.inc:core_drive_mechanism_outputs sets this flag on entering state 1; 14_mww_usb_command_control.inc clears it when logical outputs are reset.',
    'src/rdx_core/17_cartridge_state_and_validation.inc:core_update_sense_record_context reports completion only when this flag is zero, the transition timer expired, and output state is 4 or 8.')
add('core_mechanism_update_context','core_cartridge_eject_pending','confirmed',
    'src/rdx_core/21_persistence_and_state_setters.inc:core_update_mechanism_update_context_for_advance_mechanism_state sets the pending flag.',
    'src/rdx_core/07_eject_reconnect_smart.inc:core_coordinate_safe_cartridge_eject checks it before eject coordination and clears it after completing the sequence.')
add('core_response_tail_storage','core_capacity_high_word_threshold','confirmed',
    'src/rdx_core/04_hash_flash_usb_thermal.inc:core_handle_response_end_word_response_tail_storage compares the fixed symbol address 0xFFFFFFFE against the high word of the 64-bit READ CAPACITY result; it never accesses storage at this address.',
    'The adjacent 0xFFFFFFFF fixed symbol represents the saturation high word. This 0xFFFFFFFE value is the strict-greater threshold before handling the low word.')

output=Path('temp/naming-audit/core-state-renames.json')
output.write_text(json.dumps(changes,indent=2)+'\n')
print('renames',len(changes))
all_names={r['semantic_name'] for r in manifest}
print('collisions',[r['new'] for r in changes if r['new'] in all_names])
print('duplicate targets',len(changes)-len({r['new'] for r in changes}))
