/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
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
    int core_process_ata_command_for_ata_standby_context_result;
    int working_result;

    /* Call core reset output record through the defined unprototyped function-pointer signature. */
    ((void (*)())core_reset_output_record)();
    core_process_ata_command_for_ata_standby_context_result = core_process_ata_command_for_ata_standby_context();
    working_result = core_get_runtime_context_base_for_build_scsi_mode();
    working_result = core_initialize_output_record_from_source(working_result + 0x2c, context);
    return working_result != 0 && core_process_ata_command_for_ata_standby_context_result != 0;
}

/**
 * @brief Get gpio pin context.
 *
 * @return Result produced by the procedure.
 */
bool core_get_gpio_pin_context()

{
    /* Get gpio pin context; persistent state is carried in core gpio pin context. */
    int core_submit_ata_standby_immediate_result;

    core_submit_ata_standby_immediate_result = core_submit_ata_standby_immediate(0);
    if (core_submit_ata_standby_immediate_result != 0) {
        core_process_device_status_context(core_submit_ata_standby_immediate_result);
        core_gpio_pin_context = 0;
        core_process_command_state_context();
    }
    return core_submit_ata_standby_immediate_result != 0;
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
    /* Call core process USB event context through the defined unprototyped function-pointer signature. */
    ((void (*)())core_process_usb_event_context)(context, &context_derived, offset);
    ti_memset_result = (uint32_t)ti_memset(&context_derived, 0, 0x10);
    return CONCAT44(result_context, ti_memset_result);
}

/**
 * @brief Update flash completion context.
 *
 * @return Result produced by the procedure.
 */
bool core_update_flash_completion_context()

{
    /* Update flash completion context; persistent state is carried in core flash completion context, core transfer ready flag and related record fields. */
    int operation_status;

    operation_status = ((int (*)())core_program_spi_flash_pages)(0, 4, &core_flash_completion_context);
    if (operation_status == 0) {
        core_transfer_ready_flag = 0;
        core_mcp3008_sample = 0;
    }
    return operation_status == 0;
}

/**
 * @brief Update device update context.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_device_update_context(condition, value, length, flags)
int condition;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Update device update context; persistent state is carried in core device update context. */
    int operation_status;
    uint32_t operation_result;
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    operation_status = ((int (*)())core_read_mcp3008_channel)(condition, &flags_snapshot);
    operation_result = 0;
    if ((operation_status == 0) && ((uint32_t)*(uint16_t *)(&core_device_update_context + condition * 2) < (flags_snapshot & 0xffff))) {
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
 * @brief Update sense record context.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_update_sense_record_context()

{
    /* Update sense record context; persistent state is carried in core sense record context, core command completion context and related record fields. */
    int core_get_thermal_control_context_result;
    uint32_t operation_result;

    operation_result = 1;
    if (((core_sense_record_context == '\0') && (core_get_thermal_control_context_result = core_get_thermal_control_context(&core_command_completion_context), core_get_thermal_control_context_result != 0)) &&
        (core_state_111 == 4 || core_state_111 == 8)) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Update multiplier multiplier.
 *
 * @param value_words Procedure input.
 * @param value Procedure input.
 * @param high_multiplier Procedure input.
 * @param low_multiplier Low half of the scaled subtraction multiplier.
 * @return No value.
 */
uint32_t core_update_multiplier_multiplier(value_words, value, high_multiplier, low_multiplier)
uint32_t *value_words;
int value;
int high_multiplier;
int low_multiplier;

{
    /* Update multiplier multiplier using only caller-provided data. */
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
 * @brief Update device label context.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
bool core_update_device_label_context(context) uint32_t context;

{
    /* Update device label context; persistent state is carried in core device label context, core state 198. */
    uint32_t core_device_label_context_snapshot;

    core_device_label_context_snapshot = (uint32_t)core_device_label_context;
    if (3 >= core_device_label_context_snapshot) {
        *(uint32_t *)(&core_state_198 + core_device_label_context_snapshot * 4) = context;
        core_device_label_context = core_device_label_context + 1;
    }
    return 3 < core_device_label_context_snapshot;
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
    ((void (*)())core_transfer_tracker)(context, *buffer, 1, length);
    /* Call core transfer tracker through the defined unprototyped function-pointer signature. */
    ((void (*)())core_transfer_tracker)(context, buffer[1], 1);
    /* Call core transfer tracker through the defined unprototyped function-pointer signature. */
    ((void (*)())core_transfer_tracker)(context, *(uint16_t *)(buffer + 2), 2);
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
    core_forward_end_remaining(*buffer + destination_offset * 2, length, copy_length << 1);
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
 * @brief Is supported descriptor opcode.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_supported_descriptor_opcode(condition)
int condition;

{
    /* Is supported descriptor opcode using only caller-provided data. */
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
 * @brief Process clear memory for cartridge monitor context.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_process_clear_memory_for_cartridge_monitor_context(offset) int offset;

{
    /* Process clear memory for cartridge monitor context using only caller-provided data. */
    ti_memset(offset, 0, 0x14);
    /* Call core update flag one interrupt transfer context through the defined unprototyped function-pointer signature. */
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(offset + 0xd);
    /* Call core update flag one interrupt transfer context through the defined unprototyped function-pointer signature. */
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(offset + 0xe);
    /* Call core update flag one interrupt transfer context through the defined unprototyped function-pointer signature. */
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(offset + 0xf);
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
 * @brief Transfer pending ATA queue.
 *
 * @return No value.
 */
void core_transfer_pending_ata_queue()

{
    /* Transfer pending ATA queue; persistent state is carried in core ATA queue context. */
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
    core_ata_queue_context = 0;
    ata_callback_processing_index = 0;
    return;
}

/**
 * @brief Get AHCI control register.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
bool core_get_ahci_control_register(context) int context;

{
    /* Get AHCI control register; persistent state is carried in AHCI port command register. */
    bool ahci_port_command_register_pointer;

    ahci_port_command_register_pointer = (*(uint32_t *)(&ahci_port_command_register + context * 0x80) & 1) != 0;
    if (!ahci_port_command_register_pointer) {
        core_update_masked_register_bits(&ahci_port_command_register + context * 0x80, 0x10, 0x10);
    }
    return ahci_port_command_register_pointer;
}

/**
 * @brief Process command processing context.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_command_processing_context()

{
    /* Process command processing context; persistent state is carried in core command processing context. */
    int operation_status;
    uint32_t operation_result;

    operation_result = 0;
    operation_status = core_get_flash_validation_context_for_validate_structured_flash(0);
    if ((operation_status == 0x38) && (operation_status = core_get_thermal_control_context(&core_command_processing_context), operation_status == 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Transfer x01.
 *
 * @param destination Procedure input.
 * @param source Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_transfer_x01(destination, source, condition) char *destination;
char *source;
int condition;

{
    /* Transfer x01 using only caller-provided data. */
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
    /* Process constant 0x24 fixed command context; persistent state is carried in core fixed command context. */
    uint16_t offset_field_0x24;
    uint32_t core_fixed_command_context_pointer;
    uint32_t status_bits;
    uint8_t working_buffer[256];

    if (8 < *(uint16_t *)(offset + 0x24)) {
        offset_field_0x24 = *(uint16_t *)(offset + 0x24) - 8;
        status_bits = (uint32_t)offset_field_0x24;
        core_fixed_command_context_pointer = 0;
        if (((status_bits != 0) && ((offset_field_0x24 & 7) == 0)) && (status_bits < 0x104)) {
            /* Call core transfer fourth byte byte through the defined unprototyped function-pointer signature. */
            ((void (*)())core_transfer_fourth_byte_byte)(working_buffer, offset + 0x2c, (int)status_bits >> 2 & 0xff);
            core_fixed_command_context_pointer = ((uint32_t (*)())core_transfer_packed_command_words_command_words)(offset + 300, working_buffer, &core_fixed_command_context);
        }
        return core_fixed_command_context_pointer;
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

    operation_result = (uint32_t)core_get_ata_command_buffer();
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
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;

    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 10);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 6, 7, 2);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 9, 9, 0x20);
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
 * @brief Process input pin state.
 *
 * @return No value.
 */
void core_process_input_pin_state()

{
    /* Process input pin state; persistent state is carried in core input pin state. */
    spi_set_logical_output(0xe, 1);
    /* Call core update flag one interrupt transfer context through the defined unprototyped function-pointer signature. */
    ((void (*)())core_update_flag_one_interrupt_transfer_context)(&core_input_pin_state);
    rti_process_thermal_timer_state(&core_input_pin_state, 30000);
    return;
}

/**
 * @brief Update default MWW transfer value.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_update_default_mww_transfer_value()

{
    /* Update default MWW transfer value; persistent state is carried in core MWW transfer value, core thermal monitor enabled. */
    uint32_t operation_result;
    int callback_result = 0;
    uint32_t default_result = 0;

    core_mww_transfer_value = 1;
    if (core_thermal_monitor_enabled == '\0') {
        core_set_thermal_monitor_enabled_to_one(0);
        *(uint8_t *)(callback_result + 3) = 0;
        operation_result = default_result;
    } else {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Transfer SPI transfer storage.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t *core_transfer_spi_transfer_storage(condition)
uint32_t condition;

{
    /* Transfer SPI transfer storage; persistent state is carried in core SPI transfer storage. */
    uint8_t *core_spi_transfer_storage_pointer;
    int operation_status;

    operation_status = 0x2a;
    core_spi_transfer_storage_pointer = (uint8_t *)&core_spi_transfer_storage;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_spi_transfer_storage_pointer = core_spi_transfer_storage_pointer + 0x18;
        if (condition == *core_spi_transfer_storage_pointer) {
            return core_spi_transfer_storage_pointer;
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
    uint64_t core_is_supported_descriptor_opcode_derived;

    core_is_supported_descriptor_opcode_derived = ((uint32_t (*)())core_is_supported_descriptor_opcode)(context, context);
    working_result = (int)((uint64_t)core_is_supported_descriptor_opcode_derived >> 0x20);
    if (((((int)core_is_supported_descriptor_opcode_derived == 0) && (working_result != 8)) && (working_result != 0x28)) &&
        ((working_result != 0x88 && (working_result != 0xa8)))) {
        operation_result = 0;
    } else {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Update command validation context.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_command_validation_context(context, buffer)
uint32_t context;
uint16_t *buffer;

{
    /* Update command validation context; persistent state is carried in core command validation context, MWW mode page buffer. */
    int core_command_validation_context_snapshot;

    core_command_validation_context_snapshot = (int)core_command_validation_context;
    *(uint8_t **)(buffer + 2) = (uint8_t *)&mww_mode_page_buffer;
    *buffer = *(uint16_t *)(core_command_validation_context_snapshot + 4);
    return 1;
}

