
void core_process_cartridge_input_state()
{
    int sci_read_logical_input_result;
    if (core_cartridge_input_state == 6) {
        core_cartridge_input_state = 2;
        sci_read_logical_input_result = sci_read_logical_input(0);
        core_process_command_phase(sci_read_logical_input_result == 0);
    }
    return;
}
bool core_process_ata_command_for_lookup_key_metadata(context) uint32_t context;
{
    int core_process_ata_command_for_ata_standby_context_result;
    int working_result;
    ((void (*)())core_reset_output_record)();
    core_process_ata_command_for_ata_standby_context_result = core_process_ata_command_for_ata_standby_context();
    working_result = core_get_runtime_context_base_for_build_scsi_mode();
    working_result = core_initialize_output_record_from_source(working_result + 0x2c, context);
    return working_result != 0 && core_process_ata_command_for_ata_standby_context_result != 0;
}
bool core_get_gpio_pin_context()
{
    int core_submit_ata_standby_immediate_result;
    core_submit_ata_standby_immediate_result = core_submit_ata_standby_immediate(0);
    if (core_submit_ata_standby_immediate_result != 0) {
        core_process_device_status_context(core_submit_ata_standby_immediate_result);
        core_gpio_pin_context = 0;
        core_process_command_state_context();
    }
    return core_submit_ata_standby_immediate_result != 0;
}
uint64_t core_process_ti_memset_ti_memset(context, offset, length, flags)
uint32_t context;
int offset;
uint32_t length;
uint32_t flags;
{
    uint32_t ti_memset_result;
    uint32_t context_derived;
    int result_context;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;
    context_derived = context;
    result_context = offset;
    length_component = length;
    flags_snapshot = flags;
    ((uint64_t (*)())core_process_clear_memory_clear_memory_secondary_memory_region)(&context_derived, offset + 0xc);
    ((void (*)())core_process_usb_event_context)(context, &context_derived, offset);
    ti_memset_result = (uint32_t)ti_memset(&context_derived, 0, 0x10);
    return CONCAT44(result_context, ti_memset_result);
}
bool core_update_flash_completion_context()
{
    int operation_status;
    operation_status = ((int (*)())core_program_spi_flash_pages)(0, 4, &core_flash_completion_context);
    if (operation_status == 0) {
        core_transfer_ready_flag = 0;
        core_mcp3008_sample = 0;
    }
    return operation_status == 0;
}
uint32_t core_update_device_update_context(condition, value, length, flags)
int condition;
uint32_t value;
uint32_t length;
uint32_t flags;
{
    int operation_status;
    uint32_t operation_result;
    uint32_t flags_snapshot;
    flags_snapshot = flags;
    operation_status = ((int (*)())core_read_mcp3008_channel)(condition, &flags_snapshot);
    operation_result = 0;
    if ((operation_status == 0) && ((uint32_t)*(uint16_t *)(&core_device_update_context + condition * 2) < (flags_snapshot & 0xffff))) {
        operation_result = 1;
    }
    return operation_result;
}
void core_set_channel_mode_bits(context, condition) int context;
int condition;
{
    uint32_t context_derived;
    context_derived = *(uint32_t *)(context * 0x400 + -0x877f8);
    if (condition == 0) {
        context_derived = context_derived & 0xffffffdc;
    } else {
        context_derived = context_derived & 0xfffffffc | 0x20;
    }
    *(uint32_t *)(context * 0x400 + -0x877f8) = context_derived;
    return;
}
uint32_t core_update_sense_record_context()
{
    int core_get_thermal_control_context_result;
    uint32_t operation_result;
    operation_result = 1;
    if (((core_sense_record_context == '\0') && (core_get_thermal_control_context_result = core_get_thermal_control_context(&core_command_completion_context), core_get_thermal_control_context_result != 0)) &&
        (core_mechanism_output_state == 4 || core_mechanism_output_state == 8)) {
        operation_result = 0;
    }
    return operation_result;
}
uint32_t core_update_multiplier_multiplier(value_words, value, high_multiplier, low_multiplier)
uint32_t *value_words;
int value;
int high_multiplier;
int low_multiplier;
{
    int value_derived;
    uint32_t buffer_1_cursor;
    value_derived = value * low_multiplier + value * high_multiplier * 0x10000;
    buffer_1_cursor = *value_words - value_derived;
    *value_words = buffer_1_cursor;
    if (-value_derived - 1U < buffer_1_cursor) {
        value_words[1] = value_words[1] - 1;
    }
    value_words[1] = value_words[1] - ((uint32_t)(value * high_multiplier) >> 0x10);
    return 0;
}
bool core_register_usb_event_callback(context) uint32_t context;
{
    uint32_t core_usb_event_callback_count_snapshot;
    core_usb_event_callback_count_snapshot = (uint32_t)core_usb_event_callback_count;
    if (3 >= core_usb_event_callback_count_snapshot) {
        *(uint32_t *)(&core_usb_event_callbacks + core_usb_event_callback_count_snapshot * 4) = context;
        core_usb_event_callback_count = core_usb_event_callback_count + 1;
    }
    return 3 < core_usb_event_callback_count_snapshot;
}
void core_write_descriptor_fields(context, buffer, length) uint32_t context;
uint8_t *buffer;
uint32_t length;
{
    ((void (*)())core_transfer_tracker)(context, *buffer, 1, length);
    ((void (*)())core_transfer_tracker)(context, buffer[1], 1);
    ((void (*)())core_transfer_tracker)(context, *(uint16_t *)(buffer + 2), 2);
    return;
}
uint64_t core_get_structure_field_hex_for_response_end_word(offset)
int offset;
{
    uint32_t core_get_structure_field_hex_for_structure_field_hex_result;
    int callback_result = 0;
    if (*(char *)(offset + 0x6f) != '\0') {
        core_get_structure_field_hex_for_structure_field_hex_result = core_get_structure_field_hex_for_structure_field_hex(offset);
        return CONCAT44(*(int *)(callback_result + 0x68) - (uint32_t)(*(uint32_t *)(callback_result + 100) < core_get_structure_field_hex_for_structure_field_hex_result),
                        *(uint32_t *)(callback_result + 100) - core_get_structure_field_hex_for_structure_field_hex_result);
    }
    return 0;
}
void core_process_copy(buffer, destination_offset, length, copy_length) int *buffer;
int destination_offset;
uint32_t length;
int copy_length;
{
    if (0x800 < (uint32_t)(copy_length + destination_offset)) {
        core_halt_forever();
    }
    core_forward_end_remaining(*buffer + destination_offset * 2, length, copy_length << 1);
    return;
}
int core_get_runtime_context_base_for_command_descriptor_device(condition)
int condition;
{
    int operation_result;
    operation_result = core_get_runtime_context_base_for_build_scsi_mode();
    operation_result = core_is_regular_request_code(condition, operation_result + 0x2c);
    if (((operation_result != 0) && (condition != 0xb0)) && (condition != 0xe0)) {
        core_process_command_state_context();
    }
    return operation_result;
}
uint32_t core_is_supported_descriptor_opcode(condition)
int condition;
{
    if ((((condition != 10) && (condition != 0x2a)) && (condition != 0x2e)) &&
        (((condition != 0x8a && (condition != 0x8e)) && (condition != 0xaa && condition != 0xae)))) {
        return 0;
    }
    return 1;
}
bool core_update_valid(condition, buffer) int condition;
int *buffer;
{
    bool entry_valid;
    int working_result;
    entry_valid = false;
    if ((*buffer == 0) || (condition != 0)) {
        working_result = 0;
    } else {
        entry_valid = true;
        if (buffer[2] == 3) {
            working_result = 4;
        } else {
            entry_valid = buffer[2] != 0;
            working_result = 2;
        }
    }
    buffer[2] = working_result;
    return entry_valid;
}
void core_process_clear_memory_for_cartridge_monitor_context(offset) int offset;
{
    ti_memset(offset, 0, 0x14);
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(offset + 0xd);
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(offset + 0xe);
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(offset + 0xf);
    return;
}
uint32_t core_update_constant_0x12_x01(offset)
int offset;
{
    char offset_field_0x12;
    uint32_t operation_result;
    operation_result = 0;
    if ((*(int *)(offset + 0x44) == 2) && (((offset_field_0x12 = *(char *)(offset + 0x12),
                                             offset_field_0x12 == '\x01' || (offset_field_0x12 == '\x02' || offset_field_0x12 == '\x04')) &&
                                            ((*(uint32_t *)(offset + 0x24) & ~*(uint32_t *)(offset + 0x74)) == 0)))) {
        operation_result = 1;
    }
    return operation_result;
}
uint32_t core_update_device_for_component_for_device(offset, value, length)
int offset;
uint32_t value;
uint32_t length;
{
    int core_update_device_for_device_for_component_result;
    uint32_t operation_result;
    *(uint8_t *)(offset + 8) = 1;
    core_update_device_for_device_for_component_result = core_update_device_for_device_for_component(value, length);
    if (core_update_device_for_device_for_component_result == 0) {
        operation_result = 3;
    } else {
        operation_result = 0;
    }
    *(uint8_t *)(offset + 8) = 0;
    return operation_result;
}
uint32_t core_update_device_for_device_ti_memset(context, value, length)
int context;
uint32_t value;
uint32_t length;
{
    int core_update_device_for_device_for_device_result;
    uint32_t operation_result;
    *(uint8_t *)(context + 9) = 1;
    core_update_device_for_device_for_device_result = core_update_device_for_device_for_device(value, length);
    if (core_update_device_for_device_for_device_result == 0) {
        operation_result = 4;
    } else {
        operation_result = 0;
    }
    *(uint8_t *)(context + 9) = 0;
    return operation_result;
}
void core_transfer_pending_ata_queue()
{
    int operation_status;
    uint8_t *ata_callback_pending_pointer;
    operation_status = 0x10;
    ata_callback_pending_pointer = (uint8_t *)&ata_callback_pending;
    do {
        operation_status = operation_status + -1;
        *ata_callback_pending_pointer = 0;
        ata_callback_pending_pointer = ata_callback_pending_pointer + 1;
    } while (operation_status != 0);
    core_ata_queue_context = 0;
    ata_callback_processing_index = 0;
    return;
}
bool core_get_ahci_control_register(context) int context;
{
    bool ahci_port_command_register_pointer;
    ahci_port_command_register_pointer = (*(uint32_t *)(&ahci_port_command_register + context * 0x80) & 1) != 0;
    if (!ahci_port_command_register_pointer) {
        core_update_masked_register_bits(&ahci_port_command_register + context * 0x80, 0x10, 0x10);
    }
    return ahci_port_command_register_pointer;
}
uint32_t core_process_command_processing_context()
{
    int operation_status;
    uint32_t operation_result;
    operation_result = 0;
    operation_status = core_get_flash_validation_context_for_validate_structured_flash(0);
    if ((operation_status == 0x38) && (operation_status = core_get_thermal_control_context(&core_command_processing_context), operation_status == 0)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_transfer_x01(destination, source, condition) char *destination;
char *source;
int condition;
{
    char buffer_2_cursor;
    char working_result;
    if (condition != 0) {
        working_result = '\0';
        do {
            condition = condition + -1;
            buffer_2_cursor = *source - working_result;
            working_result = working_result + '\x01';
            *destination = buffer_2_cursor + -0x54;
            source = source + 1;
            destination = destination + 1;
        } while (condition != 0);
    }
    return;
}
uint32_t core_process_constant_0x24_fixed_command_context(offset)
int offset;
{
    uint16_t offset_field_0x24;
    uint32_t core_fixed_command_context_pointer;
    uint32_t status_bits;
    uint8_t working_buffer[256];
    if (8 < *(uint16_t *)(offset + 0x24)) {
        offset_field_0x24 = *(uint16_t *)(offset + 0x24) - 8;
        status_bits = (uint32_t)offset_field_0x24;
        core_fixed_command_context_pointer = 0;
        if (((status_bits != 0) && ((offset_field_0x24 & 7) == 0)) && (status_bits < 0x104)) {
            ((void (*)())core_transfer_fourth_byte_byte)(working_buffer, offset + 0x2c, (int)status_bits >> 2 & 0xff);
            core_fixed_command_context_pointer = ((uint32_t (*)())core_transfer_packed_command_words_command_words)(offset + 300, working_buffer, &core_fixed_command_context);
        }
        return core_fixed_command_context_pointer;
    }
    return 0;
}
uint32_t core_translate_command_code(context, value)
uint32_t context;
uint32_t value;
{
    int core_is_command_code_in_standard_range_result;
    uint32_t operation_result;
    int callback_result = 0;
    core_is_command_code_in_standard_range_result = core_is_command_code_in_standard_range(context, value, context);
    if (core_is_command_code_in_standard_range_result == 0) {
        operation_result = 0xff;
    } else {
        operation_result = ((callback_result - 0x36U & 7) - (callback_result - 0x36U & 0xf8)) + 0x78 & 0xff;
    }
    return operation_result;
}
int core_compare_buffers_from_end(context, value, condition)
int context;
int value;
int condition;
{
    char *value_derived;
    char *context_derived;
    if (condition != 0) {
        context_derived = (char *)(context + condition);
        value_derived = (char *)(value + condition);
        do {
            value_derived = value_derived + -1;
            context_derived = context_derived + -1;
            condition = condition + -1;
            if (*value_derived != *context_derived) {
                return condition;
            }
        } while (condition != 0);
    }
    return 0;
}
uint32_t core_process_ata_command_buffer_for_clear_memory_lookup(context)
uint32_t context;
{
    uint32_t operation_result;
    int working_result;
    uint64_t core_get_runtime_context_base_for_build_scsi_mode_result;
    operation_result = (uint32_t)core_get_ata_command_buffer();
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode(operation_result, operation_result);
    working_result = (int)((uint64_t)core_get_runtime_context_base_for_build_scsi_mode_result >> 0x20);
    operation_result = ((bool (*)())core_configure_initialize_output_record)((int)core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, *(uint32_t *)(working_result + 8),
                                                        *(uint32_t *)(working_result + 0xc));
    core_process_configuration_block_payload(context);
    return operation_result;
}
void core_process_configuration_block_payload(context) uint32_t context;
{
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint8_t working_buffer[40];
    uint8_t payload_buffer[296];
    uint8_t configuration_block[136];
    core_configure_word_descriptor(working_buffer, context, payload_buffer, configuration_block);
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    ((void (*)())core_transfer_command_sequence_update_flag)(working_buffer, core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    return;
}
uint32_t core_build_record_type_two_response(context)
uint32_t context;
{
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;
    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 10);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 6, 7, 2);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 9, 9, 0x20);
    return 0;
}
bool core_update_constant_0xc_for_word_words_resource(offset, buffer) int offset;
uint32_t *buffer;
{
    int offset_field_0xc;
    bool buffer_cursor;
    buffer_cursor = false;
    if ((((*(uint16_t *)(offset + 8) < 4) || (offset_field_0xc = *(int *)(offset + 0xc), offset_field_0xc == 0)) ||
         (*(char *)(offset_field_0xc + 2) != '\0')) ||
        (buffer_cursor = *(char *)(offset_field_0xc + 3) == '\0', !buffer_cursor)) {
        *buffer = 0x2b;
    }
    return buffer_cursor;
}
void core_process_input_pin_state()
{
    spi_set_logical_output(0xe, 1);
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(&core_input_pin_state);
    rti_process_thermal_timer_state(&core_input_pin_state, 30000);
    return;
}
uint32_t core_update_default_mww_transfer_value()
{
    uint32_t operation_result;
    int callback_result = 0;
    uint32_t default_result = 0;
    core_mww_transfer_value = 1;
    if (core_thermal_monitor_enabled == '\0') {
        core_set_thermal_monitor_enabled_to_one(0);
        *(uint8_t *)(callback_result + 3) = 0;
        operation_result = default_result;
    } else {
        operation_result = 0;
    }
    return operation_result;
}
uint8_t *core_transfer_spi_transfer_storage(condition)
uint32_t condition;
{
    uint8_t *core_spi_transfer_storage_pointer;
    int operation_status;
    operation_status = 0x2a;
    core_spi_transfer_storage_pointer = (uint8_t *)&core_spi_transfer_storage;
    do {
        core_spi_transfer_storage_pointer = core_spi_transfer_storage_pointer + 0x18;
        if (condition == *core_spi_transfer_storage_pointer) {
            return core_spi_transfer_storage_pointer;
        }
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    return (uint8_t *)0x0;
}
uint32_t core_is_supported_transfer_opcode(context)
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
uint32_t core_update_command_validation_context(context, buffer)
uint32_t context;
uint16_t *buffer;
{
    int core_command_validation_context_snapshot;
    core_command_validation_context_snapshot = (int)core_command_validation_context;
    *(uint8_t **)(buffer + 2) = (uint8_t *)&mww_mode_page_buffer;
    *buffer = *(uint16_t *)(core_command_validation_context_snapshot + 4);
    return 1;
}
