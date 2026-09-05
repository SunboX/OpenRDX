/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Sha256 compress block.
 *
 * @param offset Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
uint32_t core_sha256_compress_block(offset, buffer)
int offset;
uint8_t *buffer;

{
    /* Sha256 compress block; persistent state is carried in core sha256 round constants. */
    uint8_t *buffer_cursor;
    uint8_t *third_input_byte;
    uint8_t *fourth_input_byte;
    uint8_t first_input_byte;
    uint32_t working_result;
    uint32_t offset_field_0x5c;
    uint32_t offset_field_0x50;
    uint32_t offset_field_0x60;
    uint32_t offset_field_0x4c;
    int accumulator_word;
    int *core_sha256_round_constants_pointer;
    int *concat11_result;
    int *data_cursor;
    int operation_status;
    uint32_t offset_field_0x54;
    uint32_t status_bits;
    uint32_t state_word;
    int working_state[9];
    volatile int packed_command_words[5];
    volatile uint32_t expanded_state[50];
    uint32_t offset_field_0x68;
    uint32_t state_word_g;
    uint32_t offset_field_0x58;
    volatile int assembled_word = 0;

    operation_status = 0x10;
    core_sha256_round_constants_pointer = working_state;
    concat11_result = working_state;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        first_input_byte = *buffer;
        buffer_cursor = buffer + 1;
        third_input_byte = buffer + 2;
        fourth_input_byte = buffer + 3;
        operation_status = operation_status + -1;
        buffer = buffer + 4;
        data_cursor = concat11_result + 1;
        *concat11_result = CONCAT31(CONCAT21(CONCAT11(first_input_byte, *buffer_cursor), *third_input_byte), *fourth_input_byte);
        concat11_result = data_cursor;
    } while (operation_status != 0);
    operation_status = 0x30;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        offset_field_0x5c = core_sha256_round_constants_pointer[0xe];
        offset_field_0x4c = core_sha256_round_constants_pointer[1];
        concat11_result = core_sha256_round_constants_pointer + 9;
        accumulator_word = *core_sha256_round_constants_pointer;
        operation_status = operation_status + -1;
        core_sha256_round_constants_pointer = core_sha256_round_constants_pointer + 1;
        *data_cursor = accumulator_word +
                            ((offset_field_0x4c >> 7 | offset_field_0x4c << 0x19) ^ (offset_field_0x4c >> 0x12 | offset_field_0x4c << 0xe) ^
                             offset_field_0x4c >> 3) +
                            *concat11_result +
                            ((offset_field_0x5c >> 0x11 | offset_field_0x5c << 0xf) ^ (offset_field_0x5c >> 0x13 | offset_field_0x5c << 0xd) ^
                             offset_field_0x5c >> 10);
        data_cursor = data_cursor + 1;
    } while (operation_status != 0);
    operation_status = 0x40;
    offset_field_0x58 = *(uint32_t *)(offset + 0x58);
    state_word_g = *(uint32_t *)(offset + 100);
    offset_field_0x68 = *(uint32_t *)(offset + 0x68);
    offset_field_0x4c = *(uint32_t *)(offset + 0x4c);
    offset_field_0x5c = *(uint32_t *)(offset + 0x5c);
    concat11_result = working_state;
    offset_field_0x50 = *(uint32_t *)(offset + 0x50);
    offset_field_0x60 = *(uint32_t *)(offset + 0x60);
    core_sha256_round_constants_pointer = (int *)&core_sha256_round_constants;
    offset_field_0x54 = *(uint32_t *)(offset + 0x54);
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        state_word = offset_field_0x54;
        status_bits = offset_field_0x60;
        offset_field_0x54 = offset_field_0x50;
        offset_field_0x60 = offset_field_0x5c;
        offset_field_0x50 = offset_field_0x4c;
        offset_field_0x5c = offset_field_0x58;
        working_result = state_word_g;
        assembled_word = *concat11_result;
        operation_status = operation_status + -1;
        accumulator_word = offset_field_0x68 + *concat11_result + *core_sha256_round_constants_pointer +
                         (state_word_g & ~offset_field_0x60 ^ status_bits & offset_field_0x60) +
                         ((offset_field_0x60 >> 6 | offset_field_0x60 << 0x1a) ^ (offset_field_0x60 >> 0xb | offset_field_0x60 << 0x15) ^
                          (offset_field_0x60 >> 0x19 | offset_field_0x60 << 7));
        state_word_g = status_bits;
        offset_field_0x68 = working_result;
        offset_field_0x58 = state_word;
        offset_field_0x5c = offset_field_0x5c + accumulator_word;
        offset_field_0x4c =
            (state_word & offset_field_0x54 ^ state_word & offset_field_0x50 ^ offset_field_0x54 & offset_field_0x50) +
            ((offset_field_0x50 >> 2 | offset_field_0x50 << 0x1e) ^ (offset_field_0x50 >> 0xd | offset_field_0x50 << 0x13) ^
             (offset_field_0x50 >> 0x16 | offset_field_0x50 << 10)) +
            accumulator_word;
        concat11_result = concat11_result + 1;
        core_sha256_round_constants_pointer = core_sha256_round_constants_pointer + 1;
    } while (operation_status != 0);
    *(uint32_t *)(offset + 0x4c) = offset_field_0x4c + *(int *)(offset + 0x4c);
    *(uint32_t *)(offset + 0x5c) = offset_field_0x5c + *(int *)(offset + 0x5c);
    *(uint32_t *)(offset + 0x50) = *(int *)(offset + 0x50) + offset_field_0x50;
    *(uint32_t *)(offset + 0x54) = *(int *)(offset + 0x54) + offset_field_0x54;
    *(uint32_t *)(offset + 0x60) = offset_field_0x60 + *(int *)(offset + 0x60);
    *(uint32_t *)(offset + 0x58) = state_word + *(int *)(offset + 0x58);
    *(uint32_t *)(offset + 100) = status_bits + *(int *)(offset + 100);
    *(uint32_t *)(offset + 0x68) = working_result + *(int *)(offset + 0x68);
    return 0;
}

/**
 * @brief Handle SCSI read capacity.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void core_handle_scsi_read_capacity(buffer, value, length, flags) uint32_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Handle SCSI read capacity; persistent state is carried in core capacity saturation high word, core capacity high word threshold. */
    int operation_status;
    uint32_t core_initialize_scsi_response_buffer_result;
    uint32_t working_result;
    uint32_t callback_result = 0;
    uint32_t core_transfer_constant_0x13_result;
    uint32_t core_process_ata_command_buffer_for_build_scsi_mode_result;
    uint32_t response_end;
    uint32_t zero_high_word = 0;
    uint8_t condition_met;
    uint32_t status_bits;
    uint32_t *core_capacity_saturation_high_word_pointer;
    bool core_capacity_high_word_threshold_pointer;
    int64_t bit_position;

    operation_status = core_get_runtime_context_base_for_build_scsi_mode();
    bit_position = core_get_structure_field_hex_for_response_end_word(operation_status + 0x2c);
    operation_status = core_get_response_end_context_for_response_end_word();
    core_process_ata_command_buffer_for_build_scsi_mode_result = callback_result;
    core_transfer_constant_0x13_result = zero_high_word;
    if (operation_status != 0) {
        operation_status = (int)core_get_cartridge_ata_device_info();
        core_transfer_constant_0x13_result = (uint32_t)bit_position - (uint32_t)*(uint16_t *)(operation_status + 0x14);
        core_process_ata_command_buffer_for_build_scsi_mode_result = core_transfer_constant_0x13_result - 7;
        core_transfer_constant_0x13_result =
            (int)((uint64_t)bit_position >> 0x20) +
            (-(uint32_t)(core_transfer_constant_0x13_result < 7) - (uint32_t)((uint32_t)bit_position < (uint32_t)*(uint16_t *)(operation_status + 0x14)));
        bit_position = CONCAT44(core_transfer_constant_0x13_result >> 3, core_process_ata_command_buffer_for_build_scsi_mode_result >> 3 | core_transfer_constant_0x13_result * 0x20000000);
    }
    core_capacity_saturation_high_word_pointer = (uint32_t *)(uint32_t)((uint64_t)bit_position >> 0x20);
    status_bits = (uint32_t)bit_position;
    if (*(int8_t *)buffer[2] == -0x62) {
        core_initialize_scsi_response_buffer_result = 0x20;
    } else {
        core_initialize_scsi_response_buffer_result = 8;
    }
    core_initialize_scsi_response_buffer_result = (uint32_t)core_initialize_scsi_response_buffer(buffer, core_initialize_scsi_response_buffer_result, core_process_ata_command_buffer_for_build_scsi_mode_result, core_transfer_constant_0x13_result, flags);
    core_process_ata_command_buffer_for_build_scsi_mode_result = core_process_ata_command_buffer_for_build_scsi_mode();
    core_process_ata_command_buffer_for_build_scsi_mode_result = core_process_ata_command_buffer_for_build_scsi_mode_result & 0xffff;
    if (*(char *)buffer[2] == '%') {
        if (core_capacity_saturation_high_word_pointer != (uint32_t *)0x0) {
            status_bits = 0xffffffff;
        }
        core_copy_bounded_response_field(core_initialize_scsi_response_buffer_result, 0, 3, status_bits, 0);
        working_result = 4;
        response_end = 7;
        goto core_read_capacity_write_final_field;
    }
    if (*(int8_t *)buffer[2] != -0x62) {
        *buffer = 8;
        return;
    }
    operation_status = (int)core_get_cartridge_ata_device_info();
    condition_met = *(uint8_t *)(operation_status + 0x13);
    core_update_release_record_payload(core_initialize_scsi_response_buffer_result, buffer, 10, 0xd);
    core_capacity_high_word_threshold_pointer = &core_capacity_high_word_threshold < core_capacity_saturation_high_word_pointer;
    if (core_capacity_saturation_high_word_pointer == (uint32_t *)&core_capacity_saturation_high_word) {
        core_capacity_high_word_threshold_pointer = 0xfffffffd < status_bits;
    }
    if (!core_capacity_high_word_threshold_pointer || bit_position == -2) {
        buffer = core_capacity_saturation_high_word_pointer;
    }
    if (core_capacity_high_word_threshold_pointer && (core_capacity_saturation_high_word_pointer != (uint32_t *)&core_capacity_saturation_high_word || status_bits != 0xfffffffe)) {
        status_bits = 0xfffffffe;
        buffer = (uint32_t *)&core_capacity_saturation_high_word;
    }
    core_copy_bounded_response_field(core_initialize_scsi_response_buffer_result, 0, 7, status_bits, buffer);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 8, 0xb, core_process_ata_command_buffer_for_build_scsi_mode_result);
    operation_status = core_get_response_end_context_for_response_end_word();
    if (operation_status != 0) {
        if (condition_met < 3) {
            condition_met = 0;
        } else {
            condition_met = condition_met - 3;
        }
    }
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0xd, 0xd, condition_met);
    operation_status = core_get_response_end_context_for_response_end_word();
    if (operation_status == 0) {
        core_get_cartridge_ata_device_info();
        core_transfer_constant_0x13_result = core_transfer_constant_0x13();
        operation_status = (int)core_get_cartridge_ata_device_info();
        status_bits = (uint32_t)*(uint16_t *)(operation_status + 0x14);
        core_process_ata_command_buffer_for_build_scsi_mode_result = core_transfer_constant_0x13_result - status_bits;
        if ((status_bits == 0) || (core_transfer_constant_0x13_result < status_bits))
            goto core_read_capacity_clear_alignment_offset;
    } else {
    /* Shared target for core read capacity clear alignment offset; incoming paths preserve the same state assumptions. */
    core_read_capacity_clear_alignment_offset:
        core_process_ata_command_buffer_for_build_scsi_mode_result = 0;
    }
    working_result = 0xe;
    response_end = 0xf;
/* Shared target for core read capacity write final field; incoming paths preserve the same state assumptions. */
core_read_capacity_write_final_field:
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, working_result, response_end, core_process_ata_command_buffer_for_build_scsi_mode_result);
    return;
}

/**
 * @brief Sha256 finalize.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void core_sha256_finalize(offset, value, condition) int offset;
int value;
int condition;

{
    /* Sha256 finalize using only caller-provided data. */
    uint32_t offset_field_0x40;
    int status_bits;
    uint32_t core_sha256_compress_block_derived;
    uint8_t *computed_offset;
    uint32_t offset_field_0x44;
    int operation_status;

    offset_field_0x40 = *(uint32_t *)(offset + 0x40);
    if (offset_field_0x40 < 0x38) {
        computed_offset = (uint8_t *)(offset + offset_field_0x40);
        *computed_offset = 0x80;
        if (offset_field_0x40 + 1 < 0x38) {
            status_bits = 0x38 - (offset_field_0x40 + 1);
            /* Repeat while status_bits != 0; the body advances or polls the state needed to leave the loop. */
            do {
                computed_offset = computed_offset + 1;
                status_bits = status_bits + -1;
                *computed_offset = 0;
            } while (status_bits != 0);
        }
    } else {
        *(uint8_t *)(offset + offset_field_0x40) = 0x80;
        offset_field_0x40 = offset_field_0x40 + 1;
        if (offset_field_0x40 < 0x40) {
            status_bits = 0x40 - offset_field_0x40;
            computed_offset = (uint8_t *)(offset + offset_field_0x40);
            /* Repeat while status_bits != 0; the body advances or polls the state needed to leave the loop. */
            do {
                status_bits = status_bits + -1;
                *computed_offset = 0;
                computed_offset = computed_offset + 1;
            } while (status_bits != 0);
        }
        core_sha256_compress_block_derived = ((uint32_t (*)())core_sha256_compress_block)(offset, offset);
        ti_memset(core_sha256_compress_block_derived, 0, 0x38);
    }
    offset_field_0x44 = *(uint32_t *)(offset + 0x44);
    offset_field_0x40 = *(int *)(offset + 0x40) * 8;
    *(uint32_t *)(offset + 0x44) = offset_field_0x40 + offset_field_0x44;
    *(uint32_t *)(offset + 0x48) = *(int *)(offset + 0x48) + (uint32_t)CARRY4(offset_field_0x40, offset_field_0x44);
    *(uint8_t *)(offset + 0x3f) = *(uint8_t *)(offset + 0x44);
    *(char *)(offset + 0x3e) = (char)((uint32_t)*(uint32_t *)(offset + 0x44) >> 8);
    *(char *)(offset + 0x3d) = (char)((uint32_t)*(uint32_t *)(offset + 0x44) >> 0x10);
    *(char *)(offset + 0x3c) = (char)((uint32_t)*(uint32_t *)(offset + 0x44) >> 0x18);
    *(char *)(offset + 0x3b) = (char)*(uint32_t *)(offset + 0x48);
    *(char *)(offset + 0x3a) = (char)((uint32_t)*(uint32_t *)(offset + 0x48) >> 8);
    *(char *)(offset + 0x39) = (char)((uint32_t)*(uint32_t *)(offset + 0x48) >> 0x10);
    *(char *)(offset + 0x38) = (char)((uint32_t)*(uint32_t *)(offset + 0x48) >> 0x18);
    ((uint32_t (*)())core_sha256_compress_block)(offset);
    offset_field_0x40 = 0;
    /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
    do {
        operation_status = 8;
        status_bits = 0;
        /* Repeat while operation_status != 0; the body advances or polls the state needed to leave the loop. */
        do {
            offset_field_0x44 = offset_field_0x40 + status_bits & 0xff;
            if ((int)offset_field_0x44 < condition) {
                *(char *)(value + offset_field_0x44) =
                    (char)(*(uint32_t *)(offset + status_bits + 0x4c) >> (0x18U - offset_field_0x40 * 8U & 0xff));
            }
            operation_status = operation_status + -1;
            status_bits = status_bits + 4;
        } while (operation_status != 0);
        offset_field_0x40 = offset_field_0x40 + 1;
    } while (offset_field_0x40 < 4);
    return;
}

/**
 * @brief Prepare runtime services hook.
 *
 * @return No value.
 */
void core_prepare_runtime_services_hook()

{
    /* Prepare runtime services hook using only caller-provided data. */
    return;
}

/**
 * @brief Transition flash validation context.
 *
 * @param buffer Procedure input.
 * @param condition Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transition_flash_validation_context(buffer, condition, length, flags)
int *buffer;
int condition;
uint32_t length;
int flags;

{
    /* Transition flash validation context; persistent state is carried in core identity record variant code, core identity record offset 7a bytes and related record fields. */
    int working_result;
    uint8_t *data_cursor;
    uint32_t core_process_identity_update_context_result;
    uint32_t mode_value;
    int status_bits;

    status_bits = flags;
    if (condition == 2) {
        /* Record type 2 carries the main identity block; its version controls which tail fields exist. */
        mode_value = buffer[7];
        if (5 < mode_value) {
            /* Version 6 adds the validation selector and three 16-byte identity extensions. */
            core_identity_record_variant_code = (uint16_t)buffer[0x1e];
            core_copy_bytes(&core_identity_record_offset_7a_bytes, (int)buffer + 0x7a, 0x10);
            core_copy_bytes(&core_identity_record_offset_8a_bytes, (int)buffer + 0x8a, 0x10);
            core_copy_bytes(&core_identity_record_offset_9a_bytes, (int)buffer + 0x9a, 0x10);
        }
        if (4 < mode_value) {
            /* Version 5 adds the eight-byte auxiliary identity field. */
            core_copy_bytes(&core_identity_record_offset_70_bytes, buffer + 0x1c, 8);
        }
        if (3 < mode_value) {
            /* Version 4 adds two words and preserves the parsed record version. */
            core_identity_record_offset_68_word = buffer[0x1a];
            core_identity_record_offset_6c_word = buffer[0x1b];
            core_identity_record_version = mode_value;
        }
        core_identity_record_offset_20_word = buffer[8];
        core_identity_record_offset_24_word = buffer[9];
        core_copy_bytes(&core_unit_serial_number, buffer + 4, 10);
        /* Select the version-independent payload segment copied after the common fields. */
        data_cursor = (uint8_t *)&core_identity_record_offset_28_bytes;
        working_result = 0x28;
        core_process_identity_update_context_result = 0x40;
    } else {
        if (1 < condition - 3U) {
            if (condition - 3U != 7) {
                return 1;
            }
            /* Record type 10 authenticates its first word against the defined firmware constant. */
            status_bits = 0;
            core_copy_bytes(&status_bits, 0x800e744, 4);
            status_bits = core_accumulate_image_byte_lanes(buffer + 1, 0x1c, status_bits);
            /* Gate the following state transition on *buffer != status_bits; the handler is skipped when this condition is false. */
            if (*buffer != status_bits) {
                return 1;
            }
            core_copy_bytes(&core_compact_identity_vendor_identifier, buffer + 2, 8);
            /* Once verified, import the compact serial/identity tuple into persistent globals. */
            core_copy_bytes(&core_compact_identity_product_identifier, buffer + 4, 0x10);
            core_compact_identity_offset_four_word = (uint16_t)buffer[1];
            core_compact_identity_offset_six_word = *(uint16_t *)((int)buffer + 6);
            goto core_identity_import_commit_fields;
        }
        core_copy_bytes(&core_unit_product_identifier, buffer + 7, 0x10);
        /* Types 3 and 4 share a smaller 16-byte payload followed by an eight-byte field. */
        working_result = 0x10;
        data_cursor = (uint8_t *)&core_unit_identity_offset_10_bytes;
        core_process_identity_update_context_result = 8;
    }
    core_copy_bytes(data_cursor, (int)buffer + working_result, core_process_identity_update_context_result);
/* Shared target for core identity import commit fields; incoming paths preserve the same state assumptions. */
core_identity_import_commit_fields:
    /* Commit all imported fields through the common identity-update path. */
    core_process_identity_update_context_result = core_process_identity_update_context();
    return core_process_identity_update_context_result;
}

/**
 * @brief Transfer descriptor capacity previous descriptor address descriptor queue context.
 *
 * @param transfer_direction Procedure input.
 * @param remaining_bytes Procedure input.
 * @param command_parameter Command-specific value stored with the transfer.
 * @return Result produced by the procedure.
 */
uint64_t core_transfer_descriptor_capacity_previous_descriptor_address_descriptor_queue_context(transfer_direction, remaining_bytes, command_parameter)
int transfer_direction;
uint32_t remaining_bytes;
uint32_t command_parameter;

{
    /* Transfer descriptor capacity previous descriptor address descriptor queue context; persistent state is carried in usb current configuration, USB core revision and related record fields. */
    int working_result;
    uint32_t condition_3_field;
    uint32_t *data_cursor;
    uint32_t *descriptor_base;
    uint32_t *usb_current_configuration_pointer;
    uint32_t status_bits;
    uint32_t chain_flag;
    uint32_t completion_mask;
    uint32_t condition_2_field;
    uint32_t base_command_flags;
    int previous_descriptor_address;
    uint32_t condition_1_field;
    uint32_t operation_status;
    uint32_t descriptor_capacity;
    uint32_t *usb_core_revision_pointer;

    /* Descriptor-queue mode changes the completion bits written into each TRB. */
    if (usb_active_mass_storage_class == 0) {
        completion_mask = 0x20000;
    } else {
        completion_mask = 0x30000;
    }
    condition_1_field = 0;
    if (transfer_direction == 0) {
        /* OUT transfers use eight descriptors and split data into 128 KiB chunks. */
        working_result = 0x1c0;
        operation_status = 0x20000;
        descriptor_capacity = 8;
        descriptor_base = (uint32_t *)&usb_trb_ring_out;
    } else {
        /* IN transfers use ten descriptors and the mode-dependent 16/32 KiB chunk limit. */
        working_result = 0x184;
        descriptor_base = (uint32_t *)&usb_trb_ring_in;
        if (usb_active_mass_storage_class == 0) {
            operation_status = 0x8000;
        } else {
            operation_status = 0x4000;
        }
        descriptor_capacity = 10;
    }
    condition_3_field = (command_parameter & 0xffff) << 0xe;
    /* Preserve the command parameter in the TRB control word beside the defined base flags. */
    base_command_flags = condition_3_field | 0x811;
    data_cursor = descriptor_base;
    usb_core_revision_pointer = (uint32_t *)((int)&usb_core_revision + working_result);
    /* Repeat while (remaining_bytes != 0) && (data_cursor = usb_current_configuration_pointer + 4, condition_1_field < descriptor_capacity); the body advances or polls the state needed to leave the loop. */
    do {
        usb_current_configuration_pointer = data_cursor;
        usb_core_revision_pointer = usb_core_revision_pointer + 1;
        condition_2_field = remaining_bytes;
        if (operation_status <= remaining_bytes) {
            condition_2_field = operation_status;
        }
        /* Clear stale completion state before publishing this segment to hardware. */
        *(uint32_t *)(&usb_current_configuration + working_result) = ~completion_mask & *(uint32_t *)(&usb_current_configuration + working_result);
        remaining_bytes = remaining_bytes - condition_2_field;
        status_bits = base_command_flags;
        if ((((usb_core_revision == 0x120a) && (command_parameter != 0)) && (condition_1_field == 0)) && (transfer_direction == 0)) {
            /* Revision 0x120a clears the low control bit on the first parameterized OUT TRB. */
            status_bits = condition_3_field | 0x810;
        }
        if (remaining_bytes == 0) {
            /* Mark the final data TRB; otherwise keep the chain open for the next segment. */
            chain_flag = 2;
        } else {
            chain_flag = 4;
        }
        *usb_current_configuration_pointer = *(uint32_t *)(&usb_current_configuration + working_result);
        condition_1_field = condition_1_field + 1;
        usb_current_configuration_pointer[1] = 0;
        usb_current_configuration_pointer[2] = condition_2_field;
        previous_descriptor_address = *(int *)(&usb_current_configuration + working_result);
        usb_current_configuration_pointer[3] = chain_flag | status_bits;
        *(uint32_t *)(&usb_current_configuration + working_result) = condition_2_field + previous_descriptor_address;
        *usb_core_revision_pointer = condition_2_field;
    } while ((remaining_bytes != 0) && (data_cursor = usb_current_configuration_pointer + 4, condition_1_field < descriptor_capacity));
    if (descriptor_capacity == condition_1_field) {
        /* A full ring ends with a link TRB that wraps execution back to the ring base. */
        usb_current_configuration_pointer[4] = (uint32_t)descriptor_base;
        usb_current_configuration_pointer[5] = 0;
        usb_current_configuration_pointer[6] = 0;
        usb_current_configuration_pointer[7] = 0x81;
        condition_1_field = 0;
    }
    *(uint32_t *)(&usb_self_powered_capable + working_result) = condition_1_field;
    /* Retain producer index, residual byte count, and command metadata for the completion path. */
    *(uint32_t *)((int)&usb_core_revision + working_result) = remaining_bytes;
    *(uint32_t *)(&usb_u2_enabled + working_result) = command_parameter;
    *(uint32_t *)(&usb_remote_wakeup_enabled + working_result) = 0;
    return CONCAT44(descriptor_base, descriptor_base);
}

/**
 * @brief Execute SPI flash operation.
 *
 * @param operation_code Procedure input.
 * @param transfer_address Procedure input.
 * @param data_address Procedure input.
 * @param byte_count Number of blocks still to transfer.
 * @return Result produced by the procedure.
 */
uint64_t core_execute_spi_flash_operation(operation_code, transfer_address, data_address, byte_count)
int operation_code;
uint32_t transfer_address;
int data_address;
int byte_count;

{
    /* Execute SPI flash operation using only caller-provided data. */
    int operation_status;
    int remaining_count;
    uint32_t status_bits;
    uint32_t received_data_word;
    int read_response_word;
    int command_response_word;

    received_data_word = transfer_address;
    read_response_word = data_address;
    command_response_word = byte_count;
    if (operation_code == 2) {
    /* Shared target for core execute spi flash operation before run watchdog reset remaining; incoming paths preserve the same state assumptions. */
    core_spi_flash_begin_write_transaction:
        operation_status = core_run_wdt_reset_remaining(1);
        if ((operation_status != 0) || (operation_status = core_send_spi_flash_command_address(1, operation_code, transfer_address), operation_status != 0))
            goto core_spi_flash_return_transfer_result;
        remaining_count = 0;
        status_bits = 0x14000000;
        operation_status = 0;
        /* Iterate while (byte_count != 0 && (operation_status == 0)), preserving the defined cursor and bound. */
        while ((byte_count != 0 && (operation_status == 0))) {
            if (byte_count == 1) {
                status_bits = status_bits & 0xefffffff;
            }
            operation_status = core_update_spi_configure_spi(*(uint8_t *)(data_address + remaining_count) | status_bits, &received_data_word);
            byte_count = byte_count + -1;
            remaining_count = remaining_count + 1;
        }
    } else {
        if (operation_code == 3) {
            operation_status = core_run_wdt_reset_remaining(1);
            if (operation_status == 0) {
                operation_status = core_send_spi_flash_command_address(1, 3, transfer_address);
                if (operation_status == 0) {
                    remaining_count = 0;
                    /* Iterate while (byte_count != 0 && (operation_status == 0)), preserving the defined cursor and bound. */
                    while ((byte_count != 0 && (operation_status == 0))) {
                        core_wait_spi_transmit_ready();
                        operation_status = spi_transfer_word(0x14060000, &read_response_word);
                        if (operation_status == 0) {
                            byte_count = byte_count + -1;
                            *(uint8_t *)(data_address + remaining_count) = (uint8_t)read_response_word;
                            remaining_count = remaining_count + 1;
                        }
                    }
                }
                spi_transfer_word(0x60000, &read_response_word);
            }
            goto core_spi_flash_return_transfer_result;
        }
        if (operation_code == 6) {
            operation_status = spi_transfer_word(0x4060006, &command_response_word);
            if (operation_status == 0) {
                core_run_wdt_reset_remaining(1);
                core_run_wdt_reset_remaining(2);
            }
            goto core_spi_flash_return_transfer_result;
        }
        if (operation_code == 10)
            goto core_spi_flash_begin_write_transaction;
        if (operation_code != 0x20) {
            operation_status = -1;
            goto core_spi_flash_return_transfer_result;
        }
        operation_status = core_send_spi_flash_command_address(0, 0x20, transfer_address);
        if (operation_status != 0)
            goto core_spi_flash_return_transfer_result;
    }
    operation_status = core_run_wdt_reset_remaining(1);
/* Shared target for core spi flash return transfer result; incoming paths preserve the same state assumptions. */
core_spi_flash_return_transfer_result:
    return CONCAT44(received_data_word, operation_status);
}

/**
 * @brief Transition queue capacity command queue capacity context.
 *
 * @param condition Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_transition_queue_capacity_command_queue_capacity_context(condition)
uint32_t condition;

{
    /* Transition queue capacity command queue capacity context; persistent state is carried in usb out trb bytes remaining, USB trb ring out and related record fields. */
    uint32_t operation_status;
    uint32_t core_process_mww_update_resource_result;
    int *data_cursor;
    uint32_t status_bits;
    int *queue_state;
    uint32_t queue_capacity;
    uint32_t command_bits;
    int previous_counter_value;

    if ((condition >> 7 & 1) == 0) {
        /* Endpoint direction selects the OUT ring or the separate MWW IN ring. */
        queue_capacity = 8;
        queue_state = (int *)&usb_out_trb_bytes_remaining;
        data_cursor = (int *)(&usb_trb_ring_out + core_usb_out_trb_producer_index * 0x10);
        operation_status = usb_out_trb_bytes_remaining;
        if (usb_out_trb_bytes_remaining >> 0x11 != 0) {
            operation_status = 0x20000;
        }
    } else {
        queue_capacity = 10;
        queue_state = (int *)&usb_in_trb_bytes_remaining;
        data_cursor = (int *)&usb_trb_ring_in;
        operation_status = usb_in_trb_bytes_remaining;
        if (usb_active_mass_storage_class == 0) {
            if (usb_in_trb_bytes_remaining >> 0xf != 0) {
                operation_status = 0x8000;
            }
        } else if (usb_in_trb_bytes_remaining >> 0xe != 0) {
            operation_status = 0x4000;
        }
    }
    if (usb_active_mass_storage_class == 0) {
        /* Match the completion-mask encoding used when the ring was initially built. */
        command_bits = 0x20000;
    } else {
        command_bits = 0x30000;
    }
    /* Gate the following state transition on *queue_state == 0; the handler is skipped when this condition is false. */
    if (*queue_state == 0) {
        /* An empty queue needs no TRB publication and is already complete. */
        core_process_mww_update_resource_result = 1;
    } else {
        /* Consume one hardware-sized chunk and publish it at the current producer slot. */
        previous_counter_value = *queue_state;
        queue_state[0xd] = queue_state[0xd] & ~command_bits;
        command_bits = queue_state[0xe];
        *queue_state = previous_counter_value - operation_status;
        if (previous_counter_value - operation_status == 0) {
            /* The final chunk terminates the chain; intermediate chunks remain chained. */
            status_bits = 2;
        } else {
            status_bits = 4;
        }
        *data_cursor = queue_state[0xd];
        data_cursor[1] = 0;
        data_cursor[2] = operation_status;
        data_cursor[3] = status_bits | (command_bits & 0xffff) << 0xe | 0x811;
        queue_state[queue_state[0xb] + 1] = operation_status;
        queue_state[0xd] = operation_status + queue_state[0xd];
        previous_counter_value = queue_state[0xb];
        queue_state[0xb] = previous_counter_value + 1U;
        if (queue_capacity <= previous_counter_value + 1U) {
            /* Wrap the software producer index at the physical ring capacity. */
            queue_state[0xb] = 0;
        }
        if ((usb_core_revision < 0x109a) &&
            ((*(uint32_t *)(&usb_endpoint_command_register + ((condition & 0x7fffff7f) << 1 | condition >> 7) * 0x10) & 0xff) == 7)) {
            /* Older cores defer the update while the endpoint command reports state 7. */
            core_process_mww_update_resource_result = 0;
        } else {
            core_process_mww_update_resource_result = core_process_mww_update_resource(condition);
        }
    }
    return core_process_mww_update_resource_result;
}

/**
 * @brief Ramp thermal pwm.
 *
 * @return No value.
 */
void core_ramp_thermal_pwm()

{
    /* Ramp thermal pwm; persistent state is carried in core thermal pwm state, core thermal pwm control phase and related record fields. */
    int operation_status;
    uint32_t working_result;

    /* State 8 is the explicit inactive/terminal value for this PWM sequencer. */
    if (core_thermal_pwm_state == 8) {
        return;
    }
    if (core_thermal_pwm_control_phase == 0) {
        /* An idle mechanism forces the output off and resets the ramp state. */
        core_thermal_pwm_state = 4;
        core_thermal_pwm_control_phase = 3;
    /* Shared target for core thermal pwm stop and reset ramp; incoming paths preserve the same state assumptions. */
    core_thermal_pwm_stop_and_reset_ramp:
        core_set_thermal_pwm_duty(0);
        core_thermal_pwm_state = 0;
        return;
    }
    if (core_thermal_pwm_state == 0) {
        /* Mechanism phases 1 and 2 begin with a 100 ms settling interval. */
        if (core_thermal_pwm_control_phase != 1 && core_thermal_pwm_control_phase != 2) {
            return;
        }
        rti_set_software_timer(0x800f280, 100);
        core_thermal_pwm_state = 5;
        return;
    }
    if (core_thermal_pwm_state == 1) {
        /* This state translates mechanism direction changes into the next PWM ramp phase. */
        if (core_thermal_pwm_control_phase == 2) {
            rti_set_software_timer(0x800f280, 0);
            core_thermal_pwm_state = 3;
            return;
        }
        if (core_thermal_pwm_control_phase != 4) {
            return;
        }
        operation_status = 0x32;
        core_pwm_duty_target = 0x32;
        core_thermal_pwm_control_phase = 5;
        core_thermal_pwm_state = 5;
        goto core_thermal_pwm_apply_ramp_step;
    }
    if (core_thermal_pwm_state == 2) {
        /* Re-enter the downward ramp at 50 percent only from mechanism phase 1. */
        if (core_thermal_pwm_control_phase != 1) {
            return;
        }
        core_set_thermal_pwm_duty(0x32);
        core_thermal_pwm_state = 1;
        return;
    }
    if (core_thermal_pwm_state == 3) {
        /* On each one-minute tick, advance the stored duty target by 13 until it exceeds 99. */
        operation_status = core_is_software_timer_expired(0x800f280);
        if (operation_status == 0) {
            return;
        }
        if (99 < core_pwm_duty_target) {
            core_thermal_pwm_state = 2;
            return;
        }
        core_set_thermal_pwm_duty(core_pwm_duty_target + 0xd);
        working_result = 60000;
        goto core_thermal_pwm_arm_ramp_timer;
    }
    if (core_thermal_pwm_state == 4)
        goto core_thermal_pwm_stop_and_reset_ramp;
    if (core_thermal_pwm_state != 5) {
        return;
    }
    operation_status = core_is_software_timer_expired(0x800f280);
    if (operation_status == 0) {
        return;
    }
    if (core_thermal_pwm_control_phase == 4) {
        /* Start the short ramp at 5 percent when the mechanism enters phase 4. */
        operation_status = 5;
        core_pwm_duty_target = 5;
        core_thermal_pwm_control_phase = 5;
    /* Shared target for core thermal pwm apply ramp step; incoming paths preserve the same state assumptions. */
    core_thermal_pwm_apply_ramp_step:
        core_set_thermal_pwm_duty(operation_status);
    } else if (core_thermal_pwm_control_phase == 5) {
        /* Increase duty in five-point steps and advance after the defined 104-percent sentinel. */
        core_set_thermal_pwm_duty(core_pwm_duty_target + 5);
        if (0x68 < core_pwm_duty_target) {
            core_thermal_pwm_control_phase = 6;
        }
    } else {
        if (core_thermal_pwm_control_phase != 6) {
            core_thermal_pwm_state = 1;
            core_set_thermal_pwm_duty(0x32);
            return;
        }
        if (core_pwm_duty_target != 5) {
            operation_status = core_pwm_duty_target - 5;
            goto core_thermal_pwm_apply_ramp_step;
        }
        core_thermal_pwm_control_phase = 5;
    }
    working_result = 5000;
/* Shared target for core thermal pwm arm ramp timer; incoming paths preserve the same state assumptions. */
core_thermal_pwm_arm_ramp_timer:
    /* Ramp steps normally run every five seconds; state 3 selects the one-minute interval above. */
    rti_set_software_timer(0x800f280, working_result);
    return;
}

/**
 * @brief Handle descriptor clear memory device descriptor context.
 *
 * @param context Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t core_handle_descriptor_clear_memory_device_descriptor_context(context)
uint32_t context;

{
    /* Handle descriptor clear memory device descriptor context; persistent state is carried in core primary descriptor state. */
    int operation_status;
    uint32_t core_initialize_scsi_response_buffer_result;
    uint32_t working_result;
    char data_cursor[4];
    uint8_t working_buffer[148];
    uint8_t record_descriptor[44];

    operation_status = core_process_command_phase_three();
    data_cursor[0] = '\0';
    if (operation_status != 0) {
        operation_status = core_get_runtime_context_base_for_build_scsi_mode();
        operation_status = core_initialize_output_record_from_source(operation_status + 0x2c, record_descriptor);
        if (operation_status == 0) {
            return 0x21;
        }
        operation_status = core_transfer_match_found_expected_type(record_descriptor, data_cursor, 0, 0, 1);
        if (operation_status == 0) {
            return 0x21;
        }
    }
    ti_memset(working_buffer, 0, 0x4c);
    core_initialize_scsi_response_buffer_result = (uint32_t)core_initialize_scsi_response_buffer(context, 0x18);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0, 1, 0x20);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 2, 3, 0x14);
    if (data_cursor[0] == '\0') {
        working_result = 0;
    } else {
        working_result = 0x42;
    }
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 4, 4, working_result);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 5, 5);
    if ((data_cursor[0] == '\0') || (core_primary_descriptor_state != 2 && core_primary_descriptor_state != 3)) {
        working_result = 1;
    } else {
        working_result = 2;
    }
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 6, 6, working_result);
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 7, 7, 1);
    if (data_cursor[0] == '\0') {
        working_result = 0;
    } else {
        working_result = 8;
    }
    core_write_bounded_response_field(core_initialize_scsi_response_buffer_result, 0xc, 0xc, working_result);
    return 0;
}
