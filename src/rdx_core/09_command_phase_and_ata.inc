/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
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
    /* Transition atapi device flag; persistent state is carried in core atapi device flag, usb bot cbw pointer and related record fields. */
    uint32_t working_result;

    if ((&core_atapi_device_flag)[(uint32_t)*(uint8_t *)(usb_bot_cbw_pointer + 0xd) * 0x1d0] != '\0') {
        return 1;
    }
    if (*(int *)(usb_bot_cbw_pointer + 8) == 0) {
        if (core_mww_saved_request == 1) {
            working_result = 2;
        } else {
            if (core_mww_saved_request != 2) {
                return 1;
            }
            working_result = 3;
        }
    } else if (*(int8_t *)(usb_bot_cbw_pointer + 0xc) == -0x80) {
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
    /* Process MWW saved request for MWW handle saved; persistent state is carried in core MWW saved request, usb ep3 out transfer active. */
    int operation_status;
    int flags_snapshot;

    flags_snapshot = flags;
    if (core_mww_saved_request == 2) {
        mww_update_mww_transfer_storage(3, &flags_snapshot);
        operation_status =
            (*(uint32_t *)(*(int *)(flags_snapshot + 0x18) + 8) & 0xffffff) + (*(int *)(flags_snapshot + 8) - *(int *)(flags_snapshot + 0xc));
        *(int *)(flags_snapshot + 8) = operation_status;
        if (operation_status < 1) {
            usb_ep3_out_transfer_active = 0;
        } else {
            core_end_usb_endpoint_transfer(3);
            if (condition != 0) {
                mww_process_mww_transfer_storage(3);
            }
        }
    }
    core_end_usb_endpoint_transfer(0x83);
    if ((condition != 0) && (core_mww_saved_request == 1)) {
        mww_process_mww_transfer_storage(0x83);
    }
    core_release_device_command();
    core_release_mww_device_command();
    return;
}

/**
 * @brief Transfer firmware context remaining firmware remaining context.
 *
 * @return No value.
 */
uint32_t core_transfer_firmware_context_remaining_firmware_remaining_context()

{
    /* Transfer firmware context remaining firmware remaining context; persistent state is carried in usb setup packet. */
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
                operation_status = (*data_cursor)(&usb_setup_packet);
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
    /* Update USB transfer state for USB state; persistent state is carried in usb ep0 max packet size by speed, usb device speed and related record fields. */
    uint32_t usb_ep0_max_packet_size_by_speed_pointer;

    if (usb_device_speed == 4) {
        usb_ep0_max_packet_size_by_speed_pointer = 0x200;
    } else {
        usb_ep0_max_packet_size_by_speed_pointer = (uint32_t)*(uint16_t *)(&usb_ep0_max_packet_size_by_speed + usb_device_speed * 2);
    }
    usb_ep0_out_max_packet_size = (uint16_t)usb_ep0_max_packet_size_by_speed_pointer;
    usb_ep0_in_max_packet_size = usb_ep0_out_max_packet_size;
    mww_send_usb_endpoint_command(0, 1, usb_ep0_max_packet_size_by_speed_pointer << 3, 0x500);
    if (condition != 0) {
        mww_send_usb_endpoint_command(0, 2, 1, 0);
    }
    mww_send_usb_endpoint_command(0x80, 1, usb_ep0_max_packet_size_by_speed_pointer << 3 | (context & 0x1f) << 0x11, 0x2000500);
    if (condition != 0) {
        mww_send_usb_endpoint_command(0x80, 2, 1, 0);
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
            core_append_little_endian_value(length, *(uint32_t *)(context + (uint32_t)record_offset), operation_status, &core_get_payload_offset_from_flags_result);
        } else if (buffer[2] == '\x01') {
            if ((uint8_t)buffer[3] < operation_status) {
                operation_status = (uint32_t)(uint8_t)buffer[3];
            }
            core_append_buffer_bytes(length, (uint32_t *)(context + (uint32_t)record_offset), operation_status, &core_get_payload_offset_from_flags_result);
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
 * @brief Append typed payload record.
 *
 * @param destination Procedure input.
 * @param destination_capacity Procedure input.
 * @param buffer Procedure input.
 * @param record_offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_append_typed_payload_record(destination, destination_capacity, buffer, record_offset)
int destination;
int destination_capacity;
uint32_t *buffer;
int record_offset;

{
    /* Append typed payload record using only caller-provided data. */
    uint16_t payload_length;
    int16_t record_offset_short;
    uint16_t record_bytes_available;
    int available_length;
    uint32_t output_length;

    if (destination_capacity < record_offset) {
        available_length = 0;
    } else {
        available_length = destination_capacity - record_offset;
    }
    payload_length = *(uint16_t *)(buffer + 2);
    record_offset_short = (int16_t)record_offset;
    if (available_length < (int)(payload_length + 4)) {
        if (record_offset <= destination_capacity) {
            record_bytes_available = (int16_t)destination_capacity - record_offset_short;
            goto core_record_append_check_complete_capacity;
        }
    } else {
        record_bytes_available = payload_length + 4;
    /* Shared target for core record append check complete capacity; incoming paths preserve the same state assumptions. */
    core_record_append_check_complete_capacity:
        output_length = (uint32_t)record_bytes_available;
        if (output_length - payload_length == 4) {
            core_write_big_endian_u32_field(destination, (int)record_offset_short, (int)record_offset_short, *buffer);
            core_write_big_endian_u32_field(destination, (int)(int16_t)(record_offset_short + 1), (int)(int16_t)(record_offset_short + 1), buffer[1]);
            core_write_big_endian_u32_field(destination, (int)(int16_t)(record_offset_short + 2), (int)(int16_t)(record_offset_short + 3), *(uint16_t *)(buffer + 2));
            core_copy_bytes(destination + record_offset + 4, buffer[3], output_length - 4);
            goto core_record_append_return_next_offset;
        }
    }
    output_length = 0;
/* Shared target for core record append return next offset; incoming paths preserve the same state assumptions. */
core_record_append_return_next_offset:
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
    /* Configure ATA devices address ATA device table; persistent state is carried in core ata true max lba high, core ahci port initialization state and related record fields. */
    uint32_t core_ata_true_max_lba_high_snapshot;
    bool core_ahci_port_initialization_state_snapshot;

    core_ata_true_max_lba_high_snapshot = core_ata_true_max_lba_high;
    core_ahci_port_initialization_state_snapshot = core_ahci_port_initialization_state == 0x10;
    if (core_ahci_port_initialization_state_snapshot) {
        buffer[2] = core_ata_true_max_lba_low;
        buffer[3] = core_ata_true_max_lba_high_snapshot;
        *buffer = 0x800f454;
        *(uint16_t *)(buffer + 4) = ata_true_logical_sector_size_bytes;
        *(uint8_t *)((int)buffer + 0x13) = core_ata_logical_per_physical_sector_exponent;
        *(uint8_t *)((int)buffer + 0x12) = core_ata_lba48_supported;
        *(uint16_t *)(buffer + 5) = core_ata_logical_sector_alignment;
        buffer[1] = ATA_DEVICES_ADDRESS;
        core_build_ata_identity_summary(value, &ata_identify_device_data, buffer + 2);
    }
    return core_ahci_port_initialization_state_snapshot;
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
    operation_status = core_get_notification_mode();
    if ((operation_status == 2) && (operation_status = core_is_cartridge_input_state_six(), operation_status != 0)) {
        core_update_reconnect_state();
    }
    core_cartridge_input_state = 0;
    core_reset_ahci_port_initialization_state();
    core_sense_selection_flags = 0;
    core_update_enabled(0x800f2b4, 0);
    core_update_store_context_enable(0);
    core_clear_ata_discovery_flags();
    core_process_cartridge_input_state();
    core_process_command_descriptor_context();
    core_process_primary_command_context();
    if (core_ata_configuration_loaded != '\0') {
        core_process_runtime_context_base_for_cartridge_input_state();
        core_ata_configuration_loaded = '\0';
    }
    core_clear_ata_discovery_flags();
    core_cartridge_link_retry_count = 0;
    core_cartridge_input_raw_level = 0;
    core_ata_standby_request_pending = 0;
    core_set_thermal_pwm_control_phase_zero();
    return;
}

/**
 * @brief Update USB transfer state for response buffer.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_update_usb_transfer_state_for_response_buffer()

{
    /* Update USB transfer state for response buffer; persistent state is carried in usb device speed, usb u1 enabled and related record fields. */
    int core_process_sci_read_logical_for_resource_usb_state_result;
    uint8_t condition_met;

    core_process_sci_read_logical_for_resource_usb_state_result = core_process_sci_read_logical_for_resource_usb_state(0);
    condition_met = core_process_sci_read_logical_for_resource_usb_state_result != 0;
    if (usb_device_speed == 3) {
        /* In USB state 3, expose each defined endpoint/transfer condition as a separate status bit. */
        if (usb_u1_enabled != '\0') {
            condition_met = condition_met | 4;
        }
        if (usb_u2_enabled != '\0') {
            condition_met = condition_met | 8;
        }
        if (usb_ltm_enabled != '\0') {
            condition_met = condition_met | 0x10;
        }
    } else if ((usb_remote_wakeup_capable != '\0') && (usb_remote_wakeup_enabled != '\0')) {
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
    /* Process secondary command context; persistent state is carried in core secondary descriptor state. */
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
                core_configure_component_for_secondary_resource_state_result = core_configure_component_for_secondary_resource_state(core_get_0xc_for_lookup_key_metadata_result & 0xff, buffer[3] + 4, &core_secondary_descriptor_state, status_bits, working_buffer);
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
        if ((operation_status == 0) && (operation_status = core_scsi_write_requests_fua_state_value(), operation_status == 0)) {
            /* With no pending notification or feature state, publish mode data and not-ready sense. */
            core_set_command_phase_flags(8);
            core_request_ata_standby_when_idle();
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
    int core_is_software_timer_expired_result;
    uint32_t operation_result;

    operation_result = 0;
    if (condition != 0) {
        return 0;
    }
    /* Gate the following state transition on ((char)buffer[4] != '\x01') || (*buffer == 0); the handler is skipped when this condition is false. */
    if (((char)buffer[4] != '\x01') || (*buffer == 0)) {
        buffer[2] = 1;
        goto core_cartridge_monitor_reset_followup_timers;
    }
    if (buffer[2] == 4) {
        core_is_software_timer_expired_result = core_is_software_timer_expired((int)buffer + 0xd);
        if (core_is_software_timer_expired_result != 0) {
            buffer[2] = 2;
            goto core_cartridge_monitor_arm_one_second_transition;
        }
        core_is_software_timer_expired_result = 5;
        *(uint8_t *)(buffer + 3) = 1;
    /* Shared target for core cartridge monitor commit transition phase; incoming paths preserve the same state assumptions. */
    core_cartridge_monitor_commit_transition_phase:
        buffer[2] = core_is_software_timer_expired_result;
    } else if (buffer[2] == 2) {
    /* Shared target for core cartridge monitor arm one second transition; incoming paths preserve the same state assumptions. */
    core_cartridge_monitor_arm_one_second_transition:
        rti_set_software_timer((int)buffer + 0xd, 1000);
        core_is_software_timer_expired_result = 3;
        goto core_cartridge_monitor_commit_transition_phase;
    }
    operation_result = 1;
/* Shared target for core cartridge monitor reset followup timers; incoming paths preserve the same state assumptions. */
core_cartridge_monitor_reset_followup_timers:
    rti_set_software_timer((int)buffer + 0xe, 5000);
    rti_set_software_timer((int)buffer + 0xf, 60000);
    return operation_result;
}

/**
 * @brief Handle USB BOT class request.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_usb_bot_class_request(buffer)
uint8_t *buffer;

{
    /* Handle USB BOT class request; persistent state is carried in usb ep0 state, USB ep0 buffer. */
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
    /* Handle USB BOT class request; this branch selects whether its associated handler runs. */
    } else if (((((*buffer & 0x80) == 0x80) && (buffer[1] == 0xfe)) && (*(int16_t *)(buffer + 2) == 0)) &&
               ((*(int16_t *)(buffer + 6) == 1 && (*(int16_t *)(buffer + 4) == 0)))) {
        usb_ep0_state = 1;
        usb_ep0_buffer = 0;
        /* The 0xfe record primes an EP0 firmware transfer with an 0x80-byte component. */
        /* Call core process component descriptor component context through the defined unprototyped function-pointer signature. */
        ((void (*)())core_process_component_descriptor_component_context)(0x80);
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Encode ATA taskfile lba.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_encode_ata_taskfile_lba(offset, buffer, length, condition) int offset;
uint32_t *buffer;
uint32_t length;
int condition;

{
    /* Encode ATA taskfile lba using only caller-provided data. */
    uint32_t status_bits;

    core_initialize_ata_taskfile();
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
 * @brief Select ATA transfer mode.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_select_ata_transfer_mode(context, condition)
int context;
int condition;

{
    /* Select ATA transfer mode using only caller-provided data. */
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
 * @brief Queue ATA d2h completion.
 *
 * @return No value.
 */
void core_queue_ata_d2h_completion()

{
    /* Queue ATA d2h completion; persistent state is carried in core ata callback write index, ATA received d2h status and related record fields. */
    int core_ata_callback_write_index_snapshot;
    uint8_t ata_received_d2h_status_snapshot;
    uint8_t working_result;
    int ahci_command_list_prd_byte_count_pointer;

    /* D2H register FIS completion is folded into the current 12-byte callback record. */
    ata_received_d2h_status_snapshot = ata_received_d2h_status;
    core_ata_callback_write_index_snapshot = core_ata_callback_write_index * 0xc;
    core_ata_device_fault_flag = (uint8_t)(((uint32_t)ata_received_d2h_status_snapshot << 0x1a) >> 0x1f);
    *(uint8_t *)(core_ata_callback_write_index_snapshot + ATA_CALLBACK_DATA_ADDRESS) = core_ahci_active_command_tag;
    /* Preserve the ATA error/status pair exactly as delivered by the device. */
    working_result = ata_received_d2h_error;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f522) = working_result;
    working_result = ata_received_d2h_status;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f521) = working_result;
    ahci_command_list_prd_byte_count_pointer = *(int *)(&ahci_command_list_prd_byte_count + (uint32_t)core_ahci_active_command_tag * 0x20);
    /* Report the completed PRD byte count from the command-list slot selected by the tag. */
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(int *)(core_ata_callback_write_index_snapshot + 0x800f524) = ahci_command_list_prd_byte_count_pointer;
    if (ahci_command_list_prd_byte_count_pointer != 0) {
        /* A non-empty data completion wakes the deferred transfer-processing path. */
        core_ata_data_completion_pending = 1;
    }
    core_enqueue_ata_completion_callback(ahci_command_completion_callback);
    return;
}

/**
 * @brief Queue ATA pio completion.
 *
 * @return No value.
 */
void core_queue_ata_pio_completion()

{
    /* Queue ATA pio completion; persistent state is carried in core ata callback write index, ATA received pio setup flags and related record fields. */
    int core_ata_callback_write_index_snapshot;
    uint8_t ata_received_pio_setup_flags_snapshot;
    uint16_t ata_received_pio_setup_transfer_count_snapshot;
    uint8_t working_result;

    /* PIO Setup FIS completion uses the same callback layout as D2H completion. */
    core_ata_callback_write_index_snapshot = core_ata_callback_write_index * 0xc;
    *(uint8_t *)(core_ata_callback_write_index_snapshot + ATA_CALLBACK_DATA_ADDRESS) = core_ahci_active_command_tag;
    working_result = ata_received_pio_setup_error;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f522) = working_result;
    working_result = ata_received_pio_setup_status;
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f521) = working_result;
    /* Snapshot core MMIO register 001 before decoding its status or capability fields. */
    ata_received_pio_setup_transfer_count_snapshot = ata_received_pio_setup_transfer_count;
    /* Store the device-requested PIO transfer count for the queued callback. */
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint32_t *)(core_ata_callback_write_index_snapshot + 0x800f524) = (uint32_t)ata_received_pio_setup_transfer_count_snapshot;
    ata_received_pio_setup_flags_snapshot = ata_received_pio_setup_flags;
    /* Translate the FIS direction bit into the callback record's high-bit marker. */
    if ((ata_received_pio_setup_flags_snapshot >> 5 & 1) == 0) {
        working_result = 0;
    } else {
        working_result = 0x80;
    }
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    *(uint8_t *)(core_ata_callback_write_index_snapshot + 0x800f528) = working_result;
    core_enqueue_ata_completion_callback(ahci_command_completion_callback);
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
    /* Call core encode ata taskfile lba through the defined unprototyped function-pointer signature. */
    ((void (*)())core_encode_ata_taskfile_lba)(data_cursor, &status_bits, 0, 0);
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
 * @brief Execute ATA security unlock.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_execute_ata_security_unlock(context) uint32_t context;

{
    /* Execute ATA security unlock; persistent state is carried in core ata security unlock succeeded. */
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
            core_ata_security_unlock_succeeded = 1;
            /* Only this defined RDX-specific unlock success authenticates the disk. */
            rdx_sata_usb_authenticate();
        }
        core_clear_request_and_release_device_command(working_buffer);
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
    uint8_t *serial_cursor;
    uint8_t *signature_cursor;
    uint64_t core_get_unit_serial_number_result;
    uint8_t status_bits;
    uint8_t response_byte;
    volatile uint8_t data_cursor[9];
    uint8_t signature_prefix_byte;
    volatile uint8_t signature_bytes[8];

    signature_bytes[0] = 0x55;
    signature_bytes[1] = 0xaa;
    signature_bytes[2] = 0x25;
    signature_bytes[3] = 0x46;
    signature_bytes[4] = 4;
    signature_bytes[5] = 0xc1;
    signature_bytes[6] = 0x1d;
    signature_bytes[7] = 0xb7;
    core_get_unit_serial_number_result = (uint64_t)core_get_unit_serial_number();
    core_copy_bytes(&status_bits, (int)core_get_unit_serial_number_result, 10);
    operation_result = 0;
    /* Gate the following state transition on ((0x9ffffffff < core_get_unit_serial_number_result) && (*buffer == '\0')) && (buffer[1] == '\0'); the handler is skipped when this condition is false. */
    if (((0x9ffffffff < core_get_unit_serial_number_result) && (*buffer == '\0')) && (buffer[1] == '\0')) {
        serial_cursor = &response_byte;
        buffer_cursor = (uint8_t *)(buffer + 1);
        operation_status = 8;
        signature_cursor = &signature_prefix_byte;
        operation_result = 1;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            serial_cursor = serial_cursor + 1;
            signature_cursor = signature_cursor + 1;
            buffer_cursor = buffer_cursor + 1;
            /* Gate the following state transition on *buffer_cursor != (*signature_cursor ^ *serial_cursor); the handler is skipped when this condition is false. */
            if (*buffer_cursor != (*signature_cursor ^ *serial_cursor)) {
                return 0;
            }
            operation_status = operation_status + -1;
        } while (operation_status != 0);
    }
    return operation_result;
}
