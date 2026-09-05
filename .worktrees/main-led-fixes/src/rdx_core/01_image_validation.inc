/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Validate structured flash image.
 *
 * @param buffer Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param condition Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
int core_validate_structured_flash_image(buffer, offset, length, condition, index)
int *buffer;
int offset;
int length;
uint32_t condition;
uint8_t index;

{
    /* Validate structured flash image; persistent state is carried in core state 041. */
    bool condition_met;
    uint32_t core_transfer_range_end_range_start_for_validate_structured_flash_result;
    uint32_t record_payload_offset;
    int operation_status;
    int core_calculate_crc32_result;
    uint32_t core_get_flash_validation_context_for_validate_structured_flash_result;
    uint8_t *computed_offset;
    int *buffer_cursor;
    uint32_t *active_length_field;
    uint32_t transfer_offset_or_length;
    int active_offset_or_status;
    int operation_result;
    int working_result;
    int core_process_constant_0x24_fixed_command_context_result;
    uint32_t status_bits;
    int source_base_offset;
    int length_component;
    uint32_t source_length;
    uint32_t continuation_index;
    uint32_t response_length;
    int extension_offset;
    int record_data_offset;
    uint32_t record_end_offset;
    int response_code;
    int record_field_value;
    int record_address;
    uint32_t record_end;
    int success_code;

    continuation_index = (uint32_t)index;
    response_length = 0;
    source_base_offset = offset;
    length_component = length;
    source_length = condition;
    /* Gate the following state transition on *buffer == 0; the handler is skipped when this condition is false. */
    if (*buffer == 0) {
        response_code = 0x20;
        if (condition < 0x20) {
        /* Shared target for core branch target 003; incoming paths preserve the same state assumptions. */
        core_branch_target_003:
            if (response_code != 0) {
            /* Shared target for core branch target 004; incoming paths preserve the same state assumptions. */
            core_branch_target_004:
                core_process_ti_memset_0x20(buffer);
            }
        } else {
            core_transfer_range_end_range_start_for_validate_structured_flash_result = core_transfer_range_end_range_start_for_validate_structured_flash(offset, 0, 3);
            operation_result = 4;
            success_code = 0;
            status_bits = 0;
            computed_offset = (uint8_t *)(offset + 7);
            active_length_field = &status_bits;
            /* Repeat while operation_result != 0; the body advances or polls the state needed to leave the loop. */
            do {
                operation_result = operation_result + -1;
                *(uint8_t *)active_length_field = *computed_offset;
                computed_offset = computed_offset + -1;
                active_length_field = (uint32_t *)((int)active_length_field + 1);
            } while (operation_result != 0);
            record_end_offset = status_bits;
            transfer_offset_or_length = status_bits - core_transfer_range_end_range_start_for_validate_structured_flash_result;
            if ((((status_bits < core_transfer_range_end_range_start_for_validate_structured_flash_result) || (source_length < transfer_offset_or_length)) || (source_length < transfer_offset_or_length + 4)) ||
                ((operation_result = core_transfer_range_end_range_start_for_validate_structured_flash(offset, (int)(int16_t)transfer_offset_or_length, (int)(int16_t)((int16_t)transfer_offset_or_length + 3)),
                  operation_result != 1 || ((source_length - transfer_offset_or_length) - 4 < 8))))
                goto core_branch_target_003;
            active_offset_or_status = offset + transfer_offset_or_length + 4;
            operation_result = core_transfer_range_end_range_start_for_validate_structured_flash(active_offset_or_status, 2, 3);
            buffer[1] = operation_result;
            operation_result = core_transfer_range_end_range_start_for_validate_structured_flash(active_offset_or_status, 4, 7);
            buffer[2] = operation_result;
            record_data_offset = transfer_offset_or_length + 0xc;
            buffer[3] = operation_result;
            if (buffer[1] != 1)
                goto core_branch_target_003;
            operation_result = core_calculate_crc32(offset, record_data_offset, 0xffffffff);
            buffer[5] = operation_result;
            core_transfer_range_end_range_start_for_validate_structured_flash_result = source_length - record_data_offset;
            if (core_transfer_range_end_range_start_for_validate_structured_flash_result < 0x18) {
            /* Shared target for core validate structured flash image before transfer range end range start for validate structured flash; incoming paths preserve the same state assumptions. */
            core_validate_structured_flash_image_before_transfer_range_end_range_start_for_validate_structured_flash:
                response_code = 0x2d;
                goto core_branch_target_004;
            }
            active_offset_or_status = offset + transfer_offset_or_length + 0xc;
            record_payload_offset = core_transfer_range_end_range_start_for_validate_structured_flash(active_offset_or_status, 0x14, 0x17);
            operation_result = success_code;
            if (record_payload_offset < 0xa8)
                goto core_validate_structured_flash_image_before_transfer_range_end_range_start_for_validate_structured_flash;
            working_result = buffer[8];
            extension_offset = success_code;
            if (0x17 < core_transfer_range_end_range_start_for_validate_structured_flash_result) {
                operation_status = core_transfer_range_end_range_start_for_validate_structured_flash(active_offset_or_status, 0xc, 0xf);
                record_field_value = operation_status;
                record_payload_offset = core_transfer_range_end_range_start_for_validate_structured_flash(active_offset_or_status, 0x14, 0x17);
                record_end = record_payload_offset + operation_status;
                if (record_end <= core_transfer_range_end_range_start_for_validate_structured_flash_result) {
                    core_calculate_crc32_result = core_calculate_crc32(active_offset_or_status, operation_status, buffer[5]);
                    operation_result = success_code;
                    buffer[5] = core_calculate_crc32_result;
                    record_address = source_base_offset + transfer_offset_or_length + operation_status + 0xc;
                    buffer_cursor = buffer + 5;
                    if (0xa7 < record_payload_offset) {
                        core_transfer_range_end_range_start_for_validate_structured_flash_result = core_transfer_range_end_range_start_for_validate_structured_flash(operation_status + active_offset_or_status, 0xa0, 0xa3);
                        if (0xa8 < record_payload_offset) {
                            extension_offset = 0xa4;
                        }
                        active_offset_or_status = core_transfer_range_end_range_start_for_validate_structured_flash(record_address, (int)(int16_t)(record_payload_offset - 4),
                                                           (int)(int16_t)((int16_t)record_payload_offset + -1));
                        operation_status = core_calculate_crc32(record_address, record_payload_offset - 4, *buffer_cursor);
                        *buffer_cursor = operation_status;
                        if (active_offset_or_status == operation_status) {
                            operation_result = core_calculate_crc32(record_payload_offset + record_address - 4U, 4);
                            *buffer_cursor = operation_result;
                            if ((char)working_result == '\0') {
                                if (core_transfer_range_end_range_start_for_validate_structured_flash_result != 0)
                                    goto core_validate_structured_flash_image_before_compare_buffers_from_end;
                                core_transfer_range_end_range_start_for_validate_structured_flash_result = 0x30;
                            } else {
                                core_transfer_range_end_range_start_for_validate_structured_flash_result = 0xffffffff;
                            /* Shared target for core validate structured flash image before compare buffers from end; incoming paths preserve the same state assumptions. */
                            core_validate_structured_flash_image_before_compare_buffers_from_end:
                                core_transfer_range_end_range_start_for_validate_structured_flash_result = (core_transfer_range_end_range_start_for_validate_structured_flash_result ^ 0x30) & 0x30;
                            }
                            operation_result = 1;
                            if (((core_transfer_range_end_range_start_for_validate_structured_flash_result >> 4 & 1) != 0) &&
                                (active_offset_or_status = core_compare_buffers_from_end(record_address + 0x10, &core_state_041, 4), active_offset_or_status != 0)) {
                                operation_result = success_code;
                            }
                            if ((core_transfer_range_end_range_start_for_validate_structured_flash_result >> 5 != 0) && (operation_result != 0)) {
                                core_get_flash_validation_context_for_validate_structured_flash_result = core_get_flash_validation_context_for_validate_structured_flash(success_code);
                                operation_result = core_transition_success_flag_clear_memory_transfer_success_context(core_get_flash_validation_context_for_validate_structured_flash_result, record_address, record_payload_offset);
                            }
                        }
                    }
                    if (operation_result == 0)
                        goto core_branch_target_003;
                    response_length = record_end;
                    if (extension_offset != 0) {
                        extension_offset = record_field_value + extension_offset;
                    }
                }
            }
            if (operation_result == 0)
                goto core_branch_target_003;
            operation_result = success_code;
            if ((extension_offset != 0) && (core_transfer_range_end_range_start_for_validate_structured_flash_result = extension_offset + record_data_offset & 0xffff, core_transfer_range_end_range_start_for_validate_structured_flash_result <= source_length)) {
                operation_result = source_base_offset + core_transfer_range_end_range_start_for_validate_structured_flash_result;
            }
            operation_result = core_process_descriptor_type_ti_memset(buffer, operation_result);
            if (operation_result == 0)
                goto core_validate_structured_flash_image_before_transfer_range_end_range_start_for_validate_structured_flash;
            operation_result = core_get_validation_flags();
            if (operation_result != 0) {
                response_code = 0x2c;
                goto core_branch_target_004;
            }
            buffer[6] = record_end_offset;
            buffer[4] = success_code;
            response_code = success_code;
            buffer[3] = (buffer[2] - response_length) - 8U;
            response_length = record_data_offset + response_length;
            buffer[7] = response_length;
        }
        operation_result = response_code;
        if (response_code != 0)
            goto core_validate_structured_flash_image_before_process_device_status_update;
        *buffer = 1;
    /* Shared target for core validate structured flash image before updating operation result; incoming paths preserve the same state assumptions. */
    core_validate_structured_flash_image_before_updating_operation_result:
        operation_result = 0x20;
        working_result = source_base_offset + response_length;
        active_offset_or_status = source_length - response_length;
        if ((buffer[7] == response_length + length_component) && (active_length_field = (uint32_t *)(buffer + 3), buffer[1] == 1)) {
            transfer_offset_or_length = source_length - response_length;
            condition_met = transfer_offset_or_length < *active_length_field;
            core_transfer_range_end_range_start_for_validate_structured_flash_result = response_length;
            if (!condition_met) {
                transfer_offset_or_length = buffer[3];
                core_transfer_range_end_range_start_for_validate_structured_flash_result = *active_length_field;
            }
            operation_status = core_write_validated_flash_data(buffer[4], working_result, transfer_offset_or_length);
            *active_length_field = *active_length_field - transfer_offset_or_length;
            buffer[4] = buffer[4] + transfer_offset_or_length;
            if (operation_status != 0) {
                if (condition_met) {
                    buffer[7] = active_offset_or_status + buffer[7];
                    operation_result = core_calculate_crc32(working_result, active_offset_or_status, buffer[5]);
                    buffer[5] = operation_result;
                    core_sha256_update(buffer, working_result, active_offset_or_status);
                    return 0;
                }
                operation_status = core_calculate_crc32(working_result, core_transfer_range_end_range_start_for_validate_structured_flash_result, buffer[5]);
                buffer[5] = operation_status;
                core_sha256_update(buffer, working_result, core_transfer_range_end_range_start_for_validate_structured_flash_result);
                working_result = buffer[7];
                buffer[7] = working_result + core_transfer_range_end_range_start_for_validate_structured_flash_result;
                if ((3 < active_offset_or_status - core_transfer_range_end_range_start_for_validate_structured_flash_result) && (buffer[6] - (working_result + core_transfer_range_end_range_start_for_validate_structured_flash_result) == 4)) {
                    core_process_constant_0x24_fixed_command_context_result = 0;
                    active_offset_or_status = 4;
                    computed_offset = (uint8_t *)(core_transfer_range_end_range_start_for_validate_structured_flash_result + source_base_offset + 3);
                    buffer_cursor = &core_process_constant_0x24_fixed_command_context_result;
                    /* Repeat while active_offset_or_status != 0; the body advances or polls the state needed to leave the loop. */
                    do {
                        active_offset_or_status = active_offset_or_status + -1;
                        *(uint8_t *)buffer_cursor = *computed_offset;
                        computed_offset = computed_offset + -1;
                        buffer_cursor = (int *)((int)buffer_cursor + 1);
                    } while (active_offset_or_status != 0);
                    if (core_process_constant_0x24_fixed_command_context_result == buffer[5]) {
                        active_offset_or_status = core_process_constant_0x24_fixed_command_context(buffer);
                        if (active_offset_or_status == 0) {
                            operation_result = 0x2d;
                            goto core_branch_target_007;
                        }
                        active_offset_or_status = core_update_flash_completion_context();
                        if (active_offset_or_status != 0) {
                            if (continuation_index == 0) {
                                *buffer = 2;
                            } else {
                                core_process_device_status_update(buffer);
                            }
                            return 0;
                        }
                    }
                }
            }
        }
    } else {
        operation_result = 0x20;
    /* Shared target for core validate structured flash image before process device status update; incoming paths preserve the same state assumptions. */
    core_validate_structured_flash_image_before_process_device_status_update:
        /* Gate the following state transition on *buffer == 1; the handler is skipped when this condition is false. */
        if (*buffer == 1)
            goto core_validate_structured_flash_image_before_updating_operation_result;
        /* Gate the following state transition on ((*buffer == 2) && (continuation_index != 0)) && (source_length == 0); the handler is skipped when this condition is false. */
        if (((*buffer == 2) && (continuation_index != 0)) && (source_length == 0)) {
            core_process_device_status_update(buffer);
            return 0;
        }
    }
    if (operation_result == 0) {
        return 0;
    }
/* Shared target for core branch target 007; incoming paths preserve the same state assumptions. */
core_branch_target_007:
    core_process_ti_memset_0x20(buffer);
    return operation_result;
}

/**
 * @brief Build SCSI mode sense response.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_build_scsi_mode_sense_response(buffer) uint32_t *buffer;

{
    /* Build SCSI mode sense response; persistent state is carried in core state 040, core state 037 and related record fields. */
    bool condition_met;
    uint8_t page_code;
    uint8_t working_result;
    int16_t core_process_ata_command_buffer_for_build_scsi_mode_result;
    int operation_status;
    int core_get_mode_sense_block_descriptor_length_result;
    uint8_t *data_cursor;
    int status_bits;
    int callback_result = 0;
    uint32_t response_length;
    uint32_t core_forward_end_remaining_result;
    bool include_extended_page_data;

    operation_status = core_is_command_phase_three_for_build_scsi_mode();
    if ((operation_status == 0) || (core_process_ata_command_buffer_for_build_scsi_mode_result = core_process_ata_command_buffer_for_build_scsi_mode(), core_process_ata_command_buffer_for_build_scsi_mode_result == 0)) {
        core_process_ata_command_buffer_for_build_scsi_mode_result = 0x200;
    }
    if (*(char *)buffer[2] == '\x1a') {
        operation_status = 4;
    } else {
        operation_status = 8;
    }
    core_get_mode_sense_block_descriptor_length_result = core_get_mode_sense_block_descriptor_length(buffer);
    page_code = *(uint8_t *)(callback_result + 2) & 0x3f;
    core_forward_end_remaining_result = operation_status + core_get_mode_sense_block_descriptor_length_result & 0xffff;
    if ((*(uint8_t *)(callback_result + 2) & 0x3f) != 0) {
        if (page_code == 5) {
            data_cursor = (uint8_t *)&core_state_040;
        } else if (page_code == 8) {
        /* Shared target for core build SCSI mode sense response before updating forward end remaining result; incoming paths preserve the same state assumptions. */
        core_build_scsi_mode_sense_response_before_updating_forward_end_remaining_result:
            core_forward_end_remaining_result = core_forward_end_remaining_result + 0x14 & 0xffff;
            if (page_code != 0x3f)
                goto core_build_scsi_mode_sense_response_before_process_ti_memset_for_build_scsi_mode;
        /* Shared target for core build SCSI mode sense response before updating data cursor; incoming paths preserve the same state assumptions. */
        core_build_scsi_mode_sense_response_before_updating_data_cursor:
            data_cursor = (uint8_t *)&core_state_037;
        } else {
            if (page_code != 0x31) {
                if (page_code == 0x33)
                    goto core_build_scsi_mode_sense_response_before_updating_data_cursor;
                if (page_code != 0x34) {
                    if (page_code != 0x3f) {
                        core_forward_end_remaining_result = 0;
                        *buffer = 8;
                        goto core_build_scsi_mode_sense_response_before_process_ti_memset_for_build_scsi_mode;
                    }
                    goto core_build_scsi_mode_sense_response_before_updating_forward_end_remaining_result;
                }
            }
            data_cursor = (uint8_t *)&core_state_034;
        }
        core_forward_end_remaining_result = core_forward_end_remaining_result + *data_cursor + 2 & 0xffff;
    }
/* Shared target for core build SCSI mode sense response before process ti memset for build SCSI mode; incoming paths preserve the same state assumptions. */
core_build_scsi_mode_sense_response_before_process_ti_memset_for_build_scsi_mode:
    if (core_forward_end_remaining_result == 0) {
        return;
    }
    operation_status = (int)core_process_ti_memset_for_build_scsi_mode(buffer, core_forward_end_remaining_result);
    core_get_mode_sense_block_descriptor_length_result = core_get_mode_sense_block_descriptor_length(buffer);
    status_bits = core_is_command_phase_three_for_build_scsi_mode();
    condition_met = false;
    if ((status_bits != 0) && (status_bits = core_process_command_context_value(), status_bits != 0)) {
        condition_met = true;
    }
    if (*(char *)buffer[2] == '\x1a') {
        core_update_release_record_payload(operation_status, buffer, 4, 4);
        **(char **)(operation_status + 4) = (char)core_forward_end_remaining_result + -1;
        if (condition_met) {
            working_result = 0x80;
        } else {
            working_result = 0;
        }
        *(uint8_t *)(*(int *)(operation_status + 4) + 2) = working_result;
        if (core_get_mode_sense_block_descriptor_length_result != 0) {
            *(char *)(*(int *)(operation_status + 4) + 3) = (char)core_get_mode_sense_block_descriptor_length_result;
            core_process_halt_forever_for_build_scsi_mode(operation_status, 10, 0xb, core_process_ata_command_buffer_for_build_scsi_mode_result);
        }
        core_get_mode_sense_block_descriptor_length_result = core_get_mode_sense_block_descriptor_length_result + 4;
    } else {
        core_update_release_record_payload(operation_status, buffer, 7, 8);
        core_process_halt_forever_for_build_scsi_mode(operation_status, 0, 1, core_forward_end_remaining_result - 2);
        if (condition_met) {
            working_result = 0x80;
        } else {
            working_result = 0;
        }
        *(uint8_t *)(*(int *)(operation_status + 4) + 3) = working_result;
        if (core_get_mode_sense_block_descriptor_length_result != 0) {
            core_process_halt_forever_for_build_scsi_mode(operation_status, 6, 7, core_get_mode_sense_block_descriptor_length_result);
            core_process_halt_forever_for_build_scsi_mode(operation_status, 0xe, 0xf, core_process_ata_command_buffer_for_build_scsi_mode_result);
        }
        core_get_mode_sense_block_descriptor_length_result = core_get_mode_sense_block_descriptor_length_result + 8;
    }
    page_code = *(uint8_t *)(buffer[2] + 2);
    status_bits = (int)(uint32_t)page_code >> 6;
    core_forward_end_remaining_result = page_code & 0x3f;
    include_extended_page_data = status_bits != 1;
    condition_met = status_bits == 0 || status_bits == 3;
    if ((page_code & 0x3f) == 0) {
        return;
    }
    if (core_forward_end_remaining_result == 5) {
        if (!include_extended_page_data) {
            return;
        }
        core_forward_end_remaining(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4), &core_state_039, 10);
        return;
    }
    if (core_forward_end_remaining_result != 8) {
        if (core_forward_end_remaining_result == 0x31) {
            if (include_extended_page_data) {
                core_forward_end_remaining(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4), &core_state_033, 2);
            }
            if (status_bits != 0 && status_bits != 3) {
                return;
            }
            *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 4) = core_command_descriptor_context;
            status_bits = core_get_runtime_context_base_for_build_scsi_mode();
            working_result = core_read_mode_page_field_1c_value(status_bits + 0x2c);
            *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 5) = working_result;
            status_bits = core_get_runtime_context_base_for_build_scsi_mode();
            working_result = core_read_mode_page_field_48_value(status_bits + 0x2c);
        /* Shared target for core branch target 012; incoming paths preserve the same state assumptions. */
        core_branch_target_012:
            *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 6) = working_result;
            return;
        }
        if (core_forward_end_remaining_result == 0x33)
            goto core_branch_target_011;
        if (core_forward_end_remaining_result == 0x34) {
            core_forward_end_remaining(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4), &core_state_035, 2);
            if (condition_met) {
                page_code = core_get_notification_context_value();
                page_code = page_code & 3;
                if (page_code == 0) {
                    page_code = 1;
                }
                *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 4) = page_code;
                page_code = core_get_default_mode_page_flags();
                *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 6) = page_code & 3;
            }
            if (status_bits != 1) {
                return;
            }
            *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4)) =
                *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4)) | 0x80;
            working_result = 0;
            *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 4) = 3;
            goto core_branch_target_012;
        }
        if (core_forward_end_remaining_result != 0x3f) {
            *buffer = 8;
            return;
        }
    }
    if (include_extended_page_data) {
        response_length = 3;
    } else {
        response_length = 2;
    }
    core_forward_end_remaining(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4), &core_state_038, response_length);
    core_get_mode_sense_block_descriptor_length_result = core_get_mode_sense_block_descriptor_length_result + 0x14;
    if (core_forward_end_remaining_result != 0x3f) {
        return;
    }
/* Shared target for core branch target 011; incoming paths preserve the same state assumptions. */
core_branch_target_011:
    if (include_extended_page_data) {
        core_forward_end_remaining(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4), &core_state_036, 2);
    }
    if (condition_met) {
        *(bool *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 3) = core_state_153 != '\0';
    }
    if (status_bits != 1) {
        return;
    }
    *(uint8_t *)(core_get_mode_sense_block_descriptor_length_result + *(int *)(operation_status + 4) + 3) = 1;
    return;
}

/**
 * @brief Transfer capacity word default block size capacity block context.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_transfer_capacity_word_default_block_size_capacity_block_context(offset) int offset;

{
    /* Transfer capacity word default block size capacity block context; persistent state is carried in core state 183, ATA identify major version and related record fields. */
    uint8_t core_state_183_pointer;
    uint16_t register_bits;
    uint16_t register_value;
    uint16_t device_capability_bits;
    int16_t ata_identify_major_version_snapshot;
    uint16_t ata_identify_command_set_extensions_supported_snapshot;
    uint16_t ata_identify_logical_sector_words_high_snapshot;
    uint8_t working_result;
    uint16_t *data_cursor;
    uint32_t entry_index;
    uint32_t default_block_size;
    uint32_t status_bits;
    char *core_atapi_device_flag_pointer;
    uint16_t *word_cursor;
    int computed_offset;
    uint32_t ata_identify_lba28_sector_count_snapshot;
    uint32_t capacity_high_word;
    int device_record_value;

    register_bits = ata_identify_device_data;
    device_record_value = offset * 0x1d0;
    computed_offset = 10;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(device_record_value + 0x800f47d) = (uint8_t)register_bits >> 7;
    data_cursor = (uint16_t *)&ata_identify_serial_number;
    word_cursor = (uint16_t *)(device_record_value + ATA_DEVICES_ADDRESS);
    /* Repeat while computed_offset != 0; the body advances or polls the state needed to leave the loop. */
    do {
        computed_offset = computed_offset + -1;
        *word_cursor = *data_cursor >> 8 | *data_cursor << 8;
        data_cursor = data_cursor + 1;
        word_cursor = word_cursor + 1;
    } while (computed_offset != 0);
    computed_offset = 4;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    data_cursor = (uint16_t *)(device_record_value + 0x800f44c);
    word_cursor = (uint16_t *)&ata_identify_firmware_revision;
    /* Repeat while computed_offset != 0; the body advances or polls the state needed to leave the loop. */
    do {
        computed_offset = computed_offset + -1;
        *data_cursor = *word_cursor >> 8 | *word_cursor << 8;
        data_cursor = data_cursor + 1;
        word_cursor = word_cursor + 1;
    } while (computed_offset != 0);
    computed_offset = 0x14;
    data_cursor = (uint16_t *)&ata_identify_model_number;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    word_cursor = (uint16_t *)(device_record_value + 0x800f454);
    /* Repeat while computed_offset != 0; the body advances or polls the state needed to leave the loop. */
    do {
        computed_offset = computed_offset + -1;
        *word_cursor = *data_cursor >> 8 | *data_cursor << 8;
        data_cursor = data_cursor + 1;
        word_cursor = word_cursor + 1;
    } while (computed_offset != 0);
    /* Snapshot core MMIO register 007 before decoding its status or capability fields. */
    ata_identify_lba28_sector_count_snapshot = ata_identify_lba28_sector_count;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint32_t *)(device_record_value + 0x800f48c) = ata_identify_lba28_sector_count_snapshot;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint32_t *)(device_record_value + 0x800f490) = 0;
    if ((&core_atapi_device_flag)[device_record_value] != '\0') {
        /* Snapshot core MMIO register 008 before decoding its status or capability fields. */
        register_value = ata_identify_obsolete_and_multiword_dma_words;
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        *(bool *)(device_record_value + 0x800f489) = (register_value & 0x8000) != 0;
    }
    /* Snapshot core MMIO register 009 before decoding its status or capability fields. */
    register_bits = ata_identify_multiword_dma_modes;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint16_t *)(device_record_value + 0x800f47e) = register_bits;
    /* Snapshot core MMIO register 006 before decoding its status or capability fields. */
    register_value = ata_identify_field_validity_word;
    if ((register_value >> 1 & 1) != 0) {
        /* Snapshot core MMIO register 010 before decoding its status or capability fields. */
        register_bits = ata_identify_pio_modes;
        *(uint16_t *)(device_record_value + 0x800f482) = register_bits;
    }
    /* Snapshot core MMIO register 011 before decoding its status or capability fields. */
    register_value = ata_identify_queue_depth;
    working_result = (uint8_t)(register_value & 0x1f);
    *(uint8_t *)(device_record_value + 0x800f4a9) = working_result;
    if (6 < (register_value & 0x1f)) {
        working_result = 7;
    }
    /* Snapshot core MMIO register 012 before decoding its status or capability fields. */
    register_value = ata_identify_sata_capabilities;
    status_bits = (uint32_t)register_value;
    *(uint8_t *)(device_record_value + 0x800f4a8) = working_result;
    *(uint8_t *)(device_record_value + 0x800f486) = (uint8_t)((status_bits << 0x16) >> 0x1f);
    entry_index = 3;
    (&core_state_182)[device_record_value] = (uint8_t)((status_bits << 0x17) >> 0x1f);
    /* Repeat while entry_index != 0; the body advances or polls the state needed to leave the loop. */
    do {
        if ((1 << (entry_index & 0xff) & status_bits) != 0) {
            (&core_state_179)[device_record_value] = (char)entry_index;
            break;
        }
        entry_index = entry_index - 1;
    } while (entry_index != 0);
    /* Snapshot core MMIO register 013 before decoding its status or capability fields. */
    register_value = ata_identify_sata_features_supported;
    device_capability_bits = ata_identify_sata_features_enabled;
    /* Snapshot core MMIO register 015 before decoding its status or capability fields. */
    ata_identify_major_version_snapshot = ata_identify_major_version;
    (&core_state_180)[device_record_value] = (uint8_t)(((uint32_t)register_value << 0x1d) >> 0x1f);
    *(uint8_t *)(device_record_value + 0x800f488) = (uint8_t)(((uint32_t)device_capability_bits << 0x19) >> 0x1f);
    if (ata_identify_major_version_snapshot != -1 && ata_identify_major_version_snapshot != 0) {
        *(char *)(device_record_value + 0x800f484) = (char)ata_identify_major_version_snapshot;
    }
    /* Snapshot core MMIO register 017 before decoding its status or capability fields. */
    ata_identify_command_set_extensions_supported_snapshot = ata_identify_command_set_extensions_supported;
    device_capability_bits = ata_identify_command_sets_supported;
    /* Snapshot core MMIO register 006 before decoding its status or capability fields. */
    register_value = ata_identify_field_validity_word;
    *(uint8_t *)(device_record_value + 0x800f49c) = (uint8_t)(((uint32_t)ata_identify_command_set_extensions_supported_snapshot << 0x19) >> 0x1f);
    (&core_state_181)[device_record_value] = (uint8_t)(((uint32_t)device_capability_bits << 0x15) >> 0x1f);
    if ((register_value >> 2 & 1) != 0) {
        /* Snapshot core MMIO register 018 before decoding its status or capability fields. */
        register_bits = ata_identify_ultra_dma_modes;
        *(uint16_t *)(device_record_value + 0x800f480) = register_bits;
    }
    if (((&core_state_181)[device_record_value] != '\0') &&
        (*(int *)(device_record_value + 0x800f490) == 0 && *(int *)(device_record_value + 0x800f48c) == 0xfffffff)) {
        core_transfer_end_remaining(device_record_value + 0x800f48c, &ata_identify_lba48_sector_count, 8);
    }
    /* Snapshot core MMIO register 017 before decoding its status or capability fields. */
    register_value = ata_identify_command_set_extensions_supported;
    if ((register_value >> 8 & 1) != 0) {
        *(uint8_t *)(device_record_value + 0x800f4a6) = 1;
        core_transfer_end_remaining(device_record_value + 0x800f49e, &ata_identify_world_wide_name, 8);
        register_value = ata_identify_world_wide_name;
        if ((register_value & 0xf000) != 0x5000) {
            *(uint8_t *)(device_record_value + 0x800f4a6) = 0;
        }
    }
    *(uint32_t *)(device_record_value + 0x800f4b8) = 1;
    entry_index = 0;
    (&core_state_183)[device_record_value] = 0;
    computed_offset = offset * 0x1d0;
    core_atapi_device_flag_pointer = (char *)&core_atapi_device_flag + computed_offset;
    ata_identify_lba28_sector_count_snapshot = *(uint32_t *)(computed_offset + 0x800f48c);
    capacity_high_word = *(uint32_t *)(computed_offset + 0x800f490);
    /* Snapshot core MMIO register 019 before decoding its status or capability fields. */
    register_value = ata_identify_sector_size_information;
    if (((register_value >> 0xe & 1) == 0) || ((int)(uint32_t)register_value >> 0xf == 1)) {
        /* Gate the following state transition on *core_atapi_device_flag_pointer == '\0'; the handler is skipped when this condition is false. */
        if (*core_atapi_device_flag_pointer == '\0') {
            default_block_size = 0x200;
        } else {
            default_block_size = 0x800;
        }
        *(uint32_t *)(computed_offset + 0x800f4ac) = default_block_size;
        *(uint32_t *)(computed_offset + 0x800f4bc) = default_block_size;
        core_atapi_device_flag_pointer = (char *)&core_atapi_device_flag + offset * 0x1d0;
        device_record_value = *(int *)(offset * 0x1d0 + 0x800f4ac);
    } else {
        if ((register_value >> 0xc & 1) == 0) {
            device_record_value = 0x200;
        } else {
            /* Snapshot core MMIO register 020 before decoding its status or capability fields. */
            register_bits = ata_identify_logical_sector_words_low;
            ata_identify_logical_sector_words_high_snapshot = ata_identify_logical_sector_words_high;
            device_record_value = CONCAT22(register_bits, ata_identify_logical_sector_words_high_snapshot) << 1;
        }
        *(int *)(computed_offset + 0x800f4ac) = device_record_value;
        /* Snapshot core MMIO register 019 before decoding its status or capability fields. */
        register_value = ata_identify_sector_size_information;
        if ((register_value >> 0xd & 1) == 0) {
            device_record_value = *(int *)(computed_offset + 0x800f4ac);
            *(int *)(computed_offset + 0x800f4bc) = device_record_value;
        } else {
            (&core_state_183)[computed_offset] = (uint8_t)register_value & 0xf;
            offset = offset * 0x1d0;
            core_atapi_device_flag_pointer = (char *)&core_atapi_device_flag + offset;
            core_state_183_pointer = (&core_state_183)[offset];
            /* Iterate while ; entry_index < core_state_183_pointer; entry_index = entry_index + 1, preserving the defined cursor and bound. */
            for (; entry_index < core_state_183_pointer; entry_index = entry_index + 1) {
                *(int *)(offset + 0x800f4b8) = *(int *)(offset + 0x800f4b8) << 1;
            }
            device_record_value = *(int *)(offset + 0x800f4ac);
            /* Snapshot core MMIO register 022 before decoding its status or capability fields. */
            register_value = ata_identify_alignment_word;
            *(int *)(offset + 0x800f4bc) = device_record_value * *(int *)(offset + 0x800f4b8);
            if (((register_value >> 0xe & 1) != 0) && ((int)(uint32_t)register_value >> 0xf != 1)) {
                *(uint16_t *)(&core_state_184 + offset) = register_value & 0x3fff;
            }
        }
    }
    *(int *)(core_atapi_device_flag_pointer + 0x34) = device_record_value;
    *(uint32_t *)(core_atapi_device_flag_pointer + 0x18) = ata_identify_lba28_sector_count_snapshot;
    *(uint32_t *)(core_atapi_device_flag_pointer + 0x1c) = capacity_high_word;
    *(uint16_t *)(core_atapi_device_flag_pointer + 0x46) = *(uint16_t *)(core_atapi_device_flag_pointer + 0x44);
    return;
}

/**
 * @brief Handle payload byte type or response payload response context.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_handle_payload_byte_type_or_response_payload_response_context(buffer) uint32_t *buffer;

{
    /* Handle payload byte type or response payload response context; persistent state is carried in core payload response context. */
    int16_t working_result;
    uint32_t status_bits;
    uint32_t *data_cursor;
    uint32_t operation_status;
    uint32_t response_code;
    int entry_count_or_status;
    int *record_type_cursor;
    uint32_t *core_payload_response_context_pointer;
    int record_type_or_response_offset;
    int entry_length_or_status;
    uint16_t *core_process_ti_memset_for_build_scsi_mode_result;
    uint8_t working_buffer[44];
    uint8_t entry_value_bytes[3];
    uint8_t record_payload_byte;
    uint32_t *buffer_cursor;
    uint32_t requested_record_type;

    core_process_ti_memset_for_build_scsi_mode_result = (uint16_t *)0x0;
    status_bits = *(uint8_t *)(buffer[2] + 1) & 0x1f;
    buffer_cursor = buffer;
    if ((*(uint8_t *)(buffer[2] + 1) & 0x1f) == 0) {
        requested_record_type = core_release_record_payload_when_short(buffer + 1, 8, 9);
        if (requested_record_type == 0) {
        /* Shared target for core handle payload byte type or response payload response context before get runtime context base for build SCSI mode; incoming paths preserve the same state assumptions. */
        core_handle_payload_byte_type_or_response_payload_response_context_before_get_runtime_context_base_for_build_scsi_mode:
            entry_count_or_status = core_get_runtime_context_base_for_build_scsi_mode();
            entry_count_or_status = core_initialize_output_record_from_source(entry_count_or_status + 0x2c, working_buffer);
            if (entry_count_or_status == 0) {
                response_code = 0x21;
            /* Shared target for core branch target 017; incoming paths preserve the same state assumptions. */
            core_branch_target_017:
                *buffer_cursor = response_code;
                goto core_handle_payload_byte_type_or_response_payload_response_context_before_update_release_record_payload;
            }
            core_process_ti_memset_for_build_scsi_mode_result = (uint16_t *)core_process_ti_memset_for_build_scsi_mode(buffer_cursor, 0x200);
            entry_count_or_status = 3;
            status_bits = 4;
            data_cursor = (uint32_t *)0x800ea18;
            /* Repeat while entry_count_or_status != 0; the body advances or polls the state needed to leave the loop. */
            do {
                data_cursor = data_cursor + 1;
                operation_status = *data_cursor;
                if (requested_record_type <= operation_status) {
                    record_type_or_response_offset = status_bits + 5;
                    if (operation_status == 0x803) {
                        entry_length_or_status = 0xa0;
                    } else if (operation_status == 0x805) {
                        entry_length_or_status = 1;
                    } else if (operation_status == 0x806) {
                        entry_length_or_status = 0x20;
                    } else {
                        entry_length_or_status = 0;
                    }
                    if ((uint32_t)(record_type_or_response_offset + entry_length_or_status) <= (uint32_t)*core_process_ti_memset_for_build_scsi_mode_result) {
                        if (operation_status == 0x803) {
                            entry_length_or_status = core_transition_clear_memory_transition_context(working_buffer, record_type_or_response_offset + *(int *)(core_process_ti_memset_for_build_scsi_mode_result + 2), 0);
                            if (entry_length_or_status == 0) {
                                response_code = 2;
                                entry_length_or_status = 0xa0;
                                goto core_branch_target_015;
                            }
                        } else if (operation_status == 0x805) {
                            entry_length_or_status = core_transition_clear_memory_transition_context(working_buffer, 0, entry_value_bytes);
                            if (entry_length_or_status == 0) {
                                core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, (int)(int16_t)record_type_or_response_offset, (int)(int16_t)record_type_or_response_offset,
                                                 record_payload_byte);
                                entry_length_or_status = 1;
                                response_code = 0;
                                goto core_branch_target_015;
                            }
                        } else if ((operation_status == 0x806) &&
                                   (entry_length_or_status = core_process_clear_memory_buffer_compare_context(working_buffer, record_type_or_response_offset + *(int *)(core_process_ti_memset_for_build_scsi_mode_result + 2)),
                                    entry_length_or_status == 0)) {
                            entry_length_or_status = 0x20;
                            response_code = 1;
                        /* Shared target for core branch target 015; incoming paths preserve the same state assumptions. */
                        core_branch_target_015:
                            working_result = (int16_t)status_bits;
                            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, (int)working_result, (int)(int16_t)(working_result + 1), operation_status);
                            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, (int)(int16_t)(working_result + 2), (int)(int16_t)(working_result + 2),
                                             response_code);
                            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, (int)(int16_t)(working_result + 3), (int)(int16_t)(working_result + 4),
                                             entry_length_or_status);
                            status_bits = entry_length_or_status + record_type_or_response_offset;
                        }
                    }
                }
                entry_count_or_status = entry_count_or_status + -1;
            } while (entry_count_or_status != 0);
            goto core_handle_payload_byte_type_or_response_payload_response_context_before_store_duplicate_record_value;
        }
        entry_count_or_status = 3;
        data_cursor = (uint32_t *)0x800ea18;
        /* Repeat while entry_count_or_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            data_cursor = data_cursor + 1;
            if (requested_record_type == *data_cursor)
                goto core_handle_payload_byte_type_or_response_payload_response_context_before_get_runtime_context_base_for_build_scsi_mode;
            entry_count_or_status = entry_count_or_status + -1;
        } while (entry_count_or_status != 0);
        *buffer_cursor = 8;
    /* Shared target for core branch target 018; incoming paths preserve the same state assumptions. */
    core_branch_target_018:
        core_process_ti_memset_for_build_scsi_mode_result = (uint16_t *)core_process_ti_memset_for_build_scsi_mode(buffer_cursor, 4);
        core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 1, 2);
        response_code = 3;
        entry_count_or_status = 1;
    } else {
        if (status_bits == 1) {
            entry_count_or_status = core_get_runtime_context_base_for_build_scsi_mode();
            entry_count_or_status = core_initialize_output_record_from_source(entry_count_or_status + 0x2c, working_buffer);
            if (entry_count_or_status == 0)
                goto core_handle_payload_byte_type_or_response_payload_response_context_before_update_release_record_payload;
            core_process_ti_memset_for_build_scsi_mode_result = (uint16_t *)core_process_ti_memset_for_build_scsi_mode(buffer_cursor, 10);
            entry_count_or_status = 3;
            status_bits = 4;
            record_type_cursor = (int *)0x800ea18;
            /* Repeat while entry_count_or_status != 0; the body advances or polls the state needed to leave the loop. */
            do {
                record_type_cursor = record_type_cursor + 1;
                record_type_or_response_offset = *record_type_cursor;
                if ((record_type_or_response_offset == 0x803) || (record_type_or_response_offset == 0x805)) {
                    entry_length_or_status = core_transition_clear_memory_transition_context(working_buffer, 0, 0);
                /* Shared target for core branch target 013; incoming paths preserve the same state assumptions. */
                core_branch_target_013:
                    if (entry_length_or_status == 0) {
                        core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, (int)(int16_t)status_bits, (int)(int16_t)((int16_t)status_bits + 1),
                                         record_type_or_response_offset);
                        status_bits = status_bits + 2;
                    }
                } else if (record_type_or_response_offset == 0x806) {
                    entry_length_or_status = core_process_clear_memory_buffer_compare_context(working_buffer, 0);
                    goto core_branch_target_013;
                }
                entry_count_or_status = entry_count_or_status + -1;
            } while (entry_count_or_status != 0);
        /* Shared target for core handle payload byte type or response payload response context before store duplicate record value; incoming paths preserve the same state assumptions. */
        core_handle_payload_byte_type_or_response_payload_response_context_before_store_duplicate_record_value:
            core_store_duplicate_record_value(buffer_cursor, status_bits & 0xffff);
            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 3, status_bits - 4);
            core_reset_output_record(working_buffer);
            goto core_handle_payload_byte_type_or_response_payload_response_context_before_update_release_record_payload;
        }
        status_bits = status_bits - 2;
        if (status_bits < 2)
            goto core_branch_target_018;
        if (status_bits != 3) {
            response_code = 8;
            goto core_branch_target_017;
        }
        core_process_ti_memset_for_build_scsi_mode_result = (uint16_t *)core_process_ti_memset_for_build_scsi_mode(buffer, 10);
        entry_length_or_status = 3;
        core_payload_response_context_pointer = (uint32_t *)&core_payload_response_context;
        record_type_or_response_offset = 4;
        /* Repeat while entry_length_or_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            entry_count_or_status = record_type_or_response_offset;
            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, (int)(int16_t)entry_count_or_status, (int)(int16_t)((int16_t)entry_count_or_status + 1),
                             *core_payload_response_context_pointer);
            entry_length_or_status = entry_length_or_status + -1;
            core_payload_response_context_pointer = core_payload_response_context_pointer + 1;
            record_type_or_response_offset = entry_count_or_status + 2;
        } while (entry_length_or_status != 0);
        entry_count_or_status = entry_count_or_status + -2;
        response_code = 0;
    }
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, response_code, 3, entry_count_or_status);
/* Shared target for core handle payload byte type or response payload response context before update release record payload; incoming paths preserve the same state assumptions. */
core_handle_payload_byte_type_or_response_payload_response_context_before_update_release_record_payload:
    core_update_release_record_payload(core_process_ti_memset_for_build_scsi_mode_result, buffer_cursor, 10, 0xd);
    return;
}

