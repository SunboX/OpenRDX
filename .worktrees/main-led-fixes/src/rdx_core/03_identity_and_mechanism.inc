/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Initialize RDX identity records.
 *
 * @return No value.
 */
void core_initialize_rdx_identity_records()

{
    /* Initialize RDX identity records; persistent state is carried in core secondary response buffer, core state 088 and related record fields. */
    int operation_status;
    uint8_t *core_secondary_response_buffer_pointer;
    volatile uint8_t *volatile core_state_088_pointer = 0;
    volatile uint8_t *volatile core_unit_serial_number_pointer = 0;
    volatile uint16_t core_state_085_snapshot = 0;
    volatile uint16_t core_state_086_snapshot = 0;
    volatile uint8_t working_result = 0;
    volatile uint8_t response_code = 0;

    operation_status = core_read_unit_identity_record(&core_unit_identity_record);
    if ((operation_status != 0) || (operation_status = core_calculate_record_checksum(&core_unit_identity_record, 0x100), core_unit_identity_record != operation_status)) {
        ti_memset(&core_unit_identity_record, 0, 0x100);
        ti_memset(&core_unit_identity_record, 0, 0x100);
        core_forward_end_remaining(&core_state_081, &core_state_013, 0x10);
        core_forward_end_remaining(&core_state_080, 0x800e758, 8);
        core_forward_end_remaining(&core_unit_serial_number, "7360242889", 10);
        core_forward_end_remaining(&core_state_091, 0x800e750, 8);
        core_forward_end_remaining(&core_secondary_response_buffer, &core_branch_target_120, 8);
        core_forward_end_remaining(&core_state_088, &core_state_014, 0x10);
        core_flash_validation_context = 0x38;
        core_state_085 = 0x1a5a;
        core_state_082 = 0;
        core_state_086 = 6;
        core_primary_response_buffer = 1;
        core_forward_end_remaining(&core_state_093, &core_state_015, 0x10);
        core_forward_end_remaining(&core_state_094, &core_state_016, 0x10);
        core_forward_end_remaining(&core_state_095, &core_state_017, 0x10);
        core_state_078 = 6;
    }
    operation_status = core_get_flash_validation_context_for_validate_structured_flash();
    core_state_088_pointer = (volatile uint8_t *)&core_state_088;
    core_unit_serial_number_pointer = (volatile uint8_t *)&core_unit_serial_number;
    core_secondary_response_buffer_pointer = (uint8_t *)&core_secondary_response_buffer;
    core_state_085_snapshot = core_state_085;
    core_state_086_snapshot = core_state_086;
    if (operation_status == 0x38) {
        working_result = 0x70;
        response_code = 0x70;
    } else {
        working_result = 1;
        response_code = 4;
    }
    core_update_persistent_state_status(&core_secondary_response_buffer_pointer);
    operation_status = core_read_primary_persistent_state(&core_persistent_identity_record);
    if ((operation_status != 0) || (operation_status = core_get_persistent_identity_record_for_initialize_rdx_identity(), operation_status != 0)) {
        operation_status = core_read_fallback_persistent_state(&core_persistent_identity_record);
        if ((operation_status != 0) || (operation_status = core_get_persistent_identity_record_for_initialize_rdx_identity(), operation_status != 0)) {
            ti_memset(&core_persistent_identity_record, 0, 0x40);
        }
        core_load_flash_configuration_byte(&core_persistent_identity_record);
        core_persistent_identity_record = core_calculate_record_checksum(&core_persistent_identity_record, 0x40);
        core_rewrite_primary_persistent_state(&core_persistent_identity_record);
    }
    core_finalize_identity_initialization_hook();
    return;
}

/**
 * @brief Handle descriptor command descriptor context.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_handle_descriptor_command_descriptor_context(buffer, value, length, flags) uint32_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Handle descriptor command descriptor context; persistent state is carried in core command descriptor context, core state 153. */
    bool condition_met;
    char b_derived;
    int operation_status;
    char *descriptor_cursor;
    uint32_t working_result;
    uint32_t callback_result;
    uint8_t core_command_descriptor_context_snapshot;
    uint32_t field_end;
    uint32_t core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result;
    uint8_t descriptor_flags;
    uint32_t core_release_record_payload_when_short_result;
    uint32_t status_bits;
    uint64_t core_is_cartridge_input_state_six_result;
    uint32_t length_component;
    uint32_t flags_snapshot;

    core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result = 0;
    status_bits = 0;
    length_component = length;
    flags_snapshot = flags;
    if (*(char *)buffer[2] == '\x15') {
        working_result = 4;
        field_end = 4;
    /* Shared target for core handle descriptor command descriptor context before release record payload when short; incoming paths preserve the same state assumptions. */
    core_handle_descriptor_command_descriptor_context_before_release_record_payload_when_short:
        core_release_record_payload_when_short_result = core_release_record_payload_when_short(buffer + 1, working_result, field_end);
        core_release_record_payload_when_short_result = core_release_record_payload_when_short_result & 0xffff;
    } else {
        if (*(char *)buffer[2] == 'U') {
            working_result = 7;
            field_end = 8;
            goto core_handle_descriptor_command_descriptor_context_before_release_record_payload_when_short;
        }
        core_release_record_payload_when_short_result = 0;
    }
    operation_status = core_get_constant_hex_1000_value();
    if (operation_status < (int)core_release_record_payload_when_short_result) {
        working_result = 8;
    /* Shared target for core handle descriptor command descriptor context before update command validation context; incoming paths preserve the same state assumptions. */
    core_handle_descriptor_command_descriptor_context_before_update_command_validation_context:
        *buffer = working_result;
        return;
    }
    operation_status = core_update_command_validation_context(buffer, &length_component, core_release_record_payload_when_short_result);
    if (operation_status == 0) {
        working_result = 0x1f;
        goto core_handle_descriptor_command_descriptor_context_before_update_command_validation_context;
    }
    descriptor_cursor = (char *)buffer[2];
    descriptor_flags = descriptor_cursor[1];
    core_release_record_payload_when_short_result = (uint32_t)(descriptor_flags >> 5);
    if ((descriptor_flags >> 4 & 1) == 0)
        goto core_handle_descriptor_command_descriptor_context_before_finalize_command_response_hook;
    core_release_record_payload_when_short_result = length_component & 0xffff;
    /* Gate the following state transition on *descriptor_cursor == '\x15'; the handler is skipped when this condition is false. */
    if (*descriptor_cursor == '\x15') {
        if (3 < core_release_record_payload_when_short_result) {
            core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result = *(uint8_t *)(flags_snapshot + 3) + 4;
        }
    } else if (7 < core_release_record_payload_when_short_result) {
        core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result = core_transfer_range_end_range_start_for_descriptor_descriptor_resource(flags_snapshot, 6, 7);
        core_release_record_payload_when_short_result = length_component & 0xffff;
        core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result = (core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result & 0xff) + 8;
        descriptor_flags = *(uint8_t *)(buffer[2] + 1);
    }
    if (core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result < core_release_record_payload_when_short_result) {
        b_derived = *(char *)(flags_snapshot + core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result);
        status_bits = core_release_record_payload_when_short_result - core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result;
    } else {
        b_derived = '\0';
    }
    condition_met = false;
    core_release_record_payload_when_short_result = flags_snapshot;
    if (b_derived != '\0') {
        if (b_derived == '\b') {
            *buffer = 10;
            if ((2 < status_bits) &&
                (core_release_record_payload_when_short_result = flags_snapshot + core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result, (*(uint8_t *)(core_release_record_payload_when_short_result + 2) >> 2 & 1) != 0)) {
                working_result = 0;
            /* Shared target for core handle descriptor command descriptor context before updating buffer; incoming paths preserve the same state assumptions. */
            core_handle_descriptor_command_descriptor_context_before_updating_buffer:
                *buffer = working_result;
            }
        } else if (b_derived == '1') {
            core_command_descriptor_context_snapshot = *(uint8_t *)(flags_snapshot + core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result + 4);
            status_bits = (uint32_t)core_command_descriptor_context_snapshot;
            core_release_record_payload_when_short_result = flags_snapshot + core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result;
            if ((core_command_descriptor_context_snapshot & 0xe2) != 0)
                goto core_handle_descriptor_command_descriptor_context_before_process_request_request_for_descriptor_descriptor_resource;
            if ((core_command_descriptor_context_snapshot >> 4 & 1) == 0) {
            /* Shared target for core handle descriptor command descriptor context before is cartridge input state six; incoming paths preserve the same state assumptions. */
            core_handle_descriptor_command_descriptor_context_before_is_cartridge_input_state_six:
                if ((status_bits >> 3 & 1) != 0)
                    goto core_handle_descriptor_command_descriptor_context_before_updating_command_descriptor_context_snapshot;
                core_command_descriptor_context_snapshot = core_command_descriptor_context & 0xf7;
            } else {
                if ((core_command_descriptor_context_snapshot >> 3 & 1) == 0) {
                    core_is_cartridge_input_state_six_result = core_is_cartridge_input_state_six(core_command_descriptor_context_snapshot >> 4);
                    core_release_record_payload_when_short_result = (uint32_t)((uint64_t)core_is_cartridge_input_state_six_result >> 0x20);
                    if ((int)core_is_cartridge_input_state_six_result == 0)
                        goto core_handle_descriptor_command_descriptor_context_before_process_request_request_for_descriptor_descriptor_resource;
                    core_process_cartridge_input_state();
                    status_bits = status_bits & 0xffffffef;
                    goto core_handle_descriptor_command_descriptor_context_before_is_cartridge_input_state_six;
                }
            /* Shared target for core handle descriptor command descriptor context before updating command descriptor context snapshot; incoming paths preserve the same state assumptions. */
            core_handle_descriptor_command_descriptor_context_before_updating_command_descriptor_context_snapshot:
                core_command_descriptor_context_snapshot = core_command_descriptor_context | 8;
            }
            if ((status_bits >> 4 & 1) == 0) {
                core_release_record_payload_when_short_result = core_command_descriptor_context_snapshot & 0xef;
            } else {
                core_release_record_payload_when_short_result = core_command_descriptor_context_snapshot | 0x10;
            }
            core_command_descriptor_context = (uint8_t)core_release_record_payload_when_short_result;
        } else if (b_derived == '3') {
            operation_status = core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result + flags_snapshot;
            if (((*(char *)(operation_status + 1) != '\x02') || (*(char *)(operation_status + 2) != '\0')) ||
                (core_command_descriptor_context_snapshot = *(uint8_t *)(operation_status + 3), core_release_record_payload_when_short_result = (uint32_t)core_command_descriptor_context_snapshot,
                 (core_command_descriptor_context_snapshot & 0xfe) != 0))
                goto core_handle_descriptor_command_descriptor_context_before_process_request_request_for_descriptor_descriptor_resource;
            core_release_record_payload_when_short_result = core_command_descriptor_context_snapshot & 1;
            core_state_153 = (uint8_t)core_release_record_payload_when_short_result;
        } else {
            if (b_derived != '4') {
            /* Shared target for core handle descriptor command descriptor context before process request request for descriptor descriptor resource; incoming paths preserve the same state assumptions. */
            core_handle_descriptor_command_descriptor_context_before_process_request_request_for_descriptor_descriptor_resource:
                working_result = 10;
                goto core_handle_descriptor_command_descriptor_context_before_updating_buffer;
            }
            core_process_request_request_for_descriptor_descriptor_resource(buffer, core_transfer_range_end_range_start_for_descriptor_descriptor_resource_result + flags_snapshot);
            condition_met = true;
            core_release_record_payload_when_short_result = callback_result;
        }
    }
    if (((descriptor_flags & 1) != 0) && (!condition_met)) {
        *buffer = 8;
    }
/* Shared target for core handle descriptor command descriptor context before finalize command response hook; incoming paths preserve the same state assumptions. */
core_handle_descriptor_command_descriptor_context_before_finalize_command_response_hook:
    core_finalize_command_response_hook(buffer, core_release_record_payload_when_short_result);
    return;
}

/**
 * @brief Monitor cartridge input.
 *
 * @return No value.
 */
void core_monitor_cartridge_input()

{
    /* Monitor cartridge input; persistent state is carried in core state 103, core state 104 and related record fields. */
    bool condition_met;
    uint32_t operation_status;
    int followup_status;
    uint32_t core_update_configuration_update_context_result;

    /* Sample the cartridge-present input before running the dependent readiness probes. */
    operation_status = sci_read_logical_input(4);
    followup_status = core_process_flash_validation_context();
    condition_met = false;
    /* A cartridge is accepted only when flash, sense, ADC, and command contexts all report ready. */
    if ((((followup_status == 0) && (operation_status != 0)) && (followup_status = core_update_sense_record_context(), followup_status == 0)) &&
        ((followup_status = core_forward_mcp3008_sample_value(), followup_status == 0 && (followup_status = core_process_command_processing_context(), followup_status == 0)))) {
        condition_met = true;
    }
    /* Restart the 100 ms debounce interval whenever the raw input changes. */
    if (core_state_103 != operation_status) {
        core_state_103 = (uint8_t)operation_status;
        rti_process_thermal_timer_state(&core_state_104, 100);
    }
    followup_status = core_get_thermal_control_context(&core_state_104);
    if (followup_status != 0) {
        /* Publish only a stable, fully validated insertion or removal state. */
        if (condition_met) {
            core_state_105 = '\x01';
            core_update_spi_set_logical_for_monitor_cartridge_input();
        } else if (operation_status == 0) {
            core_update_spi_set_logical_for_monitor_cartridge_input(0);
            core_state_105 = '\0';
        }
    }
    followup_status = core_update_sense_record_context();
    if ((followup_status != 0) || (followup_status = core_forward_mcp3008_sample_value(), followup_status != 0)) {
        return;
    }
    if (core_cartridge_input_state == 0) {
        /* State 0 waits for both flash services and the debounced cartridge input. */
        followup_status = core_process_flash_validation_context();
        if (followup_status != 0) {
            return;
        }
        followup_status = core_process_sci_read_logical_for_monitor_cartridge_input();
        if (followup_status == 0) {
            return;
        }
        core_handle_cartridge_input_state_for_monitor_cartridge_input(1);
        core_update_command_buffer_state(0);
        return;
    }
    if (core_cartridge_input_state == 1) {
        /* State 1 gives AHCI initialization time to discover the newly inserted device. */
        followup_status = ahci_continue_port_initialization();
        if (followup_status == 0) {
            if (core_state_105 == '\0') {
                /* No stable cartridge yet: arm the retry delay selected by the configuration byte. */
                core_update_configuration_update_context_result = core_update_configuration_update_context(core_state_108 & 0xff);
                rti_process_thermal_timer_state(&core_state_102, core_update_configuration_update_context_result);
                return;
            }
            followup_status = core_get_thermal_control_context(&core_state_102);
            if (followup_status == 0) {
                return;
            }
            core_update_configuration_update_context_result = 3;
            goto core_branch_target_041;
        }
    /* Shared target for core branch target 040; incoming paths preserve the same state assumptions. */
    core_branch_target_040:
        core_update_configuration_update_context_result = 2;
        goto core_branch_target_041;
    }
    if (core_cartridge_input_state == 2) {
    /* Shared target for core branch target 039; incoming paths preserve the same state assumptions. */
    core_branch_target_039:
        followup_status = core_process_sci_read_logical_for_monitor_cartridge_input();
        if (followup_status != 0) {
            return;
        }
    } else {
        if (core_cartridge_input_state != 3) {
            if (core_cartridge_input_state == 4) {
                /* State 4 retries link initialization, but limits the recovery loop to three attempts. */
                followup_status = core_process_sci_read_logical_for_monitor_cartridge_input();
                if (followup_status != 0) {
                    followup_status = ahci_continue_port_initialization();
                    if (followup_status == 0) {
                        followup_status = core_get_thermal_control_context(&core_state_102);
                        if (followup_status == 0) {
                            return;
                        }
                        core_sense_selection_flags = core_sense_selection_flags | 4;
                        core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
                        if (core_state_108 < 3) {
                            core_handle_cartridge_input_state_for_monitor_cartridge_input(3);
                            core_state_108 = core_state_108 + 1;
                            return;
                        }
                        core_update_configuration_update_context_result = 5;
                        goto core_branch_target_041;
                    }
                    core_sense_selection_flags = core_sense_selection_flags & 0xfffffffb;
                    core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
                    goto core_branch_target_040;
                }
                goto core_branch_target_042;
            }
            if (core_cartridge_input_state != 5) {
                if (core_cartridge_input_state != 6) {
                    return;
                }
                goto core_branch_target_039;
            }
        }
        followup_status = core_process_sci_read_logical_for_monitor_cartridge_input();
        if (followup_status != 0) {
            core_update_configuration_update_context_result = 4;
            goto core_branch_target_041;
        }
    }
/* Shared target for core branch target 042; incoming paths preserve the same state assumptions. */
core_branch_target_042:
    core_update_configuration_update_context_result = 0;
/* Shared target for core branch target 041; incoming paths preserve the same state assumptions. */
core_branch_target_041:
    /* All terminal paths converge here so the state change is published exactly once. */
    core_handle_cartridge_input_state_for_monitor_cartridge_input(core_update_configuration_update_context_result);
    return;
}

/**
 * @brief Handle clear memory lookup ATA command buffer.
 *
 * @return No value.
 */
void core_handle_clear_memory_lookup_ata_command_buffer()

{
    /* Handle clear memory lookup ATA command buffer; persistent state is carried in core ATA command buffer, core response end context and related record fields. */
    bool condition_met;
    int operation_status;
    int lookup_status;
    uint8_t working_buffer[88];

    /* Begin with a clean ATA request and response-tail context for the new device probe. */
    core_update_clear_memory_response_end_context();
    ti_memset(&core_ata_command_buffer, 0, 0x28);
    operation_status = core_configure_ata_devices_address_ata_device_table(&core_ata_command_buffer, working_buffer);
    core_response_end_context = operation_status != 0;
    if (operation_status == 0) {
        /* Device-table setup failed: expose the sense condition and defer work to command phase 2. */
        core_state_099 = 1;
        core_sense_selection_flags = core_sense_selection_flags | 4;
        core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
        core_set_command_phase_to_two();
        return;
    }
    condition_met = false;
    /* Submit the defined identification command and retain whether standby processing is needed. */
    core_handle_ata_command_opcode_ata_command_context(working_buffer);
    operation_status = core_process_ata_command_buffer_for_clear_memory_lookup(working_buffer);
    core_ata_standby_context = (uint8_t)operation_status;
    lookup_status = core_get_runtime_context_base_for_build_scsi_mode();
    lookup_status = core_read_context_field_28_value(lookup_status + 0x2c);
    if (lookup_status == 0) {
        if (operation_status != 0) {
            operation_status = sci_read_logical_input(0);
            lookup_status = core_get_runtime_context_base_for_build_scsi_mode();
            lookup_status = core_read_context_field_6c_byte(lookup_status + 0x2c);
            if (lookup_status == 0) {
                core_sense_selection_flags = core_sense_selection_flags | 1;
                core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
            }
            lookup_status = core_get_primary_success_flag(operation_status == 0);
            condition_met = lookup_status != 0;
            if (condition_met) {
                /* Publish only after both RDX authentication and device setup have completed. */
                rdx_sata_usb_publish();
                /* The primary device is usable: enable SMART polling and arm the five-minute sample timer. */
                core_command_state_context = 1;
                core_initialize_smart_monitoring();
                core_update_response_update_context();
                rti_process_thermal_timer_state(&core_state_130, 300000);
                core_gpio_pin_context = 0;
                core_process_command_phase(operation_status == 0);
                core_process_validation_flags();
            }
            operation_status = core_get_runtime_context_base_for_build_scsi_mode();
            core_device_status_context = core_get_0x40(operation_status + 0x2c, &core_state_133);
            core_process_device_status_context();
            goto core_handle_clear_memory_lookup_ata_command_buffer_before_set_command_phase_to_one;
        }
    } else if (lookup_status == 1) {
        core_get_secondary_success_flag();
    }
    core_sense_selection_flags = core_sense_selection_flags | 2;
    /* Bit 1 records the generic discovery/lookup failure when no more specific path succeeded. */
    core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
/* Shared target for core handle clear memory lookup ATA command buffer before set command phase to one; incoming paths preserve the same state assumptions. */
core_handle_clear_memory_lookup_ata_command_buffer_before_set_command_phase_to_one:
    if (!condition_met) {
        core_state_099 = 1;
        core_set_command_phase_to_one();
    }
    return;
}

/**
 * @brief Advance mechanism state.
 *
 * @return No value.
 */
void core_advance_mechanism_state()

{
    /* Advance mechanism state; persistent state is carried in core state 111, core command completion context and related record fields. */
    char working_result;
    uint8_t sci_read_logical_input_result;
    int operation_status;
    uint32_t dispatch_state;

    /* States 1..7 select the next physical output pattern; timer expiry gates every transition. */
    if (core_state_111 == 1) {
        operation_status = core_update_scsi_logical_address_context();
        if (operation_status == 0) {
            operation_status = core_process_sci_read_logical_for_monitor_cartridge_input();
            if (operation_status == 0) {
                dispatch_state = 2;
            } else {
                operation_status = core_get_thermal_control_context(&core_command_completion_context);
                if (operation_status == 0)
                    goto core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state;
                operation_status = core_get_response_type_value();
                if (operation_status == 0)
                    goto core_branch_target_044;
            /* Shared target for core branch target 045; incoming paths preserve the same state assumptions. */
            core_branch_target_045:
                dispatch_state = 8;
            }
        } else {
        /* Shared target for core branch target 046; incoming paths preserve the same state assumptions. */
        core_branch_target_046:
            dispatch_state = 4;
        }
    } else if (core_state_111 == 2) {
        operation_status = core_update_scsi_logical_address_context();
        if (operation_status != 0)
            goto core_branch_target_046;
        operation_status = core_get_thermal_control_context(&core_command_completion_context);
        if (operation_status == 0)
            goto core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state;
        dispatch_state = 3;
    } else if (core_state_111 == 3) {
        operation_status = core_update_scsi_logical_address_context();
        if (operation_status != 0)
            goto core_branch_target_046;
        operation_status = core_get_thermal_control_context(&core_command_completion_context);
        if (operation_status == 0)
            goto core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state;
    /* Shared target for core branch target 044; incoming paths preserve the same state assumptions. */
    core_branch_target_044:
        dispatch_state = 5;
    } else if (core_state_111 == 5) {
        operation_status = core_update_scsi_logical_address_context();
        if (operation_status != 0)
            goto core_branch_target_046;
        operation_status = core_get_thermal_control_context(&core_command_completion_context);
        if (operation_status == 0)
            goto core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state;
        dispatch_state = 6;
    } else if (core_state_111 == 6) {
        operation_status = core_update_scsi_logical_address_context();
        if (operation_status == 0) {
            operation_status = core_get_thermal_control_context(&core_command_completion_context);
            if (operation_status == 0)
                goto core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state;
            core_state_110 = core_state_110 + 1;
            /* Two retries reuse state 5; a third expiry moves to the terminal state 8. */
            if (2 < core_state_110)
                goto core_branch_target_045;
            goto core_branch_target_044;
        }
        dispatch_state = 7;
    } else {
        if ((core_state_111 != 7) || (operation_status = core_get_thermal_control_context(&core_command_completion_context), operation_status == 0))
            goto core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state;
        dispatch_state = 1;
    }
    core_drive_mechanism_outputs(dispatch_state);
/* Shared target for core advance mechanism state before update SCI read logical for advance mechanism state; incoming paths preserve the same state assumptions. */
core_advance_mechanism_state_before_update_sci_read_logical_for_advance_mechanism_state:
    if (core_state_115 == '\0') {
        /* Detect the first edge and start a separate 100 ms input debounce timer. */
        operation_status = core_update_sci_read_logical_for_advance_mechanism_state();
        working_result = core_state_116;
        if (operation_status != 0) {
            core_state_115 = 1;
            rti_process_thermal_timer_state(&core_state_114, 100);
            return;
        }
    } else {
        /* Once the edge is stable, update the mechanism context only when the cartridge input agrees. */
        operation_status = core_update_sci_read_logical_for_advance_mechanism_state();
        working_result = core_state_116;
        if (operation_status == 0) {
            core_state_115 = '\0';
            if (core_state_116 != '\0') {
                sci_read_logical_input_result = sci_read_logical_input(4);
                operation_status = core_update_valid(sci_read_logical_input_result, &core_cartridge_monitor_context);
                working_result = '\0';
                if (operation_status == 0) {
                    core_update_mechanism_update_context_for_advance_mechanism_state(0);
                }
            }
        } else if ((core_state_116 == '\0') &&
                   (operation_status = core_get_thermal_control_context(&core_state_114), working_result = core_state_116, operation_status != 0)) {
            sci_read_logical_input_result = sci_read_logical_input(4);
            operation_status = core_process_x01_for_advance_mechanism_state(sci_read_logical_input_result, &core_cartridge_monitor_context);
            if (operation_status == 0) {
                core_update_mechanism_update_context_for_advance_mechanism_state(1);
            }
            working_result = '\x01';
        }
    }
    core_state_116 = working_result;
    /* Preserve the latest raw level so the next invocation can distinguish edges from steady state. */
    return;
}

/**
 * @brief Select RDX sense status.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_select_rdx_sense_status(buffer) uint32_t *buffer;

{
    /* Select RDX sense status; persistent state is carried in core validation flags. */
    int operation_status;
    uint32_t working_result;
    uint32_t core_get_sense_selection_flags_value_result;
    uint32_t entry_index;
    uint32_t response_detail;
    uint32_t response_code;

    core_update_clear_memory_response_buffer_context();
    /* The command byte contributes the upper five bits of the initial fixed-format response code. */
    core_update_response_buffer_context(*(char *)(buffer[2] + 1) << 5 | 0xf);
    core_mark_secondary_sense_record_not_ready(0x800f29c);
    operation_status = core_get_set_validation_flags();
    if ((operation_status == 0) || (operation_status = core_transfer_ti_memset_n(), operation_status == 0)) {
        response_code = 0x1a;
        working_result = 5;
        response_detail = 0x4400;
    /* Shared target for core branch target 050; incoming paths preserve the same state assumptions. */
    core_branch_target_050:
        core_write_selected_sense_fields(working_result, 4, response_detail);
        *buffer = response_code;
        return;
    }
    operation_status = core_is_command_phase_three_for_build_scsi_mode();
    if (operation_status != 0) {
        core_mark_primary_sense_record_not_ready(0x800f2b4);
        operation_status = core_process_ata_command_for_select_rdx_sense();
        if (operation_status == 0) {
            response_code = 0x1e;
            working_result = 6;
            response_detail = 0x4092;
            goto core_branch_target_050;
        }
        core_write_selected_sense_fields(0, 0, 0);
    }
    if (core_validation_flags != 0) {
        /* Priority is encoded by the least-significant asserted validation bit. */
        core_get_sense_selection_flags_value_result = 0;
        /* Repeat while (int)core_get_sense_selection_flags_value_result < 0x20; the body advances or polls the state needed to leave the loop. */
        do {
            if ((1 << (core_get_sense_selection_flags_value_result & 0xff) & core_validation_flags) != 0)
                break;
            core_get_sense_selection_flags_value_result = core_get_sense_selection_flags_value_result + 1;
        } while ((int)core_get_sense_selection_flags_value_result < 0x20);
        if (core_get_sense_selection_flags_value_result == 0) {
            working_result = 0x4080;
            response_detail = 0x1d;
        } else if (core_get_sense_selection_flags_value_result == 3) {
            working_result = 0x4081;
            response_detail = 0x1c;
        } else if (core_get_sense_selection_flags_value_result == 4) {
            working_result = 0x4082;
            response_detail = 0x1b;
        } else {
            working_result = 0x4400;
            response_detail = 0x1a;
        }
        response_code = 4;
        goto core_branch_target_049;
    }
    core_get_sense_selection_flags_value_result = core_get_sense_selection_flags_value();
    if (core_get_sense_selection_flags_value_result == 0) {
        return;
    }
    entry_index = 0;
    /* When validation is clear, select the first pending runtime sense condition instead. */
    /* Repeat while (int)entry_index < 0x20; the body advances or polls the state needed to leave the loop. */
    do {
        if ((1 << (entry_index & 0xff) & core_get_sense_selection_flags_value_result) != 0)
            break;
        entry_index = entry_index + 1;
    } while ((int)entry_index < 0x20);
    if (entry_index == 0) {
        response_code = 4;
        working_result = 0x4090;
    } else {
        if (entry_index != 1) {
            if (entry_index == 2) {
                response_code = 4;
                working_result = 0x4091;
            /* Shared target for core select RDX sense status before updating response detail; incoming paths preserve the same state assumptions. */
            core_select_rdx_sense_status_before_updating_response_detail:
                response_detail = 5;
            } else {
                if (entry_index == 3) {
                    working_result = 0x1100;
                    response_detail = 0x17;
                } else {
                    if (entry_index != 4) {
                        if ((entry_index == 5) || (entry_index == 7)) {
                            response_code = 7;
                            working_result = 0x2700;
                            response_detail = 0x15;
                            goto core_branch_target_049;
                        }
                        response_code = 3;
                        working_result = 0x5200;
                        goto core_select_rdx_sense_status_before_updating_response_detail;
                    }
                    working_result = 0xc00;
                    response_detail = 0x16;
                }
                response_code = 3;
            }
            goto core_branch_target_049;
        }
        response_code = 2;
        working_result = 0x3000;
    }
    response_detail = 6;
/* Shared target for core branch target 049; incoming paths preserve the same state assumptions. */
core_branch_target_049:
    *buffer = response_detail;
    core_write_selected_sense_fields(4, response_code, working_result);
    return;
}

