/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Process word words device descriptor context.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param offset Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_word_words_device_descriptor_context(context, condition, offset, flags)
uint32_t context;
int condition;
int offset;
uint32_t flags;

{
    /* Process word words device descriptor context; persistent state is carried in core primary descriptor state. */
    int core_transfer_match_found_expected_type_result;
    uint32_t input_word = 0;
    char data_cursor[4];
    uint32_t result_words[5];

    result_words[0] = 0x28;
    if ((((0x27 < *(uint16_t *)(offset + 8)) && (core_transfer_match_found_expected_type_result = core_update_constant_0xc_for_word_words_resource(offset, result_words), core_transfer_match_found_expected_type_result != 0)) &&
         (core_transfer_match_found_expected_type_result = core_build_status_control_byte(offset, result_words), core_transfer_match_found_expected_type_result != 0)) &&
        (core_transfer_match_found_expected_type_result = core_transfer_match_found_expected_type(flags, data_cursor, 0, 0, input_word & 0xffffff00), core_transfer_match_found_expected_type_result != 0)) {
        if ((core_primary_descriptor_state == 3) || (data_cursor[0] != '\0')) {
            result_words[0] = 0x27;
        } else if (condition == 2) {
            result_words[0] = core_process_parsed_header_descriptor(0x14, *(int16_t *)(offset + 8) + 0x18, offset, flags, context);
        }
    }
    return result_words[0];
}

/**
 * @brief Transition sequence x01.
 *
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
void core_transition_sequence_x01(condition, buffer) int condition;
int *buffer;

{
    /* Transition sequence x01 using only caller-provided data. */
    char next_sequence;
    int core_is_software_timer_expired_result;

    /* Gate the following state transition on *buffer == 0; the handler is skipped when this condition is false. */
    if (*buffer == 0) {
        if (condition != 0) {
            return;
        }
        if (buffer[2] != 1) {
            return;
        }
        core_is_software_timer_expired_result = core_is_software_timer_expired((int)buffer + 0xe);
        if (core_is_software_timer_expired_result == 0) {
            return;
        }
        *(uint8_t *)(buffer + 1) = 0;
    } else {
        if ((condition != 0) || (((core_is_software_timer_expired_result = core_is_software_timer_expired((int)buffer + 0xe), core_is_software_timer_expired_result != 0 && (buffer[2] == 3)) ||
                                  (core_is_software_timer_expired_result = core_is_software_timer_expired((int)buffer + 0xf), core_is_software_timer_expired_result != 0)))) {
            core_is_software_timer_expired_result = 0;
            buffer[2] = 0;
            goto core_cartridge_monitor_store_sequence_status;
        }
        if ((char)buffer[3] == '\x01') {
            core_is_software_timer_expired_result = 4;
            *(uint8_t *)(buffer + 3) = 0;
            goto core_cartridge_monitor_store_sequence_status;
        }
        core_is_software_timer_expired_result = core_is_software_timer_expired((int)buffer + 0xd);
        if (core_is_software_timer_expired_result == 0) {
            return;
        }
        if (buffer[2] != 4) {
            return;
        }
        if (*(uint8_t *)(buffer + 1) < 6) {
            next_sequence = *(uint8_t *)(buffer + 1) + 1;
        } else {
            next_sequence = '\0';
        }
        *(char *)(buffer + 1) = next_sequence;
        buffer[2] = 2;
    }
    core_is_software_timer_expired_result = 7;
/* Shared target for core cartridge monitor store sequence status; incoming paths preserve the same state assumptions. */
core_cartridge_monitor_store_sequence_status:
    *buffer = core_is_software_timer_expired_result;
    return;
}

/**
 * @brief Configure range start.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_configure_range_start(offset, value, length, flags)
int offset;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Configure range start using only caller-provided data. */
    int flags_snapshot;
    int operation_result;
    uint8_t working_buffer[4];
    int working_result = 0;
    int range_start = 0;

    operation_result = core_run_target_type_clear_memory_target_type_context(offset, length, value);
    if (((operation_result == 0x11) &&
         (operation_result = core_run_target_type_clear_memory_target_type_context(offset, 0, working_buffer), flags_snapshot = working_result, operation_result == 0)) &&
        (flags <= (uint32_t)(range_start - working_result))) {
        operation_result = flags + working_result;
        working_result = operation_result;
        core_copy_record_header_to_destination(offset, working_buffer);
        operation_result = core_submit_control_transfer_fields(offset, length, 1, flags_snapshot, operation_result + -1);
        if ((operation_result == 0) &&
            (operation_result = ((int (*)())core_handle_fallback_decoded_request_bytes)(*(uint32_t *)(offset + 0x20), offset + 0xc, offset), operation_result == 0)) {
            operation_result = core_run_target_type_clear_memory_target_type_context(offset, length, value);
        }
    }
    return operation_result;
}

/**
 * @brief Transfer match found expected type.
 *
 * @param context Procedure input.
 * @param output_type Procedure input.
 * @param expected_type Procedure input.
 * @param match_found Optional output flag set when a matching entry is found.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_match_found_expected_type(context, output_type, expected_type, match_found, index)
uint32_t context;
char *output_type;
uint8_t *expected_type;
uint8_t *match_found;
char index;

{
    /* Transfer match found expected type using only caller-provided data. */
    int operation_status;
    uint32_t core_get_0xc_for_lookup_key_metadata_result;
    char working_result;
    uint32_t entry_index;
    uint8_t working_buffer[16];
    uint8_t entry_metadata[12];

    if (match_found != (uint8_t *)0x0) {
        *match_found = 0;
    }
    /* Count valid descriptor slots and optionally report whether one has the requested type. */
    working_result = '\0';
    entry_index = 0;
    /* Repeat while (int)entry_index < 8; the body advances or polls the state needed to leave the loop. */
    do {
        operation_status = core_validate_device_descriptor_length(context, entry_index & 0xff, working_buffer, entry_metadata);
        /* The optional index flag suppresses entries whose descriptor bit 3 is asserted. */
        if (((operation_status != 0) && (operation_status = core_has_device_descriptor_high_bit(working_buffer), operation_status != 0)) &&
            ((index == '\0' || (core_get_0xc_for_lookup_key_metadata_result = core_get_0xc_for_device_descriptor_response(working_buffer), (core_get_0xc_for_lookup_key_metadata_result >> 3 & 1) == 0)))) {
            if (((expected_type != (uint8_t *)0x0) && (match_found != (uint8_t *)0x0)) &&
                (core_get_0xc_for_lookup_key_metadata_result = core_get_0xc_for_lookup_key_metadata(working_buffer), *expected_type == core_get_0xc_for_lookup_key_metadata_result)) {
                *match_found = 1;
            }
            working_result = working_result + '\x01';
        }
        entry_index = entry_index + 1;
    } while ((int)entry_index < 8);
    if (output_type != (char *)0x0) {
        /* Return the aggregate count separately from the always-success function result. */
        *output_type = working_result;
    }
    return 1;
}

/**
 * @brief Handle SCSI get event status.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_handle_scsi_get_event_status(buffer) uint32_t *buffer;

{
    /* Handle SCSI get event status using only caller-provided data. */
    int core_initialize_scsi_response_buffer_result;
    int operation_status;

    if ((*(uint8_t *)(buffer[2] + 1) & 1) == 0) {
        *buffer = 8;
        return;
    }
    if (*(char *)(buffer[2] + 4) == '\0') {
        operation_status = 0;
    } else {
        operation_status = 4;
    }
    core_initialize_scsi_response_buffer_result = (int)core_initialize_scsi_response_buffer(buffer, operation_status + 4);
    core_update_release_record_payload(core_initialize_scsi_response_buffer_result, buffer, 7, 8);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0, 1, operation_status);
    *(uint8_t *)(*(int *)(core_initialize_scsi_response_buffer_result + 4) + 3) = 0x10;
    if (operation_status == 0) {
        *(uint8_t *)(*(int *)(core_initialize_scsi_response_buffer_result + 4) + 2) = 0x80;
        return;
    }
    *(uint8_t *)(*(int *)(core_initialize_scsi_response_buffer_result + 4) + 2) = 4;
    operation_status = core_get_command_buffer_state_value();
    if (operation_status != 0) {
        *(uint8_t *)(*(int *)(core_initialize_scsi_response_buffer_result + 4) + 4) = 1;
        core_update_command_buffer_state(0);
    }
    operation_status = core_is_command_phase_zero();
    if (operation_status == 0) {
        *(uint8_t *)(*(int *)(core_initialize_scsi_response_buffer_result + 4) + 5) = 2;
    }
    core_set_notification_state_to_one();
    return;
}

/**
 * @brief Control temperature monitor.
 *
 * @return No value.
 */
void core_control_temperature_monitor()

{
    /* Control temperature monitor; persistent state is carried in core smart temperature celsius, core thermal hot sample retry count and related record fields. */
    bool core_smart_temperature_celsius_snapshot;
    uint8_t core_thermal_hot_sample_retry_count_snapshot;
    uint16_t limited_state;
    int core_read_smart_temperature_result;
    uint32_t working_result;

    /* Poll only after SMART monitoring is enabled and the current sampling timer expires. */
    if (((core_smart_monitoring_active != '\0') && (core_read_smart_temperature_result = core_is_software_timer_expired(&core_smart_temperature_sample_timer), core_read_smart_temperature_result != 0)) &&
        (core_read_smart_temperature_result = core_read_smart_temperature(0), core_read_smart_temperature_result != 0)) {
        working_result = 300000;
        core_smart_temperature_celsius_snapshot = false;
        core_thermal_hot_sample_retry_count_snapshot = core_thermal_hot_sample_retry_count;
        if (core_thermal_hot_sample_retry_count < 3) {
            /* Temperatures above 44 trigger a 10-second retry and increment the hot-sample counter. */
            limited_state = core_smart_temperature_celsius;
            if (core_smart_temperature_celsius < 0x2d) {
                limited_state = 3;
            }
            core_smart_temperature_celsius_snapshot = 0x2c < core_smart_temperature_celsius;
            core_thermal_hot_sample_retry_count_snapshot = (uint8_t)limited_state;
            if (core_smart_temperature_celsius_snapshot) {
                working_result = 10000;
                core_thermal_hot_sample_retry_count_snapshot = core_thermal_hot_sample_retry_count + 1;
            }
        }
        core_thermal_hot_sample_retry_count = core_thermal_hot_sample_retry_count_snapshot;
        rti_set_software_timer(&core_smart_temperature_sample_timer, working_result);
        /* With no device-status override, map the defined temperature bands to mechanism phases. */
        if ((core_thermal_pwm_override_active == '\0') && (!core_smart_temperature_celsius_snapshot)) {
            if (core_smart_temperature_celsius < 0x2d) {
                if (core_smart_temperature_celsius < 0x29) {
                    core_set_thermal_pwm_control_phase_one();
                    return;
                }
            } else {
                core_set_thermal_pwm_control_phase_two();
            }
        }
    }
    return;
}

/**
 * @brief Transition response flag x01.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transition_response_flag_x01(buffer)
int *buffer;

{
    /* Transition response flag x01 using only caller-provided data. */
    int working_result;
    uint8_t response_flag;
    int output_value;
    int current_state;
    int *state_record;
    int *buffer_cursor;

    state_record = (int *)buffer[5];
    if (state_record == (int *)0x0) {
        return 0;
    }
    current_state = *state_record;
    if (current_state == 0) {
        return 0;
    }
    working_result = 1;
    if ((current_state == 8) || (current_state == 10)) {
        *(uint8_t *)(buffer + 1) = 0;
        current_state = 1;
    /* Shared target for core output sequence store output state; incoming paths preserve the same state assumptions. */
    core_output_sequence_store_output_state:
        *buffer = current_state;
    } else {
        if (current_state == 4) {
            *state_record = 5;
            *(uint8_t *)(buffer + 2) = 0x14;
            response_flag = 2;
        } else {
            if (current_state != 5) {
                output_value = working_result;
                buffer_cursor = buffer;
                if (current_state == 9) {
                /* Shared target for core output sequence store state and flag; incoming paths preserve the same state assumptions. */
                core_output_sequence_store_state_and_flag:
                    *buffer_cursor = output_value;
                    *(char *)(buffer + 1) = (char)working_result;
                    return 1;
                }
                if (current_state != 1) {
                    if (current_state != 2) {
                        return 1;
                    }
                    working_result = 0;
                    output_value = 3;
                    buffer_cursor = state_record;
                    goto core_output_sequence_store_state_and_flag;
                }
                working_result = state_record[1];
                current_state = 0;
                *(uint8_t *)((int)buffer + 6) = 0;
                *(uint8_t *)(buffer + 1) = 0;
                *(char *)(buffer + 2) = ((char)working_result + '\x01') * '\x02';
                *state_record = 2;
                goto core_output_sequence_store_output_state;
            }
            response_flag = 0;
            *state_record = 6;
        }
        *(uint8_t *)((int)buffer + 6) = response_flag;
    }
    return 1;
}

/**
 * @brief Accumulate split transfer offsets.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
bool core_accumulate_split_transfer_offsets(offset) int offset;

{
    /* Accumulate split transfer offsets using only caller-provided data. */
    uint32_t core_saturating_add_u32_result;
    int core_transfer_comparison_context_result;

    if (*(int *)(offset + 0x58) != 0 || *(uint32_t *)(offset + 0x54) != 0) {
        core_saturating_add_u32_result =
            core_saturating_add_u32(*(uint32_t *)(offset + 0x30), *(uint32_t *)(offset + 0x54) >> 0x14 | *(int *)(offset + 0x58) << 0xc);
        *(uint32_t *)(offset + 0x30) = core_saturating_add_u32_result;
        *(uint8_t *)(offset + 0x14) = 2;
        *(uint32_t *)(offset + 0x54) = 0;
        *(uint32_t *)(offset + 0x58) = 0;
    }
    if (*(int *)(offset + 0x60) != 0 || *(uint32_t *)(offset + 0x5c) != 0) {
        core_saturating_add_u32_result =
            core_saturating_add_u32(*(uint32_t *)(offset + 0x34), *(uint32_t *)(offset + 0x5c) >> 0x14 | *(int *)(offset + 0x60) << 0xc);
        *(uint32_t *)(offset + 0x34) = core_saturating_add_u32_result;
        *(uint8_t *)(offset + 0x15) = 2;
        *(uint32_t *)(offset + 0x5c) = 0;
        *(uint32_t *)(offset + 0x60) = 0;
    }
    core_transfer_comparison_context_result = core_transfer_comparison_context(offset);
    if (core_transfer_comparison_context_result != 0) {
        *(int *)(offset + 0x80) = core_transfer_comparison_context_result;
    }
    return core_transfer_comparison_context_result == 0;
}

/**
 * @brief Encrypt keyed word block.
 *
 * @param buffer Procedure input.
 * @param word_count Procedure input.
 * @param key_words Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_encrypt_keyed_word_block(buffer, word_count, key_words, flags) uint32_t *buffer;
uint32_t word_count;
int key_words;
int flags;

{
    /* Encrypt keyed word block using only caller-provided data. */
    int rounds_remaining;
    uint32_t key_word_index;
    uint32_t word_index;
    uint32_t key_sum;
    uint32_t mixed_word;
    uint32_t *buffer_cursor;
    uint32_t words_remaining;
    int cipher_delta;

    mixed_word = buffer[word_count - 1];
    cipher_delta = flags;
    core_decode_word_cipher_delta(&cipher_delta);
    if ((1 < word_count) && (0x34 / word_count != 0xfffffffa)) {
        rounds_remaining = 0x34 / word_count + 6;
        key_sum = 0;
        /* Repeat while words_remaining != 0; the body advances or polls the state needed to leave the loop. */
        do {
            word_index = 0;
            key_sum = cipher_delta + key_sum;
            buffer_cursor = buffer;
            words_remaining = word_count;
            /* Repeat while words_remaining != 0; the body advances or polls the state needed to leave the loop. */
            do {
                key_word_index = word_index & 3;
                words_remaining = words_remaining - 1;
                word_index = word_index + 1;
                mixed_word = (key_sum + *(int *)(key_words + ((key_sum & 0xf) >> 2 ^ key_word_index) * 4) ^
                                    mixed_word + (mixed_word << 4 ^ mixed_word >> 5)) +
                                   *buffer_cursor;
                *buffer_cursor = mixed_word;
                buffer_cursor = buffer_cursor + 1;
            } while (words_remaining != 0);
            rounds_remaining = rounds_remaining + -1;
        } while (rounds_remaining != 0);
    }
    return;
}

/**
 * @brief Transition use alternate lookup type copy.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param record_type Procedure input.
 * @param flags Procedure input.
 * @param offset Procedure input.
 * @param use_alternate_lookup Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transition_use_alternate_lookup_type_copy(buffer, value, record_type, flags, offset, use_alternate_lookup)
uint8_t *buffer;
uint32_t value;
int record_type;
uint32_t flags;
int offset;
char use_alternate_lookup;

{
    /* Transition use alternate lookup type copy using only caller-provided data. */
    uint32_t core_get_remaining_descriptor_space_result;
    int operation_status;
    int record_type_copy;
    volatile uint32_t flags_snapshot = 0;

    record_type_copy = record_type;
    flags_snapshot = flags;
    core_get_remaining_descriptor_space_result = core_get_remaining_descriptor_space(value);
    *(uint32_t *)(buffer + 8) = core_get_remaining_descriptor_space_result;
    operation_status = core_get_payload_offset_from_flags(value);
    buffer[1] = (char)record_type;
    *(int *)(buffer + 4) = operation_status;
    *buffer = (char)flags;
    if (record_type != 0) {
        if (*(uint32_t *)(buffer + 8) < 8) {
            return 2;
        }
        *(int *)(buffer + 4) = operation_status + 8;
        *(uint32_t *)(buffer + 8) = *(uint32_t *)(buffer + 8) - 8;
        if (use_alternate_lookup != '\0') {
            operation_status = core_update_descriptor_requested(*(uint32_t *)(offset + 0x20), &record_type_copy, offset + 0xc);
            core_get_remaining_descriptor_space_result = 2;
            if (operation_status != 0)
                goto core_record_transfer_validate_result_bounds;
            operation_status = core_get_default_record_type_code(*(uint32_t *)(offset + 0x20), &record_type_copy, offset);
            if (operation_status != 0) {
                if (operation_status == 0x15) {
                    core_get_remaining_descriptor_space_result = 3;
                }
                goto core_record_transfer_validate_result_bounds;
            }
        }
    }
    core_get_remaining_descriptor_space_result = 0;
/* Shared target for core record transfer validate result bounds; incoming paths preserve the same state assumptions. */
core_record_transfer_validate_result_bounds:
    operation_status = core_validate_combined_record_bounds(buffer);
    if (operation_status == 0) {
        core_get_remaining_descriptor_space_result = 4;
    }
    return core_get_remaining_descriptor_space_result;
}

/**
 * @brief Process copy context.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
bool core_process_copy_context(context, offset) uint32_t context;
int offset;

{
    /* Process copy context; persistent state is carried in core copy record final region descriptor, core copy record leading region descriptor and related record fields. */
    int status_bits;
    int core_process_remaining_descriptor_space_for_copy_copy_resource_result;
    int second_copy_status;
    int third_copy_status;
    bool condition_met;
    uint8_t working_buffer[36];

    status_bits = core_initialize_output_record_from_source(context, working_buffer);
    if (status_bits == 0) {
        condition_met = false;
    } else {
        status_bits = core_process_remaining_descriptor_space_for_copy_copy_resource(working_buffer, offset + 0x50, 0x10, &core_copy_record_final_region_descriptor);
        core_process_remaining_descriptor_space_for_copy_copy_resource_result = core_process_remaining_descriptor_space_for_copy_copy_resource(working_buffer, offset, 0x20, &core_copy_record_leading_region_descriptor);
        second_copy_status = core_process_remaining_descriptor_space_for_copy_copy_resource(working_buffer, offset + 0x20, 0x20, &core_copy_record_middle_region_descriptor);
        third_copy_status = core_process_remaining_descriptor_space_for_copy_copy_resource(working_buffer, offset + 0x40, 0x10, &core_copy_record_trailing_region_descriptor);
        condition_met = third_copy_status == 0 && (second_copy_status == 0 && (core_process_remaining_descriptor_space_for_copy_copy_resource_result == 0 && status_bits == 0));
        core_reset_output_record(working_buffer);
    }
    return condition_met;
}

/**
 * @brief Process x01 for retries remaining ti.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_x01_for_retries_remaining_ti(offset, buffer, length)
int offset;
uint8_t *buffer;
uint32_t length;

{
    /* Process x01 for retries remaining ti using only caller-provided data. */
    uint16_t destination_offset;
    int operation_result;
    uint32_t status_bits;
    uint32_t core_transfer_remaining_bounds_result;
    uint32_t core_get_payload_offset_from_flags_result;
    uint8_t working_buffer[24];

    /* Process x01 for retries remaining ti; this branch selects whether its associated handler runs. */
    operation_result = core_run_expected_flag_expected_type(*buffer, 0, working_buffer, length);
    if (operation_result == 0) {
        status_bits = core_get_remaining_descriptor_space(working_buffer);
        core_get_payload_offset_from_flags_result = core_get_payload_offset_from_flags(working_buffer);
        if (buffer[2] == '\0') {
            destination_offset = *(uint16_t *)(buffer + 6);
            core_transfer_remaining_bounds_result = core_transfer_remaining_bounds(&core_get_payload_offset_from_flags_result, status_bits & 0xff, length);
            *(uint32_t *)(offset + (uint32_t)destination_offset) = core_transfer_remaining_bounds_result;
        } else {
            if (buffer[2] != '\x01') {
                return 0xb;
            }
            if ((uint8_t)buffer[3] < status_bits) {
                status_bits = (uint32_t)(uint8_t)buffer[3];
            }
            core_read_buffer_bytes((uint32_t)*(uint16_t *)(buffer + 6) + offset, status_bits, core_get_payload_offset_from_flags_result, length);
        }
        *(uint8_t *)(offset + (uint32_t)*(uint16_t *)(buffer + 4)) = 1;
    }
    return operation_result;
}

/**
 * @brief Run watchdog reset remaining.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_run_wdt_reset_remaining(condition, value, length, flags)
uint32_t condition;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Run watchdog reset remaining using only caller-provided data. */
    bool condition_met;
    int spi_transfer_word_result;
    uint32_t remaining_count;
    uint32_t flags_snapshot;

    condition_met = false;
    remaining_count = 0;
    flags_snapshot = flags;
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
        wdt_reset();
        spi_transfer_word_result = spi_transfer_word(0x10060005, &flags_snapshot);
        if ((spi_transfer_word_result == 0) && (spi_transfer_word_result = spi_transfer_word(0x10060000, &flags_snapshot), spi_transfer_word_result == 0)) {
            if (condition == 2) {
                if ((flags_snapshot >> 1 & 1) != 0)
                    goto core_flash_status_mark_poll_condition_met;
            } else if ((condition & flags_snapshot) == 0) {
            /* Shared target for core flash status mark poll condition met; incoming paths preserve the same state assumptions. */
            core_flash_status_mark_poll_condition_met:
                condition_met = true;
            }
        }
        if (200000 < remaining_count) {
            spi_transfer_word_result = 1;
        /* Shared target for core flash status end poll and return; incoming paths preserve the same state assumptions. */
        core_flash_status_end_poll_and_return:
            spi_transfer_word(0x60000, &flags_snapshot);
            return spi_transfer_word_result;
        }
        if (condition_met || spi_transfer_word_result != 0)
            goto core_flash_status_end_poll_and_return;
        rti_delay_microseconds_with_watchdog(100);
        remaining_count = remaining_count + 1;
    } while (true);
}

/**
 * @brief Transition clear memory transition context.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t core_transition_clear_memory_transition_context(context, condition, buffer, flags)
uint32_t context;
int condition;
uint32_t *buffer;
uint32_t flags;

{
    /* Transition clear memory transition context; persistent state is carried in core memory transition header descriptor, core memory transition record descriptor. */
    int operation_status;
    uint32_t *buffer_cursor;
    uint8_t operation_result;
    uint32_t flags_snapshot;

    buffer_cursor = buffer;
    if (buffer == (uint32_t *)0x0) {
        buffer_cursor = &flags_snapshot;
    }
    flags_snapshot = flags;
    operation_status = core_process_remaining_descriptor_space_for_copy_copy_resource(context, buffer_cursor, 4, &core_memory_transition_header_descriptor);
    operation_result = 2;
    if (operation_status == 0) {
        if (condition != 0) {
            operation_status = core_process_remaining_descriptor_space_for_copy_copy_resource(context, condition, 0xa0, &core_memory_transition_record_descriptor);
        }
        if (operation_status == 0) {
            return *(char *)((int)buffer_cursor + 2) == '\0';
        }
    }
    if (operation_status == 6) {
        if (buffer != (uint32_t *)0x0) {
            ti_memset(buffer, 0, 4);
        }
        operation_result = 1;
        if (condition != 0) {
            ti_memset(condition, 0, 0xa0);
        }
    }
    return operation_result;
}

/**
 * @brief Handle feature command feature state.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_handle_feature_command_feature_state(buffer) uint32_t *buffer;

{
    /* Handle feature command feature state; persistent state is carried in core command feature state, core mode page 31 flags. */
    uint8_t feature_flags;
    int core_is_command_phase_three_for_build_scsi_mode_result;
    uint32_t working_result;

    core_is_command_phase_three_for_build_scsi_mode_result = core_is_command_phase_three_for_build_scsi_mode();
    feature_flags = *(uint8_t *)(buffer[2] + 4);
    if ((feature_flags >> 1 & 1) == 0) {
        if (core_is_command_phase_three_for_build_scsi_mode_result != 0) {
            if ((feature_flags & 1) == 0) {
                core_process_ata_standby_context();
                return;
            }
            core_verify_ata_media_and_refresh_state(feature_flags >> 1);
            return;
        }
        working_result = 2;
    /* Shared target for core feature command return rejection status; incoming paths preserve the same state assumptions. */
    core_feature_command_return_rejection_status:
        *buffer = working_result;
        return;
    }
    if ((feature_flags & 1) == 0) {
        if (core_command_feature_state != '\0') {
            working_result = 0xb;
            goto core_feature_command_return_rejection_status;
        }
        if ((core_mode_page_31_flags >> 3 & 1) == 0) {
            core_handle_ata_standby_context_for_cartridge_monitor_context(core_mode_page_31_flags >> 4);
            return;
        }
        core_update_cartridge_input_state();
        if ((core_mode_page_31_flags >> 4 & 1) != 0) {
            core_process_cartridge_input_state(core_mode_page_31_flags >> 5);
            core_mode_page_31_flags = core_mode_page_31_flags & 0xe7;
            return;
        }
    } else {
        core_update_command_phase(buffer, feature_flags & 1, feature_flags, feature_flags >> 2);
    }
    return;
}

/**
 * @brief Process component descriptor component context.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_process_component_descriptor_component_context(condition, value, length, flags) int condition;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process component descriptor component context; persistent state is carried in usb ep0 out max packet size, usb data trb buffer pointer high address bias and related record fields. */
    int working_result;
    uint32_t usb_ep0_out_max_packet_size_snapshot;
    uint32_t length_component;
    int flags_snapshot;

    flags_snapshot = flags;
    mww_update_mww_transfer_storage(condition, &flags_snapshot);
    if (*(int *)(flags_snapshot + 0x20) == 0) {
        if (condition == 0) {
            length_component = (uint32_t)*(uint16_t *)(flags_snapshot + 0x14);
            working_result = 0x11250;
            length_component = length_component * (((length + length_component) - 1) / length_component);
        } else {
            working_result = 0x11210;
            length_component = length;
        }
        *(uint32_t *)(working_result + -0x40000000) = value;
        usb_ep0_out_max_packet_size_snapshot = (uint32_t)usb_ep0_out_max_packet_size;
        *(uint32_t *)(&usb_data_trb_buffer_pointer_high_address_bias + working_result) = 0;
        if (usb_ep0_out_max_packet_size_snapshot <= length_component) {
            length_component = usb_ep0_out_max_packet_size_snapshot;
        }
        *(uint32_t *)(&usb_data_trb_status_address_bias + working_result) = length_component;
        *(uint32_t *)(&usb_data_trb_control_address_bias + working_result) = 0x853;
        mww_configure_transfer_descriptor(condition, (uint32_t *)(working_result + -0x40000000), length);
    }
    return;
}

/**
 * @brief Run target type clear memory target type context.
 *
 * @param request_type Procedure input.
 * @param target_type Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
int core_run_target_type_clear_memory_target_type_context(request_type, target_type, buffer)
uint32_t request_type;
int target_type;
uint8_t *buffer;

{
    /* Run target type clear memory target type context; persistent state is carried in core record lookup type order. */
    int operation_result;
    uint32_t entry_index;

    if (target_type == 0x17) {
        ti_memset(buffer, 0, 0x14);
        *buffer = 0x17;
        *(uint16_t *)(buffer + 2) = 0xc;
        buffer[1] = 3;
        operation_result = 0;
    } else {
        entry_index = 0;
        /* Iterate while true, preserving the defined cursor and bound. */
        while (true) {
            if (target_type == (&core_record_lookup_type_order)[entry_index]) {
                return 0x11;
            }
            operation_result = core_run_ready_state_handler(request_type, (&core_record_lookup_type_order)[entry_index], buffer);
            if ((operation_result == 0) && (operation_result = core_run_expected_flag_expected_type(target_type, 1, buffer, request_type), operation_result == 6)) {
                operation_result = 0x11;
            }
            entry_index = entry_index + 1 & 0xff;
            if (3 < entry_index)
                break;
            if (operation_result == 0) {
                return 0;
            }
        }
    }
    return operation_result;
}
