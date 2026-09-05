/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Build record type one response.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_record_type_one_response(context)
uint32_t context;

{
    /* Build record type one response using only caller-provided data. */
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;

    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 6);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 1, 1);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 2, 3, 2);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 4, 5, 1);
    return 0;
}

/**
 * @brief Update state constant 137 three.
 *
 * @param offset Procedure input.
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_state_constant_137_three(offset, condition, buffer)
int offset;
int condition;
uint32_t *buffer;

{
    /* Update state constant 137 three using only caller-provided data. */
    int core_process_command_phase_three_result;
    uint32_t working_result;

    core_process_command_phase_three_result = core_process_command_phase_three();
    if (core_process_command_phase_three_result == 0) {
        working_result = 2;
    } else {
        if (condition <= (int)(uint32_t)*(uint16_t *)(offset + 8)) {
            return 1;
        }
        working_result = 0x28;
    }
    *buffer = working_result;
    return 0;
}

/**
 * @brief Process command phase.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_process_command_phase(context) uint8_t context;

{
    /* Process command phase; persistent state is carried in core command phase, core command context value. */
    int operation_status;

    operation_status = core_get_notification_context_value();
    if ((operation_status == 2) && (operation_status = core_is_command_phase_three_for_build_scsi_mode(), operation_status == 0)) {
        core_update_reconnect_state();
    }
    core_update_sense_refresh_context();
    core_command_phase = 3;
    core_command_context_value = context;
    core_update_command_phase_flags_for_command_phase(4);
    return;
}

/**
 * @brief Update command phase.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_update_command_phase(buffer) uint32_t *buffer;

{
    /* Update command phase; persistent state is carried in core command phase. */
    uint32_t working_result;

    if (core_command_phase == 0) {
        working_result = 2;
    } else if (core_command_phase == 1) {
        working_result = 6;
    } else {
        if (core_command_phase != 2) {
            return core_command_phase != 3;
        }
        working_result = 5;
    }
    *buffer = working_result;
    return true;
}

/**
 * @brief Update firmware context USB transfer state.
 *
 * @return No value.
 */
void core_update_firmware_context_usb_transfer_state()

{
    /* Update firmware context USB transfer state; persistent state is carried in USB device state, USB event callback status and related record fields. */
    usb_device_state = 4;
    usb_event_callback_status = 3;
    core_endpoint_context = 0;
    if (core_usb_event_callback != (firmware_callback_t)0) {
        /* Notify core USB event callback only after the associated state fields have been committed. */
        (*core_usb_event_callback)(3);
    }
    core_response_status_value = 0;
    return;
}

/**
 * @brief Process firmware context endpoint context.
 *
 * @return No value.
 */
void core_process_firmware_context_endpoint_context()

{
    /* Process firmware context endpoint context; persistent state is carried in core endpoint context, USB stack ready and related record fields. */
    core_endpoint_context = 1;
    usb_hal_cancel_all_io_requests();
    core_update_command_status_update(0);
    usb_stack_ready = 1;
    usb_event_callback_status = 2;
    if (core_usb_event_callback != (firmware_callback_t)0) {
        /* Notify core USB event callback only after the associated state fields have been committed. */
        (*core_usb_event_callback)(2);
    }
    return;
}

/**
 * @brief Update device base device base context.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
bool core_update_device_base_device_base_context(context, value) uint8_t context;
uint32_t value;

{
    /* Update device base device base context; persistent state is carried in core device base context. */
    uint32_t core_device_base_context_snapshot;
    uint32_t device_base;

    core_device_base_context_snapshot = (uint32_t)core_device_base_context;
    if (3 >= core_device_base_context_snapshot) {
        device_base = (uint32_t)core_device_base_context;
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        *(uint32_t *)(core_device_base_context_snapshot * 4 + 0x800f6d0) = value;
        core_device_base_context = core_device_base_context + 1;
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        *(uint8_t *)(device_base + 0x800f6e0) = context & 0x7f;
    }
    return 3 < core_device_base_context_snapshot;
}

/**
 * @brief Transfer component for fallback decoded request.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_component_for_fallback_decoded_request(context, condition, length)
int context;
uint32_t condition;
uint32_t length;

{
    /* Transfer component for fallback decoded request using only caller-provided data. */
    uint32_t entry_index;
    char *context_derived;
    uint32_t condition_field;
    uint32_t length_component;

    entry_index = 0;
    length_component = length;
    /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
    do {
        if (entry_index < condition) {
            condition_field = (condition - entry_index) + 3 >> 2;
            context_derived = (char *)(context + entry_index);
            /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
            do {
                condition_field = condition_field - 1;
                *(char *)((int)&length_component + entry_index) = *context_derived + *(char *)((int)&length_component + entry_index);
                context_derived = context_derived + 4;
            } while (condition_field != 0);
        }
        entry_index = entry_index + 1;
    } while (entry_index < 4);
    return length_component;
}

/**
 * @brief Transfer component for flash validation context.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_component_for_flash_validation_context(context, condition, length)
int context;
uint32_t condition;
uint32_t length;

{
    /* Transfer component for flash validation context using only caller-provided data. */
    uint32_t entry_index;
    char *context_derived;
    uint32_t condition_field;
    uint32_t length_component;

    entry_index = 0;
    length_component = length;
    /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
    do {
        if (entry_index < condition) {
            condition_field = (condition - entry_index) + 3 >> 2;
            context_derived = (char *)(context + entry_index);
            /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
            do {
                condition_field = condition_field - 1;
                *(char *)((int)&length_component + entry_index) = *context_derived + *(char *)((int)&length_component + entry_index);
                context_derived = context_derived + 4;
            } while (condition_field != 0);
        }
        entry_index = entry_index + 1;
    } while (entry_index < 4);
    return length_component;
}

/**
 * @brief Apply pwm percentage.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_apply_pwm_percentage(context, condition) uint8_t context;
uint32_t condition;

{
    /* Apply pwm percentage using only caller-provided data. */
    int *data_cursor;

    data_cursor = (int *)spi_get_spi_transfer_storage(context);
    /* Gate the following state transition on *data_cursor == 0xc || *data_cursor == 0xd; the handler is skipped when this condition is false. */
    if (*data_cursor == 0xc || *data_cursor == 0xd) {
        if (99 < condition) {
            condition = 100;
        }
        if ((char)data_cursor[1] == '\0') {
            condition = 100 - condition;
        }
        /* Call core update RTI clock mhz period units through the defined unprototyped function-pointer signature. */
        ((void (*)())core_update_rti_clock_mhz_period_units)(*data_cursor + -0xc, condition);
    }
    return;
}

/**
 * @brief Process reset output record.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
bool core_process_reset_output_record(context) uint32_t context;

{
    /* Process reset output record using only caller-provided data. */
    int status_bits;
    int working_result;

    core_reset_output_record();
    status_bits = core_get_runtime_context_base_for_build_scsi_mode();
    status_bits = core_accumulate_split_transfer_offsets(status_bits + 0x2c);
    working_result = core_get_runtime_context_base_for_build_scsi_mode();
    working_result = core_initialize_output_record_from_source(working_result + 0x2c, context);
    return working_result != 0 && status_bits != 0;
}

/**
 * @brief Update availability x01.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_availability_x01(offset)
int offset;

{
    /* Update availability x01 using only caller-provided data. */
    char core_read_context_field_6c_byte_result;
    int availability_status;
    uint32_t operation_result;
    uint32_t callback_result = 0;

    operation_result = 0;
    if (((*(int *)(offset + 0x44) == 2) &&
         (((core_read_context_field_6c_byte_result = *(char *)(offset + 0x11),
            core_read_context_field_6c_byte_result == '\x01' || (core_read_context_field_6c_byte_result == '\x02' || core_read_context_field_6c_byte_result == '\x04')) &&
           ((*(uint32_t *)(offset + 0x20) & ~*(uint32_t *)(offset + 0x78)) == 0)))) &&
        (availability_status = core_read_context_field_6c_byte(), operation_result = callback_result, availability_status != 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Get word.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_get_word(context, value)
uint32_t context;
uint32_t value;

{
    /* Get word using only caller-provided data. */
    uint32_t core_transfer_remaining_bounds_derived;
    uint32_t high_word;

    core_transfer_remaining_bounds_derived = ((int (*)())core_transfer_remaining_bounds)(context, 4, value);
    high_word = ((int (*)())core_transfer_remaining_bounds)(context, 4, value);
    return CONCAT44(high_word, core_transfer_remaining_bounds_derived);
}

/**
 * @brief Validate transfer capacity.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_validate_transfer_capacity(buffer, value, length)
char *buffer;
uint32_t value;
uint32_t length;

{
    /* Validate transfer capacity using only caller-provided data. */
    int core_get_remaining_bounded_capacity_result;
    uint32_t operation_result;
    uint32_t callback_result = 0;

    operation_result = 1;
    /* Gate the following state transition on (((buffer[1] & 5U) != 0) || (*buffer != '\0'); the handler is skipped when this condition is false. */
    if ((((buffer[1] & 5U) != 0) || (*buffer != '\0')) &&
        (core_get_remaining_bounded_capacity_result = core_get_remaining_bounded_capacity(value, length), operation_result = callback_result, core_get_remaining_bounded_capacity_result != 0)) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Reset output record.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_reset_output_record(offset) int offset;

{
    /* Reset output record using only caller-provided data. */
    core_release_nested_record_resource(offset);
    *(uint32_t *)(offset + 0x20) = 0;
    ti_memset(offset + 0xc, 0, 0x14);
    *(uint32_t *)(offset + 0x20) = 0;
    *(uint8_t *)(offset + 0xc) = 0x1f;
    return;
}

/**
 * @brief Update configuration update context.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
int core_update_configuration_update_context(condition)
int condition;

{
    /* Update configuration update context; persistent state is carried in core configuration update context, core state 044. */
    int *data_cursor;
    int core_process_command_processing_context_result;
    int operation_result;

    if (condition < 4) {
        data_cursor = (int *)(&core_configuration_update_context + condition * 4);
    } else {
        data_cursor = (int *)&core_state_044;
    }
    operation_result = *data_cursor;
    core_process_command_processing_context_result = core_process_command_processing_context();
    if ((core_process_command_processing_context_result != 0) && (condition == 0)) {
        operation_result = operation_result + 5000;
    }
    return operation_result;
}

/**
 * @brief Update capacity block context.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_capacity_block_context(buffer)
uint32_t *buffer;

{
    /* Update capacity block context; persistent state is carried in ATA identify device data. */
    int core_update_default_mww_transfer_value_result;

    core_update_default_mww_transfer_value_result = core_update_default_mww_transfer_value();
    if (core_update_default_mww_transfer_value_result != 0) {
        *(uint16_t *)(buffer + 1) = 0;
        *(uint16_t *)((int)buffer + 6) = 0;
        *buffer = (uint32_t)&ata_identify_device_data;
        return 1;
    }
    return 0;
}

/**
 * @brief Process clear memory clear memory secondary memory region.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_process_clear_memory_clear_memory_secondary_memory_region(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Process clear memory clear memory secondary memory region; persistent state is carried in core secondary memory region. */
    uint32_t ti_memset_result;
    uint32_t context_derived;
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;

    context_derived = context;
    value_derived = value;
    length_component = length;
    flags_snapshot = flags;
    core_process_end_remaining_for_ti_memset_ti(&context_derived, &core_secondary_memory_region);
    /* Call core configure word two byte one word two byte two through the defined unprototyped function-pointer signature. */
    ((void (*)())core_configure_word_two_byte_one_word_two_byte_two)(context, &context_derived, value);
    ti_memset_result = (uint32_t)ti_memset(&context_derived, 0, 0x10);
    return CONCAT44(value_derived, ti_memset_result);
}

/**
 * @brief Process clear memory clear memory primary memory region.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_process_clear_memory_clear_memory_primary_memory_region(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Process clear memory clear memory primary memory region; persistent state is carried in core primary memory region. */
    uint32_t ti_memset_result;
    uint32_t context_derived;
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;

    context_derived = context;
    value_derived = value;
    length_component = length;
    flags_snapshot = flags;
    core_process_end_remaining_for_ti_memset_ti(&context_derived, &core_primary_memory_region);
    /* Call core process USB event context through the defined unprototyped function-pointer signature. */
    ((void (*)())core_process_usb_event_context)(context, &context_derived, value);
    ti_memset_result = (uint32_t)ti_memset(&context_derived, 0, 0x10);
    return CONCAT44(value_derived, ti_memset_result);
}

/**
 * @brief Process clear memory clear memory copy context.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_process_clear_memory_clear_memory_copy_context(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Process clear memory clear memory copy context; persistent state is carried in core memory copy context. */
    uint32_t ti_memset_result;
    uint32_t context_derived;
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;

    context_derived = context;
    value_derived = value;
    length_component = length;
    flags_snapshot = flags;
    core_transfer_bounded_transfer_context(&context_derived, &core_memory_copy_context);
    /* Call core transfer remaining bytes word through the defined unprototyped function-pointer signature. */
    ((void (*)())core_transfer_remaining_bytes_word)(context, value >> 2, &context_derived);
    ti_memset_result = (uint32_t)ti_memset(&context_derived, 0, 0x10);
    return CONCAT44(value_derived, ti_memset_result);
}

/**
 * @brief Process command code in.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_command_code_in(context, value)
int context;
uint32_t value;

{
    /* Process command code in using only caller-provided data. */
    int core_is_command_code_in_standard_range_result;
    uint32_t core_translate_command_code_result;
    uint32_t callback_result = 0;
    uint32_t context_derived;
    uint32_t null_context = 0;

    core_is_command_code_in_standard_range_result = core_is_command_code_in_standard_range(value, value, value);
    context_derived = 0;
    if (core_is_command_code_in_standard_range_result != 0) {
        core_translate_command_code_result = core_translate_command_code(callback_result);
        context_derived = null_context;
        if ((uint32_t)((int)core_translate_command_code_result >> 3) < 0x10) {
            context_derived = (int)(uint32_t)*(uint8_t *)(context + ((int)core_translate_command_code_result >> 3)) >> (core_translate_command_code_result & 7) & 1;
        }
    }
    return context_derived;
}

/**
 * @brief Transfer reverse.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_reverse(context, value, condition)
int context;
int value;
int condition;

{
    /* Transfer reverse using only caller-provided data. */
    uint32_t *condition_field;
    uint32_t *reverse_cursor;

    if (condition != 0) {
        condition_field = (uint32_t *)(condition * 4 + context);
        reverse_cursor = (uint32_t *)(condition * 4 + value);
        /* Repeat while condition != 0; the body advances or polls the state needed to leave the loop. */
        do {
            reverse_cursor = reverse_cursor + -1;
            condition_field = condition_field + -1;
            /* Gate the following state transition on *reverse_cursor < *condition_field; the handler is skipped when this condition is false. */
            if (*reverse_cursor < *condition_field) {
                return 1;
            }
            /* Gate the following state transition on *condition_field < *reverse_cursor; the handler is skipped when this condition is false. */
            if (*condition_field < *reverse_cursor) {
                return 0xffffffff;
            }
            condition = condition + -1;
        } while (condition != 0);
    }
    return 0;
}

/**
 * @brief Process RTI clock mhz.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_process_rti_clock_mhz(context) int context;

{
    /* Process RTI clock mhz using only caller-provided data. */
    *(uint32_t *)(context * 0x400 + -0x877fc) = 1;
    *(uint32_t *)(context * 0x400 + -0x877f8) = 0x12;
    core_update_rti_clock_mhz_period_units(context, 0, 4000);
    core_signal_channel_completion(context);
    return;
}

/**
 * @brief Process notify change notification context.
 *
 * @param mode Procedure input.
 * @param notify_change Procedure input.
 * @return No value.
 */
void core_process_notify_change_notification_context(mode, notify_change) int mode;
int notify_change;

{
    /* Process notify change notification context; persistent state is carried in core notification context, core command feature state. */
    core_notification_context = mode;
    if (notify_change != 0) {
        core_process_persistent_identity_record_for_notify_change_resource(mode);
    }
    core_update_reconnect_state();
    if (mode == 2) {
        core_set_notification_state_to_zero_for_notify_change_resource();
        core_command_feature_state = 0;
    }
    return;
}

/**
 * @brief Process configuration.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_process_configuration(buffer) uint32_t *buffer;

{
    /* Process configuration using only caller-provided data. */
    int operation_status;
    uint32_t status_bits;
    uint32_t working_result = 0;
    uint16_t configuration_value = 0;

    operation_status = ((bool (*)())core_handle_descriptor_request_bytes)(&status_bits, buffer);
    if (operation_status != 0) {
        operation_status = core_configure_ata_command(status_bits, working_result, configuration_value);
        if (operation_status == 0) {
            *buffer = 0x17;
        }
        core_update_mark_primary_sense();
    }
    return;
}

/**
 * @brief Update flag one interrupt transfer context.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_update_flag_one_interrupt_transfer_context(buffer) char *buffer;

{
    /* Update flag one interrupt transfer context; persistent state is carried in core interrupt transfer context, core thermal control context. */
    uint32_t core_interrupt_transfer_context_snapshot;

    core_interrupt_transfer_context_snapshot = (uint32_t)core_interrupt_transfer_context;
    if (0x12 < core_interrupt_transfer_context_snapshot) {
        core_halt_forever();
        core_interrupt_transfer_context_snapshot = (uint32_t)core_interrupt_transfer_context;
    }
    *buffer = (char)core_interrupt_transfer_context_snapshot;
    *(uint32_t *)(&core_thermal_control_context + core_interrupt_transfer_context_snapshot * 4) = 0;
    core_interrupt_transfer_context = (char)core_interrupt_transfer_context_snapshot + '\x01';
    return;
}

/**
 * @brief Process halt forever for response end word.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
void core_process_halt_forever_for_response_end_word(uint16_t *buffer, uint32_t value, int condition, uint32_t flags, uint32_t index)

{
    /* Process halt forever for response end word using only caller-provided data. */
    if ((int)(uint32_t)*buffer <= condition) {
        core_halt_forever();
    }
    core_copy_reversed_field_bytes(*(uint32_t *)(buffer + 2), value, condition, flags, index);
    return;
}

/**
 * @brief Update persistent state status.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_update_persistent_state_status(buffer) uint32_t *buffer;

{
    /* Update persistent state status; persistent state is carried in core persistent state status, core state 190. */
    core_process_response_status_value(*buffer, buffer[1], buffer[2], *(uint16_t *)(buffer + 3), *(uint16_t *)((int)buffer + 0xe));
    core_persistent_state_status = *(uint8_t *)((int)buffer + 0x11);
    core_state_190 = *(uint8_t *)(buffer + 4);
    return;
}

/**
 * @brief Get range end range start.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param length Procedure input.
 * @param range_end Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_range_end_range_start(context, range_start, length, range_end)
uint32_t context;
uint32_t range_start;
uint32_t length;
uint32_t range_end;

{
    /* Get range end range start using only caller-provided data. */
    uint32_t length_component;
    uint32_t condition_2_field;

    length_component = length;
    condition_2_field = range_end;
    ((uint32_t (*)())core_configure_component_scaled)(&length_component, context);
    if ((range_start <= condition_2_field) && ((range_start < condition_2_field || (range_end < length_component)))) {
        return 1;
    }
    return 0;
}

/**
 * @brief Process capacity block context.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_capacity_block_context(buffer)
uint32_t *buffer;

{
    /* Process capacity block context using only caller-provided data. */
    uint16_t working_result;
    int operation_status;
    uint32_t core_read_buffer_word_result;

    operation_status = core_update_capacity_block_context(buffer + 2);
    if (operation_status != 0) {
        core_read_buffer_word_result = core_read_buffer_word(buffer + 2);
        *buffer = core_read_buffer_word_result;
        operation_status = (int)core_get_ata_command_buffer();
        working_result = *(uint16_t *)(operation_status + 0x10);
        buffer[1] = (uint32_t)working_result;
        *(uint16_t *)(buffer + 3) = working_result;
        return 1;
    }
    return 0;
}

/**
 * @brief Process bitset base.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_process_bitset_base(context, value) uint32_t context;
uint32_t value;

{
    /* Process bitset base using only caller-provided data. */
    int core_is_command_code_in_standard_range_result;
    uint32_t core_translate_command_code_result;
    uint32_t callback_result = 0;
    uint32_t operation_status;
    int bitset_base = 0;

    core_is_command_code_in_standard_range_result = core_is_command_code_in_standard_range(value, value, value, context);
    if (core_is_command_code_in_standard_range_result != 0) {
        core_translate_command_code_result = core_translate_command_code(callback_result);
        operation_status = (int)core_translate_command_code_result >> 3;
        if (operation_status < 0x10) {
            *(uint8_t *)(bitset_base + operation_status) =
                (uint8_t)(1 << (core_translate_command_code_result & 7)) | *(uint8_t *)(bitset_base + operation_status);
        }
    }
    return;
}

/**
 * @brief Get transfer transfer.
 *
 * @param transfer_context Procedure input.
 * @param source_value Procedure input.
 * @param transfer_length Required transfer length.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @param destination Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_transfer_transfer(transfer_context, source_value, transfer_length, flags, index, destination)
int transfer_context;
int source_value;
int transfer_length;
int flags;
uint32_t index;
uint32_t destination;

{
    /* Get transfer transfer using only caller-provided data. */
    uint32_t operation_result;

    operation_result = 0;
    if ((((transfer_context != 0) && (source_value != 0)) && (transfer_length != 0)) && (((flags != 0 && (index < 0x21)) && (destination < 0x21)))) {
        ((uint32_t (*)())core_transfer_find_last_nonzero)(transfer_context);
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Process AHCI port control.
 *
 * @return No value.
 */
void core_process_ahci_port_control()

{
    /* Process AHCI port control; persistent state is carried in AHCI port SATA control register. */
    core_update_masked_register_bits(&ahci_port_sata_control_register, 0xf, 1);
    rti_delay_microseconds(5000);
    core_update_masked_register_bits(&ahci_port_sata_control_register, 0xf, 0);
    return;
}

