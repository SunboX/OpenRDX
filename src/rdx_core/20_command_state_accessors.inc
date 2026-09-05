/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief SCSI write requests FUA.
 *
 * @param command_code Procedure input.
 * @param feature_flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_scsi_write_requests_fua(command_code, feature_flags)
int command_code;
uint32_t feature_flags;

{
    /* SCSI write requests FUA using only caller-provided data. */
    uint32_t operation_result;

    if (((command_code == 0x2a) || (command_code == 0x8a || command_code == 0xaa)) && ((feature_flags >> 3 & 1) != 0)) {
        operation_result = 1;
    } else {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Update transfer.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
uint32_t core_update_transfer(context, buffer)
uint32_t context;
uint8_t *buffer;

{
    /* Update transfer using only caller-provided data. */
    uint8_t working_buffer[24];
    uint8_t transfer_state[24];

    ((int (*)())core_process_transfer_requested)(context, buffer[1], *buffer, transfer_state, working_buffer);
    return 0;
}

/**
 * @brief Set mechanism output selection.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_mechanism_output_selection(context) uint8_t context;

{
    /* Set mechanism output selection; persistent state is carried in core mechanism output selection. */
    int core_get_response_type_value_result;

    core_get_response_type_value_result = core_get_response_type_value();
    if (core_get_response_type_value_result != 0) {
        core_mechanism_output_selection = context;
    }
    return;
}

/**
 * @brief Process set validation flags.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_process_set_validation_flags(context) int context;

{
    /* Process set validation flags using only caller-provided data. */
    core_set_validation_flags_flag_bit_three(context == 0);
    core_restart_ahci_port_initialization();
    core_forward_cartridge_input_state();
    return;
}

/**
 * @brief Restart AHCI port initialization.
 *
 * @return No value.
 */
void core_restart_ahci_port_initialization()

{
    /* Restart AHCI port initialization; persistent state is carried in core ahci port initialization state. */
    core_ahci_port_initialization_state = 0;
    ahci_init_port();
    core_reset_sata_link();
    return;
}

/**
 * @brief Check logical input and state.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_check_logical_input_and_state()

{
    /* Check logical input and state using only caller-provided data. */
    int operation_status;
    uint32_t operation_result;

    operation_status = sci_read_logical_input(4);
    operation_result = 0;
    if ((operation_status != 0) && (operation_status = core_process_flash_validation_context(), operation_status != 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Extend pending cartridge eject wait.
 *
 * @return No value.
 */
void core_extend_pending_cartridge_eject_wait()

{
    /* Extend pending cartridge eject wait; persistent state is carried in core cartridge eject pending, core cartridge eject wait timer. */
    if (core_cartridge_eject_pending != '\0') {
        rti_set_software_timer(&core_cartridge_eject_wait_timer, 5000);
        return;
    }
    return;
}

/**
 * @brief Reset smart temperature monitor.
 *
 * @return No value.
 */
void core_reset_smart_temperature_monitor()

{
    /* Reset smart temperature monitor; persistent state is carried in core smart temperature celsius, core smart monitoring active and related record fields. */
    core_smart_temperature_celsius = 0xff;
    core_smart_monitoring_active = 0;
    core_thermal_hot_sample_retry_count = 0;
    return;
}

/**
 * @brief Process identity update context.
 *
 * @return No value.
 */
uint32_t core_process_identity_update_context()

{
    /* Process identity update context; persistent state is carried in core unit identity record. */
    core_unit_identity_record = core_update_identity_update_context();
    /* Call core rewrite unit identity record through the defined unprototyped function-pointer signature. */
    ((void (*)())core_rewrite_unit_identity_record)(&core_unit_identity_record);
    return 0;
}

/**
 * @brief Erase boot flash sectors.
 *
 * @return No value.
 */
uint32_t core_erase_boot_flash_sectors()

{
    /* Erase boot flash sectors using only caller-provided data. */
    int core_get_set_interrupt_mmio_result;
    int entry_index;

    core_get_set_interrupt_mmio_result = 0;
    /* Iterate while entry_index = 0; (core_get_set_interrupt_mmio_result == 0 && (entry_index < 0x10)); entry_index = entry_index + 1, preserving the defined cursor and bound. */
    for (entry_index = 0; (core_get_set_interrupt_mmio_result == 0 && (entry_index < 0x10)); entry_index = entry_index + 1) {
        core_get_set_interrupt_mmio_result = core_get_set_interrupt_mmio(entry_index << 0xc);
    }
    return 0;
}

/**
 * @brief Clear firmware image validation.
 *
 * @return No value.
 */
void core_clear_firmware_image_validation()

{
    /* Clear firmware image validation; persistent state is carried in core firmware image validation active, core firmware image validation context. */
    core_firmware_image_validation_active = 0;
    ti_memset(&core_firmware_image_validation_context, 0, 0x198);
    return;
}

/**
 * @brief Update device for device for component.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return No value.
 */
uint32_t core_update_device_for_device_for_component(context, offset)
uint32_t context;
int offset;

{
    /* Update device for device for component using only caller-provided data. */
    int device_record;

    device_record = *(int *)(offset + 8);
    *(uint8_t *)(device_record + 0x28) = 1;
    /* Call core execute ata read dma through the defined unprototyped function-pointer signature. */
    ((void (*)())core_execute_ata_read_dma)(context, device_record);
    *(uint8_t *)(device_record + 0x28) = 0;
    return 0;
}

/**
 * @brief Update device for device for device.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return No value.
 */
uint32_t core_update_device_for_device_for_device(context, offset)
uint32_t context;
int offset;

{
    /* Update device for device for device using only caller-provided data. */
    int device_record;

    device_record = *(int *)(offset + 8);
    *(uint8_t *)(device_record + 0x28) = 1;
    /* Call core execute ata write dma through the defined unprototyped function-pointer signature. */
    ((void (*)())core_execute_ata_write_dma)(context, device_record);
    *(uint8_t *)(device_record + 0x28) = 0;
    return 0;
}

/**
 * @brief USB revision uses deferred eject.
 *
 * @return Result produced by the procedure.
 */
bool core_usb_revision_uses_deferred_eject()

{
    /* USB revision uses deferred eject; persistent state is carried in USB core revision. */
    return 0x1089 < usb_core_revision;
}

/**
 * @brief Reset device command arbitration.
 *
 * @return No value.
 */
void core_reset_device_command_arbitration()

{
    /* Reset device command arbitration; persistent state is carried in core device command busy, core mww dispatch blocked flag and related record fields. */
    core_device_command_busy = 0;
    core_mww_dispatch_blocked_flag = 0;
    core_mww_dispatch_deferred = 0;
    core_flash_update_context = 0;
    core_mww_device_command_active = 0;
    core_mww_command_received_flag = 0;
    return;
}

/**
 * @brief Resume deferred mww dispatch.
 *
 * @return No value.
 */
void core_resume_deferred_mww_dispatch()

{
    /* Resume deferred mww dispatch; persistent state is carried in core mww dispatch deferred, core mww dispatch blocked flag. */
    if (core_mww_dispatch_deferred != '\0') {
        core_mww_dispatch_deferred = 0;
        core_mww_dispatch_blocked_flag = 0;
        mww_handle_transfer_mww0_addr_device_transfer_state();
        return;
    }
    return;
}

/**
 * @brief Get notification state.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_notification_state()

{
    /* Get notification state; persistent state is carried in core notification state, core notification suppression flag. */
    uint32_t operation_result;

    operation_result = 0;
    if ((core_notification_state != '\0') && (core_notification_suppression_flag == '\0')) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Update reconnect state.
 *
 * @return No value.
 */
void core_update_reconnect_state()

{
    /* Update reconnect state; persistent state is carried in core reconnect state. */
    int operation_status;

    operation_status = core_is_mww_command_received();
    if (operation_status == 0) {
        core_reconnect_state = 3;
    } else {
        core_reconnect_state = 2;
    }
    return;
}

/**
 * @brief Process SCI read logical for resource USB state.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_sci_read_logical_for_resource_usb_state()

{
    /* Process SCI read logical for resource USB state using only caller-provided data. */
    int operation_status;
    uint32_t sci_read_logical_input_result;

    operation_status = ((int16_t (*)())core_get_identity_variant_code)(0);
    if (operation_status == 0x38) {
        sci_read_logical_input_result = sci_read_logical_input(0x10);
    } else {
        sci_read_logical_input_result = 1;
    }
    return sci_read_logical_input_result;
}

/**
 * @brief Initialize media record.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return No value.
 */
void core_initialize_media_record(context, offset) uint32_t context;
int offset;

{
    /* Initialize media record using only caller-provided data. */
    ti_memset(offset, 0, 0x84);
    *(uint32_t *)(offset + 0xc) = context;
    return;
}

/**
 * @brief Run ready state handler.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
uint32_t core_run_ready_state_handler(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Run ready state handler using only caller-provided data. */
    int operation_status;

    operation_status = ((int (*)())core_run_target_type_clear_memory_target_type_context)();
    if (operation_status == 0) {
        ((int (*)())core_handle_device_ti_memset)(context, length);
    }
    return 0;
}

/**
 * @brief Check idle input and state.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_check_idle_input_and_state()

{
    /* Check idle input and state using only caller-provided data. */
    int operation_status;
    uint32_t operation_result;

    operation_status = core_process_sci_read_logical_for_resource_usb_state();
    operation_result = 0;
    if ((operation_status == 0) && (operation_status = core_process_flash_validation_context(), operation_status != 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Process device status value.
 *
 * @return No value.
 */
void core_process_device_status_value()

{
    /* Process device status value using only caller-provided data. */
    int core_get_device_status_value_result;

    core_get_device_status_value_result = core_get_device_status_value(0);
    if (core_get_device_status_value_result != 0) {
        rti_delay_milliseconds(1);
        core_request_system_reset();
    }
    return;
}

/**
 * @brief Process ATA command buffer for build SCSI mode.
 *
 * @return Result produced by the procedure.
 */
uint16_t core_process_ata_command_buffer_for_build_scsi_mode()

{
    /* Process ATA command buffer for build SCSI mode using only caller-provided data. */
    int operation_status;
    uint16_t operation_result;

    operation_status = (int)core_get_cartridge_ata_device_info();
    operation_result = *(uint16_t *)(operation_status + 0x10);
    operation_status = core_get_response_end_context_for_response_end_word();
    if (operation_status != 0) {
        operation_result = 0x1000;
    }
    return operation_result;
}

/**
 * @brief Process command phase three.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_process_command_phase_three()

{
    /* Process command phase three using only caller-provided data. */
    int operation_status;
    uint32_t operation_result;

    operation_status = core_is_command_phase_three_for_build_scsi_mode();
    operation_result = 1;
    if ((operation_status == 0) && (operation_status = core_is_cartridge_input_state_six(), operation_status == 0)) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Copy bytes to alternate positions.
 *
 * @param destination Procedure input.
 * @param source Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_copy_bytes_to_alternate_positions(destination, source, condition) uint8_t *destination;
uint8_t *source;
int condition;

{
    /* Copy bytes to alternate positions using only caller-provided data. */
    uint32_t condition_field;

    if (condition != 0) {
        condition_field = condition + 1U >> 1;
        /* Repeat while condition_field != 0; the body advances or polls the state needed to leave the loop. */
        do {
            destination = destination + 2;
            condition_field = condition_field - 1;
            *destination = *source;
            source = source + 1;
        } while (condition_field != 0);
    }
    return;
}

/**
 * @brief Submit ATA command context.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return No value.
 */
void core_submit_ata_command_context(context, offset) uint32_t context;
int offset;

{
    /* Submit ATA command context using only caller-provided data. */
    ((void (*)())core_build_encrypted_password_record)(context, offset + 8, offset + 0x30, offset);
    return;
}

/**
 * @brief Update component.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @param index Procedure input.
 * @return No value.
 */
uint32_t core_update_component(uint32_t context, uint32_t value, uint32_t length, uint32_t flags, uint32_t index)

{
    /* Update component using only caller-provided data. */
    uint32_t length_component;

    length_component = length;
    ((uint32_t (*)())core_get_end_remaining)(context, value, &length_component, flags, index, 1);
    return 0;
}

/**
 * @brief Swap adjacent bytes.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_swap_adjacent_bytes(context, condition) int context;
uint32_t condition;

{
    /* Swap adjacent bytes using only caller-provided data. */
    uint8_t *data_cursor;
    uint8_t working_result;
    uint8_t *context_derived;

    condition = condition >> 1;
    context_derived = (uint8_t *)(context + -2);
    /* Repeat while condition != 0; the body advances or polls the state needed to leave the loop. */
    do {
        data_cursor = context_derived + 2;
        working_result = *data_cursor;
        *data_cursor = context_derived[3];
        condition = condition - 1;
        context_derived[3] = working_result;
        context_derived = data_cursor;
    } while (condition != 0);
    return;
}

/**
 * @brief Decode header flags and offset.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_decode_header_flags_and_offset(context)
int context;

{
    /* Decode header flags and offset using only caller-provided data. */
    uint8_t header_flags;
    uint32_t working_result;

    header_flags = *(uint8_t *)(context + 1);
    if ((header_flags >> 2 & 1) == 0) {
        if ((header_flags & 1) == 0) {
            working_result = 4;
        } else {
            working_result = 0xc;
        }
    } else {
        working_result = 0x14;
    }
    return (uint64_t)CONCAT14(header_flags >> 3, working_result);
}

/**
 * @brief Get structure field hex for structure field hex.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
int core_get_structure_field_hex_for_structure_field_hex(context, value)
uint32_t context;
uint32_t value;

{
    /* Get structure field hex for structure field hex using only caller-provided data. */
    uint32_t core_get_structure_field_hex_4c_value_result;
    uint32_t callback_result = 0;
    uint64_t core_get_structure_field_hex_50_value_result;

    core_get_structure_field_hex_4c_value_result = core_get_structure_field_hex_4c_value(context, value, context);
    core_get_structure_field_hex_50_value_result = core_get_structure_field_hex_50_value(callback_result, core_get_structure_field_hex_4c_value_result);
    return (int)core_get_structure_field_hex_50_value_result + (int)((uint64_t)core_get_structure_field_hex_50_value_result >> 0x20);
}

/**
 * @brief Update saturating add u32 for ATA standby context.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_update_saturating_add_u32_for_ata_standby_context(offset) int offset;

{
    /* Update saturating add u32 for ATA standby context using only caller-provided data. */
    uint32_t core_saturating_add_u32_result;

    core_saturating_add_u32_result = core_saturating_add_u32(*(uint32_t *)(offset + 0x2c), 1);
    *(uint32_t *)(offset + 0x2c) = core_saturating_add_u32_result;
    *(uint8_t *)(offset + 0x13) = 2;
    return;
}

/**
 * @brief Get remaining descriptor space.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
int core_get_remaining_descriptor_space(context, value)
uint32_t context;
uint32_t value;

{
    /* Get remaining descriptor space using only caller-provided data. */
    uint32_t core_decode_header_flags_and_offset_derived;
    int operation_result;
    int callback_result = 0;

    core_decode_header_flags_and_offset_derived = ((uint64_t (*)())core_decode_header_flags_and_offset)(context, value, context);
    if (*(uint16_t *)(callback_result + 2) < core_decode_header_flags_and_offset_derived) {
        operation_result = 0;
    } else {
        operation_result = *(uint16_t *)(callback_result + 2) - core_decode_header_flags_and_offset_derived;
    }
    return operation_result;
}

/**
 * @brief Get sata link speed.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_sata_link_speed(context)
int context;

{
    /* Get sata link speed; persistent state is carried in AHCI port SATA status register. */
    return (*(uint32_t *)(&ahci_port_sata_status_register + context * 0x80) & 0xff) >> 4;
}

/**
 * @brief Is cartridge input state six.
 *
 * @return Result produced by the procedure.
 */
bool core_is_cartridge_input_state_six()

{
    /* Is cartridge input state six; persistent state is carried in core cartridge input state. */
    return core_cartridge_input_state == 6;
}

/**
 * @brief Get constant zero value for pin resource state.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_constant_zero_value_for_pin_resource_state()

{
    /* Get constant zero value for pin resource state using only caller-provided data. */
    return 0;
}

/**
 * @brief Arm standby activity check.
 *
 * @return No value.
 */
void core_arm_standby_activity_check()

{
    /* Arm standby activity check; persistent state is carried in core standby activity check timer. */
    rti_set_software_timer(&core_standby_activity_check_timer, 300000);
    return;
}

/**
 * @brief Initialize smart temperature timer.
 *
 * @return No value.
 */
void core_initialize_smart_temperature_timer()

{
    /* Initialize smart temperature timer; persistent state is carried in core smart temperature sample timer. */
    ((void (*)())core_allocate_software_timer)(&core_smart_temperature_sample_timer);
    core_reset_smart_temperature_monitor();
    return;
}

/**
 * @brief Process end remaining for ti memset ti.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_process_end_remaining_for_ti_memset_ti(context, value) uint32_t context;
uint32_t value;

{
    /* Process end remaining for ti memset ti using only caller-provided data. */
    core_copy_bytes(context, value, 0x10);
    ((uint64_t (*)())core_process_clear_memory_clear_memory_copy_context)(context, 0x10);
    return;
}

/**
 * @brief Copy word buffer.
 *
 * @param destination Procedure input.
 * @param source Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_copy_word_buffer(destination, source, length) uint32_t *destination;
uint32_t *source;
int length;

{
    /* Copy word buffer using only caller-provided data. */
    /* Iterate while ; length != 0; length = length + -1, preserving the defined cursor and bound. */
    for (; length != 0; length = length + -1) {
        *destination = *source;
        source = source + 1;
        destination = destination + 1;
    }
    return;
}

/**
 * @brief Get 0xc for lookup key metadata.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t core_get_0xc_for_lookup_key_metadata(offset)
int offset;

{
    /* Get 0xc for lookup key metadata using only caller-provided data. */
    uint8_t offset_field_0xc;

    offset_field_0xc = 0xff;
    if ((*(int16_t *)(offset + 8) != 0) && (*(uint8_t **)(offset + 0xc) != (uint8_t *)0x0)) {
        offset_field_0xc = **(uint8_t **)(offset + 0xc) & 0x7f;
    }
    return offset_field_0xc;
}

/**
 * @brief Update device descriptor context.
 *
 * @return No value.
 */
void core_update_device_descriptor_context()

{
    /* Update device descriptor context; persistent state is carried in core primary descriptor state. */
    int core_get_set_validation_flags_result;

    core_get_set_validation_flags_result = core_get_set_validation_flags();
    if (core_get_set_validation_flags_result != 0) {
        core_primary_descriptor_state = 1;
    }
    return;
}

/**
 * @brief Request mww dispatch pause.
 *
 * @return Result produced by the procedure.
 */
bool core_request_mww_dispatch_pause()

{
    /* Get usb in endpoint info; persistent state is carried in core mww dispatch blocked flag, core device command busy. */
    core_mww_dispatch_blocked_flag = 1;
    return core_device_command_busy == '\0';
}

/**
 * @brief Process prepare runtime services.
 *
 * @return No value.
 */
void core_process_prepare_runtime_services()

{
    /* Process prepare runtime services using only caller-provided data. */
    core_prepare_runtime_services_hook();
    core_service_usb_reconnect();
    core_handle_thermal_control_context();
    core_handle_pin_input_pin_state();
    return;
}

/**
 * @brief Set command phase to one.
 *
 * @return No value.
 */
void core_set_command_phase_to_one()

{
    /* Set command phase to one; persistent state is carried in core command phase. */
    core_clear_transfer_byte_counters();
    core_command_phase = 1;
    return;
}

/**
 * @brief Clear command phase flags.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_clear_command_phase_flags(context) uint32_t context;

{
    /* Clear command phase flags; persistent state is carried in core command phase flags. */
    core_command_phase_flags = core_command_phase_flags & ~context;
    return;
}

/**
 * @brief Set command phase to two.
 *
 * @return No value.
 */
void core_set_command_phase_to_two()

{
    /* Set command phase to two; persistent state is carried in core command phase. */
    core_clear_transfer_byte_counters();
    core_command_phase = 2;
    return;
}
