
uint32_t core_get_command_feature(command_code, feature_flags)
int command_code;
uint32_t feature_flags;
{
    uint32_t operation_result;
    if (((command_code == 0x2a) || (command_code == 0x8a || command_code == 0xaa)) && ((feature_flags >> 3 & 1) != 0)) {
        operation_result = 1;
    } else {
        operation_result = 0;
    }
    return operation_result;
}
uint32_t core_update_transfer(context, buffer)
uint32_t context;
uint8_t *buffer;
{
    uint8_t working_buffer[24];
    uint8_t transfer_state[24];
    ((int (*)())core_process_transfer_requested)(context, buffer[1], *buffer, transfer_state, working_buffer);
    return 0;
}
void core_update_sense_update_context(context) uint8_t context;
{
    int core_get_response_type_value_result;
    core_get_response_type_value_result = core_get_response_type_value();
    if (core_get_response_type_value_result != 0) {
        core_sense_update_context = context;
    }
    return;
}
void core_process_set_validation_flags(context) int context;
{
    core_set_validation_flags_flag_bit_three(context == 0);
    core_process_command_runtime_state();
    core_forward_cartridge_input_state();
    return;
}
void core_process_command_runtime_state()
{
    core_command_runtime_state = 0;
    ahci_init_port();
    core_process_ahci_port_control();
    return;
}
uint32_t core_check_logical_input_and_state()
{
    int operation_status;
    uint32_t operation_result;
    operation_status = sci_read_logical_input(4);
    operation_result = 0;
    if ((operation_status != 0) && (operation_status = core_process_flash_validation_context(), operation_status != 0)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_update_mechanism_update_context_for_primary_sense_context()
{
    if (core_mechanism_update_context != '\0') {
        rti_process_thermal_timer_state(&core_cartridge_eject_wait_timer, 5000);
        return;
    }
    return;
}
void core_update_persistent_update_context()
{
    core_persistent_update_context = 0xff;
    core_smart_monitoring_active = 0;
    core_thermal_hot_sample_retry_count = 0;
    return;
}
uint32_t core_process_identity_update_context()
{
    core_unit_identity_record = core_update_identity_update_context();
    ((void (*)())core_rewrite_unit_identity_record)(&core_unit_identity_record);
    return 0;
}
uint32_t core_update_set_interrupt_mmio()
{
    int core_get_set_interrupt_mmio_result;
    int entry_index;
    core_get_set_interrupt_mmio_result = 0;
    for (entry_index = 0; (core_get_set_interrupt_mmio_result == 0 && (entry_index < 0x10)); entry_index = entry_index + 1) {
        core_get_set_interrupt_mmio_result = core_get_set_interrupt_mmio(entry_index << 0xc);
    }
    return 0;
}
void core_update_clear_memory_mcp3008_sample()
{
    core_mcp3008_sample = 0;
    ti_memset(&core_command_queue_context, 0, 0x198);
    return;
}
uint32_t core_update_device_for_device_for_component(context, offset)
uint32_t context;
int offset;
{
    int device_record;
    device_record = *(int *)(offset + 8);
    *(uint8_t *)(device_record + 0x28) = 1;
    ((void (*)())core_configure_command_profile_c8_or_25_selector)(context, device_record);
    *(uint8_t *)(device_record + 0x28) = 0;
    return 0;
}
uint32_t core_update_device_for_device_for_device(context, offset)
uint32_t context;
int offset;
{
    int device_record;
    device_record = *(int *)(offset + 8);
    *(uint8_t *)(device_record + 0x28) = 1;
    ((void (*)())core_configure_command_profile_ca_or_35_selector)(context, device_record);
    *(uint8_t *)(device_record + 0x28) = 0;
    return 0;
}
bool core_get_cartridge_eject_control()
{
    return 0x1089 < usb_core_revision;
}
void core_update_thermal_monitor_enabled_for_clear_memory_resource()
{
    core_thermal_monitor_enabled = 0;
    core_mww_transfer_value = 0;
    core_spi_transfer_context = 0;
    core_flash_update_context = 0;
    core_mww_transfer_context = 0;
    core_cartridge_input_value = 0;
    return;
}
void core_update_spi_transfer_context()
{
    if (core_spi_transfer_context != '\0') {
        core_spi_transfer_context = 0;
        core_mww_transfer_value = 0;
        mww_handle_transfer_mww0_addr_device_transfer_state();
        return;
    }
    return;
}
uint32_t core_get_notification_state()
{
    uint32_t operation_result;
    operation_result = 0;
    if ((core_notification_state != '\0') && (core_notification_suppression_flag == '\0')) {
        operation_result = 1;
    }
    return operation_result;
}
void core_update_reconnect_state()
{
    int operation_status;
    operation_status = core_forward_cartridge_input_value();
    if (operation_status == 0) {
        core_reconnect_state = 3;
    } else {
        core_reconnect_state = 2;
    }
    return;
}
uint32_t core_process_sci_read_logical_for_resource_usb_state()
{
    int operation_status;
    uint32_t sci_read_logical_input_result;
    operation_status = ((int16_t (*)())core_get_flash_validation_context_for_validate_structured_flash)(0);
    if (operation_status == 0x38) {
        sci_read_logical_input_result = sci_read_logical_input(0x10);
    } else {
        sci_read_logical_input_result = 1;
    }
    return sci_read_logical_input_result;
}
void core_update_ti_memset_for_lookup_arguments(context, offset) uint32_t context;
int offset;
{
    ti_memset(offset, 0, 0x84);
    *(uint32_t *)(offset + 0xc) = context;
    return;
}
uint32_t core_run_ready_state_handler(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;
{
    int operation_status;
    operation_status = ((int (*)())core_run_target_type_clear_memory_target_type_context)();
    if (operation_status == 0) {
        ((int (*)())core_handle_device_ti_memset)(context, length);
    }
    return 0;
}
uint32_t core_check_idle_input_and_state()
{
    int operation_status;
    uint32_t operation_result;
    operation_status = core_process_sci_read_logical_for_resource_usb_state();
    operation_result = 0;
    if ((operation_status == 0) && (operation_status = core_process_flash_validation_context(), operation_status != 0)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_process_device_status_value()
{
    int core_get_device_status_value_result;
    core_get_device_status_value_result = core_get_device_status_value(0);
    if (core_get_device_status_value_result != 0) {
        rti_delay_milliseconds(1);
        core_set_SYSECR_REG_OFF_to_hex_8000_handler();
    }
    return;
}
uint16_t core_process_ata_command_buffer_for_build_scsi_mode()
{
    int operation_status;
    uint16_t operation_result;
    operation_status = (int)core_get_ata_command_buffer();
    operation_result = *(uint16_t *)(operation_status + 0x10);
    operation_status = core_get_response_end_context_for_response_end_word();
    if (operation_status != 0) {
        operation_result = 0x1000;
    }
    return operation_result;
}
uint32_t core_process_command_phase_three()
{
    int operation_status;
    uint32_t operation_result;
    operation_status = core_is_command_phase_three_for_build_scsi_mode();
    operation_result = 1;
    if ((operation_status == 0) && (operation_status = core_is_cartridge_input_state_six(), operation_status == 0)) {
        operation_result = 0;
    }
    return operation_result;
}
void core_copy_words_to_interleaved_destination(destination, source, condition) uint8_t *destination;
uint8_t *source;
int condition;
{
    uint32_t condition_field;
    if (condition != 0) {
        condition_field = condition + 1U >> 1;
        do {
            destination = destination + 2;
            condition_field = condition_field - 1;
            *destination = *source;
            source = source + 1;
        } while (condition_field != 0);
    }
    return;
}
void core_submit_ata_command_context(context, offset) uint32_t context;
int offset;
{
    ((void (*)())core_process_response_word_clear_memory_response_word_context)(context, offset + 8, offset + 0x30, offset);
    return;
}
uint32_t core_update_component(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)
{
    uint32_t length_component;
    length_component = length;
    ((uint32_t (*)())core_get_end_remaining)(context, value, &length_component, flags, index, 1);
    return 0;
}
void core_swap_adjacent_bytes(context, condition) int context;
uint32_t condition;
{
    uint8_t *data_cursor;
    uint8_t working_result;
    uint8_t *context_derived;
    condition = condition >> 1;
    context_derived = (uint8_t *)(context + -2);
    do {
        data_cursor = context_derived + 2;
        working_result = *data_cursor;
        *data_cursor = context_derived[3];
        condition = condition - 1;
        context_derived[3] = working_result;
        context_derived = data_cursor;
    } while (condition != 0);
    return;
}
uint64_t core_decode_header_flags_and_offset(context)
int context;
{
    uint8_t header_flags;
    uint32_t working_result;
    header_flags = *(uint8_t *)(context + 1);
    if ((header_flags >> 2 & 1) == 0) {
        if ((header_flags & 1) == 0) {
            working_result = 4;
        } else {
            working_result = 0xc;
        }
    } else {
        working_result = 0x14;
    }
    return (uint64_t)CONCAT14(header_flags >> 3, working_result);
}
int core_get_structure_field_hex_for_structure_field_hex(context, value)
uint32_t context;
uint32_t value;
{
    uint32_t core_get_structure_field_hex_4c_value_result;
    uint32_t callback_result = 0;
    uint64_t core_get_structure_field_hex_50_value_result;
    core_get_structure_field_hex_4c_value_result = core_get_structure_field_hex_4c_value(context, value, context);
    core_get_structure_field_hex_50_value_result = core_get_structure_field_hex_50_value(callback_result, core_get_structure_field_hex_4c_value_result);
    return (int)core_get_structure_field_hex_50_value_result + (int)((uint64_t)core_get_structure_field_hex_50_value_result >> 0x20);
}
void core_update_saturating_add_u32_for_ata_standby_context(offset) int offset;
{
    uint32_t core_saturating_add_u32_result;
    core_saturating_add_u32_result = core_saturating_add_u32(*(uint32_t *)(offset + 0x2c), 1);
    *(uint32_t *)(offset + 0x2c) = core_saturating_add_u32_result;
    *(uint8_t *)(offset + 0x13) = 2;
    return;
}
int core_get_remaining_descriptor_space(context, value)
uint32_t context;
uint32_t value;
{
    uint32_t core_decode_header_flags_and_offset_derived;
    int operation_result;
    int callback_result = 0;
    core_decode_header_flags_and_offset_derived = ((uint64_t (*)())core_decode_header_flags_and_offset)(context, value, context);
    if (*(uint16_t *)(callback_result + 2) < core_decode_header_flags_and_offset_derived) {
        operation_result = 0;
    } else {
        operation_result = *(uint16_t *)(callback_result + 2) - core_decode_header_flags_and_offset_derived;
    }
    return operation_result;
}
uint32_t core_get_ahci_status_register(context)
int context;
{
    return (*(uint32_t *)(&ahci_port_sata_status_register + context * 0x80) & 0xff) >> 4;
}
bool core_is_cartridge_input_state_six()
{
    return core_cartridge_input_state == 6;
}
uint8_t core_get_constant_zero_value_for_pin_resource_state()
{
    return 0;
}
void core_update_response_update_context()
{
    rti_process_thermal_timer_state(&core_response_update_context, 300000);
    return;
}
void core_process_media_processing_context()
{
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(&core_media_processing_context);
    core_update_persistent_update_context();
    return;
}
void core_process_end_remaining_for_ti_memset_ti(context, value) uint32_t context;
uint32_t value;
{
    core_forward_end_remaining(context, value, 0x10);
    ((uint64_t (*)())core_process_clear_memory_clear_memory_copy_context)(context, 0x10);
    return;
}
void core_copy_word_buffer(destination, source, length) uint32_t *destination;
uint32_t *source;
int length;
{
    for (; length != 0; length = length + -1) {
        *destination = *source;
        source = source + 1;
        destination = destination + 1;
    }
    return;
}
uint8_t core_get_0xc_for_lookup_key_metadata(offset)
int offset;
{
    uint8_t offset_field_0xc;
    offset_field_0xc = 0xff;
    if ((*(int16_t *)(offset + 8) != 0) && (*(uint8_t **)(offset + 0xc) != (uint8_t *)0x0)) {
        offset_field_0xc = **(uint8_t **)(offset + 0xc) & 0x7f;
    }
    return offset_field_0xc;
}
void core_update_device_descriptor_context()
{
    int core_get_set_validation_flags_result;
    core_get_set_validation_flags_result = core_get_set_validation_flags();
    if (core_get_set_validation_flags_result != 0) {
        core_device_descriptor_context = 1;
    }
    return;
}
bool core_get_mww_transfer_context()
{
    core_mww_transfer_value = 1;
    return core_thermal_monitor_enabled == '\0';
}
void core_process_prepare_runtime_services()
{
    core_prepare_runtime_services_hook();
    core_forward_reconnect_state();
    core_handle_thermal_control_context();
    core_handle_pin_input_pin_state();
    return;
}
void core_set_command_phase_to_one()
{
    core_update_sense_refresh_context();
    core_command_phase = 1;
    return;
}
void core_update_command_phase_flags_for_firmware_context_byte(context) uint32_t context;
{
    core_command_phase_flags = core_command_phase_flags & ~context;
    return;
}
void core_set_command_phase_to_two()
{
    core_update_sense_refresh_context();
    core_command_phase = 2;
    return;
}
