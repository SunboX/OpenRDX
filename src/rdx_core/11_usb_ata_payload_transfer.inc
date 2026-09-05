/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Write big endian u32 field.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
uint32_t core_write_big_endian_u32_field(context, range_start, range_end, flags)
int context;
int range_start;
int range_end;
uint32_t flags;

{
    /* Write big endian u32 field using only caller-provided data. */
    char operation_status;
    uint32_t condition_2_field;
    int copy_count;
    uint32_t *data_cursor;
    uint8_t *clear_cursor;
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    if (range_end < range_start) {
        core_halt_forever();
    }
    if (range_start < 0) {
        core_halt_forever();
    }
    condition_2_field = (range_end - range_start) + 1;
    if (condition_2_field < 5) {
        operation_status = (char)condition_2_field;
    } else {
        operation_status = '\x04';
    }
    if (range_start <= range_end) {
        copy_count = (range_end - range_start) + 1;
        data_cursor = &flags_snapshot;
        /* Repeat while copy_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if (operation_status < '\x01')
                break;
            operation_status = operation_status + -1;
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
    return 0;
}

/**
 * @brief Submit ATA sata feature control.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_submit_ata_sata_feature_control(context, condition) uint32_t context;
int condition;

{
    /* Submit ATA sata feature control using only caller-provided data. */
    int operation_status;
    uint8_t status_bits;
    volatile uint8_t working_result = 0;
    volatile uint8_t command_opcode = 0;
    volatile uint8_t configuration_flags = 0;
    volatile uint8_t response_length = 0;
    volatile uint8_t valid_flag = 0;

    ti_memset(&status_bits, 0, 0x30);
    status_bits = 0x27;
    working_result = 0x80;
    command_opcode = 0xef;
    if (condition == 0) {
        configuration_flags = 0x90;
    } else {
        configuration_flags = 0x10;
    }
    valid_flag = 1;
    response_length = 2;
    operation_status = ((uint32_t (*)())core_submit_command_descriptor_to_device)(context, &status_bits, 0);
    if (operation_status == 0) {
        core_issue_ahci_command_slot(context, 0, 0);
    }
    return;
}

/**
 * @brief Divide multiword by word.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param divisor Procedure input.
 * @param dividend_word_count Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_divide_multiword_by_word(offset, value, divisor, dividend_word_count)
int offset;
uint32_t value;
uint32_t divisor;
int dividend_word_count;

{
    /* Divide multiword by word using only caller-provided data. */
    uint32_t status_bits;
    int operation_status;
    uint32_t *computed_offset;
    uint32_t remaining_count;
    uint32_t high_result;
    uint32_t condition_1_field;
    volatile uint32_t condition_2_field = 0;

    if (dividend_word_count == 0 || divisor == 0) {
        status_bits = 0;
        high_result = value;
    } else {
        operation_status = 0x20;
        status_bits = 0x80000000;
        remaining_count = 0;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((status_bits & divisor) != 0)
                break;
            operation_status = operation_status + -1;
            remaining_count = remaining_count + 1;
            status_bits = status_bits >> 1;
        } while (operation_status != 0);
        high_result = value;
        condition_1_field = divisor;
        condition_2_field = dividend_word_count;
        high_result = core_shift_multiword_left(offset, value, remaining_count);
        computed_offset = (uint32_t *)(offset + dividend_word_count * 4);
        /* Repeat while dividend_word_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            computed_offset = computed_offset + -1;
            condition_1_field = *computed_offset;
            condition_2_field = high_result;
            core_divide_doubleword_by_word(computed_offset, &high_result, &condition_1_field, divisor << (remaining_count & 0xff));
            dividend_word_count = dividend_word_count + -1;
        } while (dividend_word_count != 0);
        status_bits = high_result >> (remaining_count & 0xff);
        high_result = status_bits;
    }
    return CONCAT44(high_result, status_bits);
}

/**
 * @brief Process runtime context base for component.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_runtime_context_base_for_component(context, condition, offset)
uint8_t context;
uint32_t condition;
int offset;

{
    /* Process runtime context base for component using only caller-provided data. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint32_t working_result;

    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_clear_secondary_descriptor_mask(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, 0x400000);
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_clear_primary_descriptor_mask(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, 0x400000);
    if ((condition >> 1 & 1) != 0) {
        core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode(condition >> 2);
        core_set_secondary_descriptor_mask(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, 0x400000);
    }
    if ((condition & 1) != 0) {
        core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode(condition >> 1);
        core_set_primary_descriptor_mask(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, 0x400000);
    }
    if ((condition >> 5 & 1) == 0) {
        working_result = 3;
    } else {
        working_result = 2;
    }
    *(uint32_t *)(offset + 4) = working_result;
    *(uint8_t *)(offset + 8) = context;
    *(uint32_t *)(offset + 0xc) = condition;
    return 0;
}

/**
 * @brief Submit ATA set transfer mode.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
uint32_t core_submit_ata_set_transfer_mode(context, value)
uint32_t context;
uint32_t value;

{
    /* Submit ATA set transfer mode using only caller-provided data. */
    int operation_status;
    uint8_t status_bits;
    volatile uint8_t working_result = 0;
    volatile uint8_t command_opcode = 0;
    volatile uint8_t command_code = 0;
    volatile uint8_t core_select_ata_transfer_mode_result = 0;
    volatile uint8_t valid_flag = 0;

    ti_memset(&status_bits, 0, 0x30);
    status_bits = 0x27;
    command_opcode = 0xef;
    working_result = 0x80;
    command_code = 3;
    core_select_ata_transfer_mode_result = core_select_ata_transfer_mode(context, value);
    valid_flag = 1;
    operation_status = ((uint32_t (*)())core_submit_command_descriptor_to_device)(context, &status_bits, 0);
    if (operation_status == 0) {
        core_issue_ahci_command_slot(context, 0, 0);
    }
    return 0;
}

/**
 * @brief Handle cartridge input state for monitor cartridge input.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_handle_cartridge_input_state_for_monitor_cartridge_input(condition) int condition;

{
    /* Handle cartridge input state for monitor cartridge input; persistent state is carried in core cartridge link retry count, core cartridge input state and related record fields. */
    uint32_t core_cartridge_link_retry_count_snapshot;
    uint32_t core_get_configured_delay_ms_result;

    core_cartridge_input_state = condition;
    if (condition == 0) {
        core_handle_cartridge_input_state();
        return;
    }
    if (condition == 1) {
        core_update_store_context_enable(1);
        core_cartridge_link_retry_count_snapshot = 0;
    /* Shared target for core cartridge link arm retry timer; incoming paths preserve the same state assumptions. */
    core_cartridge_link_arm_retry_timer:
        core_get_configured_delay_ms_result = core_get_configured_delay_ms(core_cartridge_link_retry_count_snapshot);
        rti_set_software_timer(&core_cartridge_ready_retry_timer, core_get_configured_delay_ms_result);
        return;
    }
    if (condition == 2) {
        core_handle_clear_memory_lookup_ata_command_buffer();
        return;
    }
    if (condition != 3) {
        if (condition == 4) {
            core_cartridge_link_retry_count_snapshot = core_cartridge_link_retry_count + 1U & 0xff;
            goto core_cartridge_link_arm_retry_timer;
        }
        if (condition != 5) {
            return;
        }
        core_set_command_phase_to_two();
    }
    core_restart_ahci_port_initialization();
    return;
}

/**
 * @brief Process end remaining.
 *
 * @param destination_record Procedure input.
 * @param source_record Procedure input.
 * @return No value.
 */
void core_process_end_remaining(destination_record, source_record) uint32_t *destination_record;
uint32_t *source_record;

{
    /* Process end remaining using only caller-provided data. */
    uint32_t copied_header_word;

    *destination_record = *source_record;
    copied_header_word = source_record[2];
    destination_record[1] = source_record[1];
    destination_record[2] = copied_header_word;
    copied_header_word = source_record[4];
    destination_record[3] = source_record[3];
    destination_record[4] = copied_header_word;
    core_copy_bytes(destination_record + 0x25, source_record + 5, 0x80);
    core_copy_bytes(destination_record + 5, &core_validation_record_fixed_operand_template, 0x80);
    core_process_clear_memory_clear_memory_copy_context(destination_record + 5, 0x80);
    core_process_clear_memory_clear_memory_copy_context(destination_record + 0x25, 0x80);
    return;
}

/**
 * @brief Process descriptor type ti memset.
 *
 * @param destination_record Procedure input.
 * @param source_record Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_descriptor_type_ti_memset(destination_record, source_record)
int destination_record;
int source_record;

{
    /* Process descriptor type ti memset using only caller-provided data. */
    uint16_t core_read_big_endian_u32_field_result;
    uint32_t descriptor_type;
    uint32_t offset_1_field_0x24;
    uint32_t operation_result;

    ti_memset(destination_record + 0x24, 0, 0x108);
    operation_result = 0;
    if (source_record != 0) {
        core_read_big_endian_u32_field_result = core_read_big_endian_u32_field(source_record, 0, 1);
        *(uint16_t *)(destination_record + 0x24) = core_read_big_endian_u32_field_result;
        descriptor_type = core_read_big_endian_u32_field(source_record, 2, 2);
        offset_1_field_0x24 = (uint32_t)*(uint16_t *)(destination_record + 0x24);
        *(uint32_t *)(destination_record + 0x28) = descriptor_type;
        if ((8 < offset_1_field_0x24) && (offset_1_field_0x24 < 0x109)) {
            core_copy_bytes(destination_record + 0x2c, source_record + 8, offset_1_field_0x24 - 8);
            core_sha256_initialize(destination_record + 300);
            operation_result = 1;
        }
    }
    return operation_result;
}

/**
 * @brief Find USB descriptor.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_find_usb_descriptor(condition)
uint32_t condition;

{
    /* Find USB descriptor using only caller-provided data. */
    uint8_t *data_cursor;
    uint32_t *length_output;
    int callback_result;
    int entry_index;
    uint32_t operation_status;
    uint64_t usb_get_device_descriptor_result;

    usb_get_device_descriptor_result = (uint64_t)usb_get_device_descriptor();
    length_output = (uint32_t *)(uint32_t)((uint64_t)usb_get_device_descriptor_result >> 0x20);
    data_cursor = (uint8_t *)(uint32_t)usb_get_device_descriptor_result;
    operation_status = (uint32_t)*data_cursor;
    if (operation_status != 0) {
        entry_index = 0;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((condition == data_cursor[1]) && (entry_index = entry_index + 1, entry_index - callback_result == 1)) {
            /* Shared target for core usb descriptor return matching entry; incoming paths preserve the same state assumptions. */
            core_usb_descriptor_return_matching_entry:
                if (length_output != (uint32_t *)0x0) {
                    *length_output = (uint32_t)data_cursor;
                }
                return 0;
            }
            if (data_cursor[1] == 0x21) {
                if ((condition == 0x22) && (entry_index = entry_index + 1, entry_index - callback_result == 1)) {
                    data_cursor = data_cursor + operation_status;
                    goto core_usb_descriptor_return_matching_entry;
                }
                data_cursor = data_cursor + *(uint16_t *)(data_cursor + 7);
            }
            data_cursor = data_cursor + operation_status;
            operation_status = (uint32_t)*data_cursor;
        } while (operation_status != 0);
    }
    return 1;
}

/**
 * @brief Process response flag x01.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_process_response_flag_x01(buffer) uint32_t *buffer;

{
    /* Process response flag x01 using only caller-provided data. */
    int operation_status;
    uint32_t buffer_cursor;

    if (*(char *)((int)buffer + 9) == '\0') {
        if ((char)buffer[2] == '\0') {
            *(uint8_t *)(buffer + 1) = *(uint8_t *)((int)buffer + 7);
            operation_status = core_transition_response_flag_x01(buffer);
            if (operation_status != 0) {
                return;
            }
        } else {
            *(char *)(buffer + 2) = (char)buffer[2] + -1;
            *(bool *)(buffer + 1) = (char)buffer[1] == '\0';
            operation_status = core_update_response_flag(buffer);
            if (operation_status != 0) {
                return;
            }
        }
        buffer_cursor = (uint32_t)(*(char *)((int)buffer + 5) != '\0');
    } else {
        *(char *)((int)buffer + 9) = *(char *)((int)buffer + 9) + -1;
        *(bool *)(buffer + 1) = (char)buffer[1] == '\0';
        buffer_cursor = 1;
    }
    *buffer = buffer_cursor;
    return;
}

/**
 * @brief Copy descriptor text field.
 *
 * @param descriptor Procedure input.
 * @param start_offset Procedure input.
 * @param end_offset Procedure input.
 * @param input_text Procedure input.
 * @param pad_with_spaces Whether the transfer must wait for completion.
 * @return Result produced by the procedure.
 */
uint32_t core_copy_descriptor_text_field(descriptor, start_offset, end_offset, input_text, pad_with_spaces)
uint16_t *descriptor;
int start_offset;
int end_offset;
char *input_text;
char pad_with_spaces;

{
    /* Copy descriptor text field using only caller-provided data. */
    int characters_remaining;
    uint32_t remaining_count;
    int destination_offset;
    char output_character;

    if ((int)(uint32_t)*descriptor <= end_offset) {
        core_halt_forever();
    }
    if (end_offset < start_offset) {
        core_halt_forever();
    }
    remaining_count = 0;
    if (-1 < end_offset - start_offset) {
        characters_remaining = (end_offset - start_offset) + 1;
        /* Repeat while characters_remaining != 0; the body advances or polls the state needed to leave the loop. */
        do {
            output_character = *input_text;
            if (output_character == '\0') {
                if (pad_with_spaces == '\0')
                    break;
                output_character = ' ';
            } else {
                input_text = input_text + 1;
            }
            destination_offset = start_offset + remaining_count;
            characters_remaining = characters_remaining + -1;
            remaining_count = remaining_count + 1;
            *(char *)(*(int *)(descriptor + 2) + destination_offset) = output_character;
        } while (characters_remaining != 0);
    }
    return remaining_count & 0xffff;
}

/**
 * @brief Execute ATA standby or verify.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_execute_ata_standby_or_verify(condition)
int condition;

{
    /* Execute ATA standby or verify using only caller-provided data. */
    int core_update_capacity_block_context_result;
    uint32_t ahci_execute_taskfile_command_result;
    uint32_t status_bits;
    volatile uint32_t working_result = 0;
    uint8_t working_buffer[32];
    uint8_t ata_command[10];
    volatile uint8_t command_opcode = 0;

    core_update_capacity_block_context_result = core_update_capacity_block_context(working_buffer);
    if (core_update_capacity_block_context_result == 0) {
        ahci_execute_taskfile_command_result = 0;
    } else {
        if (condition == 1) {
            core_initialize_ata_taskfile(ata_command);
            command_opcode = 0xe0;
        } else {
            status_bits = 0;
            working_result = 0;
            /* Call core encode ata taskfile lba through the defined unprototyped function-pointer signature. */
            ((void (*)())core_encode_ata_taskfile_lba)(ata_command, &status_bits, 0);
            command_opcode = 0x40;
        }
        ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        core_clear_request_and_release_device_command(working_buffer);
    }
    return ahci_execute_taskfile_command_result;
}

/**
 * @brief Execute ATA read verify.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_execute_ata_read_verify(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Execute ATA read verify using only caller-provided data. */
    int operation_status;
    uint32_t ahci_execute_taskfile_command_result;
    uint32_t context_derived;
    volatile uint32_t value_derived = 0;
    uint8_t working_buffer[32];
    uint8_t ata_command[10];
    volatile uint8_t working_result = 0;

    context_derived = context;
    value_derived = value;
    operation_status = core_update_capacity_block_context(working_buffer);
    if (operation_status == 0) {
        ahci_execute_taskfile_command_result = 0;
    } else {
        operation_status = (int)core_get_cartridge_ata_device_info(operation_status);
        /* Call core encode ata taskfile lba through the defined unprototyped function-pointer signature. */
        ((void (*)())core_encode_ata_taskfile_lba)(ata_command, &context_derived, length, *(uint8_t *)(operation_status + 0x12));
        operation_status = (int)core_get_cartridge_ata_device_info();
        if (*(char *)(operation_status + 0x12) == '\0') {
            working_result = 0x40;
        } else {
            working_result = 0x42;
        }
        ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        core_clear_request_and_release_device_command(working_buffer);
    }
    return ahci_execute_taskfile_command_result;
}

/**
 * @brief Shift multiword right.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param shift_count Procedure input.
 * @param word_count Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_shift_multiword_right(context, value, shift_count, word_count)
int context;
int value;
uint32_t shift_count;
int word_count;

{
    /* Shift multiword right using only caller-provided data. */
    uint32_t condition_1_field;
    uint32_t *data_cursor;
    uint32_t status_bits;
    uint32_t low_mask;
    uint32_t *condition_2_field;
    int shifts_remaining;

    if (shift_count < 0x20) {
        low_mask = 1;
        if (1 < shift_count) {
            shifts_remaining = shift_count - 1;
            /* Repeat while shifts_remaining != 0; the body advances or polls the state needed to leave the loop. */
            do {
                shifts_remaining = shifts_remaining + -1;
                low_mask = low_mask | low_mask << 1;
            } while (shifts_remaining != 0);
        }
        if (shift_count == 0) {
            low_mask = 0;
        }
        condition_1_field = 0;
        if (word_count != 0) {
            condition_2_field = (uint32_t *)(word_count * 4 + value);
            data_cursor = (uint32_t *)(word_count * 4 + context);
            /* Repeat while word_count != 0; the body advances or polls the state needed to leave the loop. */
            do {
                condition_2_field = condition_2_field + -1;
                status_bits = *condition_2_field;
                word_count = word_count + -1;
                data_cursor = data_cursor + -1;
                *data_cursor = condition_1_field | status_bits >> (shift_count & 0xff);
                condition_1_field = (low_mask & status_bits) << (0x20 - shift_count & 0xff);
            } while (word_count != 0);
        }
    } else {
        condition_1_field = 0;
    }
    return condition_1_field;
}

/**
 * @brief Parse typed payload record.
 *
 * @param buffer Procedure input.
 * @param source Procedure input.
 * @param record_offset Procedure input.
 * @param source_capacity Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_parse_typed_payload_record(buffer, source, record_offset, source_capacity)
uint32_t *buffer;
int source;
int record_offset;
int source_capacity;

{
    /* Parse typed payload record using only caller-provided data. */
    int16_t core_read_big_endian_record_field_result;
    uint16_t payload_length;
    uint32_t decoded_value;
    uint32_t operation_result;

    ti_memset(buffer, 0, 0x10);
    operation_result = 0;
    if ((record_offset + 4 < source_capacity) &&
        (core_read_big_endian_record_field_result = (int16_t)record_offset,
         payload_length = core_read_big_endian_record_field(source, (int)(int16_t)(core_read_big_endian_record_field_result + 2), (int)(int16_t)(core_read_big_endian_record_field_result + 3)),
         (int)((uint32_t)payload_length + record_offset + 4) <= source_capacity)) {
        decoded_value = core_read_big_endian_record_field(source, (int)core_read_big_endian_record_field_result, (int)core_read_big_endian_record_field_result);
        *(uint16_t *)(buffer + 2) = payload_length;
        buffer[3] = record_offset + source + 4;
        operation_result = 1;
        *buffer = decoded_value;
    }
    return operation_result;
}

/**
 * @brief Find typed payload with minimum length.
 *
 * @param buffer Procedure input.
 * @param source_base Procedure input.
 * @param descriptor Procedure input.
 * @param target_type Procedure input.
 * @param minimum_payload_length Procedure input.
 * @return Result produced by the procedure.
 */
int core_find_typed_payload_with_minimum_length(buffer, source_base, descriptor, target_type, minimum_payload_length)
int *buffer;
int source_base;
int descriptor;
int target_type;
uint32_t minimum_payload_length;

{
    /* Find typed payload with minimum length using only caller-provided data. */
    int core_parse_typed_payload_record_result;
    uint32_t payload_length;
    uint32_t descriptor_length;
    uint32_t status_bits;

    status_bits = 0;
    descriptor_length = *(uint32_t *)(descriptor + 8);
    /* Repeat while target_type != *buffer && *buffer != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_parse_typed_payload_record_result = core_parse_typed_payload_record(buffer, source_base + *(int *)(descriptor + 4), status_bits, descriptor_length & 0xffff);
        if (core_parse_typed_payload_record_result == 0)
            goto core_payload_lookup_return_failure;
        payload_length = (uint32_t)*(uint16_t *)(buffer + 2);
        descriptor_length = *(uint32_t *)(descriptor + 8);
        status_bits = status_bits + payload_length + 4;
        if (descriptor_length < status_bits)
            goto core_payload_lookup_return_failure;
        status_bits = status_bits & 0xffff;
    } while (target_type != *buffer && *buffer != 0);
    if ((payload_length < minimum_payload_length) || (descriptor_length < payload_length + ((buffer[3] - source_base) - *(int *)(descriptor + 4)))) {
    /* Shared target for core payload lookup return failure; incoming paths preserve the same state assumptions. */
    core_payload_lookup_return_failure:
        core_parse_typed_payload_record_result = 0;
    }
    return core_parse_typed_payload_record_result;
}

/**
 * @brief Build type two fingerprint.
 *
 * @param output Procedure input.
 * @param source Procedure input.
 * @return No value.
 */
void core_build_type_two_fingerprint(output, source) int output;
int source;

{
    /* Build type two fingerprint using only caller-provided data. */
    core_copy_bytes(output, "FprintTyp2- ", 0xc);
    *(uint8_t *)(output + 0xb) = 0x30;
    core_copy_bytes(output + 0xc, *(uint32_t *)(source + 0x10), 0x14);
    core_process_ti_memset_ti_memset(output + 0x20, source);
    core_update_response_word_ti(output + 0x40, source + 0xc);
    core_process_clear_memory_clear_memory_primary_memory_region(output + 0x60, source);
    *(uint32_t *)(output + 0x7c) = *(uint32_t *)(output + 0x7c) & 0x7fffffff;
    return;
}

/**
 * @brief Build encrypted password record.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
void core_build_encrypted_password_record(offset, value, length, buffer) int offset;
uint32_t value;
uint32_t length;
uint32_t *buffer;

{
    /* Build encrypted password record; persistent state is carried in core encoded password prefix. */
    uint8_t working_buffer[16];
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    volatile uint32_t buffer_cursor = 0;
    volatile uint32_t response_word = 0;

    buffer_cursor = *buffer;
    response_word = buffer[1];
    value_derived = value;
    length_component = length;
    core_process_clear_memory_clear_memory_secondary_memory_region(working_buffer, &value_derived);
    ti_memset(offset + 0x1d, 0, 3);
    core_decode_indexed_bytes(offset, &core_encoded_password_prefix, 9);
    core_copy_bytes(offset + 9, length, 0x14);
    /* Call core encrypt keyed word block through the defined unprototyped function-pointer signature. */
    ((void (*)())core_encrypt_keyed_word_block)(offset, 8, working_buffer);
    return;
}

/**
 * @brief Dispatch USB control completion.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_dispatch_usb_control_completion(context, value) uint32_t context;
uint32_t value;

{
    /* Dispatch USB control completion; persistent state is carried in usb ep0 state, usb setup packet and related record fields. */
    if (usb_ep0_state == 0) {
        core_copy_memory_bytes(&usb_setup_packet, &usb_ep0_buffer, 8);
        usb_ep0_three_stage_transfer = 0;
        /* Notify usb setup packet callback only after the associated state fields have been committed. */
        (*usb_setup_packet_callback)();
    } else {
        /* Notify usb data transfer callback only after the associated state fields have been committed. */
        (*usb_data_transfer_callback)(context, value);
        if (usb_ep0_state == 0) {
            core_arm_usb_control_setup_reception();
            return;
        }
    }
    return;
}

/**
 * @brief Process transfer requested.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param requested_length Procedure input.
 * @param flags Procedure input.
 * @param transfer_state Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_transfer_requested(context, condition, requested_length, flags, transfer_state)
uint32_t context;
int condition;
uint32_t requested_length;
uint32_t flags;
int transfer_state;

{
    /* Process transfer requested using only caller-provided data. */
    int operation_result;
    uint32_t working_result;

    if (condition == 0) {
        working_result = 0x1c;
    } else {
        working_result = 0x1d;
    }
    operation_result = ((int (*)())core_run_target_type_clear_memory_target_type_context)(context, working_result, transfer_state);
    if (operation_result == 0) {
        if ((uint32_t)(*(int *)(transfer_state + 8) - *(int *)(transfer_state + 4)) < requested_length) {
            operation_result = 0x14;
        } else {
            operation_result = requested_length + *(int *)(transfer_state + 4);
            *(int *)(transfer_state + 4) = operation_result;
            *(int *)(transfer_state + 8) = operation_result;
            operation_result = ((int (*)())core_handle_device_ti_memset)(context, transfer_state);
            if (operation_result == 0) {
                if (condition == 0) {
                    working_result = 0x3b;
                } else {
                    working_result = 0x3c;
                }
                operation_result = core_submit_zero_based_transfer(working_result, flags, context);
                return operation_result;
            }
        }
    }
    return operation_result;
}

/**
 * @brief Configure initialize output record.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
bool core_configure_initialize_output_record(offset) int offset;

{
    /* Configure initialize output record using only caller-provided data. */
    int operation_status;
    uint32_t working_result;
    uint8_t working_buffer[36];

    core_update_ti_memset_for_initialize_output_record();
    *(uint32_t *)(offset + 0x44) = 1;
    operation_status = core_initialize_output_record_from_source(offset, working_buffer);
    if (operation_status == 0) {
        operation_status = 2;
    } else {
        operation_status = ((int (*)())core_update_validation_0x20)(working_buffer);
        if ((operation_status == 0) && (operation_status = ((uint32_t (*)())core_transfer_retries_remaining_ti_memset)(offset, working_buffer), operation_status == 0)) {
            working_result = 2;
            *(uint32_t *)(offset + 0x48) = *(uint32_t *)(offset + 0x1c);
        } else {
            working_result = 3;
        }
        *(uint32_t *)(offset + 0x44) = working_result;
        core_reset_output_record(working_buffer);
    }
    return operation_status == 0;
}
