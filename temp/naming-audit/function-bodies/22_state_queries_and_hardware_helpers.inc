
uint32_t core_get_x02(condition)
int condition;
{
    uint32_t operation_result;
    if (*(char *)(condition + 6) == '\x02') {
        operation_result = 100;
    } else {
        operation_result = 500;
    }
    return operation_result;
}
void core_update_masked_register_bits(buffer, value, length) uint32_t *buffer;
uint32_t value;
uint32_t length;
{
    *buffer = length & value | *buffer & ~value;
    return;
}
void core_update_device_request_update(context) uint32_t context;
{
    core_device_request_update = context;
    return;
}
uint32_t core_repack_shifted_value(context)
uint32_t context;
{
    return (int)context >> 8 & 0xffffU | (context & 0xff) << 8;
}
uint32_t core_get_sense_selection_flags_value()
{
    return core_sense_selection_flags;
}
uint8_t *core_get_ata_command_buffer()
{
    return (uint8_t *)&core_ata_command_buffer;
}
uint32_t *core_get_cartridge_monitor_context()
{
    return (uint32_t *)&core_cartridge_monitor_context;
}
uint8_t core_get_gpio_pin_context_value()
{
    return core_gpio_pin_context;
}
void core_update_ti_memset_for_ata_command_command(buffer) uint32_t *buffer;
{
    ti_memset(*buffer, 0, 0x1000);
    return;
}
uint8_t core_get_resource_ahci_state()
{
    uint8_t core_get_ahci_status_register_result;
    core_get_ahci_status_register_result = core_get_ahci_status_register(0);
    return core_get_ahci_status_register_result;
}
void core_set_command_runtime_state_to_zero()
{
    core_command_runtime_state = 0;
    return;
}
void core_update_secondary_update_context(context) uint32_t context;
{
    ((void (*)())core_transfer_x01)(context, &core_secondary_update_context, 4);
    return;
}
uint8_t *core_get_response_byte_context()
{
    return (uint8_t *)&core_unit_serial_number;
}
uint8_t core_get_mcp3008_sample_value()
{
    return core_mcp3008_sample;
}
char core_convert_nibble_to_hex_character(condition)
int condition;
{
    char condition_field;
    if (condition < 10) {
        condition_field = (char)condition + '0';
    } else {
        condition_field = (char)condition + '7';
    }
    return condition_field;
}
void core_update_mark_primary_sense()
{
    core_mark_primary_sense_record_not_ready(0x800f2b4);
    return;
}
void core_update_store_context_enable(context) uint32_t context;
{
    core_store_context_enable_byte(0x800f2b4, context);
    return;
}
void core_update_drive_mechanism_outputs(condition) int condition;
{
    uint32_t working_result;
    if (condition == 0) {
        working_result = 1;
    } else {
        working_result = 6;
    }
    core_drive_mechanism_outputs(working_result);
    return;
}
void core_process_validation_flags()
{
    core_get_set_validation_flags();
    core_get_validation_flags();
    return;
}
uint32_t core_get_set_validation_flags()
{
    core_set_validation_flags_flag_bit_five(0);
    return 1;
}
uint8_t core_get_mww_transfer_value()
{
    return core_mww_transfer_value;
}
void core_set_thermal_monitor_enabled_to_zero()
{
    core_thermal_monitor_enabled = 0;
    return;
}
uint8_t core_get_cartridge_input_value()
{
    return core_cartridge_input_value;
}
void core_set_thermal_monitor_enabled_to_one()
{
    core_thermal_monitor_enabled = 1;
    return;
}
uint32_t core_get_notification_context_value()
{
    return core_notification_context;
}
uint8_t core_get_device_status_value()
{
    return core_device_status_value;
}
uint8_t core_get_command_context_value()
{
    return core_command_context_value;
}
uint8_t core_get_command_feature_state_value()
{
    return core_command_feature_state;
}
uint8_t core_get_response_type_value()
{
    return core_response_type;
}
uint8_t core_get_command_buffer_state_value()
{
    return core_command_buffer_state;
}
void core_process_notification_context()
{
    core_update_notification_context();
    core_update_device_descriptor_context();
    return;
}
void core_store_management_dispatch_result(buffer) uint32_t *buffer;
{
    uint32_t core_handle_type_type_derived;
    core_handle_type_type_derived = ((int (*)())core_handle_type_type)();
    *buffer = core_handle_type_type_derived;
    return;
}
void core_update_secondary_sense_field_context(context) uint16_t context;
{
    core_secondary_sense_field_context = context;
    return;
}
void core_update_response_buffer_context(context) uint8_t context;
{
    core_response_buffer_context = context;
    return;
}
void core_update_primary_sense_field_context(context) uint8_t context;
{
    core_primary_sense_field_context = context;
    return;
}
void core_update_command_phase_auxiliary(context) uint32_t context;
{
    core_command_phase_auxiliary = context;
    return;
}
void core_update_command_feature_state(context) uint8_t context;
{
    core_command_feature_state = context;
    return;
}
void core_update_device_status_value(context) uint8_t context;
{
    core_device_status_value = context;
    return;
}
void core_update_command_buffer_state(context) uint8_t context;
{
    core_command_buffer_state = context;
    return;
}
void core_set_SYSECR_REG_OFF_to_hex_8000_handler()
{
    SYSECR_REG_OFF = 0x8000;
    return;
}
void core_update_mww_process_command(context, value) uint32_t context;
uint16_t value;
{
    mww_process_command_word_channel_mww_command_channel(context, 2, value, 0);
    return;
}
uint8_t core_get_response_status_value()
{
    return core_response_status_value;
}
void core_update_firmware_context_primary_firmware_callback(context) uint32_t context;
{
    core_primary_firmware_callback = (firmware_callback_t)context;
    return;
}
void core_update_firmware_context_secondary_firmware_callback(context) uint32_t context;
{
    core_secondary_firmware_callback = (firmware_callback_t)context;
    return;
}
void core_clear_primary_descriptor_mask(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x74) = *(uint32_t *)(offset + 0x74) & ~value;
    return;
}
void core_clear_secondary_descriptor_mask(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x78) = *(uint32_t *)(offset + 0x78) & ~value;
    return;
}
uint8_t core_get_response_end_context_for_response_end_word()
{
    int core_get_response_end_context_pointer_result;
    core_get_response_end_context_pointer_result = (int)core_get_response_end_context_pointer();
    return *(uint8_t *)(core_get_response_end_context_pointer_result + 5);
}
void core_run_device_address_transfer()
{
    ((uint32_t (*)())core_transfer_find_last_nonzero)();
    return;
}
uint8_t core_get_persistent_identity_record_for_notification_context()
{
    int core_get_persistent_identity_record_pointer_result;
    core_get_persistent_identity_record_pointer_result = (int)core_get_persistent_identity_record_pointer();
    return *(uint8_t *)(core_get_persistent_identity_record_pointer_result + 0xd);
}
void core_store_duplicate_record_value(offset, value) int offset;
uint16_t value;
{
    **(uint16_t **)(offset + 0xc) = value;
    *(uint16_t *)(*(int *)(offset + 0xc) + 2) = value;
    return;
}
void core_set_CDDIS_REG_OFF_to_hex_38_handler()
{
    CDDIS_REG_OFF = 0x38;
    return;
}
void core_set_primary_descriptor_mask(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x74) = value | *(uint32_t *)(offset + 0x74);
    return;
}
void core_set_secondary_descriptor_mask(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x78) = value | *(uint32_t *)(offset + 0x78);
    return;
}
uint32_t core_write_four_byte_value(context, value)
uint32_t context;
uint32_t value;
{
    ((int (*)())core_transfer_remaining_bounds)(context, 4, value);
    return 0;
}
void core_set_secondary_descriptor_value_ready(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x24) = value;
    *(uint8_t *)(offset + 0x12) = 2;
    return;
}
void core_set_primary_descriptor_value_ready(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x20) = value;
    *(uint8_t *)(offset + 0x11) = 2;
    return;
}
void core_update_spi_update_spi()
{
    spi_wait_for_flag(0x200);
    return;
}
