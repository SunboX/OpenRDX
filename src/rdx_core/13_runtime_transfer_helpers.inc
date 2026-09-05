/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Multiply u32 to u64.
 *
 * @param product_words Procedure input.
 * @param multiplicand Procedure input.
 * @param multiplier Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_multiply_u32_to_u64(product_words, multiplicand, multiplier)
uint32_t *product_words;
uint32_t multiplicand;
uint32_t multiplier;

{
    /* Multiply u32 to u64 using only caller-provided data. */
    int high_product_carry;
    uint32_t product_low_word;
    uint32_t upper_lower_product;
    uint32_t shifted_cross_product;
    uint32_t cross_product_sum;

    upper_lower_product = (multiplier & 0xffff) * (multiplicand >> 0x10);
    cross_product_sum = (multiplier >> 0x10) * (multiplicand & 0xffff) + upper_lower_product;
    *product_words = (multiplicand & 0xffff) * (multiplier & 0xffff);
    shifted_cross_product = cross_product_sum * 0x10000;
    product_low_word = shifted_cross_product + *product_words;
    high_product_carry =
        (cross_product_sum >> 0x10) + (uint32_t)(cross_product_sum <= upper_lower_product && upper_lower_product - cross_product_sum != 0) * 0x10000;
    *product_words = product_low_word;
    if (product_low_word <= shifted_cross_product && shifted_cross_product - product_low_word != 0) {
        high_product_carry = high_product_carry + 1;
    }
    product_words[1] = (multiplier >> 0x10) * (multiplicand >> 0x10) + high_product_carry;
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
    /* Process clear memory buffer compare context; persistent state is carried in core record type 0d descriptor. */
    int operation_status;
    uint8_t operation_result;
    uint8_t working_buffer[32];
    uint8_t output_buffer[32];

    if (buffer == (uint8_t *)0x0) {
        buffer = working_buffer;
    }
    operation_status = core_process_remaining_descriptor_space_for_copy_copy_resource(context, buffer, 0x20, &core_record_type_0d_descriptor);
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
    /* Handle ATA standby context for cartridge monitor context; persistent state is carried in core mechanism output selection, core ata configuration loaded. */
    uint8_t core_mechanism_output_selection_snapshot;
    int operation_status;

    operation_status = core_is_mechanism_transition_pending();
    if (operation_status == 0) {
        core_increment_persistent_mechanism_completion_count();
        core_mark_secondary_sense_record_not_ready(0x800f29c);
        if (core_ata_configuration_loaded != '\0') {
            operation_status = core_get_runtime_context_base_for_build_scsi_mode();
            core_update_saturating_add_u32_for_ata_standby_context(operation_status + 0x2c);
        }
        core_handle_ata_standby_context();
        operation_status = core_get_response_type_value();
        core_mechanism_output_selection_snapshot = core_mechanism_output_selection;
        if (operation_status == 0) {
            core_mechanism_output_selection_snapshot = 0;
        }
        core_update_drive_mechanism_outputs(core_mechanism_output_selection_snapshot);
    }
    return;
}

/**
 * @brief Initialize cartridge mechanism inputs.
 *
 * @return No value.
 */
void core_initialize_cartridge_mechanism_inputs()

{
    /* Initialize cartridge mechanism inputs; persistent state is carried in core mechanism transition timer, core mechanism output state and related record fields. */
    int operation_status;

    /* Controller F2B4 uses logical outputs 6 then 7. Insertion and activity
     * use this pair as cartridge green then amber; physical
     * identity remains separate from the generic first/second state. */
    core_process_clear_memory_spi_response_context(
        0x800f2b4,
        RDX_CARTRIDGE_GREEN_LOGICAL_OUTPUT,
        RDX_CARTRIDGE_AMBER_LOGICAL_OUTPUT);
    gio_configure_logical_pin_interrupt(3, &gio_pin_three_callback_entry);
    core_allocate_software_timer(&core_mechanism_transition_timer);
    operation_status = core_forward_sci_read_logical();
    if (operation_status == 0) {
        core_mechanism_output_state = 8;
    } else {
        core_mechanism_output_state = 4;
    }
    core_allocate_software_timer(&core_mechanism_input_debounce_timer);
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
 * @brief Test transfer buffer memory.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_test_transfer_buffer_memory()

{
    /* Test transfer buffer memory using only caller-provided data. */
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
        core_clear_request_and_release_device_command(working_buffer);
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
    /* Process MWW saved request; persistent state is carried in core MWW saved request, usb bot cbw pointer. */
    int core_is_supported_forward_opcode_result;

    if (((core_mww_saved_request == 1) || (core_mww_saved_request == 2 || core_mww_saved_request == 3)) &&
        (core_is_supported_forward_opcode_result = core_is_supported_forward_opcode(*(uint8_t *)(usb_bot_cbw_pointer + 0xf)), core_is_supported_forward_opcode_result != 0)) {
        /* Call core process MWW saved request for MWW handle saved through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_mww_saved_request_for_mww_handle_saved)(1);
        core_process_request_for_mww_saved_request(*(uint8_t *)(usb_bot_cbw_pointer + 0xf), *(uint8_t *)(context + 2), *(uint8_t *)(context + 1));
        mww_reset_rw_offsets();
        mww_process_device_transfer_state(1);
    }
    return;
}

/**
 * @brief Submit USB control status trb.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_submit_usb_control_status_trb(condition) int condition;

{
    /* Submit USB control status trb; persistent state is carried in usb ep0 three stage transfer, usb data trb buffer pointer high address bias and related record fields. */
    int status_bits;
    int working_result;

    working_result = 0x11250;
    if (condition != 0) {
        working_result = 0x11210;
    }
    if (usb_ep0_three_stage_transfer == '\0') {
        status_bits = 3;
    } else {
        status_bits = 4;
    }
    *(uint32_t *)(working_result + -0x40000000) = 0;
    *(uint32_t *)(&usb_data_trb_buffer_pointer_high_address_bias + working_result) = 0;
    *(uint32_t *)(&usb_data_trb_status_address_bias + working_result) = 0;
    *(uint32_t *)(&usb_data_trb_control_address_bias + working_result) = status_bits << 4 | 3;
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
    /* Advance control transfer state; persistent state is carried in usb ep0 state, usb test mode. */
    if (usb_ep0_state == 1) {
        if (((int)buffer[2] < 1) || (buffer[3] != buffer[1])) {
            usb_ep0_state = 4;
        } else {
            core_update_mww_process_transfer(0x80, *buffer);
        }
    } else if (usb_ep0_state == 3) {
        if (usb_test_mode != 0) {
            core_set_usb_test_mode();
        }
        usb_ep0_state = 0;
    } else {
        usb_ep0_state = 5;
    }
    core_abort_failed_usb_control_transfer();
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
        core_read_buffer_bytes(value, operation_status, core_get_payload_offset_from_flags_result, context);
    }
    return operation_result;
}

/**
 * @brief Append little endian value.
 *
 * @param destination Procedure input.
 * @param encoded_value Procedure input.
 * @param byte_count Procedure input.
 * @param offset_tracker Procedure input.
 * @return No encoded_value.
 */
void core_append_little_endian_value(destination, encoded_value, byte_count, offset_tracker) int *destination;
uint32_t encoded_value;
uint32_t byte_count;
uint32_t *offset_tracker;

{
    /* Append little endian value using only caller-provided data. */
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
    if (operation_status < byte_count) {
        if (buffer_length <= entry_index) {
            byte_count = 0;
            goto core_bounded_word_write_emit_available_bytes;
        }
        byte_count = buffer_length - entry_index;
    }
    if (4 < byte_count) {
        byte_count = 4;
    }
/* Shared target for core bounded word write emit available bytes; incoming paths preserve the same state assumptions. */
core_bounded_word_write_emit_available_bytes:
    if (byte_count != 0) {
        data_cursor = (uint8_t *)(entry_index + *destination);
        entry_index = byte_count;
        /* Repeat while entry_index != 0; the body advances or polls the state needed to leave the loop. */
        do {
            *data_cursor = (char)encoded_value;
            entry_index = entry_index - 1;
            encoded_value = encoded_value >> 8;
            data_cursor = data_cursor + 1;
        } while (entry_index != 0);
    }
    *offset_tracker = byte_count + *offset_tracker;
    return;
}

/**
 * @brief Read big endian u32 field.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_big_endian_u32_field(context, range_start, range_end)
int context;
int range_start;
int range_end;

{
    /* Read big endian u32 field using only caller-provided data. */
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
        goto core_output_cycle_apply_logical_outputs;
    }
    buffer_cursor = *buffer;
    if (buffer_cursor == 0) {
        buffer_cursor = 3;
    /* Shared target for core output cycle select inactive flag; incoming paths preserve the same state assumptions. */
    core_output_cycle_select_inactive_flag:
        working_result = 0;
    } else if (buffer_cursor == 3) {
        buffer_cursor = 1;
        working_result = 1;
    } else {
        if (buffer_cursor == 1) {
            buffer_cursor = 2;
            goto core_output_cycle_select_inactive_flag;
        }
        if (buffer_cursor != 2)
            goto core_output_cycle_apply_logical_outputs;
        buffer_cursor = 0;
        working_result = 1;
    }
    *buffer = buffer_cursor;
    *(uint8_t *)(buffer + 1) = working_result;
/* Shared target for core output cycle apply logical outputs; incoming paths preserve the same state assumptions. */
core_output_cycle_apply_logical_outputs:
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
        operation_status = ((int (*)())core_find_typed_payload_with_minimum_length)(length, *buffer, flags, index, 0x28);
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
 * @brief Read big endian record field.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_big_endian_record_field(context, range_start, range_end)
int context;
int range_start;
int range_end;

{
    /* Read big endian record field using only caller-provided data. */
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
 * @brief End USB endpoint transfer.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_end_usb_endpoint_transfer(context, value, length, flags)
uint32_t context;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process watchdog reset using only caller-provided data. */
    uint32_t core_wait_usb_endpoint_command_result;
    int mww_send_usb_endpoint_command_result;
    int flags_snapshot;

    flags_snapshot = flags;
    mww_update_mww_transfer_storage(context, &flags_snapshot);
    if (*(int *)(flags_snapshot + 0x20) == 0) {
        core_wait_usb_endpoint_command_result = 0;
    } else {
        mww_send_usb_endpoint_command_result = mww_send_usb_endpoint_command(context, *(int *)(flags_snapshot + 0x1c) << 0x10 | 0x908, 0, 0);
        if (mww_send_usb_endpoint_command_result == 0) {
            *(uint32_t *)(flags_snapshot + 0x20) = 0;
        }
        wdt_reset();
        core_wait_usb_endpoint_command_result = core_wait_usb_endpoint_command(context, 10000);
    }
    return core_wait_usb_endpoint_command_result;
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
    /* Run requested available availability request; persistent state is carried in core capacity range descriptor table. */
    uint8_t *core_capacity_range_descriptor_table_pointer;
    int operation_status;
    uint32_t status_bits;
    uint32_t requested_length;
    uint32_t available_length;
    bool condition_met;

    status_bits = *(uint32_t *)(offset + 0x14);
    requested_length = *(uint32_t *)(offset + 0x18);
    core_capacity_range_descriptor_table_pointer = (uint8_t *)&core_capacity_range_descriptor_table;
    operation_status = 0x10;
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
        condition_met = *(uint32_t *)(core_capacity_range_descriptor_table_pointer + 8) <= requested_length;
        if (requested_length == *(uint32_t *)(core_capacity_range_descriptor_table_pointer + 8)) {
            condition_met = *(uint32_t *)(core_capacity_range_descriptor_table_pointer + 4) <= status_bits;
        }
        if (condition_met) {
            available_length = *(uint32_t *)(core_capacity_range_descriptor_table_pointer + 0x10);
            condition_met = available_length <= requested_length;
            if (requested_length == available_length) {
                condition_met = *(uint32_t *)(core_capacity_range_descriptor_table_pointer + 0xc) <= status_bits;
            }
            if (!condition_met || requested_length == available_length && status_bits == *(uint32_t *)(core_capacity_range_descriptor_table_pointer + 0xc)) {
                /* Call core process end remaining through the defined unprototyped function-pointer signature. */
                ((void (*)())core_process_end_remaining)();
                return 1;
            }
        }
        operation_status = operation_status + -1;
        core_capacity_range_descriptor_table_pointer = core_capacity_range_descriptor_table_pointer + 0x94;
        if (operation_status == 0) {
            return 0;
        }
    } while (true);
}

/**
 * @brief Enqueue ATA completion callback.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_enqueue_ata_completion_callback(condition) int condition;

{
    /* Enqueue ATA completion callback; persistent state is carried in core ata callback write index. */
    if (condition != 0) {
        *(int *)(core_ata_callback_write_index * 4 + ATA_CALLBACK_QUEUE_ADDRESS) = condition;
        (&ata_callback_pending)[core_ata_callback_write_index] = 1;
        core_ata_callback_write_index = core_ata_callback_write_index + 1;
        if (0xf < core_ata_callback_write_index) {
            core_ata_callback_write_index = 0;
        }
    }
    return;
}

/**
 * @brief Queue ATA set device bits completion.
 *
 * @return No value.
 */
void core_queue_ata_set_device_bits_completion()

{
    /* Queue ATA set device bits completion; persistent state is carried in core ata callback write index, ATA received set device bits error and related record fields. */
    int core_ata_callback_write_index_snapshot;
    uint8_t working_result;

    /* Set Device Bits FIS events have no command tag, so 0xfd marks the callback source. */
    core_ata_callback_write_index_snapshot = core_ata_callback_write_index * 0xc;
    *(uint8_t *)(core_ata_callback_write_index_snapshot + ATA_CALLBACK_DATA_ADDRESS) = 0xfd;
    working_result = ata_received_set_device_bits_error;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f522) = working_result;
    working_result = ata_received_set_device_bits_status;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f521) = working_result;
    core_enqueue_ata_completion_callback(ahci_command_completion_callback);
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
    /* Process clear memory secondary response buffer; persistent state is carried in core compact identity vendor identifier, core compact identity product identifier and related record fields. */
    ti_memset(offset, 0, 0xac);
    core_copy_bytes(offset + 8, &core_compact_identity_vendor_identifier, 8);
    core_copy_bytes(offset + 0x10, &core_compact_identity_product_identifier, 0x10);
    *(uint16_t *)(offset + 6) = core_compact_identity_offset_six_word;
    *(uint16_t *)(offset + 4) = core_compact_identity_offset_four_word;
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
    int core_execute_spi_flash_operation_derived;
    uint32_t status_bits;

    status_bits = flags;
    core_execute_spi_flash_operation_derived = ((uint64_t (*)())core_execute_spi_flash_operation)(3, 0x3f190, &status_bits, 1);
    if ((core_execute_spi_flash_operation_derived == 0) && ((status_bits & 0x80) != 0x80)) {
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
    /* Update USB transfer state; persistent state is carried in usb remote wakeup enabled, usb device speed and related record fields. */
    uint8_t usb_remote_wakeup_enabled_snapshot;

    usb_remote_wakeup_enabled_snapshot = 0;
    if ((usb_device_speed == 3) && (usb_remote_wakeup_enabled_snapshot = 0, usb_remote_wakeup_enabled != '\0')) {
        usb_remote_wakeup_enabled_snapshot = 2;
    }
    *core_response_buffer = usb_remote_wakeup_enabled_snapshot;
    return 0;
}

/**
 * @brief Reduce multiword modulo.
 *
 * @param remainder_output Procedure input.
 * @param dividend_words Procedure input.
 * @param dividend_word_count Procedure input.
 * @param modulus_words Procedure input.
 * @param modulus_word_count Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_reduce_multiword_modulo(uint32_t remainder_output, uint32_t dividend_words, uint32_t dividend_word_count, uint32_t modulus_words, uint32_t modulus_word_count)

{
    /* Reduce multiword modulo using only caller-provided data. */
    uint8_t quotient_workspace[256];
    uint8_t remainder_workspace[256];

    ((uint32_t (*)())core_divide_multiword_unsigned)(quotient_workspace, remainder_workspace, dividend_words, dividend_word_count, modulus_words, modulus_word_count);
    core_copy_word_buffer(remainder_output, remainder_workspace, modulus_word_count);
    core_zero_word_buffer(remainder_workspace, dividend_word_count);
    core_zero_word_buffer(quotient_workspace);
    return 0;
}
