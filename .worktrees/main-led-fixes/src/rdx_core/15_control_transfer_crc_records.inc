/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Finalize control transfer state.
 *
 * @return No value.
 */
void core_finalize_control_transfer_state()

{
    /* Finalize control transfer state; persistent state is carried in core firmware transfer context. */
    int core_transfer_firmware_context_remaining_firmware_remaining_context_result;

    core_firmware_transfer_context = 3;
    core_transfer_firmware_context_remaining_firmware_remaining_context_result = core_transfer_firmware_context_remaining_firmware_remaining_context();
    if (core_transfer_firmware_context_remaining_firmware_remaining_context_result != 0) {
        core_firmware_transfer_context = 5;
    }
    core_process_firmware_transfer_resource();
    return;
}

/**
 * @brief Get end remaining.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param condition Procedure input.
 * @param destination Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_end_remaining(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t condition, uint32_t destination)

{
    /* Get end remaining using only caller-provided data. */
    uint32_t core_get_transfer_transfer_result;
    uint8_t working_buffer[128];

    if (condition < 0x21) {
        core_forward_end_remaining(working_buffer, flags, condition << 2);
        core_get_transfer_transfer_result = core_get_transfer_transfer(context, value, length, working_buffer, condition, destination);
    } else {
        core_get_transfer_transfer_result = 0;
    }
    return core_get_transfer_transfer_result;
}

/**
 * @brief Update constant 0xc.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint16_t core_update_constant_0xc(offset, buffer)
int offset;
uint8_t *buffer;

{
    /* Update constant 0xc using only caller-provided data. */
    uint8_t offset_field_0xc;
    uint16_t operation_result;

    if (buffer != (uint8_t *)0x0) {
        *buffer = 1;
    }
    if (0x4c < *(uint16_t *)(offset + 8)) {
        offset_field_0xc = *(uint8_t *)(*(int *)(offset + 0xc) + 0x4c);
        operation_result = (uint16_t)offset_field_0xc;
        if ((((uint16_t)(operation_result + 0x4c) <= *(uint16_t *)(offset + 8)) && (operation_result < 0xc9)) &&
            ((offset_field_0xc & 3) == 0)) {
            return operation_result;
        }
        if (buffer != (uint8_t *)0x0) {
            *buffer = 0;
        }
    }
    return 0;
}

/**
 * @brief Handle ATA standby context.
 *
 * @return No value.
 */
void core_handle_ata_standby_context()

{
    /* Handle ATA standby context; persistent state is carried in core ATA standby context, core command state context. */
    core_process_command_descriptor_context();
    core_update_persistent_update_context();
    core_set_mechanism_phase_to_zero();
    if (core_ata_standby_context != '\0') {
        core_process_runtime_context_base_for_cartridge_input_state();
        core_ata_standby_context = '\0';
    }
    if (core_command_state_context != '\0') {
        core_get_constant_one_value_for_ata_standby_context();
        core_submit_ata_standby_immediate(1);
    }
    core_update_clear_memory_response_end_context();
    return;
}

/**
 * @brief Calculate crc32.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_calculate_crc32(context, condition, length)
int context;
int condition;
uint32_t length;

{
    /* Calculate crc32 using only caller-provided data. */
    uint8_t *context_derived;
    uint32_t length_component;
    int operation_status;

    if (condition != 0) {
        context_derived = (uint8_t *)(context + -1);
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            context_derived = context_derived + 1;
            operation_status = 8;
            length_component = ((uint32_t)*context_derived ^ length >> 0x18) << 0x18;
            /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
            do {
                length_component = length_component << 1;
                if ((length_component & 0x80000000) != 0) {
                    length_component = length_component ^ 0x4c11db7;
                }
                operation_status = operation_status + -1;
            } while (operation_status != 0);
            condition = condition + -1;
            length = length_component ^ length << 8;
        } while (condition != 0);
    }
    return length;
}

/**
 * @brief Process USB event context.
 *
 * @param output Procedure input.
 * @param value Procedure input.
 * @param source Procedure input.
 * @return No value.
 */
void core_process_usb_event_context(output, value, source) int output;
uint32_t value;
int source;

{
    /* Process USB event context; persistent state is carried in core USB event context. */
    core_transfer_x01(output, &core_usb_event_context, 0xc);
    *(uint8_t *)(output + 0xb) = 0x30;
    core_forward_end_remaining(output + 0xc, *(uint32_t *)(source + 0x10), 0x14);
    /* Call core transfer mixed word key sum through the defined unprototyped function-pointer signature. */
    ((void (*)())core_transfer_mixed_word_key_sum)(output, 8, value);
    return;
}

/**
 * @brief Get response buffer.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_response_buffer()

{
    /* Get response buffer; persistent state is carried in core response buffer. */
    int core_get_mww_active_flag_result;

    core_get_mww_active_flag_result = core_get_mww_active_flag(0);
    *(bool *)core_response_buffer = core_get_mww_active_flag_result != 0;
    return 0;
}

/**
 * @brief Handle thermal control context.
 *
 * @return No value.
 */
void core_handle_thermal_control_context()

{
    /* Handle thermal control context using only caller-provided data. */
    int core_get_thermal_control_context_result;

    core_get_thermal_control_context_result = core_get_thermal_control_context(0x800f2cc);
    if (core_get_thermal_control_context_result != 0) {
        core_ramp_thermal_pwm();
        rti_process_thermal_timer_state(0x800f2cc, 800);
    }
    core_transfer_spi_response_context();
    /* Call core advance mechanism state through the defined unprototyped function-pointer signature. */
    ((void (*)())core_advance_mechanism_state)(0);
    core_handle_cartridge_monitor_context_for_thermal_control_context();
    return;
}

/**
 * @brief Process request request for descriptor descriptor resource.
 *
 * @param output_status Procedure input.
 * @param request Procedure input.
 * @return No value.
 */
void core_process_request_request_for_descriptor_descriptor_resource(output_status, request) uint32_t *output_status;
uint8_t *request;

{
    /* Process request request for descriptor descriptor resource using only caller-provided data. */
    uint8_t request_code;
    int core_get_constant_one_value_for_request_request_for_result;

    request_code = request[6];
    /* Gate the following state transition on ((request[4] < 3) && (request_code < 4)) && ((*request >> 6 & 1) == 0); the handler is skipped when this condition is false. */
    if (((request[4] < 3) && (request_code < 4)) && ((*request >> 6 & 1) == 0)) {
        core_process_notify_change_notification_context(request[4] & 3, *(uint8_t *)(output_status[2] + 1) & 1);
        core_get_constant_one_value_for_request_request_for_result = core_get_constant_one_value_for_request_request_for(request_code);
        if (core_get_constant_one_value_for_request_request_for_result != 0) {
            return;
        }
    }
    *output_status = 10;
    return;
}

/**
 * @brief Update decoded request.
 *
 * @param request Procedure input.
 * @param output_value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_decoded_request(request, output_value, length, flags)
uint8_t *request;
uint32_t *output_value;
uint32_t length;
int flags;

{
    /* Update decoded request using only caller-provided data. */
    int core_get_pending_transfer_buffer_result;
    uint32_t working_result;
    uint32_t operation_result;
    int decoded_status;

    operation_result = 1;
    if ((output_value != (uint32_t *)0x0) &&
        /* Update decoded request; this branch selects whether its associated handler runs. */
        (decoded_status = flags, core_get_pending_transfer_buffer_result = core_get_pending_transfer_buffer(*request, &decoded_status), core_get_pending_transfer_buffer_result != 0)) {
        if (decoded_status == 0) {
            operation_result = 5;
        } else {
            if (decoded_status == 1) {
                working_result = 0x80;
            } else {
                if (decoded_status != 2) {
                    return 1;
                }
                working_result = 0;
            }
            operation_result = 0;
            *output_value = working_result;
        }
    }
    return operation_result;
}

/**
 * @brief Set validation flags flag bit three.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_set_validation_flags_flag_bit_three(condition) int condition;

{
    /* Set validation flags flag bit three; persistent state is carried in core validation flags. */
    if (condition == 0) {
        core_validation_flags = core_validation_flags & 0xfffffff7;
    } else {
        core_validation_flags = core_validation_flags | 8;
    }
    core_update_enabled(0x800f29c, core_validation_flags != 0);
    return;
}

/**
 * @brief Set validation flags flag bit five.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_set_validation_flags_flag_bit_five(condition) int condition;

{
    /* Set validation flags flag bit five; persistent state is carried in core validation flags. */
    if (condition == 0) {
        core_validation_flags = core_validation_flags & 0xffffffdf;
    } else {
        core_validation_flags = core_validation_flags | 0x20;
    }
    core_update_enabled(0x800f29c, core_validation_flags != 0);
    return;
}

/**
 * @brief Transfer interrupt transfer context.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_transfer_interrupt_transfer_context(condition) uint32_t condition;

{
    /* Transfer interrupt transfer context; persistent state is carried in core interrupt transfer context, core state 218. */
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
 * @brief Transfer polls remaining channel USB poll context.
 *
 * @param channel_flags Procedure input.
 * @param polls_remaining Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_polls_remaining_channel_usb_poll_context(channel_flags, polls_remaining)
uint32_t channel_flags;
int polls_remaining;

{
    /* Transfer polls remaining channel USB poll context; persistent state is carried in USB endpoint command register. */
    if (0 < polls_remaining) {
        /* Repeat while polls_remaining != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((*(uint32_t *)(&usb_endpoint_command_register + ((channel_flags & 0x7fffff7f) << 1 | channel_flags >> 7) * 0x10) >> 10 & 1) == 0) {
                return 0;
            }
            rti_delay_microseconds(1);
            polls_remaining = polls_remaining + -1;
        } while (polls_remaining != 0);
    }
    return 2;
}

/**
 * @brief Transfer USB endpoint transfer for response buffer.
 *
 * @return No value.
 */
void core_transfer_usb_endpoint_transfer_for_response_buffer()

{
    /* Transfer USB endpoint transfer for response buffer; persistent state is carried in core USB endpoint transfer, core descriptor queue context and related record fields. */
    int operation_status;
    uint32_t *core_usb_endpoint_transfer_pointer;

    operation_status = 2;
    core_usb_endpoint_transfer_pointer = (uint32_t *)&core_usb_endpoint_transfer;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        *core_usb_endpoint_transfer_pointer = core_usb_endpoint_transfer_pointer[6];
        operation_status = operation_status + -1;
        core_usb_endpoint_transfer_pointer[4] = core_usb_endpoint_transfer_pointer[8];
        core_usb_endpoint_transfer_pointer = core_usb_endpoint_transfer_pointer + 1;
    } while (operation_status != 0);
    core_descriptor_queue_context = 0;
    usb_hal_configure_endpts();
    /* Notify core primary firmware callback only after the associated state fields have been committed. */
    (*core_primary_firmware_callback)();
    return;
}

/**
 * @brief Transfer USB endpoint transfer.
 *
 * @return No value.
 */
void core_transfer_usb_endpoint_transfer()

{
    /* Transfer USB endpoint transfer; persistent state is carried in core USB endpoint transfer, core descriptor queue context and related record fields. */
    int operation_status;
    uint32_t *core_usb_endpoint_transfer_pointer;

    operation_status = 2;
    core_usb_endpoint_transfer_pointer = (uint32_t *)&core_usb_endpoint_transfer;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        *core_usb_endpoint_transfer_pointer = core_usb_endpoint_transfer_pointer[10];
        operation_status = operation_status + -1;
        core_usb_endpoint_transfer_pointer[4] = core_usb_endpoint_transfer_pointer[0xc];
        core_usb_endpoint_transfer_pointer = core_usb_endpoint_transfer_pointer + 1;
    } while (operation_status != 0);
    core_descriptor_queue_context = 1;
    usb_hal_configure_endpts();
    /* Notify core state 230 only after the associated state fields have been committed. */
    (*core_state_230)();
    return;
}

/**
 * @brief Initialize transfer workspace.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_initialize_transfer_workspace(buffer) int *buffer;

{
    /* Initialize transfer workspace using only caller-provided data. */
    uint32_t status_bits;
    uint8_t working_buffer[20];
    volatile uint32_t working_result = 0;

    status_bits = 4;
    working_result = 4;
    /* Initialize transfer workspace; this branch selects whether its associated handler runs. */
    ti_memset(*buffer + 4, 0, 0x1fc);
    /* Call core configure descriptor payload through the defined unprototyped function-pointer signature. */
    ((void (*)())core_configure_descriptor_payload)(working_buffer, &status_bits, buffer);
    core_process_ti_memset_for_initialize_transfer_workspace(working_buffer, buffer);
    return;
}

/**
 * @brief Get zero word buffer.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_zero_word_buffer(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, int index)

{
    /* Get zero word buffer using only caller-provided data. */
    uint8_t working_buffer[256];

    ((uint32_t (*)())core_transfer_word_copy)(working_buffer, value, length, index);
    ((uint32_t (*)())core_get_descriptor)(context, working_buffer, index << 1, flags, index);
    core_zero_word_buffer(working_buffer, index << 1);
    return 0;
}

/**
 * @brief Process secondary.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_process_secondary(buffer) int *buffer;

{
    /* Process secondary using only caller-provided data. */
    int operation_status;
    uint8_t working_buffer[8];
    uint16_t working_result = 0;
    uint8_t secondary_value = 0;

    operation_status = ((bool (*)())core_handle_descriptor_request_bytes)(working_buffer, buffer);
    if (operation_status != 0) {
        operation_status = ((uint32_t (*)())core_transition_reserved_command_byte_reserved_command_word_descriptor_queue_context)(working_buffer);
        if (operation_status == 0) {
            core_handle_primary_sense_context(working_result, secondary_value);
            *(uint8_t *)(buffer + 7) = 1;
        } else {
            *buffer = operation_status;
        }
    }
    return;
}

/**
 * @brief Update response flag.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_response_flag(buffer)
uint32_t *buffer;

{
    /* Update response flag using only caller-provided data. */
    uint32_t operation_result;
    uint8_t working_result;
    uint8_t response_flag;
    int *state_record;

    state_record = (int *)buffer[5];
    operation_result = 0;
    /* Gate the following state transition on (state_record != (int *)0x0) && (*state_record != 0); the handler is skipped when this condition is false. */
    if ((state_record != (int *)0x0) && (*state_record != 0)) {
        response_flag = 0;
        *buffer = 0;
        operation_result = 1;
        /* Gate the following state transition on *state_record == 4; the handler is skipped when this condition is false. */
        if (*state_record == 4) {
            working_result = 0x14;
            *state_record = 5;
            response_flag = 2;
        } else {
            /* Gate the following state transition on *state_record != 8; the handler is skipped when this condition is false. */
            if (*state_record != 8) {
                return 1;
            }
            *(uint8_t *)(buffer + 1) = 0;
            working_result = 0;
        }
        *(uint8_t *)(buffer + 2) = working_result;
        *(uint8_t *)((int)buffer + 6) = response_flag;
    }
    return operation_result;
}

/**
 * @brief Initialize output record from source.
 *
 * @param source_record Procedure input.
 * @param output_record Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_initialize_output_record_from_source(source_record, output_record)
int source_record;
int output_record;

{
    /* Initialize output record from source using only caller-provided data. */
    int core_copy_source_record_header_result;

    core_copy_source_record_header_result = core_copy_source_record_header(*(uint32_t *)(source_record + 0xc));
    if (core_copy_source_record_header_result != 0) {
        *(uint32_t *)(output_record + 0x20) = 0;
        ti_memset(output_record + 0xc, 0, 0x14);
        *(int *)(output_record + 0x20) = source_record;
        *(uint8_t *)(output_record + 0xc) = 0x1f;
        return 1;
    }
    return 0;
}

/**
 * @brief Transfer ti memset component.
 *
 * @param destination Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param source_state Procedure input.
 * @return No value.
 */
void core_transfer_ti_memset_component(destination, value, length, source_state) uint8_t *destination;
int value;
uint32_t length;
int *source_state;

{
    /* Transfer ti memset component using only caller-provided data. */
    uint8_t *length_component;

    ti_memset(destination, 0, value);
    /* Iterate while ; (length < (uint32_t)source_state[1] && (value != 0)); value = value + -1, preserving the defined cursor and bound. */
    for (; (length < (uint32_t)source_state[1] && (value != 0)); value = value + -1) {
        length_component = (uint8_t *)(length + *source_state);
        length = length + 1;
        *destination = *length_component;
        destination = destination + 1;
    }
    return;
}

/**
 * @brief Update descriptor subtraction.
 *
 * @param offset Procedure input.
 * @param output_difference Procedure input.
 * @param descriptor Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_descriptor_subtraction(offset, output_difference, descriptor)
int offset;
int *output_difference;
char *descriptor;

{
    /* Update descriptor subtraction using only caller-provided data. */
    uint32_t operation_result;
    uint32_t subtraction_value;
    uint32_t base_value;
    int offset_field_0x68;

    /* Gate the following state transition on *descriptor == '\x1f'; the handler is skipped when this condition is false. */
    if (*descriptor == '\x1f') {
        return 2;
    }
    if (*(char *)(offset + 0x6f) == '\0') {
        operation_result = 0x13;
    } else {
        operation_result = 0;
    }
    subtraction_value = *(uint32_t *)(descriptor + 4);
    base_value = *(uint32_t *)(offset + 100);
    offset_field_0x68 = *(int *)(offset + 0x68);
    *output_difference = base_value - subtraction_value;
    output_difference[1] = offset_field_0x68 - (uint32_t)(base_value < subtraction_value);
    return operation_result;
}

/**
 * @brief Transfer bytes remaining descriptor.
 *
 * @param descriptor Procedure input.
 * @param source_byte Procedure input.
 * @param bytes_remaining Procedure input.
 * @param output_cursor Procedure input.
 * @return No value.
 */
void core_transfer_bytes_remaining_descriptor(descriptor, source_byte, bytes_remaining, output_cursor) int *descriptor;
int source_byte;
int bytes_remaining;
uint32_t *output_cursor;

{
    /* Transfer bytes remaining descriptor using only caller-provided data. */
    uint32_t buffer_2_cursor;
    int remaining_count;
    uint8_t condition_1_field;

    if (bytes_remaining != 0) {
        buffer_2_cursor = *output_cursor;
        remaining_count = 0;
        /* Repeat while bytes_remaining != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((uint32_t)descriptor[1] <= buffer_2_cursor) {
                return;
            }
            if (source_byte == 0) {
                condition_1_field = 0;
            } else {
                condition_1_field = *(uint8_t *)(source_byte + remaining_count);
            }
            *(uint8_t *)(buffer_2_cursor + *descriptor) = condition_1_field;
            remaining_count = remaining_count + 1;
            bytes_remaining = bytes_remaining + -1;
            buffer_2_cursor = *output_cursor + 1;
            *output_cursor = buffer_2_cursor;
        } while (bytes_remaining != 0);
    }
    return;
}

/**
 * @brief Update structure field hex.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
bool core_update_structure_field_hex(buffer, value, length) uint32_t *buffer;
uint32_t value;
uint32_t length;

{
    /* Update structure field hex using only caller-provided data. */
    uint32_t value_derived;
    uint32_t operation_status;
    uint32_t working_result;
    bool buffer_cursor;
    uint64_t core_get_structure_field_hex_for_response_end_word_result;

    core_get_structure_field_hex_for_response_end_word_result = core_get_structure_field_hex_for_response_end_word(length);
    value_derived = (uint32_t)core_get_structure_field_hex_for_response_end_word_result - value;
    working_result = value_derived + 1;
    value_derived = ((int)((uint64_t)core_get_structure_field_hex_for_response_end_word_result >> 0x20) - (uint32_t)((uint32_t)core_get_structure_field_hex_for_response_end_word_result < value)) +
                       (uint32_t)(0xfffffffe < value_derived);
    operation_status = buffer[1];
    buffer_cursor = value_derived <= operation_status;
    if (operation_status == value_derived) {
        buffer_cursor = working_result <= *buffer;
    }
    return buffer_cursor && (operation_status != value_derived || *buffer != working_result);
}

/**
 * @brief Enable request flag eight.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_enable_request_flag_eight(context) uint32_t context;

{
    /* Enable request flag eight; persistent state is carried in core sense selection flags. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;

    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_update_saturating_add_u32_for_enable_request_flag(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, context);
    core_sense_selection_flags = core_sense_selection_flags | 8;
    core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
    return;
}

/**
 * @brief Enable request flag sixteen.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_enable_request_flag_sixteen(context) uint32_t context;

{
    /* Enable request flag sixteen; persistent state is carried in core sense selection flags. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;

    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_update_saturating_add_u32_for_enable_request_flag(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, context);
    core_sense_selection_flags = core_sense_selection_flags | 0x10;
    core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
    return;
}

/**
 * @brief Process flash validation status.
 *
 * @return No value.
 */
void core_process_flash_validation_status()

{
    /* Process flash validation status; persistent state is carried in core flash validation status, core mechanism phase. */
    int operation_status;

    operation_status = core_get_flash_validation_context_for_flash_validation_status();
    if (operation_status == 0) {
        core_flash_validation_status = 8;
    } else {
        core_update_flash_validation_status(10);
        core_flash_validation_status = 0;
        core_mechanism_phase = 3;
        core_update_flag_one_interrupt_transfer_context();
    }
    return;
}

/**
 * @brief Calculate record checksum.
 *
 * @param source Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_calculate_record_checksum(source, length)
int source;
uint32_t length;

{
    /* Calculate record checksum using only caller-provided data. */
    uint32_t core_transfer_component_for_flash_validation_context_result;

    core_transfer_component_for_flash_validation_context_result = 0xbadf00d;
    if (4 < length) {
        core_forward_end_remaining(&core_transfer_component_for_flash_validation_context_result, 0x800e744, 4);
        core_transfer_component_for_flash_validation_context_result = core_transfer_component_for_flash_validation_context(source + 4, length - 4, core_transfer_component_for_flash_validation_context_result);
    }
    return core_transfer_component_for_flash_validation_context_result;
}

/**
 * @brief Update SPI set logical for x01 for resource.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_update_spi_set_logical_for_x01_for_resource(buffer) int *buffer;

{
    /* Update SPI set logical for x01 for resource using only caller-provided data. */
    uint32_t working_result;

    if ((char)buffer[1] == '\0') {
        working_result = 0;
    } else {
        /* Gate the following state transition on *buffer != 0; the handler is skipped when this condition is false. */
        if (*buffer != 0) {
            spi_set_logical_output(buffer[3], 0);
            working_result = 1;
            goto core_update_spi_set_logical_for_x01_for_resource_before_set_logical_output;
        }
        working_result = 1;
    }
    spi_set_logical_output(buffer[3], working_result);
    working_result = 0;
/* Shared target for core update SPI set logical for x01 for resource before set logical output; incoming paths preserve the same state assumptions. */
core_update_spi_set_logical_for_x01_for_resource_before_set_logical_output:
    spi_set_logical_output(buffer[4], working_result);
    return;
}

/**
 * @brief Build record type twelve response.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_record_type_twelve_response(context)
uint32_t context;

{
    /* Build record type twelve response using only caller-provided data. */
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;

    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 0x10);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 1, 0x12);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 2, 3, 0xc);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 7, 7, 4);
    return 0;
}

/**
 * @brief Build record type eleven response.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_record_type_eleven_response(context)
uint32_t context;

{
    /* Build record type eleven response using only caller-provided data. */
    uint32_t core_process_ti_memset_for_build_scsi_mode_result;

    core_process_ti_memset_for_build_scsi_mode_result = (uint32_t)core_process_ti_memset_for_build_scsi_mode(context, 5);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 1, 0x11);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 2, 3, 1);
    core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 4, 4, 1);
    return 0;
}

