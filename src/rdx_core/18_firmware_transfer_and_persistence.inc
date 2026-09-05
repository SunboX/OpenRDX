/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Initialize software timers.
 *
 * @return No value.
 */
void core_initialize_software_timers()

{
    /* Initialize software timers; persistent state is carried in core software timer remaining ticks, core software timer count and related record fields. */
    ti_memset(&core_software_timer_remaining_ticks, 0, 0x4c);
    core_software_timer_count = 0;
    core_interrupt_timer_elapsed_ticks = 0;
    return;
}

/**
 * @brief Notify USB resume.
 *
 * @return No value.
 */
void core_notify_usb_resume()

{
    /* Notify USB resume; persistent state is carried in usb power management state, core USB event callback. */
    core_disable_bus_dynamic_clock_gating();
    usb_power_management_state = 1;
    if (core_usb_event_callback != (firmware_callback_t)0) {
        /* Notify core USB event callback only after the associated state fields have been committed. */
        (*core_usb_event_callback)(1);
    }
    return;
}

/**
 * @brief Abort failed USB control transfer.
 *
 * @return No value.
 */
void core_abort_failed_usb_control_transfer()

{
    /* Abort failed USB control transfer; persistent state is carried in usb ep0 state. */
    if (usb_ep0_state == 5) {
        ((uint32_t (*)())core_end_usb_endpoint_transfer)(0);
        ((uint32_t (*)())core_end_usb_endpoint_transfer)(0x80);
        mww_process_mww_transfer_storage(0);
    }
    return;
}

/**
 * @brief Update ti memset for initialize output record.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_update_ti_memset_for_initialize_output_record(offset, value, length) int offset;
int value;
int length;

{
    /* Update ti memset for initialize output record using only caller-provided data. */
    core_clear_media_record_state();
    *(uint8_t *)(offset + 0x6f) = 1;
    *(int *)(offset + 100) = value + -1;
    *(uint32_t *)(offset + 0x68) = length - (uint32_t)(value == 0);
    return;
}

/**
 * @brief Process ti memset for initialize transfer workspace.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_process_ti_memset_for_initialize_transfer_workspace(offset, value, length, flags) int offset;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Process ti memset for initialize transfer workspace using only caller-provided data. */
    uint32_t status_bits;
    uint32_t flags_snapshot;

    status_bits = length;
    flags_snapshot = flags;
    ti_memset(&flags_snapshot, 0, 4);
    status_bits = *(uint32_t *)(offset + 0x14);
    /* Call core write descriptor fields through the defined unprototyped function-pointer signature. */
    ((void (*)())core_write_descriptor_fields)(value, &flags_snapshot, &status_bits);
    return;
}

/**
 * @brief Process SCI read logical for monitor cartridge input.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_sci_read_logical_for_monitor_cartridge_input()

{
    /* Process SCI read logical for monitor cartridge input using only caller-provided data. */
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

/**
 * @brief Compare record payloads.
 *
 * @param destination_record Procedure input.
 * @param source_record Procedure input.
 * @return Result produced by the procedure.
 */
bool core_compare_record_payloads(destination_record, source_record) int destination_record;
int source_record;

{
    /* Compare record payloads using only caller-provided data. */
    int core_compare_buffers_from_end_result;
    uint32_t destination_buffer;
    uint32_t source_buffer;

    source_buffer = *(uint32_t *)(source_record + 8);
    destination_buffer = *(uint32_t *)(destination_record + 8);
    ((uint64_t (*)())core_process_ti_memset_ti_memset)(destination_buffer, destination_record);
    core_compare_buffers_from_end_result = core_compare_buffers_from_end(destination_buffer, source_buffer, 0x20);
    return core_compare_buffers_from_end_result == 0;
}

/**
 * @brief Process first pin initial pin.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_process_first_pin_initial_pin(context, value) uint8_t context;
uint32_t value;

{
    /* Process first pin initial pin using only caller-provided data. */
    int spi_get_logical_pin_configuration_result;
    uint32_t *first_pin_state;
    int callback_result = 0;
    uint32_t *initial_pin_value = 0;
    uint8_t first_pin_index = 0;
    uint32_t first_pin_context = 0;

    spi_get_logical_pin_configuration_result = (int)spi_get_logical_pin_configuration(context, value, value);
    *(uint8_t *)(spi_get_logical_pin_configuration_result + 4) = *(uint8_t *)(callback_result + 4);
    first_pin_state = (uint32_t *)spi_get_logical_pin_configuration(first_pin_index);
    *first_pin_state = *initial_pin_value;
    spi_configure_logical_pin_direction(first_pin_context);
    return;
}

/**
 * @brief Build status control byte.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_status_control_byte(context, buffer, length, flags)
uint32_t context;
uint32_t *buffer;
uint32_t length;
uint32_t flags;

{
    /* Build status control byte using only caller-provided data. */
    uint32_t status_bits;

    status_bits = CONCAT31((int32_t)((uint32_t)flags >> 8), 1);
    ((uint16_t (*)())core_update_constant_0xc)(context, &status_bits);
    if ((char)status_bits == '\0') {
        *buffer = 0x28;
    }
    return status_bits & 0xff;
}

/**
 * @brief Initialize SCSI response buffer.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
uint16_t *core_initialize_scsi_response_buffer(offset, value)
int offset;
uint32_t value;

{
    /* Initialize SCSI response buffer using only caller-provided data. */
    uint16_t *data_cursor;

    data_cursor = (uint16_t *)core_select_scsi_response_buffer();
    ti_memset(*(uint32_t *)(data_cursor + 2), 0, value);
    *(uint16_t **)(offset + 0xc) = data_cursor;
    *data_cursor = (int16_t)value;
    *(int16_t *)(*(int *)(offset + 0xc) + 2) = (int16_t)value;
    return data_cursor;
}

/**
 * @brief Format hex u32.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_format_hex_u32(offset, value) int offset;
uint32_t value;

{
    /* Format hex u32 using only caller-provided data. */
    uint8_t core_convert_nibble_to_hex_character_result;
    int operation_status;
    int callback_result;
    uint32_t packed_value;
    uint8_t *computed_offset;

    operation_status = 8;
    computed_offset = (uint8_t *)(offset + 8);
    *computed_offset = 0;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_convert_nibble_to_hex_character_result = core_convert_nibble_to_hex_character(value & 0xf, operation_status);
        value = packed_value >> 4;
        operation_status = callback_result + -1;
        computed_offset = computed_offset + -1;
        *computed_offset = core_convert_nibble_to_hex_character_result;
    } while (operation_status != 0);
    return;
}

/**
 * @brief Write bounded response field.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_write_bounded_response_field(buffer, value, condition, flags) uint16_t *buffer;
uint32_t value;
int condition;
uint32_t flags;

{
    /* Write bounded response field using only caller-provided data. */
    if ((int)(uint32_t)*buffer <= condition) {
        core_halt_forever();
    }
    core_write_big_endian_u32_field(*(uint32_t *)(buffer + 2), value, condition, flags);
    return;
}

/**
 * @brief Configure word descriptor.
 *
 * @param configuration Procedure input.
 * @param source_descriptor Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_configure_word_descriptor(configuration, source_descriptor, length, flags) uint32_t *configuration;
uint32_t *source_descriptor;
uint32_t length;
uint32_t flags;

{
    /* Configure word descriptor using only caller-provided data. */
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

/**
 * @brief Dispatch available record.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
uint32_t core_dispatch_available_record(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)

{
    /* Dispatch available record using only caller-provided data. */
    int operation_status;

    operation_status = core_get_expected_candidate();
    if (operation_status == 0) {
        /* Call core process expected flag expected through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_expected_flag_expected)(value, length, flags, index, context);
    }
    return 0;
}

/**
 * @brief Validate combined record bounds.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_validate_combined_record_bounds(offset)
int offset;

{
    /* Validate combined record bounds using only caller-provided data. */
    uint32_t operation_result;

    operation_result = 0;
    if (((*(uint32_t *)(offset + 4) < 0x1fc) && (*(uint32_t *)(offset + 8) < 0x1fc)) &&
        (*(uint32_t *)(offset + 4) + *(uint32_t *)(offset + 8) < 0x1fc)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Set thermal PWM duty.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_thermal_pwm_duty(context) uint32_t context;

{
    /* Set thermal PWM duty; persistent state is carried in core thermal pwm state, core pwm duty target. */
    if (core_thermal_pwm_state != 8) {
        /* Call core apply pwm percentage through the defined unprototyped function-pointer signature. */
        ((void (*)())core_apply_pwm_percentage)(0xf, context, 16000);
        core_pwm_duty_target = context;
    }
    return;
}

/**
 * @brief Get persistent identity record for initialize RDX identity.
 *
 * @return Result produced by the procedure.
 */
bool core_get_persistent_identity_record_for_initialize_rdx_identity()

{
    /* Get persistent identity record for initialize RDX identity; persistent state is carried in core persistent identity record. */
    int core_persistent_identity_record_pointer;

    core_persistent_identity_record_pointer = ((uint32_t (*)())core_calculate_record_checksum)(&core_persistent_identity_record, 0x40);
    return core_persistent_identity_record != core_persistent_identity_record_pointer;
}

/**
 * @brief Reset flash validation context.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_reset_flash_validation_context(offset) int offset;

{
    /* Reset flash validation context using only caller-provided data. */
    char offset_field_0x20;

    offset_field_0x20 = *(char *)(offset + 0x20);
    ti_memset(offset, 0, 0x198);
    core_reset_command_context_hook();
    if (offset_field_0x20 != '\0') {
        *(uint8_t *)(offset + 0x20) = 1;
    }
    return;
}

/**
 * @brief Initialize led controller registry.
 *
 * @return No value.
 */
void core_initialize_led_controller_registry()

{
    /* Initialize led controller registry; persistent state is carried in core led controller table, core led controller count. */
    ti_memset(&core_led_controller_table, 0, 8);
    core_led_controller_count = 0;
    /* Call core allocate software timer through the defined unprototyped function-pointer signature. */
    ((void (*)())core_allocate_software_timer)(0x800f291);
    return;
}

/**
 * @brief Initialize USB reconnect state.
 *
 * @return No value.
 */
void core_initialize_usb_reconnect_state()

{
    /* Initialize USB reconnect state; persistent state is carried in core usb reconnect settle timer. */
    core_reset_device_command_arbitration();
    ti_memset(&core_usb_reconnect_settle_timer, 0, 2);
    /* Call core allocate software timer through the defined unprototyped function-pointer signature. */
    ((void (*)())core_allocate_software_timer)(&core_usb_reconnect_settle_timer);
    return;
}

/**
 * @brief Reconnect device and start timer.
 *
 * @return No value.
 */
void core_reconnect_device_and_start_timer()

{
    /* Reconnect device and start timer; persistent state is carried in core usb reconnect settle timer, core usb disconnect requested. */
    usb_hal_connect();
    rti_set_software_timer(&core_usb_reconnect_settle_timer, 2000);
    core_usb_disconnect_requested = 0;
    return;
}

/**
 * @brief Unmask USB interrupt when allowed.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_unmask_usb_interrupt_when_allowed(condition) int condition;

{
    /* Unmask USB interrupt when allowed; persistent state is carried in core mww dispatch deferred, VIM request mask set zero register. */
    if ((core_mww_dispatch_deferred == '\0') || (condition != 0)) {
        /* Program VIM request mask set zero register with 0x200000; write ordering is hardware-significant. */
        vim_request_mask_set_zero_register = 0x200000;
    }
    return;
}

/**
 * @brief Find value in fixed table.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_find_value_in_fixed_table(condition)
int condition;

{
    /* Find value in fixed table using only caller-provided data. */
    int *data_cursor;
    int operation_status;

    operation_status = 3;
    data_cursor = (int *)0x800ea18;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        data_cursor = data_cursor + 1;
        if (condition == *data_cursor) {
            return 1;
        }
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    return 0;
}

/**
 * @brief Reset BOT command state.
 *
 * @return No value.
 */
void core_reset_bot_command_state()

{
    /* Reset BOT command state; persistent state is carried in core MWW saved request, core mww saved request reset companion word and related record fields. */
    core_mww_saved_request = 0;
    core_mww_saved_request_reset_companion_word = 0;
    usb_bot_transfer_case = 0;
    usb_bot_usb_transfer_complete = 0;
    usb_bot_sata_transfer_complete = 0;
    usb_bot_check_condition = 0;
    mww_submit_usb_transfer(3, usb_bot_cbw_pointer, 0x1f);
    return;
}

/**
 * @brief Process MWW process command.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_process_mww_process_command(context, value, length, flags) uint32_t context;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process MWW process command using only caller-provided data. */
    int operation_status;
    int flags_snapshot;

    flags_snapshot = flags;
    operation_status = mww_send_usb_endpoint_command(context, 5, 0, 0);
    if (operation_status == 0) {
        mww_update_mww_transfer_storage(context, &flags_snapshot);
        *(uint32_t *)(flags_snapshot + 0x10) = 0;
    }
    return;
}

/**
 * @brief Process MWW update resource.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
uint32_t core_process_mww_update_resource(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process MWW update resource using only caller-provided data. */
    int flags_snapshot;

    flags_snapshot = flags;
    mww_update_mww_transfer_storage(context, &flags_snapshot);
    mww_send_usb_endpoint_command(context, *(int *)(flags_snapshot + 0x1c) << 0x10 | 7, 0, 0);
    return 0;
}

/**
 * @brief Dispatch USB power management callbacks.
 *
 * @param context USB event value passed to each registered callback.
 * @return No value.
 */
void core_dispatch_usb_power_management_callbacks(context) uint32_t context;

{
    /* Advance from the cursor origin and invoke up to four callbacks, stopping at the first empty entry. */
    uint32_t *core_usb_uas_out_endpoint_three_callback_pointer;
    int operation_status;

    operation_status = 4;
    core_usb_uas_out_endpoint_three_callback_pointer = (uint32_t *)&core_usb_uas_out_endpoint_three_callback;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_usb_uas_out_endpoint_three_callback_pointer = core_usb_uas_out_endpoint_three_callback_pointer + 1;
        if ((firmware_callback_t *)*core_usb_uas_out_endpoint_three_callback_pointer == (firmware_callback_t *)0x0) {
            return;
        }
        (*(firmware_callback_t *)*core_usb_uas_out_endpoint_three_callback_pointer)(context);
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    return;
}

/**
 * @brief Clear media record state.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_clear_media_record_state(offset) int offset;

{
    /* Clear media record state using only caller-provided data. */
    ti_memset(offset + 0x10, 0, 0x34);
    ti_memset(offset + 0x44, 0, 0x38);
    return;
}

/**
 * @brief Copy record header to destination.
 *
 * @param buffer Procedure input.
 * @param offset Procedure input.
 * @return No value.
 */
void core_copy_record_header_to_destination(buffer, offset) int *buffer;
int offset;

{
    /* Copy record header to destination using only caller-provided data. */
    uint32_t core_decode_header_flags_and_offset_result;

    core_decode_header_flags_and_offset_result = core_decode_header_flags_and_offset(offset);
    core_copy_bytes(*(int *)(offset + 0x14) + *buffer, offset, core_decode_header_flags_and_offset_result);
    return;
}

/**
 * @brief Submit ATA identify device.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_submit_ata_identify_device(context, value, length, flags) uint32_t context;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Submit ATA identify device using only caller-provided data. */
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    uint32_t status_bits;

    value_derived = value;
    length_component = length;
    status_bits = flags;
    core_initialize_ata_taskfile(&value_derived);
    status_bits = (status_bits & 0xff000000u) | (CONCAT12(0xec, (uint16_t)status_bits) & 0x00ffffffu);
    ahci_execute_taskfile_command(&value_derived, context, 1, 0);
    return;
}

/**
 * @brief Rewrite unit identity record.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_rewrite_unit_identity_record(context) uint32_t context;

{
    /* Rewrite unit identity record using only caller-provided data. */
    int operation_status;

    operation_status = core_get_set_interrupt_mmio(0x3e000);
    if (operation_status == 0) {
        core_program_spi_flash_pages(0x3e000, 0x100, context);
    }
    return;
}

/**
 * @brief Process lookup arguments.
 *
 * @return No value.
 */
void core_process_lookup_arguments()

{
    /* Process lookup arguments using only caller-provided data. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint32_t callback_result = 0;
    uint64_t lookup_arguments;

    lookup_arguments = core_get_runtime_context_base_for_build_scsi_mode();
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode((int)lookup_arguments, (int)((uint64_t)lookup_arguments >> 0x20), (int)lookup_arguments);
    core_initialize_media_record(callback_result, core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    return;
}

/**
 * @brief Initialize word buffer.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_initialize_word_buffer(buffer, value, condition) uint32_t *buffer;
uint32_t value;
uint32_t condition;

{
    /* Initialize word buffer using only caller-provided data. */
    int condition_field;
    uint32_t *buffer_cursor;

    if (1 < condition) {
        condition_field = condition - 1;
        buffer_cursor = buffer;
        /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            buffer_cursor = buffer_cursor + 1;
            condition_field = condition_field + -1;
            *buffer_cursor = 0;
        } while (condition_field != 0);
    }
    *buffer = value;
    return;
}

/**
 * @brief Update flash validation context.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_update_flash_validation_context()

{
    /* Update flash validation context using only caller-provided data. */
    int core_get_identity_variant_code_result;
    uint32_t operation_result;

    core_get_identity_variant_code_result = core_get_identity_variant_code();
    if (core_get_identity_variant_code_result == 0x35) {
        return 2;
    }
    if (core_get_identity_variant_code_result == 0x36) {
        operation_result = 1;
    } else {
        if (core_get_identity_variant_code_result == 0x37) {
            return 2;
        }
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Process halt forever for selected or bit.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
uint32_t core_process_halt_forever_for_selected_or_bit(buffer, value, condition)
uint8_t *buffer;
uint32_t value;
int condition;

{
    /* Process halt forever for selected or bit using only caller-provided data. */
    if ((int)(uint32_t)*buffer <= condition) {
        core_halt_forever();
    }
    core_read_reversed_field_value(*(uint32_t *)(buffer + 4), value, condition);
    return 0;
}

/**
 * @brief Update release record payload.
 *
 * @param condition Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_update_release_record_payload(condition, offset, length, flags) int condition;
int offset;
uint32_t length;
uint32_t flags;

{
    /* Update release record payload using only caller-provided data. */
    uint32_t core_release_record_payload_when_short_result;

    core_release_record_payload_when_short_result = core_release_record_payload_when_short(offset + 4, length, flags);
    if (core_release_record_payload_when_short_result < *(uint16_t *)(condition + 2)) {
        *(int16_t *)(condition + 2) = (int16_t)core_release_record_payload_when_short_result;
    }
    return;
}

/**
 * @brief Get remaining bounded capacity.
 *
 * @param condition Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
int core_get_remaining_bounded_capacity(condition, offset)
uint32_t condition;
int offset;

{
    /* Get remaining bounded capacity using only caller-provided data. */
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

/**
 * @brief Update constant 0x54.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_update_constant_0x54(offset, value) int offset;
uint32_t value;

{
    /* Update constant 0x54 using only caller-provided data. */
    uint32_t offset_field_0x54;

    offset_field_0x54 = *(uint32_t *)(offset + 0x54);
    *(uint32_t *)(offset + 0x54) = value + offset_field_0x54;
    *(uint32_t *)(offset + 0x58) = *(int *)(offset + 0x58) + (uint32_t)CARRY4(value, offset_field_0x54);
    return;
}

/**
 * @brief Update constant 0x5c.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_update_constant_0x5c(offset, value) int offset;
uint32_t value;

{
    /* Update constant 0x5c using only caller-provided data. */
    uint32_t offset_field_0x5c;

    offset_field_0x5c = *(uint32_t *)(offset + 0x5c);
    *(uint32_t *)(offset + 0x5c) = value + offset_field_0x5c;
    *(uint32_t *)(offset + 0x60) = *(int *)(offset + 0x60) + (uint32_t)CARRY4(value, offset_field_0x5c);
    return;
}
