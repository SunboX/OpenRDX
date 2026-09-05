
void core_update_clear_memory_thermal_control_context()
{
    ti_memset(&core_thermal_control_context, 0, 0x4c);
    core_interrupt_transfer_context = 0;
    core_interrupt_timer_elapsed_ticks = 0;
    return;
}
void core_update_firmware_context_usb_completion_state()
{
    core_set_CDDIS_REG_OFF_to_hex_38_handler();
    usb_event_callback_status = 1;
    if (core_usb_event_callback != (firmware_callback_t)0) {
        (*core_usb_event_callback)(1);
    }
    return;
}
void core_process_firmware_transfer_resource()
{
    if (core_firmware_transfer_context == 5) {
        ((uint32_t (*)())core_process_wdt_reset)(0);
        ((uint32_t (*)())core_process_wdt_reset)(0x80);
        mww_process_mww_transfer_storage(0);
    }
    return;
}
void core_update_ti_memset_for_initialize_output_record(offset, value, length) int offset;
int value;
int length;
{
    core_update_ti_memset_for_ti_memset_for();
    *(uint8_t *)(offset + 0x6f) = 1;
    *(int *)(offset + 100) = value + -1;
    *(uint32_t *)(offset + 0x68) = length - (uint32_t)(value == 0);
    return;
}
void core_process_ti_memset_for_initialize_transfer_workspace(offset, value, length, flags) int offset;
uint32_t value;
uint32_t length;
uint32_t flags;
{
    uint32_t status_bits;
    uint32_t flags_snapshot;
    status_bits = length;
    flags_snapshot = flags;
    ti_memset(&flags_snapshot, 0, 4);
    status_bits = *(uint32_t *)(offset + 0x14);
    ((void (*)())core_write_descriptor_fields)(value, &flags_snapshot, &status_bits);
    return;
}
uint32_t core_process_sci_read_logical_for_monitor_cartridge_input()
{
    int sci_read_logical_input_result;
    int operation_status;
    uint32_t operation_result;
    sci_read_logical_input_result = sci_read_logical_input(4);
    operation_status = ahci_is_link_active();
    operation_result = 1;
    if ((operation_status == 0) && (operation_status = core_update_sci_read_logical_for_sci_read_logical(), operation_status == 0 && sci_read_logical_input_result == 0)) {
        operation_result = 0;
    }
    return operation_result;
}
bool core_compare_record_payloads(destination_record, source_record) int destination_record;
int source_record;
{
    int core_compare_buffers_from_end_result;
    uint32_t destination_buffer;
    uint32_t source_buffer;
    source_buffer = *(uint32_t *)(source_record + 8);
    destination_buffer = *(uint32_t *)(destination_record + 8);
    ((uint64_t (*)())core_process_ti_memset_ti_memset)(destination_buffer, destination_record);
    core_compare_buffers_from_end_result = core_compare_buffers_from_end(destination_buffer, source_buffer, 0x20);
    return core_compare_buffers_from_end_result == 0;
}
void core_process_first_pin_initial_pin(context, value) uint8_t context;
uint32_t value;
{
    int spi_get_spi_transfer_storage_result;
    uint32_t *first_pin_state;
    int callback_result = 0;
    uint32_t *initial_pin_value = 0;
    uint8_t first_pin_index = 0;
    uint32_t first_pin_context = 0;
    spi_get_spi_transfer_storage_result = (int)spi_get_spi_transfer_storage(context, value, value);
    *(uint8_t *)(spi_get_spi_transfer_storage_result + 4) = *(uint8_t *)(callback_result + 4);
    first_pin_state = (uint32_t *)spi_get_spi_transfer_storage(first_pin_index);
    *first_pin_state = *initial_pin_value;
    spi_process_pin_mask_spi_pc3(first_pin_context);
    return;
}
uint32_t core_build_status_control_byte(context, buffer, length, flags)
uint32_t context;
uint32_t *buffer;
uint32_t length;
uint32_t flags;
{
    uint32_t status_bits;
    status_bits = CONCAT31((int32_t)((uint32_t)flags >> 8), 1);
    ((uint16_t (*)())core_update_constant_0xc)(context, &status_bits);
    if ((char)status_bits == '\0') {
        *buffer = 0x28;
    }
    return status_bits & 0xff;
}
uint16_t *core_process_ti_memset_for_build_scsi_mode(offset, value)
int offset;
uint32_t value;
{
    uint16_t *data_cursor;
    data_cursor = (uint16_t *)core_get_mww_mode_page_buffer();
    ti_memset(*(uint32_t *)(data_cursor + 2), 0, value);
    *(uint16_t **)(offset + 0xc) = data_cursor;
    *data_cursor = (int16_t)value;
    *(int16_t *)(*(int *)(offset + 0xc) + 2) = (int16_t)value;
    return data_cursor;
}
void core_transfer_packed(offset, value) int offset;
uint32_t value;
{
    uint8_t core_convert_nibble_to_hex_character_result;
    int operation_status;
    int callback_result;
    uint32_t packed_value;
    uint8_t *computed_offset;
    operation_status = 8;
    computed_offset = (uint8_t *)(offset + 8);
    *computed_offset = 0;
    do {
        core_convert_nibble_to_hex_character_result = core_convert_nibble_to_hex_character(value & 0xf, operation_status);
        value = packed_value >> 4;
        operation_status = callback_result + -1;
        computed_offset = computed_offset + -1;
        *computed_offset = core_convert_nibble_to_hex_character_result;
    } while (operation_status != 0);
    return;
}
void core_process_halt_forever_for_build_scsi_mode(buffer, value, condition, flags) uint16_t *buffer;
uint32_t value;
int condition;
uint32_t flags;
{
    if ((int)(uint32_t)*buffer <= condition) {
        core_halt_forever();
    }
    core_transfer_range_end_range_start_for_short_available(*(uint32_t *)(buffer + 2), value, condition, flags);
    return;
}
void core_configure_word_descriptor(configuration, source_descriptor, length, flags) uint32_t *configuration;
uint32_t *source_descriptor;
uint32_t length;
uint32_t flags;
{
    uint32_t second_source_word;
    configuration[1] = length;
    configuration[2] = flags;
    *configuration = 0;
    configuration[4] = (uint32_t)source_descriptor + 0xc;
    configuration[3] = (uint32_t)source_descriptor + 2;
    second_source_word = source_descriptor[1];
    configuration[5] = *source_descriptor;
    configuration[6] = second_source_word;
    return;
}
uint32_t core_dispatch_available_record(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)
{
    int operation_status;
    operation_status = core_get_expected_candidate();
    if (operation_status == 0) {
        ((void (*)())core_process_expected_flag_expected)(value, length, flags, index, context);
    }
    return 0;
}
uint32_t core_validate_combined_record_bounds(offset)
int offset;
{
    uint32_t operation_result;
    operation_result = 0;
    if (((*(uint32_t *)(offset + 4) < 0x1fc) && (*(uint32_t *)(offset + 8) < 0x1fc)) &&
        (*(uint32_t *)(offset + 4) + *(uint32_t *)(offset + 8) < 0x1fc)) {
        operation_result = 1;
    }
    return operation_result;
}
void core_update_flash_validation_status(context) uint32_t context;
{
    if (core_flash_validation_status != 8) {
        ((void (*)())core_apply_pwm_percentage)(0xf, context, 16000);
        core_pwm_duty_target = context;
    }
    return;
}
bool core_get_persistent_identity_record_for_initialize_rdx_identity()
{
    int core_persistent_identity_record_pointer;
    core_persistent_identity_record_pointer = ((uint32_t (*)())core_calculate_record_checksum)(&core_persistent_identity_record, 0x40);
    return core_persistent_identity_record != core_persistent_identity_record_pointer;
}
void core_process_ti_memset_0x20(offset) int offset;
{
    char offset_field_0x20;
    offset_field_0x20 = *(char *)(offset + 0x20);
    ti_memset(offset, 0, 0x198);
    core_reset_command_context_hook();
    if (offset_field_0x20 != '\0') {
        *(uint8_t *)(offset + 0x20) = 1;
    }
    return;
}
void core_process_clear_memory_spi_request_context()
{
    ti_memset(&core_spi_request_context, 0, 8);
    core_spi_response_context = 0;
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(0x800f291);
    return;
}
void core_process_clear_memory_transfer_buffer_context()
{
    core_update_thermal_monitor_enabled_for_clear_memory_resource();
    ti_memset(&core_transfer_buffer_context, 0, 2);
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(&core_transfer_buffer_context);
    return;
}
void core_reconnect_device_and_start_timer()
{
    usb_hal_connect();
    rti_process_thermal_timer_state(&core_transfer_buffer_context, 2000);
    core_pin_update_context = 0;
    return;
}
void core_update_spi_transfer_context_for_program_spi_flash(condition) int condition;
{
    if ((core_spi_transfer_context == '\0') || (condition != 0)) {
        vim_request_mask_set_zero_register = 0x200000;
    }
    return;
}
uint32_t core_find_value_in_fixed_table(condition)
int condition;
{
    int *data_cursor;
    int operation_status;
    operation_status = 3;
    data_cursor = (int *)0x800ea18;
    do {
        data_cursor = data_cursor + 1;
        if (condition == *data_cursor) {
            return 1;
        }
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    return 0;
}
void core_update_mww_saved_request()
{
    core_mww_saved_request = 0;
    core_mww_saved_request_reset_companion_word = 0;
    mww_transfer_phase = 0;
    mww_deferred_command_pending = 0;
    mww_saved_command_ready = 0;
    mww_command_class = 0;
    mww_process_transfer_descriptor_mww0_addr_mww_transfer_descriptor(3, mww_source_descriptor, 0x1f);
    return;
}
void core_process_mww_process_command(context, value, length, flags) uint32_t context;
uint32_t value;
uint32_t length;
int flags;
{
    int operation_status;
    int flags_snapshot;
    flags_snapshot = flags;
    operation_status = mww_process_command_word_channel_mww_command_channel(context, 5, 0, 0);
    if (operation_status == 0) {
        mww_update_mww_transfer_storage(context, &flags_snapshot);
        *(uint32_t *)(flags_snapshot + 0x10) = 0;
    }
    return;
}
uint32_t core_process_mww_update_resource(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
int flags;
{
    int flags_snapshot;
    flags_snapshot = flags;
    mww_update_mww_transfer_storage(context, &flags_snapshot);
    mww_process_command_word_channel_mww_command_channel(context, *(int *)(flags_snapshot + 0x1c) << 0x10 | 7, 0, 0);
    return 0;
}
void core_dispatch_usb_event_callbacks(context) uint32_t context;
{
    uint32_t *core_usb_event_callback_cursor_origin_pointer;
    int operation_status;
    operation_status = 4;
    core_usb_event_callback_cursor_origin_pointer = (uint32_t *)&core_usb_event_callback_cursor_origin;
    do {
        core_usb_event_callback_cursor_origin_pointer = core_usb_event_callback_cursor_origin_pointer + 1;
        if ((firmware_callback_t *)*core_usb_event_callback_cursor_origin_pointer == (firmware_callback_t *)0x0) {
            return;
        }
        (*(firmware_callback_t *)*core_usb_event_callback_cursor_origin_pointer)(context);
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    return;
}
void core_update_ti_memset_for_ti_memset_for(offset) int offset;
{
    ti_memset(offset + 0x10, 0, 0x34);
    ti_memset(offset + 0x44, 0, 0x38);
    return;
}
void core_copy_record_header_to_destination(buffer, offset) int *buffer;
int offset;
{
    uint32_t core_decode_header_flags_and_offset_result;
    core_decode_header_flags_and_offset_result = core_decode_header_flags_and_offset(offset);
    core_forward_end_remaining(*(int *)(offset + 0x14) + *buffer, offset, core_decode_header_flags_and_offset_result);
    return;
}
void core_submit_fixed_ata_command(context, value, length, flags) uint32_t context;
uint32_t value;
uint32_t length;
uint32_t flags;
{
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    uint32_t status_bits;
    value_derived = value;
    length_component = length;
    status_bits = flags;
    core_update_ti_memset_for_ti_memset(&value_derived);
    status_bits = (status_bits & 0xff000000u) | (CONCAT12(0xec, (uint16_t)status_bits) & 0x00ffffffu);
    ahci_execute_taskfile_command(&value_derived, context, 1, 0);
    return;
}
void core_rewrite_unit_identity_record(context) uint32_t context;
{
    int operation_status;
    operation_status = core_get_set_interrupt_mmio(0x3e000);
    if (operation_status == 0) {
        core_program_spi_flash_pages(0x3e000, 0x100, context);
    }
    return;
}
void core_process_lookup_arguments()
{
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint32_t callback_result = 0;
    uint64_t lookup_arguments;
    lookup_arguments = core_get_runtime_context_base_for_build_scsi_mode();
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode((int)lookup_arguments, (int)((uint64_t)lookup_arguments >> 0x20), (int)lookup_arguments);
    core_update_ti_memset_for_lookup_arguments(callback_result, core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    return;
}
void core_initialize_word_buffer(buffer, value, condition) uint32_t *buffer;
uint32_t value;
uint32_t condition;
{
    int condition_field;
    uint32_t *buffer_cursor;
    if (1 < condition) {
        condition_field = condition - 1;
        buffer_cursor = buffer;
        do {
            buffer_cursor = buffer_cursor + 1;
            condition_field = condition_field + -1;
            *buffer_cursor = 0;
        } while (condition_field != 0);
    }
    *buffer = value;
    return;
}
uint32_t core_update_flash_validation_context()
{
    int core_get_flash_validation_context_for_validate_structured_flash_result;
    uint32_t operation_result;
    core_get_flash_validation_context_for_validate_structured_flash_result = core_get_flash_validation_context_for_validate_structured_flash();
    if (core_get_flash_validation_context_for_validate_structured_flash_result == 0x35) {
        return 2;
    }
    if (core_get_flash_validation_context_for_validate_structured_flash_result == 0x36) {
        operation_result = 1;
    } else {
        if (core_get_flash_validation_context_for_validate_structured_flash_result == 0x37) {
            return 2;
        }
        operation_result = 0;
    }
    return operation_result;
}
uint32_t core_process_halt_forever_for_selected_or_bit(buffer, value, condition)
uint8_t *buffer;
uint32_t value;
int condition;
{
    if ((int)(uint32_t)*buffer <= condition) {
        core_halt_forever();
    }
    core_read_reversed_field_value(*(uint32_t *)(buffer + 4), value, condition);
    return 0;
}
void core_update_release_record_payload(condition, offset, length, flags) int condition;
int offset;
uint32_t length;
uint32_t flags;
{
    uint32_t core_release_record_payload_when_short_result;
    core_release_record_payload_when_short_result = core_release_record_payload_when_short(offset + 4, length, flags);
    if (core_release_record_payload_when_short_result < *(uint16_t *)(condition + 2)) {
        *(int16_t *)(condition + 2) = (int16_t)core_release_record_payload_when_short_result;
    }
    return;
}
int core_get_remaining_bounded_capacity(condition, offset)
uint32_t condition;
int offset;
{
    int condition_field;
    uint32_t maximum_count;
    maximum_count = *(int *)(offset + 4) - 1;
    if (0x1f7 < maximum_count) {
        maximum_count = 0x1f8;
    }
    if (condition < maximum_count) {
        condition_field = maximum_count - condition;
    } else {
        condition_field = 0;
    }
    return condition_field;
}
void core_update_constant_0x54(offset, value) int offset;
uint32_t value;
{
    uint32_t offset_field_0x54;
    offset_field_0x54 = *(uint32_t *)(offset + 0x54);
    *(uint32_t *)(offset + 0x54) = value + offset_field_0x54;
    *(uint32_t *)(offset + 0x58) = *(int *)(offset + 0x58) + (uint32_t)CARRY4(value, offset_field_0x54);
    return;
}
void core_update_constant_0x5c(offset, value) int offset;
uint32_t value;
{
    uint32_t offset_field_0x5c;
    offset_field_0x5c = *(uint32_t *)(offset + 0x5c);
    *(uint32_t *)(offset + 0x5c) = value + offset_field_0x5c;
    *(uint32_t *)(offset + 0x60) = *(int *)(offset + 0x60) + (uint32_t)CARRY4(value, offset_field_0x5c);
    return;
}
