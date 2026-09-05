/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Load persistent notification mode.
 *
 * @return No value.
 */
void core_load_persistent_notification_mode()

{
    /* Load persistent notification mode; persistent state is carried in core notification mode. */
    core_notification_mode = core_get_persistent_identity_record_for_notification_context();
    return;
}

/**
 * @brief Is command phase three for build SCSI mode.
 *
 * @return Result produced by the procedure.
 */
bool core_is_command_phase_three_for_build_scsi_mode()

{
    /* Is command phase three for build SCSI mode; persistent state is carried in core command phase. */
    return core_command_phase == 3;
}

/**
 * @brief Is software timer expired.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool core_is_software_timer_expired(buffer) uint8_t *buffer;

{
    /* Is software timer expired; persistent state is carried in core software timer remaining ticks. */
    return *(int *)(&core_software_timer_remaining_ticks + (uint32_t)*buffer * 4) == 0;
}

/**
 * @brief Set USB device address.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_usb_device_address(context) uint32_t context;

{
    /* Set USB device address; persistent state is carried in usb device configuration register. */
    core_update_masked_register_bits(&usb_device_configuration_register, 0x3f8, (context & 0x7f) << 3);
    return;
}

/**
 * @brief Has active command flags.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_has_active_command_flags(buffer)
uint32_t *buffer;

{
    /* Has active command flags using only caller-provided data. */
    uint32_t operation_result;

    operation_result = 0;
    /* Gate the following state transition on ((*buffer >> 2 & 1) != 0) && ((*buffer & 3) != 0); the handler is skipped when this condition is false. */
    if (((*buffer >> 2 & 1) != 0) && ((*buffer & 3) != 0)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Update saturating add u32 for enable request flag.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_update_saturating_add_u32_for_enable_request_flag(offset) int offset;

{
    /* Update saturating add u32 for enable request flag using only caller-provided data. */
    uint32_t core_saturating_add_u32_result;

    core_saturating_add_u32_result = core_saturating_add_u32(*(uint32_t *)(offset + 0x3c));
    *(uint32_t *)(offset + 0x3c) = core_saturating_add_u32_result;
    *(uint8_t *)(offset + 0x17) = 2;
    return;
}

/**
 * @brief Update f x1c.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_update_f_x1c(buffer)
char *buffer;

{
    /* Update f x1c using only caller-provided data. */
    char buffer_cursor;
    uint32_t operation_result;

    buffer_cursor = *buffer;
    operation_result = 1;
    if ((buffer_cursor != '\f') && (buffer_cursor != '\x1c' && buffer_cursor != '\x1d')) {
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Process set notification state.
 *
 * @return No value.
 */
void core_process_set_notification_state()

{
    /* Process set notification state using only caller-provided data. */
    core_set_notification_state_to_zero_for_notify_change_resource();
    core_release_mww_device_command();
    core_update_command_feature_state(0);
    return;
}

/**
 * @brief Initialize ATA taskfile.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_initialize_ata_taskfile(offset) int offset;

{
    /* Initialize ATA taskfile using only caller-provided data. */
    ti_memset(offset, 0, 0xb);
    *(uint8_t *)(offset + 9) = 0x40;
    return;
}

/**
 * @brief Transfer constant 0x13.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
int core_transfer_constant_0x13(offset)
int offset;

{
    /* Transfer constant 0x13 using only caller-provided data. */
    uint32_t offset_field_0x13;
    int operation_result;

    operation_result = 1;
    /* Iterate while offset_field_0x13 = (uint32_t)*(uint8_t *)(offset + 0x13); offset_field_0x13 != 0; offset_field_0x13 = offset_field_0x13 - 1, preserving the defined cursor and bound. */
    for (offset_field_0x13 = (uint32_t)*(uint8_t *)(offset + 0x13); offset_field_0x13 != 0; offset_field_0x13 = offset_field_0x13 - 1) {
        operation_result = operation_result << 1;
    }
    return operation_result;
}

/**
 * @brief Update response word ti.
 *
 * @param context Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
void core_update_response_word_ti(context, buffer) uint32_t context;
uint32_t *buffer;

{
    /* Update response word ti using only caller-provided data. */
    core_build_encrypted_password_record(context, *buffer, buffer[1], buffer + 2);
    return;
}

/**
 * @brief Zero word buffer.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_zero_word_buffer(buffer, value) uint32_t *buffer;
int value;

{
    /* Zero word buffer using only caller-provided data. */
    /* Iterate while ; value != 0; value = value + -1, preserving the defined cursor and bound. */
    for (; value != 0; value = value + -1) {
        *buffer = 0;
        buffer = buffer + 1;
    }
    return;
}

/**
 * @brief Get 0xc for device descriptor response.
 *
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint8_t core_get_0xc_for_device_descriptor_response(offset)
int offset;

{
    /* Get 0xc for device descriptor response using only caller-provided data. */
    uint8_t offset_field_0xc;

    offset_field_0xc = 0;
    if ((1 < *(uint16_t *)(offset + 8)) && (*(int *)(offset + 0xc) != 0)) {
        offset_field_0xc = *(uint8_t *)(*(int *)(offset + 0xc) + 1);
    }
    return offset_field_0xc;
}

/**
 * @brief Release record payload when short.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
uint32_t core_release_record_payload_when_short(buffer, value, condition)
uint8_t *buffer;
uint32_t value;
int condition;

{
    /* Release record payload when short using only caller-provided data. */
    if (condition < (int)(uint32_t)*buffer) {
        ((uint32_t (*)())core_read_big_endian_record_field)(*(uint32_t *)(buffer + 4));
        return 0;
    }
    return 0;
}

/**
 * @brief Submit zero based transfer.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
uint32_t core_submit_zero_based_transfer(context, value, length)
uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Submit zero based transfer using only caller-provided data. */
    ((uint32_t (*)())core_run_expected_flag_expected_type)(context, 0, value, length);
    return 0;
}

/**
 * @brief Submit default transfer value.
 *
 * @param context Procedure input.
 * @return No value.
 */
uint32_t core_submit_default_transfer_value(context)
uint32_t context;

{
    /* Submit default transfer value using only caller-provided data. */
    uint32_t status_bits;

    status_bits = 0;
    core_write_four_byte_value(&status_bits, context);
    return 0;
}

/**
 * @brief Set AHCI callbacks.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_set_ahci_callbacks(context, value, length) uint32_t context;
uint32_t value;
uint32_t length;

{
    /* Set AHCI callbacks; persistent state is carried in ahci command completion callback, core ahci dma setup callback and related record fields. */
    ahci_command_completion_callback = context;
    core_ahci_dma_setup_callback = value;
    ahci_error_callback = length;
    return;
}

/**
 * @brief Clear ATA discovery flags.
 *
 * @return No value.
 */
void core_clear_ata_discovery_flags()

{
    /* Clear ATA discovery flags; persistent state is carried in core ata device discovered. */
    ti_memset(&core_ata_device_discovered, 0, 6);
    return;
}

/**
 * @brief Reset mechanism outputs when idle.
 *
 * @return No value.
 */
void core_reset_mechanism_outputs_when_idle()

{
    /* Reset mechanism outputs when idle using only caller-provided data. */
    int operation_status;

    operation_status = core_forward_sci_read_logical();
    if (operation_status == 0) {
        core_update_drive_mechanism_outputs(0);
    }
    return;
}

/**
 * @brief Is not cartridge input state zero.
 *
 * @return Result produced by the procedure.
 */
bool core_is_not_cartridge_input_state_zero()

{
    /* Is not cartridge input state zero; persistent state is carried in core cartridge input state. */
    return core_cartridge_input_state != 0;
}

/**
 * @brief Mark cartridge eject pending.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_mark_cartridge_eject_pending(condition) int condition;

{
    /* Mark cartridge eject pending; persistent state is carried in core cartridge eject pending. */
    if (condition != 0) {
        core_cartridge_eject_pending = 1;
    }
    return;
}

/**
 * @brief Set thermal PWM control phase zero.
 *
 * @return No value.
 */
void core_set_thermal_pwm_control_phase_zero()

{
    /* Set thermal PWM control phase zero; persistent state is carried in core thermal pwm control phase. */
    core_thermal_pwm_control_phase = 0;
    return;
}

/**
 * @brief Set thermal PWM control phase two.
 *
 * @return No value.
 */
void core_set_thermal_pwm_control_phase_two()

{
    /* Set thermal PWM control phase two; persistent state is carried in core thermal pwm control phase. */
    core_thermal_pwm_control_phase = 2;
    return;
}

/**
 * @brief Set thermal PWM control phase one.
 *
 * @return No value.
 */
void core_set_thermal_pwm_control_phase_one()

{
    /* Set thermal PWM control phase one; persistent state is carried in core thermal pwm control phase. */
    core_thermal_pwm_control_phase = 1;
    return;
}

/**
 * @brief Update identity update context.
 *
 * @return No value.
 */
uint32_t core_update_identity_update_context()

{
    /* Update identity update context; persistent state is carried in core unit identity record. */
    ((uint32_t (*)())core_calculate_record_checksum)(&core_unit_identity_record, 0x100);
    return 0;
}

/**
 * @brief Increment persistent mechanism completion count.
 *
 * @return No value.
 */
void core_increment_persistent_mechanism_completion_count()

{
    /* Increment persistent mechanism completion count; persistent state is carried in core persistent mechanism completion count. */
    core_persistent_mechanism_completion_count = core_persistent_mechanism_completion_count + 1;
    core_save_persistent_state_with_checksum();
    return;
}

/**
 * @brief Read unit identity record.
 *
 * @param context Procedure input.
 * @return No value.
 */
uint32_t core_read_unit_identity_record(context)
uint32_t context;

{
    /* Read unit identity record using only caller-provided data. */
    core_execute_spi_flash_operation(3, 0x3e000, context, 0x100);
    return 0;
}

/**
 * @brief Set transfer ready flag to one.
 *
 * @return No value.
 */
void core_set_transfer_ready_flag_to_one()

{
    /* Set transfer ready flag to one; persistent state is carried in core transfer ready flag. */
    core_transfer_ready_flag = 1;
    return;
}

/**
 * @brief Is command code in standard range.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_is_command_code_in_standard_range(condition)
uint32_t condition;

{
    /* Is command code in standard range using only caller-provided data. */
    uint32_t operation_result;

    operation_result = 0;
    if ((0x35 < condition) && (condition < 0xb6)) {
        operation_result = 1;
    }
    return operation_result;
}

/**
 * @brief Update enabled.
 *
 * @param state_record Procedure input.
 * @param enabled Procedure input.
 * @return No value.
 */
void core_update_enabled(state_record, enabled) int state_record;
int enabled;

{
    /* Update enabled using only caller-provided data. */
    if ((enabled != 0) && (*(char *)(state_record + 7) == '\0')) {
        *(uint8_t *)(state_record + 7) = 1;
    }
    *(char *)(state_record + 5) = (char)enabled;
    return;
}

/**
 * @brief Mark record active with code eight.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void core_mark_record_active_with_code_eight(offset) int offset;

{
    /* Mark record active with code eight using only caller-provided data. */
    if (*(char *)(offset + 7) == '\0') {
        *(uint8_t *)(offset + 7) = 1;
    }
    *(uint8_t *)(offset + 9) = 8;
    return;
}

/**
 * @brief Start PWM channel.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_start_pwm_channel(context) int context;

{
    /* Start PWM channel using only caller-provided data. */
    *(uint32_t *)(context * 0x400 + -0x877f4) = 1;
    return;
}

/**
 * @brief Get flash validation context for flash validation status.
 *
 * @return Result produced by the procedure.
 */
bool core_get_flash_validation_context_for_flash_validation_status()

{
    /* Get flash validation context for flash validation status using only caller-provided data. */
    int core_get_identity_variant_code_result;

    core_get_identity_variant_code_result = core_get_identity_variant_code();
    return core_get_identity_variant_code_result != 0x36;
}

/**
 * @brief Pack context value pair.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
uint64_t core_pack_context_value_pair(context, value)
uint32_t context;
uint32_t value;

{
    /* Pack context value pair using only caller-provided data. */
    uint32_t core_get_ti_memset_derived;

    core_get_ti_memset_derived = ((uint32_t (*)())core_get_ti_memset)();
    return CONCAT44(value, core_get_ti_memset_derived);
}

/**
 * @brief Mask USB interrupt.
 *
 * @return No value.
 */
void core_mask_usb_interrupt()

{
    /* Mask USB interrupt; persistent state is carried in VIM request mask clear zero register. */
    /* Program VIM request mask clear zero register with 0x200000; write ordering is hardware-significant. */
    vim_request_mask_clear_zero_register = 0x200000;
    return;
}

/**
 * @brief Defer mww dispatch and mask USB interrupt.
 *
 * @return No value.
 */
void core_defer_mww_dispatch_and_mask_usb_interrupt()

{
    /* Defer mww dispatch and mask USB interrupt; persistent state is carried in core mww dispatch deferred. */
    core_mww_dispatch_deferred = 1;
    core_mask_usb_interrupt();
    return;
}

/**
 * @brief Mark mww command received.
 *
 * @return No value.
 */
void core_mark_mww_command_received()

{
    /* Mark mww command received; persistent state is carried in core mww command received flag. */
    core_mww_command_received_flag = 1;
    return;
}

/**
 * @brief Claim device command for mww.
 *
 * @return No value.
 */
void core_claim_device_command_for_mww()

{
    /* Update usb in endpoint info for MWW handle transfer; persistent state is carried in core mww device command active. */
    core_mww_device_command_active = 1;
    core_claim_device_command();
    return;
}

/**
 * @brief Set notification state to one.
 *
 * @return No value.
 */
void core_set_notification_state_to_one()

{
    /* Set notification state to one; persistent state is carried in core notification state. */
    core_notification_state = 1;
    return;
}

/**
 * @brief Set notification state to zero for notify change resource.
 *
 * @return No value.
 */
void core_set_notification_state_to_zero_for_notify_change_resource()

{
    /* Set notification state to zero for notify change resource; persistent state is carried in core notification state. */
    core_notification_state = 0;
    return;
}

/**
 * @brief Get validation flags.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_validation_flags()

{
    /* Get validation flags; persistent state is carried in core validation flags. */
    return (core_validation_flags & 0x3f) >> 5;
}

/**
 * @brief Is command phase zero.
 *
 * @return Result produced by the procedure.
 */
bool core_is_command_phase_zero()

{
    /* Is command phase zero; persistent state is carried in core command phase. */
    return core_command_phase == 0;
}

/**
 * @brief Get command phase flags.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_command_phase_flags(context)
uint32_t context;

{
    /* Get command phase flags; persistent state is carried in core command phase flags. */
    return context & core_command_phase_flags & 0xff;
}

/**
 * @brief Clear selected sense fields.
 *
 * @return No value.
 */
void core_clear_selected_sense_fields()

{
    /* Clear selected sense fields; persistent state is carried in core selected sense status byte. */
    ti_memset(&core_selected_sense_status_byte, 0, 4);
    return;
}

/**
 * @brief Set command phase flags.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_command_phase_flags(context) uint32_t context;

{
    /* Set command phase flags; persistent state is carried in core command phase flags. */
    core_command_phase_flags = context | core_command_phase_flags;
    return;
}

/**
 * @brief Process persistent identity record for notify change resource.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_process_persistent_identity_record_for_notify_change_resource(context) uint8_t context;

{
    /* Process persistent identity record for notify change resource using only caller-provided data. */
    int core_get_persistent_identity_record_pointer_result;

    core_get_persistent_identity_record_pointer_result = (int)core_get_persistent_identity_record_pointer();
    *(uint8_t *)(core_get_persistent_identity_record_pointer_result + 0xd) = context;
    core_save_persistent_state_with_checksum();
    return;
}

/**
 * @brief Increment USB event counter.
 *
 * @return No value.
 */
void core_increment_usb_event_counter()

{
    /* Increment USB event counter; persistent state is carried in core USB event counter. */
    core_usb_event_counter = core_usb_event_counter + 1;
    return;
}

/**
 * @brief Clear USB event counters.
 *
 * @return No value.
 */
void core_clear_usb_event_counters()

{
    /* Clear USB event counters; persistent state is carried in core USB event counter. */
    ti_memset(&core_usb_event_counter, 0, 0x14);
    return;
}

/**
 * @brief Set USB test mode.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_usb_test_mode(context) int context;

{
    /* Set USB test mode; persistent state is carried in USB device control register. */
    core_update_masked_register_bits(&usb_device_control_register, 0x1e, context << 1);
    return;
}

/**
 * @brief Get available primary mask.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_available_primary_mask(context)
uint32_t context;

{
    /* Get available primary mask using only caller-provided data. */
    uint64_t core_read_descriptor_field_24_value_result;

    core_read_descriptor_field_24_value_result = core_read_descriptor_field_24_value(context, context);
    return (uint32_t)core_read_descriptor_field_24_value_result & ~*(uint32_t *)((int)((uint64_t)core_read_descriptor_field_24_value_result >> 0x20) + 0x74);
}

/**
 * @brief Get available secondary mask.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_available_secondary_mask(context)
uint32_t context;

{
    /* Get available secondary mask using only caller-provided data. */
    uint64_t core_read_descriptor_field_20_value_result;

    core_read_descriptor_field_20_value_result = core_read_descriptor_field_20_value(context, context);
    return (uint32_t)core_read_descriptor_field_20_value_result & ~*(uint32_t *)((int)((uint64_t)core_read_descriptor_field_20_value_result >> 0x20) + 0x78);
}

/**
 * @brief Saturating add u32.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_saturating_add_u32(condition, value)
uint32_t condition;
int value;

{
    /* Saturating add u32 using only caller-provided data. */
    uint32_t value_derived;

    value_derived = value + condition;
    if (value_derived < condition) {
        value_derived = 0xffffffff;
    }
    return value_derived;
}

/**
 * @brief Read primary persistent state.
 *
 * @param context Procedure input.
 * @return No value.
 */
uint32_t core_read_primary_persistent_state(context)
uint32_t context;

{
    /* Read primary persistent state using only caller-provided data. */
    core_execute_spi_flash_operation(3, 0x3f000, context, 0x40);
    return 0;
}

/**
 * @brief Read fallback persistent state.
 *
 * @param context Procedure input.
 * @return No value.
 */
uint32_t core_read_fallback_persistent_state(context)
uint32_t context;

{
    /* Read fallback persistent state using only caller-provided data. */
    core_execute_spi_flash_operation(3, 0x3fc00, context, 0x40);
    return 0;
}

/**
 * @brief Process device status update.
 *
 * @return No value.
 */
void core_process_device_status_update()

{
    /* Process device status update using only caller-provided data. */
    ((void (*)())core_reset_flash_validation_context)();
    core_update_device_status_value(1);
    return;
}
