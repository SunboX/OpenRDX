
void core_update_notification_context()
{
    core_notification_context = core_get_persistent_identity_record_for_notification_context();
    return;
}
bool core_is_command_phase_three_for_build_scsi_mode()
{
    return core_command_phase == 3;
}
bool core_get_thermal_control_context(buffer) uint8_t *buffer;
{
    return *(int *)(&core_thermal_control_context + (uint32_t)*buffer * 4) == 0;
}
void core_update_command_status_update(context) uint32_t context;
{
    core_update_masked_register_bits(&usb_device_configuration_register, 0x3f8, (context & 0x7f) << 3);
    return;
}
uint32_t core_has_active_command_flags(buffer)
uint32_t *buffer;
{
    uint32_t operation_result;
    operation_result = 0;
    if (((*buffer >> 2 & 1) != 0) && ((*buffer & 3) != 0)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_update_saturating_add_u32_for_enable_request_flag(offset) int offset;
{
    uint32_t core_saturating_add_u32_result;
    core_saturating_add_u32_result = core_saturating_add_u32(*(uint32_t *)(offset + 0x3c));
    *(uint32_t *)(offset + 0x3c) = core_saturating_add_u32_result;
    *(uint8_t *)(offset + 0x17) = 2;
    return;
}
uint32_t core_update_f_x1c(buffer)
char *buffer;
{
    char buffer_cursor;
    uint32_t operation_result;
    buffer_cursor = *buffer;
    operation_result = 1;
    if ((buffer_cursor != '\f') && (buffer_cursor != '\x1c' && buffer_cursor != '\x1d')) {
        operation_result = 0;
    }
    return operation_result;
}
void core_process_set_notification_state()
{
    core_set_notification_state_to_zero_for_notify_change_resource();
    core_update_mww_transfer_context_for_mww_process_resource();
    core_update_command_feature_state(0);
    return;
}
void core_update_ti_memset_for_ti_memset(offset) int offset;
{
    ti_memset(offset, 0, 0xb);
    *(uint8_t *)(offset + 9) = 0x40;
    return;
}
int core_transfer_constant_0x13(offset)
int offset;
{
    uint32_t offset_field_0x13;
    int operation_result;
    operation_result = 1;
    for (offset_field_0x13 = (uint32_t)*(uint8_t *)(offset + 0x13); offset_field_0x13 != 0; offset_field_0x13 = offset_field_0x13 - 1) {
        operation_result = operation_result << 1;
    }
    return operation_result;
}
void core_update_response_word_ti(context, buffer) uint32_t context;
uint32_t *buffer;
{
    core_process_response_word_clear_memory_response_word_context(context, *buffer, buffer[1], buffer + 2);
    return;
}
void core_zero_word_buffer(buffer, value) uint32_t *buffer;
int value;
{
    for (; value != 0; value = value + -1) {
        *buffer = 0;
        buffer = buffer + 1;
    }
    return;
}
uint8_t core_get_0xc_for_device_descriptor_response(offset)
int offset;
{
    uint8_t offset_field_0xc;
    offset_field_0xc = 0;
    if ((1 < *(uint16_t *)(offset + 8)) && (*(int *)(offset + 0xc) != 0)) {
        offset_field_0xc = *(uint8_t *)(*(int *)(offset + 0xc) + 1);
    }
    return offset_field_0xc;
}
uint32_t core_release_record_payload_when_short(buffer, value, condition)
uint8_t *buffer;
uint32_t value;
int condition;
{
    if (condition < (int)(uint32_t)*buffer) {
        ((uint32_t (*)())core_transfer_range_end_range_start_for_descriptor_descriptor_resource)(*(uint32_t *)(buffer + 4));
        return 0;
    }
    return 0;
}
uint32_t core_submit_zero_based_transfer(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;
{
    ((uint32_t (*)())core_run_expected_flag_expected_type)(context, 0, value, length);
    return 0;
}
uint32_t core_submit_default_transfer_value(context)
uint32_t context;
{
    uint32_t status_bits;
    status_bits = 0;
    core_write_four_byte_value(&status_bits, context);
    return 0;
}
void core_update_ahci_device_context(context, value, length) uint32_t context;
uint32_t value;
uint32_t length;
{
    ahci_command_completion_context = context;
    core_ahci_dma_setup_callback_context = value;
    ahci_error_callback = length;
    return;
}
void core_update_clear_memory_response_end_context()
{
    ti_memset(&core_response_end_context, 0, 6);
    return;
}
void core_reset_mechanism_outputs_when_idle()
{
    int operation_status;
    operation_status = core_forward_sci_read_logical();
    if (operation_status == 0) {
        core_update_drive_mechanism_outputs(0);
    }
    return;
}
bool core_is_not_cartridge_input_state_zero()
{
    return core_cartridge_input_state != 0;
}
void core_update_mechanism_update_context_for_advance_mechanism_state(condition) int condition;
{
    if (condition != 0) {
        core_mechanism_update_context = 1;
    }
    return;
}
void core_set_mechanism_phase_to_zero()
{
    core_mechanism_phase = 0;
    return;
}
void core_set_mechanism_phase_to_two()
{
    core_mechanism_phase = 2;
    return;
}
void core_set_mechanism_phase_to_one()
{
    core_mechanism_phase = 1;
    return;
}
uint32_t core_update_identity_update_context()
{
    ((uint32_t (*)())core_calculate_record_checksum)(&core_unit_identity_record, 0x100);
    return 0;
}
void core_update_identity_persistence_context()
{
    core_identity_persistence_context = core_identity_persistence_context + 1;
    core_process_persistent_identity_record_for_identity_persistence_context();
    return;
}
uint32_t core_read_unit_identity_record(context)
uint32_t context;
{
    core_transfer_address_stack_transfer_address(3, 0x3e000, context, 0x100);
    return 0;
}
void core_set_transfer_ready_flag_to_one()
{
    core_transfer_ready_flag = 1;
    return;
}
uint32_t core_is_command_code_in_standard_range(condition)
uint32_t condition;
{
    uint32_t operation_result;
    operation_result = 0;
    if ((0x35 < condition) && (condition < 0xb6)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_update_enabled(state_record, enabled) int state_record;
int enabled;
{
    if ((enabled != 0) && (*(char *)(state_record + 7) == '\0')) {
        *(uint8_t *)(state_record + 7) = 1;
    }
    *(char *)(state_record + 5) = (char)enabled;
    return;
}
void core_mark_record_active_with_code_eight(offset) int offset;
{
    if (*(char *)(offset + 7) == '\0') {
        *(uint8_t *)(offset + 7) = 1;
    }
    *(uint8_t *)(offset + 9) = 8;
    return;
}
void core_signal_channel_completion(context) int context;
{
    *(uint32_t *)(context * 0x400 + -0x877f4) = 1;
    return;
}
bool core_get_flash_validation_context_for_flash_validation_status()
{
    int core_get_flash_validation_context_for_validate_structured_flash_result;
    core_get_flash_validation_context_for_validate_structured_flash_result = core_get_flash_validation_context_for_validate_structured_flash();
    return core_get_flash_validation_context_for_validate_structured_flash_result != 0x36;
}
uint64_t core_pack_context_value_pair(context, value)
uint32_t context;
uint32_t value;
{
    uint32_t core_get_ti_memset_derived;
    core_get_ti_memset_derived = ((uint32_t (*)())core_get_ti_memset)();
    return CONCAT44(value, core_get_ti_memset_derived);
}
void core_set_transfer_interrupt_enable_mask()
{
    vim_request_mask_clear_zero_register = 0x200000;
    return;
}
void core_update_spi_transfer_context_for_mww_handle_transfer()
{
    core_spi_transfer_context = 1;
    core_set_transfer_interrupt_enable_mask();
    return;
}
void core_set_cartridge_input_value_to_one()
{
    core_cartridge_input_value = 1;
    return;
}
void core_update_mww_transfer_context_for_mww_handle_transfer()
{
    core_mww_transfer_context = 1;
    core_set_thermal_monitor_enabled_to_one();
    return;
}
void core_set_notification_state_to_one()
{
    core_notification_state = 1;
    return;
}
void core_set_notification_state_to_zero_for_notify_change_resource()
{
    core_notification_state = 0;
    return;
}
uint32_t core_get_validation_flags()
{
    return (core_validation_flags & 0x3f) >> 5;
}
bool core_is_command_phase_zero()
{
    return core_command_phase == 0;
}
uint32_t core_get_command_phase_flags(context)
uint32_t context;
{
    return context & core_command_phase_flags & 0xff;
}
void core_update_clear_memory_response_buffer_context()
{
    ti_memset(&core_response_buffer_context, 0, 4);
    return;
}
void core_update_command_phase_flags_for_command_phase(context) uint32_t context;
{
    core_command_phase_flags = context | core_command_phase_flags;
    return;
}
void core_process_persistent_identity_record_for_notify_change_resource(context) uint8_t context;
{
    int core_get_persistent_identity_record_pointer_result;
    core_get_persistent_identity_record_pointer_result = (int)core_get_persistent_identity_record_pointer();
    *(uint8_t *)(core_get_persistent_identity_record_pointer_result + 0xd) = context;
    core_process_persistent_identity_record_for_identity_persistence_context();
    return;
}
void core_increment_usb_event_counter()
{
    core_usb_event_counter = core_usb_event_counter + 1;
    return;
}
void core_update_clear_memory_usb_event_counter()
{
    ti_memset(&core_usb_event_counter, 0, 0x14);
    return;
}
void core_update_usb_reset_state(context) int context;
{
    core_update_masked_register_bits(&usb_device_control_register, 0x1e, context << 1);
    return;
}
uint32_t core_get_available_primary_mask(context)
uint32_t context;
{
    uint64_t core_read_descriptor_field_24_value_result;
    core_read_descriptor_field_24_value_result = core_read_descriptor_field_24_value(context, context);
    return (uint32_t)core_read_descriptor_field_24_value_result & ~*(uint32_t *)((int)((uint64_t)core_read_descriptor_field_24_value_result >> 0x20) + 0x74);
}
uint32_t core_get_available_secondary_mask(context)
uint32_t context;
{
    uint64_t core_read_descriptor_field_20_value_result;
    core_read_descriptor_field_20_value_result = core_read_descriptor_field_20_value(context, context);
    return (uint32_t)core_read_descriptor_field_20_value_result & ~*(uint32_t *)((int)((uint64_t)core_read_descriptor_field_20_value_result >> 0x20) + 0x78);
}
uint32_t core_saturating_add_u32(condition, value)
uint32_t condition;
int value;
{
    uint32_t value_derived;
    value_derived = value + condition;
    if (value_derived < condition) {
        value_derived = 0xffffffff;
    }
    return value_derived;
}
uint32_t core_read_primary_persistent_state(context)
uint32_t context;
{
    core_transfer_address_stack_transfer_address(3, 0x3f000, context, 0x40);
    return 0;
}
uint32_t core_read_fallback_persistent_state(context)
uint32_t context;
{
    core_transfer_address_stack_transfer_address(3, 0x3fc00, context, 0x40);
    return 0;
}
void core_process_device_status_update()
{
    ((void (*)())core_process_ti_memset_0x20)();
    core_update_device_status_value(1);
    return;
}
