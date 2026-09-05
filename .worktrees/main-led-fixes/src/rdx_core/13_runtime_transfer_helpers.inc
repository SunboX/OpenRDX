/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Configure component scaled.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_configure_component_scaled(buffer, value, length)
uint32_t *buffer;
uint32_t value;
uint32_t length;

{
    /* Configure component scaled using only caller-provided data. */
    int entry_index;
    uint32_t buffer_cursor;
    uint32_t length_component;
    uint32_t status_bits;
    uint32_t scaled_length;

    length_component = (length & 0xffff) * (value >> 0x10);
    scaled_length = (length >> 0x10) * (value & 0xffff) + length_component;
    *buffer = (value & 0xffff) * (length & 0xffff);
    status_bits = scaled_length * 0x10000;
    buffer_cursor = status_bits + *buffer;
    entry_index =
        (scaled_length >> 0x10) + (uint32_t)(scaled_length <= length_component && length_component - scaled_length != 0) * 0x10000;
    *buffer = buffer_cursor;
    if (buffer_cursor <= status_bits && status_bits - buffer_cursor != 0) {
        entry_index = entry_index + 1;
    }
    buffer[1] = (length >> 0x10) * (value >> 0x10) + entry_index;
    return 0;
}

/**
 * @brief Process clear memory buffer compare context.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t core_process_clear_memory_buffer_compare_context(context, buffer)
uint32_t context;
uint8_t *buffer;

{
    /* Process clear memory buffer compare context; persistent state is carried in core buffer compare context. */
    int operation_status;
    uint8_t operation_result;
    uint8_t working_buffer[32];
    uint8_t output_buffer[32];

    if (buffer == (uint8_t *)0x0) {
        buffer = working_buffer;
    }
    operation_status = core_process_remaining_descriptor_space_for_copy_copy_resource(context, buffer, 0x20, &core_buffer_compare_context);
    if (operation_status == 0) {
        ti_memset(output_buffer, 0, 0x20);
        operation_status = core_compare_buffers_from_end(buffer, output_buffer, 0x20);
        operation_result = operation_status == 0;
    } else {
        operation_result = 2;
    }
    return operation_result;
}

/**
 * @brief Process component for command sequence update.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_component_for_command_sequence_update(context, value, length, buffer)
uint32_t context;
uint32_t value;
uint8_t length;
uint32_t *buffer;

{
    /* Process component for command sequence update using only caller-provided data. */
    int operation_result;
    uint32_t core_get_payload_offset_from_flags_result;
    uint8_t working_buffer[24];
    uint8_t status_bits;
    volatile uint8_t length_component = 0;
    volatile uint8_t working_result = 0;

    working_result = 1;
    status_bits = 0x27;
    length_component = length;
    operation_result = core_get_expected_candidate(value, &status_bits);
    if (operation_result == 0) {
        operation_result = core_run_expected_flag_expected_type(status_bits, 0, working_buffer, value);
    }
    if (operation_result == 0) {
        core_get_payload_offset_from_flags_result = core_get_payload_offset_from_flags(working_buffer);
        *buffer = core_get_payload_offset_from_flags_result;
    }
    return operation_result;
}

/**
 * @brief Transfer remaining bounds.
 *
 * @param cursor_state Procedure input.
 * @param condition Procedure input.
 * @param bounds Procedure input.
 * @return Result produced by the procedure.
 */
int core_transfer_remaining_bounds(cursor_state, condition, bounds)
uint32_t *cursor_state;
uint32_t condition;
int *bounds;

{
    /* Transfer remaining bounds using only caller-provided data. */
    uint8_t buffer_2_cursor;
    uint32_t status_bits;
    uint32_t remaining_count;
    int operation_result;
    uint32_t buffer_1_cursor;

    remaining_count = 0;
    operation_result = 0;
    if (condition < 5) {
        if (condition == 0) {
            return 0;
        }
    } else {
        condition = 4;
    }
    buffer_1_cursor = *cursor_state;
    /* Repeat while condition != 0; the body advances or polls the state needed to leave the loop. */
    do {
        if ((uint32_t)bounds[1] <= buffer_1_cursor) {
            return operation_result;
        }
        buffer_2_cursor = *(uint8_t *)(buffer_1_cursor + *bounds);
        condition = condition - 1;
        buffer_1_cursor = *cursor_state + 1;
        status_bits = remaining_count & 0x1f;
        remaining_count = remaining_count + 1 & 0xff;
        *cursor_state = buffer_1_cursor;
        operation_result = operation_result + ((uint32_t)buffer_2_cursor << (status_bits << 3));
    } while (condition != 0);
    return operation_result;
}

/**
 * @brief Handle ATA standby context for cartridge monitor context.
 *
 * @return No value.
 */
void core_handle_ata_standby_context_for_cartridge_monitor_context()

{
    /* Handle ATA standby context for cartridge monitor context; persistent state is carried in core sense update context, core ATA standby context. */
    uint8_t core_sense_update_context_snapshot;
    int operation_status;

    operation_status = core_update_sense_record_context();
    if (operation_status == 0) {
        core_update_identity_persistence_context();
        core_mark_secondary_sense_record_not_ready(0x800f29c);
        if (core_ata_standby_context != '\0') {
            operation_status = core_get_runtime_context_base_for_build_scsi_mode();
            core_update_saturating_add_u32_for_ata_standby_context(operation_status + 0x2c);
        }
        core_handle_ata_standby_context();
        operation_status = core_get_response_type_value();
        core_sense_update_context_snapshot = core_sense_update_context;
        if (operation_status == 0) {
            core_sense_update_context_snapshot = 0;
        }
        core_update_drive_mechanism_outputs(core_sense_update_context_snapshot);
    }
    return;
}

/**
 * @brief Process command completion context.
 *
 * @return No value.
 */
void core_process_command_completion_context()

{
    /* Process command completion context; persistent state is carried in core command completion context, core state 111 and related record fields. */
    int operation_status;

    /* Controller F2B4 uses logical outputs 6 then 7. Insertion and activity
     * use this pair as cartridge green then amber; physical
     * identity remains separate from the generic first/second state. */
    core_process_clear_memory_spi_response_context(
        0x800f2b4,
        RDX_CARTRIDGE_GREEN_LOGICAL_OUTPUT,
        RDX_CARTRIDGE_AMBER_LOGICAL_OUTPUT);
    gio_configure_pin_configuration_pin_number_peripheral_pin_configuration(3, &gio_pin_three_callback_entry);
    core_update_flag_one_interrupt_transfer_context(&core_command_completion_context);
    operation_status = core_forward_sci_read_logical();
    if (operation_status == 0) {
        core_state_111 = 8;
    } else {
        core_state_111 = 4;
    }
    core_update_flag_one_interrupt_transfer_context(&core_state_114);
    return;
}

/**
 * @brief Process runtime context base for RDX management record.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_runtime_context_base_for_rdx_management_record(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Process runtime context base for RDX management record using only caller-provided data. */
    int operation_status;
    uint32_t core_process_word_words_device_descriptor_context_derived;
    uint8_t working_buffer[44];

    operation_status = core_process_command_phase_three();
    if (operation_status == 0) {
        core_process_word_words_device_descriptor_context_derived = 2;
    } else {
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, working_buffer);
        if (operation_status == 0) {
            core_process_word_words_device_descriptor_context_derived = 0x21;
        } else {
            core_process_word_words_device_descriptor_context_derived = ((uint32_t (*)())core_process_word_words_device_descriptor_context)(context, value, length, working_buffer);
            core_reset_output_record(working_buffer);
        }
    }
    return core_process_word_words_device_descriptor_context_derived;
}

/**
 * @brief Process component for parsed header descriptor.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_process_component_for_parsed_header_descriptor(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)

{
    /* Process component for parsed header descriptor using only caller-provided data. */
    int operation_status;
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;

    value_derived = value;
    length_component = length;
    flags_snapshot = flags;
    operation_status = ((uint32_t (*)())core_handle_descriptor_device)(context, 1, value, (uint16_t)index, &value_derived);
    if (operation_status == 0) {
        operation_status = ((uint32_t (*)())core_process_device_use_lookup)(context, &value_derived, length, flags, index >> 0x10 & 0xff);
        core_process_ata_command_for_lookup_key_metadata(context);
    }
    return CONCAT44(value_derived, (uint32_t)(operation_status == 0));
}

/**
 * @brief Transfer ti memset n.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_ti_memset_n()

{
    /* Transfer ti memset n using only caller-provided data. */
    int operation_status;
    char *data_cursor;
    uint32_t operation_result;
    uint8_t working_buffer[32];

    operation_status = core_update_capacity_block_context(working_buffer);
    operation_result = 1;
    if (operation_status != 0) {
        operation_status = core_read_buffer_word(working_buffer);
        ti_memset(operation_status, 10, 0x1000);
        data_cursor = (char *)(operation_status + -1);
        operation_status = 0x1000;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            data_cursor = data_cursor + 1;
            /* Gate the following state transition on *data_cursor != '\n'; the handler is skipped when this condition is false. */
            if (*data_cursor != '\n') {
                operation_result = 0;
                break;
            }
            operation_status = operation_status + -1;
        } while (operation_status != 0);
        core_update_thermal_monitor_enabled_for_read_smart_temperature(working_buffer);
    }
    return operation_result;
}

/**
 * @brief Process MWW saved request.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_process_mww_saved_request(context) int context;

{
    /* Process MWW saved request; persistent state is carried in core MWW saved request, MWW source descriptor. */
    int core_is_supported_forward_opcode_result;

    if (((core_mww_saved_request == 1) || (core_mww_saved_request == 2 || core_mww_saved_request == 3)) &&
        (core_is_supported_forward_opcode_result = core_is_supported_forward_opcode(*(uint8_t *)(mww_source_descriptor + 0xf)), core_is_supported_forward_opcode_result != 0)) {
        /* Call core process MWW saved request for MWW handle saved through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_mww_saved_request_for_mww_handle_saved)(1);
        core_process_request_for_mww_saved_request(*(uint8_t *)(mww_source_descriptor + 0xf), *(uint8_t *)(context + 2), *(uint8_t *)(context + 1));
        mww_reset_rw_offsets();
        mww_process_device_transfer_state(1);
    }
    return;
}

/**
 * @brief Configure descriptor configuration context.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_configure_descriptor_configuration_context(condition) int condition;

{
    /* Configure descriptor configuration context; persistent state is carried in core descriptor configuration context, core storage 004 and related record fields. */
    int status_bits;
    int working_result;

    working_result = 0x11250;
    if (condition != 0) {
        working_result = 0x11210;
    }
    if (core_descriptor_configuration_context == '\0') {
        status_bits = 3;
    } else {
        status_bits = 4;
    }
    *(uint32_t *)(working_result + -0x40000000) = 0;
    *(uint32_t *)(&core_storage_004 + working_result) = 0;
    *(uint32_t *)(&core_storage_005 + working_result) = 0;
    *(uint32_t *)(&core_storage_006 + working_result) = status_bits << 4 | 3;
    mww_configure_transfer_descriptor(condition, (uint32_t *)(working_result + -0x40000000), 0);
    return;
}

/**
 * @brief Advance control transfer state.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_advance_control_transfer_state(buffer) uint32_t *buffer;

{
    /* Advance control transfer state; persistent state is carried in core firmware transfer context, core state 058. */
    if (core_firmware_transfer_context == 1) {
        if (((int)buffer[2] < 1) || (buffer[3] != buffer[1])) {
            core_firmware_transfer_context = 4;
        } else {
            core_update_mww_process_transfer(0x80, *buffer);
        }
    } else if (core_firmware_transfer_context == 3) {
        if (core_state_058 != 0) {
            core_update_usb_reset_state();
        }
        core_firmware_transfer_context = 0;
    } else {
        core_firmware_transfer_context = 5;
    }
    core_process_firmware_transfer_resource();
    return;
}

/**
 * @brief Process remaining descriptor space for copy copy resource.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_remaining_descriptor_space_for_copy_copy_resource(context, value, condition, flags)
uint32_t context;
uint32_t value;
uint32_t condition;
uint32_t flags;

{
    /* Process remaining descriptor space for copy copy resource using only caller-provided data. */
    int operation_result;
    uint32_t operation_status;
    uint32_t core_get_payload_offset_from_flags_result;
    uint8_t working_buffer[24];

    operation_result = core_process_expected_candidate(context, flags, working_buffer);
    if (operation_result == 0) {
        operation_status = core_get_remaining_descriptor_space(working_buffer);
        core_get_payload_offset_from_flags_result = core_get_payload_offset_from_flags(working_buffer);
        if (condition <= operation_status) {
            operation_status = condition;
        }
        core_transfer_ti_memset_component(value, operation_status, core_get_payload_offset_from_flags_result, context);
    }
    return operation_result;
}

/**
 * @brief Transfer tracker.
 *
 * @param destination Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @param offset_tracker Procedure input.
 * @return No value.
 */
void core_transfer_tracker(destination, value, condition, offset_tracker) int *destination;
uint32_t value;
uint32_t condition;
uint32_t *offset_tracker;

{
    /* Transfer tracker using only caller-provided data. */
    uint32_t entry_index;
    uint8_t *data_cursor;
    uint32_t buffer_length;
    uint32_t operation_status;

    entry_index = *offset_tracker;
    buffer_length = destination[1];
    if (entry_index < buffer_length) {
        operation_status = buffer_length - entry_index;
    } else {
        operation_status = 0;
    }
    if (operation_status < condition) {
        if (buffer_length <= entry_index) {
            condition = 0;
            goto core_transfer_tracker_before_updating_data_cursor;
        }
        condition = buffer_length - entry_index;
    }
    if (4 < condition) {
        condition = 4;
    }
/* Shared target for core transfer tracker before updating data cursor; incoming paths preserve the same state assumptions. */
core_transfer_tracker_before_updating_data_cursor:
    if (condition != 0) {
        data_cursor = (uint8_t *)(entry_index + *destination);
        entry_index = condition;
        /* Repeat while entry_index != 0; the body advances or polls the state needed to leave the loop. */
        do {
            *data_cursor = (char)value;
            entry_index = entry_index - 1;
            value = value >> 8;
            data_cursor = data_cursor + 1;
        } while (entry_index != 0);
    }
    *offset_tracker = condition + *offset_tracker;
    return;
}

/**
 * @brief Transfer range end range start for validate structured flash.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_range_end_range_start_for_validate_structured_flash(context, range_start, range_end)
int context;
int range_start;
int range_end;

{
    /* Transfer range end range start for validate structured flash using only caller-provided data. */
    int condition_2_field;
    uint32_t *data_cursor;
    uint8_t *source_cursor;
    uint32_t operation_result;

    operation_result = 0;
    if (range_end < range_start) {
        core_halt_forever();
    }
    if (range_start < 0) {
        core_halt_forever();
    }
    if (3 < (uint32_t)(range_end - range_start)) {
        core_halt_forever();
    }
    if (range_start <= range_end) {
        condition_2_field = (range_end - range_start) + 1;
        data_cursor = &operation_result;
        source_cursor = (uint8_t *)(range_end + context);
        /* Repeat while condition_2_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            condition_2_field = condition_2_field + -1;
            *(uint8_t *)data_cursor = *source_cursor;
            data_cursor = (uint32_t *)((int)data_cursor + 1);
            source_cursor = source_cursor + -1;
        } while (condition_2_field != 0);
    }
    return operation_result;
}

/**
 * @brief Process flag one for SPI response context.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_process_flag_one_for_spi_response_context(buffer) int *buffer;

{
    /* Process flag one for SPI response context using only caller-provided data. */
    int buffer_cursor;
    uint8_t working_result;

    if (*(char *)((int)buffer + 6) != '\x01') {
        core_process_response_flag_x01(buffer);
        goto core_process_flag_one_for_spi_response_context_before_update_spi_set_logical_for_x01_for_resource;
    }
    buffer_cursor = *buffer;
    if (buffer_cursor == 0) {
        buffer_cursor = 3;
    /* Shared target for core process flag one for SPI response context before updating working result; incoming paths preserve the same state assumptions. */
    core_process_flag_one_for_spi_response_context_before_updating_working_result:
        working_result = 0;
    } else if (buffer_cursor == 3) {
        buffer_cursor = 1;
        working_result = 1;
    } else {
        if (buffer_cursor == 1) {
            buffer_cursor = 2;
            goto core_process_flag_one_for_spi_response_context_before_updating_working_result;
        }
        if (buffer_cursor != 2)
            goto core_process_flag_one_for_spi_response_context_before_update_spi_set_logical_for_x01_for_resource;
        buffer_cursor = 0;
        working_result = 1;
    }
    *buffer = buffer_cursor;
    *(uint8_t *)(buffer + 1) = working_result;
/* Shared target for core process flag one for SPI response context before update SPI set logical for x01 for resource; incoming paths preserve the same state assumptions. */
core_process_flag_one_for_spi_response_context_before_update_spi_set_logical_for_x01_for_resource:
    core_update_spi_set_logical_for_x01_for_resource(buffer);
    return;
}

/**
 * @brief Get ti memset.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_ti_memset(buffer, value, length, flags, index)
uint32_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;
uint32_t index;

{
    /* Get ti memset using only caller-provided data. */
    int operation_status;

    operation_status = ((uint32_t (*)())core_dispatch_transfer_state_update)(buffer, 0, value, flags, flags & 0xffffff00);
    if (operation_status == 0) {
        operation_status = ((int (*)())core_transfer_maximum_payload_payload)(length, *buffer, flags, index, 0x28);
        if (operation_status == 0) {
            return 0;
        }
    } else {
        if (operation_status != 1) {
            return 0;
        }
        ti_memset(length, 0, 0x10);
    }
    return 1;
}

/**
 * @brief Transfer range end range start for descriptor descriptor resource.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_range_end_range_start_for_descriptor_descriptor_resource(context, range_start, range_end)
int context;
int range_start;
int range_end;

{
    /* Transfer range end range start for descriptor descriptor resource using only caller-provided data. */
    int condition_2_field;
    uint32_t *data_cursor;
    uint8_t *source_cursor;
    uint32_t operation_result;

    operation_result = 0;
    if (range_end < range_start) {
        core_halt_forever();
    }
    if (range_start < 0) {
        core_halt_forever();
    }
    if (3 < (uint32_t)(range_end - range_start)) {
        core_halt_forever();
    }
    if (range_start <= range_end) {
        condition_2_field = (range_end - range_start) + 1;
        data_cursor = &operation_result;
        source_cursor = (uint8_t *)(range_end + context);
        /* Repeat while condition_2_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            condition_2_field = condition_2_field + -1;
            *(uint8_t *)data_cursor = *source_cursor;
            data_cursor = (uint32_t *)((int)data_cursor + 1);
            source_cursor = source_cursor + -1;
        } while (condition_2_field != 0);
    }
    return operation_result;
}

/**
 * @brief Process watchdog reset.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_wdt_reset(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process watchdog reset using only caller-provided data. */
    uint32_t core_transfer_polls_remaining_channel_usb_poll_context_result;
    int mww_process_command_word_channel_mww_command_channel_result;
    int flags_snapshot;

    flags_snapshot = flags;
    mww_update_mww_transfer_storage(context, &flags_snapshot);
    if (*(int *)(flags_snapshot + 0x20) == 0) {
        core_transfer_polls_remaining_channel_usb_poll_context_result = 0;
    } else {
        mww_process_command_word_channel_mww_command_channel_result = mww_process_command_word_channel_mww_command_channel(context, *(int *)(flags_snapshot + 0x1c) << 0x10 | 0x908, 0, 0);
        if (mww_process_command_word_channel_mww_command_channel_result == 0) {
            *(uint32_t *)(flags_snapshot + 0x20) = 0;
        }
        wdt_reset();
        core_transfer_polls_remaining_channel_usb_poll_context_result = core_transfer_polls_remaining_channel_usb_poll_context(context, 10000);
    }
    return core_transfer_polls_remaining_channel_usb_poll_context_result;
}

/**
 * @brief Run requested available availability request.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_run_requested_available_availability_request(context, offset)
uint32_t context;
int offset;

{
    /* Run requested available availability request; persistent state is carried in core availability request. */
    uint8_t *core_availability_request_pointer;
    int operation_status;
    uint32_t status_bits;
    uint32_t requested_length;
    uint32_t available_length;
    bool condition_met;

    status_bits = *(uint32_t *)(offset + 0x14);
    requested_length = *(uint32_t *)(offset + 0x18);
    core_availability_request_pointer = (uint8_t *)&core_availability_request;
    operation_status = 0x10;
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
        condition_met = *(uint32_t *)(core_availability_request_pointer + 8) <= requested_length;
        if (requested_length == *(uint32_t *)(core_availability_request_pointer + 8)) {
            condition_met = *(uint32_t *)(core_availability_request_pointer + 4) <= status_bits;
        }
        if (condition_met) {
            available_length = *(uint32_t *)(core_availability_request_pointer + 0x10);
            condition_met = available_length <= requested_length;
            if (requested_length == available_length) {
                condition_met = *(uint32_t *)(core_availability_request_pointer + 0xc) <= status_bits;
            }
            if (!condition_met || requested_length == available_length && status_bits == *(uint32_t *)(core_availability_request_pointer + 0xc)) {
                /* Call core process end remaining through the defined unprototyped function-pointer signature. */
                ((void (*)())core_process_end_remaining)();
                return 1;
            }
        }
        operation_status = operation_status + -1;
        core_availability_request_pointer = core_availability_request_pointer + 0x94;
        if (operation_status == 0) {
            return 0;
        }
    } while (true);
}

/**
 * @brief Update ATA queue address ATA pending ATA queue context.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_update_ata_queue_address_ata_pending_ata_queue_context(condition) int condition;

{
    /* Update ATA queue address ATA pending ATA queue context; persistent state is carried in core ATA queue context. */
    if (condition != 0) {
        *(int *)(core_ata_queue_context * 4 + ATA_CALLBACK_QUEUE_ADDRESS) = condition;
        (&ata_callback_pending)[core_ata_queue_context] = 1;
        core_ata_queue_context = core_ata_queue_context + 1;
        if (0xf < core_ata_queue_context) {
            core_ata_queue_context = 0;
        }
    }
    return;
}

/**
 * @brief Update ATA address ATA queue context.
 *
 * @return No value.
 */
void core_update_ata_address_ata_queue_context()

{
    /* Update ATA address ATA queue context; persistent state is carried in core ATA queue context, ATA received set device bits error and related record fields. */
    int core_ata_queue_context_snapshot;
    uint8_t working_result;

    /* Set Device Bits FIS events have no command tag, so 0xfd marks the callback source. */
    core_ata_queue_context_snapshot = core_ata_queue_context * 0xc;
    *(uint8_t *)(core_ata_queue_context_snapshot + ATA_CALLBACK_DATA_ADDRESS) = 0xfd;
    working_result = ata_received_set_device_bits_error;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f522) = working_result;
    working_result = ata_received_set_device_bits_status;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f521) = working_result;
    core_update_ata_queue_address_ata_pending_ata_queue_context(ahci_command_completion_context);
    return;
}

/**
 * @brief Process clear memory secondary response buffer.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_process_clear_memory_secondary_response_buffer(offset) int offset;

{
    /* Process clear memory secondary response buffer; persistent state is carried in core secondary response buffer, core state 088 and related record fields. */
    ti_memset(offset, 0, 0xac);
    core_forward_end_remaining(offset + 8, &core_secondary_response_buffer, 8);
    core_forward_end_remaining(offset + 0x10, &core_state_088, 0x10);
    *(uint16_t *)(offset + 6) = core_state_086;
    *(uint16_t *)(offset + 4) = core_state_085;
    return;
}

/**
 * @brief Load flash configuration byte.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_load_flash_configuration_byte(offset, value, length, flags) int offset;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Load flash configuration byte using only caller-provided data. */
    int core_transfer_address_stack_transfer_address_derived;
    uint32_t status_bits;

    status_bits = flags;
    core_transfer_address_stack_transfer_address_derived = ((uint64_t (*)())core_transfer_address_stack_transfer_address)(3, 0x3f190, &status_bits, 1);
    if ((core_transfer_address_stack_transfer_address_derived == 0) && ((status_bits & 0x80) != 0x80)) {
        status_bits = (status_bits & 0xffffff00u) | ((uint8_t)status_bits & 3u);
        if ((uint8_t)status_bits == 1 || (uint8_t)status_bits == 2) {
            *(uint8_t *)(offset + 0xd) = (uint8_t)status_bits;
        }
    }
    return;
}

/**
 * @brief Update USB transfer state.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_update_usb_transfer_state()

{
    /* Update USB transfer state; persistent state is carried in core state 057, USB device state and related record fields. */
    uint8_t core_state_057_snapshot;

    core_state_057_snapshot = 0;
    if ((usb_device_state == 3) && (core_state_057_snapshot = 0, core_state_057 != '\0')) {
        core_state_057_snapshot = 2;
    }
    *core_response_buffer = core_state_057_snapshot;
    return 0;
}

/**
 * @brief Get descriptor.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_descriptor(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)

{
    /* Get descriptor using only caller-provided data. */
    uint8_t working_buffer[256];
    uint8_t descriptor_buffer[256];

    ((uint32_t (*)())core_handle_descriptor_or_descriptor_delta)(working_buffer, descriptor_buffer, value, length, flags, index);
    core_copy_word_buffer(context, descriptor_buffer, index);
    core_zero_word_buffer(descriptor_buffer, length);
    core_zero_word_buffer(working_buffer);
    return 0;
}
