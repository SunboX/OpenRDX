/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Transfer range end range start for short available.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
uint32_t core_transfer_range_end_range_start_for_short_available(context, range_start, range_end, flags)
int context;
int range_start;
int range_end;
uint32_t flags;

{
    /* Transfer range end range start for short available using only caller-provided data. */
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
 * @brief Configure command opcode response.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_configure_command_opcode_response(context, condition) uint32_t context;
int condition;

{
    /* Configure command opcode response using only caller-provided data. */
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
        core_update_ahci_control_register(context, 0, 0);
    }
    return;
}

/**
 * @brief Transfer word bit mask.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param bit_mask Procedure input.
 * @param word_count Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_transfer_word_bit_mask(offset, value, bit_mask, word_count)
int offset;
uint32_t value;
uint32_t bit_mask;
int word_count;

{
    /* Transfer word bit mask using only caller-provided data. */
    uint32_t status_bits;
    int operation_status;
    uint32_t *computed_offset;
    uint32_t remaining_count;
    uint32_t high_result;
    uint32_t condition_1_field;
    volatile uint32_t condition_2_field = 0;

    if (word_count == 0 || bit_mask == 0) {
        status_bits = 0;
        high_result = value;
    } else {
        operation_status = 0x20;
        status_bits = 0x80000000;
        remaining_count = 0;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((status_bits & bit_mask) != 0)
                break;
            operation_status = operation_status + -1;
            remaining_count = remaining_count + 1;
            status_bits = status_bits >> 1;
        } while (operation_status != 0);
        high_result = value;
        condition_1_field = bit_mask;
        condition_2_field = word_count;
        high_result = core_transfer_shifts_remaining_words(offset, value, remaining_count);
        computed_offset = (uint32_t *)(offset + word_count * 4);
        /* Repeat while word_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            computed_offset = computed_offset + -1;
            condition_1_field = *computed_offset;
            condition_2_field = high_result;
            core_transition_scale_word_range_word(computed_offset, &high_result, &condition_1_field, bit_mask << (remaining_count & 0xff));
            word_count = word_count + -1;
        } while (word_count != 0);
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
 * @brief Process command opcode command.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
uint32_t core_process_command_opcode_command(context, value)
uint32_t context;
uint32_t value;

{
    /* Process command opcode command using only caller-provided data. */
    int operation_status;
    uint8_t status_bits;
    volatile uint8_t working_result = 0;
    volatile uint8_t command_opcode = 0;
    volatile uint8_t command_code = 0;
    volatile uint8_t core_transfer_device_feature_mask_feature_mask_result = 0;
    volatile uint8_t valid_flag = 0;

    ti_memset(&status_bits, 0, 0x30);
    status_bits = 0x27;
    command_opcode = 0xef;
    working_result = 0x80;
    command_code = 3;
    core_transfer_device_feature_mask_feature_mask_result = core_transfer_device_feature_mask_feature_mask(context, value);
    valid_flag = 1;
    operation_status = ((uint32_t (*)())core_submit_command_descriptor_to_device)(context, &status_bits, 0);
    if (operation_status == 0) {
        core_update_ahci_control_register(context, 0, 0);
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
    /* Handle cartridge input state for monitor cartridge input; persistent state is carried in core state 108, core cartridge input state and related record fields. */
    uint32_t core_state_108_snapshot;
    uint32_t core_update_configuration_update_context_result;

    core_cartridge_input_state = condition;
    if (condition == 0) {
        core_handle_cartridge_input_state();
        return;
    }
    if (condition == 1) {
        core_update_store_context_enable(1);
        core_state_108_snapshot = 0;
    /* Shared target for core handle cartridge input state for monitor cartridge input before update configuration update context; incoming paths preserve the same state assumptions. */
    core_handle_cartridge_input_state_for_monitor_cartridge_input_before_update_configuration_update_context:
        core_update_configuration_update_context_result = core_update_configuration_update_context(core_state_108_snapshot);
        rti_process_thermal_timer_state(&core_state_102, core_update_configuration_update_context_result);
        return;
    }
    if (condition == 2) {
        core_handle_clear_memory_lookup_ata_command_buffer();
        return;
    }
    if (condition != 3) {
        if (condition == 4) {
            core_state_108_snapshot = core_state_108 + 1U & 0xff;
            goto core_handle_cartridge_input_state_for_monitor_cartridge_input_before_update_configuration_update_context;
        }
        if (condition != 5) {
            return;
        }
        core_set_command_phase_to_two();
    }
    core_process_command_runtime_state();
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
    uint32_t working_result;

    *destination_record = *source_record;
    working_result = source_record[2];
    destination_record[1] = source_record[1];
    destination_record[2] = working_result;
    working_result = source_record[4];
    destination_record[3] = source_record[3];
    destination_record[4] = working_result;
    core_forward_end_remaining(destination_record + 0x25, source_record + 5, 0x80);
    core_forward_end_remaining(destination_record + 5, &core_branch_target_117, 0x80);
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
    uint16_t core_transfer_range_end_range_start_for_validate_structured_flash_result;
    uint32_t descriptor_type;
    uint32_t offset_1_field_0x24;
    uint32_t operation_result;

    ti_memset(destination_record + 0x24, 0, 0x108);
    operation_result = 0;
    if (source_record != 0) {
        core_transfer_range_end_range_start_for_validate_structured_flash_result = core_transfer_range_end_range_start_for_validate_structured_flash(source_record, 0, 1);
        *(uint16_t *)(destination_record + 0x24) = core_transfer_range_end_range_start_for_validate_structured_flash_result;
        descriptor_type = core_transfer_range_end_range_start_for_validate_structured_flash(source_record, 2, 2);
        offset_1_field_0x24 = (uint32_t)*(uint16_t *)(destination_record + 0x24);
        *(uint32_t *)(destination_record + 0x28) = descriptor_type;
        if ((8 < offset_1_field_0x24) && (offset_1_field_0x24 < 0x109)) {
            core_forward_end_remaining(destination_record + 0x2c, source_record + 8, offset_1_field_0x24 - 8);
            core_sha256_initialize(destination_record + 300);
            operation_result = 1;
        }
    }
    return operation_result;
}

/**
 * @brief Transfer USB get device.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_usb_get_device(condition)
uint32_t condition;

{
    /* Transfer USB get device using only caller-provided data. */
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
            /* Shared target for core transfer USB get device return path; incoming paths preserve the same state assumptions. */
            core_transfer_usb_get_device_return_path:
                if (length_output != (uint32_t *)0x0) {
                    *length_output = (uint32_t)data_cursor;
                }
                return 0;
            }
            if (data_cursor[1] == 0x21) {
                if ((condition == 0x22) && (entry_index = entry_index + 1, entry_index - callback_result == 1)) {
                    data_cursor = data_cursor + operation_status;
                    goto core_transfer_usb_get_device_return_path;
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
 * @brief Transfer descriptor wait completion.
 *
 * @param descriptor Procedure input.
 * @param start_offset Procedure input.
 * @param end_offset Procedure input.
 * @param input_text Procedure input.
 * @param wait_for_completion Whether the transfer must wait for completion.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_descriptor_wait_completion(descriptor, start_offset, end_offset, input_text, wait_for_completion)
uint16_t *descriptor;
int start_offset;
int end_offset;
char *input_text;
char wait_for_completion;

{
    /* Transfer descriptor wait completion using only caller-provided data. */
    int condition_2_field;
    uint32_t remaining_count;
    int condition_1_field;
    char operation_status;

    if ((int)(uint32_t)*descriptor <= end_offset) {
        core_halt_forever();
    }
    if (end_offset < start_offset) {
        core_halt_forever();
    }
    remaining_count = 0;
    if (-1 < end_offset - start_offset) {
        condition_2_field = (end_offset - start_offset) + 1;
        /* Repeat while condition_2_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            operation_status = *input_text;
            if (operation_status == '\0') {
                if (wait_for_completion == '\0')
                    break;
                operation_status = ' ';
            } else {
                input_text = input_text + 1;
            }
            condition_1_field = start_offset + remaining_count;
            condition_2_field = condition_2_field + -1;
            remaining_count = remaining_count + 1;
            *(char *)(*(int *)(descriptor + 2) + condition_1_field) = operation_status;
        } while (condition_2_field != 0);
    }
    return remaining_count & 0xffff;
}

/**
 * @brief Submit ATA standby immediate.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_submit_ata_standby_immediate(condition)
int condition;

{
    /* Submit ATA standby immediate using only caller-provided data. */
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
            core_update_ti_memset_for_ti_memset(ata_command);
            command_opcode = 0xe0;
        } else {
            status_bits = 0;
            working_result = 0;
            /* Call core configure ti memset through the defined unprototyped function-pointer signature. */
            ((void (*)())core_configure_ti_memset)(ata_command, &status_bits, 0);
            command_opcode = 0x40;
        }
        ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        core_update_thermal_monitor_enabled_for_read_smart_temperature(working_buffer);
    }
    return ahci_execute_taskfile_command_result;
}

/**
 * @brief Configure ATA command.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_configure_ata_command(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Configure ATA command using only caller-provided data. */
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
        operation_status = (int)core_get_ata_command_buffer(operation_status);
        /* Call core configure ti memset through the defined unprototyped function-pointer signature. */
        ((void (*)())core_configure_ti_memset)(ata_command, &context_derived, length, *(uint8_t *)(operation_status + 0x12));
        operation_status = (int)core_get_ata_command_buffer();
        if (*(char *)(operation_status + 0x12) == '\0') {
            working_result = 0x40;
        } else {
            working_result = 0x42;
        }
        ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        core_update_thermal_monitor_enabled_for_read_smart_temperature(working_buffer);
    }
    return ahci_execute_taskfile_command_result;
}

/**
 * @brief Transfer shifts remaining word.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param shift_count Procedure input.
 * @param word_count Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_shifts_remaining_word(context, value, shift_count, word_count)
int context;
int value;
uint32_t shift_count;
int word_count;

{
    /* Transfer shifts remaining word using only caller-provided data. */
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
 * @brief Configure payload ti memset.
 *
 * @param buffer Procedure input.
 * @param source Procedure input.
 * @param record_offset Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_configure_payload_ti_memset(buffer, source, record_offset, condition)
uint32_t *buffer;
int source;
int record_offset;
int condition;

{
    /* Configure payload ti memset using only caller-provided data. */
    int16_t core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result;
    uint16_t payload_length;
    uint32_t decoded_value;
    uint32_t operation_result;

    ti_memset(buffer, 0, 0x10);
    operation_result = 0;
    if ((record_offset + 4 < condition) &&
        (core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result = (int16_t)record_offset,
         payload_length = core_transfer_range_end_range_start_for_descriptor_descriptor_resource(source, (int)(int16_t)(core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result + 2), (int)(int16_t)(core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result + 3)),
         (int)((uint32_t)payload_length + record_offset + 4) <= condition)) {
        decoded_value = core_transfer_range_end_range_start_for_descriptor_descriptor_resource(source, (int)core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result, (int)core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result);
        *(uint16_t *)(buffer + 2) = payload_length;
        buffer[3] = record_offset + source + 4;
        operation_result = 1;
        *buffer = decoded_value;
    }
    return operation_result;
}

/**
 * @brief Transfer maximum payload payload.
 *
 * @param buffer Procedure input.
 * @param source_base Procedure input.
 * @param descriptor Procedure input.
 * @param target_type Procedure input.
 * @param maximum_payload Procedure input.
 * @return Result produced by the procedure.
 */
int core_transfer_maximum_payload_payload(buffer, source_base, descriptor, target_type, maximum_payload)
int *buffer;
int source_base;
int descriptor;
int target_type;
uint32_t maximum_payload;

{
    /* Transfer maximum payload payload using only caller-provided data. */
    int core_configure_payload_ti_memset_result;
    uint32_t payload_length;
    uint32_t descriptor_length;
    uint32_t status_bits;

    status_bits = 0;
    descriptor_length = *(uint32_t *)(descriptor + 8);
    /* Repeat while target_type != *buffer && *buffer != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_configure_payload_ti_memset_result = core_configure_payload_ti_memset(buffer, source_base + *(int *)(descriptor + 4), status_bits, descriptor_length & 0xffff);
        if (core_configure_payload_ti_memset_result == 0)
            goto core_transfer_maximum_payload_payload_return_path;
        payload_length = (uint32_t)*(uint16_t *)(buffer + 2);
        descriptor_length = *(uint32_t *)(descriptor + 8);
        status_bits = status_bits + payload_length + 4;
        if (descriptor_length < status_bits)
            goto core_transfer_maximum_payload_payload_return_path;
        status_bits = status_bits & 0xffff;
    } while (target_type != *buffer && *buffer != 0);
    if ((payload_length < maximum_payload) || (descriptor_length < payload_length + ((buffer[3] - source_base) - *(int *)(descriptor + 4)))) {
    /* Shared target for core transfer maximum payload payload return path; incoming paths preserve the same state assumptions. */
    core_transfer_maximum_payload_payload_return_path:
        core_configure_payload_ti_memset_result = 0;
    }
    return core_configure_payload_ti_memset_result;
}

/**
 * @brief Process fprinttyp2.
 *
 * @param output Procedure input.
 * @param source Procedure input.
 * @return No value.
 */
void core_process_fprinttyp2(output, source) int output;
int source;

{
    /* Process fprinttyp2 using only caller-provided data. */
    core_forward_end_remaining(output, "FprintTyp2- ", 0xc);
    *(uint8_t *)(output + 0xb) = 0x30;
    core_forward_end_remaining(output + 0xc, *(uint32_t *)(source + 0x10), 0x14);
    core_process_ti_memset_ti_memset(output + 0x20, source);
    core_update_response_word_ti(output + 0x40, source + 0xc);
    core_process_clear_memory_clear_memory_primary_memory_region(output + 0x60, source);
    *(uint32_t *)(output + 0x7c) = *(uint32_t *)(output + 0x7c) & 0x7fffffff;
    return;
}

/**
 * @brief Process response word clear memory response word context.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
void core_process_response_word_clear_memory_response_word_context(offset, value, length, buffer) int offset;
uint32_t value;
uint32_t length;
uint32_t *buffer;

{
    /* Process response word clear memory response word context; persistent state is carried in core response word context. */
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
    core_transfer_x01(offset, &core_response_word_context, 9);
    core_forward_end_remaining(offset + 9, length, 0x14);
    /* Call core transfer mixed word key sum through the defined unprototyped function-pointer signature. */
    ((void (*)())core_transfer_mixed_word_key_sum)(offset, 8, working_buffer);
    return;
}

/**
 * @brief Process firmware transfer for MWW transition.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_process_firmware_transfer_for_mww_transition(context, value) uint32_t context;
uint32_t value;

{
    /* Process firmware transfer for MWW transition; persistent state is carried in core firmware transfer context, core firmware remaining context and related record fields. */
    if (core_firmware_transfer_context == 0) {
        core_transfer_end_remaining(&core_firmware_remaining_context, &usb_ep0_buffer, 8);
        core_descriptor_configuration_context = 0;
        /* Notify USB primary state callback only after the associated state fields have been committed. */
        (*usb_primary_state_callback)();
    } else {
        /* Notify USB secondary state callback only after the associated state fields have been committed. */
        (*usb_secondary_state_callback)(context, value);
        if (core_firmware_transfer_context == 0) {
            core_process_firmware_transfer_for_mww();
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

