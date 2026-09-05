/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Issue AHCI command slot.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_issue_ahci_command_slot(context, value, condition)
int context;
uint8_t value;
int condition;

{
    /* Issue AHCI command slot; persistent state is carried in AHCI port command register, core ahci active command tag and related record fields. */
    int context_derived;
    int value_derived;

    context_derived = context * 0x80;
    if ((*(uint32_t *)(&ahci_port_command_register + context_derived) & 1) != 0) {
        if (condition == 0) {
            value_derived = 1 << (uint32_t)value;
            (&core_ahci_active_command_tag)[context * 0x1d0] = value;
        } else {
            value_derived = 1 << (uint32_t)value;
            *(int *)(&ahci_port_sata_active_register + context_derived) = value_derived;
        }
        *(int *)(&ahci_port_command_issue_register + context_derived) = value_derived;
        return 0;
    }
    return 1;
}

/**
 * @brief Initialize smart monitoring.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_initialize_smart_monitoring()

{
    /* Initialize smart monitoring; persistent state is carried in core smart monitoring active, core smart temperature sample timer and related record fields. */
    uint8_t core_smart_monitoring_active_snapshot;
    int core_update_capacity_block_context_result;
    uint8_t working_buffer[4];
    volatile uint16_t working_result = 0;

    rti_set_software_timer(&core_smart_temperature_sample_timer, 0);
    core_smart_temperature_celsius = 0xff;
    core_thermal_hot_sample_retry_count = 0;
    core_update_capacity_block_context_result = core_update_capacity_block_context(working_buffer);
    if (core_update_capacity_block_context_result == 0) {
        core_smart_monitoring_active_snapshot = 0;
    } else {
        working_result = 0;
        core_smart_monitoring_active = ((uint32_t (*)())core_build_ata_smart_command)(0xd8, working_buffer, 0);
        core_clear_request_and_release_device_command(working_buffer);
        core_smart_monitoring_active_snapshot = core_smart_monitoring_active;
    }
    return core_smart_monitoring_active_snapshot;
}

/**
 * @brief Shift multiword left.
 *
 * @param destination_words Procedure input.
 * @param source_words Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_shift_multiword_left(destination_words, source_words, condition, flags)
uint32_t *destination_words;
uint32_t *source_words;
uint32_t condition;
int flags;

{
    /* Shift multiword left using only caller-provided data. */
    uint32_t condition_field;
    uint32_t buffer_2_cursor;
    uint32_t status_bits;
    int shifts_remaining;

    if (condition < 0x20) {
        status_bits = 0x80000000;
        if (1 < condition) {
            shifts_remaining = condition - 1;
            /* Repeat while shifts_remaining != 0; the body advances or polls the state needed to leave the loop. */
            do {
                shifts_remaining = shifts_remaining + -1;
                status_bits = status_bits | status_bits >> 1;
            } while (shifts_remaining != 0);
        }
        if (condition == 0) {
            status_bits = 0;
        }
        condition_field = 0;
        /* Iterate while ; flags != 0; flags = flags + -1, preserving the defined cursor and bound. */
        for (; flags != 0; flags = flags + -1) {
            buffer_2_cursor = *source_words;
            *destination_words = condition_field | buffer_2_cursor << (condition & 0xff);
            condition_field = (status_bits & buffer_2_cursor) >> (0x20 - condition & 0xff);
            source_words = source_words + 1;
            destination_words = destination_words + 1;
        }
    } else {
        condition_field = 0;
    }
    return condition_field;
}

/**
 * @brief Sha256 initialize.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_sha256_initialize(offset) int offset;

{
    /* Sha256 initialize using only caller-provided data. */
    *(uint32_t *)(offset + 0x40) = 0;
    *(uint32_t *)(offset + 0x44) = 0;
    *(uint32_t *)(offset + 0x48) = 0;
    *(uint32_t *)(offset + 0x4c) = 0x6a09e667;
    *(uint32_t *)(offset + 0x50) = 0xbb67ae85;
    *(uint32_t *)(offset + 0x54) = 0x3c6ef372;
    *(uint32_t *)(offset + 0x58) = 0xa54ff53a;
    *(uint32_t *)(offset + 0x5c) = 0x510e527f;
    *(uint32_t *)(offset + 0x60) = 0x9b05688c;
    *(uint32_t *)(offset + 100) = 0x1f83d9ab;
    *(uint32_t *)(offset + 0x68) = 0x5be0cd19;
    return;
}

/**
 * @brief Dispatch transfer state update.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_dispatch_transfer_state_update(context, value, length, flags, index)
uint32_t context;
uint32_t value;
uint32_t length;
uint32_t flags;
uint8_t index;

{
    /* Dispatch transfer state update using only caller-provided data. */
    int status_bits;
    uint32_t core_transition_use_alternate_lookup_type_copy_snapshot;
    uint8_t working_buffer[24];

    status_bits = ((int (*)())core_process_transfer_requested)();
    if (status_bits == 0) {
        core_transition_use_alternate_lookup_type_copy_snapshot = ((uint32_t (*)())core_transition_use_alternate_lookup_type_copy)(flags, working_buffer, value, length, context, index);
    } else if ((status_bits == 0x14) || (status_bits == 6 || status_bits == 0x11)) {
        core_transition_use_alternate_lookup_type_copy_snapshot = 1;
    } else {
        core_transition_use_alternate_lookup_type_copy_snapshot = 2;
    }
    return core_transition_use_alternate_lookup_type_copy_snapshot;
}

/**
 * @brief Process component for device ti memset.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param buffer Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_component_for_device_ti_memset(offset, value, buffer, flags)
int offset;
uint32_t value;
uint32_t *buffer;
uint32_t flags;

{
    /* Process component for device ti memset using only caller-provided data. */
    int operation_result;
    int core_write_four_byte_value_result;
    uint32_t working_result;
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    operation_result = core_update_device_for_component_for_device();
    if (operation_result == 0) {
        if (*(char *)(offset + 0x6d) == '\0') {
            working_result = 0x12345678;
        } else {
            working_result = 0x87654321;
        }
        /* Process component for device ti memset; this branch selects whether its associated handler runs. */
        operation_result = core_accumulate_response_byte_lanes(*buffer, 0x1fc, working_result);
        flags_snapshot = 0x1fc;
        core_write_four_byte_value_result = core_write_four_byte_value(&flags_snapshot, buffer);
        if (operation_result == core_write_four_byte_value_result) {
            *(uint8_t *)(offset + 0x6e) = 1;
            return 0;
        }
        operation_result = 5;
    }
    return operation_result;
}

/**
 * @brief Send SPI flash command address.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
uint32_t core_send_spi_flash_command_address(condition, value, length, flags)
int condition;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Send SPI flash command address using only caller-provided data. */
    int operation_status;
    uint32_t condition_pointer;
    uint32_t status_bits;
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    operation_status = spi_transfer_word(value | 0x14060000, &flags_snapshot);
    if (operation_status == 0) {
        condition_pointer = 0;
        status_bits = 0x14000000;
        operation_status = 0;
        /* Repeat while condition_pointer < 3; the body advances or polls the state needed to leave the loop. */
        do {
            if (operation_status == 0) {
                if (condition_pointer == 2 && condition == 0) {
                    status_bits = status_bits & 0xefffffff;
                }
                operation_status =
                    core_update_spi_configure_spi(length >> (0x10U - condition_pointer * 8U & 0xff) & 0xff | status_bits, &flags_snapshot);
            }
            condition_pointer = condition_pointer + 1;
        } while (condition_pointer < 3);
    }
    return 0;
}

/**
 * @brief Execute ATA read dma.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_execute_ata_read_dma(context, offset, length, flags) uint32_t context;
int offset;
uint32_t length;
uint32_t flags;

{
    /* Execute ATA read dma using only caller-provided data. */
    uint8_t working_result;
    int operation_status;
    int command_buffer;
    volatile uint32_t length_component = 0;
    uint32_t status_bits;

    command_buffer = offset;
    length_component = length;
    status_bits = flags;
    operation_status = (int)core_get_cartridge_ata_device_info();
    /* Call core encode ata taskfile lba through the defined unprototyped function-pointer signature. */
    ((void (*)())core_encode_ata_taskfile_lba)(&command_buffer, context, 1, *(uint8_t *)(operation_status + 0x12));
    operation_status = (int)core_get_cartridge_ata_device_info();
    if (*(char *)(operation_status + 0x12) == '\0') {
        working_result = 200;
    } else {
        working_result = 0x25;
    }
    status_bits = (status_bits & 0xff000000u) | (CONCAT12(working_result, (uint16_t)status_bits) & 0x00ffffffu);
    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    operation_status = ((uint32_t (*)())core_transition_range_word_range_word)(context, 1, 0, operation_status + 0x2c);
    if (operation_status != 0) {
        ahci_execute_taskfile_command(&command_buffer, offset + 8, 1, 0);
    }
    return;
}

/**
 * @brief Execute ATA write dma.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_execute_ata_write_dma(context, offset, length, flags) uint32_t context;
int offset;
uint32_t length;
uint32_t flags;

{
    /* Execute ATA write dma using only caller-provided data. */
    uint8_t working_result;
    int operation_status;
    int command_buffer;
    volatile uint32_t length_component = 0;
    uint32_t status_bits;

    command_buffer = offset;
    length_component = length;
    status_bits = flags;
    operation_status = (int)core_get_cartridge_ata_device_info();
    /* Call core encode ata taskfile lba through the defined unprototyped function-pointer signature. */
    ((void (*)())core_encode_ata_taskfile_lba)(&command_buffer, context, 1, *(uint8_t *)(operation_status + 0x12));
    operation_status = (int)core_get_cartridge_ata_device_info();
    if (*(char *)(operation_status + 0x12) == '\0') {
        working_result = 0xca;
    } else {
        working_result = 0x35;
    }
    status_bits = (status_bits & 0xff000000u) | (CONCAT12(working_result, (uint16_t)status_bits) & 0x00ffffffu);
    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    operation_status = ((uint32_t (*)())core_transition_range_word_range_word)(context, 1, 1, operation_status + 0x2c);
    if (operation_status != 0) {
        ahci_execute_taskfile_command(&command_buffer, offset + 8, 0, 0);
    }
    return;
}

/**
 * @brief Configure board logical pins.
 *
 * @return No value.
 */
void core_configure_board_logical_pins()

{
    /* Configure board logical pins; persistent state is carried in core variant 38 spi logical pin configuration, core default spi logical pin configuration. */
    int16_t operation_status;
    uint8_t *data_cursor;
    uint32_t *offset_cursor;
    uint32_t register_offsets[8];

    offset_cursor = register_offsets;
    register_offsets[1] = 0;
    register_offsets[2] = 4;
    register_offsets[3] = 0xc;
    register_offsets[4] = 0xf;
    register_offsets[5] = 0xb;
    register_offsets[6] = 0xe;
    register_offsets[7] = 0x10;
    operation_status = core_get_identity_variant_code();
    if (operation_status == 0x38) {
        data_cursor = (uint8_t *)&core_variant_38_spi_logical_pin_configuration;
    } else {
        data_cursor = (uint8_t *)&core_default_spi_logical_pin_configuration;
    }
    register_offsets[0] = 0;
    /* Repeat while register_offsets[0] < 7; the body advances or polls the state needed to leave the loop. */
    do {
        offset_cursor = offset_cursor + 1;
        /* Configure board logical pins; this branch selects whether its associated handler runs. */
        core_process_first_pin_initial_pin(*offset_cursor, data_cursor);
        data_cursor = data_cursor + 8;
        register_offsets[0] = register_offsets[0] + 1;
    } while (register_offsets[0] < 7);
    return;
}

/**
 * @brief Configure component for parsed header descriptor.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_configure_component_for_parsed_header_descriptor(uint32_t condition, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)

{
    /* Configure component for parsed header descriptor using only caller-provided data. */
    int operation_status;
    int working_result;
    uint32_t *data_cursor;
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    volatile uint32_t flags_snapshot = 0;

    data_cursor = &value_derived;
    value_derived = value;
    length_component = length;
    flags_snapshot = flags;
    operation_status = ((uint32_t (*)())core_handle_descriptor_device)(condition, 0, value, index, &value_derived);
    if (operation_status == 0) {
        operation_status = ((uint32_t (*)())core_process_device_use_lookup)(condition, &value_derived, flags, 0, (uint32_t)data_cursor & 0xffffff00);
        working_result = core_process_ata_command_for_lookup_key_metadata(condition);
        if (((operation_status == 0) && (working_result != 0)) && (working_result = core_handle_terminal_found_lookup_succeeded(condition, 0), working_result == 0)) {
            operation_status = 2;
        }
    }
    return CONCAT44(value_derived, (uint32_t)(operation_status == 0));
}

/**
 * @brief Transition device address.
 *
 * @param value_record Procedure input.
 * @param state_record Procedure input.
 * @return No value.
 */
void core_transition_device_address(value_record, state_record) uint32_t *value_record;
int *state_record;

{
    /* Transition device address using only caller-provided data. */
    int operation_status;
    uint32_t buffer_1_cursor;

    buffer_1_cursor = *value_record;
    /* Gate the following state transition on *state_record == 0; the handler is skipped when this condition is false. */
    if (*state_record == 0) {
        operation_status = core_process_device_address(value_record, state_record);
        if (operation_status != 0) {
            if (state_record[1] == 0) {
                buffer_1_cursor = buffer_1_cursor | 1;
            } else if (state_record[1] == 1) {
                buffer_1_cursor = buffer_1_cursor | 2;
            }
        }
    /* Transition device address; this branch selects whether its associated handler runs. */
    } else if ((*state_record == 1) && (operation_status = core_compare_record_payloads(value_record, state_record), operation_status != 0)) {
        if (state_record[1] == 0) {
            buffer_1_cursor = buffer_1_cursor | 4;
        } else if (state_record[1] == 1) {
            buffer_1_cursor = buffer_1_cursor | 8;
        }
    }
    *value_record = buffer_1_cursor;
    return;
}

/**
 * @brief Process expected flag expected.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
void core_process_expected_flag_expected(uint8_t *buffer, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)

{
    /* Process expected flag expected using only caller-provided data. */
    int operation_status;
    uint8_t working_buffer[24];

    /* Process expected flag expected; this branch selects whether its associated handler runs. */
    operation_status = core_run_expected_flag_expected_type(*buffer, 0, working_buffer, index);
    if (operation_status == 0) {
        ((int (*)())core_copy_and_pad_record_payload)(index, buffer, working_buffer, length, flags);
    } else if (operation_status == 6) {
        /* Process expected flag expected; this branch selects whether its associated handler runs. */
        core_build_indexed_transfer_descriptor(*buffer, value, length, flags, index);
    }
    return;
}

/**
 * @brief Update descriptor requested.
 *
 * @param offset Procedure input.
 * @param output_difference Procedure input.
 * @param descriptor Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_descriptor_requested(offset, output_difference, descriptor)
int offset;
int *output_difference;
char *descriptor;

{
    /* Update descriptor requested using only caller-provided data. */
    uint32_t operation_result;
    uint32_t requested_value;
    int difference_value;
    int carry4_result;

    /* Gate the following state transition on *descriptor == '\x1f'; the handler is skipped when this condition is false. */
    if (*descriptor == '\x1f') {
        operation_result = 2;
    } else {
        operation_result = 0;
        if (*(char *)(offset + 0x6d) == '\0') {
            difference_value = *(int *)(descriptor + 4);
            carry4_result = 0;
        } else {
            if (*(char *)(offset + 0x6f) == '\0') {
                operation_result = 0x13;
            }
            requested_value = *(uint32_t *)(descriptor + 4) + *(uint32_t *)(offset + 100);
            difference_value = requested_value - 0xfff;
            carry4_result = (*(int *)(offset + 0x68) + (uint32_t)CARRY4(*(uint32_t *)(descriptor + 4), *(uint32_t *)(offset + 100))) -
                             (uint32_t)(requested_value < 0xfff);
        }
        *output_difference = difference_value;
        output_difference[1] = carry4_result;
    }
    return operation_result;
}

/**
 * @brief Transfer found flag.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_transfer_found_flag(offset, buffer, condition, flags)
int offset;
int16_t *buffer;
uint32_t condition;
uint32_t flags;

{
    /* Transfer found flag using only caller-provided data. */
    uint16_t working_result;
    int operation_status;
    uint32_t found_flag;
    int16_t *buffer_cursor;
    uint32_t condition_field;
    volatile uint32_t flags_snapshot = 0;

    operation_status = 0x1e;
    working_result = 2;
    found_flag = 0;
    buffer_cursor = buffer;
    condition_field = condition;
    flags_snapshot = flags;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        core_copy_bytes(&buffer_cursor, offset + (uint32_t)working_result, 0xc);
        if ((condition & 0xff) == ((uint32_t)buffer_cursor & 0xff)) {
            found_flag = 1;
            *buffer = (uint16_t)((condition_field >> 8) & 0xffu) + (uint16_t)((condition_field >> 16) & 0xffu) * 0x100;
            break;
        }
        operation_status = operation_status + -1;
        working_result = working_result + 0xc;
    } while (operation_status != 0);
    return CONCAT44(buffer_cursor, found_flag);
}

/**
 * @brief Service led patterns.
 *
 * @return No value.
 */
void core_service_led_patterns()

{
    /* Service led patterns; persistent state is carried in core led controller count, core led controller table. */
    int core_is_software_timer_expired_result;
    uint32_t working_result;
    uint32_t entry_index;

    core_is_software_timer_expired_result = core_is_software_timer_expired(0x800f291);
    if (core_is_software_timer_expired_result != 0) {
        /* Iterate while entry_index = 0; entry_index < core_led_controller_count; entry_index = entry_index + 1 & 0xff, preserving the defined cursor and bound. */
        for (entry_index = 0; entry_index < core_led_controller_count; entry_index = entry_index + 1 & 0xff) {
            working_result = *(uint32_t *)(&core_led_controller_table + entry_index * 4);
            core_transition_firmware_context_0x14_firmware_record_context(working_result);
            core_process_flag_one_for_spi_response_context(working_result);
        }
        working_result = core_get_x02(core_led_controller_table);
        rti_set_software_timer(0x800f291, working_result);
    }
    return;
}

/**
 * @brief Handle pin input pin state.
 *
 * @return No value.
 */
void core_handle_pin_input_pin_state()

{
    /* Handle pin input pin state; persistent state is carried in core logical output fourteen hold timer. */
    int working_result;
    int operation_status;
    uint32_t pin_state;

    working_result = usb_hal_get_link_readiness();
    operation_status = core_process_sci_read_logical_for_resource_usb_state();
    if ((((operation_status == 0) || (working_result != 0)) ||
         ((working_result = core_is_not_cartridge_input_state_zero(), working_result != 0 && (working_result = core_is_ata_standby_active(), working_result == 0)))) ||
        (((working_result = core_is_software_timer_expired(&core_logical_output_fourteen_hold_timer),
           working_result == 0 || (working_result = core_get_constant_zero_value_for_pin_resource_state(), working_result != 0)) ||
          (working_result = core_is_mechanism_transition_pending(), working_result != 0)))) {
        pin_state = 1;
    } else {
        pin_state = 0;
    }
    spi_set_logical_output(0xe, pin_state);
    return;
}

/**
 * @brief Process request request for request request.
 *
 * @param context Procedure input.
 * @param request_value Procedure input.
 * @param request_flags Procedure input.
 * @return No value.
 */
void core_process_request_request_for_request_request(context, request_value, request_flags) uint32_t context;
uint32_t request_value;
uint32_t request_flags;

{
    /* Process request request for request request using only caller-provided data. */
    int operation_status;
    uint32_t working_result;
    uint32_t callback_result = 0;

    if ((request_flags & 1) == 0) {
        working_result = 0x26;
    } else {
        if (((request_value & 0xffffff80) == 0) ||
            (operation_status = core_is_scsi_write_opcode(context, request_value, request_flags >> 1), context = callback_result, operation_status == 0)) {
            operation_status = core_is_scsi_write_opcode(context);
            if (operation_status == 0) {
                core_set_command_sense_table_index(0x17);
                core_enable_request_flag_eight(1);
                return;
            }
            core_set_command_sense_table_index(0x16);
            core_enable_request_flag_sixteen(1);
            return;
        }
        working_result = 0x18;
    }
    core_set_command_sense_table_index(working_result);
    return;
}

/**
 * @brief Submit SCSI ATA command.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
int core_submit_scsi_ata_command(condition)
int condition;

{
    /* Submit SCSI ATA command; persistent state is carried in core active command descriptor. */
    int operation_result;
    uint32_t working_result;

    operation_result = core_submit_command_descriptor_to_device(*(uint8_t *)(core_active_command_descriptor + 0x10), condition, *(uint8_t *)(core_active_command_descriptor + 0x12));
    if (operation_result != 0) {
        if (operation_result == 6) {
            operation_result = 0xd;
        } else if (operation_result == 7) {
            return 0xe;
        }
        return operation_result;
    }
    if ((*(char *)(condition + 2) == 'a') || (*(char *)(condition + 2) == '`')) {
        working_result = 1;
    } else {
        working_result = 0;
    }
    operation_result = core_issue_ahci_command_slot(*(uint8_t *)(core_active_command_descriptor + 0x10), *(uint8_t *)(core_active_command_descriptor + 0x12), working_result);
    if (operation_result != 0) {
        return 0xe;
    }
    return 8;
}

/**
 * @brief Read reversed field value.
 *
 * @param context Procedure input.
 * @param range_start Procedure input.
 * @param range_end Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_read_reversed_field_value(context, range_start, range_end)
int context;
int range_start;
int range_end;

{
    /* Read reversed field value using only caller-provided data. */
    int condition_2_field;
    uint32_t *data_cursor;
    uint8_t *source_cursor;
    uint32_t status_bits;
    uint32_t working_result;

    status_bits = 0;
    working_result = 0;
    if (range_end < range_start) {
        core_halt_forever();
    }
    if (range_start < 0) {
        core_halt_forever();
    }
    if (8 < (uint32_t)(range_end - range_start)) {
        core_halt_forever();
    }
    if (range_start <= range_end) {
        condition_2_field = (range_end - range_start) + 1;
        data_cursor = &status_bits;
        source_cursor = (uint8_t *)(range_end + context);
        /* Repeat while condition_2_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            condition_2_field = condition_2_field + -1;
            *(uint8_t *)data_cursor = *source_cursor;
            data_cursor = (uint32_t *)((int)data_cursor + 1);
            source_cursor = source_cursor + -1;
        } while (condition_2_field != 0);
    }
    return CONCAT44(working_result, status_bits);
}

/**
 * @brief Configure USB endpoint.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_configure_usb_endpoint(condition, value, length) uint32_t condition;
int value;
uint32_t length;

{
    /* Configure USB endpoint; persistent state is carried in usb ep0 out max packet size, usb ep0 in max packet size. */
    uint16_t *data_cursor;
    uint32_t value_derived;
    uint32_t condition_field;

    data_cursor = (uint16_t *)&usb_ep0_out_max_packet_size;
    value_derived = value << 3 | 6;
    condition_field = condition << 0x1a | 0x700;
    if ((condition >> 7 & 1) != 0) {
        condition_field = condition << 0x1a | 0x2000700;
        data_cursor = (uint16_t *)&usb_ep0_in_max_packet_size;
        value_derived = value_derived | (length & 0x1f) << 0x11;
    }
    data_cursor[(condition & 0xffffff7f) * 0x14] = (int16_t)value;
    mww_send_usb_endpoint_command(condition, 1, value_derived, condition_field);
    core_update_mww_process_command(condition, 1);
    return;
}

/**
 * @brief Complete USB control out transfer.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_complete_usb_control_out_transfer(buffer) uint32_t *buffer;

{
    /* Complete USB control out transfer; persistent state is carried in usb ep0 state, core usb ep0 out transfer callback. */
    if (usb_ep0_state == 2) {
        if (((int)buffer[2] < 1) || (buffer[3] != buffer[1])) {
            if (core_usb_ep0_out_transfer_callback != (firmware_callback_t)0) {
                /* Notify core usb ep0 out transfer callback only after the associated state fields have been committed. */
                (*core_usb_ep0_out_transfer_callback)();
            }
            usb_ep0_state = 3;
        } else {
            core_update_mww_process_transfer(0, *buffer);
        }
    } else if (usb_ep0_state == 4) {
        usb_ep0_state = 0;
    } else {
        usb_ep0_state = 5;
    }
    core_abort_failed_usb_control_transfer();
    return;
}

/**
 * @brief Build ATA identity summary.
 *
 * @param output_record Procedure input.
 * @param offset Procedure input.
 * @param header_words Procedure input.
 * @return No value.
 */
void core_build_ata_identity_summary(output_record, offset, header_words) uint32_t *output_record;
int offset;
uint32_t *header_words;

{
    /* Build ATA identity summary using only caller-provided data. */
    uint32_t second_header_word;

    core_copy_bytes(output_record + 0xc, offset + 0x14, 0x14);
    core_copy_bytes(output_record + 2, offset + 0x36, 0x28);
    core_swap_adjacent_bytes(output_record + 0xc, 0x14);
    core_swap_adjacent_bytes(output_record + 2, 0x28);
    second_header_word = header_words[1];
    *output_record = *header_words;
    output_record[1] = second_header_word;
    return;
}

/**
 * @brief Execute ATA flush cache.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_execute_ata_flush_cache()

{
    /* Execute ATA flush cache using only caller-provided data. */
    int operation_status;
    uint32_t ahci_execute_taskfile_command_result;
    uint8_t working_buffer[32];
    uint8_t ata_command[10];
    volatile uint8_t working_result = 0;

    operation_status = core_update_capacity_block_context(working_buffer);
    if (operation_status == 0) {
        ahci_execute_taskfile_command_result = 0;
    } else {
        core_initialize_ata_taskfile(ata_command);
        operation_status = (int)core_get_cartridge_ata_device_info();
        if (*(char *)(operation_status + 0x12) == '\0') {
            working_result = 0xe7;
        } else {
            working_result = 0xea;
        }
        ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        core_clear_request_and_release_device_command(working_buffer);
    }
    return ahci_execute_taskfile_command_result;
}
