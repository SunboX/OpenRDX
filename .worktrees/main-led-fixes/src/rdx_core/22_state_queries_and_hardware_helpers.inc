/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Get x02.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_get_x02(condition)
int condition;

{
    /* Get x02 using only caller-provided data. */
    uint32_t operation_result;

    if (*(char *)(condition + 6) == '\x02') {
        operation_result = 100;
    } else {
        operation_result = 500;
    }
    return operation_result;
}

/**
 * @brief Update masked register bits.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void core_update_masked_register_bits(buffer, value, length) uint32_t *buffer;
uint32_t value;
uint32_t length;

{
    /* Update masked register bits using only caller-provided data. */
    *buffer = length & value | *buffer & ~value;
    return;
}

/**
 * @brief Update device request update.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_device_request_update(context) uint32_t context;

{
    /* Update device request update; persistent state is carried in core device request update. */
    core_device_request_update = context;
    return;
}

/**
 * @brief Repack shifted value.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_repack_shifted_value(context)
uint32_t context;

{
    /* Repack shifted value using only caller-provided data. */
    return (int)context >> 8 & 0xffffU | (context & 0xff) << 8;
}

/**
 * @brief Get sense selection flags value.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_sense_selection_flags_value()

{
    /* Get sense selection flags value; persistent state is carried in core sense selection flags. */
    return core_sense_selection_flags;
}

/**
 * @brief Get ATA command buffer.
 *
 * @return Result produced by the procedure.
 */
uint8_t *core_get_ata_command_buffer()

{
    /* Get ATA command buffer; persistent state is carried in core ATA command buffer. */
    return (uint8_t *)&core_ata_command_buffer;
}

/**
 * @brief Get cartridge monitor context.
 *
 * @return Result produced by the procedure.
 */
uint32_t *core_get_cartridge_monitor_context()

{
    /* Get cartridge monitor context; persistent state is carried in core cartridge monitor context. */
    return (uint32_t *)&core_cartridge_monitor_context;
}

/**
 * @brief Get gpio pin context value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_gpio_pin_context_value()

{
    /* Get gpio pin context value; persistent state is carried in core gpio pin context. */
    return core_gpio_pin_context;
}

/**
 * @brief Update ti memset for ATA command command.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_update_ti_memset_for_ata_command_command(buffer) uint32_t *buffer;

{
    /* Update ti memset for ATA command command using only caller-provided data. */
    ti_memset(*buffer, 0, 0x1000);
    return;
}

/**
 * @brief Get resource AHCI state.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_resource_ahci_state()

{
    /* Get resource AHCI state using only caller-provided data. */
    uint8_t core_get_ahci_status_register_result;

    core_get_ahci_status_register_result = core_get_ahci_status_register(0);
    return core_get_ahci_status_register_result;
}

/**
 * @brief Set command runtime state to zero.
 *
 * @return No value.
 */
void core_set_command_runtime_state_to_zero()

{
    /* Set command runtime state to zero; persistent state is carried in core command runtime state. */
    core_command_runtime_state = 0;
    return;
}

/**
 * @brief Update secondary update context.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_secondary_update_context(context) uint32_t context;

{
    /* Update secondary update context; persistent state is carried in core secondary update context. */
    ((void (*)())core_transfer_x01)(context, &core_secondary_update_context, 4);
    return;
}

/**
 * @brief Get response byte context.
 *
 * @return Result produced by the procedure.
 */
uint8_t *core_get_response_byte_context()

{
    /* Get response byte context; persistent state is carried in core unit serial number. */
    return (uint8_t *)&core_unit_serial_number;
}

/**
 * @brief Get mcp3008 sample value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_mcp3008_sample_value()

{
    /* Get mcp3008 sample value; persistent state is carried in core mcp3008 sample. */
    return core_mcp3008_sample;
}

/**
 * @brief Convert nibble to hex character.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
char core_convert_nibble_to_hex_character(condition)
int condition;

{
    /* Convert nibble to hex character using only caller-provided data. */
    char condition_field;

    if (condition < 10) {
        condition_field = (char)condition + '0';
    } else {
        condition_field = (char)condition + '7';
    }
    return condition_field;
}

/**
 * @brief Update mark primary sense.
 *
 * @return No value.
 */
void core_update_mark_primary_sense()

{
    /* Update mark primary sense using only caller-provided data. */
    core_mark_primary_sense_record_not_ready(0x800f2b4);
    return;
}

/**
 * @brief Update store context enable.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_store_context_enable(context) uint32_t context;

{
    /* Update store context enable using only caller-provided data. */
    core_store_context_enable_byte(0x800f2b4, context);
    return;
}

/**
 * @brief Update drive mechanism outputs.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void core_update_drive_mechanism_outputs(condition) int condition;

{
    /* Update drive mechanism outputs using only caller-provided data. */
    uint32_t working_result;

    if (condition == 0) {
        working_result = 1;
    } else {
        working_result = 6;
    }
    core_drive_mechanism_outputs(working_result);
    return;
}

/**
 * @brief Process validation flags.
 *
 * @return No value.
 */
void core_process_validation_flags()

{
    /* Process validation flags using only caller-provided data. */
    core_get_set_validation_flags();
    core_get_validation_flags();
    return;
}

/**
 * @brief Get set validation flags.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_set_validation_flags()

{
    /* Get set validation flags using only caller-provided data. */
    core_set_validation_flags_flag_bit_five(0);
    return 1;
}

/**
 * @brief Get MWW transfer value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_mww_transfer_value()

{
    /* Get MWW transfer value; persistent state is carried in core MWW transfer value. */
    return core_mww_transfer_value;
}

/**
 * @brief Set thermal monitor enabled to zero.
 *
 * @return No value.
 */
void core_set_thermal_monitor_enabled_to_zero()

{
    /* Set thermal monitor enabled to zero; persistent state is carried in core thermal monitor enabled. */
    core_thermal_monitor_enabled = 0;
    return;
}

/**
 * @brief Get cartridge input value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_cartridge_input_value()

{
    /* Get cartridge input value; persistent state is carried in core cartridge input value. */
    return core_cartridge_input_value;
}

/**
 * @brief Set thermal monitor enabled to one.
 *
 * @return No value.
 */
void core_set_thermal_monitor_enabled_to_one()

{
    /* Set thermal monitor enabled to one; persistent state is carried in core thermal monitor enabled. */
    core_thermal_monitor_enabled = 1;
    return;
}

/**
 * @brief Get notification context value.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_notification_context_value()

{
    /* Get notification context value; persistent state is carried in core notification context. */
    return core_notification_context;
}

/**
 * @brief Get device status value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_device_status_value()

{
    /* Get device status value; persistent state is carried in core device status value. */
    return core_device_status_value;
}

/**
 * @brief Get command context value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_command_context_value()

{
    /* Get command context value; persistent state is carried in core command context value. */
    return core_command_context_value;
}

/**
 * @brief Get command feature state value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_command_feature_state_value()

{
    /* Get command feature state value; persistent state is carried in core command feature state. */
    return core_command_feature_state;
}

/**
 * @brief Get response type value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_response_type_value()

{
    /* Get response type value; persistent state is carried in core response type. */
    return core_response_type;
}

/**
 * @brief Get command buffer state value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_command_buffer_state_value()

{
    /* Get command buffer state value; persistent state is carried in core command buffer state. */
    return core_command_buffer_state;
}

/**
 * @brief Process notification context.
 *
 * @return No value.
 */
void core_process_notification_context()

{
    /* Process notification context using only caller-provided data. */
    core_update_notification_context();
    core_update_device_descriptor_context();
    return;
}

/**
 * @brief Store management dispatch result.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void core_store_management_dispatch_result(buffer) uint32_t *buffer;

{
    /* Store management dispatch result using only caller-provided data. */
    uint32_t core_handle_type_type_derived;

    core_handle_type_type_derived = ((int (*)())core_handle_type_type)();
    *buffer = core_handle_type_type_derived;
    return;
}

/**
 * @brief Update secondary sense field context.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_secondary_sense_field_context(context) uint16_t context;

{
    /* Update secondary sense field context; persistent state is carried in core secondary sense field context. */
    core_secondary_sense_field_context = context;
    return;
}

/**
 * @brief Update response buffer context.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_response_buffer_context(context) uint8_t context;

{
    /* Update response buffer context; persistent state is carried in core response buffer context. */
    core_response_buffer_context = context;
    return;
}

/**
 * @brief Update primary sense field context.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_primary_sense_field_context(context) uint8_t context;

{
    /* Update primary sense field context; persistent state is carried in core primary sense field context. */
    core_primary_sense_field_context = context;
    return;
}

/**
 * @brief Update command phase auxiliary.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_command_phase_auxiliary(context) uint32_t context;

{
    /* Update command phase auxiliary; persistent state is carried in core command phase auxiliary. */
    core_command_phase_auxiliary = context;
    return;
}

/**
 * @brief Update command feature state.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_command_feature_state(context) uint8_t context;

{
    /* Update command feature state; persistent state is carried in core command feature state. */
    core_command_feature_state = context;
    return;
}

/**
 * @brief Update device status value.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_device_status_value(context) uint8_t context;

{
    /* Update device status value; persistent state is carried in core device status value. */
    core_device_status_value = context;
    return;
}

/**
 * @brief Update command buffer state.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_command_buffer_state(context) uint8_t context;

{
    /* Update command buffer state; persistent state is carried in core command buffer state. */
    core_command_buffer_state = context;
    return;
}

/**
 * @brief Set SYSECR REG OFF to hex 8000 handler.
 *
 * @return No value.
 */
void core_set_SYSECR_REG_OFF_to_hex_8000_handler()

{
    /* Set SYSECR REG OFF to hex 8000 handler using only caller-provided data. */
    /* Program SYSECR REG OFF with 0x8000; write ordering is hardware-significant. */
    SYSECR_REG_OFF = 0x8000;
    return;
}

/**
 * @brief Update MWW process command.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_update_mww_process_command(context, value) uint32_t context;
uint16_t value;

{
    /* Update MWW process command using only caller-provided data. */
    mww_process_command_word_channel_mww_command_channel(context, 2, value, 0);
    return;
}

/**
 * @brief Get response status value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_response_status_value()

{
    /* Get response status value; persistent state is carried in core response status value. */
    return core_response_status_value;
}

/**
 * @brief Update firmware context primary firmware callback.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_firmware_context_primary_firmware_callback(context) uint32_t context;

{
    /* Update firmware context primary firmware callback; persistent state is carried in core primary firmware callback. */
    core_primary_firmware_callback = (firmware_callback_t)context;
    return;
}

/**
 * @brief Update firmware context secondary firmware callback.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_update_firmware_context_secondary_firmware_callback(context) uint32_t context;

{
    /* Update firmware context secondary firmware callback; persistent state is carried in core secondary firmware callback. */
    core_secondary_firmware_callback = (firmware_callback_t)context;
    return;
}

/**
 * @brief Clear primary descriptor mask.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_clear_primary_descriptor_mask(offset, value) int offset;
uint32_t value;

{
    /* Clear primary descriptor mask using only caller-provided data. */
    *(uint32_t *)(offset + 0x74) = *(uint32_t *)(offset + 0x74) & ~value;
    return;
}

/**
 * @brief Clear secondary descriptor mask.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_clear_secondary_descriptor_mask(offset, value) int offset;
uint32_t value;

{
    /* Clear secondary descriptor mask using only caller-provided data. */
    *(uint32_t *)(offset + 0x78) = *(uint32_t *)(offset + 0x78) & ~value;
    return;
}

/**
 * @brief Get response end context for response end word.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_response_end_context_for_response_end_word()

{
    /* Get response end context for response end word using only caller-provided data. */
    int core_get_response_end_context_pointer_result;

    core_get_response_end_context_pointer_result = (int)core_get_response_end_context_pointer();
    return *(uint8_t *)(core_get_response_end_context_pointer_result + 5);
}

/**
 * @brief Run device address transfer.
 *
 * @return No value.
 */
void core_run_device_address_transfer()

{
    /* Run device address transfer using only caller-provided data. */
    ((uint32_t (*)())core_transfer_find_last_nonzero)();
    return;
}

/**
 * @brief Get persistent identity record for notification context.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_persistent_identity_record_for_notification_context()

{
    /* Get persistent identity record for notification context using only caller-provided data. */
    int core_get_persistent_identity_record_pointer_result;

    core_get_persistent_identity_record_pointer_result = (int)core_get_persistent_identity_record_pointer();
    return *(uint8_t *)(core_get_persistent_identity_record_pointer_result + 0xd);
}

/**
 * @brief Store duplicate record value.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_store_duplicate_record_value(offset, value) int offset;
uint16_t value;

{
    /* Store duplicate record value using only caller-provided data. */
    **(uint16_t **)(offset + 0xc) = value;
    *(uint16_t *)(*(int *)(offset + 0xc) + 2) = value;
    return;
}

/**
 * @brief Set CDDIS REG OFF to hex 38 handler.
 *
 * @return No value.
 */
void core_set_CDDIS_REG_OFF_to_hex_38_handler()

{
    /* Set CDDIS REG OFF to hex 38 handler using only caller-provided data. */
    /* Program CDDIS REG OFF with 0x38; write ordering is hardware-significant. */
    CDDIS_REG_OFF = 0x38;
    return;
}

/**
 * @brief Set primary descriptor mask.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_set_primary_descriptor_mask(offset, value) int offset;
uint32_t value;

{
    /* Set primary descriptor mask using only caller-provided data. */
    *(uint32_t *)(offset + 0x74) = value | *(uint32_t *)(offset + 0x74);
    return;
}

/**
 * @brief Set secondary descriptor mask.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_set_secondary_descriptor_mask(offset, value) int offset;
uint32_t value;

{
    /* Set secondary descriptor mask using only caller-provided data. */
    *(uint32_t *)(offset + 0x78) = value | *(uint32_t *)(offset + 0x78);
    return;
}

/**
 * @brief Write four byte value.
 *
 * @param context Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
uint32_t core_write_four_byte_value(context, value)
uint32_t context;
uint32_t value;

{
    /* Write four byte value using only caller-provided data. */
    ((int (*)())core_transfer_remaining_bounds)(context, 4, value);
    return 0;
}

/**
 * @brief Set secondary descriptor value ready.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_set_secondary_descriptor_value_ready(offset, value) int offset;
uint32_t value;

{
    /* Set secondary descriptor value ready using only caller-provided data. */
    *(uint32_t *)(offset + 0x24) = value;
    *(uint8_t *)(offset + 0x12) = 2;
    return;
}

/**
 * @brief Set primary descriptor value ready.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_set_primary_descriptor_value_ready(offset, value) int offset;
uint32_t value;

{
    /* Set primary descriptor value ready using only caller-provided data. */
    *(uint32_t *)(offset + 0x20) = value;
    *(uint8_t *)(offset + 0x11) = 2;
    return;
}

/**
 * @brief Update SPI update SPI.
 *
 * @return No value.
 */
void core_update_spi_update_spi()

{
    /* Update SPI update SPI using only caller-provided data. */
    spi_wait_for_flag(0x200);
    return;
}

