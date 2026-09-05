/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Process word validity flag command validation context.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_word_validity_flag_command_validation_context(buffer)
uint32_t *buffer;

{
    /* Process word validity flag command validation context; persistent state is carried in core command validation context. */
    uint32_t *core_command_validation_context_snapshot;
    uint32_t operation_result;
    int status_bits;
    volatile uint8_t buffer_cursor = 0;
    volatile uint32_t input_word = 0;
    int operation_status;
    volatile uint8_t *volatile core_command_validation_context_pointer = 0;
    char validity_flag;

    ti_memset(&core_command_validation_context, 0, 4);
    status_bits = 0;
    validity_flag = '\0';
    core_command_validation_context_pointer = (volatile uint8_t *)&core_command_validation_context;
    operation_status = 0;
    input_word = *buffer;
    buffer_cursor = *(uint8_t *)((int)buffer + 0x11);
    core_command_validation_context = buffer;
    core_transfer_firmware_t_byte(&status_bits);
    core_command_validation_context_snapshot = (uint32_t *)core_command_validation_context;
    if (validity_flag == '\0') {
        if (status_bits == 0) {
            if (operation_status == 0) {
                operation_result = 0;
            } else {
                core_command_validation_context[2] = *(uint32_t *)(operation_status + 4);
                operation_result = 9;
                core_command_validation_context_snapshot[3] = (uint32_t)*(uint16_t *)(operation_status + 2);
            }
        } else {
            operation_result = 1;
        }
    } else {
        operation_result = 8;
    }
    return operation_result;
}

/**
 * @brief Transfer fourth byte byte.
 *
 * @param output_words Procedure input.
 * @param input_bytes Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_transfer_fourth_byte_byte(output_words, input_bytes, condition) int *output_words;
uint8_t *input_bytes;
uint32_t condition;

{
    /* Transfer fourth byte byte using only caller-provided data. */
    uint8_t buffer_2_cursor;
    uint8_t second_source_byte;
    uint8_t third_source_byte;
    uint8_t fourth_source_byte;
    uint8_t *reverse_input_cursor;
    int *buffer_1_cursor;
    int condition_field;

    if (1 < condition) {
        condition_field = (int)condition >> 1;
        buffer_1_cursor = output_words + condition;
        reverse_input_cursor = input_bytes + condition * 4;
        /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            buffer_1_cursor = buffer_1_cursor + -1;
            buffer_2_cursor = *input_bytes;
            second_source_byte = input_bytes[1];
            third_source_byte = input_bytes[2];
            fourth_source_byte = input_bytes[3];
            condition_field = condition_field + -1;
            input_bytes = input_bytes + 4;
            *output_words =
                (uint32_t)reverse_input_cursor[-1] +
                ((uint32_t)reverse_input_cursor[-2] + ((uint32_t)reverse_input_cursor[-3] + (uint32_t)reverse_input_cursor[-4] * 0x100) * 0x100) * 0x100;
            *buffer_1_cursor =
                (uint32_t)fourth_source_byte +
                ((uint32_t)third_source_byte + ((uint32_t)second_source_byte + (uint32_t)buffer_2_cursor * 0x100) * 0x100) * 0x100;
            output_words = output_words + 1;
            reverse_input_cursor = reverse_input_cursor + -4;
        } while (condition_field != 0);
    }
    return;
}

/**
 * @brief Process remaining descriptor space for transfer completion flag.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_process_remaining_descriptor_space_for_transfer_completion_flag(context, offset, length, flags, condition) uint32_t context;
int offset;
uint32_t length;
uint32_t flags;
uint32_t condition;

{
    /* Process remaining descriptor space for transfer completion flag using only caller-provided data. */
    uint32_t core_get_remaining_descriptor_space_result;

    core_get_remaining_descriptor_space_result = core_get_remaining_descriptor_space(offset);
    if (core_get_remaining_descriptor_space_result <= condition) {
        condition = core_get_remaining_descriptor_space_result;
    }
    core_write_descriptor_fields(context, offset, length);
    if ((*(uint8_t *)(offset + 1) & 1) != 0) {
        core_transfer_tracker(context, *(uint32_t *)(offset + 4), 4, length);
        core_transfer_tracker(context, *(uint32_t *)(offset + 8), 4);
    }
    core_transfer_bytes_remaining_descriptor(context, flags, condition, length);
    if (condition < core_get_remaining_descriptor_space_result) {
        core_transfer_bytes_remaining_descriptor(context, 0, core_get_remaining_descriptor_space_result - condition);
    }
    return;
}

/**
 * @brief Run expected flag expected type.
 *
 * @param expected_type Procedure input.
 * @param expected_flag Procedure input.
 * @param buffer Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_run_expected_flag_expected_type(expected_type, expected_flag, buffer, flags)
uint32_t expected_type;
uint32_t expected_flag;
uint8_t *buffer;
uint32_t flags;

{
    /* Run expected flag expected type using only caller-provided data. */
    uint32_t working_result;
    int operation_status;
    uint32_t status_bits;

    status_bits = 4;
    /* Iterate while true, preserving the defined cursor and bound. */
    while (true) {
        working_result = status_bits;
        core_configure_descriptor_payload(buffer, &status_bits, flags);
        operation_status = core_get_descriptor_requested(working_result, buffer);
        if (operation_status == 0) {
            return 6;
        }
        if ((((buffer[1] & 5) != 0) == expected_flag) && (expected_type == *buffer))
            break;
        operation_status = core_validate_transfer_capacity(buffer, status_bits, flags);
        if (operation_status != 0) {
            return 6;
        }
    }
    *(uint32_t *)(buffer + 0x14) = working_result;
    return 0;
}

/**
 * @brief Transfer retries remaining ti memset.
 *
 * @param offset Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_retries_remaining_ti_memset(offset, condition)
int offset;
uint32_t condition;

{
    /* Transfer retries remaining ti memset using only caller-provided data. */
    char status_bits;
    int operation_status;
    char *data_cursor;
    int retries_remaining;
    uint32_t operation_result;

    ti_memset(offset + 0x10, 0, 0x34);
    data_cursor = "\x02\x17";
    retries_remaining = 10;
    operation_result = 0;
    /* Repeat while retries_remaining != 0; the body advances or polls the state needed to leave the loop. */
    do {
        operation_status = core_get_expected_candidate(condition, data_cursor);
        if ((operation_status != 0) ||
            (operation_status = core_process_x01_for_retries_remaining_ti(offset + 0x10, data_cursor, condition), operation_status != 0)) {
            *(uint8_t *)(offset + 0x10 + (uint32_t)*(uint16_t *)(data_cursor + 4)) = 3;
            status_bits = *data_cursor;
            if ((status_bits == '\x04') || (status_bits == '/' || status_bits == '\x02')) {
                operation_result = 0xc;
            }
        }
        retries_remaining = retries_remaining + -1;
        data_cursor = data_cursor + 8;
    } while (retries_remaining != 0);
    return operation_result;
}

/**
 * @brief Process ti memset copy.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param source_offset Procedure input.
 * @param copy_length Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_ti_memset_copy(buffer, value, length, source_offset, copy_length)
int *buffer;
uint32_t value;
uint32_t length;
int source_offset;
uint32_t copy_length;

{
    /* Process ti memset copy using only caller-provided data. */
    int operation_result;
    uint32_t core_get_remaining_descriptor_space_result;
    int core_get_payload_offset_from_flags_result;

    operation_result = core_process_expected_candidate();
    if (operation_result == 0) {
        core_get_remaining_descriptor_space_result = core_get_remaining_descriptor_space(length);
        if (core_get_remaining_descriptor_space_result < copy_length) {
            copy_length = core_get_remaining_descriptor_space_result;
        }
        if (source_offset == 0) {
            if (copy_length != 0) {
                return 0xb;
            }
        } else {
            core_get_payload_offset_from_flags_result = core_get_payload_offset_from_flags(length);
            core_forward_end_remaining(core_get_payload_offset_from_flags_result + *buffer, source_offset, copy_length);
        }
        if (core_get_remaining_descriptor_space_result != copy_length) {
            core_get_payload_offset_from_flags_result = core_get_payload_offset_from_flags(length);
            /* Process ti memset copy; this branch selects whether its associated handler runs. */
            ti_memset(*buffer + copy_length + core_get_payload_offset_from_flags_result, 0, core_get_remaining_descriptor_space_result - copy_length);
        }
    }
    return operation_result;
}

/**
 * @brief Handle structure field hex.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_structure_field_hex(buffer, value, condition, offset)
uint32_t *buffer;
uint32_t value;
int condition;
int offset;

{
    /* Handle structure field hex using only caller-provided data. */
    int operation_status;
    uint32_t core_get_structure_field_hex_4c_value_result;
    uint32_t buffer_cursor;

    operation_status = core_update_structure_field_hex(buffer, value, offset);
    if (operation_status == 0) {
        core_get_structure_field_hex_4c_value_result = core_get_structure_field_hex_4c_value(offset);
        buffer_cursor = *buffer;
        *buffer = core_get_structure_field_hex_4c_value_result + buffer_cursor;
        buffer[1] = buffer[1] + (uint32_t)CARRY4(core_get_structure_field_hex_4c_value_result, buffer_cursor);
        if (condition == 0) {
            operation_status = core_update_constant_0x12_x01(offset);
        } else {
            operation_status = core_update_availability_x01(offset);
        }
        if (operation_status != 0) {
            if ((condition != 0) && (operation_status = core_update_x01_x02(offset), operation_status != 0)) {
                *(uint8_t *)(offset + 0x10) = 2;
                *(uint32_t *)(offset + 0x1c) = 1;
                operation_status = core_accumulate_split_transfer_offsets(offset);
                if (operation_status == 0) {
                    *(uint8_t *)(offset + 0x70) = 1;
                }
            }
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Read mcp3008 channel.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_read_mcp3008_channel(context, buffer, length, flags)
uint32_t context;
int16_t *buffer;
uint32_t length;
uint32_t flags;

{
    /* Read mcp3008 channel using only caller-provided data. */
    int operation_result;
    uint32_t flags_snapshot;

    *buffer = 0;
    flags_snapshot = flags;
    /* Claim/configure the shared SPI path before clocking the MCP3008 transaction. */
    operation_result = core_forward_mcp3008_sample_value();
    if (operation_result == 0) {
        operation_result = spi_transfer_word(0x11050001, &flags_snapshot);
        if ((operation_result == 0) &&
            (operation_result = spi_transfer_word((context & 7) << 4 | 0x11050080, &flags_snapshot), operation_result == 0)) {
            /* The converter's ten-bit result arrives as two low bits followed by eight data bits. */
            *buffer = (int16_t)((flags_snapshot & 3) << 8);
            operation_result = spi_transfer_word(0x1050000, &flags_snapshot);
            if (operation_result == 0) {
                *buffer = ((uint16_t)flags_snapshot & 0xff) + *buffer;
            }
        }
    } else {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Process command opcode transfer size atapi device flag.
 *
 * @param condition Procedure input.
 * @return No value.
 */
uint32_t core_process_command_opcode_transfer_size_atapi_device_flag(condition)
int condition;

{
    /* Process command opcode transfer size atapi device flag; persistent state is carried in core atapi device flag. */
    int operation_status;
    uint8_t status_bits;
    volatile uint8_t working_result = 0;
    volatile uint8_t command_opcode = 0;
    volatile uint32_t transfer_size = 0;
    volatile uint8_t valid_flag = 0;

    ti_memset(&status_bits, 0, 0x30);
    /* Build an ATA IDENTIFY DEVICE or IDENTIFY PACKET DEVICE command from the device type. */
    status_bits = 0x27;
    working_result = 0x80;
    if ((&core_atapi_device_flag)[condition * 0x1d0] == '\0') {
        command_opcode = 0xec;
    } else {
        command_opcode = 0xa1;
    }
    valid_flag = 1;
    transfer_size = 0x200;
    /* The identify payload is always a single 512-byte sector. */
    operation_status = ((uint32_t (*)())core_submit_command_descriptor_to_device)(condition, &status_bits, 0);
    if (operation_status == 0) {
        core_update_ahci_control_register(condition, 0, 0);
    }
    return 0;
}

/**
 * @brief Program SPI flash pages.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
int core_program_spi_flash_pages(context, condition, length)
uint32_t context;
uint32_t condition;
int length;

{
    /* Program SPI flash pages using only caller-provided data. */
    uint32_t operation_status;
    int operation_result;

    operation_result = 0;
    /* Iterate while (condition != 0 && (operation_result == 0)), preserving the defined cursor and bound. */
    while ((condition != 0 && (operation_result == 0))) {
        /* Never cross a 256-byte SPI-flash page boundary in one page-program operation. */
        operation_status = 0x100 - (context & 0xff) & 0xffff;
        if (condition < operation_status) {
            operation_status = condition & 0xffff;
        }
        core_set_transfer_interrupt_enable_mask();
        /* Issue write-enable before every page because flash clears WEL after programming. */
        operation_result = core_transfer_address_stack_transfer_address(6, 0, 0, 0);
        if (operation_result == 0) {
            operation_result = core_transfer_address_stack_transfer_address(2, context, length, operation_status);
        }
        core_update_spi_transfer_context_for_program_spi_flash(1);
        /* Preserve the defined two-millisecond settling delay between page programs. */
        rti_delay_microseconds_with_watchdog(2000);
        context = operation_status + context;
        condition = condition - operation_status;
        length = length + operation_status;
    }
    return operation_result;
}

/**
 * @brief Build supported record types response.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_supported_record_types_response(context)
uint32_t context;

{
    /* Build supported record types response using only caller-provided data. */
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;

    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 0x12);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 2, 3, 0xe);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 6, 7, 1);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 8, 9, 0x10);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 10, 0xb, 0x11);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0xc, 0xd, 0x12);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0xe, 0xf, 0x20);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x10, 0x11, 0x21);
    return 0;
}

/**
 * @brief Transition firmware context 0x14 firmware record context.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_transition_firmware_context_0x14_firmware_record_context(offset) int offset;

{
    /* Transition firmware context 0x14 firmware record context; persistent state is carried in core firmware record context, core state 029. */
    int *offset_field_0x14;
    int operation_status;

    offset_field_0x14 = *(int **)(offset + 0x14);
    if (offset_field_0x14 == (int *)0x0) {
        return;
    }
    operation_status = *offset_field_0x14;
    if (operation_status == 0) {
        return;
    }
    if (operation_status != 7) {
        if (operation_status == 8) {
            operation_status = 9;
            goto core_transition_firmware_context_0x14_firmware_record_context_return_path;
        }
        if (operation_status == 9) {
            operation_status = 10;
            goto core_transition_firmware_context_0x14_firmware_record_context_return_path;
        }
        if (operation_status == 10) {
            operation_status = 1;
            goto core_transition_firmware_context_0x14_firmware_record_context_return_path;
        }
        if (operation_status == 1) {
            operation_status = 2;
            goto core_transition_firmware_context_0x14_firmware_record_context_return_path;
        }
        if (operation_status != 3) {
            if (operation_status != 6) {
                return;
            }
            *offset_field_0x14 = 0;
            if (6 < *(uint8_t *)(offset_field_0x14 + 1)) {
                return;
            }
            operation_status = (uint32_t)*(uint8_t *)(offset_field_0x14 + 1) * 8;
            if ((&core_firmware_record_context)[operation_status] == '\0') {
                return;
            }
            /* Invoke the callback selected by the active queue entry. */
            if (*(firmware_callback_t **)(&core_state_029 + operation_status) == (firmware_callback_t *)0x0) {
                return;
            }
            /* Invoke the callback selected by the active queue entry. */
            (**(firmware_callback_t **)(&core_state_029 + operation_status))();
            return;
        }
    }
    operation_status = 8;
/* Shared target for core transition firmware context 0x14 firmware record context return path; incoming paths preserve the same state assumptions. */
core_transition_firmware_context_0x14_firmware_record_context_return_path:
    *offset_field_0x14 = operation_status;
    return;
}

/**
 * @brief Copy reversed field bytes.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
void core_copy_reversed_field_bytes(context, range_start, range_end, flags, index) int context;
int range_start;
int range_end;
uint32_t flags;
uint32_t index;

{
    /* Copy reversed field bytes using only caller-provided data. */
    char b_derived;
    uint32_t condition_2_field;
    int copy_count;
    uint32_t *data_cursor;
    uint8_t *clear_cursor;
    uint32_t flags_snapshot;
    volatile uint32_t index_derived = 0;

    flags_snapshot = flags;
    index_derived = index;
    if (range_end < range_start) {
        core_halt_forever();
    }
    if (range_start < 0) {
        core_halt_forever();
    }
    condition_2_field = (range_end - range_start) + 1;
    if (condition_2_field < 9) {
        b_derived = (char)condition_2_field;
    } else {
        b_derived = '\b';
    }
    if (range_start <= range_end) {
        copy_count = (range_end - range_start) + 1;
        data_cursor = &flags_snapshot;
        /* Repeat while copy_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if (b_derived < '\x01')
                break;
            b_derived = b_derived + -1;
            copy_count = copy_count + -1;
            *(uint8_t *)(context + range_end) = *(uint8_t *)data_cursor;
            range_end = range_end + -1;
            data_cursor = (uint32_t *)((int)data_cursor + 1);
        } while (copy_count != 0);
    }
    if (range_start <= range_end) {
        copy_count = (range_end - range_start) + 1;
        clear_cursor = (uint8_t *)(range_end + context);
        /* Repeat while copy_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            copy_count = copy_count + -1;
            *clear_cursor = 0;
            clear_cursor = clear_cursor + -1;
        } while (copy_count != 0);
    }
    return;
}

/**
 * @brief Dispatch firmware core work according to the decoded request or state value.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_dispatch_response_buffer(condition)
int condition;

{
    /* Dispatch response buffer; persistent state is carried in core endpoint context, core response buffer and related record fields. */
    uint32_t operation_result;
    bool core_endpoint_context_snapshot;

    /* Dispatch on condition; each case preserves the defined state-machine ordering. */
    switch (condition) {
    case 0:
        *core_response_buffer = 0;
        core_response_buffer[1] = 0;
        if ((core_endpoint_context != 2) && (core_endpoint_context != 3)) {
            return 1;
        }
        operation_result = core_update_usb_transfer_state_for_response_buffer();
        if (operation_result == 0) {
            /* Call core process component descriptor component context through the defined unprototyped function-pointer signature. */
            ((void (*)())core_process_component_descriptor_component_context)(0x80, core_response_buffer, 2);
        }
        return operation_result;
    case 1:
    case 3:
        break;
    case 2:
    case 4:
        return 1;
    case 5:
        core_update_command_status_update(0);
        core_state_059 = 0;
        core_endpoint_context = 1;
        return 0;
    case 6:
        return 3;
    case 7:
    case 0xc:
        return 3;
    case 8:
        *core_response_buffer = core_state_059;
        /* Call core process component descriptor component context through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_component_descriptor_component_context)(0x80, core_response_buffer, 1);
        return 0;
    case 9:
        core_endpoint_context_snapshot = 1 < core_endpoint_context;
        if (core_endpoint_context_snapshot) {
            core_endpoint_context = 2;
            core_state_059 = 0;
            core_state_060 = 0;
            core_state_061 = 0;
        }
        return (uint32_t)!core_endpoint_context_snapshot;
    case 10:
        if (core_endpoint_context != 3) {
            return 1;
        }
        *core_response_buffer = core_state_060;
        /* Call core process component descriptor component context through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_component_descriptor_component_context)(0x80, core_response_buffer, 1);
        return 0;
    case 0xb:
        core_endpoint_context_snapshot = core_endpoint_context == 3;
        if (core_endpoint_context_snapshot) {
            core_transfer_usb_endpoint_transfer_for_response_buffer(1);
            core_state_060 = 0;
        }
        return (uint32_t)!core_endpoint_context_snapshot;
    default:
        if (condition == 0x30) {
            if (core_endpoint_context < 2) {
                operation_result = 1;
            } else {
                if (usb_device_state == 3) {
                    core_update_firmware_context_secondary_firmware_callback(&usb_secondary_firmware_entry);
                    /* Call core process component descriptor component context through the defined unprototyped function-pointer signature. */
                    ((void (*)())core_process_component_descriptor_component_context)(0, &usb_ep0_buffer, 0);
                    return 0;
                }
                operation_result = 3;
            }
            return operation_result;
        }
        if (condition != 0x31) {
            return 1;
        }
        if (usb_device_state == 3) {
            return 0;
        }
        return 3;
    }
    if (usb_device_state != 3) {
        return 3;
    }
    if (core_endpoint_context == 3) {
        return 3;
    }
    return 1;
}

/**
 * @brief Transition range word range word.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transition_range_word_range_word(buffer, value, length, offset)
uint32_t *buffer;
uint32_t value;
int length;
int offset;

{
    /* Transition range word range word using only caller-provided data. */
    char operation_status;
    int core_update_structure_field_hex_result;
    uint32_t range_high_word;
    int comparison_mode = 0;
    int callback_result = 0;
    bool condition_met;
    uint64_t core_get_structure_field_hex_4c_value_result;
    uint64_t range_value;
    int status_bits;
    volatile int range_low_word = 0;

    status_bits = length;
    range_low_word = offset;
    core_get_structure_field_hex_4c_value_result = core_get_structure_field_hex_4c_value(offset);
    status_bits = *buffer - (uint32_t)core_get_structure_field_hex_4c_value_result;
    /* Transition range word range word; this branch selects whether its associated handler runs. */
    range_low_word = buffer[1] - (uint32_t)(*buffer < (uint32_t)core_get_structure_field_hex_4c_value_result);
    if (comparison_mode == 0) {
        operation_status = *(char *)(offset + 8);
    } else {
        operation_status = *(char *)(callback_result + 9);
    }
    if (operation_status == '\0') {
        range_value = core_get_structure_field_hex_4c_value(offset);
        range_high_word = (uint32_t)((uint64_t)range_value >> 0x20);
        condition_met = range_high_word <= buffer[1];
        if (buffer[1] == range_high_word) {
            condition_met = (uint32_t)range_value <= *buffer;
        }
        if (!condition_met) {
            return 0;
        }
        core_update_structure_field_hex_result = core_update_structure_field_hex(&status_bits, (int)((uint64_t)core_get_structure_field_hex_4c_value_result >> 0x20), offset);
        if (core_update_structure_field_hex_result != 0) {
            return 0;
        }
        if (*(int *)(offset + 0x44) != 2) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Process device use lookup.
 *
 * @param buffer Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param fallback_enabled Procedure input.
 * @param use_lookup Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_device_use_lookup(buffer, offset, length, fallback_enabled, use_lookup)
int *buffer;
int offset;
uint32_t length;
int fallback_enabled;
char use_lookup;

{
    /* Process device use lookup using only caller-provided data. */
    int operation_status;
    uint32_t operation_result;
    int device_index;

    device_index = buffer[8];
    operation_status = core_validate_combined_record_bounds(offset);
    if (operation_status == 0) {
        operation_result = 4;
    } else {
        operation_status = core_update_transfer(buffer, offset);
        operation_result = 2;
        if (operation_status == 0) {
            core_forward_end_remaining(*(int *)(offset + 4) + *buffer, length, *(uint32_t *)(offset + 8));
            if (use_lookup == '\0') {
                operation_status = ((int (*)())core_handle_fallback_decoded_request_bytes)(device_index, buffer + 3, buffer);
                if (operation_status == 0) {
                    operation_result = 0;
                }
            } else if (fallback_enabled != 0) {
                operation_result = 3;
            }
        }
    }
    return operation_result;
}

/**
 * @brief Handle metadata type.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param type_output Procedure input.
 * @param flags_output Procedure input.
 * @return Result produced by the procedure.
 */
int core_handle_metadata_type(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t *type_output, uint32_t *flags_output)

{
    /* Handle metadata type using only caller-provided data. */
    int core_update_state_constant_137_three_result;
    int operation_status;
    uint32_t core_get_0xc_for_device_descriptor_response_result;
    uint8_t working_buffer[16];
    uint8_t entry_metadata[12];

    core_update_state_constant_137_three_result = core_update_state_constant_137_three(context, value, flags_output);
    if (core_update_state_constant_137_three_result != 0) {
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, flags);
        if (operation_status == 0) {
            core_get_0xc_for_device_descriptor_response_result = 0x21;
        } else {
            operation_status = core_validate_device_descriptor_length(flags, length, working_buffer, entry_metadata);
            if ((operation_status != 0) && (operation_status = core_has_device_descriptor_high_bit(working_buffer), operation_status != 0)) {
                if (type_output != (uint32_t *)0x0) {
                    core_get_0xc_for_device_descriptor_response_result = core_get_0xc_for_device_descriptor_response(working_buffer);
                    *type_output = core_get_0xc_for_device_descriptor_response_result;
                }
                return 1;
            }
            core_get_0xc_for_device_descriptor_response_result = 0x29;
        }
        *flags_output = core_get_0xc_for_device_descriptor_response_result;
    }
    return core_update_state_constant_137_three_result;
}

