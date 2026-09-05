/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Handle device ti memset.
 *
 * @param buffer Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
int core_handle_device_ti_memset(buffer, condition)
uint32_t *buffer;
int condition;

{
    /* Handle device ti memset using only caller-provided data. */
    int operation_result;
    int operation_status;
    uint32_t device_index;
    uint8_t working_buffer[8];
    uint8_t secondary_state[8];
    int status_bits;
    int secondary_result = 0;
    int working_result = 0;
    int expected_result = 0;

    device_index = buffer[8];
    operation_result = core_run_ti_memset_component(device_index, condition, buffer, &status_bits);
    operation_status = core_update_descriptor_requested(device_index, secondary_state, condition);
    if (((operation_status == 0) && (operation_status = core_update_descriptor_subtraction(device_index, working_buffer, condition), operation_status == 0)) &&
        ((*(uint8_t *)(condition + 1) >> 1 & 1) != 0)) {
        if (status_bits == 0) {
            if ((secondary_result != 0) || (expected_result != working_result)) {
                status_bits = core_process_component_for_device_ti_memset(device_index, secondary_state, buffer);
                if (status_bits == 0) {
                    secondary_result = core_update_device_for_device_ti_memset(device_index, working_buffer, buffer);
                } else {
                    secondary_result = core_process_component_for_device_ti_memset(device_index, working_buffer, buffer);
                }
            }
        } else if (secondary_result == 0) {
            secondary_result = core_process_component_for_device_ti_memset(device_index, working_buffer, buffer);
            if (secondary_result == 0) {
                status_bits = core_update_device_for_device_ti_memset(device_index, secondary_state, buffer);
            } else {
                status_bits = core_process_component_for_device_ti_memset(device_index, secondary_state, buffer);
            }
        }
    }
    if ((status_bits != 0) && (secondary_result != 0)) {
        if (status_bits == 5 && secondary_result == 5) {
            operation_result = 0x12;
        } else {
            operation_result = 3;
        }
    }
    if ((operation_result == 0x12) && (operation_status = core_update_f_x1c(condition), operation_status != 0)) {
        /* Handle device ti memset; this branch selects whether its associated handler runs. */
        ti_memset(*buffer, 0, buffer[1]);
    } else if (operation_result != 0) {
        ti_memset(buffer + 3, 0, 0x14);
        *(uint8_t *)(buffer + 3) = 0x1f;
        return operation_result;
    }
    core_forward_end_remaining(buffer + 3, condition, 0x14);
    return 0;
}

/**
 * @brief Dispatch firmware core work according to the decoded request or state value.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_drive_mechanism_outputs(context) uint32_t context;

{
    /* Drive mechanism outputs; persistent state is carried in core sense record context, core SCSI logical address context and related record fields. */
    int operation_status;
    uint32_t working_result;

    /* Dispatch on context; each case preserves the defined state-machine ordering. */
    switch (context) {
    case 0:
        core_halt_forever();
        break;
    case 1:
        core_sense_record_context = 1;
        core_scsi_logical_address_context = 0;
        core_state_113 = 0;
        core_update_spi_set_logical_for_monitor_cartridge_input(0);
        goto core_branch_target_063;
    case 2:
        working_result = 0x32;
        goto core_branch_target_062;
    case 3:
    /* Shared target for core branch target 063; incoming paths preserve the same state assumptions. */
    core_branch_target_063:
        spi_set_logical_output(10, 0);
        rti_delay_microseconds_with_watchdog(0x32);
        core_state_119 = 0;
        working_result = 0xd;
    /* Shared target for core drive mechanism outputs before apply pwm percentage; incoming paths preserve the same state assumptions. */
    core_drive_mechanism_outputs_before_apply_pwm_percentage:
        core_apply_pwm_percentage(working_result, 100, 0x32);
    /* Shared target for core branch target 061; incoming paths preserve the same state assumptions. */
    core_branch_target_061:
        working_result = 2000;
    /* Shared target for core branch target 062; incoming paths preserve the same state assumptions. */
    core_branch_target_062:
        rti_process_thermal_timer_state(&core_command_completion_context, working_result);
        break;
    case 4:
        core_process_set_validation_flags(1);
        rti_process_thermal_timer_state(&core_command_completion_context, 500);
        core_reset_logical_outputs();
        core_state_110 = 0;
        break;
    case 5:
        core_reset_logical_outputs();
        goto core_branch_target_061;
    case 6:
        core_scsi_logical_address_context = 0;
        core_state_113 = 0;
        spi_set_logical_output(10);
        rti_delay_microseconds_with_watchdog(0x32);
        operation_status = core_update_flash_validation_context();
        if (operation_status == 1) {
            working_result = 0xc;
        /* Shared target for core branch target 059; incoming paths preserve the same state assumptions. */
        core_branch_target_059:
            core_state_119 = 1;
        } else {
            working_result = 0xd;
            if (operation_status == 2) {
                spi_set_logical_output(0xb, 1);
                goto core_branch_target_059;
            }
        }
        goto core_drive_mechanism_outputs_before_apply_pwm_percentage;
    case 7:
        core_reset_logical_outputs();
        break;
    case 8:
        core_reset_logical_outputs();
        core_process_set_validation_flags(0);
        core_state_110 = 0;
    }
    core_state_111 = context;
    return;
}

/**
 * @brief Handle lookup key metadata device descriptor context.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_lookup_key_metadata_device_descriptor_context(context, buffer)
uint32_t context;
int *buffer;

{
    /* Handle lookup key metadata device descriptor context; persistent state is carried in core device descriptor context. */
    int operation_status;
    uint32_t core_get_0xc_for_device_descriptor_response_result;
    int core_process_ata_command_for_lookup_key_metadata_result;
    int callback_result = 0;
    uint32_t operation_result;
    char data_cursor[4];
    uint8_t working_buffer[44];
    uint8_t entry_metadata[12];
    uint8_t lookup_key[8];
    uint16_t working_result = 0;

    /* This handler accepts only the defined 0xc0 lookup request with a nonzero record selector. */
    /* Gate the following state transition on *buffer != 0xc0; the handler is skipped when this condition is false. */
    if (*buffer != 0xc0) {
        return 0x28;
    }
    if ((int16_t)buffer[2] == 0) {
        return 0x28;
    }
    core_process_command_phase_three();
    data_cursor[0] = core_get_0xc_for_lookup_key_metadata(buffer);
    /* The callback supplies the lookup key before the current device record is materialized. */
    if (callback_result == 0) {
        return 2;
    }
    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, working_buffer);
    if (operation_status == 0) {
        return 0x28;
    }
    operation_status = core_get_ti_memset(working_buffer, data_cursor[0], lookup_key, entry_metadata, 0xc0);
    if ((operation_status == 0) || (working_result < 0x4c)) {
        /* Reject missing or shorter-than-0x4c records before examining descriptor flags. */
        operation_result = 0x29;
    } else {
        core_get_0xc_for_device_descriptor_response_result = core_get_0xc_for_device_descriptor_response(lookup_key);
        if (((core_get_0xc_for_device_descriptor_response_result >> 4 & 1) == 0) || (core_device_descriptor_context == 3)) {
            /* Bit 4 normally blocks the request; descriptor context 3 is the defined override. */
            operation_status = core_handle_terminal_found_lookup_succeeded(working_buffer, data_cursor);
            if (operation_status != 0) {
                operation_status = core_handle_descriptor_device(working_buffer, 0, data_cursor[0], 0, entry_metadata);
                core_process_ata_command_for_lookup_key_metadata_result = core_process_ata_command_for_lookup_key_metadata(working_buffer);
                if ((operation_status == 0) && (core_process_ata_command_for_lookup_key_metadata_result != 0)) {
                    /* Retry the device operation with the alternate descriptor selection. */
                    operation_status = core_handle_descriptor_device(working_buffer, 1, data_cursor[0], 0, entry_metadata);
                    core_process_ata_command_for_lookup_key_metadata_result = core_process_ata_command_for_lookup_key_metadata(working_buffer);
                    if ((core_process_ata_command_for_lookup_key_metadata_result != 0) && (operation_status == 0)) {
                        operation_result = 0;
                        /* Device contexts 2 and 3 need the primary command path after a type-zero match. */
                        if ((core_device_descriptor_context == 2 || core_device_descriptor_context == 3) && (data_cursor[0] == '\0')) {
                            core_process_primary_command_context();
                        }
                        goto core_handle_lookup_key_metadata_device_descriptor_context_before_reset_output_record;
                    }
                }
            }
            operation_result = 0x21;
        } else {
            operation_result = 0x27;
        }
    }
/* Shared target for core handle lookup key metadata device descriptor context before reset output record; incoming paths preserve the same state assumptions. */
core_handle_lookup_key_metadata_device_descriptor_context_before_reset_output_record:
    /* Release the temporary descriptor record on every post-allocation exit path. */
    core_reset_output_record(working_buffer);
    return operation_result;
}

/**
 * @brief Handle terminal found lookup succeeded.
 *
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_terminal_found_lookup_succeeded(condition, buffer)
uint32_t condition;
uint8_t *buffer;

{
    /* Handle terminal found lookup succeeded using only caller-provided data. */
    bool condition_met;
    bool terminal_entry_found;
    bool lookup_succeeded;
    uint32_t core_get_0xc_for_device_descriptor_response_result;
    uint32_t entry_flags;
    int operation_status;
    uint8_t *data_cursor;
    uint32_t entry_index;
    uint32_t operation_result;
    uint64_t lookup_result;
    uint8_t working_buffer[16];
    uint8_t entry_metadata[12];

    /* Scan all eight descriptor slots and aggregate properties of every valid entry. */
    condition_met = false;
    lookup_succeeded = false;
    terminal_entry_found = false;
    entry_index = 0;
    data_cursor = buffer;
    /* Repeat while (int)entry_index < 8; the body advances or polls the state needed to leave the loop. */
    do {
        if ((((buffer == (uint8_t *)0x0) || (core_get_0xc_for_device_descriptor_response_result = (uint32_t)*buffer, core_get_0xc_for_device_descriptor_response_result != entry_index)) &&
             (lookup_result = core_validate_device_descriptor_length(condition, entry_index & 0xff, working_buffer, entry_metadata),
              data_cursor = (uint8_t *)(uint32_t)((uint64_t)lookup_result >> 0x20), core_get_0xc_for_device_descriptor_response_result = 0,
              (int)lookup_result != 0)) &&
            (lookup_result = core_has_device_descriptor_high_bit(working_buffer),
             data_cursor = (uint8_t *)(uint32_t)((uint64_t)lookup_result >> 0x20), core_get_0xc_for_device_descriptor_response_result = 0,
             (int)lookup_result != 0)) {
            entry_flags = core_get_0xc_for_device_descriptor_response(working_buffer);
            data_cursor = (uint8_t *)(entry_flags >> 4);
            /* Bit 3 marks an excluded lookup; bit 0 marks a non-terminal entry. */
            if ((entry_flags >> 3 & 1) == 0) {
                lookup_succeeded = true;
            }
            condition_met = true;
            core_get_0xc_for_device_descriptor_response_result = entry_flags >> 1;
            if ((entry_flags & 1) == 0) {
                terminal_entry_found = true;
            }
        }
        entry_index = entry_index + 1;
    } while ((int)entry_index < 8);
    operation_result = 1;
    if (condition_met) {
        /* Bit 22 in the secondary record mirrors whether the scan produced a usable terminal result. */
        if ((terminal_entry_found) || (!lookup_succeeded)) {
            operation_status = core_get_runtime_context_base_for_build_scsi_mode(core_get_0xc_for_device_descriptor_response_result, data_cursor);
            operation_status = operation_status + 0x2c;
            entry_index = core_read_descriptor_field_24_value(operation_status);
            entry_index = entry_index | 0x400000;
        } else {
            operation_status = core_get_runtime_context_base_for_build_scsi_mode(core_get_0xc_for_device_descriptor_response_result, data_cursor);
            operation_status = operation_status + 0x2c;
            entry_index = core_read_descriptor_field_24_value(operation_status);
            entry_index = entry_index & 0xffbfffff;
        }
        core_set_secondary_descriptor_value_ready(operation_status, entry_index);
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = operation_status + 0x2c;
        entry_index = core_read_descriptor_field_20_value(operation_status);
        entry_index = entry_index | 0x400000;
    } else {
        /* No valid slot clears the secondary ready bit and also clears it in the primary record below. */
        operation_status = core_get_runtime_context_base_for_build_scsi_mode(core_get_0xc_for_device_descriptor_response_result, data_cursor);
        entry_index = core_read_descriptor_field_24_value(operation_status + 0x2c);
        core_set_secondary_descriptor_value_ready(operation_status + 0x2c, entry_index & 0xffbfffff);
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = operation_status + 0x2c;
        entry_index = core_read_descriptor_field_20_value(operation_status);
        entry_index = entry_index & 0xffbfffff;
    }
    core_set_primary_descriptor_value_ready(operation_status, entry_index);
    /* A successful reset and ATA lookup converts the provisional success value to zero. */
    operation_status = core_process_reset_output_record(condition);
    if ((operation_status == 0) || (operation_status = core_process_ata_command_for_lookup_key_metadata(condition), operation_status == 0)) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Process parsed header descriptor.
 *
 * @param destination_offset Procedure input.
 * @param record_list_offset Procedure input.
 * @param source_descriptor Descriptor containing the source record fields.
 * @param flags Procedure input.
 * @param record_table Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_parsed_header_descriptor(destination_offset, record_list_offset, source_descriptor, flags, record_table)
int destination_offset;
int record_list_offset;
int source_descriptor;
uint32_t flags;
int record_table;

{
    /* Process parsed header descriptor using only caller-provided data. */
    uint16_t source_descriptor_length;
    int operation_status;
    uint32_t core_get_0xc_for_lookup_key_metadata_result;
    uint32_t core_get_0xc_for_device_descriptor_response_result;
    uint32_t return_code;
    int record_table_base;
    uint32_t current_record_offset;
    uint32_t input_word = 0;
    uint8_t working_result;
    int data_cursor[2];
    uint16_t record_length = 0;
    int parsed_record_header[2];
    uint16_t status_bits = 0;
    int record_header[2];
    uint16_t record_offset = 0;
    uint32_t flags_snapshot;

    working_result = (uint8_t)((uint32_t)input_word >> 0x18);
    /* The record list must begin with 0xc3, followed by 0xc4 and then an aligned 0xc1 payload. */
    operation_status = core_submit_configured_payload(record_header, record_table, record_list_offset);
    return_code = 0x28;
    if ((operation_status != 0) && (record_header[0] == 0xc3)) {
        current_record_offset = record_list_offset + (uint32_t)record_offset + 4 & 0xffff;
        /* Offsets are deliberately truncated to the defined 16-bit record-list address space. */
        flags_snapshot = flags;
        operation_status = core_submit_configured_payload(data_cursor, record_table, current_record_offset);
        if ((operation_status != 0) && (data_cursor[0] == 0xc4)) {
            current_record_offset = current_record_offset + record_length + 4 & 0xffff;
            operation_status = core_submit_configured_payload(parsed_record_header, record_table, current_record_offset);
            if ((((operation_status != 0) && (parsed_record_header[0] == 0xc1)) && (0x6f < status_bits)) && ((status_bits & 0xf) == 0)) {
                /* The 0xc1 body must be at least 0x70 bytes and aligned to a 16-byte boundary. */
                record_table_base = *(int *)(record_table + 4);
                source_descriptor_length = *(uint16_t *)(source_descriptor + 8);
                return_code = core_get_0xc_for_lookup_key_metadata(source_descriptor);
                operation_status = core_process_component_for_parsed_header_descriptor(flags_snapshot, return_code, current_record_offset + record_table_base + 4, 0,
                                                  CONCAT13(working_result, (uint32_t)(status_bits & 0xff)));
                return_code = 0x23;
                if (operation_status != 0) {
                    /* Only configure the destination after the nested component validates successfully. */
                    core_get_0xc_for_lookup_key_metadata_result = core_get_0xc_for_lookup_key_metadata(source_descriptor);
                    core_get_0xc_for_device_descriptor_response_result = core_get_0xc_for_device_descriptor_response(source_descriptor);
                    operation_status = core_configure_component_for_parsed_header_descriptor(
                        flags_snapshot, core_get_0xc_for_lookup_key_metadata_result, core_get_0xc_for_device_descriptor_response_result, record_table_base + destination_offset,
                        destination_offset + (uint32_t)record_offset + (uint32_t)source_descriptor_length + (uint32_t)record_length + 0xc & 0xff);
                    if (operation_status != 0) {
                        return_code = 0;
                    }
                }
            }
        }
    }
    return return_code;
}

/**
 * @brief Submit command descriptor to device.
 *
 * @param context Procedure input.
 * @param command_descriptor Procedure input.
 * @param device_index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_submit_command_descriptor_to_device(context, command_descriptor, device_index)
int context;
int command_descriptor;
uint32_t device_index;

{
    /* Submit command descriptor to device; persistent state is carried in core command descriptor device, AHCI port command issue register and related record fields. */
    uint32_t operation_result;
    int operation_status;
    int context_derived;
    uint32_t mww_transfer_mww0_addr_mww1_addr_mww_address_pair_result;

    context_derived = context * 0x1d0;
    if ((&core_command_descriptor_device)[context_derived] == '\0') {
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        if (*(uint8_t *)(context_derived + 0x800f4a8) < device_index) {
            return 1;
        }
        mww_transfer_mww0_addr_mww1_addr_mww_address_pair_result = 0;
        operation_result = 0;
        if (*(char *)(command_descriptor + 2) == '`' || *(char *)(command_descriptor + 2) == 'a') {
            *(char *)(command_descriptor + 0xc) = (char)(device_index << 3);
        }
        if ((1 << (device_index & 0xff) &
             (*(uint32_t *)(&ahci_port_command_issue_register + context * 0x80) | *(uint32_t *)(&core_state_213 + context * 0x80))) == 0) {
            operation_status = core_get_runtime_context_base_for_command_descriptor_device(*(uint8_t *)(command_descriptor + 2), context);
            if (operation_status == 0) {
                return 1;
            }
            /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
            *(uint8_t *)(context_derived + device_index + 0x800f4ca) = *(uint8_t *)(command_descriptor + 0x14);
            operation_status = context * 0xa00 + -0x40000000 + device_index * 0x100;
            core_transfer_end_remaining(operation_status + 0x10200, command_descriptor, 0x14);
            if (*(int8_t *)(command_descriptor + 2) == -0x60) {
                core_transfer_end_remaining(operation_status + 0x10240, command_descriptor + 0x1c, 0x10);
            }
            operation_status = *(int *)(command_descriptor + 0x18);
            /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
            *(uint8_t *)(context_derived + device_index + 0x800f5f8) = *(uint8_t *)(command_descriptor + 0x2c);
            if (operation_status != 0) {
                mww_transfer_mww0_addr_mww1_addr_mww_address_pair_result = mww_transfer_mww0_addr_mww1_addr_mww_address_pair(device_index, command_descriptor);
            }
            core_configure_device_descriptor_0x15_device_descriptor_storage(context, device_index, mww_transfer_mww0_addr_mww1_addr_mww_address_pair_result, command_descriptor);
        } else {
            operation_result = 7;
        }
    } else {
        operation_result = 6;
    }
    return operation_result;
}

/**
 * @brief Transition selected or bit selection state.
 *
 * @param buffer Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
bool core_transition_selected_or_bit_selection_state(buffer, offset) uint32_t *buffer;
int offset;

{
    /* Transition selected or bit selection state; persistent state is carried in core selection state, core state 012 and related record fields. */
    int16_t working_result;
    uint16_t operation_status;
    int selected_entry_or_status;
    uint32_t bit_offset;
    uint32_t status_bits;
    uint64_t core_process_halt_forever_for_selected_or_bit_result;

    status_bits = (uint32_t)**(uint8_t **)(offset + 4);
    buffer[3] = 0;
    if (status_bits < 0x89) {
        if (status_bits == 0x88) {
        /* Shared target for core transition selected or bit selection state before updating selected entry or status; incoming paths preserve the same state assumptions. */
        core_transition_selected_or_bit_selection_state_before_updating_selected_entry_or_status:
            selected_entry_or_status = 3;
        } else if ((status_bits == 8) || (status_bits == 10)) {
            selected_entry_or_status = 0;
        } else {
            if (((status_bits != 0x28) && (status_bits != 0x2a)) && (1 < status_bits - 0x2e))
                goto core_transition_selected_or_bit_selection_state_before_process_halt_forever_for_selected_or_bit;
            selected_entry_or_status = 1;
        }
    } else {
        if ((status_bits == 0x8a) || (status_bits - 0x8e < 2))
            goto core_transition_selected_or_bit_selection_state_before_updating_selected_entry_or_status;
        if ((status_bits != 0xa8) && ((status_bits != 0xaa && (1 < status_bits - 0xae)))) {
        /* Shared target for core transition selected or bit selection state before process halt forever for selected or bit; incoming paths preserve the same state assumptions. */
        core_transition_selected_or_bit_selection_state_before_process_halt_forever_for_selected_or_bit:
            buffer[3] = 7;
            status_bits = 7;
            goto core_transition_selected_or_bit_selection_state_return_path;
        }
        selected_entry_or_status = 2;
    }
    selected_entry_or_status = selected_entry_or_status * 4;
    core_process_halt_forever_for_selected_or_bit_result = core_process_halt_forever_for_selected_or_bit(offset, (&core_selection_state)[selected_entry_or_status], (&core_state_012)[selected_entry_or_status]);
    *(uint64_t *)buffer = core_process_halt_forever_for_selected_or_bit_result;
    working_result = core_release_record_payload_when_short(offset, (&core_storage_002)[selected_entry_or_status], (&core_storage_003)[selected_entry_or_status]);
    *(int16_t *)(buffer + 2) = working_result;
    if ((status_bits == 10 || status_bits == 8) && (working_result == 0)) {
        *(uint16_t *)(buffer + 2) = 0x100;
    }
    selected_entry_or_status = core_get_response_end_context_for_response_end_word();
    if (selected_entry_or_status != 0) {
        status_bits = buffer[2];
        operation_status = (uint16_t)status_bits << 3;
        *(uint16_t *)(buffer + 2) = operation_status;
        if (operation_status < (uint16_t)status_bits) {
            buffer[3] = 8;
        }
        selected_entry_or_status = (int)core_get_ata_command_buffer();
        status_bits = *buffer;
        operation_status = *(uint16_t *)(selected_entry_or_status + 0x14);
        bit_offset = status_bits * 8;
        *buffer = operation_status + bit_offset;
        buffer[1] = (buffer[1] << 3 | status_bits >> 0x1d) + (uint32_t)CARRY4((uint32_t)operation_status, bit_offset);
    }
    status_bits = buffer[3];
/* Shared target for core transition selected or bit selection state return path; incoming paths preserve the same state assumptions. */
core_transition_selected_or_bit_selection_state_return_path:
    return status_bits == 0;
}

/**
 * @brief Handle response base device descriptor context.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_response_base_device_descriptor_context(context, condition)
uint32_t context;
int condition;

{
    /* Handle response base device descriptor context; persistent state is carried in core device descriptor context. */
    int operation_status;
    uint32_t core_get_constant_hex_1014_value_result;
    uint32_t operation_result;
    uint32_t response_offset;
    uint32_t working_result;
    uint32_t data_cursor[11];
    uint8_t working_buffer[4];
    int base_offset = 0;
    int status_bits = 0;

    operation_status = core_process_command_phase_three();
    if (operation_status == 0) {
        return 2;
    }
    if ((core_device_descriptor_context != 3) || (condition != 0)) {
        return 0x27;
    }
    operation_status = core_get_runtime_context_base_for_build_scsi_mode(0, &core_device_descriptor_context, 0);
    operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, data_cursor);
    if (operation_status == 0) {
        return 0x1a;
    }
    operation_result = 0;
    operation_status = core_dispatch_transfer_state_update(data_cursor, 1, 0, working_buffer, 0);
    if (operation_status == 0) {
        core_get_constant_hex_1014_value_result = core_get_constant_hex_1014_value(context);
        working_result = base_offset + status_bits & 0xffff;
        response_offset = working_result + 8 & 0xffff;
        if (response_offset <= core_get_constant_hex_1014_value_result) {
            operation_status = (int)core_process_ti_memset_for_build_scsi_mode(context, response_offset);
            core_process_halt_forever_for_build_scsi_mode(operation_status, 0, 1, 0xff10);
            core_process_halt_forever_for_build_scsi_mode(operation_status, 2, 3, response_offset - 4);
            core_process_halt_forever_for_build_scsi_mode(operation_status, 4, 4, 0xc1);
            core_process_halt_forever_for_build_scsi_mode(operation_status, 6, 7, working_result);
            core_forward_end_remaining(*(int *)(operation_status + 4) + 8, data_cursor[0], working_result);
            goto core_handle_response_base_device_descriptor_context_before_reset_output_record;
        }
    }
    operation_result = 0x29;
/* Shared target for core handle response base device descriptor context before reset output record; incoming paths preserve the same state assumptions. */
core_handle_response_base_device_descriptor_context_before_reset_output_record:
    core_reset_output_record(data_cursor);
    return operation_result;
}

/**
 * @brief Handle descriptor device.
 *
 * @param device_record Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @param expected_value Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_descriptor_device(device_record, condition, length, expected_value, index)
int device_record;
int condition;
uint32_t length;
uint32_t expected_value;
uint32_t index;

{
    /* Handle descriptor device using only caller-provided data. */
    int status_bits;
    uint32_t core_get_remaining_descriptor_space_result;
    uint32_t core_transition_use_alternate_lookup_type_copy_result;
    uint8_t working_buffer[24];
    uint8_t computed_descriptor[24];

    if (condition != 0) {
        if ((expected_value & 0xf) != 0) {
            return 2;
        }
        expected_value = expected_value + 8;
    }
    status_bits = core_process_transfer_requested(device_record, condition, length, computed_descriptor, working_buffer);
    if (status_bits == 0) {
        core_get_remaining_descriptor_space_result = core_get_remaining_descriptor_space(computed_descriptor);
        if (expected_value == core_get_remaining_descriptor_space_result)
            goto core_handle_descriptor_device_before_transition_use_alternate_lookup_type_copy;
    } else {
        if (status_bits == 0x14) {
            return 1;
        }
        if (status_bits == 0x11) {
            if (condition == 0) {
                core_transition_use_alternate_lookup_type_copy_result = 0x1c;
            } else {
                core_transition_use_alternate_lookup_type_copy_result = 0x1d;
            }
            status_bits = core_configure_range_start(device_record, working_buffer, core_transition_use_alternate_lookup_type_copy_result, 8);
            if (status_bits != 0) {
                return 2;
            }
            status_bits = core_process_transfer_requested(device_record, condition, length, computed_descriptor, working_buffer);
            if (status_bits != 0 && status_bits != 6) {
                return 2;
            }
        } else if (status_bits != 6) {
            return 2;
        }
    }
    core_initialize_transfer_workspace(device_record);
    if (condition == 0) {
        core_transition_use_alternate_lookup_type_copy_result = 0x3b;
    } else {
        core_transition_use_alternate_lookup_type_copy_result = 0x3c;
    }
    status_bits = core_build_indexed_transfer_descriptor(core_transition_use_alternate_lookup_type_copy_result, expected_value, 0, 0, device_record);
    if (status_bits != 0) {
        return 2;
    }
    status_bits = core_handle_fallback_decoded_request_bytes(*(uint32_t *)(device_record + 0x20), device_record + 0xc, device_record);
    if (status_bits != 0) {
        return 2;
    }
    status_bits = core_submit_zero_based_transfer(core_transition_use_alternate_lookup_type_copy_result, computed_descriptor, device_record);
    if (status_bits != 0) {
        return 2;
    }
/* Shared target for core handle descriptor device before transition use alternate lookup type copy; incoming paths preserve the same state assumptions. */
core_handle_descriptor_device_before_transition_use_alternate_lookup_type_copy:
    core_transition_use_alternate_lookup_type_copy_result = core_transition_use_alternate_lookup_type_copy(index, computed_descriptor, condition, length, device_record, 0);
    return core_transition_use_alternate_lookup_type_copy_result;
}

/**
 * @brief Halt forever.
 *
 * @return No value.
 */
void core_halt_forever()

{
    /* Halt forever using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief Parse stream record with output word.
 *
 * @param condition Procedure input.
 * @param output_word Procedure input.
 * @param input_bytes Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_parse_stream_record_with_output_word(condition, output_word, input_bytes, flags)
int condition;
uint32_t *output_word;
uint8_t *input_bytes;
uint32_t flags;

{
    /* Parse stream record with output word; persistent state is carried in core response buffer, core persistent state status. */
    uint8_t condition_met;
    uint32_t operation_status;
    int core_transfer_usb_get_device_result;
    uint32_t operation_result;
    uint32_t status_bits;
    int core_response_buffer_snapshot;
    uint8_t *buffer_2_cursor;
    uint32_t *data_cursor;

    core_response_buffer_snapshot = (int)core_response_buffer;
    buffer_2_cursor = input_bytes;
    data_cursor = (uint32_t *)flags;
    core_transfer_usb_get_device_result = core_transfer_usb_get_device(2, &buffer_2_cursor, 0);
    if (core_transfer_usb_get_device_result == 0) {
        status_bits = (uint32_t)*buffer_2_cursor;
        data_cursor = output_word;
        core_transfer_end_remaining(core_response_buffer_snapshot, buffer_2_cursor, status_bits);
        buffer_2_cursor = buffer_2_cursor + status_bits;
        core_response_buffer_snapshot = status_bits + core_response_buffer_snapshot;
        condition_met = *buffer_2_cursor;
        /* Iterate while operation_status = (uint32_t)condition_met, operation_status != 0, preserving the defined cursor and bound. */
        while (operation_status = (uint32_t)condition_met, operation_status != 0) {
            condition_met = buffer_2_cursor[1];
            if (((condition_met == 4) || (condition_met == 5)) || (condition_met == 0x21 || condition_met == 0x24)) {
                core_transfer_end_remaining(core_response_buffer_snapshot, buffer_2_cursor, operation_status);
                if ((condition_met == 5) && (*(char *)(core_response_buffer_snapshot + 3) == '\x02')) {
                    if (condition == 2) {
                        *(uint8_t *)(core_response_buffer_snapshot + 4) = 0;
                        *(uint8_t *)(core_response_buffer_snapshot + 5) = 2;
                    } else if (condition == 1) {
                        *(uint8_t *)(core_response_buffer_snapshot + 4) = 0x40;
                        *(uint8_t *)(core_response_buffer_snapshot + 5) = 0;
                    }
                }
                status_bits = operation_status + status_bits;
                core_response_buffer_snapshot = operation_status + core_response_buffer_snapshot;
            }
            if (condition_met == 0x21) {
                buffer_2_cursor = buffer_2_cursor + *(uint16_t *)(buffer_2_cursor + 7);
            }
            buffer_2_cursor = buffer_2_cursor + operation_status;
            condition_met = *buffer_2_cursor;
        }
        *(char *)(core_response_buffer + 2) = (char)status_bits;
        *(char *)(core_response_buffer + 3) = (char)(status_bits >> 8);
        *(uint8_t *)(core_response_buffer + 8) = core_persistent_state_status;
        operation_result = 0;
        *data_cursor = status_bits;
    } else {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Prepare command transfer direction.
 *
 * @param command_bits Procedure input.
 * @param transfer_direction Procedure input.
 * @return No value.
 */
uint32_t core_prepare_command_transfer_direction(command_bits, transfer_direction)
uint32_t command_bits;
int transfer_direction;

{
    /* Prepare command transfer direction; persistent state is carried in USB ep0 buffer, core descriptor component context and related record fields. */
    uint32_t working_result;
    uint8_t *usb_ep0_buffer_pointer;
    uint32_t operation_status;
    uint8_t *core_descriptor_component_context_snapshot;

    operation_status = command_bits >> 0xc & 3;
    if (core_firmware_transfer_context == 0) {
        if ((command_bits >> 0xc & 3) == 0)
            goto core_prepare_command_transfer_direction_before_process_wdt_reset;
        core_firmware_transfer_context = 5;
    }
    if (operation_status == 2) {
        if (core_firmware_transfer_context != 5) {
            core_configure_descriptor_configuration_context();
        }
    } else if (operation_status == 1) {
        if ((core_firmware_transfer_context != 1 || transfer_direction != 0) && (core_firmware_transfer_context != 2 || transfer_direction != 0x80)) {
            if (core_state_050 == '\0') {
                if (core_descriptor_configuration_context != '\0') {
                    if (core_firmware_transfer_context == 3 || core_firmware_transfer_context == 4) {
                        core_firmware_transfer_context = 1;
                        core_state_050 = '\x01';
                        if (transfer_direction == 0) {
                            core_firmware_transfer_context = 2;
                            working_result = 0;
                            usb_ep0_buffer_pointer = (uint8_t *)&usb_ep0_buffer;
                            core_descriptor_component_context_snapshot = (uint8_t *)(uint32_t)core_descriptor_component_context;
                        } else {
                            usb_ep0_buffer_pointer = (uint8_t *)0x0;
                            working_result = 0x80;
                            core_descriptor_component_context_snapshot = usb_ep0_buffer_pointer;
                        }
                        core_update_mww_process_transfer(working_result, usb_ep0_buffer_pointer, core_descriptor_component_context_snapshot);
                    }
                    goto core_prepare_command_transfer_direction_before_process_wdt_reset;
                }
            } else if (((transfer_direction != 0) || (core_state_067 != 0)) && ((transfer_direction != 0x80 || (core_state_071 != 0))))
                goto core_prepare_command_transfer_direction_before_process_wdt_reset;
        }
        core_firmware_transfer_context = 5;
    }
/* Shared target for core prepare command transfer direction before process watchdog reset; incoming paths preserve the same state assumptions. */
core_prepare_command_transfer_direction_before_process_wdt_reset:
    if (core_firmware_transfer_context == 5) {
        core_process_wdt_reset(0);
        core_process_wdt_reset(0x80);
        mww_process_mww_transfer_storage(0);
    }
    return 0;
}

