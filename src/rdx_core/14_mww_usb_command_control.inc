/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Update word update context.
 *
 * @return No value.
 */
void core_update_word_update_context()

{
    /* Update word update context; persistent state is carried in usb serial number string index, usb self powered capable and related record fields. */
    int operation_status;
    int input_word = 0;
    int status_bits;

    status_bits = input_word;
    /* Query the active USB descriptors and cache the fields needed by the transfer dispatcher. */
    operation_status = ((uint32_t (*)())core_find_usb_descriptor)(1, &status_bits, 0);
    if (operation_status == 0) {
        usb_serial_number_string_index = *(uint8_t *)(status_bits + 0x10);
    }
    operation_status = ((uint32_t (*)())core_find_usb_descriptor)(2, &status_bits);
    if (operation_status == 0) {
        /* Bits 6 and 5 of descriptor byte 7 become the two defined capability flags. */
        usb_self_powered_capable = (uint8_t)(((uint32_t)*(uint8_t *)(status_bits + 7) << 0x19) >> 0x1f);
        usb_remote_wakeup_capable = (uint8_t)(((uint32_t)*(uint8_t *)(status_bits + 7) << 0x1a) >> 0x1f);
    }
    return;
}

/**
 * @brief Process MWW command state.
 *
 * @return No value.
 */
void core_process_mww_command_state()

{
    /* Process MWW command state; persistent state is carried in usb bot persistent stall, core mww command reset companion byte. */
    /* Restore the default AHCI request context before discarding the saved MWW transaction. */
    core_set_ahci_callbacks(&ahci_command_completion_callback_entry, 0, 0x80098e9);
    core_update_device_request_update(0x800c001);
    /* Call core process MWW saved request for MWW handle saved through the defined unprototyped function-pointer signature. */
    ((void (*)())core_process_mww_saved_request_for_mww_handle_saved)(0);
    usb_bot_persistent_stall = 0;
    core_mww_command_reset_companion_byte = 0;
    /* Publish the cleared state only after both in-memory activity flags are reset. */
    core_reset_bot_command_state();
    return;
}

/**
 * @brief Process buffer compare context.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return Result produced by the procedure.
 */
bool core_process_buffer_compare_context(offset, value, length) int offset;
uint32_t value;
uint32_t length;

{
    /* Process buffer compare context; persistent state is carried in core record type 0d descriptor. */
    bool condition_met;
    int operation_status;
    uint8_t working_buffer[24];

    operation_status = ((int (*)())core_copy_and_pad_record_payload)(offset, &core_record_type_0d_descriptor, working_buffer, length, value);
    if (operation_status == 0) {
        operation_status = ((int (*)())core_handle_fallback_decoded_request_bytes)(*(uint32_t *)(offset + 0x20), offset + 0xc, offset);
        condition_met = operation_status == 0;
    } else {
        condition_met = false;
    }
    return condition_met;
}

/**
 * @brief Queue ATA dma setup completion.
 *
 * @return No value.
 */
void core_queue_ata_dma_setup_completion()

{
    /* Queue ATA dma setup completion; persistent state is carried in ATA received dma setup tag, core ata callback write index and related record fields. */
    uint8_t ata_received_dma_setup_tag_snapshot;
    int callback_result = 0;

    ata_received_dma_setup_tag_snapshot = ata_received_dma_setup_tag;
    *(uint8_t *)(core_ata_callback_write_index * 0xc + ATA_CALLBACK_DATA_ADDRESS) = ata_received_dma_setup_tag_snapshot & 0x1f;
    core_enqueue_ata_completion_callback(core_ahci_dma_setup_callback);
    *(uint32_t *)(callback_result + 0x14) = 1;
    return;
}

/**
 * @brief Update mechanism input release.
 *
 * @return Result produced by the procedure.
 */
bool core_update_mechanism_input_release()

{
    /* Update mechanism input release; persistent state is carried in core mechanism input cycle state, core mechanism input release latch. */
    int operation_status;

    /* The logical input advances monotonically from unseen (0), through asserted (1), to released (2). */
    operation_status = core_update_sci_read_logical_for_scsi_logical_address_context();
    if (operation_status == 0) {
        core_mechanism_input_cycle_state = 1;
    } else if ((core_mechanism_input_cycle_state == 1) || (core_mechanism_input_release_latch != '\0')) {
        core_mechanism_input_cycle_state = 2;
    }
    return core_mechanism_input_cycle_state == 2;
}

/**
 * @brief Reset logical outputs.
 *
 * @return No value.
 */
void core_reset_logical_outputs()

{
    /* Reset logical outputs; persistent state is carried in core mechanism completion inhibited, core mechanism alternate output active. */
    /* Drop all three mechanism drive outputs before restoring the shared enable line. */
    spi_set_logical_output(0xd, 0);
    spi_set_logical_output(0xb, 0);
    spi_set_logical_output(0xc, 0);
    rti_delay_microseconds_with_watchdog(0x32);
    /* The 50 microsecond dead time avoids overlapping opposing output states. */
    spi_set_logical_output(10, 1);
    core_mechanism_completion_inhibited = 0;
    core_mechanism_alternate_output_active = 0;
    return;
}

/**
 * @brief Handle cartridge monitor context.
 *
 * @return No value.
 */
void core_handle_cartridge_monitor_context()

{
    /* Handle cartridge monitor context; persistent state is carried in core cartridge monitor context, core standby activity check timer and related record fields. */
    core_reset_ahci_port_initialization_state();
    core_initialize_cartridge_monitor(&core_cartridge_monitor_context);
    core_initialize_cartridge_mechanism_inputs();
    core_allocate_software_timer(&core_standby_activity_check_timer);
    core_allocate_software_timer(&core_cartridge_eject_wait_timer);
    core_reset_mechanism_outputs_when_idle();
    core_initialize_smart_temperature_timer();
    core_allocate_software_timer(&core_thermal_monitor_timer);
    return;
}

/**
 * @brief Process ATA standby context.
 *
 * @return Result produced by the procedure.
 */
int core_process_ata_standby_context()

{
    /* Process ATA standby context; persistent state is carried in core ata configuration loaded, core ata standby active. */
    int operation_result;
    int core_execute_ata_flush_cache_result;

    if (core_ata_configuration_loaded != '\0') {
        /* A pending standby request waits until the split-transfer bookkeeping is empty. */
        operation_result = core_get_runtime_context_base_for_build_scsi_mode();
        operation_result = core_accumulate_split_transfer_offsets(operation_result + 0x2c);
        if (operation_result == 0) {
            return 0;
        }
    }
    core_execute_ata_flush_cache_result = core_execute_ata_flush_cache();
    operation_result = 0;
    if ((core_execute_ata_flush_cache_result != 0) && (operation_result = core_execute_ata_standby_or_verify(1), operation_result != 0)) {
        /* Once standby is accepted, return the mechanism to idle and remember the deferred status update. */
        core_set_thermal_pwm_control_phase_zero();
        core_ata_standby_active = 1;
    }
    return operation_result;
}

/**
 * @brief Process device address.
 *
 * @param device_record Procedure input.
 * @param candidate_record Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_process_device_address(device_record, candidate_record)
int device_record;
int candidate_record;

{
    /* Process device address using only caller-provided data. */
    int operation_status;
    uint32_t output_address;
    uint32_t source_buffer;
    uint32_t comparison_value;

    comparison_value = *(uint32_t *)(candidate_record + 8);
    output_address = *(uint32_t *)(device_record + 4);
    source_buffer = *(uint32_t *)(device_record + 8);
    operation_status = core_run_requested_available_availability_request(output_address, device_record);
    if (operation_status != 0) {
        core_run_device_address_transfer(source_buffer, comparison_value, output_address);
        core_build_type_two_fingerprint(output_address, device_record);
        operation_status = core_compare_buffers_from_end(source_buffer, output_address, 0x80);
        if (operation_status == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Validate firmware image chunk.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
uint32_t core_validate_firmware_image_chunk(context, condition, length, flags)
uint32_t context;
int condition;
uint32_t length;
uint8_t flags;

{
    /* Validate firmware image chunk; persistent state is carried in core firmware image validation context, core firmware image validation active. */
    int core_validate_structured_flash_image_result;

    if (condition == 0) {
        core_reset_flash_validation_context(&core_firmware_image_validation_context);
        core_firmware_image_validation_active = 1;
    }
    core_validate_structured_flash_image_result = core_validate_structured_flash_image(&core_firmware_image_validation_context, context, condition, length, flags);
    if (core_validate_structured_flash_image_result != 0) {
        core_firmware_image_validation_active = 0;
    }
    return 0;
}

/**
 * @brief Update RTI clock mhz period units.
 *
 * @param context Procedure input.
 * @param pulse_percent Procedure input.
 * @param period_units Procedure input.
 * @return No value.
 */
void core_update_rti_clock_mhz_period_units(context, pulse_percent, period_units) int context;
int pulse_percent;
int period_units;

{
    /* Update RTI clock mhz period units using only caller-provided data. */
    uint32_t *context_derived;
    int callback_result = 0;

    context_derived = (uint32_t *)(context * 0x400 + -0x877f8);
    *context_derived = *context_derived | 2;
    core_start_pwm_channel();
    *(int *)(callback_result + 0xc) = period_units * rti_clock_mhz;
    *(uint32_t *)(callback_result + 0x10) = ((uint32_t)(rti_clock_mhz * period_units) / 100) * pulse_percent;
    return;
}

/**
 * @brief Process primary command context.
 *
 * @return No value.
 */
void core_process_primary_command_context()

{
    /* Process primary command context; persistent state is carried in core secondary descriptor state, core primary descriptor state. */
    int core_get_runtime_context_base_for_build_scsi_mode_result;

    if (core_secondary_descriptor_state == 2) {
        core_secondary_descriptor_state = 1;
    }
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_clear_secondary_descriptor_mask(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, 0x400000);
    core_get_runtime_context_base_for_build_scsi_mode_result = core_get_runtime_context_base_for_build_scsi_mode();
    core_clear_primary_descriptor_mask(core_get_runtime_context_base_for_build_scsi_mode_result + 0x2c, 0x400000);
    if (core_primary_descriptor_state == 2 || core_primary_descriptor_state == 3) {
        core_primary_descriptor_state = 1;
    }
    return;
}

/**
 * @brief Build state dependent sense record.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_build_state_dependent_sense_record(buffer)
uint8_t *buffer;

{
    /* Build state dependent sense record; persistent state is carried in core command sense table index. */
    uint8_t core_command_status_sense_key_table_pointer;
    int core_command_sense_table_index_snapshot;

    core_command_sense_table_index_snapshot = core_command_sense_table_index;
    ti_memset(buffer, 0, 0x14);
    core_command_sense_table_index_snapshot = core_command_sense_table_index_snapshot * 3;
    *buffer = 0x70;
    core_command_status_sense_key_table_pointer = (&core_command_status_sense_key_table)[core_command_sense_table_index_snapshot];
    buffer[7] = 0xc;
    buffer[2] = core_command_status_sense_key_table_pointer;
    buffer[0xc] = (&core_command_status_sense_triplet_base)[core_command_sense_table_index_snapshot];
    buffer[0xd] = (&core_command_status_sense_qualifier_table)[core_command_sense_table_index_snapshot];
    return 0x14;
}

/**
 * @brief Process command descriptor context.
 *
 * @return No value.
 */
void core_process_command_descriptor_context()

{
    /* Process command descriptor context; persistent state is carried in core mode page 31 flags, core command phase. */
    int operation_status;

    operation_status = core_get_notification_mode();
    if (((operation_status == 2) && (operation_status = core_is_command_phase_three_for_build_scsi_mode(), operation_status != 0)) &&
        (operation_status = core_is_cartridge_input_state_six(), operation_status == 0)) {
        core_update_reconnect_state();
    }
    operation_status = core_is_cartridge_input_state_six();
    if (operation_status == 0) {
        core_mode_page_31_flags = core_mode_page_31_flags & 0xe7;
    }
    core_command_phase = 0;
    return;
}

/**
 * @brief Copy memory bytes.
 *
 * @param destination Procedure input.
 * @param source Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_copy_memory_bytes(destination, source, length) uint32_t *destination;
uint32_t *source;
int length;

{
    /* Copy memory bytes using only caller-provided data. */
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
 * @brief Update word MWW transfer word.
 *
 * @return No value.
 */
void core_update_word_mww_transfer_word()

{
    /* Update word MWW transfer word; persistent state is carried in core MWW saved request, usb bot cbw pointer. */
    int operation_status;
    int input_word;
    int core_mww_saved_request_snapshot;

    if (*(int *)(usb_bot_cbw_pointer + 8) == 0) {
    /* Shared target for core usb bot mark csw pending; incoming paths preserve the same state assumptions. */
    core_usb_bot_mark_csw_pending:
        core_mww_saved_request = 3;
    } else {
        core_mww_saved_request_snapshot = input_word;
        operation_status = core_update_decoded_request(usb_bot_cbw_pointer + 0xf, &core_mww_saved_request_snapshot);
        if (operation_status == 0) {
            if (core_mww_saved_request_snapshot == 0) {
                core_mww_saved_request = 2;
                return;
            }
        } else {
            if (operation_status == 5)
                goto core_usb_bot_mark_csw_pending;
            if (*(char *)(usb_bot_cbw_pointer + 0xc) == '\0') {
                core_mww_saved_request = 2;
                return;
            }
        }
        core_mww_saved_request = 1;
    }
    return;
}

/**
 * @brief Arm USB control setup reception.
 *
 * @return No value.
 */
void core_arm_usb_control_setup_reception()

{
    /* Arm USB control setup reception; persistent state is carried in usb ep0 state, usb ep0 zero length packet pending and related record fields. */
    usb_ep0_state = 0;
    usb_ep0_zero_length_packet_pending = 0;
    /* Program core MMIO register 002 with 0xc0010c00; write ordering is hardware-significant. */
    usb_ep0_setup_trb_buffer_pointer_low = 0xc0010c00;
    usb_ep0_setup_trb_buffer_pointer_high = 0;
    /* Program core MMIO register 004 with 8; write ordering is hardware-significant. */
    usb_ep0_setup_trb_status = 8;
    usb_ep0_setup_trb_control = 0x23;
    mww_configure_transfer_descriptor(0);
    if (usb_reset_initialization_complete == '\0') {
        core_handle_usb_bus_reset();
    }
    return;
}

/**
 * @brief Process USB transfer state.
 *
 * @return No value.
 */
void core_process_usb_transfer_state()

{
    /* Process USB transfer state; persistent state is carried in usb device speed, usb disconnect countdown and related record fields. */
    uint32_t working_result;

    /* Reinitialize the device-side transfer register before sampling negotiated USB speed. */
    core_disable_bus_dynamic_clock_gating();
    usb_device_speed = usb_hal_get_connect_speed();
    usb_disconnect_countdown = 0x14;
    if (usb_device_speed == 3) {
        /* State 3 uses the shorter compare interval; all other speeds/states use 100 ticks. */
        working_result = 0x32;
    } else {
        working_result = 100;
    }
    rti_config_compare_interrupt(2, working_result);
    core_update_usb_transfer_state_for_usb_state(0, 0);
    if (usb_ep0_setup_configuration_pending != '\0') {
        /* Resume a deferred firmware/MWW transfer only after the USB state has been republished. */
        usb_ep0_setup_configuration_pending = '\0';
        core_arm_usb_control_setup_reception();
    }
    return;
}

/**
 * @brief Add multiword unsigned.
 *
 * @param sum_output Procedure input.
 * @param left_words Procedure input.
 * @param right_words End values paired with the range starts.
 * @param word_count Procedure input.
 * @return No value.
 */
void core_add_multiword_unsigned(sum_output, left_words, right_words, word_count) uint32_t *sum_output;
int *left_words;
uint32_t *right_words;
int word_count;

{
    /* Add multiword unsigned using only caller-provided data. */
    uint32_t remaining_count;
    uint32_t buffer_1_cursor;
    int buffer_2_cursor;
    uint32_t buffer_3_cursor;

    remaining_count = 0;
    /* Iterate while ; word_count != 0; word_count = word_count + -1, preserving the defined cursor and bound. */
    for (; word_count != 0; word_count = word_count + -1) {
        buffer_2_cursor = *left_words;
        *sum_output = remaining_count + buffer_2_cursor;
        remaining_count = (uint32_t)(remaining_count + buffer_2_cursor < remaining_count);
        buffer_1_cursor = *sum_output;
        buffer_3_cursor = *right_words;
        *sum_output = buffer_3_cursor + buffer_1_cursor;
        if (buffer_3_cursor + buffer_1_cursor < *right_words) {
            remaining_count = remaining_count + 1;
        }
        sum_output = sum_output + 1;
        right_words = right_words + 1;
        left_words = left_words + 1;
    }
    return;
}

/**
 * @brief Configure component for secondary resource state.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_configure_component_for_secondary_resource_state(uint32_t context, uint32_t value, uint32_t length, int flags, uint32_t index)

{
    /* Configure component for secondary resource state using only caller-provided data. */
    int operation_status;
    uint32_t core_process_runtime_context_base_for_component_result;
    uint32_t context_derived;
    uint32_t value_derived;
    volatile uint32_t length_component = 0;
    int flags_snapshot;

    context_derived = context;
    value_derived = value;
    length_component = length;
    flags_snapshot = flags;
    operation_status = core_pack_context_value_pair(index, context, &context_derived);
    if (operation_status == 0) {
        core_process_runtime_context_base_for_component_result = 0x22;
    } else {
        operation_status = core_compare_buffers_from_end(flags_snapshot + 4, value, 0x40);
        if (operation_status == 0) {
            core_process_runtime_context_base_for_component_result = core_process_runtime_context_base_for_component(context, flags, length);
        } else {
            core_process_runtime_context_base_for_component_result = 0x2a;
        }
    }
    return CONCAT44(value_derived, core_process_runtime_context_base_for_component_result);
}

/**
 * @brief Build indexed transfer descriptor.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
uint32_t core_build_indexed_transfer_descriptor(context, value, length, flags, index)
uint8_t context;
int16_t value;
uint32_t length;
uint32_t flags;
uint32_t index;

{
    /* Build indexed transfer descriptor using only caller-provided data. */
    uint8_t data_cursor[2];
    int16_t working_result;

    ti_memset(data_cursor, 0, 0x14);
    data_cursor[0] = context;
    working_result = core_decode_header_flags_and_offset(data_cursor);
    working_result = value + working_result;
    ((uint32_t (*)())core_transfer_transfer_completion_flag)(data_cursor, length, flags, index);
    return 0;
}

/**
 * @brief Submit control transfer fields.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
uint32_t core_submit_control_transfer_fields(context, value, condition, flags, index)
uint32_t context;
uint8_t value;
int condition;
uint32_t flags;
uint32_t index;

{
    /* Submit control transfer fields using only caller-provided data. */
    uint8_t value_derived;
    uint8_t status_bits;
    volatile uint16_t working_result = 0;
    volatile uint32_t flags_snapshot = 0;
    volatile uint32_t index_derived = 0;

    if (condition == 0) {
        status_bits = 0;
    } else {
        status_bits = 2;
    }
    working_result = 0xc;
    status_bits = status_bits | 1;
    index_derived = index;
    value_derived = value;
    flags_snapshot = flags;
    ((uint32_t (*)())core_transfer_transfer_completion_flag)(&value_derived, 0, 0, context);
    return 0;
}

/**
 * @brief Process ATA command for select RDX sense.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_ata_command_for_select_rdx_sense()

{
    /* Process ATA command for select RDX sense using only caller-provided data. */
    int core_update_capacity_block_context_result;
    uint32_t ahci_execute_taskfile_command_result;
    uint8_t working_buffer[32];
    uint8_t ata_command[10];
    volatile uint8_t working_result = 0;

    core_update_capacity_block_context_result = core_update_capacity_block_context(working_buffer);
    if (core_update_capacity_block_context_result == 0) {
        ahci_execute_taskfile_command_result = 0;
    } else {
        core_initialize_ata_taskfile(ata_command);
        working_result = 0x90;
        ahci_execute_taskfile_command_result = ahci_execute_taskfile_command(ata_command, working_buffer, 0, 0);
        core_clear_request_and_release_device_command(working_buffer);
    }
    return ahci_execute_taskfile_command_result;
}

/**
 * @brief Process clear memory response end context.
 *
 * @return No value.
 */
void core_process_clear_memory_response_end_context()

{
    /* Process clear memory response end context; persistent state is carried in core ata device discovered, core cartridge ready retry timer and related record fields. */
    ti_memset(&core_ata_device_discovered, 0, 0x98);
    core_allocate_software_timer(&core_cartridge_ready_retry_timer);
    core_allocate_software_timer(&core_cartridge_input_debounce_timer);
    core_allocate_software_timer(&core_cartridge_startup_delay_timer);
    rti_set_software_timer(&core_cartridge_startup_delay_timer, 5000);
    return;
}

/**
 * @brief Process command state context.
 *
 * @return No value.
 */
void core_process_command_state_context()

{
    /* Process command state context; persistent state is carried in core cartridge monitor enabled, core thermal monitor timer and related record fields. */
    int usb_hal_get_link_readiness_result;
    uint32_t working_result;

    if (core_cartridge_monitor_enabled != '\0') {
        usb_hal_get_link_readiness_result = usb_hal_get_link_readiness();
        if (usb_hal_get_link_readiness_result == 0) {
            working_result = 60000;
        } else {
            working_result = 300000;
        }
        rti_set_software_timer(&core_thermal_monitor_timer, working_result);
        if (core_ata_standby_active != '\0') {
            core_ata_standby_active = '\0';
            core_apply_thermal_pwm_override();
        }
    }
    return;
}

/**
 * @brief Get set interrupt MMIO.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
int core_get_set_interrupt_mmio(context)
uint32_t context;

{
    /* Get set interrupt MMIO using only caller-provided data. */
    int core_execute_spi_flash_operation_result;

    core_mask_usb_interrupt();
    core_execute_spi_flash_operation_result = core_execute_spi_flash_operation(6, 0, 0, 0);
    if (core_execute_spi_flash_operation_result == 0) {
        core_execute_spi_flash_operation_result = core_execute_spi_flash_operation(0x20, context, 0, 0);
    }
    core_unmask_usb_interrupt_when_allowed(1);
    rti_delay_microseconds_with_watchdog(4000);
    return core_execute_spi_flash_operation_result;
}

/**
 * @brief Write validated flash data.
 *
 * @param condition Procedure input.
 * @param source_cursor Procedure input.
 * @param remaining_length Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_write_validated_flash_data(condition, source_cursor, remaining_length)
int condition;
int source_cursor;
int remaining_length;

{
    /* Write validated flash data; persistent state is carried in core firmware image commit word. */
    int operation_status;

    if (condition == 0) {
        /* Address zero reserves the first four source bytes as the flash completion marker. */
        core_copy_bytes(&core_firmware_image_commit_word, source_cursor, 4);
        /* Erase/prepare the destination region before programming the remaining payload. */
        operation_status = core_erase_boot_flash_sectors();
        if (operation_status != 0) {
            return 0;
        }
        condition = 4;
        /* Skip the marker so only the validated payload is written starting at flash offset four. */
        source_cursor = source_cursor + 4;
        remaining_length = remaining_length + -4;
    }
    operation_status = core_program_spi_flash_pages(condition, remaining_length, source_cursor);
    /* Convert the lower-level zero-success convention into this routine's Boolean result. */
    if (operation_status != 0) {
        return 0;
    }
    return 1;
}

/**
 * @brief Process clear memory SPI response context.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_process_clear_memory_spi_response_context(offset, value, length) int offset;
uint32_t value;
uint32_t length;

{
    /* Process clear memory SPI response context; persistent state is carried in core led controller count, core led controller table. */
    uint32_t core_led_controller_count_snapshot;

    ti_memset(offset, 0, 0x18);
    core_led_controller_count_snapshot = (uint32_t)core_led_controller_count;
    *(uint32_t *)(offset + 0xc) = value;
    *(uint32_t *)(offset + 0x10) = length;
    core_led_controller_count = core_led_controller_count + 1;
    *(int *)(&core_led_controller_table + core_led_controller_count_snapshot * 4) = offset;
    if (2 < core_led_controller_count) {
        core_halt_forever();
    }
    return;
}

/**
 * @brief Process ATA command buffer for MWW handle transfer.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_process_ata_command_buffer_for_mww_handle_transfer(context, buffer) uint32_t context;
int *buffer;

{
    /* Process ATA command buffer for MWW handle transfer using only caller-provided data. */
    int core_transition_selected_or_bit_selection_state_snapshot;
    int core_get_cartridge_ata_device_info_result;
    uint8_t data_cursor[4];
    volatile uint32_t context_derived = 0;
    uint8_t working_buffer[8];
    uint16_t working_result = 0;

    *buffer = 0;
    data_cursor[0] = 0x10;
    context_derived = context;
    core_transition_selected_or_bit_selection_state_snapshot = ((bool (*)())core_transition_selected_or_bit_selection_state)(working_buffer, data_cursor);
    if (core_transition_selected_or_bit_selection_state_snapshot != 0) {
        core_get_cartridge_ata_device_info_result = (int)core_get_cartridge_ata_device_info(core_transition_selected_or_bit_selection_state_snapshot);
        *buffer = (uint32_t)working_result * (uint32_t)*(uint16_t *)(core_get_cartridge_ata_device_info_result + 0x10);
    }
    return core_transition_selected_or_bit_selection_state_snapshot == 0;
}

/**
 * @brief Process clear memory command phase.
 *
 * @return No value.
 */
void core_process_clear_memory_command_phase()

{
    /* Process clear memory command phase; persistent state is carried in core command phase, core command written bytes low and related record fields. */
    ti_memset(&core_command_phase, 0, 0x40);
    core_set_command_phase_flags(1);
    core_command_written_bytes_low = 0;
    core_command_written_bytes_high = 0;
    core_command_read_bytes_low = 0;
    core_command_read_bytes_high = 0;
    core_allocate_software_timer(&core_reconnect_delay_timer);
    return;
}

/**
 * @brief Validate BOT command wrapper.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_validate_bot_command_wrapper()

{
    /* Get usb bot cbw received length; persistent state is carried in usb bot cbw received length, usb bot cbw pointer. */
    uint32_t operation_result;

    operation_result = 0;
    /* Gate the following state transition on (((usb_bot_cbw_received_length == 0x1f) && (*usb_bot_cbw_pointer == 0x43425355)) && (*(char *)((int)usb_bot_cbw_pointer + 0xd) == '\0'); the handler is skipped when this condition is false. */
    if ((((usb_bot_cbw_received_length == 0x1f) && (*usb_bot_cbw_pointer == 0x43425355)) && (*(char *)((int)usb_bot_cbw_pointer + 0xd) == '\0')) &&
        (((*(uint8_t *)((int)usb_bot_cbw_pointer + 0xe) < 0x11 && (*(uint8_t *)((int)usb_bot_cbw_pointer + 0xe) != 0)) &&
          ((*(uint8_t *)(usb_bot_cbw_pointer + 3) & 0x7f) == 0)))) {
        operation_result = 1;
    }
    return operation_result;
}
