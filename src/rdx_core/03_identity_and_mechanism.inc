/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
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
    /* Initialize RDX identity records; persistent state is carried in core compact identity vendor identifier, core compact identity product identifier and related record fields. */
    int operation_status;
    uint8_t *core_compact_identity_vendor_identifier_pointer;
    volatile uint8_t *volatile core_compact_identity_product_identifier_pointer = 0;
    volatile uint8_t *volatile core_unit_serial_number_pointer = 0;
    volatile uint16_t core_compact_identity_offset_four_word_snapshot = 0;
    volatile uint16_t core_compact_identity_offset_six_word_snapshot = 0;
    volatile uint8_t working_result = 0;
    volatile uint8_t response_code = 0;

    operation_status = core_read_unit_identity_record(&core_unit_identity_record);
    if ((operation_status != 0) || (operation_status = core_calculate_record_checksum(&core_unit_identity_record, 0x100), core_unit_identity_record != operation_status)) {
        ti_memset(&core_unit_identity_record, 0, 0x100);
        ti_memset(&core_unit_identity_record, 0, 0x100);
        core_copy_bytes(&core_unit_product_identifier, &core_default_unit_product_identifier, 0x10);
        core_copy_bytes(&core_unit_identity_offset_10_bytes, 0x800e758, 8);
        core_copy_bytes(&core_unit_serial_number, "7360242889", 10);
        core_copy_bytes(&core_identity_record_offset_70_bytes, 0x800e750, 8);
        core_copy_bytes(&core_compact_identity_vendor_identifier, &core_default_identity_vendor_identifier, 8);
        core_copy_bytes(&core_compact_identity_product_identifier, &core_default_compact_product_identifier, 0x10);
        core_identity_record_variant_code = 0x38;
        core_compact_identity_offset_four_word = 0x1a5a;
        core_identity_record_offset_20_word = 0;
        core_compact_identity_offset_six_word = 6;
        core_identity_record_offset_24_word = 1;
        core_copy_bytes(&core_identity_record_offset_7a_bytes, &core_default_identity_offset_7a_bytes, 0x10);
        core_copy_bytes(&core_identity_record_offset_8a_bytes, &core_default_identity_offset_8a_bytes, 0x10);
        core_copy_bytes(&core_identity_record_offset_9a_bytes, &core_default_identity_offset_9a_bytes, 0x10);
        core_identity_record_version = 6;
    }
    operation_status = core_get_identity_variant_code();
    core_compact_identity_product_identifier_pointer = (volatile uint8_t *)&core_compact_identity_product_identifier;
    core_unit_serial_number_pointer = (volatile uint8_t *)&core_unit_serial_number;
    core_compact_identity_vendor_identifier_pointer = (uint8_t *)&core_compact_identity_vendor_identifier;
    core_compact_identity_offset_four_word_snapshot = core_compact_identity_offset_four_word;
    core_compact_identity_offset_six_word_snapshot = core_compact_identity_offset_six_word;
    if (operation_status == 0x38) {
        working_result = 0x70;
        response_code = 0x70;
    } else {
        working_result = 1;
        response_code = 4;
    }
    core_update_descriptor_response_metadata(&core_compact_identity_vendor_identifier_pointer);
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
 * @brief Handle SCSI mode select.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_handle_scsi_mode_select(buffer, value, length, flags) uint32_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Handle SCSI mode select; persistent state is carried in core mode page 31 flags, core notification suppression flag. */
    bool condition_met;
    char b_derived;
    int operation_status;
    char *descriptor_cursor;
    uint32_t working_result;
    uint32_t callback_result;
    uint8_t core_mode_page_31_flags_snapshot;
    uint32_t field_end;
    uint32_t core_read_big_endian_record_field_result;
    uint8_t descriptor_flags;
    uint32_t core_release_record_payload_when_short_result;
    uint32_t status_bits;
    uint64_t core_is_cartridge_input_state_six_result;
    uint32_t length_component;
    uint32_t flags_snapshot;

    core_read_big_endian_record_field_result = 0;
    status_bits = 0;
    length_component = length;
    flags_snapshot = flags;
    if (*(char *)buffer[2] == '\x15') {
        working_result = 4;
        field_end = 4;
    /* Shared target for core mode select read parameter length; incoming paths preserve the same state assumptions. */
    core_mode_select_read_parameter_length:
        core_release_record_payload_when_short_result = core_release_record_payload_when_short(buffer + 1, working_result, field_end);
        core_release_record_payload_when_short_result = core_release_record_payload_when_short_result & 0xffff;
    } else {
        if (*(char *)buffer[2] == 'U') {
            working_result = 7;
            field_end = 8;
            goto core_mode_select_read_parameter_length;
        }
        core_release_record_payload_when_short_result = 0;
    }
    operation_status = core_get_transfer_buffer_capacity();
    if (operation_status < (int)core_release_record_payload_when_short_result) {
        working_result = 8;
    /* Shared target for core mode select store validation error; incoming paths preserve the same state assumptions. */
    core_mode_select_store_validation_error:
        *buffer = working_result;
        return;
    }
    operation_status = core_get_active_command_response_buffer(buffer, &length_component, core_release_record_payload_when_short_result);
    if (operation_status == 0) {
        working_result = 0x1f;
        goto core_mode_select_store_validation_error;
    }
    descriptor_cursor = (char *)buffer[2];
    descriptor_flags = descriptor_cursor[1];
    core_release_record_payload_when_short_result = (uint32_t)(descriptor_flags >> 5);
    if ((descriptor_flags >> 4 & 1) == 0)
        goto core_mode_select_finalize_response;
    core_release_record_payload_when_short_result = length_component & 0xffff;
    /* Gate the following state transition on *descriptor_cursor == '\x15'; the handler is skipped when this condition is false. */
    if (*descriptor_cursor == '\x15') {
        if (3 < core_release_record_payload_when_short_result) {
            core_read_big_endian_record_field_result = *(uint8_t *)(flags_snapshot + 3) + 4;
        }
    } else if (7 < core_release_record_payload_when_short_result) {
        core_read_big_endian_record_field_result = core_read_big_endian_record_field(flags_snapshot, 6, 7);
        core_release_record_payload_when_short_result = length_component & 0xffff;
        core_read_big_endian_record_field_result = (core_read_big_endian_record_field_result & 0xff) + 8;
        descriptor_flags = *(uint8_t *)(buffer[2] + 1);
    }
    if (core_read_big_endian_record_field_result < core_release_record_payload_when_short_result) {
        b_derived = *(char *)(flags_snapshot + core_read_big_endian_record_field_result);
        status_bits = core_release_record_payload_when_short_result - core_read_big_endian_record_field_result;
    } else {
        b_derived = '\0';
    }
    condition_met = false;
    core_release_record_payload_when_short_result = flags_snapshot;
    if (b_derived != '\0') {
        if (b_derived == '\b') {
            *buffer = 10;
            if ((2 < status_bits) &&
                (core_release_record_payload_when_short_result = flags_snapshot + core_read_big_endian_record_field_result, (*(uint8_t *)(core_release_record_payload_when_short_result + 2) >> 2 & 1) != 0)) {
                working_result = 0;
            /* Shared target for core mode select store page status; incoming paths preserve the same state assumptions. */
            core_mode_select_store_page_status:
                *buffer = working_result;
            }
        } else if (b_derived == '1') {
            core_mode_page_31_flags_snapshot = *(uint8_t *)(flags_snapshot + core_read_big_endian_record_field_result + 4);
            status_bits = (uint32_t)core_mode_page_31_flags_snapshot;
            core_release_record_payload_when_short_result = flags_snapshot + core_read_big_endian_record_field_result;
            if ((core_mode_page_31_flags_snapshot & 0xe2) != 0)
                goto core_mode_select_reject_page_parameters;
            if ((core_mode_page_31_flags_snapshot >> 4 & 1) == 0) {
            /* Shared target for core mode select choose page 31 flags; incoming paths preserve the same state assumptions. */
            core_mode_select_choose_page_31_flags:
                if ((status_bits >> 3 & 1) != 0)
                    goto core_mode_select_set_page_31_bit_three;
                core_mode_page_31_flags_snapshot = core_mode_page_31_flags & 0xf7;
            } else {
                if ((core_mode_page_31_flags_snapshot >> 3 & 1) == 0) {
                    core_is_cartridge_input_state_six_result = core_is_cartridge_input_state_six(core_mode_page_31_flags_snapshot >> 4);
                    core_release_record_payload_when_short_result = (uint32_t)((uint64_t)core_is_cartridge_input_state_six_result >> 0x20);
                    if ((int)core_is_cartridge_input_state_six_result == 0)
                        goto core_mode_select_reject_page_parameters;
                    core_process_cartridge_input_state();
                    status_bits = status_bits & 0xffffffef;
                    goto core_mode_select_choose_page_31_flags;
                }
            /* Shared target for core mode select set page 31 bit three; incoming paths preserve the same state assumptions. */
            core_mode_select_set_page_31_bit_three:
                core_mode_page_31_flags_snapshot = core_mode_page_31_flags | 8;
            }
            if ((status_bits >> 4 & 1) == 0) {
                core_release_record_payload_when_short_result = core_mode_page_31_flags_snapshot & 0xef;
            } else {
                core_release_record_payload_when_short_result = core_mode_page_31_flags_snapshot | 0x10;
            }
            core_mode_page_31_flags = (uint8_t)core_release_record_payload_when_short_result;
        } else if (b_derived == '3') {
            operation_status = core_read_big_endian_record_field_result + flags_snapshot;
            if (((*(char *)(operation_status + 1) != '\x02') || (*(char *)(operation_status + 2) != '\0')) ||
                (core_mode_page_31_flags_snapshot = *(uint8_t *)(operation_status + 3), core_release_record_payload_when_short_result = (uint32_t)core_mode_page_31_flags_snapshot,
                 (core_mode_page_31_flags_snapshot & 0xfe) != 0))
                goto core_mode_select_reject_page_parameters;
            core_release_record_payload_when_short_result = core_mode_page_31_flags_snapshot & 1;
            core_notification_suppression_flag = (uint8_t)core_release_record_payload_when_short_result;
        } else {
            if (b_derived != '4') {
            /* Shared target for core mode select reject page parameters; incoming paths preserve the same state assumptions. */
            core_mode_select_reject_page_parameters:
                working_result = 10;
                goto core_mode_select_store_page_status;
            }
            core_process_request_request_for_descriptor_descriptor_resource(buffer, core_read_big_endian_record_field_result + flags_snapshot);
            condition_met = true;
            core_release_record_payload_when_short_result = callback_result;
        }
    }
    if (((descriptor_flags & 1) != 0) && (!condition_met)) {
        *buffer = 8;
    }
/* Shared target for core mode select finalize response; incoming paths preserve the same state assumptions. */
core_mode_select_finalize_response:
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
    /* Monitor cartridge input; persistent state is carried in core cartridge input raw level, core cartridge input debounce timer and related record fields. */
    bool condition_met;
    uint32_t operation_status;
    int followup_status;
    uint32_t core_get_configured_delay_ms_result;

    /* Sample the cartridge-present input before running the dependent readiness probes. */
    operation_status = sci_read_logical_input(4);
    followup_status = core_process_flash_validation_context();
    condition_met = false;
    /* A cartridge is accepted only when flash, sense, ADC, and command contexts all report ready. */
    if ((((followup_status == 0) && (operation_status != 0)) && (followup_status = core_is_mechanism_transition_pending(), followup_status == 0)) &&
        ((followup_status = core_is_firmware_image_validation_active(), followup_status == 0 && (followup_status = core_is_cartridge_startup_delay_active(), followup_status == 0)))) {
        condition_met = true;
    }
    /* Restart the 100 ms debounce interval whenever the raw input changes. */
    if (core_cartridge_input_raw_level != operation_status) {
        core_cartridge_input_raw_level = (uint8_t)operation_status;
        rti_set_software_timer(&core_cartridge_input_debounce_timer, 100);
    }
    followup_status = core_is_software_timer_expired(&core_cartridge_input_debounce_timer);
    if (followup_status != 0) {
        /* Publish only a stable, fully validated insertion or removal state. */
        if (condition_met) {
            core_cartridge_input_debounced_present = '\x01';
            core_update_spi_set_logical_for_monitor_cartridge_input();
        } else if (operation_status == 0) {
            core_update_spi_set_logical_for_monitor_cartridge_input(0);
            core_cartridge_input_debounced_present = '\0';
        }
    }
    followup_status = core_is_mechanism_transition_pending();
    if ((followup_status != 0) || (followup_status = core_is_firmware_image_validation_active(), followup_status != 0)) {
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
            if (core_cartridge_input_debounced_present == '\0') {
                /* No stable cartridge yet: arm the retry delay selected by the configuration byte. */
                core_get_configured_delay_ms_result = core_get_configured_delay_ms(core_cartridge_link_retry_count & 0xff);
                rti_set_software_timer(&core_cartridge_ready_retry_timer, core_get_configured_delay_ms_result);
                return;
            }
            followup_status = core_is_software_timer_expired(&core_cartridge_ready_retry_timer);
            if (followup_status == 0) {
                return;
            }
            core_get_configured_delay_ms_result = 3;
            goto core_cartridge_monitor_publish_selected_state;
        }
    /* Shared target for core cartridge monitor enter device discovery; incoming paths preserve the same state assumptions. */
    core_cartridge_monitor_enter_device_discovery:
        core_get_configured_delay_ms_result = 2;
        goto core_cartridge_monitor_publish_selected_state;
    }
    if (core_cartridge_input_state == 2) {
    /* Shared target for core cartridge monitor wait for removal; incoming paths preserve the same state assumptions. */
    core_cartridge_monitor_wait_for_removal:
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
                        followup_status = core_is_software_timer_expired(&core_cartridge_ready_retry_timer);
                        if (followup_status == 0) {
                            return;
                        }
                        core_sense_selection_flags = core_sense_selection_flags | 4;
                        core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
                        if (core_cartridge_link_retry_count < 3) {
                            core_handle_cartridge_input_state_for_monitor_cartridge_input(3);
                            core_cartridge_link_retry_count = core_cartridge_link_retry_count + 1;
                            return;
                        }
                        core_get_configured_delay_ms_result = 5;
                        goto core_cartridge_monitor_publish_selected_state;
                    }
                    core_sense_selection_flags = core_sense_selection_flags & 0xfffffffb;
                    core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
                    goto core_cartridge_monitor_enter_device_discovery;
                }
                goto core_cartridge_monitor_enter_absent_state;
            }
            if (core_cartridge_input_state != 5) {
                if (core_cartridge_input_state != 6) {
                    return;
                }
                goto core_cartridge_monitor_wait_for_removal;
            }
        }
        followup_status = core_process_sci_read_logical_for_monitor_cartridge_input();
        if (followup_status != 0) {
            core_get_configured_delay_ms_result = 4;
            goto core_cartridge_monitor_publish_selected_state;
        }
    }
/* Shared target for core cartridge monitor enter absent state; incoming paths preserve the same state assumptions. */
core_cartridge_monitor_enter_absent_state:
    core_get_configured_delay_ms_result = 0;
/* Shared target for core cartridge monitor publish selected state; incoming paths preserve the same state assumptions. */
core_cartridge_monitor_publish_selected_state:
    /* All terminal paths converge here so the state change is published exactly once. */
    core_handle_cartridge_input_state_for_monitor_cartridge_input(core_get_configured_delay_ms_result);
    return;
}

/**
 * @brief Handle clear memory lookup ATA command buffer.
 *
 * @return No value.
 */
void core_handle_clear_memory_lookup_ata_command_buffer()

{
    /* Handle clear memory lookup ATA command buffer; persistent state is carried in core cartridge ata device info, core ata device discovered and related record fields. */
    bool condition_met;
    int operation_status;
    int lookup_status;
    uint8_t working_buffer[88];

    /* Begin with a clean ATA request and response-tail context for the new device probe. */
    core_clear_ata_discovery_flags();
    ti_memset(&core_cartridge_ata_device_info, 0, 0x28);
    operation_status = core_configure_ata_devices_address_ata_device_table(&core_cartridge_ata_device_info, working_buffer);
    core_ata_device_discovered = operation_status != 0;
    if (operation_status == 0) {
        /* Device-table setup failed: expose the sense condition and defer work to command phase 2. */
        core_ata_discovery_failed_flag = 1;
        core_sense_selection_flags = core_sense_selection_flags | 4;
        core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
        core_set_command_phase_to_two();
        return;
    }
    condition_met = false;
    /* Submit the defined identification command and retain whether standby processing is needed. */
    core_execute_ata_security_unlock(working_buffer);
    operation_status = core_process_ata_command_buffer_for_clear_memory_lookup(working_buffer);
    core_ata_configuration_loaded = (uint8_t)operation_status;
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
                core_cartridge_monitor_enabled = 1;
                core_initialize_smart_monitoring();
                core_arm_standby_activity_check();
                rti_set_software_timer(&core_thermal_monitor_timer, 300000);
                core_ata_standby_active = 0;
                core_process_command_phase(operation_status == 0);
                core_process_validation_flags();
            }
            operation_status = core_get_runtime_context_base_for_build_scsi_mode();
            core_thermal_pwm_override_active = core_get_0x40(operation_status + 0x2c, &core_device_status_probe_output);
            core_apply_thermal_pwm_override();
            goto core_ata_discovery_mark_failure;
        }
    } else if (lookup_status == 1) {
        core_get_secondary_success_flag();
    }
    core_sense_selection_flags = core_sense_selection_flags | 2;
    /* Bit 1 records the generic discovery/lookup failure when no more specific path succeeded. */
    core_update_enabled(0x800f2b4, core_sense_selection_flags != 0);
/* Shared target for core ata discovery mark failure; incoming paths preserve the same state assumptions. */
core_ata_discovery_mark_failure:
    if (!condition_met) {
        core_ata_discovery_failed_flag = 1;
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
    /* Advance mechanism state; persistent state is carried in core mechanism output state, core mechanism transition timer and related record fields. */
    char working_result;
    uint8_t sci_read_logical_input_result;
    int operation_status;
    uint32_t dispatch_state;

    /* States 1..7 select the next physical output pattern; timer expiry gates every transition. */
    if (core_mechanism_output_state == 1) {
        operation_status = core_update_mechanism_input_release();
        if (operation_status == 0) {
            operation_status = core_process_sci_read_logical_for_monitor_cartridge_input();
            if (operation_status == 0) {
                dispatch_state = 2;
            } else {
                operation_status = core_is_software_timer_expired(&core_mechanism_transition_timer);
                if (operation_status == 0)
                    goto core_mechanism_update_input_debounce;
                operation_status = core_get_response_type_value();
                if (operation_status == 0)
                    goto core_mechanism_select_retry_delay;
            /* Shared target for core mechanism select terminal stop; incoming paths preserve the same state assumptions. */
            core_mechanism_select_terminal_stop:
                dispatch_state = 8;
            }
        } else {
        /* Shared target for core mechanism stop after input release; incoming paths preserve the same state assumptions. */
        core_mechanism_stop_after_input_release:
            dispatch_state = 4;
        }
    } else if (core_mechanism_output_state == 2) {
        operation_status = core_update_mechanism_input_release();
        if (operation_status != 0)
            goto core_mechanism_stop_after_input_release;
        operation_status = core_is_software_timer_expired(&core_mechanism_transition_timer);
        if (operation_status == 0)
            goto core_mechanism_update_input_debounce;
        dispatch_state = 3;
    } else if (core_mechanism_output_state == 3) {
        operation_status = core_update_mechanism_input_release();
        if (operation_status != 0)
            goto core_mechanism_stop_after_input_release;
        operation_status = core_is_software_timer_expired(&core_mechanism_transition_timer);
        if (operation_status == 0)
            goto core_mechanism_update_input_debounce;
    /* Shared target for core mechanism select retry delay; incoming paths preserve the same state assumptions. */
    core_mechanism_select_retry_delay:
        dispatch_state = 5;
    } else if (core_mechanism_output_state == 5) {
        operation_status = core_update_mechanism_input_release();
        if (operation_status != 0)
            goto core_mechanism_stop_after_input_release;
        operation_status = core_is_software_timer_expired(&core_mechanism_transition_timer);
        if (operation_status == 0)
            goto core_mechanism_update_input_debounce;
        dispatch_state = 6;
    } else if (core_mechanism_output_state == 6) {
        operation_status = core_update_mechanism_input_release();
        if (operation_status == 0) {
            operation_status = core_is_software_timer_expired(&core_mechanism_transition_timer);
            if (operation_status == 0)
                goto core_mechanism_update_input_debounce;
            core_mechanism_output_retry_count = core_mechanism_output_retry_count + 1;
            /* Two retries reuse state 5; a third expiry moves to the terminal state 8. */
            if (2 < core_mechanism_output_retry_count)
                goto core_mechanism_select_terminal_stop;
            goto core_mechanism_select_retry_delay;
        }
        dispatch_state = 7;
    } else {
        if ((core_mechanism_output_state != 7) || (operation_status = core_is_software_timer_expired(&core_mechanism_transition_timer), operation_status == 0))
            goto core_mechanism_update_input_debounce;
        dispatch_state = 1;
    }
    core_drive_mechanism_outputs(dispatch_state);
/* Shared target for core mechanism update input debounce; incoming paths preserve the same state assumptions. */
core_mechanism_update_input_debounce:
    if (core_mechanism_input_debounce_pending == '\0') {
        /* Detect the first edge and start a separate 100 ms input debounce timer. */
        operation_status = core_update_sci_read_logical_for_advance_mechanism_state();
        working_result = core_mechanism_input_previous_level;
        if (operation_status != 0) {
            core_mechanism_input_debounce_pending = 1;
            rti_set_software_timer(&core_mechanism_input_debounce_timer, 100);
            return;
        }
    } else {
        /* Once the edge is stable, update the mechanism context only when the cartridge input agrees. */
        operation_status = core_update_sci_read_logical_for_advance_mechanism_state();
        working_result = core_mechanism_input_previous_level;
        if (operation_status == 0) {
            core_mechanism_input_debounce_pending = '\0';
            if (core_mechanism_input_previous_level != '\0') {
                sci_read_logical_input_result = sci_read_logical_input(4);
                operation_status = core_update_valid(sci_read_logical_input_result, &core_cartridge_monitor_context);
                working_result = '\0';
                if (operation_status == 0) {
                    core_mark_cartridge_eject_pending(0);
                }
            }
        } else if ((core_mechanism_input_previous_level == '\0') &&
                   (operation_status = core_is_software_timer_expired(&core_mechanism_input_debounce_timer), working_result = core_mechanism_input_previous_level, operation_status != 0)) {
            sci_read_logical_input_result = sci_read_logical_input(4);
            operation_status = core_process_x01_for_advance_mechanism_state(sci_read_logical_input_result, &core_cartridge_monitor_context);
            if (operation_status == 0) {
                core_mark_cartridge_eject_pending(1);
            }
            working_result = '\x01';
        }
    }
    core_mechanism_input_previous_level = working_result;
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

    core_clear_selected_sense_fields();
    /* The command byte contributes the upper five bits of the initial fixed-format response code. */
    core_set_selected_sense_status(*(char *)(buffer[2] + 1) << 5 | 0xf);
    core_mark_secondary_sense_record_not_ready(0x800f29c);
    operation_status = core_get_set_validation_flags();
    if ((operation_status == 0) || (operation_status = core_test_transfer_buffer_memory(), operation_status == 0)) {
        response_code = 0x1a;
        working_result = 5;
        response_detail = 0x4400;
    /* Shared target for core sense status publish hardware error; incoming paths preserve the same state assumptions. */
    core_sense_status_publish_hardware_error:
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
            goto core_sense_status_publish_hardware_error;
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
        goto core_sense_status_publish_selected_condition;
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
            /* Shared target for core sense status select failure code five; incoming paths preserve the same state assumptions. */
            core_sense_status_select_failure_code_five:
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
                            goto core_sense_status_publish_selected_condition;
                        }
                        response_code = 3;
                        working_result = 0x5200;
                        goto core_sense_status_select_failure_code_five;
                    }
                    working_result = 0xc00;
                    response_detail = 0x16;
                }
                response_code = 3;
            }
            goto core_sense_status_publish_selected_condition;
        }
        response_code = 2;
        working_result = 0x3000;
    }
    response_detail = 6;
/* Shared target for core sense status publish selected condition; incoming paths preserve the same state assumptions. */
core_sense_status_publish_selected_condition:
    *buffer = response_detail;
    core_write_selected_sense_fields(4, response_code, working_result);
    return;
}
