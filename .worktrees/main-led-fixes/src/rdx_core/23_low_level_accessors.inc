/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Update SPI configure SPI.
 *
 * @param context Procedure input.
 * @return No value.
 */
uint32_t core_update_spi_configure_spi(context)
uint32_t context;

{
    /* Update SPI configure SPI using only caller-provided data. */
    spi_transfer_word(context | 0x60000);
    return 0;
}

/**
 * @brief Get response end context pointer.
 *
 * @return Result produced by the procedure.
 */
uint8_t *core_get_response_end_context_pointer()

{
    /* Get response end context pointer; persistent state is carried in core response end context. */
    return (uint8_t *)&core_response_end_context;
}

/**
 * @brief Update thermal monitor enabled for read smart temperature.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_update_thermal_monitor_enabled_for_read_smart_temperature(buffer) uint32_t *buffer;

{
    /* Update thermal monitor enabled for read smart temperature; persistent state is carried in core thermal monitor enabled. */
    *buffer = 0;
    core_thermal_monitor_enabled = 0;
    return;
}

/**
 * @brief Get persistent identity record pointer.
 *
 * @return Result produced by the procedure.
 */
uint32_t *core_get_persistent_identity_record_pointer()

{
    /* Get persistent identity record pointer; persistent state is carried in core persistent identity record. */
    return (uint32_t *)&core_persistent_identity_record;
}

/**
 * @brief Sha256 update.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_sha256_update(offset, buffer, length) int offset;
uint8_t *buffer;
int length;

{
    /* Sha256 update using only caller-provided data. */
    uint32_t offset_field_0x44;

    offset = offset + 300;
    /* Iterate while ; length != 0; length = length + -1, preserving the defined cursor and bound. */
    for (; length != 0; length = length + -1) {
        *(uint8_t *)(offset + *(int *)(offset + 0x40)) = *buffer;
        *(int *)(offset + 0x40) = *(int *)(offset + 0x40) + 1;
        if (*(int *)(offset + 0x40) == 0x40) {
            offset = ((uint32_t (*)())core_sha256_compress_block)(offset, offset);
            offset_field_0x44 = *(uint32_t *)(offset + 0x44);
            *(uint32_t *)(offset + 0x40) = 0;
            *(uint32_t *)(offset + 0x44) = offset_field_0x44 + 0x200;
            *(uint32_t *)(offset + 0x48) = *(int *)(offset + 0x48) + (uint32_t)(0xfffffdff < offset_field_0x44);
        }
        buffer = buffer + 1;
    }
    return;
}

/**
 * @brief Get runtime context base for build SCSI mode.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_runtime_context_base_for_build_scsi_mode()

{
    /* Get runtime context base for build SCSI mode using only caller-provided data. */
    return 0x800f324;
}

/**
 * @brief Submit configured payload.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
uint32_t core_submit_configured_payload(context, buffer, length)
uint32_t context;
uint16_t *buffer;
uint32_t length;

{
    /* Submit configured payload using only caller-provided data. */
    ((uint32_t (*)())core_configure_payload_ti_memset)(context, *(uint32_t *)(buffer + 2), length, *buffer);
    return 0;
}

/**
 * @brief Update SPI set logical for monitor cartridge input.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_spi_set_logical_for_monitor_cartridge_input(context) uint32_t context;

{
    /* Update SPI set logical for monitor cartridge input using only caller-provided data. */
    spi_set_logical_output(5, context);
    return;
}

/**
 * @brief Read context field 6c byte.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t core_read_context_field_6c_byte(offset)
int offset;

{
    /* Read context field 6c byte using only caller-provided data. */
    return *(uint8_t *)(offset + 0x6c);
}

/**
 * @brief Get structure field hex 70 value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t core_get_structure_field_hex_70_value(offset)
int offset;

{
    /* Get structure field hex 70 value using only caller-provided data. */
    return *(uint8_t *)(offset + 0x70);
}

/**
 * @brief Get descriptor requested.
 *
 * @param requested_length Procedure input.
 * @param descriptor Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_descriptor_requested(requested_length, descriptor)
uint32_t requested_length;
int descriptor;

{
    /* Get descriptor requested using only caller-provided data. */
    uint32_t operation_result;

    operation_result = 0;
    if (((requested_length < 0x1fc) && (*(uint16_t *)(descriptor + 2) < 0x1fc)) && (requested_length + *(uint16_t *)(descriptor + 2) < 0x1fc)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Update SCI read logical for SCSI logical address context.
 *
 * @return No value.
 */
uint32_t core_update_sci_read_logical_for_scsi_logical_address_context()

{
    /* Update SCI read logical for SCSI logical address context using only caller-provided data. */
    sci_read_logical_input(3);
    return 0;
}

/**
 * @brief Mark primary sense record not ready.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_mark_primary_sense_record_not_ready(offset) int offset;

{
    /* Mark primary sense record not ready using only caller-provided data. */
    if (*(char *)(offset + 7) == '\0') {
        *(uint8_t *)(offset + 7) = 1;
    }
    *(uint8_t *)(offset + 8) = 6;
    return;
}

/**
 * @brief Mark secondary sense record not ready.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_mark_secondary_sense_record_not_ready(offset) int offset;

{
    /* Mark secondary sense record not ready using only caller-provided data. */
    if (*(char *)(offset + 7) == '\0') {
        *(uint8_t *)(offset + 7) = 1;
    }
    *(uint8_t *)(offset + 8) = 6;
    return;
}

/**
 * @brief Release nested record resource.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_release_nested_record_resource(offset) int offset;

{
    /* Release nested record resource using only caller-provided data. */
    ((void (*)())core_update_thermal_monitor_enabled_for_read_smart_temperature)(*(int *)(offset + 8) + 8);
    return;
}

/**
 * @brief Update SCI read logical for SCI read logical.
 *
 * @return No value.
 */
uint32_t core_update_sci_read_logical_for_sci_read_logical()

{
    /* Update SCI read logical for SCI read logical using only caller-provided data. */
    sci_read_logical_input(0);
    return 0;
}

/**
 * @brief Update SCI read logical for advance mechanism state.
 *
 * @return No value.
 */
uint32_t core_update_sci_read_logical_for_advance_mechanism_state()

{
    /* Update SCI read logical for advance mechanism state using only caller-provided data. */
    sci_read_logical_input(1);
    return 0;
}

/**
 * @brief Release record buffer.
 *
 * @param offset Procedure input.
 * @return No value.
 */
uint32_t core_release_record_buffer(offset)
int offset;

{
    /* Release record buffer using only caller-provided data. */
    ((uint32_t (*)())core_transfer_range_end_range_start_for_descriptor_descriptor_resource)(*(uint32_t *)(offset + 4));
    return 0;
}

/**
 * @brief Get constant hex 1014 value.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_constant_hex_1014_value()

{
    /* Get constant hex 1014 value using only caller-provided data. */
    return 0x1014;
}

/**
 * @brief Get constant hex 1000 value.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_constant_hex_1000_value()

{
    /* Get constant hex 1000 value using only caller-provided data. */
    return 0x1000;
}

/**
 * @brief Update MWW process transfer.
 *
 * @return No value.
 */
void core_update_mww_process_transfer()

{
    /* Update MWW process transfer using only caller-provided data. */
    mww_process_transfer_descriptor_mww0_addr_mww_transfer_descriptor();
    return;
}

/**
 * @brief Read mode page field 1c value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_mode_page_field_1c_value(offset)
int offset;

{
    /* Read mode page field 1c value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x1c);
}

/**
 * @brief Read mode page field 48 value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_mode_page_field_48_value(offset)
int offset;

{
    /* Read mode page field 48 value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x48);
}

/**
 * @brief Read context field 28 value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_context_field_28_value(offset)
int offset;

{
    /* Read context field 28 value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x28);
}

/**
 * @brief Read descriptor field 24 value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_descriptor_field_24_value(offset)
int offset;

{
    /* Read descriptor field 24 value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x24);
}

/**
 * @brief Read descriptor field 20 value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_descriptor_field_20_value(offset)
int offset;

{
    /* Read descriptor field 20 value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x20);
}

/**
 * @brief Get structure field hex 4c value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_structure_field_hex_4c_value(offset)
int offset;

{
    /* Get structure field hex 4c value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x4c);
}

/**
 * @brief Get structure field hex 50 value.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_structure_field_hex_50_value(offset)
int offset;

{
    /* Get structure field hex 50 value using only caller-provided data. */
    return *(uint32_t *)(offset + 0x50);
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_forward_ti_memset(offset) int offset;

{
    /* Forward ti memset using only caller-provided data. */
    ti_memset(offset + 0x10, 0, 0x34);
    ti_memset(offset + 0x44, 0, 0x38);
    return;
}

/**
 * @brief Get default record type code.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_default_record_type_code()

{
    /* Get default record type code using only caller-provided data. */
    return 0x15;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return No value.
 */
void core_set_notification_state_to_zero()

{
    /* Set notification state to zero; persistent state is carried in core notification state. */
    core_notification_state = 0;
    return;
}

/**
 * @brief Read buffer word.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_buffer_word(buffer)
uint32_t *buffer;

{
    /* Read buffer word using only caller-provided data. */
    return *buffer;
}

/**
 * @brief Get primary success flag.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_primary_success_flag()

{
    /* Get primary success flag using only caller-provided data. */
    return 1;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return Result produced by the procedure.
 */
bool core_forward_mww_transfer_value()

{
    /* Forward MWW transfer value; persistent state is carried in core MWW transfer value, core thermal monitor enabled. */
    core_mww_transfer_value = 1;
    return core_thermal_monitor_enabled == '\0';
}

/**
 * @brief Get secondary success flag.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_secondary_success_flag()

{
    /* Get secondary success flag using only caller-provided data. */
    return 1;
}

/**
 * @brief Get constant one value for ATA standby context.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_constant_one_value_for_ata_standby_context()

{
    /* Get constant one value for ATA standby context using only caller-provided data. */
    return 1;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return No value.
 */
void core_forward_cartridge_input_state()

{
    /* Forward cartridge input state; persistent state is carried in core cartridge input state, core sense selection flags and related record fields. */
    int operation_status;

    operation_status = core_get_notification_context_value();
    if ((operation_status == 2) && (operation_status = core_is_cartridge_input_state_six(), operation_status != 0)) {
        core_update_reconnect_state();
    }
    core_cartridge_input_state = 0;
    core_set_command_runtime_state_to_zero();
    core_sense_selection_flags = 0;
    core_update_enabled(0x800f2b4, 0);
    core_update_store_context_enable(0);
    core_update_clear_memory_response_end_context();
    core_process_cartridge_input_state();
    core_process_command_descriptor_context();
    core_process_primary_command_context();
    if (core_ata_standby_context != '\0') {
        core_process_runtime_context_base_for_cartridge_input_state();
        core_ata_standby_context = '\0';
    }
    core_update_clear_memory_response_end_context();
    core_state_108 = 0;
    core_state_103 = 0;
    core_mode_processing_context = 0;
    core_set_mechanism_phase_to_zero();
    return;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_forward_mcp3008_sample_value()

{
    /* Forward mcp3008 sample value; persistent state is carried in core mcp3008 sample. */
    return core_mcp3008_sample;
}

/**
 * @brief Get condition response code.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_condition_response_code()

{
    /* Get condition response code using only caller-provided data. */
    return 0x20;
}

/**
 * @brief Get extended response code.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_extended_response_code()

{
    /* Get extended response code using only caller-provided data. */
    return 0xac;
}

/**
 * @brief Store context enable byte.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_store_context_enable_byte(context, value) int context;
uint8_t value;

{
    /* Store context enable byte using only caller-provided data. */
    *(uint8_t *)(context + 7) = value;
    return;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return Result produced by the procedure.
 */
bool core_is_command_phase_three()

{
    /* Is command phase three; persistent state is carried in core command phase. */
    return core_command_phase == 3;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return No value.
 */
uint32_t core_forward_sci_read_logical()

{
    /* Forward SCI read logical using only caller-provided data. */
    sci_read_logical_input(3);
    return 0;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_forward_interrupt_transfer_context(condition) uint32_t condition;

{
    /* Forward interrupt transfer context; persistent state is carried in core interrupt transfer context, core state 218. */
    uint32_t *core_interrupt_transfer_context_pointer;
    uint32_t condition_field;
    int operation_status;

    operation_status = 0x13;
    core_interrupt_transfer_context_pointer = (uint32_t *)&core_interrupt_transfer_context;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_interrupt_transfer_context_pointer = core_interrupt_transfer_context_pointer + 1;
        /* Gate the following state transition on *core_interrupt_transfer_context_pointer != 0; the handler is skipped when this condition is false. */
        if (*core_interrupt_transfer_context_pointer != 0) {
            if (condition < *core_interrupt_transfer_context_pointer) {
                condition_field = *core_interrupt_transfer_context_pointer - condition;
            } else {
                condition_field = 0;
            }
            *core_interrupt_transfer_context_pointer = condition_field;
        }
        operation_status = operation_status + -1;
    } while (operation_status != 0);
    core_state_218 = condition + core_state_218;
    return;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @param destination Procedure input.
 * @param source Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_forward_end_remaining(destination, source, length) uint32_t *destination;
uint32_t *source;
int length;

{
    /* Forward end remaining using only caller-provided data. */
    uint32_t *buffer_2_cursor;
    uint32_t *buffer_1_cursor;
    uint32_t *source_end;
    int remaining_count;

    source_end = (uint32_t *)((int)source + length);
    if (((((uint32_t)source | (uint32_t)destination) & 3) == 0) && ((uint32_t *)((uint32_t)source_end & 0xfffffffc) != source)) {
        remaining_count = (int)((uint32_t)source_end & 0xfffffffc) - (int)source >> 2;
        buffer_2_cursor = source;
        buffer_1_cursor = destination;
        /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            source = buffer_2_cursor + 1;
            remaining_count = remaining_count + -1;
            destination = buffer_1_cursor + 1;
            *buffer_1_cursor = *buffer_2_cursor;
            buffer_2_cursor = source;
            buffer_1_cursor = destination;
        } while (remaining_count != 0);
    }
    if (source_end != source) {
        remaining_count = (int)source_end - (int)source;
        /* Repeat while remaining_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            remaining_count = remaining_count + -1;
            *(uint8_t *)destination = *(uint8_t *)source;
            source = (uint32_t *)((int)source + 1);
            destination = (uint32_t *)((int)destination + 1);
        } while (remaining_count != 0);
    }
    return;
}

/**
 * @brief Get constant zero value for MWW handle transfer.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_constant_zero_value_for_mww_handle_transfer()

{
    /* Get constant zero value for MWW handle transfer using only caller-provided data. */
    return 0;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_supported_forward_opcode(context)
uint32_t context;

{
    /* Is supported forward opcode using only caller-provided data. */
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
 * @brief Get default mode page flags.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_default_mode_page_flags()

{
    /* Get default mode page flags using only caller-provided data. */
    return 0;
}

/**
 * @brief Forward execution to the corresponding firmware core implementation routine.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_forward_cartridge_input_value()

{
    /* Forward cartridge input value; persistent state is carried in core cartridge input value. */
    return core_cartridge_input_value;
}

/**
 * @brief Get constant one value for request request for.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_constant_one_value_for_request_request_for()

{
    /* Get constant one value for request request for using only caller-provided data. */
    return 1;
}

