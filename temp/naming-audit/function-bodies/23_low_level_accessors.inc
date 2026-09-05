
uint32_t core_update_spi_configure_spi(context)
uint32_t context;
{
    spi_transfer_word(context | 0x60000);
    return 0;
}
uint8_t *core_get_response_end_context_pointer()
{
    return (uint8_t *)&core_response_end_context;
}
void core_update_thermal_monitor_enabled_for_read_smart_temperature(buffer) uint32_t *buffer;
{
    *buffer = 0;
    core_thermal_monitor_enabled = 0;
    return;
}
uint32_t *core_get_persistent_identity_record_pointer()
{
    return (uint32_t *)&core_persistent_identity_record;
}
void core_sha256_update(offset, buffer, length) int offset;
uint8_t *buffer;
int length;
{
    uint32_t offset_field_0x44;
    offset = offset + 300;
    for (; length != 0; length = length + -1) {
        *(uint8_t *)(offset + *(int *)(offset + 0x40)) = *buffer;
        *(int *)(offset + 0x40) = *(int *)(offset + 0x40) + 1;
        if (*(int *)(offset + 0x40) == 0x40) {
            offset = ((uint32_t (*)())core_sha256_compress_block)(offset, offset);
            offset_field_0x44 = *(uint32_t *)(offset + 0x44);
            *(uint32_t *)(offset + 0x40) = 0;
            *(uint32_t *)(offset + 0x44) = offset_field_0x44 + 0x200;
            *(uint32_t *)(offset + 0x48) = *(int *)(offset + 0x48) + (uint32_t)(0xfffffdff < offset_field_0x44);
        }
        buffer = buffer + 1;
    }
    return;
}
uint32_t core_get_runtime_context_base_for_build_scsi_mode()
{
    return 0x800f324;
}
uint32_t core_submit_configured_payload(context, buffer, length)
uint32_t context;
uint16_t *buffer;
uint32_t length;
{
    ((uint32_t (*)())core_configure_payload_ti_memset)(context, *(uint32_t *)(buffer + 2), length, *buffer);
    return 0;
}
void core_update_spi_set_logical_for_monitor_cartridge_input(context) uint32_t context;
{
    spi_set_logical_output(5, context);
    return;
}
uint8_t core_read_context_field_6c_byte(offset)
int offset;
{
    return *(uint8_t *)(offset + 0x6c);
}
uint8_t core_get_structure_field_hex_70_value(offset)
int offset;
{
    return *(uint8_t *)(offset + 0x70);
}
uint32_t core_get_descriptor_requested(requested_length, descriptor)
uint32_t requested_length;
int descriptor;
{
    uint32_t operation_result;
    operation_result = 0;
    if (((requested_length < 0x1fc) && (*(uint16_t *)(descriptor + 2) < 0x1fc)) && (requested_length + *(uint16_t *)(descriptor + 2) < 0x1fc)) {
        operation_result = 1;
    }
    return operation_result;
}
uint32_t core_update_sci_read_logical_for_scsi_logical_address_context()
{
    sci_read_logical_input(3);
    return 0;
}
void core_mark_primary_sense_record_not_ready(offset) int offset;
{
    if (*(char *)(offset + 7) == '\0') {
        *(uint8_t *)(offset + 7) = 1;
    }
    *(uint8_t *)(offset + 8) = 6;
    return;
}
void core_mark_secondary_sense_record_not_ready(offset) int offset;
{
    if (*(char *)(offset + 7) == '\0') {
        *(uint8_t *)(offset + 7) = 1;
    }
    *(uint8_t *)(offset + 8) = 6;
    return;
}
void core_release_nested_record_resource(offset) int offset;
{
    ((void (*)())core_update_thermal_monitor_enabled_for_read_smart_temperature)(*(int *)(offset + 8) + 8);
    return;
}
uint32_t core_update_sci_read_logical_for_sci_read_logical()
{
    sci_read_logical_input(0);
    return 0;
}
uint32_t core_update_sci_read_logical_for_advance_mechanism_state()
{
    sci_read_logical_input(1);
    return 0;
}
uint32_t core_release_record_buffer(offset)
int offset;
{
    ((uint32_t (*)())core_transfer_range_end_range_start_for_descriptor_descriptor_resource)(*(uint32_t *)(offset + 4));
    return 0;
}
uint32_t core_get_constant_hex_1014_value()
{
    return 0x1014;
}
uint32_t core_get_constant_hex_1000_value()
{
    return 0x1000;
}
void core_update_mww_process_transfer()
{
    mww_process_transfer_descriptor_mww0_addr_mww_transfer_descriptor();
    return;
}
uint32_t core_read_mode_page_field_1c_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x1c);
}
uint32_t core_read_mode_page_field_48_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x48);
}
uint32_t core_read_context_field_28_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x28);
}
uint32_t core_read_descriptor_field_24_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x24);
}
uint32_t core_read_descriptor_field_20_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x20);
}
uint32_t core_get_structure_field_hex_4c_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x4c);
}
uint32_t core_get_structure_field_hex_50_value(offset)
int offset;
{
    return *(uint32_t *)(offset + 0x50);
}
void core_forward_ti_memset(offset) int offset;
{
    ti_memset(offset + 0x10, 0, 0x34);
    ti_memset(offset + 0x44, 0, 0x38);
    return;
}
uint32_t core_get_default_record_type_code()
{
    return 0x15;
}
void core_set_notification_state_to_zero()
{
    core_notification_state = 0;
    return;
}
uint32_t core_read_buffer_word(buffer)
uint32_t *buffer;
{
    return *buffer;
}
uint32_t core_get_primary_success_flag()
{
    return 1;
}
bool core_forward_mww_transfer_value()
{
    core_mww_transfer_value = 1;
    return core_thermal_monitor_enabled == '\0';
}
uint32_t core_get_secondary_success_flag()
{
    return 1;
}
uint32_t core_get_constant_one_value_for_ata_standby_context()
{
    return 1;
}
void core_forward_cartridge_input_state()
{
    int operation_status;
    operation_status = core_get_notification_context_value();
    if ((operation_status == 2) && (operation_status = core_is_cartridge_input_state_six(), operation_status != 0)) {
        core_update_reconnect_state();
    }
    core_cartridge_input_state = 0;
    core_set_command_runtime_state_to_zero();
    core_sense_selection_flags = 0;
    core_update_enabled(0x800f2b4, 0);
    core_update_store_context_enable(0);
    core_update_clear_memory_response_end_context();
    core_process_cartridge_input_state();
    core_process_command_descriptor_context();
    core_process_primary_command_context();
    if (core_ata_standby_context != '\0') {
        core_process_runtime_context_base_for_cartridge_input_state();
        core_ata_standby_context = '\0';
    }
    core_update_clear_memory_response_end_context();
    core_cartridge_link_retry_count = 0;
    core_cartridge_input_raw_level = 0;
    core_mode_processing_context = 0;
    core_set_mechanism_phase_to_zero();
    return;
}
uint8_t core_forward_mcp3008_sample_value()
{
    return core_mcp3008_sample;
}
uint32_t core_get_condition_response_code()
{
    return 0x20;
}
uint32_t core_get_extended_response_code()
{
    return 0xac;
}
void core_store_context_enable_byte(context, value) int context;
uint8_t value;
{
    *(uint8_t *)(context + 7) = value;
    return;
}
bool core_is_command_phase_three()
{
    return core_command_phase == 3;
}
uint32_t core_forward_sci_read_logical()
{
    sci_read_logical_input(3);
    return 0;
}
void core_forward_interrupt_transfer_context(condition) uint32_t condition;
{
    uint32_t *core_interrupt_transfer_context_pointer;
    uint32_t condition_field;
    int operation_status;
    operation_status = 0x13;
    core_interrupt_transfer_context_pointer = (uint32_t *)&core_interrupt_transfer_context;
    do {
        core_interrupt_transfer_context_pointer = core_interrupt_transfer_context_pointer + 1;
        if (*core_interrupt_transfer_context_pointer != 0) {
            if (condition < *core_interrupt_transfer_context_pointer) {
                condition_field = *core_interrupt_transfer_context_pointer - condition;
            } else {
                condition_field = 0;
            }
            *core_interrupt_transfer_context_pointer = condition_field;
        }
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    core_interrupt_timer_elapsed_ticks = condition + core_interrupt_timer_elapsed_ticks;
    return;
}
void core_forward_end_remaining(destination, source, length) uint32_t *destination;
uint32_t *source;
int length;
{
    uint32_t *buffer_2_cursor;
    uint32_t *buffer_1_cursor;
    uint32_t *source_end;
    int remaining_count;
    source_end = (uint32_t *)((int)source + length);
    if (((((uint32_t)source | (uint32_t)destination) & 3) == 0) && ((uint32_t *)((uint32_t)source_end & 0xfffffffc) != source)) {
        remaining_count = (int)((uint32_t)source_end & 0xfffffffc) - (int)source >> 2;
        buffer_2_cursor = source;
        buffer_1_cursor = destination;
        do {
            source = buffer_2_cursor + 1;
            remaining_count = remaining_count + -1;
            destination = buffer_1_cursor + 1;
            *buffer_1_cursor = *buffer_2_cursor;
            buffer_2_cursor = source;
            buffer_1_cursor = destination;
        } while (remaining_count != 0);
    }
    if (source_end != source) {
        remaining_count = (int)source_end - (int)source;
        do {
            remaining_count = remaining_count + -1;
            *(uint8_t *)destination = *(uint8_t *)source;
            source = (uint32_t *)((int)source + 1);
            destination = (uint32_t *)((int)destination + 1);
        } while (remaining_count != 0);
    }
    return;
}
uint32_t core_get_constant_zero_value_for_mww_handle_transfer()
{
    return 0;
}
uint32_t core_is_supported_forward_opcode(context)
uint32_t context;
{
    uint32_t operation_result;
    int working_result;
    uint64_t core_is_supported_descriptor_opcode_derived;
    core_is_supported_descriptor_opcode_derived = ((uint32_t (*)())core_is_supported_descriptor_opcode)(context, context);
    working_result = (int)((uint64_t)core_is_supported_descriptor_opcode_derived >> 0x20);
    if (((((int)core_is_supported_descriptor_opcode_derived == 0) && (working_result != 8)) && (working_result != 0x28)) &&
        ((working_result != 0x88 && (working_result != 0xa8)))) {
        operation_result = 0;
    } else {
        operation_result = 1;
    }
    return operation_result;
}
uint32_t core_get_default_mode_page_flags()
{
    return 0;
}
uint8_t core_forward_cartridge_input_value()
{
    return core_cartridge_input_value;
}
uint32_t core_get_constant_one_value_for_request_request_for()
{
    return 1;
}
