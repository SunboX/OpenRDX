/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Process cartridge input state.
 *
 * @return No value.
 */
void core_process_cartridge_input_state()

{
    /* Process cartridge input state; persistent state is carried in core cartridge input state. */
    int sci_read_logical_input_result;

    if (core_cartridge_input_state == 6) {
        core_cartridge_input_state = 2;
        sci_read_logical_input_result = sci_read_logical_input(0);
        core_process_command_phase(sci_read_logical_input_result == 0);
    }
    return;
}

/**
 * @brief Process ATA command for lookup key metadata.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
bool core_process_ata_command_for_lookup_key_metadata(context) uint32_t context;

{
    /* Process ATA command for lookup key metadata using only caller-provided data. */
    int core_execute_ata_flush_cache_result;
    int working_result;

    /* Call core reset output record through the defined unprototyped function-pointer signature. */
    ((void (*)())core_reset_output_record)();
    core_execute_ata_flush_cache_result = core_execute_ata_flush_cache();
    working_result = core_get_runtime_context_base_for_build_scsi_mode();
    working_result = core_initialize_output_record_from_source(working_result + 0x2c, context);
    return working_result != 0 && core_execute_ata_flush_cache_result != 0;
}

/**
 * @brief Verify ATA media and refresh state.
 *
 * @return Result produced by the procedure.
 */
bool core_verify_ata_media_and_refresh_state()

{
    /* Verify ATA media and refresh state; persistent state is carried in core ata standby active. */
    int core_execute_ata_standby_or_verify_result;

    core_execute_ata_standby_or_verify_result = core_execute_ata_standby_or_verify(0);
    if (core_execute_ata_standby_or_verify_result != 0) {
        core_apply_thermal_pwm_override(core_execute_ata_standby_or_verify_result);
        core_ata_standby_active = 0;
        core_process_command_state_context();
    }
    return core_execute_ata_standby_or_verify_result != 0;
}

/**
 * @brief Process ti memset ti memset.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_process_ti_memset_ti_memset(context, offset, length, flags)
uint32_t context;
int offset;
uint32_t length;
uint32_t flags;

{
    /* Process ti memset ti memset using only caller-provided data. */
    uint32_t ti_memset_result;
    uint32_t context_derived;
    int result_context;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;

    context_derived = context;
    result_context = offset;
    length_component = length;
    flags_snapshot = flags;
    ((uint64_t (*)())core_process_clear_memory_clear_memory_secondary_memory_region)(&context_derived, offset + 0xc);
    /* Call core build type one fingerprint through the defined unprototyped function-pointer signature. */
    ((void (*)())core_build_type_one_fingerprint)(context, &context_derived, offset);
    ti_memset_result = (uint32_t)ti_memset(&context_derived, 0, 0x10);
    return CONCAT44(result_context, ti_memset_result);
}

/**
 * @brief Commit firmware image header.
 *
 * @return Result produced by the procedure.
 */
bool core_commit_firmware_image_header()

{
    /* Commit firmware image header; persistent state is carried in core firmware image commit word, core transfer ready flag and related record fields. */
    int operation_status;

    operation_status = ((int (*)())core_program_spi_flash_pages)(0, 4, &core_firmware_image_commit_word);
    if (operation_status == 0) {
        core_transfer_ready_flag = 0;
        core_firmware_image_validation_active = 0;
    }
    return operation_status == 0;
}

/**
 * @brief Is ADC sample above threshold.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_adc_sample_above_threshold(condition, value, length, flags)
int condition;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Is ADC sample above threshold; persistent state is carried in core mcp3008 channel thresholds. */
    int operation_status;
    uint32_t operation_result;
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    operation_status = ((int (*)())core_read_mcp3008_channel)(condition, &flags_snapshot);
    operation_result = 0;
    if ((operation_status == 0) && ((uint32_t)*(uint16_t *)(&core_mcp3008_channel_thresholds + condition * 2) < (flags_snapshot & 0xffff))) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Set channel mode bits.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_set_channel_mode_bits(context, condition) int context;
int condition;

{
    /* Set channel mode bits using only caller-provided data. */
    uint32_t context_derived;

    context_derived = *(uint32_t *)(context * 0x400 + -0x877f8);
    if (condition == 0) {
        context_derived = context_derived & 0xffffffdc;
    } else {
        context_derived = context_derived & 0xfffffffc | 0x20;
    }
    *(uint32_t *)(context * 0x400 + -0x877f8) = context_derived;
    return;
}

/**
 * @brief Is mechanism transition pending.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_is_mechanism_transition_pending()

{
    /* Is mechanism transition pending; persistent state is carried in core mechanism completion inhibited, core mechanism transition timer and related record fields. */
    int core_is_software_timer_expired_result;
    uint32_t operation_result;

    operation_result = 1;
    if (((core_mechanism_completion_inhibited == '\0') && (core_is_software_timer_expired_result = core_is_software_timer_expired(&core_mechanism_transition_timer), core_is_software_timer_expired_result != 0)) &&
        (core_mechanism_output_state == 4 || core_mechanism_output_state == 8)) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Subtract split product.
 *
 * @param value_words Procedure input.
 * @param value Procedure input.
 * @param high_multiplier Procedure input.
 * @param low_multiplier Low half of the scaled subtraction multiplier.
 * @return No value.
 */
uint32_t core_subtract_split_product(value_words, value, high_multiplier, low_multiplier)
uint32_t *value_words;
int value;
int high_multiplier;
int low_multiplier;

{
    /* Subtract split product using only caller-provided data. */
    int value_derived;
    uint32_t buffer_1_cursor;

    value_derived = value * low_multiplier + value * high_multiplier * 0x10000;
    buffer_1_cursor = *value_words - value_derived;
    *value_words = buffer_1_cursor;
    if (-value_derived - 1U < buffer_1_cursor) {
        value_words[1] = value_words[1] - 1;
    }
    value_words[1] = value_words[1] - ((uint32_t)(value * high_multiplier) >> 0x10);
    return 0;
}

/**
 * @brief Register USB power management callback.
 *
 * @param context USB event callback address.
 * @return True when the registry is full; false after registration.
 */
bool core_register_usb_power_management_callback(context) uint32_t context;

{
    /* Append the callback while capacity remains, then publish the updated count. */
    uint32_t core_usb_power_management_callback_count_snapshot;

    core_usb_power_management_callback_count_snapshot = (uint32_t)core_usb_power_management_callback_count;
    if (3 >= core_usb_power_management_callback_count_snapshot) {
        *(uint32_t *)(&core_usb_power_management_callbacks + core_usb_power_management_callback_count_snapshot * 4) = context;
        core_usb_power_management_callback_count = core_usb_power_management_callback_count + 1;
    }
    return 3 < core_usb_power_management_callback_count_snapshot;
}

/**
 * @brief Write descriptor fields.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_write_descriptor_fields(context, buffer, length) uint32_t context;
uint8_t *buffer;
uint32_t length;

{
    /* Write descriptor fields using only caller-provided data. */
    ((void (*)())core_append_little_endian_value)(context, *buffer, 1, length);
    /* Call core append little endian value through the defined unprototyped function-pointer signature. */
    ((void (*)())core_append_little_endian_value)(context, buffer[1], 1);
    /* Call core append little endian value through the defined unprototyped function-pointer signature. */
    ((void (*)())core_append_little_endian_value)(context, *(uint16_t *)(buffer + 2), 2);
    return;
}

/**
 * @brief Get structure field hex for response end word.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_get_structure_field_hex_for_response_end_word(offset)
int offset;

{
    /* Get structure field hex for response end word using only caller-provided data. */
    uint32_t core_get_structure_field_hex_for_structure_field_hex_result;
    int callback_result = 0;

    if (*(char *)(offset + 0x6f) != '\0') {
        core_get_structure_field_hex_for_structure_field_hex_result = core_get_structure_field_hex_for_structure_field_hex(offset);
        return CONCAT44(*(int *)(callback_result + 0x68) - (uint32_t)(*(uint32_t *)(callback_result + 100) < core_get_structure_field_hex_for_structure_field_hex_result),
                        *(uint32_t *)(callback_result + 100) - core_get_structure_field_hex_for_structure_field_hex_result);
    }
    return 0;
}

/**
 * @brief Process copy.
 *
 * @param buffer Procedure input.
 * @param destination_offset Procedure input.
 * @param length Procedure input.
 * @param copy_length Procedure input.
 * @return No value.
 */
void core_process_copy(buffer, destination_offset, length, copy_length) int *buffer;
int destination_offset;
uint32_t length;
int copy_length;

{
    /* Process copy using only caller-provided data. */
    if (0x800 < (uint32_t)(copy_length + destination_offset)) {
        core_halt_forever();
    }
    /* Process copy; this branch selects whether its associated handler runs. */
    core_copy_bytes(*buffer + destination_offset * 2, length, copy_length << 1);
    return;
}

/**
 * @brief Get runtime context base for command descriptor device.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
int core_get_runtime_context_base_for_command_descriptor_device(condition)
int condition;

{
    /* Get runtime context base for command descriptor device using only caller-provided data. */
    int operation_result;

    operation_result = core_get_runtime_context_base_for_build_scsi_mode();
    operation_result = core_is_regular_request_code(condition, operation_result + 0x2c);
    if (((operation_result != 0) && (condition != 0xb0)) && (condition != 0xe0)) {
        core_process_command_state_context();
    }
    return operation_result;
}

/**
 * @brief Is SCSI write opcode.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_scsi_write_opcode(condition)
int condition;

{
    /* Is SCSI write opcode using only caller-provided data. */
    if ((((condition != 10) && (condition != 0x2a)) && (condition != 0x2e)) &&
        (((condition != 0x8a && (condition != 0x8e)) && (condition != 0xaa && condition != 0xae)))) {
        return 0;
    }
    return 1;
}

/**
 * @brief Update valid.
 *
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_update_valid(condition, buffer) int condition;
int *buffer;

{
    /* Update valid using only caller-provided data. */
    bool entry_valid;
    int working_result;

    entry_valid = false;
    /* Gate the following state transition on (*buffer == 0) || (condition != 0); the handler is skipped when this condition is false. */
    if ((*buffer == 0) || (condition != 0)) {
        working_result = 0;
    } else {
        entry_valid = true;
        if (buffer[2] == 3) {
            working_result = 4;
        } else {
            entry_valid = buffer[2] != 0;
            working_result = 2;
        }
    }
    buffer[2] = working_result;
    return entry_valid;
}

/**
 * @brief Initialize cartridge monitor.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_initialize_cartridge_monitor(offset) int offset;

{
    /* Initialize cartridge monitor using only caller-provided data. */
    ti_memset(offset, 0, 0x14);
    /* Call core allocate software timer through the defined unprototyped function-pointer signature. */
    ((void (*)())core_allocate_software_timer)(offset + 0xd);
    /* Call core allocate software timer through the defined unprototyped function-pointer signature. */
    ((void (*)())core_allocate_software_timer)(offset + 0xe);
    /* Call core allocate software timer through the defined unprototyped function-pointer signature. */
    ((void (*)())core_allocate_software_timer)(offset + 0xf);
    return;
}

/**
 * @brief Update constant 0x12 x01.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_constant_0x12_x01(offset)
int offset;

{
    /* Update constant 0x12 x01 using only caller-provided data. */
    char offset_field_0x12;
    uint32_t operation_result;

    operation_result = 0;
    if ((*(int *)(offset + 0x44) == 2) && (((offset_field_0x12 = *(char *)(offset + 0x12),
                                             offset_field_0x12 == '\x01' || (offset_field_0x12 == '\x02' || offset_field_0x12 == '\x04')) &&
                                            ((*(uint32_t *)(offset + 0x24) & ~*(uint32_t *)(offset + 0x74)) == 0)))) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Update device for component for device.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_device_for_component_for_device(offset, value, length)
int offset;
uint32_t value;
uint32_t length;

{
    /* Update device for component for device using only caller-provided data. */
    int core_update_device_for_device_for_component_result;
    uint32_t operation_result;

    *(uint8_t *)(offset + 8) = 1;
    core_update_device_for_device_for_component_result = core_update_device_for_device_for_component(value, length);
    if (core_update_device_for_device_for_component_result == 0) {
        operation_result = 3;
    } else {
        operation_result = 0;
    }
    *(uint8_t *)(offset + 8) = 0;
    return operation_result;
}

/**
 * @brief Update device for device ti memset.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_device_for_device_ti_memset(context, value, length)
int context;
uint32_t value;
uint32_t length;

{
    /* Update device for device ti memset using only caller-provided data. */
    int core_update_device_for_device_for_device_result;
    uint32_t operation_result;

    *(uint8_t *)(context + 9) = 1;
    core_update_device_for_device_for_device_result = core_update_device_for_device_for_device(value, length);
    if (core_update_device_for_device_for_device_result == 0) {
        operation_result = 4;
    } else {
        operation_result = 0;
    }
    *(uint8_t *)(context + 9) = 0;
    return operation_result;
}

/**
 * @brief Reset ATA callback queue.
 *
 * @return No value.
 */
void core_reset_ata_callback_queue()

{
    /* Reset ATA callback queue; persistent state is carried in core ata callback write index. */
    int operation_status;
    uint8_t *ata_callback_pending_pointer;

    operation_status = 0x10;
    ata_callback_pending_pointer = (uint8_t *)&ata_callback_pending;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        operation_status = operation_status + -1;
        *ata_callback_pending_pointer = 0;
        ata_callback_pending_pointer = ata_callback_pending_pointer + 1;
    } while (operation_status != 0);
    core_ata_callback_write_index = 0;
    ata_callback_processing_index = 0;
    return;
}

/**
 * @brief Start AHCI command override if stopped.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
bool core_start_ahci_command_override_if_stopped(context) int context;

{
    /* Start AHCI command override if stopped; persistent state is carried in AHCI port command register. */
    bool ahci_port_command_register_pointer;

    ahci_port_command_register_pointer = (*(uint32_t *)(&ahci_port_command_register + context * 0x80) & 1) != 0;
    if (!ahci_port_command_register_pointer) {
        core_update_masked_register_bits(&ahci_port_command_register + context * 0x80, 0x10, 0x10);
    }
    return ahci_port_command_register_pointer;
}

/**
 * @brief Is cartridge startup delay active.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_is_cartridge_startup_delay_active()

{
    /* Is cartridge startup delay active; persistent state is carried in core cartridge startup delay timer. */
    int operation_status;
    uint32_t operation_result;

    operation_result = 0;
    operation_status = core_get_identity_variant_code(0);
    if ((operation_status == 0x38) && (operation_status = core_is_software_timer_expired(&core_cartridge_startup_delay_timer), operation_status == 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Decode indexed bytes.
 *
 * @param destination Procedure input.
 * @param source Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_decode_indexed_bytes(destination, source, condition) char *destination;
char *source;
int condition;

{
    /* Decode indexed bytes using only caller-provided data. */
    char buffer_2_cursor;
    char working_result;

    if (condition != 0) {
        working_result = '\0';
        /* Repeat while condition != 0; the body advances or polls the state needed to leave the loop. */
        do {
            condition = condition + -1;
            buffer_2_cursor = *source - working_result;
            working_result = working_result + '\x01';
            *destination = buffer_2_cursor + -0x54;
            source = source + 1;
            destination = destination + 1;
        } while (condition != 0);
    }
    return;
}

/**
 * @brief Process constant 0x24 fixed command context.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_constant_0x24_fixed_command_context(offset)
int offset;

{
    /* Process constant 0x24 fixed command context; persistent state is carried in core signature verification key descriptor. */
    uint16_t offset_field_0x24;
    uint32_t core_signature_verification_key_descriptor_pointer;
    uint32_t status_bits;
    uint8_t working_buffer[256];

    if (8 < *(uint16_t *)(offset + 0x24)) {
        offset_field_0x24 = *(uint16_t *)(offset + 0x24) - 8;
        status_bits = (uint32_t)offset_field_0x24;
        core_signature_verification_key_descriptor_pointer = 0;
        if (((status_bits != 0) && ((offset_field_0x24 & 7) == 0)) && (status_bits < 0x104)) {
            /* Call core decode big endian multiword integer through the defined unprototyped function-pointer signature. */
            ((void (*)())core_decode_big_endian_multiword_integer)(working_buffer, offset + 0x2c, (int)status_bits >> 2 & 0xff);
            core_signature_verification_key_descriptor_pointer = ((uint32_t (*)())core_format_hex_u32_command_words_command_words)(offset + 300, working_buffer, &core_signature_verification_key_descriptor);
        }
        return core_signature_verification_key_descriptor_pointer;
    }
    return 0;
}

/**
 * @brief Translate command code.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_translate_command_code(context, value)
uint32_t context;
uint32_t value;

{
    /* Translate command code using only caller-provided data. */
    int core_is_command_code_in_standard_range_result;
    uint32_t operation_result;
    int callback_result = 0;

    core_is_command_code_in_standard_range_result = core_is_command_code_in_standard_range(context, value, context);
    if (core_is_command_code_in_standard_range_result == 0) {
        operation_result = 0xff;
    } else {
        operation_result = ((callback_result - 0x36U & 7) - (callback_result - 0x36U & 0xf8)) + 0x78 & 0xff;
    }
    return operation_result;
}

/**
 * @brief Compare buffers from end.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
int core_compare_buffers_from_end(context, value, condition)
int context;
int value;
int condition;

{
    /* Compare buffers from end using only caller-provided data. */
    char *value_derived;
    char *context_derived;

    if (condition != 0) {
        context_derived = (char *)(context + condition);
        value_derived = (char *)(value + condition);
        /* Repeat while condition != 0; the body advances or polls the state needed to leave the loop. */
        do {
            value_derived = value_derived + -1;
            context_derived = context_derived + -1;
            condition = condition + -1;
            /* Gate the following state transition on *value_derived != *context_derived; the handler is skipped when this condition is false. */
            if (*value_derived != *context_derived) {
                return condition;
            }
        } while (condition != 0);
    }
    return 0;
}

/**
 * @brief Process ATA command buffer for clear memory lookup.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_ata_command_buffer_for_clear_memory_lookup(context)
uint32_t context;

{
    /* Process ATA command buffer for clear memory lookup using only caller-provided data. */
    uint32_t operation_result;
    int working_result;
    uint64_t core_get_runtime_context_base_for_build_scsi_mode_result;

    operation_result = (uint32_t)core_get_cartridge_ata_device_info();
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode(operation_result, operation_result);
    working_result = (int)((uint64_t)core_get_runtime_context_base_for_build_scsi_mode_result >> 0x20);
    operation_result = ((bool (*)())core_configure_initialize_output_record)((int)core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, *(uint32_t *)(working_result + 8),
                                                        *(uint32_t *)(working_result + 0xc));
    core_process_configuration_block_payload(context);
    return operation_result;
}

/**
 * @brief Process configuration block payload.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_process_configuration_block_payload(context) uint32_t context;

{
    /* Process configuration block payload using only caller-provided data. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;
    uint8_t working_buffer[40];
    uint8_t payload_buffer[296];
    uint8_t configuration_block[136];

    core_configure_word_descriptor(working_buffer, context, payload_buffer, configuration_block);
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    /* Call core transfer command sequence update flag through the defined unprototyped function-pointer signature. */
    ((void (*)())core_transfer_command_sequence_update_flag)(working_buffer, core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c);
    return;
}

/**
 * @brief Build record type two response.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_record_type_two_response(context)
uint32_t context;

{
    /* Build record type two response using only caller-provided data. */
    uint32_t core_initialize_scsi_response_buffer_result;

    core_initialize_scsi_response_buffer_result = (uint32_t)core_initialize_scsi_response_buffer(context, 10);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 6, 7, 2);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 9, 9, 0x20);
    return 0;
}

/**
 * @brief Update constant 0xc for word words resource.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_update_constant_0xc_for_word_words_resource(offset, buffer) int offset;
uint32_t *buffer;

{
    /* Update constant 0xc for word words resource using only caller-provided data. */
    int offset_field_0xc;
    bool buffer_cursor;

    buffer_cursor = false;
    if ((((*(uint16_t *)(offset + 8) < 4) || (offset_field_0xc = *(int *)(offset + 0xc), offset_field_0xc == 0)) ||
         (*(char *)(offset_field_0xc + 2) != '\0')) ||
        (buffer_cursor = *(char *)(offset_field_0xc + 3) == '\0', !buffer_cursor)) {
        *buffer = 0x2b;
    }
    return buffer_cursor;
}

/**
 * @brief Hold logical output fourteen.
 *
 * @return No value.
 */
void core_hold_logical_output_fourteen()

{
    /* Hold logical output fourteen; persistent state is carried in core logical output fourteen hold timer. */
    spi_set_logical_output(0xe, 1);
    /* Call core allocate software timer through the defined unprototyped function-pointer signature. */
    ((void (*)())core_allocate_software_timer)(&core_logical_output_fourteen_hold_timer);
    rti_set_software_timer(&core_logical_output_fourteen_hold_timer, 30000);
    return;
}

/**
 * @brief Block mww dispatch and claim device command.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_block_mww_dispatch_and_claim_device_command()

{
    /* Block mww dispatch and claim device command; persistent state is carried in core mww dispatch blocked flag, core device command busy. */
    uint32_t operation_result;
    int callback_result = 0;
    uint32_t default_result = 0;

    core_mww_dispatch_blocked_flag = 1;
    if (core_device_command_busy == '\0') {
        core_claim_device_command(0);
        *(uint8_t *)(callback_result + 3) = 0;
        operation_result = default_result;
    } else {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Find SCSI opcode entry.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t *core_find_scsi_opcode_entry(condition)
uint32_t condition;

{
    /* Find SCSI opcode entry; persistent state is carried in core scsi opcode table cursor origin. */
    uint8_t *core_scsi_opcode_table_cursor_origin_pointer;
    int operation_status;

    operation_status = 0x2a;
    core_scsi_opcode_table_cursor_origin_pointer = (uint8_t *)&core_scsi_opcode_table_cursor_origin;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_scsi_opcode_table_cursor_origin_pointer = core_scsi_opcode_table_cursor_origin_pointer + 0x18;
        if (condition == *core_scsi_opcode_table_cursor_origin_pointer) {
            return core_scsi_opcode_table_cursor_origin_pointer;
        }
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    return (uint8_t *)0x0;
}

/**
 * @brief Is supported transfer opcode.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_supported_transfer_opcode(context)
uint32_t context;

{
    /* Is supported transfer opcode using only caller-provided data. */
    uint32_t operation_result;
    int working_result;
    uint64_t core_is_scsi_write_opcode_derived;

    core_is_scsi_write_opcode_derived = ((uint32_t (*)())core_is_scsi_write_opcode)(context, context);
    working_result = (int)((uint64_t)core_is_scsi_write_opcode_derived >> 0x20);
    if (((((int)core_is_scsi_write_opcode_derived == 0) && (working_result != 8)) && (working_result != 0x28)) &&
        ((working_result != 0x88 && (working_result != 0xa8)))) {
        operation_result = 0;
    } else {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Get active command response buffer.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_active_command_response_buffer(context, buffer)
uint32_t context;
uint16_t *buffer;

{
    /* Get active command response buffer; persistent state is carried in core active command descriptor, scsi command response buffer. */
    int core_active_command_descriptor_snapshot;

    core_active_command_descriptor_snapshot = (int)core_active_command_descriptor;
    *(uint8_t **)(buffer + 2) = (uint8_t *)&scsi_command_response_buffer;
    *buffer = *(uint16_t *)(core_active_command_descriptor_snapshot + 4);
    return 1;
}
