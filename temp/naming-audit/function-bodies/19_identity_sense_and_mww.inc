
uint32_t core_process_expected_candidate(context, buffer, length)
uint32_t context;
uint8_t *buffer;
uint32_t length;
{
    int operation_status;
    operation_status = core_get_expected_candidate();
    if (operation_status == 0) {
        ((uint32_t (*)())core_run_expected_flag_expected_type)(*buffer, 0, length, context);
    }
    return 0;
}
uint32_t core_get_expected_candidate(candidate_record, expected_record)
int candidate_record;
int expected_record;
{
    uint32_t core_run_ready_state_handler_result;
    uint8_t working_buffer[24];
    if (*(char *)(expected_record + 1) == *(char *)(candidate_record + 0xc)) {
        core_run_ready_state_handler_result = 0;
    } else {
        core_run_ready_state_handler_result = core_run_ready_state_handler(candidate_record, *(char *)(expected_record + 1), working_buffer);
    }
    return core_run_ready_state_handler_result;
}
void core_update_cartridge_input_state()
{
    if ((core_cartridge_input_state != 6) && (core_cartridge_input_state == 2)) {
        core_cartridge_input_state = 6;
        core_process_command_descriptor_context();
        return;
    }
    return;
}
void core_process_mode_processing_context()
{
    int operation_status;
    operation_status = core_forward_mww_transfer_value();
    if (operation_status == 0) {
        core_mode_processing_context = 1;
        return;
    }
    core_handle_ata_standby_context_for_cartridge_monitor_context();
    return;
}
void core_process_device_status_context()
{
    if (core_device_status_context == '\0') {
        core_set_mechanism_phase_to_one();
        return;
    }
    if (core_flash_validation_status != 8) {
        core_update_flash_validation_status(0);
        core_flash_validation_status = 7;
    }
    return;
}
void core_transfer_bounded_transfer_context(buffer, value) uint8_t *buffer;
int value;
{
    int operation_status;
    int *core_bounded_transfer_context_pointer;
    operation_status = 0x10;
    core_bounded_transfer_context_pointer = (int *)&core_bounded_transfer_context;
    do {
        operation_status = operation_status + -1;
        *buffer = *(uint8_t *)(value + *core_bounded_transfer_context_pointer);
        buffer = buffer + 1;
        core_bounded_transfer_context_pointer = core_bounded_transfer_context_pointer + 1;
    } while (operation_status != 0);
    return;
}
void core_process_persistent_identity_record_for_identity_persistence_context()
{
    core_persistent_identity_record = ((uint32_t (*)())core_calculate_record_checksum)(&core_persistent_identity_record, 0x40);
    core_rewrite_primary_persistent_state(&core_persistent_identity_record);
    return;
}
int16_t core_get_flash_validation_context_for_validate_structured_flash()
{
    int16_t core_flash_validation_context_snapshot;
    if ((core_flash_validation_context == 0) || (core_flash_validation_context_snapshot = (int16_t)core_flash_validation_context, (int32_t)core_flash_validation_context == -1 || core_flash_validation_context == 1)) {
        core_flash_validation_context_snapshot = 0x37;
    }
    return core_flash_validation_context_snapshot;
}
void core_rewrite_primary_persistent_state(context) uint32_t context;
{
    int operation_status;
    operation_status = core_get_set_interrupt_mmio(0x3f000);
    if (operation_status == 0) {
        core_program_spi_flash_pages(0x3f000, 0x40, context);
    }
    return;
}
uint32_t core_process_runtime_context_base_for_cartridge_input_state()
{
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint32_t core_accumulate_split_transfer_offsets_result;
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_accumulate_split_transfer_offsets_result = core_accumulate_split_transfer_offsets(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_forward_ti_memset(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    return core_accumulate_split_transfer_offsets_result;
}
int core_find_last_nonzero_word(offset, condition)
int offset;
int condition;
{
    int condition_field;
    int *computed_offset;
    if (condition != 0) {
        computed_offset = (int *)(offset + condition * 4);
        do {
            computed_offset = computed_offset + -1;
            condition_field = condition + -1;
            if (*computed_offset != 0) {
                return condition;
            }
            condition = condition_field;
        } while (condition_field != 0);
    }
    return 0;
}
uint32_t core_process_sense_record_context()
{
    int operation_status;
    uint32_t operation_result;
    operation_status = core_update_sense_record_context();
    operation_result = 1;
    if (((operation_status == 0) && (operation_status = core_process_sci_read_logical_for_monitor_cartridge_input(), operation_status != 0)) &&
        (operation_status = core_forward_mcp3008_sample_value(), operation_status == 0)) {
        operation_result = 0;
    }
    return operation_result;
}
uint64_t core_pack_descriptor_validation_result(context, value, offset)
uint32_t context;
uint32_t value;
int offset;
{
    int core_get_ti_memset_derived;
    core_get_ti_memset_derived = ((uint32_t (*)())core_get_ti_memset)();
    if ((core_get_ti_memset_derived != 0) && (*(uint16_t *)(offset + 8) < 0x38)) {
        core_get_ti_memset_derived = 0;
    }
    return CONCAT44(value, core_get_ti_memset_derived);
}
uint32_t core_update_pin_update_context()
{
    int core_get_thermal_control_context_result;
    uint32_t operation_result;
    operation_result = 0;
    if ((core_pin_update_context == '\0') && (core_get_thermal_control_context_result = core_get_thermal_control_context(), core_get_thermal_control_context_result != 0)) {
        operation_result = 1;
    }
    return operation_result;
}
uint32_t core_process_flash_validation_context()
{
    int core_get_flash_validation_context_for_validate_structured_flash_result;
    int core_get_response_status_value_result;
    uint32_t operation_result;
    core_get_flash_validation_context_for_validate_structured_flash_result = core_get_flash_validation_context_for_validate_structured_flash();
    core_get_response_status_value_result = core_get_response_status_value();
    operation_result = 0;
    if ((core_get_response_status_value_result == 0) && (core_get_flash_validation_context_for_validate_structured_flash_result == 0x38 || core_get_flash_validation_context_for_validate_structured_flash_result == 0x35)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_update_mww_transfer_context_for_mww_process_resource()
{
    uint8_t *core_mww_transfer_context_pointer;
    uint8_t *callback_result = 0;
    core_mww_transfer_context_pointer = (uint8_t *)&core_mww_transfer_context;
    if (core_mww_transfer_context != '\0') {
        core_set_thermal_monitor_enabled_to_zero();
        core_mww_transfer_context_pointer = callback_result;
    }
    *core_mww_transfer_context_pointer = 0;
    core_mww_transfer_context_pointer[1] = 0;
    return;
}
void core_update_flash_update_context()
{
    if ((core_flash_update_context != '\0') && (core_mww_transfer_value == '\0')) {
        core_flash_update_context = 0;
        return;
    }
    return;
}
void core_update_sense_refresh_context()
{
    core_sense_refresh_context = 0;
    core_sense_refresh_sector_offset_high = 0;
    core_primary_sense_context = 0;
    core_primary_sense_sector_offset_high = 0;
    return;
}
uint32_t core_get_mode_sense_block_descriptor_length(offset)
int offset;
{
    uint32_t operation_result;
    if (((*(uint8_t *)(*(int *)(offset + 8) + 2) & 0x3f) == 5) || ((*(uint8_t *)(*(int *)(offset + 8) + 1) >> 3 & 1) != 0)) {
        operation_result = 0;
    } else {
        operation_result = 8;
    }
    return operation_result;
}
uint32_t core_process_command_context_value()
{
    int operation_status;
    uint32_t operation_result;
    operation_status = core_get_command_context_value();
    operation_result = 1;
    if (operation_status == 0) {
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_update_availability_x01(operation_status + 0x2c);
        if (operation_status != 0) {
            operation_result = 0;
        }
    }
    return operation_result;
}
int core_get_mww_mode_page_buffer(offset)
int offset;
{
    *(uint8_t **)(offset + 0x14) = (uint8_t *)&mww_mode_page_buffer;
    return offset + 0x10;
}
void core_process_command_feature_state(offset) int offset;
{
    uint8_t mode_flags;
    int core_update_command_feature_state_result;
    mode_flags = *(uint8_t *)(*(int *)(offset + 8) + 4);
    if (((mode_flags & 1) == 0) ||
        (core_update_command_feature_state_result = ((bool (*)())core_update_command_phase)(offset, mode_flags >> 1), core_update_command_feature_state_result == 0)) {
        core_update_command_feature_state(mode_flags & 1);
    }
    return;
}
void core_process_runtime_context_base(buffer) uint32_t *buffer;
{
    int operation_status;
    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    operation_status = core_accumulate_split_transfer_offsets(operation_status + 0x2c);
    if ((operation_status == 0) || (operation_status = core_process_ata_command_for_ata_standby_context(), operation_status == 0)) {
        *buffer = 0x16;
    }
    return;
}
void core_update_cddis_reg_off_cartridge_eject_control()
{
    if (0x101a < usb_core_revision) {
        CDDIS_REG_OFF = 3;
    }
    return;
}
uint32_t core_get_mww_active_flag(condition)
uint32_t condition;
{
    uint32_t *data_cursor;
    if ((condition >> 7 & 1) == 0) {
        data_cursor = (uint32_t *)&mww_window_transfer_active;
    } else {
        data_cursor = (uint32_t *)&core_mww_window_transfer_state;
    }
    return data_cursor[(condition & 0xffffff7f) * 10];
}
void core_update_firmware_record_update(condition, value) uint32_t condition;
uint32_t value;
{
    uint8_t *data_cursor;
    if ((condition >> 7 & 1) == 0) {
        data_cursor = (uint8_t *)&core_firmware_record_update;
    } else {
        data_cursor = (uint8_t *)&core_firmware_record_update_alternate_table;
    }
    *(uint32_t *)(data_cursor + (condition & 0xffffff7f) * 4 + -8) = value;
    return;
}
uint32_t core_is_regular_request_code(offset)
int offset;
{
    if ((((offset != 0x37) && (offset != 0xf1)) && (3 < offset - 0xf3U)) && (offset - 0xf3U != 6)) {
        return 1;
    }
    return 0;
}
bool core_get_0x40(offset, buffer) int offset;
uint8_t *buffer;
{
    bool offset_field_0x40;
    offset_field_0x40 = (*(uint32_t *)(offset + 0x40) & 0xff) == 1;
    if ((offset_field_0x40) && (buffer != (uint8_t *)0x0)) {
        *buffer = (char)(*(uint32_t *)(offset + 0x40) >> 8);
    }
    return offset_field_0x40;
}
uint32_t core_update_x01_x02(offset)
int offset;
{
    char status_bits;
    uint32_t operation_result;
    status_bits = *(char *)(offset + 0x10);
    operation_result = 0;
    if (((status_bits == '\x01') || (status_bits == '\x02' || status_bits == '\x04')) && (*(int *)(offset + 0x1c) == 0)) {
        operation_result = 1;
    }
    return operation_result;
}
int core_get_payload_offset_from_flags(offset)
int offset;
{
    int working_result;
    if ((*(uint8_t *)(offset + 1) >> 2 & 1) == 0) {
        if ((*(uint8_t *)(offset + 1) & 1) == 0) {
            working_result = 4;
        } else {
            working_result = 0xc;
        }
    } else {
        working_result = 0x14;
    }
    return working_result + *(int *)(offset + 0x14);
}
uint32_t core_copy_source_record_header(source_record, destination_record)
uint32_t *source_record;
uint32_t *destination_record;
{
    int core_process_capacity_block_context_derived;
    core_process_capacity_block_context_derived = ((uint32_t (*)())core_process_capacity_block_context)();
    if (core_process_capacity_block_context_derived != 0) {
        destination_record[2] = (uint32_t)source_record;
        *destination_record = *source_record;
        destination_record[1] = source_record[1];
    }
    return 0;
}
int core_validate_device_descriptor_length(context, value, offset, flags)
uint32_t context;
uint32_t value;
int offset;
uint32_t flags;
{
    int core_get_ti_memset_derived;
    core_get_ti_memset_derived = ((uint32_t (*)())core_get_ti_memset)(context, value, offset, flags, 0xc0);
    if ((core_get_ti_memset_derived != 0) && (*(uint16_t *)(offset + 8) < 0x4c)) {
        core_get_ti_memset_derived = 0;
    }
    return core_get_ti_memset_derived;
}
uint32_t core_has_device_descriptor_high_bit(offset)
int offset;
{
    uint32_t operation_result;
    operation_result = 0;
    if (((*(int16_t *)(offset + 8) != 0) && (*(uint8_t **)(offset + 0xc) != (uint8_t *)0x0)) &&
        ((**(uint8_t **)(offset + 0xc) & 0x80) == 0x80)) {
        operation_result = 1;
    }
    return operation_result;
}
int core_handle_type_type(offset, value, length, flags)
int offset;
uint32_t value;
uint32_t length;
uint32_t flags;
{
    char operation_status;
    uint16_t record_type_code;
    int16_t record_type;
    int operation_result;
    uint32_t core_release_record_payload_when_short_result;
    uint32_t core_get_constant_hex_1000_value_result;
    int computed_offset;
    uint32_t length_component;
    volatile uint32_t flags_snapshot = 0;
    if (**(int8_t **)(offset + 8) != -0x5e) {
        if (**(int8_t **)(offset + 8) == -0x4b) {
            computed_offset = offset + 4;
            length_component = length;
            flags_snapshot = flags;
            operation_status = core_release_record_payload_when_short(computed_offset, 1, 1);
            operation_result = core_get_validation_flags();
            if (operation_result == 0) {
                operation_result = 8;
                if ((operation_status == ' ') && (record_type = core_release_record_payload_when_short(computed_offset, 2, 3), record_type == 0x10)) {
                    core_release_record_payload_when_short_result = core_release_record_payload_when_short(computed_offset, 6, 9);
                    core_get_constant_hex_1000_value_result = core_get_constant_hex_1000_value();
                    if (core_release_record_payload_when_short_result <= core_get_constant_hex_1000_value_result) {
                        operation_result = ((uint32_t (*)())core_update_command_validation_context)(offset, &length_component, core_release_record_payload_when_short_result & 0xffff);
                        if (operation_result == 0) {
                            operation_result = 0x1f;
                        } else {
                            operation_result = ((uint64_t (*)())core_dispatch_rdx_management_record)(&length_component);
                        }
                    }
                }
            } else {
                operation_result = 0x2c;
            }
            return operation_result;
        }
        return 7;
    }
    operation_status = *(char *)(*(int *)(offset + 8) + 1);
    operation_result = 8;
    if (operation_status == '\0') {
        record_type = core_release_record_payload_when_short(offset + 4, 2, 3);
        if (record_type == 0) {
            operation_result = core_build_record_type_two_response(offset);
        } else if (record_type == 1) {
            core_process_ti_memset_for_build_scsi_mode(offset, 4);
            goto core_handle_type_type_before_update_release_record_payload;
        }
    } else if (operation_status == ' ') {
        record_type_code = core_release_record_payload_when_short(offset + 4, 2, 3);
        if (record_type_code == 0) {
            operation_result = core_build_supported_record_types_response(offset);
        } else if (record_type_code == 1) {
            operation_result = core_build_record_type_one_response(offset);
        } else if (record_type_code == 0x10) {
            operation_result = core_process_ti_memset_for_type_type(offset);
        } else if (record_type_code == 0x11) {
            operation_result = core_build_record_type_eleven_response(offset);
        } else if (record_type_code == 0x12) {
            operation_result = core_build_record_type_twelve_response(offset);
        } else if (record_type_code == 0x20) {
            operation_result = core_handle_descriptor_clear_memory_device_descriptor_context(offset);
        } else if (record_type_code == 0x21) {
            operation_result = core_handle_device_descriptor_response_flag_device_descriptor_context(offset);
        } else if (0xfeff < record_type_code) {
            if (record_type_code < 0xff08) {
                operation_result = core_handle_response_base_device_descriptor_context(offset, (char)record_type_code);
            } else if ((0xff0f < record_type_code) && (record_type_code < 0xff18)) {
                operation_result = core_handle_word_metadata(offset, record_type_code - 0x10 & 0xff);
            }
        }
    }
    if (operation_result != 0) {
        return operation_result;
    }
core_handle_type_type_before_update_release_record_payload:
    if (*(int *)(offset + 0xc) != 0) {
        core_update_release_record_payload(*(int *)(offset + 0xc), offset, 6, 9);
    }
    return 0;
}
uint32_t core_process_request_for_mww_saved_request(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;
{
    int core_is_supported_transfer_opcode_derived;
    core_is_supported_transfer_opcode_derived = ((uint32_t (*)())core_is_supported_transfer_opcode)();
    if (core_is_supported_transfer_opcode_derived != 0) {
        core_process_request_request_for_request_request(context, value, length);
    }
    return 0;
}
void core_write_selected_sense_fields(context, value, length) uint8_t context;
uint8_t value;
uint32_t length;
{
    core_update_primary_sense_field_context(value);
    core_update_secondary_sense_field_context(length);
    core_update_response_buffer_context(context);
    return;
}
bool core_get_pending_transfer_buffer(context, buffer) uint32_t context;
uint32_t *buffer;
{
    int core_transfer_spi_transfer_storage_derived;
    core_transfer_spi_transfer_storage_derived = (int)((uint8_t *(*)())core_transfer_spi_transfer_storage)();
    if (core_transfer_spi_transfer_storage_derived != 0) {
        *buffer = *(uint32_t *)(core_transfer_spi_transfer_storage_derived + 4);
    }
    return core_transfer_spi_transfer_storage_derived != 0;
}
