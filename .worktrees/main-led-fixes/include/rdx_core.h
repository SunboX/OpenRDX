/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
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
void core_configure_device_descriptor_0x15_device_descriptor_storage();
/** @brief Firmware procedure. */
void core_configure_descriptor_payload();
/** @brief Firmware procedure. */
int core_configure_range_start();
/** @brief Firmware procedure. */
bool core_configure_ata_devices_address_ata_device_table();
/** @brief Firmware procedure. */
void core_configure_ti_memset();
/** @brief Firmware procedure. */
void core_configure_ata_address_ata_configuration_address();
/** @brief Firmware procedure. */
void core_configure_ata_address_ata_queue_context();
/** @brief Firmware procedure. */
uint32_t core_build_ata_smart_command();
/** @brief Firmware procedure. */
void core_configure_command_opcode_response();
/** @brief Firmware procedure. */
uint32_t core_configure_ata_command();
/** @brief Firmware procedure. */
uint32_t core_configure_payload_ti_memset();
/** @brief Firmware procedure. */
bool core_configure_initialize_output_record();
/** @brief Firmware procedure. */
void core_sha256_initialize();
/** @brief Firmware procedure. */
void core_configure_command_profile_c8_or_25_selector();
/** @brief Firmware procedure. */
void core_configure_command_profile_ca_or_35_selector();
/** @brief Firmware procedure. */
uint64_t core_configure_component_for_parsed_header_descriptor();

/** @brief Firmware procedure. */
uint32_t core_configure_component_scaled();
/** @brief Firmware procedure. */
void core_configure_descriptor_configuration_context();
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
void core_handle_payload_byte_type_or_response_payload_response_context();
/** @brief Firmware procedure. */
uint32_t core_handle_descriptor_or_descriptor_delta();

/** @brief Firmware procedure. */
void core_handle_response_attribute_bytes();
/** @brief Firmware procedure. */
uint32_t core_handle_device_descriptor_response_flag_device_descriptor_context();
/** @brief Firmware procedure. */
uint64_t core_dispatch_rdx_management_record();
/** @brief Firmware procedure. */
void core_initialize_rdx_identity_records();
/** @brief Firmware procedure. */
void core_handle_descriptor_command_descriptor_context();
/** @brief Firmware procedure. */
void core_monitor_cartridge_input();
/** @brief Firmware procedure. */
void core_handle_clear_memory_lookup_ata_command_buffer();
/** @brief Firmware procedure. */
void core_advance_mechanism_state();
/** @brief Firmware procedure. */
void core_select_rdx_sense_status();
/** @brief Firmware procedure. */
void core_handle_response_end_word_response_tail_storage();
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
void core_write_response_type();
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
void core_handle_x02_n();
/** @brief Firmware procedure. */
void core_handle_primary_sense_context();
/** @brief Firmware procedure. */
uint32_t core_read_smart_temperature();
/** @brief Firmware procedure. */
void core_handle_ti_memset();
/** @brief Firmware procedure. */
void core_handle_feature_command_feature_state();
/** @brief Firmware procedure. */
void core_handle_cartridge_input_state();
/** @brief Firmware procedure. */
void core_handle_command_phase();
/** @brief Firmware procedure. */
void core_handle_ata_command_opcode_ata_command_context();
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
int core_handle_type_type();
/** @brief Firmware procedure. */
void core_drive_mechanism_outputs();
/** @brief Firmware procedure. */
uint32_t core_dispatch_response_buffer();
/** @brief Firmware procedure. */
void core_forward_ti_memset();
/** @brief Firmware procedure. */
void core_set_notification_state_to_zero();
/** @brief Firmware procedure. */
bool core_forward_mww_transfer_value();
/** @brief Firmware procedure. */
void core_forward_cartridge_input_state();
/** @brief Firmware procedure. */
uint8_t core_forward_mcp3008_sample_value();
/** @brief Firmware procedure. */
bool core_is_command_phase_three();
/** @brief Firmware procedure. */
uint32_t core_forward_sci_read_logical();
/** @brief Firmware procedure. */
void core_forward_interrupt_transfer_context();
/** @brief Firmware procedure. */
void core_forward_end_remaining();
/** @brief Firmware procedure. */
uint32_t core_is_supported_forward_opcode();
/** @brief Firmware procedure. */
uint8_t core_forward_cartridge_input_value();
/** @brief Firmware procedure. */
void core_forward_reconnect_state();
/** @brief Firmware procedure. */
uint32_t core_build_supported_record_types_response();
/** @brief Firmware procedure. */
uint32_t core_get_ti_memset();

/** @brief Firmware procedure. */
uint32_t core_get_descriptor();

/** @brief Firmware procedure. */
int core_get_set_interrupt_mmio();
/** @brief Firmware procedure. */
uint32_t core_get_mww_transfer_limit();
/** @brief Firmware procedure. */
uint32_t core_get_end_remaining();

/** @brief Firmware procedure. */
uint32_t core_get_response_buffer();
/** @brief Firmware procedure. */
uint32_t core_get_zero_word_buffer();

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
bool core_get_gpio_pin_context();
/** @brief Firmware procedure. */
uint64_t core_get_structure_field_hex_for_response_end_word();
/** @brief Firmware procedure. */
int core_get_runtime_context_base_for_command_descriptor_device();
/** @brief Firmware procedure. */
uint32_t core_is_supported_descriptor_opcode();
/** @brief Firmware procedure. */
bool core_get_ahci_control_register();
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
int16_t core_get_flash_validation_context_for_validate_structured_flash();
/** @brief Firmware procedure. */
uint64_t core_pack_descriptor_validation_result();
/** @brief Firmware procedure. */
uint32_t core_get_mode_sense_block_descriptor_length();
/** @brief Firmware procedure. */
int core_get_mww_mode_page_buffer();
/** @brief Firmware procedure. */
uint32_t core_get_mww_active_flag();
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
uint32_t core_get_command_feature();
/** @brief Firmware procedure. */
bool core_get_cartridge_eject_control();
/** @brief Firmware procedure. */
uint32_t core_get_notification_state();
/** @brief Firmware procedure. */
int core_get_structure_field_hex_for_structure_field_hex();
/** @brief Firmware procedure. */
uint32_t core_get_ahci_status_register();
/** @brief Firmware procedure. */
bool core_is_cartridge_input_state_six();
/** @brief Firmware procedure. */
uint8_t core_get_constant_zero_value_for_pin_resource_state();
/** @brief Firmware procedure. */
uint8_t core_get_0xc_for_lookup_key_metadata();
/** @brief Firmware procedure. */
bool core_get_mww_transfer_context();
/** @brief Firmware procedure. */
bool core_is_command_phase_three_for_build_scsi_mode();
/** @brief Firmware procedure. */
bool core_get_thermal_control_context();
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
uint8_t *core_get_ata_command_buffer();
/** @brief Firmware procedure. */
uint32_t *core_get_cartridge_monitor_context();
/** @brief Firmware procedure. */
uint8_t core_get_gpio_pin_context_value();
/** @brief Firmware procedure. */
uint8_t core_get_resource_ahci_state();
/** @brief Firmware procedure. */
uint8_t *core_get_response_byte_context();
/** @brief Firmware procedure. */
uint8_t core_get_mcp3008_sample_value();
/** @brief Firmware procedure. */
char core_convert_nibble_to_hex_character();
/** @brief Firmware procedure. */
uint32_t core_get_set_validation_flags();
/** @brief Firmware procedure. */
uint8_t core_get_mww_transfer_value();
/** @brief Firmware procedure. */
uint8_t core_get_cartridge_input_value();
/** @brief Firmware procedure. */
uint32_t core_get_notification_context_value();
/** @brief Firmware procedure. */
uint8_t core_get_device_status_value();
/** @brief Firmware procedure. */
uint8_t core_get_command_context_value();
/** @brief Firmware procedure. */
uint8_t core_get_command_feature_state_value();
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
uint8_t *core_get_response_end_context_pointer();
/** @brief Firmware procedure. */
uint32_t *core_get_persistent_identity_record_pointer();
/** @brief Firmware procedure. */
uint32_t core_get_runtime_context_base_for_build_scsi_mode();
/** @brief Firmware procedure. */
uint8_t core_read_context_field_6c_byte();
/** @brief Firmware procedure. */
uint8_t core_get_structure_field_hex_70_value();
/** @brief Firmware procedure. */
uint32_t core_get_descriptor_requested();
/** @brief Firmware procedure. */
uint32_t core_get_constant_hex_1014_value();
/** @brief Firmware procedure. */
uint32_t core_get_constant_hex_1000_value();
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
uint32_t core_process_short_available();
/** @brief Firmware procedure. */
uint32_t core_process_secondary_command_context();
/** @brief Firmware procedure. */
uint32_t core_process_x01_for_advance_mechanism_state();
/** @brief Firmware procedure. */
uint32_t core_handle_special_control_record();
/** @brief Firmware procedure. */
uint32_t core_process_word_validity_flag_command_validation_context();
/** @brief Firmware procedure. */
void core_process_remaining_descriptor_space_for_transfer_completion_flag();
/** @brief Firmware procedure. */
int core_process_ti_memset_copy();
/** @brief Firmware procedure. */
int core_read_mcp3008_channel();
/** @brief Firmware procedure. */
uint32_t core_process_command_opcode_transfer_size_atapi_device_flag();
/** @brief Firmware procedure. */
int core_program_spi_flash_pages();
/** @brief Firmware procedure. */
uint32_t core_process_device_use_lookup();
/** @brief Firmware procedure. */
uint32_t core_process_runtime_context_base_for_component();
/** @brief Firmware procedure. */
uint32_t core_process_command_opcode_command();
/** @brief Firmware procedure. */
void core_process_end_remaining();
/** @brief Firmware procedure. */
uint32_t core_process_descriptor_type_ti_memset();
/** @brief Firmware procedure. */
void core_process_response_flag_x01();
/** @brief Firmware procedure. */
uint32_t core_submit_ata_standby_immediate();
/** @brief Firmware procedure. */
void core_process_fprinttyp2();
/** @brief Firmware procedure. */
void core_process_response_word_clear_memory_response_word_context();
/** @brief Firmware procedure. */
void core_process_firmware_transfer_for_mww_transition();
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
int core_process_a_command_validation_context();
/** @brief Firmware procedure. */
void core_process_descriptor_component_context();
/** @brief Firmware procedure. */
void core_process_firmware_transfer_context();
/** @brief Firmware procedure. */
void core_process_header_word_header_words();
/** @brief Firmware procedure. */
uint32_t core_process_ata_command_for_ata_standby_context();
/** @brief Firmware procedure. */
uint8_t core_process_clear_memory_buffer_compare_context();
/** @brief Firmware procedure. */
int core_process_component_for_command_sequence_update();
/** @brief Firmware procedure. */
void core_process_command_completion_context();
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
uint32_t core_process_wdt_reset();
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
uint32_t core_process_command_queue_context();
/** @brief Firmware procedure. */
void core_process_primary_command_context();
/** @brief Firmware procedure. */
void core_process_command_descriptor_context();
/** @brief Firmware procedure. */
void core_process_firmware_transfer_for_mww();
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
void core_process_usb_event_context();
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
void core_process_firmware_context_endpoint_context();
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
void core_process_rti_clock_mhz();
/** @brief Firmware procedure. */
void core_process_notify_change_notification_context();
/** @brief Firmware procedure. */
void core_process_configuration();
/** @brief Firmware procedure. */
void core_process_halt_forever_for_response_end_word();

/** @brief Firmware procedure. */
uint32_t core_process_capacity_block_context();
/** @brief Firmware procedure. */
void core_process_bitset_base();
/** @brief Firmware procedure. */
void core_process_ahci_port_control();
/** @brief Firmware procedure. */
void core_process_cartridge_input_state();
/** @brief Firmware procedure. */
bool core_process_ata_command_for_lookup_key_metadata();
/** @brief Firmware procedure. */
uint64_t core_process_ti_memset_ti_memset();
/** @brief Firmware procedure. */
void core_process_copy();
/** @brief Firmware procedure. */
void core_process_clear_memory_for_cartridge_monitor_context();
/** @brief Firmware procedure. */
uint32_t core_process_command_processing_context();
/** @brief Firmware procedure. */
uint32_t core_process_constant_0x24_fixed_command_context();
/** @brief Firmware procedure. */
uint32_t core_process_ata_command_buffer_for_clear_memory_lookup();
/** @brief Firmware procedure. */
void core_process_configuration_block_payload();
/** @brief Firmware procedure. */
void core_process_input_pin_state();
/** @brief Firmware procedure. */
void core_process_firmware_transfer_resource();
/** @brief Firmware procedure. */
void core_process_ti_memset_for_initialize_transfer_workspace();
/** @brief Firmware procedure. */
uint32_t core_process_sci_read_logical_for_monitor_cartridge_input();
/** @brief Firmware procedure. */
bool core_compare_record_payloads();
/** @brief Firmware procedure. */
void core_process_first_pin_initial_pin();
/** @brief Firmware procedure. */
uint16_t *core_process_ti_memset_for_build_scsi_mode();
/** @brief Firmware procedure. */
void core_process_halt_forever_for_build_scsi_mode();
/** @brief Firmware procedure. */
uint32_t core_dispatch_available_record();

/** @brief Firmware procedure. */
void core_process_ti_memset_0x20();
/** @brief Firmware procedure. */
void core_process_clear_memory_spi_request_context();
/** @brief Firmware procedure. */
void core_process_clear_memory_transfer_buffer_context();
/** @brief Firmware procedure. */
void core_reconnect_device_and_start_timer();
/** @brief Firmware procedure. */
void core_process_mww_process_command();
/** @brief Firmware procedure. */
uint32_t core_process_mww_update_resource();
/** @brief Firmware procedure. */
void core_copy_record_header_to_destination();
/** @brief Firmware procedure. */
void core_submit_fixed_ata_command();
/** @brief Firmware procedure. */
void core_rewrite_unit_identity_record();
/** @brief Firmware procedure. */
void core_process_lookup_arguments();
/** @brief Firmware procedure. */
uint32_t core_process_halt_forever_for_selected_or_bit();
/** @brief Firmware procedure. */
uint32_t core_process_expected_candidate();
/** @brief Firmware procedure. */
void core_process_mode_processing_context();
/** @brief Firmware procedure. */
void core_process_device_status_context();
/** @brief Firmware procedure. */
void core_process_persistent_identity_record_for_identity_persistence_context();
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
void core_process_command_runtime_state();
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
void core_process_media_processing_context();
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
void core_transfer_capacity_word_default_block_size_capacity_block_context();
/** @brief Firmware procedure. */
uint32_t core_sha256_compress_block();
/** @brief Firmware procedure. */
void core_sha256_finalize();
/** @brief Firmware procedure. */
uint64_t core_transfer_descriptor_capacity_previous_descriptor_address_descriptor_queue_context();
/** @brief Firmware procedure. */
uint64_t core_transfer_address_stack_transfer_address();
/** @brief Firmware procedure. */
uint32_t core_parse_stream_record_with_output_word();
/** @brief Firmware procedure. */
int core_transfer_comparison_context();
/** @brief Firmware procedure. */
void core_transfer_firmware_t_byte();
/** @brief Firmware procedure. */
uint32_t core_parse_stream_record();
/** @brief Firmware procedure. */
uint32_t core_transfer_packed_command_words_command_words();
/** @brief Firmware procedure. */
void core_transfer_remaining_bytes_word();
/** @brief Firmware procedure. */
uint32_t core_transfer_word_copy();
/** @brief Firmware procedure. */
uint32_t core_transfer_find_last_nonzero();

/** @brief Firmware procedure. */
uint32_t core_transfer_transfer_completion_flag();
/** @brief Firmware procedure. */
void core_transfer_command_sequence_update_flag();
/** @brief Firmware procedure. */
uint32_t core_transfer_match_found_expected_type();
/** @brief Firmware procedure. */
void core_transfer_mixed_word_key_sum();
/** @brief Firmware procedure. */
uint32_t core_transfer_firmware_context_remaining_firmware_remaining_context();
/** @brief Firmware procedure. */
uint64_t core_transfer_device_feature_mask_feature_mask();
/** @brief Firmware procedure. */
uint32_t core_transfer_response_byte_stack_response_byte();
/** @brief Firmware procedure. */
void core_transfer_fourth_byte_byte();
/** @brief Firmware procedure. */
uint32_t core_transfer_retries_remaining_ti_memset();
/** @brief Firmware procedure. */
void core_copy_reversed_field_bytes();
/** @brief Firmware procedure. */
uint32_t core_transfer_range_end_range_start_for_short_available();
/** @brief Firmware procedure. */
uint64_t core_transfer_word_bit_mask();
/** @brief Firmware procedure. */
uint32_t core_transfer_usb_get_device();
/** @brief Firmware procedure. */
uint32_t core_transfer_descriptor_wait_completion();
/** @brief Firmware procedure. */
uint32_t core_transfer_shifts_remaining_word();
/** @brief Firmware procedure. */
int core_transfer_maximum_payload_payload();
/** @brief Firmware procedure. */
uint32_t core_transfer_shifts_remaining_words();
/** @brief Firmware procedure. */
uint32_t core_transfer_spi_configure_spi();
/** @brief Firmware procedure. */
void core_transfer_register_offsets_register_offset_context();
/** @brief Firmware procedure. */
uint64_t core_transfer_found_flag();
/** @brief Firmware procedure. */
void core_transfer_spi_response_context();
/** @brief Firmware procedure. */
uint64_t core_read_reversed_field_value();
/** @brief Firmware procedure. */
int core_transfer_remaining_bounds();
/** @brief Firmware procedure. */
uint32_t core_transfer_ti_memset_n();
/** @brief Firmware procedure. */
void core_transfer_tracker();
/** @brief Firmware procedure. */
uint32_t core_transfer_range_end_range_start_for_validate_structured_flash();
/** @brief Firmware procedure. */
uint32_t core_transfer_range_end_range_start_for_descriptor_descriptor_resource();
/** @brief Firmware procedure. */
void core_transfer_end_remaining();
/** @brief Firmware procedure. */
void core_transfer_range_start_range_end();
/** @brief Firmware procedure. */
uint32_t core_calculate_crc32();
/** @brief Firmware procedure. */
void core_transfer_interrupt_transfer_context();
/** @brief Firmware procedure. */
uint32_t core_transfer_polls_remaining_channel_usb_poll_context();
/** @brief Firmware procedure. */
void core_transfer_usb_endpoint_transfer_for_response_buffer();
/** @brief Firmware procedure. */
void core_transfer_usb_endpoint_transfer();
/** @brief Firmware procedure. */
void core_transfer_ti_memset_component();
/** @brief Firmware procedure. */
void core_transfer_bytes_remaining_descriptor();
/** @brief Firmware procedure. */
uint32_t core_transfer_component_for_fallback_decoded_request();
/** @brief Firmware procedure. */
uint32_t core_transfer_component_for_flash_validation_context();
/** @brief Firmware procedure. */
uint32_t core_transfer_reverse();
/** @brief Firmware procedure. */
void core_transfer_pending_ata_queue();
/** @brief Firmware procedure. */
void core_transfer_x01();
/** @brief Firmware procedure. */
int core_compare_buffers_from_end();
/** @brief Firmware procedure. */
uint8_t *core_transfer_spi_transfer_storage();
/** @brief Firmware procedure. */
void core_transfer_packed();
/** @brief Firmware procedure. */
uint32_t core_find_value_in_fixed_table();
/** @brief Firmware procedure. */
void core_transfer_remaining_firmware_context();
/** @brief Firmware procedure. */
void core_initialize_word_buffer();
/** @brief Firmware procedure. */
void core_transfer_bounded_transfer_context();
/** @brief Firmware procedure. */
int core_find_last_nonzero_word();
/** @brief Firmware procedure. */
void core_copy_words_to_interleaved_destination();
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
uint32_t core_update_ahci_control_register();
/** @brief Firmware procedure. */
uint32_t core_update_descriptor_requested();
/** @brief Firmware procedure. */
void core_update_ata_queue_address_ata_pending_ata_queue_context();
/** @brief Firmware procedure. */
void core_update_ata_address_ata_queue_context();
/** @brief Firmware procedure. */
void core_load_flash_configuration_byte();
/** @brief Firmware procedure. */
uint32_t core_update_usb_transfer_state();
/** @brief Firmware procedure. */
void core_update_word_update_context();
/** @brief Firmware procedure. */
void core_update_ata_address_ata_device_address();
/** @brief Firmware procedure. */
bool core_update_scsi_logical_address_context();
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
void core_update_firmware_context_usb_transfer_state();
/** @brief Firmware procedure. */
bool core_update_device_base_device_base_context();
/** @brief Firmware procedure. */
uint32_t core_update_availability_x01();
/** @brief Firmware procedure. */
uint32_t core_validate_transfer_capacity();
/** @brief Firmware procedure. */
int core_update_configuration_update_context();
/** @brief Firmware procedure. */
uint32_t core_update_capacity_block_context();
/** @brief Firmware procedure. */
void core_update_flag_one_interrupt_transfer_context();
/** @brief Firmware procedure. */
void core_update_persistent_state_status();
/** @brief Firmware procedure. */
bool core_update_flash_completion_context();
/** @brief Firmware procedure. */
uint32_t core_update_device_update_context();
/** @brief Firmware procedure. */
void core_set_channel_mode_bits();
/** @brief Firmware procedure. */
uint32_t core_update_sense_record_context();
/** @brief Firmware procedure. */
uint32_t core_update_multiplier_multiplier();
/** @brief Firmware procedure. */
bool core_update_device_label_context();
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
uint32_t core_update_default_mww_transfer_value();
/** @brief Firmware procedure. */
uint32_t core_is_supported_transfer_opcode();
/** @brief Firmware procedure. */
uint32_t core_update_command_validation_context();
/** @brief Firmware procedure. */
void core_update_clear_memory_thermal_control_context();
/** @brief Firmware procedure. */
void core_update_firmware_context_usb_completion_state();
/** @brief Firmware procedure. */
void core_update_ti_memset_for_initialize_output_record();
/** @brief Firmware procedure. */
void core_update_flash_validation_status();
/** @brief Firmware procedure. */
void core_update_spi_transfer_context_for_program_spi_flash();
/** @brief Firmware procedure. */
void core_update_mww_saved_request();
/** @brief Firmware procedure. */
void core_update_ti_memset_for_ti_memset_for();
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
uint32_t core_update_pin_update_context();
/** @brief Firmware procedure. */
void core_update_mww_transfer_context_for_mww_process_resource();
/** @brief Firmware procedure. */
void core_update_flash_update_context();
/** @brief Firmware procedure. */
void core_update_sense_refresh_context();
/** @brief Firmware procedure. */
void core_update_cddis_reg_off_cartridge_eject_control();
/** @brief Firmware procedure. */
void core_update_firmware_record_update();
/** @brief Firmware procedure. */
uint32_t core_update_x01_x02();
/** @brief Firmware procedure. */
int core_get_payload_offset_from_flags();
/** @brief Firmware procedure. */
uint32_t core_copy_source_record_header();
/** @brief Firmware procedure. */
uint32_t core_update_transfer();
/** @brief Firmware procedure. */
void core_update_sense_update_context();
/** @brief Firmware procedure. */
void core_update_mechanism_update_context_for_primary_sense_context();
/** @brief Firmware procedure. */
void core_update_persistent_update_context();
/** @brief Firmware procedure. */
uint32_t core_update_set_interrupt_mmio();
/** @brief Firmware procedure. */
void core_update_clear_memory_mcp3008_sample();
/** @brief Firmware procedure. */
uint32_t core_update_device_for_device_for_component();
/** @brief Firmware procedure. */
uint32_t core_update_device_for_device_for_device();
/** @brief Firmware procedure. */
void core_update_thermal_monitor_enabled_for_clear_memory_resource();
/** @brief Firmware procedure. */
void core_update_spi_transfer_context();
/** @brief Firmware procedure. */
void core_update_reconnect_state();
/** @brief Firmware procedure. */
void core_update_ti_memset_for_lookup_arguments();
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
void core_update_response_update_context();
/** @brief Firmware procedure. */
void core_update_device_descriptor_context();
/** @brief Firmware procedure. */
void core_set_command_phase_to_one();
/** @brief Firmware procedure. */
void core_update_command_phase_flags_for_firmware_context_byte();
/** @brief Firmware procedure. */
void core_set_command_phase_to_two();
/** @brief Firmware procedure. */
void core_update_notification_context();
/** @brief Firmware procedure. */
void core_update_command_status_update();
/** @brief Firmware procedure. */
void core_update_saturating_add_u32_for_enable_request_flag();
/** @brief Firmware procedure. */
uint32_t core_update_f_x1c();
/** @brief Firmware procedure. */
void core_update_ti_memset_for_ti_memset();
/** @brief Firmware procedure. */
void core_update_response_word_ti();
/** @brief Firmware procedure. */
uint32_t core_release_record_payload_when_short();
/** @brief Firmware procedure. */
uint32_t core_submit_zero_based_transfer();
/** @brief Firmware procedure. */
uint32_t core_submit_default_transfer_value();
/** @brief Firmware procedure. */
void core_update_ahci_device_context();
/** @brief Firmware procedure. */
void core_update_clear_memory_response_end_context();
/** @brief Firmware procedure. */
void core_update_mechanism_update_context_for_advance_mechanism_state();
/** @brief Firmware procedure. */
void core_set_mechanism_phase_to_zero();
/** @brief Firmware procedure. */
void core_set_mechanism_phase_to_two();
/** @brief Firmware procedure. */
void core_set_mechanism_phase_to_one();
/** @brief Firmware procedure. */
uint32_t core_update_identity_update_context();
/** @brief Firmware procedure. */
void core_update_identity_persistence_context();
/** @brief Firmware procedure. */
uint32_t core_read_unit_identity_record();
/** @brief Firmware procedure. */
void core_set_transfer_ready_flag_to_one();
/** @brief Firmware procedure. */
void core_update_enabled();
/** @brief Firmware procedure. */
void core_mark_record_active_with_code_eight();
/** @brief Firmware procedure. */
void core_signal_channel_completion();
/** @brief Firmware procedure. */
void core_set_transfer_interrupt_enable_mask();
/** @brief Firmware procedure. */
void core_update_spi_transfer_context_for_mww_handle_transfer();
/** @brief Firmware procedure. */
void core_set_cartridge_input_value_to_one();
/** @brief Firmware procedure. */
void core_update_mww_transfer_context_for_mww_handle_transfer();
/** @brief Firmware procedure. */
void core_set_notification_state_to_one();
/** @brief Firmware procedure. */
void core_set_notification_state_to_zero_for_notify_change_resource();
/** @brief Firmware procedure. */
void core_update_clear_memory_response_buffer_context();
/** @brief Firmware procedure. */
void core_update_command_phase_flags_for_command_phase();
/** @brief Firmware procedure. */
void core_increment_usb_event_counter();
/** @brief Firmware procedure. */
void core_update_clear_memory_usb_event_counter();
/** @brief Firmware procedure. */
void core_update_usb_reset_state();
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
void core_set_command_runtime_state_to_zero();
/** @brief Firmware procedure. */
void core_update_secondary_update_context();
/** @brief Firmware procedure. */
void core_update_mark_primary_sense();
/** @brief Firmware procedure. */
void core_update_store_context_enable();
/** @brief Firmware procedure. */
void core_update_drive_mechanism_outputs();
/** @brief Firmware procedure. */
void core_set_thermal_monitor_enabled_to_zero();
/** @brief Firmware procedure. */
void core_set_thermal_monitor_enabled_to_one();
/** @brief Firmware procedure. */
void core_store_management_dispatch_result();
/** @brief Firmware procedure. */
void core_update_secondary_sense_field_context();
/** @brief Firmware procedure. */
void core_update_response_buffer_context();
/** @brief Firmware procedure. */
void core_update_primary_sense_field_context();
/** @brief Firmware procedure. */
void core_update_command_phase_auxiliary();
/** @brief Firmware procedure. */
void core_update_command_feature_state();
/** @brief Firmware procedure. */
void core_update_device_status_value();
/** @brief Firmware procedure. */
void core_update_command_buffer_state();
/** @brief Firmware procedure. */
void core_set_SYSECR_REG_OFF_to_hex_8000_handler();
/** @brief Firmware procedure. */
void core_update_mww_process_command();
/** @brief Firmware procedure. */
void core_update_firmware_context_primary_firmware_callback();
/** @brief Firmware procedure. */
void core_update_firmware_context_secondary_firmware_callback();
/** @brief Firmware procedure. */
void core_clear_primary_descriptor_mask();
/** @brief Firmware procedure. */
void core_clear_secondary_descriptor_mask();
/** @brief Firmware procedure. */
void core_run_device_address_transfer();
/** @brief Firmware procedure. */
void core_store_duplicate_record_value();
/** @brief Firmware procedure. */
void core_set_CDDIS_REG_OFF_to_hex_38_handler();
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
void core_update_spi_update_spi();
/** @brief Firmware procedure. */
uint32_t core_update_spi_configure_spi();
/** @brief Firmware procedure. */
void core_update_thermal_monitor_enabled_for_read_smart_temperature();
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
uint32_t core_transition_scale_word_range_word();
/** @brief Firmware procedure. */
uint32_t core_transition_reserved_command_byte_reserved_command_word_descriptor_queue_context();
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
