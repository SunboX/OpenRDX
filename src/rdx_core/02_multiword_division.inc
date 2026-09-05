/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Divide multiword unsigned.
 *
 * @param quotient_words Procedure input.
 * @param remainder_words Procedure input.
 * @param dividend_words Procedure input.
 * @param dividend_word_count Procedure input.
 * @param divisor_words Procedure input.
 * @param divisor_word_count Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_divide_multiword_unsigned(quotient_words, remainder_words, dividend_words, dividend_word_count, divisor_words, divisor_word_count)
int quotient_words;
uint32_t *remainder_words;
uint32_t dividend_words;
uint32_t dividend_word_count;
uint32_t *divisor_words;
uint32_t divisor_word_count;

{
    /* Divide multiword unsigned using only caller-provided data. */
    int operation_status;
    uint32_t core_divide_multiword_by_word_result;
    int descriptor_status_or_count;
    uint32_t status_bits;
    uint32_t remaining_count;
    uint32_t *data_cursor;
    int *buffer_1_cursor;
    int remaining_entries;
    int *context_derived;
    uint32_t *destination_cursor;
    uint32_t *descriptor_cursor;
    uint32_t descriptor_offset;
    int descriptor_index;
    int core_shift_multiword_left_result;
    int working_result;
    int adjusted_value;
    int carry_value;
    int descriptors_remaining;
    int initial_result;
    int current_descriptor_index;
    uint32_t *current_source_cursor;
    int descriptor_count_delta;
    int copied_descriptor_count;
    uint32_t *buffer_2_cursor;
    uint32_t *source_descriptor_base;
    uint32_t *reverse_source_cursor;
    int *previous_source_cursor;

    buffer_2_cursor = divisor_words;
    core_zero_word_buffer(quotient_words, dividend_word_count);
    source_descriptor_base = remainder_words;
    core_zero_word_buffer();
    operation_status = core_find_last_nonzero_word(divisor_words, divisor_word_count);
    descriptor_count_delta = core_find_last_nonzero_word(dividend_words, dividend_word_count);
    descriptor_count_delta = descriptor_count_delta - operation_status;
    if (operation_status == 0) {
        return 0xffffffff;
    }
    if (operation_status == 1) {
        core_divide_multiword_by_word_result = core_divide_multiword_by_word(quotient_words, dividend_words, *divisor_words, dividend_word_count);
        *remainder_words = core_divide_multiword_by_word_result;
        return 0;
    }
    if (descriptor_count_delta < 0) {
    /* Shared target for core multiword division copy remainder; incoming paths preserve the same state assumptions. */
    core_multiword_division_copy_remainder:
        core_copy_word_buffer(remainder_words, dividend_words, dividend_word_count);
    } else {
        if (descriptor_count_delta == 0) {
            descriptor_status_or_count = core_compare_multiword_integers(dividend_words, divisor_words, operation_status);
            if (descriptor_status_or_count < 0)
                goto core_multiword_division_copy_remainder;
            if (descriptor_status_or_count == 0) {
                core_initialize_word_buffer(quotient_words, 1, dividend_word_count);
                return 0;
            }
        }
        descriptor_cursor = buffer_2_cursor;
        descriptor_status_or_count = 0x20;
        copied_descriptor_count = 0;
        status_bits = 0x80000000;
        /* Repeat while descriptor_status_or_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((status_bits & buffer_2_cursor[operation_status + -1]) != 0)
                break;
            descriptor_status_or_count = descriptor_status_or_count + -1;
            status_bits = status_bits >> 1;
            copied_descriptor_count = copied_descriptor_count + 1;
        } while (descriptor_status_or_count != 0);
        core_shift_multiword_left(buffer_2_cursor, buffer_2_cursor, copied_descriptor_count, operation_status);
        descriptor_status_or_count = descriptor_count_delta;
        core_shift_multiword_left_result = core_shift_multiword_left(remainder_words, dividend_words);
        descriptors_remaining = descriptor_status_or_count + 1;
        context_derived = (int *)(descriptor_status_or_count * 4 + quotient_words);
        reverse_source_cursor = remainder_words + descriptor_status_or_count;
        buffer_1_cursor = (int *)remainder_words + descriptor_status_or_count + operation_status + -1;
        /* Repeat while remaining_entries != 0; the body advances or polls the state needed to leave the loop. */
        do {
            previous_source_cursor = buffer_1_cursor + -1;
            working_result = core_shift_multiword_left_result;
            core_shift_multiword_left_result = *buffer_1_cursor;
            descriptor_status_or_count = core_divide_doubleword_by_word(&descriptor_index, &descriptor_offset, &core_shift_multiword_left_result, descriptor_cursor[operation_status + -1]);
            if (descriptor_status_or_count == 0) {
            /* Shared target for core multiword division correct quotient estimate; incoming paths preserve the same state assumptions. */
            core_multiword_division_correct_quotient_estimate:
                descriptor_status_or_count = core_get_range_end_range_start(descriptor_index, descriptor_offset, descriptor_cursor[operation_status + -2], *previous_source_cursor);
                if (descriptor_status_or_count != 0) {
                    descriptor_index = descriptor_index + -1;
                    descriptor_offset = descriptor_cursor[operation_status + -1] + descriptor_offset;
                    if (((uint32_t)descriptor_cursor[operation_status + -1] <= descriptor_offset) &&
                        (descriptor_status_or_count =
                             core_get_range_end_range_start(descriptor_index, descriptor_offset, descriptor_cursor[operation_status + -2], *previous_source_cursor),
                         descriptor_status_or_count != 0)) {
                        descriptor_index = descriptor_index + -1;
                    }
                }
            } else {
                descriptor_index = -1;
                descriptor_offset = descriptor_cursor[operation_status + -1] + *buffer_1_cursor;
                if ((uint32_t)descriptor_cursor[operation_status + -1] <= descriptor_offset)
                    goto core_multiword_division_correct_quotient_estimate;
            }
            current_descriptor_index = descriptor_index;
            initial_result = working_result;
            current_source_cursor = reverse_source_cursor;
            descriptor_status_or_count = working_result;
            if (descriptor_index != 0) {
                descriptor_status_or_count = 0;
                data_cursor = reverse_source_cursor;
                remaining_entries = operation_status;
                destination_cursor = buffer_2_cursor;
                /* Repeat while remaining_entries != 0; the body advances or polls the state needed to leave the loop. */
                do {
                    core_multiply_u32_to_u64(&adjusted_value, current_descriptor_index, *destination_cursor);
                    status_bits = *data_cursor;
                    *data_cursor = status_bits - descriptor_status_or_count;
                    remaining_count = (uint32_t)(-descriptor_status_or_count - 1U < status_bits - descriptor_status_or_count);
                    status_bits = *data_cursor;
                    *data_cursor = status_bits - adjusted_value;
                    if (-adjusted_value - 1U < status_bits - adjusted_value) {
                        remaining_count = remaining_count + 1;
                    }
                    remaining_entries = remaining_entries + -1;
                    descriptor_status_or_count = remaining_count + carry_value;
                    data_cursor = data_cursor + 1;
                    destination_cursor = destination_cursor + 1;
                } while (remaining_entries != 0);
                descriptor_status_or_count = initial_result - descriptor_status_or_count;
            }
            *context_derived = descriptor_index;
            if (descriptor_status_or_count != 0) {
                *context_derived = descriptor_index + -1;
                core_add_multiword_unsigned(current_source_cursor, current_source_cursor, buffer_2_cursor, operation_status);
            }
            core_shift_multiword_left_result = *buffer_1_cursor;
            context_derived = context_derived + -1;
            descriptors_remaining = descriptors_remaining + -1;
            reverse_source_cursor = reverse_source_cursor + -1;
            buffer_1_cursor = previous_source_cursor;
        } while (descriptors_remaining != 0);
        if (0 < descriptor_count_delta) {
            descriptor_status_or_count = descriptor_count_delta;
            descriptor_cursor = source_descriptor_base + operation_status;
            /* Repeat while descriptor_status_or_count != 0; the body advances or polls the state needed to leave the loop. */
            do {
                descriptor_status_or_count = descriptor_status_or_count + -1;
                *descriptor_cursor = 0;
                descriptor_cursor = descriptor_cursor + 1;
            } while (descriptor_status_or_count != 0);
        }
        core_shift_multiword_right(source_descriptor_base, source_descriptor_base, copied_descriptor_count, operation_status);
        core_shift_multiword_right(buffer_2_cursor, buffer_2_cursor);
    }
    return 0;
}

/**
 * @brief Handle SCSI write attribute.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_handle_scsi_write_attribute(buffer) int *buffer;

{
    /* Handle SCSI write attribute using only caller-provided data. */
    int16_t working_result;
    uint16_t operation_status;
    uint32_t core_release_record_payload_when_short_result;
    uint32_t core_get_transfer_buffer_capacity_result;
    int response_code;
    uint32_t entry_type;
    uint8_t *data_cursor;
    uint32_t entry_span;
    uint32_t remaining_count;
    uint16_t highest_entry_value;
    uint8_t working_buffer[44];
    uint8_t entry_attribute_bytes[2];
    volatile uint8_t selected_entry_type = 0;
    volatile uint8_t matching_entry_type = 0;
    uint16_t range_words[2];
    int entry_data_base;
    uint8_t entry_table[20];
    uint8_t *selected_entry;
    uint8_t *previous_entry;
    uint8_t *current_entry;
    uint16_t core_release_record_buffer_result;
    uint16_t entry_length;
    volatile uint16_t record_offset_snapshot = 0;

    core_release_record_payload_when_short_result = core_release_record_payload_when_short(buffer + 1, 10, 0xd);
    core_get_transfer_buffer_capacity_result = core_get_transfer_buffer_capacity();
    if (core_get_transfer_buffer_capacity_result < core_release_record_payload_when_short_result) {
        response_code = 8;
    /* Shared target for core attribute update store validation error; incoming paths preserve the same state assumptions. */
    core_attribute_update_store_validation_error:
        *buffer = response_code;
        return;
    }
    response_code = core_get_active_command_response_buffer(buffer, range_words, core_release_record_payload_when_short_result & 0xffff);
    if (response_code == 0) {
        response_code = 0x1f;
        goto core_attribute_update_store_validation_error;
    }
    core_release_record_payload_when_short_result = core_release_record_payload_when_short(buffer + 1, 10, 0xd);
    ti_memset(entry_table, 0, 0x20);
    *buffer = 0;
    if (range_words[0] < core_release_record_payload_when_short_result) {
        core_release_record_payload_when_short_result = (uint32_t)range_words[0];
    }
    if (core_release_record_payload_when_short_result < 5) {
    /* Shared target for core attribute update apply entries; incoming paths preserve the same state assumptions. */
    core_attribute_update_apply_entries:
        response_code = core_get_runtime_context_base_for_build_scsi_mode();
        response_code = core_initialize_output_record_from_source(response_code + 0x2c, working_buffer);
        if (response_code != 0) {
            if ((current_entry != (uint8_t *)0x0) &&
                (response_code = core_process_buffer_compare_context(working_buffer, current_entry[2], entry_data_base + (uint32_t)*(uint16_t *)(current_entry + 4)),
                 response_code == 0)) {
                *buffer = 0x23;
            }
            if (selected_entry != (uint8_t *)0x0) {
                ti_memset(entry_attribute_bytes, 0, 4);
                selected_entry_type = selected_entry[2];
                if ((previous_entry != (uint8_t *)0x0) && (*(int16_t *)(previous_entry + 2) != 0)) {
                    matching_entry_type = *(uint8_t *)(entry_data_base + (uint32_t)*(uint16_t *)(previous_entry + 4));
                }
                response_code = core_normalize_transfer_status_fields(working_buffer, entry_data_base + (uint32_t)*(uint16_t *)(selected_entry + 4), entry_attribute_bytes);
                if (response_code == 0) {
                    *buffer = 0x23;
                }
            }
            core_reset_output_record(working_buffer);
            goto core_attribute_update_finalize_response;
        }
        response_code = 0x21;
    } else {
        if (core_release_record_payload_when_short_result < 0x8000) {
            highest_entry_value = 0;
            core_get_transfer_buffer_capacity_result = 4;
            remaining_count = 0;
            response_code = 0;
            /* Repeat while core_get_transfer_buffer_capacity_result < core_release_record_payload_when_short_result; the body advances or polls the state needed to leave the loop. */
            do {
                if (response_code != 0)
                    goto core_attribute_update_finalize_response;
                if ((range_words[0] < 5) || (range_words[0] - 5 < core_get_transfer_buffer_capacity_result)) {
                /* Shared target for core attribute update reject entry; incoming paths preserve the same state assumptions. */
                core_attribute_update_reject_entry:
                    *buffer = 10;
                    response_code = 10;
                } else {
                    working_result = (int16_t)core_get_transfer_buffer_capacity_result;
                    core_release_record_buffer_result = core_release_record_buffer(range_words, (int)working_result, (int)(int16_t)(working_result + 1));
                    entry_length = core_release_record_buffer(range_words, (int)(int16_t)(working_result + 3), (int)(int16_t)(working_result + 4));
                    operation_status = core_release_record_buffer_result;
                    core_get_transfer_buffer_capacity_result = core_get_transfer_buffer_capacity_result + 5;
                    record_offset_snapshot = (uint16_t)core_get_transfer_buffer_capacity_result;
                    response_code = core_find_value_in_fixed_table(core_release_record_buffer_result);
                    if (response_code == 0)
                        goto core_attribute_update_reject_entry;
                    if (operation_status == 0x803) {
                        entry_type = 0xa0;
                    } else if (operation_status == 0x805) {
                        entry_type = 1;
                    } else if (operation_status == 0x806) {
                        entry_type = 0x20;
                    } else {
                        entry_type = 0;
                    }
                    if (((operation_status <= highest_entry_value) ||
                         (entry_span = (uint32_t)entry_length, entry_type < entry_span)) ||
                        (core_release_record_payload_when_short_result - core_get_transfer_buffer_capacity_result < entry_span))
                        goto core_attribute_update_reject_entry;
                    core_get_transfer_buffer_capacity_result = entry_span + core_get_transfer_buffer_capacity_result;
                    highest_entry_value = operation_status;
                    if (operation_status == 0x803) {
                        data_cursor = entry_table + remaining_count * 6;
                        selected_entry = data_cursor;
                    /* Shared target for core attribute update append entry metadata; incoming paths preserve the same state assumptions. */
                    core_attribute_update_append_entry_metadata:
                        if (2 < remaining_count)
                            goto core_attribute_update_reject_entry;
                        core_copy_bytes(data_cursor, &core_release_record_buffer_result, 6);
                        remaining_count = remaining_count + 1;
                    } else {
                        if (operation_status == 0x805) {
                            data_cursor = entry_table + remaining_count * 6;
                            previous_entry = data_cursor;
                            goto core_attribute_update_append_entry_metadata;
                        }
                        if (operation_status == 0x806) {
                            data_cursor = entry_table + remaining_count * 6;
                            current_entry = data_cursor;
                            goto core_attribute_update_append_entry_metadata;
                        }
                    }
                    response_code = *buffer;
                }
            } while (core_get_transfer_buffer_capacity_result < core_release_record_payload_when_short_result);
            if (response_code != 0)
                goto core_attribute_update_finalize_response;
            goto core_attribute_update_apply_entries;
        }
        response_code = 10;
    }
    *buffer = response_code;
/* Shared target for core attribute update finalize response; incoming paths preserve the same state assumptions. */
core_attribute_update_finalize_response:
    core_finalize_command_response_hook(buffer);
    return;
}

/**
 * @brief Finalize identity initialization hook.
 *
 * @return No value.
 */
void core_finalize_identity_initialization_hook()

{
    /* Finalize identity initialization hook using only caller-provided data. */
    return;
}

/**
 * @brief Handle device descriptor response flag device descriptor context.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_device_descriptor_response_flag_device_descriptor_context(context)
uint32_t context;

{
    /* Handle device descriptor response flag device descriptor context; persistent state is carried in core primary descriptor state. */
    bool condition_met;
    uint16_t core_append_typed_payload_record_result;
    int operation_status;
    int lookup_status;
    uint32_t core_get_0xc_for_device_descriptor_response_result;
    uint16_t *core_initialize_scsi_response_buffer_result;
    int callback_result;
    uint32_t core_primary_descriptor_state_snapshot;
    uint32_t working_result;
    uint32_t entry_index_or_flags;
    char match_count;
    uint32_t response_flag;
    uint8_t working_buffer[4];
    volatile uint32_t selection_mode = 0;
    uint8_t entry_metadata[8];
    uint16_t entry_length;
    uint8_t device_descriptor[44];
    uint8_t previous_entry_metadata[12];
    uint8_t candidate_entry_metadata[12];
    int core_process_command_phase_three_result;
    uint32_t operation_result;

    core_process_command_phase_three_result = core_process_command_phase_three();
    match_count = '\0';
    working_result = 1;
    core_primary_descriptor_state_snapshot = 0x10;
    response_flag = 0;
    condition_met = false;
    if (core_process_command_phase_three_result == 0) {
        operation_status = 0;
        operation_result = 0x1a;
    } else {
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, device_descriptor);
        operation_result = 0x21;
        if (operation_status != 0) {
            entry_index_or_flags = 0;
            /* Repeat while (int)entry_index_or_flags < 8; the body advances or polls the state needed to leave the loop. */
            do {
                lookup_status = core_validate_device_descriptor_length(device_descriptor, entry_index_or_flags & 0xff, working_buffer, candidate_entry_metadata);
                if ((lookup_status != 0) && (lookup_status = core_has_device_descriptor_high_bit(working_buffer), lookup_status != 0)) {
                    core_get_0xc_for_device_descriptor_response_result = core_get_0xc_for_device_descriptor_response(working_buffer);
                    core_primary_descriptor_state_snapshot = callback_result + core_primary_descriptor_state_snapshot + 4;
                    if (((core_get_0xc_for_device_descriptor_response_result >> 5 & 1) != 0) &&
                        (lookup_status = core_pack_descriptor_validation_result(device_descriptor, entry_index_or_flags & 0xff, entry_metadata),
                         lookup_status != 0)) {
                        core_primary_descriptor_state_snapshot = entry_length + core_primary_descriptor_state_snapshot + 4;
                    }
                    core_get_0xc_for_device_descriptor_response_result = core_get_0xc_for_device_descriptor_response(working_buffer);
                    match_count = match_count + '\x01';
                    if ((core_get_0xc_for_device_descriptor_response_result >> 3 & 1) == 0) {
                        condition_met = true;
                    }
                }
                entry_index_or_flags = entry_index_or_flags + 1;
            } while ((int)entry_index_or_flags < 8);
        }
    }
    core_initialize_scsi_response_buffer_result = (uint16_t *)core_initialize_scsi_response_buffer(context, core_primary_descriptor_state_snapshot & 0xffff);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0, 1, 0x21);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 2, 3, core_primary_descriptor_state_snapshot - 4);
    if ((core_process_command_phase_three_result != 0) && (operation_status != 0)) {
        if (condition_met) {
            if (core_primary_descriptor_state == 2 || core_primary_descriptor_state == 3) {
                working_result = 5;
            } else {
                working_result = 6;
            }
            response_flag = 1;
        } else {
            working_result = 3;
        }
    }
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0xc, 0xc, working_result);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0xd, 0xd, response_flag);
    if ((core_process_command_phase_three_result == 0) || (match_count == '\0')) {
    /* Shared target for core descriptor response check source status; incoming paths preserve the same state assumptions. */
    core_descriptor_response_check_source_status:
        if (operation_status == 0)
            goto core_descriptor_response_preserve_initialization_error;
    } else {
        if (operation_status != 0) {
            core_append_typed_payload_record_result = 0x10;
            core_primary_descriptor_state_snapshot = 0;
            /* Repeat while (int)core_primary_descriptor_state_snapshot < 8; the body advances or polls the state needed to leave the loop. */
            do {
                lookup_status = core_validate_device_descriptor_length(device_descriptor, core_primary_descriptor_state_snapshot & 0xff, working_buffer, previous_entry_metadata);
                if ((lookup_status != 0) && (lookup_status = core_has_device_descriptor_high_bit(working_buffer), lookup_status != 0)) {
                    entry_index_or_flags = core_get_0xc_for_device_descriptor_response(working_buffer);
                    selection_mode = 2;
                    if ((entry_index_or_flags >> 5 & 1) == 0) {
                        if ((core_primary_descriptor_state_snapshot == 0) && (core_primary_descriptor_state == 3)) {
                            selection_mode = 3;
                        }
                    } else {
                        if ((core_primary_descriptor_state_snapshot == 0) && (core_primary_descriptor_state == 2 || core_primary_descriptor_state == 3)) {
                            selection_mode = 1;
                        }
                        lookup_status = core_pack_descriptor_validation_result(device_descriptor, core_primary_descriptor_state_snapshot & 0xff, entry_metadata);
                        if (lookup_status != 0) {
                            core_append_typed_payload_record_result = core_append_typed_payload_record(*(uint32_t *)(core_initialize_scsi_response_buffer_result + 2), *core_initialize_scsi_response_buffer_result, entry_metadata,
                                                                core_append_typed_payload_record_result);
                        }
                    }
                    core_append_typed_payload_record_result =
                        core_append_typed_payload_record(*(uint32_t *)(core_initialize_scsi_response_buffer_result + 2), *core_initialize_scsi_response_buffer_result, working_buffer, core_append_typed_payload_record_result);
                }
                core_primary_descriptor_state_snapshot = core_primary_descriptor_state_snapshot + 1;
            } while ((int)core_primary_descriptor_state_snapshot < 8);
            goto core_descriptor_response_check_source_status;
        }
    /* Shared target for core descriptor response preserve initialization error; incoming paths preserve the same state assumptions. */
    core_descriptor_response_preserve_initialization_error:
        if (core_process_command_phase_three_result != 0)
            goto core_descriptor_response_release_record_and_return;
    }
    operation_result = 0;
/* Shared target for core descriptor response release record and return; incoming paths preserve the same state assumptions. */
core_descriptor_response_release_record_and_return:
    if (operation_status != 0) {
        core_reset_output_record(device_descriptor);
    }
    return operation_result;
}

/**
 * @brief Dispatch RDX management record.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_dispatch_rdx_management_record(buffer, value, length, flags)
uint16_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Dispatch RDX management record using only caller-provided data. */
    bool condition_met;
    char core_release_record_buffer_result;
    char secondary_mode;
    int16_t operation_status;
    uint16_t payload_length;
    uint32_t format_flags;
    char *identifier_cursor;
    int core_parse_typed_payload_record_result;
    int comparison_result_or_count;
    uint32_t working_result;
    uint16_t *status_bits;
    uint32_t value_derived;
    uint32_t available_length;
    volatile uint32_t request_flags_copy = 0;

    working_result = 8;
    value_derived = value;
    if ((0x13 < *buffer) && (status_bits = buffer, available_length = length, request_flags_copy = flags,
                             operation_status = core_release_record_buffer(buffer, 0, 1), operation_status == 0x10)) {
        payload_length = core_release_record_buffer(buffer, 2, 3);
        format_flags = core_release_record_buffer(buffer, 4, 4);
        core_release_record_buffer_result = core_release_record_buffer(buffer, 5, 5);
        if (((format_flags & 0xfe) == 0x40) && ((core_release_record_buffer_result == '\x04' && (0xf < payload_length)))) {
            core_release_record_buffer_result = core_release_record_buffer(buffer, 8, 8);
            secondary_mode = core_release_record_buffer(buffer, 9, 9);
            condition_met = false;
            comparison_result_or_count = 8;
            identifier_cursor = (char *)(*(int *)(buffer + 2) + 10);
            /* Repeat while comparison_result_or_count != 0; the body advances or polls the state needed to leave the loop. */
            do {
                /* Gate the following state transition on *identifier_cursor != '\0'; the handler is skipped when this condition is false. */
                if (*identifier_cursor != '\0') {
                    condition_met = true;
                }
                comparison_result_or_count = comparison_result_or_count + -1;
                identifier_cursor = identifier_cursor + 1;
            } while (comparison_result_or_count != 0);
            if (((!condition_met) && (secondary_mode == '\x01')) && (core_release_record_buffer_result == '\x01')) {
                core_release_record_buffer_result = core_release_record_buffer(buffer, 6, 6);
                secondary_mode = core_release_record_buffer(buffer, 7, 7);
                working_result = 0x28;
                comparison_result_or_count = 0;
                if (secondary_mode == '\0' && (core_release_record_buffer_result == '\0' && (format_flags & 1) == 0)) {
                    comparison_result_or_count = 1;
                } else if ((format_flags & 1) == 1) {
                    if (core_release_record_buffer_result == '\x01' && secondary_mode == '\x01') {
                        comparison_result_or_count = 2;
                    } else if ((core_release_record_buffer_result == '\x02') && (secondary_mode == '\x02')) {
                        comparison_result_or_count = 3;
                    }
                }
                if (payload_length < 0x15) {
                    operation_status = 0;
                } else {
                    operation_status = core_release_record_buffer(buffer, 0x12, 0x13);
                    core_parse_typed_payload_record_result = core_parse_typed_payload_record(&status_bits, *(int *)(buffer + 2) + 0x14, 0, operation_status);
                    if (core_parse_typed_payload_record_result != 0) {
                        operation_status = (int16_t)available_length;
                    }
                }
                if ((comparison_result_or_count == 2) || (comparison_result_or_count == 3)) {
                    if (operation_status == 0)
                        goto core_management_record_return_dispatch_result;
                    if (status_bits == (uint16_t *)0xc0) {
                        working_result = core_process_runtime_context_base_for_rdx_management_record(buffer, comparison_result_or_count, &status_bits);
                        goto core_management_record_return_dispatch_result;
                    }
                    if ((status_bits == (uint16_t *)0xc2) || (status_bits == (uint16_t *)0xc4)) {
                        working_result = core_process_secondary_command_context(status_bits, comparison_result_or_count, &status_bits);
                        goto core_management_record_return_dispatch_result;
                    }
                    if (status_bits != (uint16_t *)0xc5 && status_bits != (uint16_t *)0xc6)
                        goto core_management_record_return_dispatch_result;
                } else {
                    if (comparison_result_or_count != 1)
                        goto core_management_record_return_dispatch_result;
                    if (operation_status == 0) {
                        core_process_primary_command_context();
                        working_result = 0;
                        goto core_management_record_return_dispatch_result;
                    }
                    if (status_bits == (uint16_t *)0xc0) {
                        working_result = core_handle_lookup_key_metadata_device_descriptor_context(0xc0, &status_bits);
                        goto core_management_record_return_dispatch_result;
                    }
                    if (status_bits != (uint16_t *)0xc5)
                        goto core_management_record_return_dispatch_result;
                }
                working_result = 0x2b;
            }
        }
        return 0;
    }
/* Shared target for core management record return dispatch result; incoming paths preserve the same state assumptions. */
core_management_record_return_dispatch_result:
    return CONCAT44(value_derived, working_result);
}

/**
 * @brief Divide doubleword by word.
 *
 * @param high_output Procedure input.
 * @param low_output Procedure input.
 * @param range_descriptor Encoded range descriptor used for the calculation.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_divide_doubleword_by_word(high_output, low_output, range_descriptor, flags)
uint32_t *high_output;
uint32_t *low_output;
uint32_t *range_descriptor;
uint32_t flags;

{
    /* Divide doubleword by word using only caller-provided data. */
    int *data_cursor;
    uint32_t status_bits;
    uint32_t flags_snapshot;
    uint32_t scale_low_word;
    uint32_t quotient;
    uint32_t remaining_count;
    uint32_t buffer_3_cursor;
    uint32_t operation_result;
    bool input_word = 0;
    uint32_t range_high_word;
    uint32_t range_high_snapshot;
    uint32_t *buffer_1_cursor;
    uint32_t *selected_output;
    uint32_t range_low_word;

    if (input_word) {
        range_high_word = range_descriptor[1];
        flags_snapshot = flags >> 0x10;
        scale_low_word = flags & 0xffff;
        buffer_3_cursor = *range_descriptor >> 0x10;
        range_low_word = *range_descriptor & 0xffff;
        quotient = (range_high_word >> 0x10) / flags_snapshot;
        operation_result = (range_high_word >> 0x10) / flags_snapshot;
        if ((quotient != 0) &&
            ((range_high_word & 0xffff | (range_high_word & 0xffff0000) - quotient * flags_snapshot * 0x10000U) <
             scale_low_word * quotient)) {
            operation_result = quotient - 1;
        }
        range_high_snapshot = 0;
        buffer_1_cursor = high_output;
        selected_output = low_output;
        if ((operation_result != 0) &&
            (data_cursor = (int *)core_subtract_split_product(&range_high_word, operation_result, flags_snapshot, scale_low_word),
             range_high_snapshot >> 0x10 != 0)) {
            operation_result = operation_result - 1;
            *data_cursor = *data_cursor + flags;
        }
        quotient = range_high_word / flags_snapshot;
        if ((quotient == 0x10000) ||
            (remaining_count = quotient,
             (buffer_3_cursor | (range_high_word - flags_snapshot * quotient) * 0x10000) < scale_low_word * quotient)) {
            status_bits = flags_snapshot + (range_high_word - flags_snapshot * quotient);
            remaining_count = quotient - 1;
            if ((status_bits >> 0x10 == 0) &&
                ((buffer_3_cursor | status_bits * 0x10000) < scale_low_word * remaining_count)) {
                remaining_count = quotient - 2;
            }
        }
        range_high_snapshot = range_high_word >> 0x10;
        range_high_word = buffer_3_cursor | range_high_word << 0x10;
        data_cursor = (int *)core_subtract_split_product(&range_high_word, remaining_count, flags_snapshot, scale_low_word);
        if (range_high_snapshot >> 0x10 != 0) {
            remaining_count = remaining_count - 1;
            *data_cursor = *data_cursor + flags;
        }
        quotient = range_high_word / flags_snapshot;
        *buffer_1_cursor = remaining_count << 0x10;
        if ((quotient == 0x10000) ||
            (buffer_3_cursor = quotient,
             (range_low_word | (range_high_word - flags_snapshot * quotient) * 0x10000) < scale_low_word * quotient)) {
            remaining_count = flags_snapshot + (range_high_word - flags_snapshot * quotient);
            buffer_3_cursor = quotient - 1;
            if ((remaining_count >> 0x10 == 0) && ((range_low_word | remaining_count * 0x10000) < scale_low_word * buffer_3_cursor)) {
                buffer_3_cursor = quotient - 2;
            }
        }
        range_high_snapshot = range_high_word >> 0x10;
        range_high_word = range_low_word | range_high_word << 0x10;
        data_cursor = (int *)core_subtract_split_product(data_cursor, buffer_3_cursor, flags_snapshot, scale_low_word);
        if (range_high_snapshot >> 0x10 != 0) {
            buffer_3_cursor = buffer_3_cursor - 1;
            *data_cursor = *data_cursor + flags;
        }
        *buffer_1_cursor = buffer_3_cursor & 0xffff | *buffer_1_cursor;
    } else {
        range_high_word = 0;
        operation_result = 0xffffffff;
        *low_output = 0;
        selected_output = high_output;
    }
    *selected_output = range_high_word;
    return operation_result;
}

/**
 * @brief Reset command context hook.
 *
 * @return No value.
 */
void core_reset_command_context_hook()

{
    /* Reset command context hook using only caller-provided data. */
    return;
}

/**
 * @brief Submit SCSI read write ATA FIS.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_submit_scsi_read_write_ata_fis(buffer)
uint32_t *buffer;

{
    /* Submit SCSI read write ATA FIS; persistent state is carried in usb active mass storage class, usb bot cbw pointer and related record fields. */
    uint16_t requested_block_count;
    int operation_status;
    uint32_t working_result;
    uint32_t buffer_cursor;
    uint32_t total_transfer_length;
    uint8_t status_bits;
    volatile uint8_t response_flags = 0;
    char a_derived;
    volatile uint8_t block_count_low_byte = 0;
    volatile uint8_t buffer_address_byte_zero = 0;
    volatile uint8_t buffer_address_byte_one = 0;
    volatile uint8_t buffer_address_byte_two = 0;
    volatile uint8_t command_flags = 0;
    uint8_t buffer_address_byte_three;
    volatile uint8_t buffer_word_low_byte = 0;
    volatile uint8_t buffer_word_high_byte = 0;
    volatile uint8_t block_count_high_byte = 0;
    volatile uint8_t transfer_count_low_byte = 0;
    volatile uint8_t transfer_count_high_byte = 0;
    volatile uint8_t reserved_command_byte = 0;
    volatile uint8_t reserved_address_byte = 0;
    volatile uint32_t reserved_command_word = 0;
    volatile uint8_t reserved_length_byte = 0;
    char transfer_mode;
    volatile uint8_t descriptor_valid_flag = 0;
    volatile int requested_transfer_bytes = 0;
    volatile uint8_t reserved_status_byte = 0;

    requested_block_count = *(uint16_t *)(buffer + 2);
    operation_status = (int)core_get_cartridge_ata_device_info();
    total_transfer_length = (uint32_t)requested_block_count * (uint32_t)*(uint16_t *)(operation_status + 0x10);
    working_result = usb_active_mass_storage_class;
    if ((usb_active_mass_storage_class == 0) && (working_result = *(uint32_t *)(usb_bot_cbw_pointer + 8), working_result < total_transfer_length)) {
        return 0x1f;
    }
    requested_block_count = *(uint16_t *)(buffer + 2);
    operation_status = (int)core_get_cartridge_ata_device_info(working_result);
    transfer_mode = *(char *)((int)buffer + 10);
    requested_transfer_bytes = (uint32_t)requested_block_count * (uint32_t)*(uint16_t *)(operation_status + 0x10);
    if (transfer_mode == '\0') {
        if (core_ata_lba48_supported == '\0') {
            a_derived = (uint8_t)-0x38;
        } else {
            a_derived = '%';
        }
    } else {
        if (core_ata_lba48_supported == '\0') {
            a_derived = (uint8_t)-0x36;
        } else {
            a_derived = '5';
        }
        if ((*(char *)((int)buffer + 0xb) != '\0') && (a_derived == '5')) {
            a_derived = '=';
        }
    }
    buffer_cursor = *buffer;
    response_flags = 0x80;
    status_bits = 0x27;
    buffer_address_byte_zero = (uint8_t)buffer_cursor;
    buffer_address_byte_one = (uint8_t)((uint32_t)buffer_cursor >> 8);
    buffer_address_byte_two = (uint8_t)((uint32_t)buffer_cursor >> 0x10);
    command_flags = 0x40;
    buffer_word_high_byte = (uint8_t)((uint32_t)buffer[1] >> 8);
    buffer_word_low_byte = (uint8_t)buffer[1];
    buffer_address_byte_three = (uint8_t)((uint32_t)buffer_cursor >> 0x18);
    reserved_address_byte = 0;
    descriptor_valid_flag = 1;
    if ((core_ata_native_command_queueing_supported == '\0') || (usb_active_mass_storage_class != 1)) {
        block_count_low_byte = 0;
        block_count_high_byte = 0;
        transfer_count_low_byte = (uint8_t)*(uint16_t *)(buffer + 2);
        if (core_ata_lba48_supported != '\0') {
            transfer_count_high_byte = (uint8_t)((uint16_t)*(uint16_t *)(buffer + 2) >> 8);
            goto core_ata_transfer_submit_command_descriptor;
        }
        command_flags = buffer_address_byte_three & 0xf | 0x40;
    } else {
        if (transfer_mode == '\0') {
            a_derived = '`';
        } else {
            a_derived = 'a';
        }
        transfer_count_low_byte = 0;
        block_count_low_byte = (uint8_t)*(uint16_t *)(buffer + 2);
        block_count_high_byte = (uint8_t)((uint16_t)*(uint16_t *)(buffer + 2) >> 8);
    }
    transfer_count_high_byte = 0;
/* Shared target for core ata transfer submit command descriptor; incoming paths preserve the same state assumptions. */
core_ata_transfer_submit_command_descriptor:
    reserved_command_byte = 0;
    reserved_length_byte = 0;
    reserved_status_byte = 0;
    reserved_command_word = 0;
    *(uint32_t *)(core_active_command_descriptor + 0xc) = total_transfer_length;
    operation_status = core_submit_scsi_ata_command(&status_bits);
    if (operation_status == 8) {
        buffer_cursor = 0;
    } else if (operation_status == 0xd) {
        buffer_cursor = 5;
    } else {
        buffer_cursor = 0x11;
    }
    return buffer_cursor;
}
