/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Process expected candidate.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
uint32_t core_process_expected_candidate(context, buffer, length)
uint32_t context;
uint8_t *buffer;
uint32_t length;

{
    /* Process expected candidate using only caller-provided data. */
    int operation_status;

    operation_status = core_get_expected_candidate();
    if (operation_status == 0) {
        /* Process expected candidate; this branch selects whether its associated handler runs. */
        ((uint32_t (*)())core_run_expected_flag_expected_type)(*buffer, 0, length, context);
    }
    return 0;
}

/**
 * @brief Get expected candidate.
 *
 * @param candidate_record Procedure input.
 * @param expected_record Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_expected_candidate(candidate_record, expected_record)
int candidate_record;
int expected_record;

{
    /* Get expected candidate using only caller-provided data. */
    uint32_t core_run_ready_state_handler_result;
    uint8_t working_buffer[24];

    if (*(char *)(expected_record + 1) == *(char *)(candidate_record + 0xc)) {
        core_run_ready_state_handler_result = 0;
    } else {
        core_run_ready_state_handler_result = core_run_ready_state_handler(candidate_record, *(char *)(expected_record + 1), working_buffer);
    }
    return core_run_ready_state_handler_result;
}

/**
 * @brief Update cartridge input state.
 *
 * @return No value.
 */
void core_update_cartridge_input_state()

{
    /* Update cartridge input state; persistent state is carried in core cartridge input state. */
    if ((core_cartridge_input_state != 6) && (core_cartridge_input_state == 2)) {
        core_cartridge_input_state = 6;
        core_process_command_descriptor_context();
        return;
    }
    return;
}

/**
 * @brief Request ATA standby when idle.
 *
 * @return No value.
 */
void core_request_ata_standby_when_idle()

{
    /* Request ATA standby when idle; persistent state is carried in core ata standby request pending. */
    int operation_status;

    operation_status = core_pause_mww_dispatch_for_standby();
    if (operation_status == 0) {
        core_ata_standby_request_pending = 1;
        return;
    }
    core_handle_ata_standby_context_for_cartridge_monitor_context();
    return;
}

/**
 * @brief Apply thermal PWM override.
 *
 * @return No value.
 */
void core_apply_thermal_pwm_override()

{
    /* Apply thermal PWM override; persistent state is carried in core thermal pwm override active, core thermal pwm state. */
    if (core_thermal_pwm_override_active == '\0') {
        core_set_thermal_pwm_control_phase_one();
        return;
    }
    if (core_thermal_pwm_state != 8) {
        core_set_thermal_pwm_duty(0);
        core_thermal_pwm_state = 7;
    }
    return;
}

/**
 * @brief Permute word cipher key bytes.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_permute_word_cipher_key_bytes(buffer, value) uint8_t *buffer;
int value;

{
    /* Permute word cipher key bytes; persistent state is carried in core word cipher key byte permutation. */
    int operation_status;
    int *core_word_cipher_key_byte_permutation_pointer;

    operation_status = 0x10;
    core_word_cipher_key_byte_permutation_pointer = (int *)&core_word_cipher_key_byte_permutation;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        operation_status = operation_status + -1;
        *buffer = *(uint8_t *)(value + *core_word_cipher_key_byte_permutation_pointer);
        buffer = buffer + 1;
        core_word_cipher_key_byte_permutation_pointer = core_word_cipher_key_byte_permutation_pointer + 1;
    } while (operation_status != 0);
    return;
}

/**
 * @brief Save persistent state with checksum.
 *
 * @return No value.
 */
void core_save_persistent_state_with_checksum()

{
    /* Save persistent state with checksum; persistent state is carried in core persistent identity record. */
    core_persistent_identity_record = ((uint32_t (*)())core_calculate_record_checksum)(&core_persistent_identity_record, 0x40);
    core_rewrite_primary_persistent_state(&core_persistent_identity_record);
    return;
}

/**
 * @brief Get identity variant code.
 *
 * @return Result produced by the procedure.
 */
int16_t core_get_identity_variant_code()

{
    /* Get identity variant code; persistent state is carried in core identity record variant code. */
    int16_t core_identity_record_variant_code_snapshot;

    if ((core_identity_record_variant_code == 0) || (core_identity_record_variant_code_snapshot = (int16_t)core_identity_record_variant_code, (int32_t)core_identity_record_variant_code == -1 || core_identity_record_variant_code == 1)) {
        core_identity_record_variant_code_snapshot = 0x37;
    }
    return core_identity_record_variant_code_snapshot;
}

/**
 * @brief Rewrite primary persistent state.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_rewrite_primary_persistent_state(context) uint32_t context;

{
    /* Rewrite primary persistent state using only caller-provided data. */
    int operation_status;

    operation_status = core_get_set_interrupt_mmio(0x3f000);
    if (operation_status == 0) {
        core_program_spi_flash_pages(0x3f000, 0x40, context);
    }
    return;
}

/**
 * @brief Process runtime context base for cartridge input state.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_runtime_context_base_for_cartridge_input_state()

{
    /* Process runtime context base for cartridge input state using only caller-provided data. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint32_t core_accumulate_split_transfer_offsets_result;

    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_accumulate_split_transfer_offsets_result = core_accumulate_split_transfer_offsets(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_clear_media_record_state_after_accounting(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    return core_accumulate_split_transfer_offsets_result;
}

/**
 * @brief Find last nonzero word.
 *
 * @param offset Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
int core_find_last_nonzero_word(offset, condition)
int offset;
int condition;

{
    /* Find last nonzero word using only caller-provided data. */
    int condition_field;
    int *computed_offset;

    if (condition != 0) {
        computed_offset = (int *)(offset + condition * 4);
        /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            computed_offset = computed_offset + -1;
            condition_field = condition + -1;
            /* Gate the following state transition on *computed_offset != 0; the handler is skipped when this condition is false. */
            if (*computed_offset != 0) {
                return condition;
            }
            condition = condition_field;
        } while (condition_field != 0);
    }
    return 0;
}

/**
 * @brief Process sense record context.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_sense_record_context()

{
    /* Process sense record context using only caller-provided data. */
    int operation_status;
    uint32_t operation_result;

    operation_status = core_is_mechanism_transition_pending();
    operation_result = 1;
    if (((operation_status == 0) && (operation_status = core_process_sci_read_logical_for_monitor_cartridge_input(), operation_status != 0)) &&
        (operation_status = core_is_firmware_image_validation_active(), operation_status == 0)) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Pack descriptor validation result.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_pack_descriptor_validation_result(context, value, offset)
uint32_t context;
uint32_t value;
int offset;

{
    /* Pack descriptor validation result using only caller-provided data. */
    int core_get_ti_memset_derived;

    core_get_ti_memset_derived = ((uint32_t (*)())core_get_ti_memset)();
    if ((core_get_ti_memset_derived != 0) && (*(uint16_t *)(offset + 8) < 0x38)) {
        core_get_ti_memset_derived = 0;
    }
    return CONCAT44(value, core_get_ti_memset_derived);
}

/**
 * @brief Check USB reconnect timer.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_check_usb_reconnect_timer()

{
    /* Check USB reconnect timer; persistent state is carried in core usb disconnect requested. */
    int core_is_software_timer_expired_result;
    uint32_t operation_result;

    operation_result = 0;
    if ((core_usb_disconnect_requested == '\0') && (core_is_software_timer_expired_result = core_is_software_timer_expired(), core_is_software_timer_expired_result != 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Process flash validation context.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_flash_validation_context()

{
    /* Process flash validation context using only caller-provided data. */
    int core_get_identity_variant_code_result;
    int core_get_response_status_value_result;
    uint32_t operation_result;

    core_get_identity_variant_code_result = core_get_identity_variant_code();
    core_get_response_status_value_result = core_get_response_status_value();
    operation_result = 0;
    if ((core_get_response_status_value_result == 0) && (core_get_identity_variant_code_result == 0x38 || core_get_identity_variant_code_result == 0x35)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Release mww device command.
 *
 * @return No value.
 */
void core_release_mww_device_command()

{
    /* Update usb in endpoint info for MWW process resource; persistent state is carried in core mww device command active. */
    uint8_t *core_mww_device_command_active_pointer;
    uint8_t *callback_result = 0;

    core_mww_device_command_active_pointer = (uint8_t *)&core_mww_device_command_active;
    if (core_mww_device_command_active != '\0') {
        core_release_device_command();
        core_mww_device_command_active_pointer = callback_result;
    }
    *core_mww_device_command_active_pointer = 0;
    core_mww_device_command_active_pointer[1] = 0;
    return;
}

/**
 * @brief Update flash update context.
 *
 * @return No value.
 */
void core_update_flash_update_context()

{
    /* Update flash update context; persistent state is carried in core flash update context, core mww dispatch blocked flag. */
    if ((core_flash_update_context != '\0') && (core_mww_dispatch_blocked_flag == '\0')) {
        core_flash_update_context = 0;
        return;
    }
    return;
}

/**
 * @brief Clear transfer byte counters.
 *
 * @return No value.
 */
void core_clear_transfer_byte_counters()

{
    /* Clear transfer byte counters; persistent state is carried in core command written bytes low, core command written bytes high and related record fields. */
    core_command_written_bytes_low = 0;
    core_command_written_bytes_high = 0;
    core_command_read_bytes_low = 0;
    core_command_read_bytes_high = 0;
    return;
}

/**
 * @brief Get mode sense block descriptor length.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_mode_sense_block_descriptor_length(offset)
int offset;

{
    /* Get mode sense block descriptor length using only caller-provided data. */
    uint32_t operation_result;

    if (((*(uint8_t *)(*(int *)(offset + 8) + 2) & 0x3f) == 5) || ((*(uint8_t *)(*(int *)(offset + 8) + 1) >> 3 & 1) != 0)) {
        operation_result = 0;
    } else {
        operation_result = 8;
    }
    return operation_result;
}

/**
 * @brief Process command context value.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_command_context_value()

{
    /* Process command context value using only caller-provided data. */
    int operation_status;
    uint32_t operation_result;

    operation_status = core_is_command_write_blocked();
    operation_result = 1;
    if (operation_status == 0) {
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_update_availability_x01(operation_status + 0x2c);
        if (operation_status != 0) {
            operation_result = 0;
        }
    }
    return operation_result;
}

/**
 * @brief Get scsi command response buffer.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
int core_select_scsi_response_buffer(offset)
int offset;

{
    /* Get scsi command response buffer; persistent state is carried in scsi command response buffer. */
    *(uint8_t **)(offset + 0x14) = (uint8_t *)&scsi_command_response_buffer;
    return offset + 0x10;
}

/**
 * @brief Process command feature state.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_process_command_feature_state(offset) int offset;

{
    /* Process command feature state using only caller-provided data. */
    uint8_t mode_flags;
    int core_update_command_feature_state_result;

    mode_flags = *(uint8_t *)(*(int *)(offset + 8) + 4);
    if (((mode_flags & 1) == 0) ||
        (core_update_command_feature_state_result = ((bool (*)())core_update_command_phase)(offset, mode_flags >> 1), core_update_command_feature_state_result == 0)) {
        core_update_command_feature_state(mode_flags & 1);
    }
    return;
}

/**
 * @brief Process runtime context base.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_process_runtime_context_base(buffer) uint32_t *buffer;

{
    /* Process runtime context base using only caller-provided data. */
    int operation_status;

    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    operation_status = core_accumulate_split_transfer_offsets(operation_status + 0x2c);
    if ((operation_status == 0) || (operation_status = core_execute_ata_flush_cache(), operation_status == 0)) {
        *buffer = 0x16;
    }
    return;
}

/**
 * @brief Enable processor sleep clock gating.
 *
 * @return No value.
 */
void core_enable_processor_sleep_clock_gating()

{
    /* Enable processor sleep clock gating; persistent state is carried in USB core revision. */
    if (0x101a < usb_core_revision) {
        /* Program CDDIS REG OFF with 3; write ordering is hardware-significant. */
        CDDIS_REG_OFF = 3;
    }
    return;
}

/**
 * @brief Is USB endpoint stalled.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_usb_endpoint_stalled(condition)
uint32_t condition;

{
    /* Is USB endpoint stalled; persistent state is carried in usb ep0 out stalled, usb ep0 in stalled. */
    uint32_t *data_cursor;

    if ((condition >> 7 & 1) == 0) {
        data_cursor = (uint32_t *)&usb_ep0_out_stalled;
    } else {
        data_cursor = (uint32_t *)&usb_ep0_in_stalled;
    }
    return data_cursor[(condition & 0xffffff7f) * 10];
}

/**
 * @brief Register USB BOT transfer callback.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_register_usb_bot_transfer_callback(condition, value) uint32_t condition;
uint32_t value;

{
    /* Register USB BOT transfer callback; persistent state is carried in core usb bot out transfer callbacks, core usb bot in transfer callbacks. */
    uint8_t *data_cursor;

    if ((condition >> 7 & 1) == 0) {
        data_cursor = (uint8_t *)&core_usb_bot_out_transfer_callbacks;
    } else {
        data_cursor = (uint8_t *)&core_usb_bot_in_transfer_callbacks;
    }
    *(uint32_t *)(data_cursor + (condition & 0xffffff7f) * 4 + -8) = value;
    return;
}

/**
 * @brief Is regular request code.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_regular_request_code(offset)
int offset;

{
    /* Is regular request code using only caller-provided data. */
    if ((((offset != 0x37) && (offset != 0xf1)) && (3 < offset - 0xf3U)) && (offset - 0xf3U != 6)) {
        return 1;
    }
    return 0;
}

/**
 * @brief Get 0x40.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_get_0x40(offset, buffer) int offset;
uint8_t *buffer;

{
    /* Get 0x40 using only caller-provided data. */
    bool offset_field_0x40;

    offset_field_0x40 = (*(uint32_t *)(offset + 0x40) & 0xff) == 1;
    if ((offset_field_0x40) && (buffer != (uint8_t *)0x0)) {
        *buffer = (char)(*(uint32_t *)(offset + 0x40) >> 8);
    }
    return offset_field_0x40;
}

/**
 * @brief Update x01 x02.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_x01_x02(offset)
int offset;

{
    /* Update x01 x02 using only caller-provided data. */
    char status_bits;
    uint32_t operation_result;

    status_bits = *(char *)(offset + 0x10);
    operation_result = 0;
    if (((status_bits == '\x01') || (status_bits == '\x02' || status_bits == '\x04')) && (*(int *)(offset + 0x1c) == 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Get payload offset from flags.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
int core_get_payload_offset_from_flags(offset)
int offset;

{
    /* Get payload offset from flags using only caller-provided data. */
    int working_result;

    if ((*(uint8_t *)(offset + 1) >> 2 & 1) == 0) {
        if ((*(uint8_t *)(offset + 1) & 1) == 0) {
            working_result = 4;
        } else {
            working_result = 0xc;
        }
    } else {
        working_result = 0x14;
    }
    return working_result + *(int *)(offset + 0x14);
}

/**
 * @brief Copy source record header.
 *
 * @param source_record Procedure input.
 * @param destination_record Procedure input.
 * @return No value.
 */
uint32_t core_copy_source_record_header(source_record, destination_record)
uint32_t *source_record;
uint32_t *destination_record;

{
    /* Copy source record header using only caller-provided data. */
    int core_process_capacity_block_context_derived;

    core_process_capacity_block_context_derived = ((uint32_t (*)())core_process_capacity_block_context)();
    if (core_process_capacity_block_context_derived != 0) {
        destination_record[2] = (uint32_t)source_record;
        *destination_record = *source_record;
        destination_record[1] = source_record[1];
    }
    return 0;
}

/**
 * @brief Validate device descriptor length.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param offset Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_validate_device_descriptor_length(context, value, offset, flags)
uint32_t context;
uint32_t value;
int offset;
uint32_t flags;

{
    /* Validate device descriptor length using only caller-provided data. */
    int core_get_ti_memset_derived;

    core_get_ti_memset_derived = ((uint32_t (*)())core_get_ti_memset)(context, value, offset, flags, 0xc0);
    if ((core_get_ti_memset_derived != 0) && (*(uint16_t *)(offset + 8) < 0x4c)) {
        core_get_ti_memset_derived = 0;
    }
    return core_get_ti_memset_derived;
}

/**
 * @brief Has device descriptor high bit.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_has_device_descriptor_high_bit(offset)
int offset;

{
    /* Has device descriptor high bit using only caller-provided data. */
    uint32_t operation_result;

    operation_result = 0;
    if (((*(int16_t *)(offset + 8) != 0) && (*(uint8_t **)(offset + 0xc) != (uint8_t *)0x0)) &&
        ((**(uint8_t **)(offset + 0xc) & 0x80) == 0x80)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Handle SCSI security protocol.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_handle_scsi_security_protocol(offset, value, length, flags)
int offset;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Handle SCSI security protocol using only caller-provided data. */
    char operation_status;
    uint16_t record_type_code;
    int16_t record_type;
    int operation_result;
    uint32_t core_release_record_payload_when_short_result;
    uint32_t core_get_transfer_buffer_capacity_result;
    int computed_offset;
    uint32_t length_component;
    volatile uint32_t flags_snapshot = 0;

    if (**(int8_t **)(offset + 8) != -0x5e) {
        if (**(int8_t **)(offset + 8) == -0x4b) {
            computed_offset = offset + 4;
            length_component = length;
            flags_snapshot = flags;
            operation_status = core_release_record_payload_when_short(computed_offset, 1, 1);
            operation_result = core_get_validation_flags();
            if (operation_result == 0) {
                operation_result = 8;
                if ((operation_status == ' ') && (record_type = core_release_record_payload_when_short(computed_offset, 2, 3), record_type == 0x10)) {
                    core_release_record_payload_when_short_result = core_release_record_payload_when_short(computed_offset, 6, 9);
                    core_get_transfer_buffer_capacity_result = core_get_transfer_buffer_capacity();
                    if (core_release_record_payload_when_short_result <= core_get_transfer_buffer_capacity_result) {
                        operation_result = ((uint32_t (*)())core_get_active_command_response_buffer)(offset, &length_component, core_release_record_payload_when_short_result & 0xffff);
                        if (operation_result == 0) {
                            operation_result = 0x1f;
                        } else {
                            operation_result = ((uint64_t (*)())core_dispatch_rdx_management_record)(&length_component);
                        }
                    }
                }
            } else {
                operation_result = 0x2c;
            }
            return operation_result;
        }
        return 7;
    }
    operation_status = *(char *)(*(int *)(offset + 8) + 1);
    operation_result = 8;
    if (operation_status == '\0') {
        record_type = core_release_record_payload_when_short(offset + 4, 2, 3);
        if (record_type == 0) {
            operation_result = core_build_record_type_two_response(offset);
        } else if (record_type == 1) {
            core_initialize_scsi_response_buffer(offset, 4);
            goto core_descriptor_dispatch_finalize_payload;
        }
    } else if (operation_status == ' ') {
        record_type_code = core_release_record_payload_when_short(offset + 4, 2, 3);
        if (record_type_code == 0) {
            operation_result = core_build_supported_record_types_response(offset);
        } else if (record_type_code == 1) {
            operation_result = core_build_record_type_one_response(offset);
        } else if (record_type_code == 0x10) {
            operation_result = core_process_ti_memset_for_type_type(offset);
        } else if (record_type_code == 0x11) {
            operation_result = core_build_record_type_eleven_response(offset);
        } else if (record_type_code == 0x12) {
            operation_result = core_build_record_type_twelve_response(offset);
        } else if (record_type_code == 0x20) {
            operation_result = core_handle_descriptor_clear_memory_device_descriptor_context(offset);
        } else if (record_type_code == 0x21) {
            operation_result = core_handle_device_descriptor_response_flag_device_descriptor_context(offset);
        } else if (0xfeff < record_type_code) {
            if (record_type_code < 0xff08) {
                operation_result = core_handle_response_base_device_descriptor_context(offset, (char)record_type_code);
            } else if ((0xff0f < record_type_code) && (record_type_code < 0xff18)) {
                operation_result = core_handle_word_metadata(offset, record_type_code - 0x10 & 0xff);
            }
        }
    }
    if (operation_result != 0) {
        return operation_result;
    }
/* Shared target for core descriptor dispatch finalize payload; incoming paths preserve the same state assumptions. */
core_descriptor_dispatch_finalize_payload:
    if (*(int *)(offset + 0xc) != 0) {
        core_update_release_record_payload(*(int *)(offset + 0xc), offset, 6, 9);
    }
    return 0;
}

/**
 * @brief Process request for MWW saved request.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
uint32_t core_process_request_for_mww_saved_request(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Process request for MWW saved request using only caller-provided data. */
    int core_is_supported_transfer_opcode_derived;

    core_is_supported_transfer_opcode_derived = ((uint32_t (*)())core_is_supported_transfer_opcode)();
    if (core_is_supported_transfer_opcode_derived != 0) {
        core_process_request_request_for_request_request(context, value, length);
    }
    return 0;
}

/**
 * @brief Write selected sense fields.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_write_selected_sense_fields(context, value, length) uint8_t context;
uint8_t value;
uint32_t length;

{
    /* Write selected sense fields using only caller-provided data. */
    core_set_selected_sense_key(value);
    core_set_selected_sense_asc_ascq(length);
    core_set_selected_sense_status(context);
    return;
}

/**
 * @brief Get pending transfer buffer.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_get_pending_transfer_buffer(context, buffer) uint32_t context;
uint32_t *buffer;

{
    /* Get pending transfer buffer using only caller-provided data. */
    int core_find_scsi_opcode_entry_derived;

    core_find_scsi_opcode_entry_derived = (int)((uint8_t *(*)())core_find_scsi_opcode_entry)();
    if (core_find_scsi_opcode_entry_derived != 0) {
        *buffer = *(uint32_t *)(core_find_scsi_opcode_entry_derived + 4);
    }
    return core_find_scsi_opcode_entry_derived != 0;
}
