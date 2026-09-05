/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
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
 * @brief Get cartridge ATA device info.
 *
 * @return Result produced by the procedure.
 */
uint8_t *core_get_cartridge_ata_device_info()

{
    /* Get cartridge ATA device info; persistent state is carried in core cartridge ata device info. */
    return (uint8_t *)&core_cartridge_ata_device_info;
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
 * @brief Is ATA standby active.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_is_ata_standby_active()

{
    /* Is ATA standby active; persistent state is carried in core ata standby active. */
    return core_ata_standby_active;
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
 * @brief Get primary sata link speed.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_primary_sata_link_speed()

{
    /* Get primary sata link speed using only caller-provided data. */
    uint8_t core_get_sata_link_speed_result;

    core_get_sata_link_speed_result = core_get_sata_link_speed(0);
    return core_get_sata_link_speed_result;
}

/**
 * @brief Reset AHCI port initialization state.
 *
 * @return No value.
 */
void core_reset_ahci_port_initialization_state()

{
    /* Reset AHCI port initialization state; persistent state is carried in core ahci port initialization state. */
    core_ahci_port_initialization_state = 0;
    return;
}

/**
 * @brief Decode word cipher delta.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_decode_word_cipher_delta(context) uint32_t context;

{
    /* Decode word cipher delta; persistent state is carried in core encoded word cipher delta. */
    ((void (*)())core_decode_indexed_bytes)(context, &core_encoded_word_cipher_delta, 4);
    return;
}

/**
 * @brief Get unit serial number.
 *
 * @return Result produced by the procedure.
 */
uint8_t *core_get_unit_serial_number()

{
    /* Get unit serial number; persistent state is carried in core unit serial number. */
    return (uint8_t *)&core_unit_serial_number;
}

/**
 * @brief Get firmware image validation active.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_firmware_image_validation_active()

{
    /* Get firmware image validation active; persistent state is carried in core firmware image validation active. */
    return core_firmware_image_validation_active;
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
 * @brief Is mww dispatch blocked.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_is_mww_dispatch_blocked()

{
    /* Is mww dispatch blocked; persistent state is carried in core mww dispatch blocked flag. */
    return core_mww_dispatch_blocked_flag;
}

/**
 * @brief Release device command.
 *
 * @return No value.
 */
void core_release_device_command()

{
    /* Release device command; persistent state is carried in core device command busy. */
    core_device_command_busy = 0;
    return;
}

/**
 * @brief Get mww command received.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_get_mww_command_received()

{
    /* Get mww command received; persistent state is carried in core mww command received flag. */
    return core_mww_command_received_flag;
}

/**
 * @brief Claim device command.
 *
 * @return No value.
 */
void core_claim_device_command()

{
    /* Claim device command; persistent state is carried in core device command busy. */
    core_device_command_busy = 1;
    return;
}

/**
 * @brief Get notification mode.
 *
 * @return Result produced by the procedure.
 */
uint32_t core_get_notification_mode()

{
    /* Get notification mode; persistent state is carried in core notification mode. */
    return core_notification_mode;
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
 * @brief Is command write blocked.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_is_command_write_blocked()

{
    /* Is command write blocked; persistent state is carried in core command write blocked flag. */
    return core_command_write_blocked_flag;
}

/**
 * @brief Get command feature state value.
 *
 * @return Result produced by the procedure.
 */
uint8_t core_scsi_write_requests_fua_state_value()

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
    core_load_persistent_notification_mode();
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
    uint32_t core_handle_scsi_security_protocol_derived;

    core_handle_scsi_security_protocol_derived = ((int (*)())core_handle_scsi_security_protocol)();
    *buffer = core_handle_scsi_security_protocol_derived;
    return;
}

/**
 * @brief Set selected sense asc ascq.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_selected_sense_asc_ascq(context) uint16_t context;

{
    /* Set selected sense asc ascq; persistent state is carried in core selected sense asc ascq. */
    core_selected_sense_asc_ascq = context;
    return;
}

/**
 * @brief Set selected sense status.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_selected_sense_status(context) uint8_t context;

{
    /* Set selected sense status; persistent state is carried in core selected sense status byte. */
    core_selected_sense_status_byte = context;
    return;
}

/**
 * @brief Set selected sense key.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_selected_sense_key(context) uint8_t context;

{
    /* Set selected sense key; persistent state is carried in core selected sense key. */
    core_selected_sense_key = context;
    return;
}

/**
 * @brief Set command sense table index.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_set_command_sense_table_index(context) uint32_t context;

{
    /* Set command sense table index; persistent state is carried in core command sense table index. */
    core_command_sense_table_index = context;
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
 * @brief Request system reset.
 *
 * @return No value.
 */
void core_request_system_reset()

{
    /* Request system reset using only caller-provided data. */
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
    mww_send_usb_endpoint_command(context, 2, value, 0);
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
 * @brief Register USB BOT reset callback.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_register_usb_bot_reset_callback(context) uint32_t context;

{
    /* Register USB BOT reset callback; persistent state is carried in core usb bot reset callback. */
    core_usb_bot_reset_callback = (firmware_callback_t)context;
    return;
}

/**
 * @brief Register USB ep0 out transfer callback.
 *
 * @param context Procedure input.
 * @return No value.
 */
void core_register_usb_ep0_out_transfer_callback(context) uint32_t context;

{
    /* Register USB ep0 out transfer callback; persistent state is carried in core usb ep0 out transfer callback. */
    core_usb_ep0_out_transfer_callback = (firmware_callback_t)context;
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
    int core_get_ata_discovery_flags_result;

    core_get_ata_discovery_flags_result = (int)core_get_ata_discovery_flags();
    return *(uint8_t *)(core_get_ata_discovery_flags_result + 5);
}

/**
 * @brief Run device address transfer.
 *
 * @return No value.
 */
void core_run_device_address_transfer()

{
    /* Run device address transfer using only caller-provided data. */
    ((uint32_t (*)())core_modular_exponentiation_multiword)();
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
 * @brief Disable bus dynamic clock gating.
 *
 * @return No value.
 */
void core_disable_bus_dynamic_clock_gating()

{
    /* Disable bus dynamic clock gating using only caller-provided data. */
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
 * @brief Wait SPI transmit ready.
 *
 * @return No value.
 */
void core_wait_spi_transmit_ready()

{
    /* Wait SPI transmit ready using only caller-provided data. */
    spi_wait_for_flag(0x200);
    return;
}

