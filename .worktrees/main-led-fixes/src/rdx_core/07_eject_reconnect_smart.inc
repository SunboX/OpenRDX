/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Handle word metadata.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_word_metadata(context, offset)
uint32_t context;
int offset;

{
    /* Handle word metadata using only caller-provided data. */
    int operation_status;
    uint16_t *core_process_ti_memset_for_build_scsi_mode_result;
    uint32_t operation_result;
    uint32_t input_word = 0;
    uint32_t data_cursor[11];
    uint8_t working_buffer[8];
    int16_t working_result = 0;
    uint8_t record_metadata[12];

    operation_status = core_process_command_phase_three();
    if (operation_status == 0) {
        return 2;
    }
    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, data_cursor);
    if (operation_status == 0) {
        return 0x1a;
    }
    operation_result = 0;
    operation_status = core_dispatch_transfer_state_update(data_cursor, 0, offset, record_metadata, input_word & 0xffffff00);
    if (operation_status == 0) {
        operation_status = core_transfer_maximum_payload_payload(working_buffer, data_cursor[0], record_metadata, 0xc3, 0x28);
        if (operation_status != 0) {
            core_process_ti_memset_for_build_scsi_mode_result = (uint16_t *)core_process_ti_memset_for_build_scsi_mode(context, working_result + 8);
            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 0, 1, offset + 0xff10);
            core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 2, 3, 0x42);
            core_process_short_available(*(uint32_t *)(core_process_ti_memset_for_build_scsi_mode_result + 2), *core_process_ti_memset_for_build_scsi_mode_result, working_buffer, 4);
            goto core_handle_word_metadata_before_reset_output_record;
        }
    }
    operation_result = 0x29;
/* Shared target for core handle word metadata before reset output record; incoming paths preserve the same state assumptions. */
core_handle_word_metadata_before_reset_output_record:
    core_reset_output_record(data_cursor);
    return operation_result;
}

/**
 * @brief Coordinate safe cartridge eject.
 *
 * @return No value.
 */
void core_coordinate_safe_cartridge_eject()

{
    /* Coordinate safe cartridge eject; persistent state is carried in core mechanism update context, core state 121 and related record fields. */
    int operation_status;

    if (core_mechanism_update_context != '\0') {
        operation_status = core_get_cartridge_eject_control();
        if (operation_status == 0) {
            core_handle_command_phase();
        } else {
            if (core_state_121 == '\0') {
                core_handle_command_phase();
                core_state_121 = '\x01';
                rti_process_thermal_timer_state(&core_state_122, 5000);
            }
            operation_status = core_update_sci_read_logical_for_advance_mechanism_state();
            if (operation_status == 0) {
                rti_process_thermal_timer_state(&core_state_122, 0);
            } else {
                operation_status = core_get_thermal_control_context(&core_state_122);
                if (operation_status == 0) {
                    return;
                }
                operation_status = sci_read_logical_input(4);
                if (operation_status != 0) {
                    rti_process_thermal_timer_state(&core_state_122, 0);
                    core_process_mode_processing_context();
                    usb_hal_disconnect();
                    rti_delay_microseconds_with_watchdog(500000);
                    core_reconnect_device_and_start_timer();
                }
            }
            core_state_121 = '\0';
        }
        core_mechanism_update_context = '\0';
    }
    return;
}

/**
 * @brief Handle cartridge monitor context for thermal control context.
 *
 * @return No value.
 */
void core_handle_cartridge_monitor_context_for_thermal_control_context()

{
    /* Handle cartridge monitor context for thermal control context; persistent state is carried in core cartridge monitor context, core mode processing context and related record fields. */
    uint8_t sci_read_logical_input_result;
    int operation_status;
    uint32_t working_result;
    uint8_t *data_cursor;

    core_monitor_cartridge_input();
    sci_read_logical_input_result = sci_read_logical_input(4);
    core_transition_sequence_x01(sci_read_logical_input_result, &core_cartridge_monitor_context);
    core_coordinate_safe_cartridge_eject();
    if ((core_mode_processing_context != '\0') && (operation_status = core_forward_mww_transfer_value(), operation_status != 0)) {
        core_mode_processing_context = '\0';
        core_handle_ata_standby_context_for_cartridge_monitor_context();
    }
    if (core_command_state_context == '\0') {
        return;
    }
    if (core_gpio_pin_context != '\0') {
        return;
    }
    data_cursor = (uint8_t *)&core_state_130;
    operation_status = core_get_thermal_control_context(&core_state_130);
    if (operation_status == 0) {
        core_control_temperature_monitor();
        data_cursor = (uint8_t *)&core_response_update_context;
        operation_status = core_get_thermal_control_context(&core_response_update_context);
        if (operation_status == 0) {
            operation_status = core_get_runtime_context_base_for_build_scsi_mode();
            operation_status = core_get_structure_field_hex_70_value(operation_status + 0x2c);
            if (operation_status == 0) {
                return;
            }
        }
        if (core_ata_standby_context == '\0') {
            return;
        }
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_accumulate_split_transfer_offsets(operation_status + 0x2c);
        if (operation_status != 0) {
            working_result = 300000;
            goto core_handle_cartridge_monitor_context_for_thermal_control_context_before_rti_process_thermal_timer_state;
        }
    } else {
        operation_status = core_process_ata_standby_context();
        if (operation_status != 0) {
            return;
        }
    }
    working_result = 1000;
/* Shared target for core handle cartridge monitor context for thermal control context before RTI process thermal timer state; incoming paths preserve the same state assumptions. */
core_handle_cartridge_monitor_context_for_thermal_control_context_before_rti_process_thermal_timer_state:
    rti_process_thermal_timer_state(data_cursor, working_result);
    return;
}

/**
 * @brief Transfer find last nonzero.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param offset Procedure input.
 * @param flags Procedure input.
 * @param condition Procedure input.
 * @param destination Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_find_last_nonzero(context, value, offset, flags, condition, destination)
uint32_t context;
uint32_t value;
int offset;
uint32_t flags;
int condition;
uint32_t destination;

{
    /* Transfer find last nonzero using only caller-provided data. */
    uint32_t operation_result;
    int core_find_last_nonzero_word_result;
    uint32_t *computed_offset;
    uint32_t status_bits;

    if (condition == 0) {
        operation_result = 0xffffffff;
    } else {
        core_find_last_nonzero_word_result = core_find_last_nonzero_word(offset, destination, offset, flags, flags);
        computed_offset = (uint32_t *)(offset + (core_find_last_nonzero_word_result + -1) * 4);
        status_bits = 0x80000000;
        /* Repeat while status_bits != 0; the body advances or polls the state needed to leave the loop. */
        do {
            if ((status_bits & *computed_offset) != 0) {
                if (status_bits == 1) {
                    computed_offset = computed_offset + -1;
                    core_find_last_nonzero_word_result = core_find_last_nonzero_word_result + -1;
                    status_bits = 0x80000000;
                    goto core_transfer_find_last_nonzero_before_copy_word_buffer;
                }
                break;
            }
            status_bits = status_bits >> 1;
        } while (status_bits != 0);
        status_bits = status_bits >> 1;
    /* Shared target for core transfer find last nonzero before copy word buffer; incoming paths preserve the same state assumptions. */
    core_transfer_find_last_nonzero_before_copy_word_buffer:
        core_copy_word_buffer(context, value, condition);
        /* Iterate while core_find_last_nonzero_word_result != 0, preserving the defined cursor and bound. */
        while (core_find_last_nonzero_word_result != 0) {
            core_get_zero_word_buffer(context, context, context, flags, condition);
            if ((status_bits & *computed_offset) != 0) {
                core_get_zero_word_buffer(context, context, value, flags, condition);
            }
            if (status_bits == 1) {
                computed_offset = computed_offset + -1;
                core_find_last_nonzero_word_result = core_find_last_nonzero_word_result + -1;
                status_bits = 0x80000000;
            } else {
                status_bits = status_bits >> 1;
            }
        }
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Transition reconnect state.
 *
 * @return No value.
 */
void core_transition_reconnect_state()

{
    /* Transition reconnect state; persistent state is carried in core reconnect state, core state 156. */
    int operation_status;
    uint32_t working_result;
    uint32_t next_state;

    if (core_reconnect_state == 1) {
        operation_status = core_get_thermal_control_context(&core_state_156);
        if (operation_status == 0) {
            return;
        }
        operation_status = core_forward_cartridge_input_value();
        if (operation_status != 0) {
            core_reconnect_state = 2;
            return;
        }
        next_state = 3;
    } else {
        if (core_reconnect_state != 2) {
            if (core_reconnect_state != 3) {
                if (core_reconnect_state != 4) {
                    return;
                }
                operation_status = core_get_thermal_control_context(&core_state_156);
                if (operation_status != 0) {
                    core_reconnect_state = 0;
                    core_reconnect_device_and_start_timer();
                    return;
                }
                return;
            }
            operation_status = core_get_thermal_control_context(&core_state_156);
            if (operation_status == 0) {
                return;
            }
            operation_status = core_forward_cartridge_input_value();
            if (operation_status != 0) {
                core_reconnect_state = 2;
                return;
            }
            usb_hal_disconnect();
            next_state = 4;
            working_result = 1000;
            goto core_forward_reconnect_state_before_rti_process_thermal_timer_state;
        }
        operation_status = core_get_thermal_control_context(&core_state_156);
        if (operation_status == 0) {
            return;
        }
        operation_status = core_forward_cartridge_input_value();
        if (operation_status != 0) {
            core_reconnect_state = 2;
            return;
        }
        next_state = 1;
    }
    working_result = 1;
/* Shared target for core forward reconnect state before RTI process thermal timer state; incoming paths preserve the same state assumptions. */
core_forward_reconnect_state_before_rti_process_thermal_timer_state:
    rti_process_thermal_timer_state(&core_state_156, working_result);
    core_reconnect_state = next_state;
    return;
}

/**
 * @brief Handle x02 n.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_handle_x02_n(buffer) uint32_t *buffer;

{
    /* Handle x02 n using only caller-provided data. */
    char operation_status;
    uint16_t working_result;
    int core_process_ti_memset_for_build_scsi_mode_result;
    uint32_t core_get_extended_response_code_result;
    uint8_t condition_met;

    core_transfer_range_end_range_start_for_descriptor_descriptor_resource(buffer[2], 3, 5);
    core_transfer_range_end_range_start_for_descriptor_descriptor_resource(buffer[2], 6, 8);
    operation_status = *(char *)(buffer[2] + 2);
    condition_met = *(uint8_t *)(buffer[2] + 1) & 0x1f;
    if (condition_met == 2) {
        if (operation_status == '\x02') {
            working_result = core_get_extended_response_code();
            core_process_ti_memset_for_build_scsi_mode_result = (int)core_process_ti_memset_for_build_scsi_mode(buffer, working_result);
            core_process_clear_memory_primary_response_buffer(*(uint32_t *)(core_process_ti_memset_for_build_scsi_mode_result + 4));
        } else {
            if (operation_status != '\n')
                goto core_handle_x02_n_before_process_ti_memset_for_build_scsi_mode;
            working_result = core_get_condition_response_code();
            core_process_ti_memset_for_build_scsi_mode_result = (int)core_process_ti_memset_for_build_scsi_mode(buffer, working_result);
            core_process_clear_memory_secondary_response_buffer(*(uint32_t *)(core_process_ti_memset_for_build_scsi_mode_result + 4));
        }
    } else {
        if ((condition_met != 3) || (operation_status != '\x02')) {
        /* Shared target for core handle x02 n before process ti memset for build SCSI mode; incoming paths preserve the same state assumptions. */
        core_handle_x02_n_before_process_ti_memset_for_build_scsi_mode:
            *buffer = 8;
            return;
        }
        core_process_ti_memset_for_build_scsi_mode_result = (int)core_process_ti_memset_for_build_scsi_mode(buffer, 4);
        core_get_extended_response_code_result = core_get_extended_response_code();
        core_process_halt_forever_for_build_scsi_mode(core_process_ti_memset_for_build_scsi_mode_result, 1, 3, core_get_extended_response_code_result);
    }
    core_update_release_record_payload(core_process_ti_memset_for_build_scsi_mode_result, buffer, 6, 8);
    return;
}

/**
 * @brief Configure device descriptor 0x15 device descriptor storage.
 *
 * @param context Procedure input.
 * @param slot_index Procedure input.
 * @param length Procedure input.
 * @param device_descriptor Procedure input.
 * @return No value.
 */
void core_configure_device_descriptor_0x15_device_descriptor_storage(context, slot_index, length, device_descriptor) int context;
int slot_index;
int length;
int device_descriptor;

{
    /* Configure device descriptor 0x15 device descriptor storage; persistent state is carried in AHCI command list base, AHCI command table base. */
    char offset_2_field_0x15;
    int context_derived;
    int slot_offset;
    uint32_t *ahci_command_list_base_pointer;
    int working_result;

    slot_offset = slot_index * 0x20;
    context_derived = context * 0xa00;
    *(uint32_t *)(slot_offset + context_derived + -0x40000000 + 0x10000) = 5;
    *(uint32_t *)(&ahci_command_list_base + slot_offset + context_derived) =
        *(uint32_t *)(&ahci_command_list_base + slot_offset + context_derived) | length << 0x10;
    offset_2_field_0x15 = *(char *)(device_descriptor + 0x15);
    working_result = slot_offset + context_derived + -0x40000000;
    ahci_command_list_base_pointer = (uint32_t *)(&ahci_command_list_base + slot_offset + context_derived);
    if (offset_2_field_0x15 != '\0') {
        *ahci_command_list_base_pointer = *ahci_command_list_base_pointer | 0x40;
    }
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(char *)(context * 0x1d0 + slot_index + 0x800f4d2) = offset_2_field_0x15;
    if (*(int8_t *)(device_descriptor + 2) == -0x60) {
        *(uint32_t *)(working_result + 0x10000) = *(uint32_t *)(working_result + 0x10000) | 0x20;
    }
    *(uint32_t *)(working_result + 0x10004) = 0;
    *(uint8_t **)(working_result + 0x10008) = (uint8_t *)&ahci_command_table_base + context_derived + slot_index * 0x100;
    *(uint32_t *)(working_result + 0x1000c) = 0;
    return;
}

/**
 * @brief Transfer transfer completion flag.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_transfer_completion_flag(context, value, length, flags)
int context;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Transfer transfer completion flag using only caller-provided data. */
    int core_validate_transfer_capacity_result;
    uint32_t core_process_remaining_descriptor_space_for_transfer_completion_flag_result;
    uint32_t operation_result;
    uint32_t status_bits;
    uint32_t transfer_length;
    char completion_flag;
    uint8_t transfer_flags = 0;

    transfer_length = 4;
    /* Repeat while core_validate_transfer_capacity_result == 0; the body advances or polls the state needed to leave the loop. */
    do {
        status_bits = transfer_length;
        core_configure_descriptor_payload(&completion_flag, &transfer_length, flags);
        core_validate_transfer_capacity_result = core_validate_transfer_capacity(&completion_flag, transfer_length, flags);
    } while (core_validate_transfer_capacity_result == 0);
    operation_result = 0;
    if ((((transfer_flags & 1) == 0 && (transfer_flags & 4) == 0) && (completion_flag == '\0')) &&
        (core_process_remaining_descriptor_space_for_transfer_completion_flag_result = core_get_remaining_bounded_capacity(transfer_length, flags), *(uint16_t *)(context + 2) <= core_process_remaining_descriptor_space_for_transfer_completion_flag_result)) {
        core_process_remaining_descriptor_space_for_transfer_completion_flag(flags, context, &status_bits, value, length);
        core_process_remaining_descriptor_space_for_transfer_completion_flag(flags, &completion_flag, &status_bits, 0, 0);
    } else {
        operation_result = 0xf;
    }
    return operation_result;
}

/**
 * @brief Run ti memset component.
 *
 * @param offset Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_run_ti_memset_component(offset, condition, length, buffer)
int offset;
int condition;
uint32_t length;
uint32_t *buffer;

{
    /* Run ti memset component using only caller-provided data. */
    int operation_status;
    uint32_t working_result;
    int computed_offset;
    int result_context;
    uint32_t length_component;
    volatile uint32_t *volatile output_buffer = 0;

    computed_offset = offset;
    result_context = condition;
    length_component = length;
    output_buffer = buffer;
    ti_memset(buffer, 0, 0x10);
    buffer[1] = 1;
    *buffer = 1;
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
        operation_status = core_update_descriptor_requested(offset, &length_component, condition);
        if (operation_status != 0) {
        /* Shared target for core run ti memset component before process component for device ti memset; incoming paths preserve the same state assumptions. */
        core_run_ti_memset_component_before_process_component_for_device_ti_memset:
            return CONCAT44(result_context, operation_status);
        }
        working_result = core_process_component_for_device_ti_memset(offset, &length_component, length);
        *buffer = working_result;
        working_result = core_submit_default_transfer_value(length);
        buffer[2] = working_result;
        if (((*(uint8_t *)(condition + 1) >> 1 & 1) != 0) &&
            (operation_status = core_update_descriptor_subtraction(offset, &computed_offset, condition), operation_status == 0)) {
            working_result = core_process_component_for_device_ti_memset(offset, &computed_offset, length);
            buffer[1] = working_result;
            working_result = core_submit_default_transfer_value(length);
            buffer[3] = working_result;
        }
        if (*(char *)(offset + 0x6e) != '\0')
            goto core_run_ti_memset_component_before_process_component_for_device_ti_memset;
        if (*(char *)(offset + 0x6d) != '\0') {
            *(uint8_t *)(offset + 0x6d) = 0;
            goto core_run_ti_memset_component_before_process_component_for_device_ti_memset;
        }
        *(uint8_t *)(offset + 0x6d) = 1;
    } while (true);
}

/**
 * @brief Unused secondary update hook.
 *
 * @return No value.
 */
void core_unused_secondary_update_hook()

{
    /* Unused secondary update hook using only caller-provided data. */
    return;
}

/**
 * @brief Transfer command sequence update flag.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return No value.
 */
void core_transfer_command_sequence_update_flag(context, offset) uint32_t context;
int offset;

{
    /* Transfer command sequence update flag using only caller-provided data. */
    uint8_t core_has_active_command_flags_result;
    int operation_status;
    int remaining_count;
    int *data_cursor;
    int status_bits;
    int parsed_record[9];
    uint32_t update_flag;
    volatile int working_result = 0;
    volatile int output_offset = 0;
    int command_sequence[3];

    remaining_count = 0;
    *(uint8_t *)(offset + 0x6c) = 0;
    operation_status = core_initialize_output_record_from_source(offset, parsed_record);
    if (operation_status != 0) {
        command_sequence[0] = 1;
        command_sequence[1] = 0x1a;
        command_sequence[2] = 0;
        /* Iterate while data_cursor = command_sequence; *data_cursor != 0; data_cursor = data_cursor + 1, preserving the defined cursor and bound. */
        for (data_cursor = command_sequence; *data_cursor != 0; data_cursor = data_cursor + 1) {
            operation_status = core_process_component_for_command_sequence_update(offset, parsed_record, *data_cursor, &status_bits);
            if (operation_status == 0) {
                update_flag = 0;
                operation_status = status_bits + parsed_record[0];
                output_offset = operation_status + 4;
                working_result = remaining_count;
                core_transition_device_address(context, &update_flag);
                output_offset = operation_status + 0x84;
                update_flag = 1;
                working_result = remaining_count;
                core_transition_device_address(context, &update_flag);
            }
            remaining_count = remaining_count + 1;
        }
        core_reset_output_record(parsed_record);
        core_has_active_command_flags_result = core_has_active_command_flags(context);
        *(uint8_t *)(offset + 0x6c) = core_has_active_command_flags_result;
    }
    return;
}

/**
 * @brief Process clear memory primary response buffer.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_process_clear_memory_primary_response_buffer(offset) int offset;

{
    /* Process clear memory primary response buffer; persistent state is carried in core state 089, core primary response buffer and related record fields. */
    uint32_t core_state_089_snapshot;

    ti_memset(offset, 0, 0xac);
    *(uint32_t *)(offset + 0x24) = core_primary_response_buffer;
    *(uint32_t *)(offset + 0x1c) = core_state_078;
    core_forward_end_remaining(offset + 0x10, &core_unit_serial_number, 10);
    *(uint32_t *)(offset + 0x20) = core_state_082;
    core_forward_end_remaining(offset + 0x28, &core_state_084, 0x40);
    core_state_089_snapshot = core_state_089;
    *(uint16_t *)(offset + 0x78) = core_flash_validation_context;
    *(uint32_t *)(offset + 0x68) = core_state_089_snapshot;
    core_forward_end_remaining(offset + 0x7a, &core_state_093, 0x10);
    core_forward_end_remaining(offset + 0x8a, &core_state_094, 0x10);
    core_forward_end_remaining(offset + 0x9a, &core_state_095, 0x10);
    *(uint32_t *)(offset + 0x6c) = core_state_090;
    core_forward_end_remaining(offset + 0x70, &core_state_091, 8);
    return;
}

/**
 * @brief Process channel completion USB transfer state.
 *
 * @param channel_flags Procedure input.
 * @param value Procedure input.
 * @param completion_mode Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_process_channel_completion_usb_transfer_state(channel_flags, value, completion_mode, flags) uint32_t channel_flags;
int value;
uint32_t completion_mode;
uint32_t flags;

{
    /* Process channel completion USB transfer state; persistent state is carried in core descriptor component context, USB device state and related record fields. */
    uint32_t value_derived;
    uint32_t condition_1_field;
    uint32_t core_descriptor_component_context_pointer;

    value_derived = value << 3 | 4;
    if (usb_device_state == 3) {
        value_derived = value << 3 | 0x3c00004;
    }
    condition_1_field = channel_flags << 0x1a;
    core_descriptor_component_context_pointer = condition_1_field | 0x300;
    if ((completion_mode < 2) || (core_descriptor_component_context_pointer = condition_1_field | 0x1000300, usb_core_revision != 0x120a)) {
    /* Shared target for core process channel completion USB transfer state before updating descriptor component context pointer; incoming paths preserve the same state assumptions. */
    core_process_channel_completion_usb_transfer_state_before_updating_descriptor_component_context_pointer:
        if ((channel_flags >> 7 & 1) == 0) {
            (&core_descriptor_component_context)[(channel_flags & 0xffffff7f) * 0x14] = (int16_t)value;
            goto core_process_channel_completion_usb_transfer_state_before_process_command_word_channel_mww_command_channel;
        }
    } else if ((channel_flags >> 7 & 1) == 0) {
        core_descriptor_component_context_pointer = condition_1_field | 0x1002300;
        goto core_process_channel_completion_usb_transfer_state_before_updating_descriptor_component_context_pointer;
    }
    core_descriptor_component_context_pointer = core_descriptor_component_context_pointer | 0x2000000;
    value_derived = value_derived | (flags & 0x1f) << 0x11;
    (&core_state_070)[(channel_flags & 0xffffff7f) * 0x14] = (int16_t)value;
/* Shared target for core process channel completion USB transfer state before process command word channel MWW command channel; incoming paths preserve the same state assumptions. */
core_process_channel_completion_usb_transfer_state_before_process_command_word_channel_mww_command_channel:
    mww_process_command_word_channel_mww_command_channel(channel_flags, 1, value_derived, core_descriptor_component_context_pointer);
    core_update_mww_process_command(channel_flags, completion_mode);
    return;
}

/**
 * @brief Process response status value.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
void core_process_response_status_value(context, value, length, flags, index) uint32_t context;
uint32_t value;
uint32_t length;
int flags;
uint16_t index;

{
    /* Process response status value; persistent state is carried in core response status value. */
    int operation_status;
    int flags_snapshot;

    core_response_status_value = 0;
    flags_snapshot = flags;
    operation_status = core_transfer_usb_get_device(3, &flags_snapshot, 1);
    if (operation_status == 0) {
        core_copy_words_to_interleaved_destination(flags_snapshot, context, 0x10);
    }
    operation_status = core_transfer_usb_get_device(3, &flags_snapshot, 2);
    if (operation_status == 0) {
        core_copy_words_to_interleaved_destination(flags_snapshot, value, 0x20);
    }
    operation_status = core_transfer_usb_get_device(3, &flags_snapshot, 3);
    if (operation_status == 0) {
        core_copy_words_to_interleaved_destination(flags_snapshot + 4, length, 0x14);
    }
    operation_status = core_transfer_usb_get_device(1, &flags_snapshot, 0);
    if (operation_status == 0) {
        *(char *)(flags_snapshot + 8) = (char)flags;
        *(char *)(flags_snapshot + 9) = (char)((uint32_t)flags >> 8);
        *(char *)(flags_snapshot + 10) = (char)index;
        *(char *)(flags_snapshot + 0xb) = (char)((uint16_t)index >> 8);
    }
    return;
}

/**
 * @brief Handle primary sense context.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_handle_primary_sense_context(context, condition) int context;
int condition;

{
    /* Handle primary sense context; persistent state is carried in core primary sense context, core state 150 and related record fields. */
    int working_result;
    uint32_t context_derived;
    bool carry4_result;

    core_update_mark_primary_sense();
    core_process_command_state_context();
    working_result = (int)core_get_ata_command_buffer();
    context_derived = context * (uint32_t)*(uint16_t *)(working_result + 0x10);
    if (condition == 0) {
        working_result = core_get_runtime_context_base_for_build_scsi_mode();
        core_update_constant_0x54(working_result + 0x2c, context_derived);
        carry4_result = CARRY4(context_derived, core_primary_sense_context);
        core_primary_sense_context = context_derived + core_primary_sense_context;
        core_state_150 = core_state_150 + (uint32_t)carry4_result;
    } else {
        working_result = core_get_runtime_context_base_for_build_scsi_mode();
        core_update_constant_0x5c(working_result + 0x2c, context_derived);
        carry4_result = CARRY4(context_derived, core_sense_refresh_context);
        core_sense_refresh_context = context_derived + core_sense_refresh_context;
        core_state_148 = core_state_148 + (uint32_t)carry4_result;
        if (0x7fc4 < context_derived) {
            core_update_mechanism_update_context_for_primary_sense_context();
            return;
        }
    }
    return;
}

/**
 * @brief Configure descriptor payload.
 *
 * @param output_descriptor Procedure input.
 * @param bit_offset Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_configure_descriptor_payload(output_descriptor, bit_offset, length) uint8_t *output_descriptor;
int *bit_offset;
uint32_t length;

{
    /* Configure descriptor payload using only caller-provided data. */
    uint8_t descriptor_flags;
    uint8_t core_transfer_remaining_bounds_result;
    uint16_t payload_length;
    uint32_t core_write_four_byte_value_result;
    int core_get_remaining_descriptor_space_result;
    uint32_t working_result;
    uint64_t status_bits;

    core_transfer_remaining_bounds_result = core_transfer_remaining_bounds(bit_offset, 1);
    *output_descriptor = core_transfer_remaining_bounds_result;
    core_transfer_remaining_bounds_result = core_transfer_remaining_bounds(bit_offset, 1, length);
    output_descriptor[1] = core_transfer_remaining_bounds_result;
    payload_length = core_transfer_remaining_bounds(bit_offset, 2, length);
    descriptor_flags = output_descriptor[1];
    *(uint16_t *)(output_descriptor + 2) = payload_length;
    working_result = (uint32_t)(descriptor_flags >> 3);
    if ((descriptor_flags >> 2 & 1) == 0) {
        if ((descriptor_flags & 1) != 0) {
            core_write_four_byte_value_result = core_write_four_byte_value(bit_offset, length);
            *(uint32_t *)(output_descriptor + 4) = core_write_four_byte_value_result;
            status_bits = core_write_four_byte_value(bit_offset, length);
            working_result = (uint32_t)((uint64_t)status_bits >> 0x20);
            *(int *)(output_descriptor + 8) = (int)status_bits;
        }
    } else {
        status_bits = core_get_word(bit_offset, length);
        *(uint64_t *)(output_descriptor + 4) = status_bits;
        status_bits = core_get_word(bit_offset, length);
        working_result = (uint32_t)((uint64_t)status_bits >> 0x20);
        *(uint64_t *)(output_descriptor + 0xc) = status_bits;
    }
    core_get_remaining_descriptor_space_result = core_get_remaining_descriptor_space(output_descriptor, working_result);
    *bit_offset = core_get_remaining_descriptor_space_result + *bit_offset;
    return;
}

/**
 * @brief Normalize transfer status fields.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
bool core_normalize_transfer_status_fields(offset, value, length, flags) int offset;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Normalize transfer status fields; persistent state is carried in core memory transition context, core state 023. */
    int operation_status;
    bool condition_met;
    uint32_t status_bits;

    status_bits = flags;
    core_forward_end_remaining(&status_bits, length, 4, flags, length);
    ti_memset(&status_bits, 0, 2);
    if (((status_bits >> 16) & 0xffu) < 0xa1) {
        if (((status_bits >> 16) & 0xffu) == 0) {
            status_bits = status_bits & 0xffffff;
        }
    } else {
        status_bits = (status_bits & 0xff000000u) | (CONCAT12(0xa0, (uint16_t)status_bits) & 0x00ffffffu);
    }
    operation_status = core_dispatch_available_record(offset, &core_memory_transition_context, 4, &status_bits, 4);
    condition_met = operation_status == 0;
    operation_status = core_dispatch_available_record(offset, &core_state_023, 0xa0, value, status_bits >> 0x10 & 0xff);
    if ((operation_status != 0) ||
        ((condition_met &&
          (operation_status = ((int (*)())core_handle_fallback_decoded_request_bytes)(*(uint32_t *)(offset + 0x20), offset + 0xc, offset), operation_status != 0)))) {
        condition_met = false;
    }
    return condition_met;
}

/**
 * @brief Read smart temperature.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_read_smart_temperature(condition)
int condition;

{
    /* Read smart temperature; persistent state is carried in core persistent update context. */
    int operation_status;
    uint32_t core_read_buffer_word_result;
    uint8_t working_buffer[4];
    uint16_t working_result;

    /* SMART data is read through the same 512-byte ATA buffer used by the capacity probe. */
    operation_status = core_update_capacity_block_context(working_buffer);
    if (operation_status == 0) {
        core_read_buffer_word_result = 0;
    } else {
        working_result = 0x200;
        /* Subcommand 0xd0 requests SMART READ DATA; the response is one ATA sector. */
        operation_status = core_build_ata_smart_command(0xd0, working_buffer, 0);
        if (operation_status != 0) {
            if (condition != 0) {
                core_read_buffer_word_result = core_read_buffer_word(working_buffer);
                core_forward_end_remaining(condition, core_read_buffer_word_result, working_result);
            }
            core_read_buffer_word_result = core_read_buffer_word(working_buffer);
            /* Prefer attribute 0xc2 (temperature); fall back to 0xbe when it is absent. */
            operation_status = core_transfer_found_flag(core_read_buffer_word_result, &core_persistent_update_context, 0xc2);
            if (operation_status == 0) {
                core_read_buffer_word_result = core_read_buffer_word(working_buffer);
                operation_status = core_transfer_found_flag(core_read_buffer_word_result, &core_persistent_update_context, 0xbe);
                if (operation_status == 0) {
                    /* 0xff is the defined marker for a SMART page without either temperature attribute. */
                    core_persistent_update_context = 0xff;
                }
            }
        }
        core_update_thermal_monitor_enabled_for_read_smart_temperature(working_buffer);
        core_read_buffer_word_result = 1;
    }
    return core_read_buffer_word_result;
}

