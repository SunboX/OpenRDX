/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Transition atapi device flag.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_transition_atapi_device_flag()

{
    /* Transition atapi device flag; persistent state is carried in core atapi device flag, MWW source descriptor and related record fields. */
    uint32_t working_result;

    if ((&core_atapi_device_flag)[(uint32_t)*(uint8_t *)(mww_source_descriptor + 0xd) * 0x1d0] != '\0') {
        return 1;
    }
    if (*(int *)(mww_source_descriptor + 8) == 0) {
        if (core_mww_saved_request == 1) {
            working_result = 2;
        } else {
            if (core_mww_saved_request != 2) {
                return 1;
            }
            working_result = 3;
        }
    } else if (*(int8_t *)(mww_source_descriptor + 0xc) == -0x80) {
        if (core_mww_saved_request == 1) {
            return 1;
        }
        if (core_mww_saved_request == 2) {
            working_result = 8;
        } else {
            working_result = 4;
        }
    } else {
        if (core_mww_saved_request == 2) {
            return 1;
        }
        if (core_mww_saved_request == 1) {
            working_result = 10;
        } else {
            working_result = 9;
        }
    }
    mww_transition_selection_state(working_result);
    return 0;
}

/**
 * @brief Process MWW saved request for MWW handle saved.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_process_mww_saved_request_for_mww_handle_saved(condition, value, length, flags) int condition;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process MWW saved request for MWW handle saved; persistent state is carried in core MWW saved request, core state 068. */
    int operation_status;
    int flags_snapshot;

    flags_snapshot = flags;
    if (core_mww_saved_request == 2) {
        mww_update_mww_transfer_storage(3, &flags_snapshot);
        operation_status =
            (*(uint32_t *)(*(int *)(flags_snapshot + 0x18) + 8) & 0xffffff) + (*(int *)(flags_snapshot + 8) - *(int *)(flags_snapshot + 0xc));
        *(int *)(flags_snapshot + 8) = operation_status;
        if (operation_status < 1) {
            core_state_068 = 0;
        } else {
            core_process_wdt_reset(3);
            if (condition != 0) {
                mww_process_mww_transfer_storage(3);
            }
        }
    }
    core_process_wdt_reset(0x83);
    if ((condition != 0) && (core_mww_saved_request == 1)) {
        mww_process_mww_transfer_storage(0x83);
    }
    core_set_thermal_monitor_enabled_to_zero();
    core_update_mww_transfer_context_for_mww_process_resource();
    return;
}

/**
 * @brief Transfer firmware context remaining firmware remaining context.
 *
 * @return No value.
 */
uint32_t core_transfer_firmware_context_remaining_firmware_remaining_context()

{
    /* Transfer firmware context remaining firmware remaining context; persistent state is carried in core firmware remaining context. */
    int operation_status;
    uint32_t remaining_count;
    firmware_callback_t *data_cursor;

    remaining_count = 0;
    operation_status = core_dispatch_response_buffer(0);
    if (operation_status != 0) {
        /* Iterate while ; ((operation_status != 0 && (remaining_count < 4), preserving the defined cursor and bound. */
        for (; ((operation_status != 0 && (remaining_count < 4)) &&
                /* Invoke the callback selected by the active queue entry. */
                (data_cursor = *(firmware_callback_t **)(remaining_count * 4 + 0x800f6d0),
                data_cursor != (firmware_callback_t *)0x0));
             remaining_count = remaining_count + 1) {
            /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
            if (*(char *)(remaining_count + 0x800f6e0) == '\0') {
                /* Notify data cursor only after the associated state fields have been committed. */
                operation_status = (*data_cursor)(&core_firmware_remaining_context);
            }
        }
    }
    return 0;
}

/**
 * @brief Update USB transfer state for USB state.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_update_usb_transfer_state_for_usb_state(context, condition) uint32_t context;
int condition;

{
    /* Update USB transfer state for USB state; persistent state is carried in core state 032, USB device state and related record fields. */
    uint32_t core_state_032_pointer;

    if (usb_device_state == 4) {
        core_state_032_pointer = 0x200;
    } else {
        core_state_032_pointer = (uint32_t)*(uint16_t *)(&core_state_032 + usb_device_state * 2);
    }
    core_descriptor_component_context = (uint16_t)core_state_032_pointer;
    core_state_070 = core_descriptor_component_context;
    mww_process_command_word_channel_mww_command_channel(0, 1, core_state_032_pointer << 3, 0x500);
    if (condition != 0) {
        mww_process_command_word_channel_mww_command_channel(0, 2, 1, 0);
    }
    mww_process_command_word_channel_mww_command_channel(0x80, 1, core_state_032_pointer << 3 | (context & 0x1f) << 0x11, 0x2000500);
    if (condition != 0) {
        mww_process_command_word_channel_mww_command_channel(0x80, 2, 1, 0);
    }
    return;
}

/**
 * @brief Process x01 for comparison comparison resource.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
int core_process_x01_for_comparison_comparison_resource(context, buffer, length)
int context;
uint8_t *buffer;
uint32_t length;

{
    /* Process x01 for comparison comparison resource using only caller-provided data. */
    uint16_t record_offset;
    int operation_result;
    uint32_t operation_status;
    uint32_t core_get_payload_offset_from_flags_result;
    uint8_t working_buffer[24];

    /* Process x01 for comparison comparison resource; this branch selects whether its associated handler runs. */
    operation_result = core_run_expected_flag_expected_type(*buffer, 0, working_buffer, length);
    if (operation_result == 0) {
        record_offset = *(uint16_t *)(buffer + 6);
        core_get_payload_offset_from_flags_result = core_get_payload_offset_from_flags(working_buffer);
        operation_status = core_get_remaining_descriptor_space(working_buffer);
        if (buffer[2] == '\0') {
            if (3 < operation_status) {
                operation_status = 4;
            }
            core_transfer_tracker(length, *(uint32_t *)(context + (uint32_t)record_offset), operation_status, &core_get_payload_offset_from_flags_result);
        } else if (buffer[2] == '\x01') {
            if ((uint8_t)buffer[3] < operation_status) {
                operation_status = (uint32_t)(uint8_t)buffer[3];
            }
            core_transfer_bytes_remaining_descriptor(length, (uint32_t *)(context + (uint32_t)record_offset), operation_status, &core_get_payload_offset_from_flags_result);
        } else {
            operation_result = 0xb;
        }
    }
    return operation_result;
}

/**
 * @brief Update validation 0x20.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
int core_update_validation_0x20(offset)
int offset;

{
    /* Update validation 0x20 using only caller-provided data. */
    int core_run_target_type_clear_memory_target_type_context_result;
    int validation_status;
    int offset_field_0x20;
    uint8_t working_buffer[4];
    int status_bits = 0;
    int working_result = 0;
    int fallback_result = 0;

    offset_field_0x20 = *(int *)(offset + 0x20);
    if (*(int *)(offset_field_0x20 + 0x68) == 0) {
        validation_status = core_run_target_type_clear_memory_target_type_context(offset, 2, working_buffer);
        if (validation_status != 0) {
            return validation_status;
        }
        validation_status = 0;
    } else {
        core_run_target_type_clear_memory_target_type_context_result = core_run_target_type_clear_memory_target_type_context(offset, 0x1b, working_buffer);
        validation_status = working_result;
        working_result = fallback_result;
        if (core_run_target_type_clear_memory_target_type_context_result != 0) {
            return core_run_target_type_clear_memory_target_type_context_result;
        }
    }
    if ((*(char *)(offset_field_0x20 + 0x6d) != '\0') && (validation_status != 0 || status_bits != 0)) {
        status_bits = 0;
    }
    *(int *)(offset_field_0x20 + 0x4c) = status_bits;
    *(int *)(offset_field_0x20 + 0x50) = *(int *)(offset_field_0x20 + 100) - working_result;
    return 0;
}

/**
 * @brief Process short available.
 *
 * @param destination Procedure input.
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @param record_offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_short_available(destination, condition, buffer, record_offset)
int destination;
int condition;
uint32_t *buffer;
int record_offset;

{
    /* Process short available using only caller-provided data. */
    uint16_t operation_status;
    int16_t record_offset_short;
    uint16_t condition_field;
    int available_length;
    uint32_t output_length;

    if (condition < record_offset) {
        available_length = 0;
    } else {
        available_length = condition - record_offset;
    }
    operation_status = *(uint16_t *)(buffer + 2);
    record_offset_short = (int16_t)record_offset;
    if (available_length < (int)(operation_status + 4)) {
        if (record_offset <= condition) {
            condition_field = (int16_t)condition - record_offset_short;
            goto core_process_short_available_before_transfer_range_end_range_start_for_short_available;
        }
    } else {
        condition_field = operation_status + 4;
    /* Shared target for core process short available before transfer range end range start for short available; incoming paths preserve the same state assumptions. */
    core_process_short_available_before_transfer_range_end_range_start_for_short_available:
        output_length = (uint32_t)condition_field;
        if (output_length - operation_status == 4) {
            core_transfer_range_end_range_start_for_short_available(destination, (int)record_offset_short, (int)record_offset_short, *buffer);
            core_transfer_range_end_range_start_for_short_available(destination, (int)(int16_t)(record_offset_short + 1), (int)(int16_t)(record_offset_short + 1), buffer[1]);
            core_transfer_range_end_range_start_for_short_available(destination, (int)(int16_t)(record_offset_short + 2), (int)(int16_t)(record_offset_short + 3), *(uint16_t *)(buffer + 2));
            core_forward_end_remaining(destination + record_offset + 4, buffer[3], output_length - 4);
            goto core_process_short_available_return_path;
        }
    }
    output_length = 0;
/* Shared target for core process short available return path; incoming paths preserve the same state assumptions. */
core_process_short_available_return_path:
    return output_length + record_offset & 0xffff;
}

/**
 * @brief Configure ATA devices address ATA device table.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
bool core_configure_ata_devices_address_ata_device_table(buffer, value) uint32_t *buffer;
uint32_t value;

{
    /* Configure ATA devices address ATA device table; persistent state is carried in core ATA device table, core command runtime state and related record fields. */
    uint32_t core_ata_device_table_snapshot;
    bool core_command_runtime_state_snapshot;

    core_ata_device_table_snapshot = core_ata_device_table;
    core_command_runtime_state_snapshot = core_command_runtime_state == 0x10;
    if (core_command_runtime_state_snapshot) {
        buffer[2] = core_state_221;
        buffer[3] = core_ata_device_table_snapshot;
        *buffer = 0x800f454;
        *(uint16_t *)(buffer + 4) = mww_state_005;
        *(uint8_t *)((int)buffer + 0x13) = core_state_183;
        *(uint8_t *)((int)buffer + 0x12) = core_state_181;
        *(uint16_t *)(buffer + 5) = core_state_184;
        buffer[1] = ATA_DEVICES_ADDRESS;
        core_process_header_word_header_words(value, &ata_identify_device_data, buffer + 2);
    }
    return core_command_runtime_state_snapshot;
}

/**
 * @brief Handle cartridge input state.
 *
 * @return No value.
 */
void core_handle_cartridge_input_state()

{
    /* Handle cartridge input state; persistent state is carried in core cartridge input state, core sense selection flags and related record fields. */
    int operation_status;

    /* Removal/reset invalidates the previous cartridge authentication. */
    rdx_sata_usb_reject();
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
 * @brief Update USB transfer state for response buffer.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_update_usb_transfer_state_for_response_buffer()

{
    /* Update USB transfer state for response buffer; persistent state is carried in USB device state, core state 062 and related record fields. */
    int core_process_sci_read_logical_for_resource_usb_state_result;
    uint8_t condition_met;

    core_process_sci_read_logical_for_resource_usb_state_result = core_process_sci_read_logical_for_resource_usb_state(0);
    condition_met = core_process_sci_read_logical_for_resource_usb_state_result != 0;
    if (usb_device_state == 3) {
        /* In USB state 3, expose each defined endpoint/transfer condition as a separate status bit. */
        if (core_state_062 != '\0') {
            condition_met = condition_met | 4;
        }
        if (core_state_063 != '\0') {
            condition_met = condition_met | 8;
        }
        if (core_state_064 != '\0') {
            condition_met = condition_met | 0x10;
        }
    } else if ((core_state_056 != '\0') && (core_state_057 != '\0')) {
        /* Outside state 3, bit 1 represents the combined capability-and-active condition. */
        condition_met = condition_met | 2;
    }
    /* Assemble the one-byte response only after all contributing flags have been sampled. */
    *core_response_buffer = condition_met;
    return 0;
}

/**
 * @brief Process secondary command context.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_secondary_command_context(context, value, buffer)
uint32_t context;
uint32_t value;
int *buffer;

{
    /* Process secondary command context; persistent state is carried in core secondary command context. */
    uint32_t core_get_0xc_for_lookup_key_metadata_result;
    int core_handle_metadata_type_result;
    int secondary_result = 0;
    uint32_t status_bits;
    uint32_t core_configure_component_for_secondary_resource_state_result;
    uint8_t working_buffer[44];

    core_configure_component_for_secondary_resource_state_result = 0x28;
    core_get_0xc_for_lookup_key_metadata_result = core_get_0xc_for_lookup_key_metadata(buffer, value, buffer, value);
    if ((int)core_get_0xc_for_lookup_key_metadata_result < 9) {
        /* Gate the following state transition on (secondary_result == 2) && (*buffer == 0xc4); the handler is skipped when this condition is false. */
        if ((secondary_result == 2) && (*buffer == 0xc4)) {
            core_handle_metadata_type_result = core_handle_metadata_type(buffer, 0x44, core_get_0xc_for_lookup_key_metadata_result & 0xff, working_buffer, &status_bits, &core_configure_component_for_secondary_resource_state_result);
            if (core_handle_metadata_type_result != 0) {
                core_configure_component_for_secondary_resource_state_result = core_configure_component_for_secondary_resource_state(core_get_0xc_for_lookup_key_metadata_result & 0xff, buffer[3] + 4, &core_secondary_command_context, status_bits, working_buffer);
                core_reset_output_record(working_buffer);
            }
        }
    } else {
        core_configure_component_for_secondary_resource_state_result = 0x29;
    }
    return core_configure_component_for_secondary_resource_state_result;
}

/**
 * @brief Handle command phase.
 *
 * @return No value.
 */
void core_handle_command_phase()

{
    /* Handle command phase; persistent state is carried in core command phase, core command buffer state. */
    int operation_status;

    if (core_command_phase == 0) {
        /* Phase 0 waits for either a usable cartridge or an input/state transition. */
        operation_status = core_is_cartridge_input_state_six();
        if ((operation_status == 0) && (operation_status = core_check_logical_input_and_state(), operation_status == 0)) {
            return;
        }
        operation_status = core_check_idle_input_and_state();
        if (operation_status != 0) {
            /* An idle-input conflict is surfaced immediately through the secondary sense record. */
            core_mark_record_active_with_code_eight(0x800f29c);
            return;
        }
    } else if (1 < core_command_phase - 1U) {
        /* Phases 1 and 2 share the standby tail; only phase 3 has notification work here. */
        if (core_command_phase != 3) {
            return;
        }
        operation_status = core_get_notification_state();
        if ((operation_status == 0) && (operation_status = core_get_command_feature_state_value(), operation_status == 0)) {
            /* With no pending notification or feature state, publish mode data and not-ready sense. */
            core_update_command_phase_flags_for_command_phase(8);
            core_process_mode_processing_context();
            core_mark_secondary_sense_record_not_ready(0x800f29c);
            return;
        }
        core_mark_record_active_with_code_eight(0x800f29c);
        core_command_buffer_state = 1;
        return;
    }
    /* The common phase-1/phase-2 tail advances pending ATA standby processing. */
    core_handle_ata_standby_context_for_cartridge_monitor_context();
    return;
}

/**
 * @brief Process x01 for advance mechanism state.
 *
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_x01_for_advance_mechanism_state(condition, buffer)
int condition;
int *buffer;

{
    /* Process x01 for advance mechanism state using only caller-provided data. */
    int core_get_thermal_control_context_result;
    uint32_t operation_result;

    operation_result = 0;
    if (condition != 0) {
        return 0;
    }
    /* Gate the following state transition on ((char)buffer[4] != '\x01') || (*buffer == 0); the handler is skipped when this condition is false. */
    if (((char)buffer[4] != '\x01') || (*buffer == 0)) {
        buffer[2] = 1;
        goto core_branch_target_107;
    }
    if (buffer[2] == 4) {
        core_get_thermal_control_context_result = core_get_thermal_control_context((int)buffer + 0xd);
        if (core_get_thermal_control_context_result != 0) {
            buffer[2] = 2;
            goto core_branch_target_105;
        }
        core_get_thermal_control_context_result = 5;
        *(uint8_t *)(buffer + 3) = 1;
    /* Shared target for core branch target 106; incoming paths preserve the same state assumptions. */
    core_branch_target_106:
        buffer[2] = core_get_thermal_control_context_result;
    } else if (buffer[2] == 2) {
    /* Shared target for core branch target 105; incoming paths preserve the same state assumptions. */
    core_branch_target_105:
        rti_process_thermal_timer_state((int)buffer + 0xd, 1000);
        core_get_thermal_control_context_result = 3;
        goto core_branch_target_106;
    }
    operation_result = 1;
/* Shared target for core branch target 107; incoming paths preserve the same state assumptions. */
core_branch_target_107:
    rti_process_thermal_timer_state((int)buffer + 0xe, 5000);
    rti_process_thermal_timer_state((int)buffer + 0xf, 60000);
    return operation_result;
}

/**
 * @brief Handle special control record.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_special_control_record(buffer)
uint8_t *buffer;

{
    /* Handle special control record; persistent state is carried in core firmware transfer context, USB ep0 buffer. */
    uint32_t operation_result;

    operation_result = 1;
    /* Two exact eight-byte control records enter the private MWW/firmware-transfer paths. */
    /* Gate the following state transition on (*buffer & 0x80) == 0; the handler is skipped when this condition is false. */
    if ((*buffer & 0x80) == 0) {
        if (((buffer[1] == 0xff) && (*(int16_t *)(buffer + 2) == 0)) &&
            ((*(int16_t *)(buffer + 6) == 0 && (*(int16_t *)(buffer + 4) == 0)))) {
            core_process_mww_command_state();
            /* Zero means this vendor-specific record was consumed locally. */
            operation_result = 0;
        }
    /* Handle special control record; this branch selects whether its associated handler runs. */
    } else if (((((*buffer & 0x80) == 0x80) && (buffer[1] == 0xfe)) && (*(int16_t *)(buffer + 2) == 0)) &&
               ((*(int16_t *)(buffer + 6) == 1 && (*(int16_t *)(buffer + 4) == 0)))) {
        core_firmware_transfer_context = 1;
        usb_ep0_buffer = 0;
        /* The 0xfe record primes an EP0 firmware transfer with an 0x80-byte component. */
        /* Call core process component descriptor component context through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_component_descriptor_component_context)(0x80);
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Configure ti memset.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_configure_ti_memset(offset, buffer, length, condition) int offset;
uint32_t *buffer;
uint32_t length;
int condition;

{
    /* Configure ti memset using only caller-provided data. */
    uint32_t status_bits;

    core_update_ti_memset_for_ti_memset();
    *(uint8_t *)(offset + 3) = *(uint8_t *)buffer;
    *(char *)(offset + 5) = (char)((uint32_t)*buffer >> 8);
    status_bits = *buffer;
    *(char *)(offset + 1) = (char)length;
    *(char *)(offset + 7) = (char)((uint32_t)status_bits >> 0x10);
    if (condition == 0) {
        *(uint8_t *)(offset + 9) = (uint8_t)((uint32_t)*buffer >> 0x18) & 0xf | *(uint8_t *)(offset + 9);
        return;
    }
    *(char *)(offset + 4) = (char)((uint32_t)*buffer >> 0x18);
    *(char *)(offset + 6) = (char)buffer[1];
    status_bits = buffer[1];
    *(char *)(offset + 2) = (char)((uint32_t)length >> 8);
    *(char *)(offset + 8) = (char)((uint32_t)status_bits >> 8);
    return;
}

/**
 * @brief Transfer device feature mask feature mask.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_transfer_device_feature_mask_feature_mask(context, condition)
int context;
int condition;

{
    /* Transfer device feature mask feature mask using only caller-provided data. */
    uint16_t context_derived;
    uint8_t condition_met;
    uint16_t *feature_mask_pointer;
    uint32_t device_feature_mask;

    condition_met = 0;
    if (condition == 0) {
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        context_derived = *(uint16_t *)(context * 0x1d0 + 0x800f482);
        feature_mask_pointer = (uint16_t *)(uint32_t)context_derived;
        if ((context_derived >> 1 & 1) == 0) {
            feature_mask_pointer = (uint16_t *)(uint32_t)(context_derived >> 1);
            if ((context_derived & 1) != 0) {
                condition_met = 0xb;
            }
        } else {
            condition_met = 0xc;
        }
    } else {
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        feature_mask_pointer = (uint16_t *)(context * 0x1d0 + 0x800f47e);
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        device_feature_mask = (uint32_t)*(uint16_t *)(context * 0x1d0 + 0x800f480);
        if (device_feature_mask == 0) {
            context_derived = *feature_mask_pointer;
            condition_met = 0;
            if (context_derived != 0) {
                /* Iterate while condition_met = 2; feature_mask_pointer = (uint16_t *)(1 << (uint32_t)condition_met, preserving the defined cursor and bound. */
                for (condition_met = 2; feature_mask_pointer = (uint16_t *)(1 << (uint32_t)condition_met),
                    ((uint32_t)context_derived & (uint32_t)feature_mask_pointer) == 0;
                     condition_met = condition_met - 1) {
                }
                condition_met = condition_met + 0x20;
            }
        } else {
            /* Iterate over the bounded defined table while preserving cursor order. */
            for (condition_met = 6;
                 feature_mask_pointer = (uint16_t *)(1 << (uint32_t)condition_met), (device_feature_mask & (uint32_t)feature_mask_pointer) == 0;
                 condition_met = condition_met - 1) {
            }
            condition_met = condition_met + 0x40;
        }
    }
    return CONCAT44(feature_mask_pointer, (uint32_t)condition_met);
}

/**
 * @brief Configure ATA address ATA configuration address.
 *
 * @return No value.
 */
void core_configure_ata_address_ata_configuration_address()

{
    /* Configure ATA address ATA configuration address; persistent state is carried in core ATA queue context, ATA received d2h status and related record fields. */
    int core_ata_queue_context_snapshot;
    uint8_t ata_received_d2h_status_snapshot;
    uint8_t working_result;
    int ahci_command_list_prd_byte_count_pointer;

    /* D2H register FIS completion is folded into the current 12-byte callback record. */
    ata_received_d2h_status_snapshot = ata_received_d2h_status;
    core_ata_queue_context_snapshot = core_ata_queue_context * 0xc;
    core_command_descriptor_device = (uint8_t)(((uint32_t)ata_received_d2h_status_snapshot << 0x1a) >> 0x1f);
    *(uint8_t *)(core_ata_queue_context_snapshot + ATA_CALLBACK_DATA_ADDRESS) = core_state_185;
    /* Preserve the ATA error/status pair exactly as delivered by the device. */
    working_result = ata_received_d2h_error;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f522) = working_result;
    working_result = ata_received_d2h_status;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f521) = working_result;
    ahci_command_list_prd_byte_count_pointer = *(int *)(&ahci_command_list_prd_byte_count + (uint32_t)core_state_185 * 0x20);
    /* Report the completed PRD byte count from the command-list slot selected by the tag. */
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(int *)(core_ata_queue_context_snapshot + 0x800f524) = ahci_command_list_prd_byte_count_pointer;
    if (ahci_command_list_prd_byte_count_pointer != 0) {
        /* A non-empty data completion wakes the deferred transfer-processing path. */
        core_state_220 = 1;
    }
    core_update_ata_queue_address_ata_pending_ata_queue_context(ahci_command_completion_context);
    return;
}

/**
 * @brief Configure ATA address ATA queue context.
 *
 * @return No value.
 */
void core_configure_ata_address_ata_queue_context()

{
    /* Configure ATA address ATA queue context; persistent state is carried in core ATA queue context, ATA received pio setup flags and related record fields. */
    int core_ata_queue_context_snapshot;
    uint8_t ata_received_pio_setup_flags_snapshot;
    uint16_t ata_received_pio_setup_transfer_count_snapshot;
    uint8_t working_result;

    /* PIO Setup FIS completion uses the same callback layout as D2H completion. */
    core_ata_queue_context_snapshot = core_ata_queue_context * 0xc;
    *(uint8_t *)(core_ata_queue_context_snapshot + ATA_CALLBACK_DATA_ADDRESS) = core_state_185;
    working_result = ata_received_pio_setup_error;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f522) = working_result;
    working_result = ata_received_pio_setup_status;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f521) = working_result;
    /* Snapshot core MMIO register 001 before decoding its status or capability fields. */
    ata_received_pio_setup_transfer_count_snapshot = ata_received_pio_setup_transfer_count;
    /* Store the device-requested PIO transfer count for the queued callback. */
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint32_t *)(core_ata_queue_context_snapshot + 0x800f524) = (uint32_t)ata_received_pio_setup_transfer_count_snapshot;
    ata_received_pio_setup_flags_snapshot = ata_received_pio_setup_flags;
    /* Translate the FIS direction bit into the callback record's high-bit marker. */
    if ((ata_received_pio_setup_flags_snapshot >> 5 & 1) == 0) {
        working_result = 0;
    } else {
        working_result = 0x80;
    }
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_queue_context_snapshot + 0x800f528) = working_result;
    core_update_ata_queue_address_ata_pending_ata_queue_context(ahci_command_completion_context);
    return;
}

/**
 * @brief Build ATA smart command.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
uint32_t core_build_ata_smart_command(condition, value, buffer)
int condition;
uint32_t value;
uint8_t *buffer;

{
    /* Build ATA smart command using only caller-provided data. */
    int ahci_execute_taskfile_command_result;
    uint8_t working_result;
    uint32_t status_bits;
    volatile uint32_t reserved_word = 0;
    uint8_t data_cursor[10];
    volatile uint8_t response_opcode = 0;
    uint8_t working_buffer[5];
    char response_status_byte = 0;
    char operation_status = 0;

    /* ATA SMART commands require the 0x4f/0xc2 signature in the taskfile LBA bytes. */
    status_bits = 0xc24f00;
    reserved_word = 0;
    /* Call core configure ti memset through the defined unprototyped function-pointer signature. */
    ((void (*)())core_configure_ti_memset)(data_cursor, &status_bits, 0, 0);
    response_opcode = 0xb0;
    /* The caller supplies the SMART feature subcommand, such as 0xd0 for READ DATA. */
    data_cursor[0] = (uint8_t)condition;
    ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(data_cursor, value, 1, working_buffer);
    if (((ahci_execute_taskfile_command_result != 0) && (condition == 0xda)) && (buffer != (uint8_t *)0x0)) {
        /* SMART RETURN STATUS optionally reports whether the device returned the threshold signature. */
        if ((operation_status == ',') && ((int8_t)response_status_byte == -0xc)) {
            working_result = 1;
        } else {
            working_result = 0;
        }
        *buffer = working_result;
    }
    return 0;
}

/**
 * @brief Handle ATA command opcode ATA command context.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_handle_ata_command_opcode_ata_command_context(context) uint32_t context;

{
    /* Handle ATA command opcode ATA command context; persistent state is carried in core ATA command context. */
    int operation_status;
    uint8_t working_buffer[4];
    volatile uint16_t working_result = 0;
    uint8_t device_state[40];
    uint8_t ata_command[10];
    volatile uint8_t command_opcode = 0;

    /* Re-authentication starts fail-closed and temporarily removes any old LUN. */
    rdx_sata_usb_reject();
    core_submit_ata_command_context(device_state, context);
    operation_status = core_update_capacity_block_context(working_buffer);
    if (operation_status != 0) {
        core_update_ti_memset_for_ata_command_command(working_buffer);
        core_process_copy(working_buffer, 1, device_state, 0x14);
        ti_memset(ata_command, 0, 0xb);
        command_opcode = 0xf2;
        working_result = 0x200;
        operation_status = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        if (operation_status != 0) {
            core_ata_command_context = 1;
            /* Only this defined RDX-specific unlock success authenticates the disk. */
            rdx_sata_usb_authenticate();
        }
        core_update_thermal_monitor_enabled_for_read_smart_temperature(working_buffer);
    }
    return;
}

/**
 * @brief Transfer response byte stack response byte.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transfer_response_byte_stack_response_byte(buffer)
char *buffer;

{
    /* Transfer response byte stack response byte using only caller-provided data. */
    uint32_t operation_result;
    uint8_t *buffer_cursor;
    int operation_status;
    uint8_t *bstack_27_pointer;
    uint8_t *bstack_1d_pointer;
    uint64_t core_get_response_byte_context_result;
    uint8_t status_bits;
    uint8_t response_byte;
    volatile uint8_t data_cursor[9];
    uint8_t bStack_1d;
    volatile uint8_t signature_bytes[8];

    signature_bytes[0] = 0x55;
    signature_bytes[1] = 0xaa;
    signature_bytes[2] = 0x25;
    signature_bytes[3] = 0x46;
    signature_bytes[4] = 4;
    signature_bytes[5] = 0xc1;
    signature_bytes[6] = 0x1d;
    signature_bytes[7] = 0xb7;
    core_get_response_byte_context_result = (uint64_t)core_get_response_byte_context();
    core_forward_end_remaining(&status_bits, (int)core_get_response_byte_context_result, 10);
    operation_result = 0;
    /* Gate the following state transition on ((0x9ffffffff < core_get_response_byte_context_result) && (*buffer == '\0')) && (buffer[1] == '\0'); the handler is skipped when this condition is false. */
    if (((0x9ffffffff < core_get_response_byte_context_result) && (*buffer == '\0')) && (buffer[1] == '\0')) {
        bstack_27_pointer = &response_byte;
        buffer_cursor = (uint8_t *)(buffer + 1);
        operation_status = 8;
        bstack_1d_pointer = &bStack_1d;
        operation_result = 1;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            bstack_27_pointer = bstack_27_pointer + 1;
            bstack_1d_pointer = bstack_1d_pointer + 1;
            buffer_cursor = buffer_cursor + 1;
            /* Gate the following state transition on *buffer_cursor != (*bstack_1d_pointer ^ *bstack_27_pointer); the handler is skipped when this condition is false. */
            if (*buffer_cursor != (*bstack_1d_pointer ^ *bstack_27_pointer)) {
                return 0;
            }
            operation_status = operation_status + -1;
        } while (operation_status != 0);
    }
    return operation_result;
}

