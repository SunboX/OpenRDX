/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_CORE_H
#define TUSB9261_RDX_CORE_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void core_configure_word_two_byte_one_word_two_byte_two();
/** @brief Firmware procedure. */
void core_configure_ahci_command_header();
/** @brief Firmware procedure. */
void core_configure_descriptor_payload();
/** @brief Firmware procedure. */
int core_configure_range_start();
/** @brief Firmware procedure. */
bool core_configure_ata_devices_address_ata_device_table();
/** @brief Firmware procedure. */
void core_encode_ata_taskfile_lba();
/** @brief Firmware procedure. */
void core_queue_ata_d2h_completion();
/** @brief Firmware procedure. */
void core_queue_ata_pio_completion();
/** @brief Firmware procedure. */
uint32_t core_build_ata_smart_command();
/** @brief Firmware procedure. */
void core_submit_ata_sata_feature_control();
/** @brief Firmware procedure. */
uint32_t core_execute_ata_read_verify();
/** @brief Firmware procedure. */
uint32_t core_parse_typed_payload_record();
/** @brief Firmware procedure. */
bool core_configure_initialize_output_record();
/** @brief Firmware procedure. */
void core_sha256_initialize();
/** @brief Firmware procedure. */
void core_execute_ata_read_dma();
/** @brief Firmware procedure. */
void core_execute_ata_write_dma();
/** @brief Firmware procedure. */
uint64_t core_configure_component_for_parsed_header_descriptor();

/** @brief Firmware procedure. */
uint32_t core_multiply_u32_to_u64();
/** @brief Firmware procedure. */
void core_submit_usb_control_status_trb();
/** @brief Firmware procedure. */
uint32_t core_build_state_dependent_sense_record();
/** @brief Firmware procedure. */
uint64_t core_configure_component_for_secondary_resource_state();

/** @brief Firmware procedure. */
void core_configure_word_descriptor();
/** @brief Firmware procedure. */
int core_validate_structured_flash_image();
/** @brief Firmware procedure. */
void core_build_scsi_mode_sense_response();
/** @brief Firmware procedure. */
void core_handle_scsi_read_attribute();
/** @brief Firmware procedure. */
uint32_t core_divide_multiword_unsigned();

/** @brief Firmware procedure. */
void core_handle_scsi_write_attribute();
/** @brief Firmware procedure. */
uint32_t core_handle_device_descriptor_response_flag_device_descriptor_context();
/** @brief Firmware procedure. */
uint64_t core_dispatch_rdx_management_record();
/** @brief Firmware procedure. */
void core_initialize_rdx_identity_records();
/** @brief Firmware procedure. */
void core_handle_scsi_mode_select();
/** @brief Firmware procedure. */
void core_monitor_cartridge_input();
/** @brief Firmware procedure. */
void core_handle_clear_memory_lookup_ata_command_buffer();
/** @brief Firmware procedure. */
void core_advance_mechanism_state();
/** @brief Firmware procedure. */
void core_select_rdx_sense_status();
/** @brief Firmware procedure. */
void core_handle_scsi_read_capacity();
/** @brief Firmware procedure. */
uint32_t core_handle_descriptor_clear_memory_device_descriptor_context();
/** @brief Firmware procedure. */
int core_handle_device_ti_memset();
/** @brief Firmware procedure. */
uint32_t core_handle_lookup_key_metadata_device_descriptor_context();
/** @brief Firmware procedure. */
uint32_t core_handle_terminal_found_lookup_succeeded();
/** @brief Firmware procedure. */
uint32_t core_handle_response_base_device_descriptor_context();
/** @brief Firmware procedure. */
uint32_t core_handle_descriptor_device();
/** @brief Firmware procedure. */
void core_handle_scsi_write_buffer();
/** @brief Firmware procedure. */
bool core_handle_descriptor_request_bytes();
/** @brief Firmware procedure. */
int core_handle_fallback_decoded_request_bytes();
/** @brief Firmware procedure. */
uint32_t core_handle_word_metadata();
/** @brief Firmware procedure. */
void core_coordinate_safe_cartridge_eject();
/** @brief Firmware procedure. */
void core_handle_cartridge_monitor_context_for_thermal_control_context();
/** @brief Firmware procedure. */
void core_handle_scsi_read_buffer();
/** @brief Firmware procedure. */
void core_handle_primary_sense_context();
/** @brief Firmware procedure. */
uint32_t core_read_smart_temperature();
/** @brief Firmware procedure. */
void core_handle_scsi_get_event_status();
/** @brief Firmware procedure. */
void core_handle_feature_command_feature_state();
/** @brief Firmware procedure. */
void core_handle_cartridge_input_state();
/** @brief Firmware procedure. */
void core_handle_command_phase();
/** @brief Firmware procedure. */
void core_execute_ata_security_unlock();
/** @brief Firmware procedure. */
uint32_t core_handle_structure_field_hex();
/** @brief Firmware procedure. */
int core_handle_metadata_type();

/** @brief Firmware procedure. */
void core_handle_cartridge_input_state_for_monitor_cartridge_input();
/** @brief Firmware procedure. */
void core_handle_pin_input_pin_state();
/** @brief Firmware procedure. */
void core_handle_ata_standby_context_for_cartridge_monitor_context();
/** @brief Firmware procedure. */
void core_handle_cartridge_monitor_context();
/** @brief Firmware procedure. */
void core_handle_ata_standby_context();
/** @brief Firmware procedure. */
void core_handle_thermal_control_context();
/** @brief Firmware procedure. */
int core_handle_scsi_security_protocol();
/** @brief Firmware procedure. */
void core_drive_mechanism_outputs();
/** @brief Firmware procedure. */
uint32_t core_dispatch_response_buffer();
/** @brief Firmware procedure. */
void core_clear_media_record_state_after_accounting();
/** @brief Firmware procedure. */
void core_set_notification_state_to_zero();
/** @brief Firmware procedure. */
bool core_pause_mww_dispatch_for_standby();
/** @brief Firmware procedure. */
void core_forward_cartridge_input_state();
/** @brief Firmware procedure. */
uint8_t core_is_firmware_image_validation_active();
/** @brief Firmware procedure. */
bool core_is_command_phase_three();
/** @brief Firmware procedure. */
uint32_t core_forward_sci_read_logical();
/** @brief Firmware procedure. */
void core_advance_software_timers();
/** @brief Firmware procedure. */
void core_copy_bytes();
/** @brief Firmware procedure. */
uint32_t core_is_supported_forward_opcode();
/** @brief Firmware procedure. */
uint8_t core_is_mww_command_received();
/** @brief Firmware procedure. */
void core_service_usb_reconnect();
/** @brief Firmware procedure. */
uint32_t core_build_supported_record_types_response();
/** @brief Firmware procedure. */
uint32_t core_get_ti_memset();

/** @brief Firmware procedure. */
uint32_t core_reduce_multiword_modulo();

/** @brief Firmware procedure. */
int core_get_set_interrupt_mmio();
/** @brief Firmware procedure. */
uint32_t core_validate_bot_command_wrapper();
/** @brief Firmware procedure. */
uint32_t core_get_end_remaining();

/** @brief Firmware procedure. */
uint32_t core_get_response_buffer();
/** @brief Firmware procedure. */
uint32_t core_multiply_multiword_modulo();

/** @brief Firmware procedure. */
uint32_t core_calculate_record_checksum();
/** @brief Firmware procedure. */
uint32_t core_build_record_type_twelve_response();
/** @brief Firmware procedure. */
uint32_t core_build_record_type_eleven_response();
/** @brief Firmware procedure. */
uint32_t core_build_record_type_one_response();
/** @brief Firmware procedure. */
uint64_t core_get_word();
/** @brief Firmware procedure. */
uint32_t core_get_range_end_range_start();
/** @brief Firmware procedure. */
uint32_t core_get_transfer_transfer();
/** @brief Firmware procedure. */
bool core_verify_ata_media_and_refresh_state();
/** @brief Firmware procedure. */
uint64_t core_get_structure_field_hex_for_response_end_word();
/** @brief Firmware procedure. */
int core_get_runtime_context_base_for_command_descriptor_device();
/** @brief Firmware procedure. */
uint32_t core_is_scsi_write_opcode();
/** @brief Firmware procedure. */
bool core_start_ahci_command_override_if_stopped();
/** @brief Firmware procedure. */
uint32_t core_build_record_type_two_response();
/** @brief Firmware procedure. */
uint32_t core_build_status_control_byte();
/** @brief Firmware procedure. */
uint32_t core_validate_combined_record_bounds();
/** @brief Firmware procedure. */
bool core_get_persistent_identity_record_for_initialize_rdx_identity();
/** @brief Firmware procedure. */
uint32_t core_get_expected_candidate();
/** @brief Firmware procedure. */
int16_t core_get_identity_variant_code();
/** @brief Firmware procedure. */
uint64_t core_pack_descriptor_validation_result();
/** @brief Firmware procedure. */
uint32_t core_get_mode_sense_block_descriptor_length();
/** @brief Firmware procedure. */
int core_select_scsi_response_buffer();
/** @brief Firmware procedure. */
uint32_t core_is_usb_endpoint_stalled();
/** @brief Firmware procedure. */
uint32_t core_is_regular_request_code();
/** @brief Firmware procedure. */
bool core_get_0x40();
/** @brief Firmware procedure. */
int core_validate_device_descriptor_length();
/** @brief Firmware procedure. */
uint32_t core_has_device_descriptor_high_bit();
/** @brief Firmware procedure. */
bool core_get_pending_transfer_buffer();
/** @brief Firmware procedure. */
uint32_t core_scsi_write_requests_fua();
/** @brief Firmware procedure. */
bool core_usb_revision_uses_deferred_eject();
/** @brief Firmware procedure. */
uint32_t core_get_notification_state();
/** @brief Firmware procedure. */
int core_get_structure_field_hex_for_structure_field_hex();
/** @brief Firmware procedure. */
uint32_t core_get_sata_link_speed();
/** @brief Firmware procedure. */
bool core_is_cartridge_input_state_six();
/** @brief Firmware procedure. */
uint8_t core_get_constant_zero_value_for_pin_resource_state();
/** @brief Firmware procedure. */
uint8_t core_get_0xc_for_lookup_key_metadata();
/** @brief Firmware procedure. */
bool core_request_mww_dispatch_pause();
/** @brief Firmware procedure. */
bool core_is_command_phase_three_for_build_scsi_mode();
/** @brief Firmware procedure. */
bool core_is_software_timer_expired();
/** @brief Firmware procedure. */
uint32_t core_has_active_command_flags();
/** @brief Firmware procedure. */
uint8_t core_get_0xc_for_device_descriptor_response();
/** @brief Firmware procedure. */
bool core_is_not_cartridge_input_state_zero();
/** @brief Firmware procedure. */
uint32_t core_is_command_code_in_standard_range();
/** @brief Firmware procedure. */
bool core_get_flash_validation_context_for_flash_validation_status();
/** @brief Firmware procedure. */
uint64_t core_pack_context_value_pair();
/** @brief Firmware procedure. */
uint32_t core_get_validation_flags();
/** @brief Firmware procedure. */
bool core_is_command_phase_zero();
/** @brief Firmware procedure. */
uint32_t core_get_command_phase_flags();
/** @brief Firmware procedure. */
uint32_t core_get_available_primary_mask();
/** @brief Firmware procedure. */
uint32_t core_get_available_secondary_mask();
/** @brief Firmware procedure. */
uint32_t core_saturating_add_u32();
/** @brief Firmware procedure. */
uint32_t core_get_x02();
/** @brief Firmware procedure. */
uint32_t core_repack_shifted_value();
/** @brief Firmware procedure. */
uint32_t core_get_sense_selection_flags_value();
/** @brief Firmware procedure. */
uint8_t *core_get_cartridge_ata_device_info();
/** @brief Firmware procedure. */
uint32_t *core_get_cartridge_monitor_context();
/** @brief Firmware procedure. */
uint8_t core_is_ata_standby_active();
/** @brief Firmware procedure. */
uint8_t core_get_primary_sata_link_speed();
/** @brief Firmware procedure. */
uint8_t *core_get_unit_serial_number();
/** @brief Firmware procedure. */
uint8_t core_get_firmware_image_validation_active();
/** @brief Firmware procedure. */
char core_convert_nibble_to_hex_character();
/** @brief Firmware procedure. */
uint32_t core_get_set_validation_flags();
/** @brief Firmware procedure. */
uint8_t core_is_mww_dispatch_blocked();
/** @brief Firmware procedure. */
uint8_t core_get_mww_command_received();
/** @brief Firmware procedure. */
uint32_t core_get_notification_mode();
/** @brief Firmware procedure. */
uint8_t core_get_device_status_value();
/** @brief Firmware procedure. */
uint8_t core_is_command_write_blocked();
/** @brief Firmware procedure. */
uint8_t core_scsi_write_requests_fua_state_value();
/** @brief Firmware procedure. */
uint8_t core_get_response_type_value();
/** @brief Firmware procedure. */
uint8_t core_get_command_buffer_state_value();
/** @brief Firmware procedure. */
uint8_t core_get_response_status_value();
/** @brief Firmware procedure. */
uint8_t core_get_response_end_context_for_response_end_word();
/** @brief Firmware procedure. */
uint8_t core_get_persistent_identity_record_for_notification_context();
/** @brief Firmware procedure. */
uint8_t *core_get_ata_discovery_flags();
/** @brief Firmware procedure. */
uint32_t *core_get_persistent_identity_record_pointer();
/** @brief Firmware procedure. */
uint32_t core_get_runtime_context_base_for_build_scsi_mode();
/** @brief Firmware procedure. */
uint8_t core_read_context_field_6c_byte();
/** @brief Firmware procedure. */
uint8_t core_get_structure_field_hex_70_value();
/** @brief Firmware procedure. */
uint32_t core_reduce_multiword_modulo_requested();
/** @brief Firmware procedure. */
uint32_t core_get_scsi_response_buffer_capacity();
/** @brief Firmware procedure. */
uint32_t core_get_transfer_buffer_capacity();
/** @brief Firmware procedure. */
uint32_t core_read_mode_page_field_1c_value();
/** @brief Firmware procedure. */
uint32_t core_read_mode_page_field_48_value();
/** @brief Firmware procedure. */
uint32_t core_read_context_field_28_value();
/** @brief Firmware procedure. */
uint32_t core_read_descriptor_field_24_value();
/** @brief Firmware procedure. */
uint32_t core_read_descriptor_field_20_value();
/** @brief Firmware procedure. */
uint32_t core_get_structure_field_hex_4c_value();
/** @brief Firmware procedure. */
uint32_t core_get_structure_field_hex_50_value();
/** @brief Firmware procedure. */
uint32_t core_get_default_record_type_code();
/** @brief Firmware procedure. */
uint32_t core_read_buffer_word();
/** @brief Firmware procedure. */
uint32_t core_get_primary_success_flag();
/** @brief Firmware procedure. */
uint32_t core_get_secondary_success_flag();
/** @brief Firmware procedure. */
uint32_t core_get_constant_one_value_for_ata_standby_context();
/** @brief Firmware procedure. */
uint32_t core_get_condition_response_code();
/** @brief Firmware procedure. */
uint32_t core_get_extended_response_code();
/** @brief Firmware procedure. */
uint32_t core_get_constant_zero_value_for_mww_handle_transfer();
/** @brief Firmware procedure. */
uint32_t core_get_default_mode_page_flags();
/** @brief Firmware procedure. */
uint32_t core_get_constant_one_value_for_request_request_for();
/** @brief Firmware procedure. */
uint32_t core_process_parsed_header_descriptor();
/** @brief Firmware procedure. */
uint32_t core_process_ti_memset_for_type_type();
/** @brief Firmware procedure. */
void core_process_clear_memory_primary_response_buffer();
/** @brief Firmware procedure. */
void core_process_channel_completion_usb_transfer_state();
/** @brief Firmware procedure. */
void core_process_response_status_value();

/** @brief Firmware procedure. */
bool core_normalize_transfer_status_fields();
/** @brief Firmware procedure. */
uint32_t core_process_word_words_device_descriptor_context();
/** @brief Firmware procedure. */
bool core_accumulate_split_transfer_offsets();
/** @brief Firmware procedure. */
bool core_process_copy_context();
/** @brief Firmware procedure. */
int core_process_x01_for_retries_remaining_ti();
/** @brief Firmware procedure. */
void core_process_component_descriptor_component_context();
/** @brief Firmware procedure. */
void core_process_mww_saved_request_for_mww_handle_saved();
/** @brief Firmware procedure. */
int core_process_x01_for_comparison_comparison_resource();
/** @brief Firmware procedure. */
uint32_t core_append_typed_payload_record();
/** @brief Firmware procedure. */
uint32_t core_process_secondary_command_context();
/** @brief Firmware procedure. */
uint32_t core_process_x01_for_advance_mechanism_state();
/** @brief Firmware procedure. */
uint32_t core_handle_usb_bot_class_request();
/** @brief Firmware procedure. */
uint32_t core_process_word_validity_flag_command_validation_context();
/** @brief Firmware procedure. */
void core_process_remaining_descriptor_space_for_transfer_completion_flag();
/** @brief Firmware procedure. */
int core_copy_and_pad_record_payload();
/** @brief Firmware procedure. */
int core_read_mcp3008_channel();
/** @brief Firmware procedure. */
uint32_t core_submit_ata_identify_command();
/** @brief Firmware procedure. */
int core_program_spi_flash_pages();
/** @brief Firmware procedure. */
uint32_t core_process_device_use_lookup();
/** @brief Firmware procedure. */
uint32_t core_process_runtime_context_base_for_component();
/** @brief Firmware procedure. */
uint32_t core_submit_ata_set_transfer_mode();
/** @brief Firmware procedure. */
void core_process_end_remaining();
/** @brief Firmware procedure. */
uint32_t core_process_descriptor_type_ti_memset();
/** @brief Firmware procedure. */
void core_process_response_flag_x01();
/** @brief Firmware procedure. */
uint32_t core_execute_ata_standby_or_verify();
/** @brief Firmware procedure. */
void core_build_type_two_fingerprint();
/** @brief Firmware procedure. */
void core_build_encrypted_password_record();
/** @brief Firmware procedure. */
void core_dispatch_usb_control_completion();
/** @brief Firmware procedure. */
int core_process_transfer_requested();
/** @brief Firmware procedure. */
uint8_t core_initialize_smart_monitoring();
/** @brief Firmware procedure. */
uint32_t core_dispatch_transfer_state_update();

/** @brief Firmware procedure. */
int core_process_component_for_device_ti_memset();
/** @brief Firmware procedure. */
void core_process_expected_flag_expected();

/** @brief Firmware procedure. */
void core_process_request_request_for_request_request();
/** @brief Firmware procedure. */
int core_submit_scsi_ata_command();
/** @brief Firmware procedure. */
void core_configure_usb_endpoint();
/** @brief Firmware procedure. */
void core_complete_usb_control_out_transfer();
/** @brief Firmware procedure. */
void core_build_ata_identity_summary();
/** @brief Firmware procedure. */
uint32_t core_execute_ata_flush_cache();
/** @brief Firmware procedure. */
uint8_t core_process_clear_memory_buffer_compare_context();
/** @brief Firmware procedure. */
int core_process_component_for_command_sequence_update();
/** @brief Firmware procedure. */
void core_initialize_cartridge_mechanism_inputs();
/** @brief Firmware procedure. */
uint32_t core_process_runtime_context_base_for_rdx_management_record();
/** @brief Firmware procedure. */
uint64_t core_process_component_for_parsed_header_descriptor();

/** @brief Firmware procedure. */
void core_process_mww_saved_request();
/** @brief Firmware procedure. */
void core_advance_control_transfer_state();
/** @brief Firmware procedure. */
int core_process_remaining_descriptor_space_for_copy_copy_resource();
/** @brief Firmware procedure. */
void core_process_flag_one_for_spi_response_context();
/** @brief Firmware procedure. */
uint32_t core_end_usb_endpoint_transfer();
/** @brief Firmware procedure. */
void core_process_clear_memory_secondary_response_buffer();
/** @brief Firmware procedure. */
void core_process_mww_command_state();
/** @brief Firmware procedure. */
bool core_process_buffer_compare_context();
/** @brief Firmware procedure. */
void core_reset_logical_outputs();
/** @brief Firmware procedure. */
int core_process_ata_standby_context();
/** @brief Firmware procedure. */
uint32_t core_process_device_address();
/** @brief Firmware procedure. */
uint32_t core_validate_firmware_image_chunk();
/** @brief Firmware procedure. */
void core_process_primary_command_context();
/** @brief Firmware procedure. */
void core_process_command_descriptor_context();
/** @brief Firmware procedure. */
void core_arm_usb_control_setup_reception();
/** @brief Firmware procedure. */
void core_process_usb_transfer_state();
/** @brief Firmware procedure. */
uint32_t core_build_indexed_transfer_descriptor();

/** @brief Firmware procedure. */
uint32_t core_process_ata_command_for_select_rdx_sense();
/** @brief Firmware procedure. */
void core_process_clear_memory_response_end_context();
/** @brief Firmware procedure. */
void core_process_command_state_context();
/** @brief Firmware procedure. */
uint32_t core_write_validated_flash_data();
/** @brief Firmware procedure. */
void core_process_clear_memory_spi_response_context();
/** @brief Firmware procedure. */
bool core_process_ata_command_buffer_for_mww_handle_transfer();
/** @brief Firmware procedure. */
void core_process_clear_memory_command_phase();
/** @brief Firmware procedure. */
void core_finalize_control_transfer_state();
/** @brief Firmware procedure. */
void core_build_type_one_fingerprint();
/** @brief Firmware procedure. */
void core_process_request_request_for_descriptor_descriptor_resource();
/** @brief Firmware procedure. */
void core_initialize_transfer_workspace();
/** @brief Firmware procedure. */
void core_process_secondary();
/** @brief Firmware procedure. */
uint32_t core_initialize_output_record_from_source();
/** @brief Firmware procedure. */
void core_enable_request_flag_eight();
/** @brief Firmware procedure. */
void core_enable_request_flag_sixteen();
/** @brief Firmware procedure. */
void core_process_flash_validation_status();
/** @brief Firmware procedure. */
void core_process_command_phase();
/** @brief Firmware procedure. */
void core_handle_usb_bus_reset();
/** @brief Firmware procedure. */
void core_apply_pwm_percentage();
/** @brief Firmware procedure. */
bool core_process_reset_output_record();
/** @brief Firmware procedure. */
void core_reset_output_record();
/** @brief Firmware procedure. */
uint64_t core_process_clear_memory_clear_memory_secondary_memory_region();
/** @brief Firmware procedure. */
uint64_t core_process_clear_memory_clear_memory_primary_memory_region();
/** @brief Firmware procedure. */
uint64_t core_process_clear_memory_clear_memory_copy_context();
/** @brief Firmware procedure. */
uint32_t core_process_command_code_in();
/** @brief Firmware procedure. */
void core_initialize_pwm_channel();
/** @brief Firmware procedure. */
void core_process_notify_change_notification_context();
/** @brief Firmware procedure. */
void core_process_configuration();
/** @brief Firmware procedure. */
void core_copy_bounded_response_field();

/** @brief Firmware procedure. */
uint32_t core_process_capacity_block_context();
/** @brief Firmware procedure. */
void core_process_bitset_base();
/** @brief Firmware procedure. */
void core_reset_sata_link();
/** @brief Firmware procedure. */
void core_process_cartridge_input_state();
/** @brief Firmware procedure. */
bool core_process_ata_command_for_lookup_key_metadata();
/** @brief Firmware procedure. */
uint64_t core_process_ti_memset_ti_memset();
/** @brief Firmware procedure. */
void core_process_copy();
/** @brief Firmware procedure. */
void core_initialize_cartridge_monitor();
/** @brief Firmware procedure. */
uint32_t core_is_cartridge_startup_delay_active();
/** @brief Firmware procedure. */
uint32_t core_process_constant_0x24_fixed_command_context();
/** @brief Firmware procedure. */
uint32_t core_process_ata_command_buffer_for_clear_memory_lookup();
/** @brief Firmware procedure. */
void core_process_configuration_block_payload();
/** @brief Firmware procedure. */
void core_hold_logical_output_fourteen();
/** @brief Firmware procedure. */
void core_abort_failed_usb_control_transfer();
/** @brief Firmware procedure. */
void core_process_ti_memset_for_initialize_transfer_workspace();
/** @brief Firmware procedure. */
uint32_t core_process_sci_read_logical_for_monitor_cartridge_input();
/** @brief Firmware procedure. */
bool core_compare_record_payloads();
/** @brief Firmware procedure. */
void core_process_first_pin_initial_pin();
/** @brief Firmware procedure. */
uint16_t *core_initialize_scsi_response_buffer();
/** @brief Firmware procedure. */
void core_write_bounded_response_field();
/** @brief Firmware procedure. */
uint32_t core_dispatch_available_record();

/** @brief Firmware procedure. */
void core_reset_flash_validation_context();
/** @brief Firmware procedure. */
void core_initialize_led_controller_registry();
/** @brief Firmware procedure. */
void core_initialize_usb_reconnect_state();
/** @brief Firmware procedure. */
void core_reconnect_device_and_start_timer();
/** @brief Firmware procedure. */
void core_process_mww_process_command();
/** @brief Firmware procedure. */
uint32_t core_process_mww_update_resource();
/** @brief Firmware procedure. */
void core_copy_record_header_to_destination();
/** @brief Firmware procedure. */
void core_submit_ata_identify_device();
/** @brief Firmware procedure. */
void core_rewrite_unit_identity_record();
/** @brief Firmware procedure. */
void core_process_lookup_arguments();
/** @brief Firmware procedure. */
uint32_t core_process_halt_forever_for_selected_or_bit();
/** @brief Firmware procedure. */
uint32_t core_process_expected_candidate();
/** @brief Firmware procedure. */
void core_request_ata_standby_when_idle();
/** @brief Firmware procedure. */
void core_apply_thermal_pwm_override();
/** @brief Firmware procedure. */
void core_save_persistent_state_with_checksum();
/** @brief Firmware procedure. */
void core_rewrite_primary_persistent_state();
/** @brief Firmware procedure. */
uint32_t core_process_runtime_context_base_for_cartridge_input_state();
/** @brief Firmware procedure. */
uint32_t core_process_sense_record_context();
/** @brief Firmware procedure. */
uint32_t core_process_flash_validation_context();
/** @brief Firmware procedure. */
uint32_t core_process_command_context_value();
/** @brief Firmware procedure. */
void core_process_command_feature_state();
/** @brief Firmware procedure. */
void core_process_runtime_context_base();
/** @brief Firmware procedure. */
uint32_t core_process_request_for_mww_saved_request();
/** @brief Firmware procedure. */
void core_write_selected_sense_fields();
/** @brief Firmware procedure. */
void core_process_set_validation_flags();
/** @brief Firmware procedure. */
void core_restart_ahci_port_initialization();
/** @brief Firmware procedure. */
uint32_t core_check_logical_input_and_state();
/** @brief Firmware procedure. */
uint32_t core_process_identity_update_context();
/** @brief Firmware procedure. */
uint32_t core_process_sci_read_logical_for_resource_usb_state();
/** @brief Firmware procedure. */
uint32_t core_run_ready_state_handler();
/** @brief Firmware procedure. */
uint32_t core_check_idle_input_and_state();
/** @brief Firmware procedure. */
void core_process_device_status_value();
/** @brief Firmware procedure. */
uint16_t core_process_ata_command_buffer_for_build_scsi_mode();
/** @brief Firmware procedure. */
uint32_t core_process_command_phase_three();
/** @brief Firmware procedure. */
void core_initialize_smart_temperature_timer();
/** @brief Firmware procedure. */
void core_process_end_remaining_for_ti_memset_ti();
/** @brief Firmware procedure. */
void core_process_prepare_runtime_services();
/** @brief Firmware procedure. */
void core_process_set_notification_state();
/** @brief Firmware procedure. */
void core_reset_mechanism_outputs_when_idle();
/** @brief Firmware procedure. */
void core_process_persistent_identity_record_for_notify_change_resource();
/** @brief Firmware procedure. */
void core_process_device_status_update();
/** @brief Firmware procedure. */
void core_process_validation_flags();
/** @brief Firmware procedure. */
void core_process_notification_context();
/** @brief Firmware procedure. */
void core_halt_forever();
/** @brief Firmware procedure. */
uint64_t core_run_ti_memset_component();
/** @brief Firmware procedure. */
int core_run_wdt_reset_remaining();
/** @brief Firmware procedure. */
int core_run_target_type_clear_memory_target_type_context();
/** @brief Firmware procedure. */
uint32_t core_run_expected_flag_expected_type();
/** @brief Firmware procedure. */
uint32_t core_run_requested_available_availability_request();
/** @brief Firmware procedure. */
void core_parse_ata_identify_device();
/** @brief Firmware procedure. */
uint32_t core_sha256_compress_block();
/** @brief Firmware procedure. */
void core_sha256_finalize();
/** @brief Firmware procedure. */
uint64_t core_transfer_descriptor_capacity_previous_descriptor_address_descriptor_queue_context();
/** @brief Firmware procedure. */
uint64_t core_execute_spi_flash_operation();
/** @brief Firmware procedure. */
uint32_t core_build_usb_two_configuration_descriptor();
/** @brief Firmware procedure. */
int core_transfer_comparison_context();
/** @brief Firmware procedure. */
void core_dispatch_scsi_command();
/** @brief Firmware procedure. */
uint32_t core_build_superspeed_configuration_descriptor();
/** @brief Firmware procedure. */
uint32_t core_format_hex_u32_command_words_command_words();
/** @brief Firmware procedure. */
void core_decrypt_keyed_word_block();
/** @brief Firmware procedure. */
uint32_t core_multiply_multiword_unsigned();
/** @brief Firmware procedure. */
uint32_t core_modular_exponentiation_multiword();

/** @brief Firmware procedure. */
uint32_t core_transfer_transfer_completion_flag();
/** @brief Firmware procedure. */
void core_transfer_command_sequence_update_flag();
/** @brief Firmware procedure. */
uint32_t core_transfer_match_found_expected_type();
/** @brief Firmware procedure. */
void core_encrypt_keyed_word_block();
/** @brief Firmware procedure. */
uint32_t core_transfer_firmware_context_remaining_firmware_remaining_context();
/** @brief Firmware procedure. */
uint64_t core_select_ata_transfer_mode();
/** @brief Firmware procedure. */
uint32_t core_transfer_response_byte_stack_response_byte();
/** @brief Firmware procedure. */
void core_decode_big_endian_multiword_integer();
/** @brief Firmware procedure. */
uint32_t core_transfer_retries_remaining_ti_memset();
/** @brief Firmware procedure. */
void core_copy_reversed_field_bytes();
/** @brief Firmware procedure. */
uint32_t core_write_big_endian_u32_field();
/** @brief Firmware procedure. */
uint64_t core_divide_multiword_by_word();
/** @brief Firmware procedure. */
uint32_t core_find_usb_descriptor();
/** @brief Firmware procedure. */
uint32_t core_copy_descriptor_text_field();
/** @brief Firmware procedure. */
uint32_t core_shift_multiword_right();
/** @brief Firmware procedure. */
int core_find_typed_payload_with_minimum_length();
/** @brief Firmware procedure. */
uint32_t core_shift_multiword_left();
/** @brief Firmware procedure. */
uint32_t core_send_spi_flash_command_address();
/** @brief Firmware procedure. */
void core_configure_board_logical_pins();
/** @brief Firmware procedure. */
uint64_t core_transfer_found_flag();
/** @brief Firmware procedure. */
void core_service_led_patterns();
/** @brief Firmware procedure. */
uint64_t core_read_reversed_field_value();
/** @brief Firmware procedure. */
int core_transfer_remaining_bounds();
/** @brief Firmware procedure. */
uint32_t core_test_transfer_buffer_memory();
/** @brief Firmware procedure. */
void core_append_little_endian_value();
/** @brief Firmware procedure. */
uint32_t core_read_big_endian_u32_field();
/** @brief Firmware procedure. */
uint32_t core_read_big_endian_record_field();
/** @brief Firmware procedure. */
void core_copy_memory_bytes();
/** @brief Firmware procedure. */
void core_add_multiword_unsigned();
/** @brief Firmware procedure. */
uint32_t core_calculate_crc32();
/** @brief Firmware procedure. */
void core_transfer_interrupt_transfer_context();
/** @brief Firmware procedure. */
uint32_t core_wait_usb_endpoint_command();
/** @brief Firmware procedure. */
void core_transfer_usb_endpoint_transfer_for_response_buffer();
/** @brief Firmware procedure. */
void core_transfer_usb_endpoint_transfer();
/** @brief Firmware procedure. */
void core_read_buffer_bytes();
/** @brief Firmware procedure. */
void core_append_buffer_bytes();
/** @brief Firmware procedure. */
uint32_t core_accumulate_response_byte_lanes();
/** @brief Firmware procedure. */
uint32_t core_accumulate_image_byte_lanes();
/** @brief Firmware procedure. */
uint32_t core_compare_multiword_integers();
/** @brief Firmware procedure. */
void core_reset_ata_callback_queue();
/** @brief Firmware procedure. */
void core_decode_indexed_bytes();
/** @brief Firmware procedure. */
int core_compare_buffers_from_end();
/** @brief Firmware procedure. */
uint8_t *core_find_scsi_opcode_entry();
/** @brief Firmware procedure. */
void core_format_hex_u32();
/** @brief Firmware procedure. */
uint32_t core_find_value_in_fixed_table();
/** @brief Firmware procedure. */
void core_dispatch_usb_power_management_callbacks();
/** @brief Firmware procedure. */
void core_initialize_word_buffer();
/** @brief Firmware procedure. */
void core_permute_word_cipher_key_bytes();
/** @brief Firmware procedure. */
int core_find_last_nonzero_word();
/** @brief Firmware procedure. */
void core_copy_bytes_to_alternate_positions();
/** @brief Firmware procedure. */
void core_swap_adjacent_bytes();
/** @brief Firmware procedure. */
void core_copy_word_buffer();
/** @brief Firmware procedure. */
int core_transfer_constant_0x13();
/** @brief Firmware procedure. */
void core_zero_word_buffer();
/** @brief Firmware procedure. */
void core_sha256_update();
/** @brief Firmware procedure. */
void core_finalize_identity_initialization_hook();
/** @brief Firmware procedure. */
void core_reset_command_context_hook();
/** @brief Firmware procedure. */
void core_prepare_runtime_services_hook();
/** @brief Firmware procedure. */
void core_finalize_command_response_hook();
/** @brief Firmware procedure. */
void core_unused_update_hook();
/** @brief Firmware procedure. */
void core_unused_secondary_update_hook();
/** @brief Firmware procedure. */
void core_update_usb_transfer_state_for_usb_state();
/** @brief Firmware procedure. */
int core_update_validation_0x20();
/** @brief Firmware procedure. */
uint32_t core_update_usb_transfer_state_for_response_buffer();
/** @brief Firmware procedure. */
uint32_t core_issue_ahci_command_slot();
/** @brief Firmware procedure. */
uint32_t core_update_descriptor_requested();
/** @brief Firmware procedure. */
void core_enqueue_ata_completion_callback();
/** @brief Firmware procedure. */
void core_queue_ata_set_device_bits_completion();
/** @brief Firmware procedure. */
void core_load_flash_configuration_byte();
/** @brief Firmware procedure. */
uint32_t core_update_usb_transfer_state();
/** @brief Firmware procedure. */
void core_update_word_update_context();
/** @brief Firmware procedure. */
void core_queue_ata_dma_setup_completion();
/** @brief Firmware procedure. */
bool core_update_mechanism_input_release();
/** @brief Firmware procedure. */
void core_update_rti_clock_mhz_period_units();
/** @brief Firmware procedure. */
void core_update_word_mww_transfer_word();
/** @brief Firmware procedure. */
uint32_t core_submit_control_transfer_fields();

/** @brief Firmware procedure. */
uint16_t core_update_constant_0xc();
/** @brief Firmware procedure. */
uint32_t core_update_decoded_request();
/** @brief Firmware procedure. */
void core_set_validation_flags_flag_bit_three();
/** @brief Firmware procedure. */
void core_set_validation_flags_flag_bit_five();
/** @brief Firmware procedure. */
uint32_t core_update_response_flag();
/** @brief Firmware procedure. */
uint32_t core_update_descriptor_subtraction();
/** @brief Firmware procedure. */
bool core_update_structure_field_hex();
/** @brief Firmware procedure. */
void core_update_spi_set_logical_for_x01_for_resource();
/** @brief Firmware procedure. */
uint32_t core_update_state_constant_137_three();
/** @brief Firmware procedure. */
bool core_update_command_phase();
/** @brief Firmware procedure. */
void core_notify_usb_disconnect();
/** @brief Firmware procedure. */
bool core_register_usb_request_callback();
/** @brief Firmware procedure. */
uint32_t core_update_availability_x01();
/** @brief Firmware procedure. */
uint32_t core_validate_transfer_capacity();
/** @brief Firmware procedure. */
int core_get_configured_delay_ms();
/** @brief Firmware procedure. */
uint32_t core_update_capacity_block_context();
/** @brief Firmware procedure. */
void core_allocate_software_timer();
/** @brief Firmware procedure. */
void core_update_descriptor_response_metadata();
/** @brief Firmware procedure. */
bool core_commit_firmware_image_header();
/** @brief Firmware procedure. */
uint32_t core_is_adc_sample_above_threshold();
/** @brief Firmware procedure. */
void core_set_channel_mode_bits();
/** @brief Firmware procedure. */
uint32_t core_is_mechanism_transition_pending();
/** @brief Firmware procedure. */
uint32_t core_subtract_split_product();
/** @brief Firmware procedure. */
bool core_register_usb_power_management_callback();
/** @brief Firmware procedure. */
void core_write_descriptor_fields();
/** @brief Firmware procedure. */
bool core_update_valid();
/** @brief Firmware procedure. */
uint32_t core_update_constant_0x12_x01();
/** @brief Firmware procedure. */
uint32_t core_update_device_for_component_for_device();
/** @brief Firmware procedure. */
uint32_t core_update_device_for_device_ti_memset();
/** @brief Firmware procedure. */
uint32_t core_translate_command_code();
/** @brief Firmware procedure. */
bool core_update_constant_0xc_for_word_words_resource();
/** @brief Firmware procedure. */
uint32_t core_block_mww_dispatch_and_claim_device_command();
/** @brief Firmware procedure. */
uint32_t core_is_supported_transfer_opcode();
/** @brief Firmware procedure. */
uint32_t core_get_active_command_response_buffer();
/** @brief Firmware procedure. */
void core_initialize_software_timers();
/** @brief Firmware procedure. */
void core_notify_usb_resume();
/** @brief Firmware procedure. */
void core_update_ti_memset_for_initialize_output_record();
/** @brief Firmware procedure. */
void core_set_thermal_pwm_duty();
/** @brief Firmware procedure. */
void core_unmask_usb_interrupt_when_allowed();
/** @brief Firmware procedure. */
void core_reset_bot_command_state();
/** @brief Firmware procedure. */
void core_clear_media_record_state();
/** @brief Firmware procedure. */
uint32_t core_update_flash_validation_context();
/** @brief Firmware procedure. */
void core_update_release_record_payload();
/** @brief Firmware procedure. */
int core_get_remaining_bounded_capacity();
/** @brief Firmware procedure. */
void core_update_constant_0x54();
/** @brief Firmware procedure. */
void core_update_constant_0x5c();
/** @brief Firmware procedure. */
void core_update_cartridge_input_state();
/** @brief Firmware procedure. */
uint32_t core_check_usb_reconnect_timer();
/** @brief Firmware procedure. */
void core_release_mww_device_command();
/** @brief Firmware procedure. */
void core_update_flash_update_context();
/** @brief Firmware procedure. */
void core_clear_transfer_byte_counters();
/** @brief Firmware procedure. */
void core_enable_processor_sleep_clock_gating();
/** @brief Firmware procedure. */
void core_register_usb_bot_transfer_callback();
/** @brief Firmware procedure. */
uint32_t core_update_x01_x02();
/** @brief Firmware procedure. */
int core_get_payload_offset_from_flags();
/** @brief Firmware procedure. */
uint32_t core_copy_source_record_header();
/** @brief Firmware procedure. */
uint32_t core_update_transfer();
/** @brief Firmware procedure. */
void core_set_mechanism_output_selection();
/** @brief Firmware procedure. */
void core_extend_pending_cartridge_eject_wait();
/** @brief Firmware procedure. */
void core_reset_smart_temperature_monitor();
/** @brief Firmware procedure. */
uint32_t core_erase_boot_flash_sectors();
/** @brief Firmware procedure. */
void core_clear_firmware_image_validation();
/** @brief Firmware procedure. */
uint32_t core_update_device_for_device_for_component();
/** @brief Firmware procedure. */
uint32_t core_update_device_for_device_for_device();
/** @brief Firmware procedure. */
void core_reset_device_command_arbitration();
/** @brief Firmware procedure. */
void core_resume_deferred_mww_dispatch();
/** @brief Firmware procedure. */
void core_update_reconnect_state();
/** @brief Firmware procedure. */
void core_initialize_media_record();
/** @brief Firmware procedure. */
void core_submit_ata_command_context();
/** @brief Firmware procedure. */
uint32_t core_update_component();

/** @brief Firmware procedure. */
uint64_t core_decode_header_flags_and_offset();
/** @brief Firmware procedure. */
void core_update_saturating_add_u32_for_ata_standby_context();
/** @brief Firmware procedure. */
int core_get_remaining_descriptor_space();
/** @brief Firmware procedure. */
void core_arm_standby_activity_check();
/** @brief Firmware procedure. */
void core_update_device_descriptor_context();
/** @brief Firmware procedure. */
void core_set_command_phase_to_one();
/** @brief Firmware procedure. */
void core_clear_command_phase_flags();
/** @brief Firmware procedure. */
void core_set_command_phase_to_two();
/** @brief Firmware procedure. */
void core_load_persistent_notification_mode();
/** @brief Firmware procedure. */
void core_set_usb_device_address();
/** @brief Firmware procedure. */
void core_update_saturating_add_u32_for_enable_request_flag();
/** @brief Firmware procedure. */
uint32_t core_update_f_x1c();
/** @brief Firmware procedure. */
void core_initialize_ata_taskfile();
/** @brief Firmware procedure. */
void core_update_response_word_ti();
/** @brief Firmware procedure. */
uint32_t core_release_record_payload_when_short();
/** @brief Firmware procedure. */
uint32_t core_submit_zero_based_transfer();
/** @brief Firmware procedure. */
uint32_t core_submit_default_transfer_value();
/** @brief Firmware procedure. */
void core_set_ahci_callbacks();
/** @brief Firmware procedure. */
void core_clear_ata_discovery_flags();
/** @brief Firmware procedure. */
void core_mark_cartridge_eject_pending();
/** @brief Firmware procedure. */
void core_set_thermal_pwm_control_phase_zero();
/** @brief Firmware procedure. */
void core_set_thermal_pwm_control_phase_two();
/** @brief Firmware procedure. */
void core_set_thermal_pwm_control_phase_one();
/** @brief Firmware procedure. */
uint32_t core_update_identity_update_context();
/** @brief Firmware procedure. */
void core_increment_persistent_mechanism_completion_count();
/** @brief Firmware procedure. */
uint32_t core_read_unit_identity_record();
/** @brief Firmware procedure. */
void core_set_transfer_ready_flag_to_one();
/** @brief Firmware procedure. */
void core_update_enabled();
/** @brief Firmware procedure. */
void core_mark_record_active_with_code_eight();
/** @brief Firmware procedure. */
void core_start_pwm_channel();
/** @brief Firmware procedure. */
void core_mask_usb_interrupt();
/** @brief Firmware procedure. */
void core_defer_mww_dispatch_and_mask_usb_interrupt();
/** @brief Firmware procedure. */
void core_mark_mww_command_received();
/** @brief Firmware procedure. */
void core_claim_device_command_for_mww();
/** @brief Firmware procedure. */
void core_set_notification_state_to_one();
/** @brief Firmware procedure. */
void core_set_notification_state_to_zero_for_notify_change_resource();
/** @brief Firmware procedure. */
void core_clear_selected_sense_fields();
/** @brief Firmware procedure. */
void core_set_command_phase_flags();
/** @brief Firmware procedure. */
void core_increment_usb_event_counter();
/** @brief Firmware procedure. */
void core_clear_usb_event_counters();
/** @brief Firmware procedure. */
void core_set_usb_test_mode();
/** @brief Firmware procedure. */
uint32_t core_read_primary_persistent_state();
/** @brief Firmware procedure. */
uint32_t core_read_fallback_persistent_state();
/** @brief Firmware procedure. */
void core_update_masked_register_bits();
/** @brief Firmware procedure. */
void core_update_device_request_update();
/** @brief Firmware procedure. */
void core_update_ti_memset_for_ata_command_command();
/** @brief Firmware procedure. */
void core_reset_ahci_port_initialization_state();
/** @brief Firmware procedure. */
void core_decode_word_cipher_delta();
/** @brief Firmware procedure. */
void core_update_mark_primary_sense();
/** @brief Firmware procedure. */
void core_update_store_context_enable();
/** @brief Firmware procedure. */
void core_update_drive_mechanism_outputs();
/** @brief Firmware procedure. */
void core_release_device_command();
/** @brief Firmware procedure. */
void core_claim_device_command();
/** @brief Firmware procedure. */
void core_store_management_dispatch_result();
/** @brief Firmware procedure. */
void core_set_selected_sense_asc_ascq();
/** @brief Firmware procedure. */
void core_set_selected_sense_status();
/** @brief Firmware procedure. */
void core_set_selected_sense_key();
/** @brief Firmware procedure. */
void core_set_command_sense_table_index();
/** @brief Firmware procedure. */
void core_update_command_feature_state();
/** @brief Firmware procedure. */
void core_update_device_status_value();
/** @brief Firmware procedure. */
void core_update_command_buffer_state();
/** @brief Firmware procedure. */
void core_request_system_reset();
/** @brief Firmware procedure. */
void core_update_mww_process_command();
/** @brief Firmware procedure. */
void core_register_usb_bot_reset_callback();
/** @brief Firmware procedure. */
void core_register_usb_ep0_out_transfer_callback();
/** @brief Firmware procedure. */
void core_clear_primary_descriptor_mask();
/** @brief Firmware procedure. */
void core_clear_secondary_descriptor_mask();
/** @brief Firmware procedure. */
void core_run_device_address_transfer();
/** @brief Firmware procedure. */
void core_store_duplicate_record_value();
/** @brief Firmware procedure. */
void core_disable_bus_dynamic_clock_gating();
/** @brief Firmware procedure. */
void core_set_primary_descriptor_mask();
/** @brief Firmware procedure. */
void core_set_secondary_descriptor_mask();
/** @brief Firmware procedure. */
uint32_t core_write_four_byte_value();
/** @brief Firmware procedure. */
void core_set_secondary_descriptor_value_ready();
/** @brief Firmware procedure. */
void core_set_primary_descriptor_value_ready();
/** @brief Firmware procedure. */
void core_wait_spi_transmit_ready();
/** @brief Firmware procedure. */
uint32_t core_update_spi_configure_spi();
/** @brief Firmware procedure. */
void core_clear_request_and_release_device_command();
/** @brief Firmware procedure. */
uint32_t core_submit_configured_payload();
/** @brief Firmware procedure. */
void core_update_spi_set_logical_for_monitor_cartridge_input();
/** @brief Firmware procedure. */
uint32_t core_update_sci_read_logical_for_scsi_logical_address_context();
/** @brief Firmware procedure. */
void core_mark_primary_sense_record_not_ready();
/** @brief Firmware procedure. */
void core_mark_secondary_sense_record_not_ready();
/** @brief Firmware procedure. */
void core_release_nested_record_resource();
/** @brief Firmware procedure. */
uint32_t core_update_sci_read_logical_for_sci_read_logical();
/** @brief Firmware procedure. */
uint32_t core_update_sci_read_logical_for_advance_mechanism_state();
/** @brief Firmware procedure. */
uint32_t core_release_record_buffer();
/** @brief Firmware procedure. */
void core_update_mww_process_transfer();
/** @brief Firmware procedure. */
void core_store_context_enable_byte();
/** @brief Firmware procedure. */
void core_store_record_field_14_value();
/** @brief Firmware procedure. */
uint32_t core_divide_doubleword_by_word();
/** @brief Firmware procedure. */
uint32_t core_submit_scsi_read_write_ata_fis();
/** @brief Firmware procedure. */
uint32_t core_transition_flash_validation_context();
/** @brief Firmware procedure. */
uint32_t core_transition_queue_capacity_command_queue_capacity_context();
/** @brief Firmware procedure. */
void core_ramp_thermal_pwm();
/** @brief Firmware procedure. */
uint32_t core_submit_command_descriptor_to_device();
/** @brief Firmware procedure. */
bool core_transition_selected_or_bit_selection_state();
/** @brief Firmware procedure. */
uint32_t core_prepare_command_transfer_direction();
/** @brief Firmware procedure. */
uint64_t core_transition_success_flag_clear_memory_transfer_success_context();
/** @brief Firmware procedure. */
void core_transition_reconnect_state();
/** @brief Firmware procedure. */
void core_transition_sequence_x01();
/** @brief Firmware procedure. */
void core_control_temperature_monitor();
/** @brief Firmware procedure. */
uint32_t core_transition_response_flag_x01();
/** @brief Firmware procedure. */
uint32_t core_transition_use_alternate_lookup_type_copy();

/** @brief Firmware procedure. */
uint8_t core_transition_clear_memory_transition_context();
/** @brief Firmware procedure. */
uint32_t core_transition_atapi_device_flag();
/** @brief Firmware procedure. */
void core_transition_firmware_context_0x14_firmware_record_context();
/** @brief Firmware procedure. */
uint32_t core_transition_range_word_range_word();
/** @brief Firmware procedure. */
void core_transition_device_address();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_CORE_H */
