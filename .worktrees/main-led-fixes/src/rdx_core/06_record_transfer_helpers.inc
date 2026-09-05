/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Transfer comparison context.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
int core_transfer_comparison_context(offset)
int offset;

{
    /* Transfer comparison context; persistent state is carried in core comparison context. */
    char core_get_expected_candidate_result;
    uint16_t working_result;
    int operation_result;
    int core_handle_fallback_decoded_request_bytes_result;
    uint8_t *core_comparison_context_pointer;
    uint8_t *state_entry_cursor;
    char operation_status;
    int entry_index;
    int computed_offset;
    uint8_t working_buffer[12];
    uint8_t comparison_buffer[20];
    uint32_t comparison_value;

    operation_result = core_initialize_output_record_from_source(offset, working_buffer);
    if (operation_result == 0) {
        operation_result = 2;
    } else {
        computed_offset = offset + 0x10;
        operation_status = '\x1f';
        entry_index = 0;
        state_entry_cursor = (uint8_t *)&core_comparison_context;
        core_handle_fallback_decoded_request_bytes_result = 0;
        /* Repeat while core_handle_fallback_decoded_request_bytes_result != 0; the body advances or polls the state needed to leave the loop. */
        do {
            working_result = *(uint16_t *)(state_entry_cursor + 4);
            core_get_expected_candidate_result = *(char *)(computed_offset + (uint32_t)working_result);
            operation_result = core_handle_fallback_decoded_request_bytes_result;
            if (core_get_expected_candidate_result == '\x02' || core_get_expected_candidate_result == '\x04') {
                operation_result = core_get_expected_candidate(working_buffer, state_entry_cursor);
                if ((operation_result == 0) &&
                    (operation_result = core_process_x01_for_comparison_comparison_resource(computed_offset, state_entry_cursor, working_buffer), operation_result == 0)) {
                    operation_status = state_entry_cursor[1];
                    operation_result = core_handle_fallback_decoded_request_bytes_result;
                } else {
                    *(char *)(computed_offset + (uint32_t)working_result) = '\x04';
                }
            }
            entry_index = entry_index + 1;
            if ((operation_status != '\x1f') &&
                (((9 < entry_index || (state_entry_cursor[9] != operation_status)) &&
                  (core_handle_fallback_decoded_request_bytes_result = core_handle_fallback_decoded_request_bytes(comparison_value, comparison_buffer, working_buffer), core_handle_fallback_decoded_request_bytes_result == 0)))) {
                core_comparison_context_pointer = (uint8_t *)&core_comparison_context;
                core_handle_fallback_decoded_request_bytes_result = 10;
                /* Repeat while core_handle_fallback_decoded_request_bytes_result != 0; the body advances or polls the state needed to leave the loop. */
                do {
                    if ((operation_status == core_comparison_context_pointer[1]) &&
                        (core_get_expected_candidate_result = *(char *)(computed_offset + (uint32_t)*(uint16_t *)(core_comparison_context_pointer + 4)),
                         core_get_expected_candidate_result == '\x02' || core_get_expected_candidate_result == '\x04')) {
                        *(char *)(computed_offset + (uint32_t)*(uint16_t *)(core_comparison_context_pointer + 4)) = '\x01';
                    }
                    core_handle_fallback_decoded_request_bytes_result = core_handle_fallback_decoded_request_bytes_result + -1;
                    core_comparison_context_pointer = core_comparison_context_pointer + 8;
                } while (core_handle_fallback_decoded_request_bytes_result != 0);
                operation_status = '\x1f';
            }
            state_entry_cursor = state_entry_cursor + 8;
            core_handle_fallback_decoded_request_bytes_result = operation_result;
        } while (entry_index < 10);
        core_reset_output_record(working_buffer);
        if (operation_result == 0) {
            *(uint8_t *)(offset + 0x70) = 0;
        }
    }
    return operation_result;
}

/**
 * @brief Write response type.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_write_response_type(buffer, value, length, flags) uint32_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Write response type; persistent state is carried in core response type. */
    char working_result;
    uint32_t response_code;
    uint32_t core_release_record_payload_when_short_result;
    uint32_t core_get_constant_hex_1000_value_result;
    int operation_status;
    uint32_t callback_result;
    uint32_t operation_mode;
    uint8_t record_type;
    uint32_t length_component;
    uint32_t flags_snapshot;

    working_result = *(char *)(buffer[2] + 2);
    record_type = *(uint8_t *)(buffer[2] + 1);
    length_component = length;
    flags_snapshot = flags;
    response_code = core_release_record_payload_when_short(buffer + 1, 3, 5);
    core_release_record_payload_when_short_result = core_release_record_payload_when_short(buffer + 1, 6, 8);
    core_get_constant_hex_1000_value_result = core_get_constant_hex_1000_value();
    record_type = record_type & 0x1f;
    if (core_get_constant_hex_1000_value_result < core_release_record_payload_when_short_result) {
        response_code = 8;
    /* Shared target for core write response type before update command validation context; incoming paths preserve the same state assumptions. */
    core_write_response_type_before_update_command_validation_context:
        *buffer = response_code;
        return;
    }
    operation_status = core_update_command_validation_context(buffer, &length_component, core_release_record_payload_when_short_result & 0xffff);
    if (operation_status == 0) {
        response_code = 0x1f;
        goto core_write_response_type_before_update_command_validation_context;
    }
    if (record_type != 2) {
        if (record_type == 4) {
            operation_mode = 0;
        } else {
            if (record_type != 5) {
                if (record_type == 0xf)
                    goto core_branch_target_074;
                goto core_write_response_type_before_set_transfer_ready_flag_to_one;
            }
            operation_mode = 1;
        }
        response_code = core_process_command_queue_context(flags_snapshot, response_code, core_release_record_payload_when_short_result, operation_mode);
        goto core_branch_target_073;
    }
    if ((int8_t)working_result == -0x80) {
        operation_status = core_transfer_response_byte_stack_response_byte(flags_snapshot, core_release_record_payload_when_short_result);
        if (operation_status != 0) {
            core_response_type = '\x01';
            goto core_branch_target_074;
        }
    /* Shared target for core write response type before set transfer ready flag to one; incoming paths preserve the same state assumptions. */
    core_write_response_type_before_set_transfer_ready_flag_to_one:
        response_code = 8;
    } else if ((int8_t)working_result == -0x7e) {
        if (core_response_type == '\0')
            goto core_write_response_type_before_set_transfer_ready_flag_to_one;
        core_set_transfer_ready_flag_to_one();
        response_code = 0;
    } else {
        operation_status = core_get_response_type_value();
        if (operation_status == 0)
            goto core_write_response_type_before_set_transfer_ready_flag_to_one;
        operation_status = ((uint32_t (*)())core_transition_flash_validation_context)(callback_result, working_result);
        if (operation_status == 0)
            goto core_branch_target_074;
        response_code = 0x20;
    }
/* Shared target for core branch target 073; incoming paths preserve the same state assumptions. */
core_branch_target_073:
    *buffer = response_code;
/* Shared target for core branch target 074; incoming paths preserve the same state assumptions. */
core_branch_target_074:
    core_finalize_command_response_hook(buffer);
    return;
}

/**
 * @brief Configure word two byte one word two byte two.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
void core_configure_word_two_byte_one_word_two_byte_two(context, value, buffer) uint32_t context;
uint32_t value;
uint32_t *buffer;

{
    /* Configure word two byte one word two byte two using only caller-provided data. */
    uint8_t working_buffer[16];
    uint8_t digest_bytes[16];
    uint8_t encoded_context[8];
    volatile uint8_t source_word_two_byte_zero = 0;
    volatile uint8_t source_word_two_byte_one = 0;
    volatile uint8_t source_word_two_byte_two = 0;
    volatile uint8_t source_word_two_byte_three = 0;
    volatile uint8_t request_flags = 0;
    volatile uint8_t source_word_three_byte_one = 0;
    volatile uint8_t source_word_three_byte_two = 0;
    volatile uint8_t source_word_three_byte_three = 0;

    source_word_two_byte_zero = *(uint8_t *)(buffer + 2);
    source_word_two_byte_one = (uint8_t)((uint32_t)buffer[2] >> 8);
    source_word_two_byte_two = (uint8_t)((uint32_t)buffer[2] >> 0x10);
    source_word_two_byte_three = (uint8_t)((uint32_t)buffer[2] >> 0x18);
    request_flags = (uint8_t)buffer[3];
    source_word_three_byte_one = (uint8_t)((uint32_t)buffer[3] >> 8);
    source_word_three_byte_two = (uint8_t)((uint32_t)buffer[3] >> 0x10);
    source_word_three_byte_three = (uint8_t)((uint32_t)buffer[3] >> 0x18);
    core_forward_end_remaining(working_buffer, *buffer, 0x28);
    core_transfer_mixed_word_key_sum(working_buffer, 4, value);
    core_forward_end_remaining(context, working_buffer, 0x10);
    core_transfer_mixed_word_key_sum(digest_bytes, 4, context);
    core_forward_end_remaining(context, digest_bytes, 0x10);
    core_transfer_mixed_word_key_sum(encoded_context, 4, context);
    core_forward_end_remaining(context, encoded_context, 0x10);
    return;
}

/**
 * @brief Transfer firmware t byte.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_transfer_firmware_t_byte(buffer) uint32_t *buffer;

{
    /* Transfer firmware t byte using only caller-provided data. */
    uint8_t current_byte;
    int operation_status;
    int entry_index;
    uint32_t working_result;
    uint32_t response_code;

    *buffer = 0;
    operation_status = (int)core_transfer_spi_transfer_storage(*(uint8_t *)buffer[2]);
    if (operation_status == 0) {
        working_result = 7;
    /* Shared target for core transfer firmware t byte before updating buffer; incoming paths preserve the same state assumptions. */
    core_transfer_firmware_t_byte_before_updating_buffer:
        *buffer = working_result;
        goto core_transfer_firmware_t_byte_before_update_command_phase_auxiliary;
    }
    if ((*(uint32_t *)(operation_status + 8) >> 1 & 1) != 0) {
    /* Shared target for core transfer firmware t byte before updating entry index; incoming paths preserve the same state assumptions. */
    core_transfer_firmware_t_byte_before_updating_entry_index:
        /* Iterate while entry_index = 1; entry_index < (int)(uint32_t)*(uint8_t *)(buffer + 1); entry_index = entry_index + 1, preserving the defined cursor and bound. */
        for (entry_index = 1; entry_index < (int)(uint32_t)*(uint8_t *)(buffer + 1); entry_index = entry_index + 1) {
            if ((int)(uint32_t)*(uint8_t *)(operation_status + 0x10) < entry_index) {
                current_byte = *(uint8_t *)(buffer[2] + entry_index);
            } else {
                current_byte =
                    *(uint8_t *)(buffer[2] + entry_index) & ~*(uint8_t *)(entry_index + *(int *)(operation_status + 0x14) + -1);
            }
            if (current_byte != 0) {
                working_result = 8;
                goto core_transfer_firmware_t_byte_before_updating_buffer;
            }
        }
        if (((*(uint32_t *)(operation_status + 8) & 1) == 0) || (entry_index = core_update_command_phase(buffer), entry_index == 0)) {
            /* Invoke the callback selected by the active queue entry. */
            (**(firmware_callback_t **)(operation_status + 0xc))(buffer);
        }
        goto core_transfer_firmware_t_byte_before_update_command_phase_auxiliary;
    }
    entry_index = core_get_command_phase_flags(3);
    if (entry_index == 0) {
        entry_index = core_get_command_phase_flags(4);
        if (entry_index == 0) {
            entry_index = core_get_command_phase_flags(8);
            if (entry_index == 0)
                goto core_transfer_firmware_t_byte_before_updating_entry_index;
            response_code = 0xf;
            working_result = 8;
        } else {
            response_code = 0xc;
            working_result = 4;
        }
        *buffer = response_code;
    } else {
        operation_status = core_get_command_phase_flags(2);
        if (operation_status == 0) {
            operation_status = core_get_command_phase_flags(1);
            if (operation_status != 0) {
                working_result = 0xd;
                goto core_transfer_firmware_t_byte_before_update_command_phase_flags_for_firmware_context_byte;
            }
        } else {
            working_result = 0xe;
        /* Shared target for core transfer firmware t byte before update command phase flags for firmware context byte; incoming paths preserve the same state assumptions. */
        core_transfer_firmware_t_byte_before_update_command_phase_flags_for_firmware_context_byte:
            *buffer = working_result;
        }
        working_result = 3;
    }
    core_update_command_phase_flags_for_firmware_context_byte(working_result);
/* Shared target for core transfer firmware t byte before update command phase auxiliary; incoming paths preserve the same state assumptions. */
core_transfer_firmware_t_byte_before_update_command_phase_auxiliary:
    /* Transfer firmware t byte; this branch selects whether its associated handler runs. */
    core_update_command_phase_auxiliary(*buffer);
    return;
}

/**
 * @brief Handle descriptor request bytes.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_handle_descriptor_request_bytes(offset, buffer) int offset;
int *buffer;

{
    /* Handle descriptor request bytes using only caller-provided data. */
    uint16_t descriptor_length;
    uint8_t working_result;
    int operation_status;
    uint8_t *request_bytes;

    working_result = core_is_supported_descriptor_opcode(*(uint8_t *)buffer[2]);
    request_bytes = (uint8_t *)buffer[2];
    *(uint8_t *)(offset + 10) = working_result;
    /* Handle descriptor request bytes; this branch selects whether its associated handler runs. */
    working_result = core_get_command_feature(*request_bytes, request_bytes[1]);
    operation_status = buffer[9];
    *(uint8_t *)(offset + 0xb) = working_result;
    *(char *)(offset + 0x10) = (char)operation_status;
    operation_status = ((bool (*)())core_transition_selected_or_bit_selection_state)(offset, buffer + 1);
    if (operation_status == 0) {
        operation_status = *(int *)(offset + 0xc);
    } else {
        if (*(int16_t *)(offset + 8) == 0) {
            *buffer = 8;
        }
        operation_status = (int)core_get_ata_command_buffer();
        if ((*(char *)(operation_status + 0x12) == '\0') && (0x100 < *(uint16_t *)(offset + 8))) {
            *buffer = 8;
        }
        if ((*(char *)(offset + 0xb) == '\0') || (operation_status = (int)core_get_ata_command_buffer(), *(char *)(operation_status + 0x24) != '\0')) {
            operation_status = *buffer;
            if (operation_status != 0)
                goto core_branch_target_082;
            if (*(char *)(offset + 10) == '\0') {
            /* Shared target for core handle descriptor request bytes before get runtime context base for build SCSI mode; incoming paths preserve the same state assumptions. */
            core_handle_descriptor_request_bytes_before_get_runtime_context_base_for_build_scsi_mode:
                operation_status = core_get_runtime_context_base_for_build_scsi_mode();
                operation_status = core_update_constant_0x12_x01(operation_status + 0x2c);
                if (operation_status == 0) {
                    operation_status = 0x14;
                    goto core_branch_target_081;
                }
            } else {
                operation_status = core_process_command_context_value();
                if (operation_status != 0) {
                    operation_status = 0x15;
                    goto core_branch_target_081;
                }
                if (*(char *)(offset + 10) == '\0')
                    goto core_handle_descriptor_request_bytes_before_get_runtime_context_base_for_build_scsi_mode;
            }
            descriptor_length = *(uint16_t *)(offset + 8);
            working_result = core_is_supported_descriptor_opcode(*(uint8_t *)buffer[2]);
            operation_status = core_get_runtime_context_base_for_build_scsi_mode();
            operation_status = core_handle_structure_field_hex(offset, descriptor_length, working_result, operation_status + 0x2c);
            if (operation_status == 0) {
                operation_status = 0x13;
            } else {
                operation_status = 0;
            }
        } else {
            operation_status = 8;
        }
    }
/* Shared target for core branch target 081; incoming paths preserve the same state assumptions. */
core_branch_target_081:
    *buffer = operation_status;
/* Shared target for core branch target 082; incoming paths preserve the same state assumptions. */
core_branch_target_082:
    return operation_status == 0;
}

/**
 * @brief Handle fallback decoded request bytes.
 *
 * @param offset Procedure input.
 * @param request_bytes Procedure input.
 * @param output_value Procedure input.
 * @return Result produced by the procedure.
 */
int core_handle_fallback_decoded_request_bytes(offset, request_bytes, output_value)
int offset;
uint8_t *request_bytes;
uint32_t *output_value;

{
    /* Handle fallback decoded request bytes using only caller-provided data. */
    int operation_result;
    int core_update_device_for_device_ti_memset_result;
    uint32_t core_transfer_component_for_fallback_decoded_request_result;
    uint32_t status_bits;
    uint32_t fallback_decoded_value;
    uint32_t decoded_value;
    uint8_t working_buffer[8];
    uint8_t primary_state[8];

    /* Gate the following state transition on (*request_bytes < 0x1f) && ((request_bytes[1] & 5) == 1); the handler is skipped when this condition is false. */
    if ((*request_bytes < 0x1f) && ((request_bytes[1] & 5) == 1)) {
        operation_result = core_update_descriptor_requested(offset, primary_state, request_bytes);
        if (operation_result == 0) {
            decoded_value = 0;
            if (*(char *)(offset + 0x7c) == '\0') {
                status_bits = 0;
                operation_result = core_write_four_byte_value(&status_bits, output_value);
                core_transfer_tracker(output_value, operation_result + 1, 4, &decoded_value);
                if (*(char *)(offset + 0x6d) == '\0') {
                    core_transfer_component_for_fallback_decoded_request_result = 0x12345678;
                } else {
                    core_transfer_component_for_fallback_decoded_request_result = 0x87654321;
                }
                fallback_decoded_value = 0x1fc;
                /* Handle fallback decoded request bytes; this branch selects whether its associated handler runs. */
                core_transfer_component_for_fallback_decoded_request_result = core_transfer_component_for_fallback_decoded_request(*output_value, 0x1fc, core_transfer_component_for_fallback_decoded_request_result);
                core_transfer_tracker(output_value, core_transfer_component_for_fallback_decoded_request_result, 4, &fallback_decoded_value);
            }
            operation_result = core_update_device_for_device_ti_memset(offset, primary_state, output_value);
            if (((request_bytes[1] >> 1 & 1) != 0) &&
                (((core_update_device_for_device_ti_memset_result = core_update_descriptor_subtraction(offset, working_buffer, request_bytes),
                   core_update_device_for_device_ti_memset_result != 0 || (core_update_device_for_device_ti_memset_result = core_update_device_for_device_ti_memset(offset, working_buffer, output_value), core_update_device_for_device_ti_memset_result != 0)) &&
                  (operation_result == 0)))) {
                operation_result = core_update_device_for_device_ti_memset_result;
            }
        }
    } else {
        operation_result = 0xd;
    }
    return operation_result;
}

/**
 * @brief Transition success flag clear memory transfer success context.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_transition_success_flag_clear_memory_transfer_success_context(context, offset, condition, flags)
uint32_t context;
int offset;
uint32_t condition;
uint32_t flags;

{
    /* Transition success flag clear memory transfer success context; persistent state is carried in core transfer success context, core state 002. */
    int operation_status;
    uint32_t working_result;
    uint32_t success_flag;
    uint32_t context_derived;
    int return_context;
    volatile uint32_t condition_field = 0;
    volatile uint32_t flags_snapshot = 0;

    context_derived = context;
    return_context = offset;
    condition_field = condition;
    flags_snapshot = flags;
    ti_memset(&context_derived, 0, 0x10);
    if (0x9f < condition) {
        core_forward_end_remaining(&context_derived, offset + 0x90, 0x10);
    }
    operation_status = core_compare_buffers_from_end(offset + 0x14, &core_branch_target_086, 4);
    offset = offset + 0x14;
    success_flag = 1;
    if (operation_status == 0) {
        core_process_bitset_base(&context_derived, 0x36);
        working_result = 0x37;
    /* Shared target for core transition success flag clear memory transfer success context before process bitset base; incoming paths preserve the same state assumptions. */
    core_transition_success_flag_clear_memory_transfer_success_context_before_process_bitset_base:
        core_process_bitset_base(&context_derived, working_result);
    /* Shared target for core transition success flag clear memory transfer success context before process command code in; incoming paths preserve the same state assumptions. */
    core_transition_success_flag_clear_memory_transfer_success_context_before_process_command_code_in:
        operation_status = core_process_command_code_in(&context_derived, context);
        if (operation_status != 0)
            goto core_transition_success_flag_clear_memory_transfer_success_context_return_path;
    } else {
        operation_status = core_compare_buffers_from_end(offset, &core_branch_target_087, 4);
        if (operation_status == 0) {
            working_result = 0x36;
            goto core_transition_success_flag_clear_memory_transfer_success_context_before_process_bitset_base;
        }
        operation_status = core_compare_buffers_from_end(offset, &core_transfer_success_context, 4);
        if (operation_status == 0) {
            core_process_bitset_base(&context_derived, 0x37);
            working_result = 0x38;
            goto core_transition_success_flag_clear_memory_transfer_success_context_before_process_bitset_base;
        }
        operation_status = core_compare_buffers_from_end(offset, &core_state_002, 4);
        if (operation_status == 0)
            goto core_transition_success_flag_clear_memory_transfer_success_context_before_process_command_code_in;
    }
    success_flag = 0;
/* Shared target for core transition success flag clear memory transfer success context return path; incoming paths preserve the same state assumptions. */
core_transition_success_flag_clear_memory_transfer_success_context_return_path:
    return CONCAT44(return_context, success_flag);
}

/**
 * @brief Finalize command response hook.
 *
 * @return No value.
 */
void core_finalize_command_response_hook()

{
    /* Finalize command response hook using only caller-provided data. */
    return;
}

/**
 * @brief Parse stream record.
 *
 * @param output_word Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param input_bytes Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_parse_stream_record(output_word, value, length, input_bytes)
uint32_t *output_word;
uint32_t value;
uint32_t length;
uint8_t *input_bytes;

{
    /* Parse stream record; persistent state is carried in core response buffer, core state 190. */
    uint8_t core_transfer_end_remaining_result;
    uint32_t operation_status;
    int core_transfer_usb_get_device_result;
    uint32_t operation_result;
    uint32_t status_bits;
    int core_response_buffer_snapshot;
    uint8_t *buffer_2_cursor;

    core_response_buffer_snapshot = (int)core_response_buffer;
    buffer_2_cursor = input_bytes;
    core_transfer_usb_get_device_result = core_transfer_usb_get_device(2, &buffer_2_cursor, 0);
    if (core_transfer_usb_get_device_result == 0) {
        status_bits = (uint32_t)*buffer_2_cursor;
        core_transfer_end_remaining(core_response_buffer_snapshot, buffer_2_cursor, status_bits);
        buffer_2_cursor = buffer_2_cursor + status_bits;
        core_response_buffer_snapshot = status_bits + core_response_buffer_snapshot;
        core_transfer_end_remaining_result = *buffer_2_cursor;
        /* Iterate while operation_status = (uint32_t)core_transfer_end_remaining_result, operation_status != 0, preserving the defined cursor and bound. */
        while (operation_status = (uint32_t)core_transfer_end_remaining_result, operation_status != 0) {
            core_transfer_end_remaining_result = buffer_2_cursor[1];
            if ((((core_transfer_end_remaining_result == 4) || (core_transfer_end_remaining_result == 5)) || (core_transfer_end_remaining_result == 0x30)) ||
                (core_transfer_end_remaining_result == 0x21 || core_transfer_end_remaining_result == 0x24)) {
                core_transfer_end_remaining(core_response_buffer_snapshot, buffer_2_cursor, operation_status);
                if ((core_transfer_end_remaining_result == 0x30) && (*(char *)(core_response_buffer_snapshot + 3) != '\0')) {
                    *(uint8_t *)(core_response_buffer_snapshot + 3) = 3;
                }
                status_bits = operation_status + status_bits;
                core_response_buffer_snapshot = operation_status + core_response_buffer_snapshot;
            }
            if (core_transfer_end_remaining_result == 0x21) {
                buffer_2_cursor = buffer_2_cursor + *(uint16_t *)(buffer_2_cursor + 7);
            }
            buffer_2_cursor = buffer_2_cursor + operation_status;
            core_transfer_end_remaining_result = *buffer_2_cursor;
        }
        *(char *)(core_response_buffer + 2) = (char)status_bits;
        *(char *)(core_response_buffer + 3) = (char)(status_bits >> 8);
        *(uint8_t *)(core_response_buffer + 8) = core_state_190;
        operation_result = 0;
        *output_word = status_bits;
    } else {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Transfer packed command words command words.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_packed_command_words_command_words(context, value, buffer)
uint32_t context;
uint32_t value;
uint8_t *buffer;

{
    /* Transfer packed command words command words using only caller-provided data. */
    bool condition_met;
    int core_update_component_result;
    uint32_t status_bits;
    uint8_t *abstack_38_derived;
    uint32_t buffer_cursor;
    int *istack_84_pointer;
    uint32_t operation_result;
    uint32_t *data_cursor;
    int aiStack_b4[7];
    uint32_t command_words[5];
    int status_output;
    volatile int packed_command_words[18];
    uint8_t header_bytes[4];
    uint8_t response_bytes[32];

    operation_result = 0;
    /* Gate the following state transition on (*buffer < 0x21; the handler is skipped when this condition is false. */
    if ((*buffer < 0x21) &&
        (core_update_component_result = core_update_component(aiStack_b4, value, *(uint32_t *)(buffer + 4), *(uint32_t *)(buffer + 8), *buffer),
         core_update_component_result != 0)) {
        /* Call core sha256 finalize through the defined unprototyped function-pointer signature. */
        ((void (*)())core_sha256_finalize)(context, response_bytes, 0x20);
        condition_met = true;
        data_cursor = command_words + 1;
        core_update_component_result = 8;
        abstack_38_derived = header_bytes;
        /* Repeat while core_update_component_result != 0; the body advances or polls the state needed to leave the loop. */
        do {
            data_cursor = data_cursor + -1;
            status_bits = *data_cursor;
            if ((uint32_t)abstack_38_derived[4] != status_bits >> 0x18) {
                condition_met = false;
            }
            if ((status_bits & 0xffffff) >> 0x10 != (uint32_t)abstack_38_derived[5]) {
                condition_met = false;
            }
            if ((status_bits & 0xffff) >> 8 != (uint32_t)abstack_38_derived[6]) {
                condition_met = false;
            }
            if ((status_bits & 0xff) != (uint32_t)abstack_38_derived[7]) {
                return 0;
            }
            if (!condition_met) {
                return 0;
            }
            core_update_component_result = core_update_component_result + -1;
            abstack_38_derived = abstack_38_derived + 4;
        } while (core_update_component_result != 0);
        buffer_cursor = *buffer - 1 & 0xff;
        status_bits = buffer_cursor - 1 & 0xff;
        if (0xc < status_bits) {
            istack_84_pointer = &status_output;
            core_update_component_result = status_bits - 0xc;
            /* Repeat while core_update_component_result != 0; the body advances or polls the state needed to leave the loop. */
            do {
                istack_84_pointer = istack_84_pointer + 1;
                /* Gate the following state transition on *istack_84_pointer != -1; the handler is skipped when this condition is false. */
                if (*istack_84_pointer != -1) {
                    return 0;
                }
                core_update_component_result = core_update_component_result + -1;
            } while (core_update_component_result != 0);
        }
        if (aiStack_b4[buffer_cursor] == 0x1ffff) {
            operation_result = 1;
        }
    }
    return operation_result;
}

/**
 * @brief Transfer remaining bytes word.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param offset Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_transfer_remaining_bytes_word(buffer, value, offset, flags) int *buffer;
uint32_t value;
int offset;
int flags;

{
    /* Transfer remaining bytes word using only caller-provided data. */
    uint32_t remaining_bytes;
    uint32_t status_bits;
    uint32_t word_index;
    int *data_cursor;
    uint32_t value_derived;
    uint32_t computed_offset;
    uint32_t last_index;
    uint32_t *buffer_cursor;
    int flags_snapshot;

    flags_snapshot = flags;
    core_update_secondary_update_context(&flags_snapshot);
    value_derived = flags_snapshot * (0x34 / value + 6);
    if (value_derived != 0) {
        last_index = value - 1;
        /* Repeat while remaining_bytes != 0; the body advances or polls the state needed to leave the loop. */
        do {
            computed_offset = (value_derived & 0xf) >> 2;
            if (last_index != 0) {
                remaining_bytes = last_index;
                data_cursor = buffer + last_index;
                buffer_cursor = (uint32_t *)(buffer + (value - 2));
                /* Repeat while remaining_bytes != 0; the body advances or polls the state needed to leave the loop. */
                do {
                    status_bits = *buffer_cursor;
                    word_index = remaining_bytes & 3;
                    remaining_bytes = remaining_bytes - 1;
                    *data_cursor =
                        *data_cursor - (*(int *)(offset + (computed_offset ^ word_index) * 4) + value_derived ^
                                             status_bits + (status_bits << 4 ^ status_bits >> 5));
                    data_cursor = data_cursor + -1;
                    buffer_cursor = buffer_cursor + -1;
                } while (remaining_bytes != 0);
            }
            remaining_bytes = buffer[last_index];
            computed_offset = value_derived + *(int *)(offset + computed_offset * 4);
            value_derived = value_derived - flags_snapshot;
            *buffer = *buffer - (computed_offset ^ remaining_bytes + (remaining_bytes << 4 ^ remaining_bytes >> 5));
        } while (value_derived != 0);
    }
    return;
}

/**
 * @brief Process ti memset for type type.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_ti_memset_for_type_type(context)
uint32_t context;

{
    /* Process ti memset for type type using only caller-provided data. */
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;
    int core_process_command_phase_three_result;

    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 0x2c);
    core_process_command_phase_three_result = core_process_command_phase_three();
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 1, 0x10);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 2, 3, 0x28);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x14, 0x14, 1);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x16, 0x17, 0x14);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x18, 0x18, (uint32_t)(core_process_command_phase_three_result != 0) << 7 | 0x20);
    if (core_process_command_phase_three_result == 0) {
        core_process_command_phase_three_result = 0;
    } else {
        core_process_command_phase_three_result = 2;
    }
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x19, 0x19, core_process_command_phase_three_result << 6 | 4);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x1a, 0x1b, 0x200);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x1e, 0x1f, 0x200);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0x28, 0x2b, 0x80010400);
    return 0;
}

/**
 * @brief Transfer word copy.
 *
 * @param destination_words Procedure input.
 * @param source_words Procedure input.
 * @param offset Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_word_copy(destination_words, source_words, offset, condition)
uint32_t *destination_words;
uint32_t *source_words;
int offset;
int condition;

{
    /* Transfer word copy using only caller-provided data. */
    uint32_t operation_status;
    int copy_count;
    int *computed_offset;
    int condition_field;
    uint32_t *data_cursor;
    uint32_t *buffer_1_cursor;
    uint32_t *output_cursor;
    uint32_t status_bits;
    uint32_t remaining_count;
    uint32_t *base_buffer;
    uint32_t *buffer_2_cursor;
    int word_count;

    buffer_1_cursor = destination_words;
    /* Iterate while condition_field = condition << 1; condition_field != 0; condition_field = condition_field + -1, preserving the defined cursor and bound. */
    for (condition_field = condition << 1; condition_field != 0; condition_field = condition_field + -1) {
        *buffer_1_cursor = 0;
        buffer_1_cursor = buffer_1_cursor + 1;
    }
    if (condition != 0) {
        computed_offset = (int *)(offset + -4);
        condition_field = 0;
        output_cursor = destination_words + condition;
        base_buffer = destination_words;
        buffer_2_cursor = source_words;
        word_count = condition;
        /* Repeat while copy_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            computed_offset = computed_offset + 1;
            /* Gate the following state transition on *computed_offset == 0; the handler is skipped when this condition is false. */
            if (*computed_offset == 0) {
                operation_status = 0;
            } else {
                operation_status = 0;
                copy_count = word_count;
                data_cursor = (uint32_t *)(condition_field + (int)base_buffer);
                buffer_1_cursor = buffer_2_cursor;
                /* Repeat while copy_count != 0; the body advances or polls the state needed to leave the loop. */
                do {
                    core_configure_component_scaled(&status_bits, *buffer_1_cursor, *computed_offset);
                    if (operation_status + status_bits < operation_status) {
                        remaining_count = remaining_count + 1;
                    }
                    status_bits = *data_cursor + operation_status + status_bits;
                    if (status_bits < *data_cursor) {
                        remaining_count = remaining_count + 1;
                    }
                    copy_count = copy_count + -1;
                    *data_cursor = status_bits;
                    operation_status = remaining_count;
                    data_cursor = data_cursor + 1;
                    buffer_1_cursor = buffer_1_cursor + 1;
                } while (copy_count != 0);
            }
            *output_cursor = operation_status;
            condition = condition + -1;
            condition_field = condition_field + 4;
            output_cursor = output_cursor + 1;
        } while (condition != 0);
    }
    return 0;
}

/**
 * @brief Unused update hook.
 *
 * @return No value.
 */
void core_unused_update_hook()

{
    /* Unused update hook using only caller-provided data. */
    return;
}

