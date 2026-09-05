/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : mww.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the mww module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the mww module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Handle transfer mww0 addr device transfer state.
 *
 * @return No value.
 */
void mww_handle_transfer_mww0_addr_device_transfer_state()

{
    /* Handle transfer mww0 addr device transfer state; persistent state is carried in usb bot csw pointer, usb bot cbw pointer and related record fields. */
    bool condition_met;
    int operation_status;
    uint32_t working_result;
    uint8_t *data_cursor;
    uint32_t transfer_length;

    operation_status = core_is_mww_dispatch_blocked();
    if (operation_status != 0) {
        core_defer_mww_dispatch_and_mask_usb_interrupt();
        return;
    }
    *(uint32_t *)(usb_bot_csw_pointer + 8) = *(uint32_t *)(usb_bot_cbw_pointer + 8);
    core_update_word_mww_transfer_word();
    operation_status = core_transition_atapi_device_flag();
    if (operation_status == 0) {
        return;
    }
    core_mark_mww_command_received();
    usb_bot_scsi_command_block = usb_bot_cbw_pointer + 0xf;
    usb_bot_scsi_lun = *(uint8_t *)(usb_bot_cbw_pointer + 0xd);
    usb_bot_scsi_data_transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
    usb_bot_scsi_command_block_length = *(uint8_t *)(usb_bot_cbw_pointer + 0xe);
    usb_bot_actual_transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
    operation_status = core_is_supported_forward_opcode(*(uint8_t *)(usb_bot_cbw_pointer + 0xf));
    if (operation_status != 0) {
        core_claim_device_command_for_mww();
        core_process_ata_command_buffer_for_mww_handle_transfer(usb_bot_cbw_pointer + 0xf, &usb_bot_actual_transfer_length);
        if (core_mww_saved_request != 2) {
            if (core_mww_saved_request == 1) {
                if (*(int *)(&ata_true_logical_sector_size_bytes + (uint32_t)*(uint8_t *)(usb_bot_cbw_pointer + 0xd) * 0x1d0) != 0) {
                    if (usb_bot_actual_transfer_length == 0) {
                        mww_transition_selection_state(4);
                    } else {
                        transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
                        if (usb_bot_actual_transfer_length < transfer_length) {
                            usb_bot_transfer_case = 5;
                        } else if (transfer_length < usb_bot_actual_transfer_length) {
                            usb_bot_transfer_case = 7;
                            usb_bot_actual_transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
                        }
                    }
                }
                mww_init_read_only();
                operation_status = mww_handle_saved_register_mww_saved_register();
                if (operation_status != 8) {
                    return;
                }
                if (usb_bot_actual_transfer_length == 0) {
                    mww_transition_selection_state(4);
                    return;
                }
                data_cursor = (uint8_t *)&MWW0_ADDR;
                working_result = 0x83;
                goto mww_transfer_submit_selected_endpoint;
            }
            goto mww_transfer_dispatch_saved_command;
        }
        if (*(int *)(&ata_true_logical_sector_size_bytes + (uint32_t)*(uint8_t *)(usb_bot_cbw_pointer + 0xd) * 0x1d0) != 0) {
            if (usb_bot_actual_transfer_length == 0) {
                working_result = 9;
            } else {
                transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
                if (transfer_length <= usb_bot_actual_transfer_length) {
                    if (transfer_length < usb_bot_actual_transfer_length) {
                        usb_bot_transfer_case = 0xd;
                        usb_bot_actual_transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
                    }
                    goto mww_transfer_initialize_write_window;
                }
                working_result = 0xb;
            }
            mww_transition_selection_state(working_result);
        }
    /* Shared target for mww transfer initialize write window; incoming paths preserve the same state assumptions. */
    mww_transfer_initialize_write_window:
        mww_init_write_only();
        operation_status = mww_handle_saved_register_mww_saved_register();
        if (operation_status != 8) {
            mww_process_mww_transfer_storage(3);
            return;
        }
        if (usb_bot_actual_transfer_length == 0) {
            return;
        }
        data_cursor = (uint8_t *)&MWW1_ADDR;
        working_result = 3;
    /* Shared target for mww transfer submit selected endpoint; incoming paths preserve the same state assumptions. */
    mww_transfer_submit_selected_endpoint:
        mww_submit_usb_transfer(working_result, data_cursor, usb_bot_actual_transfer_length, 0);
        return;
    }
    usb_bot_check_condition = core_get_constant_zero_value_for_mww_handle_transfer(usb_bot_cbw_pointer + 0xf);
    condition_met = true;
    if (core_mww_saved_request == 2) {
        usb_bot_scsi_data_transfer_length = 0;
        if (usb_bot_actual_transfer_length < 0x1014) {
            transfer_length = usb_bot_actual_transfer_length;
            if (usb_bot_actual_transfer_length == 0)
                goto mww_transfer_check_buffered_read_limit;
        } else {
            transfer_length = 0x1014;
        }
        mww_submit_usb_transfer(3, &scsi_command_response_buffer, transfer_length, 0);
        if (usb_bot_actual_transfer_length != 0) {
            condition_met = false;
        }
    }
/* Shared target for mww transfer check buffered read limit; incoming paths preserve the same state assumptions. */
mww_transfer_check_buffered_read_limit:
    if ((core_mww_saved_request == 1) && (0x1014 < usb_bot_actual_transfer_length)) {
        mww_process_mww_transfer_storage(0x83);
        mww_process_device_transfer_state(1);
        return;
    }
    if (!condition_met) {
        return;
    }
/* Shared target for mww transfer dispatch saved command; incoming paths preserve the same state assumptions. */
mww_transfer_dispatch_saved_command:
    mww_handle_saved_register_mww_saved_register();
    return;
}

/**
 * @brief Complete USB transfer.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
uint32_t mww_complete_usb_transfer(condition, value, length, buffer)
uint32_t condition;
uint32_t value;
uint32_t length;
int *buffer;

{
    /* Complete USB transfer; persistent state is carried in usb data transfer callback, usb in trb bytes remaining and related record fields. */
    firmware_callback_t *usb_data_transfer_callback_snapshot;
    int working_result;
    int *data_cursor;
    uint32_t *channel_state;
    int *buffer_cursor;

    buffer_cursor = buffer;
    mww_update_mww_transfer_storage(condition, &buffer_cursor);
    if (buffer_cursor[8] != 0) {
        if (buffer_cursor[9] == 0) {
            working_result = buffer_cursor[3] - (*(uint32_t *)(buffer_cursor[6] + 8) & 0xffffff);
        } else {
            channel_state = (uint32_t *)&usb_in_trb_bytes_remaining;
            if ((condition >> 7 & 1) == 0) {
                channel_state = (uint32_t *)&usb_out_trb_bytes_remaining;
                working_result = 0x11340;
            } else {
                working_result = 0x11290;
            }
            data_cursor = (int *)(working_result + -0x40000000 + channel_state[0xc] * 0x10);
            working_result = channel_state[channel_state[0xc] + 1] - (data_cursor[2] & 0xffffffU);
            if ((condition >> 7 & 1) != 0) {
                /* Program MWW window zero read offset register with working_result + *data_cursor; write ordering is hardware-significant. */
                mww_window_zero_read_offset_register = working_result + *data_cursor;
            }
        }
        buffer_cursor[8] = 0;
        buffer_cursor[1] = working_result + buffer_cursor[1];
        buffer_cursor[2] = buffer_cursor[2] - working_result;
        usb_data_transfer_callback_snapshot = (firmware_callback_t *)usb_data_transfer_callback;
        *buffer_cursor = working_result + *buffer_cursor;
        if ((condition & 0xffffff7f) == 0) {
            channel_state = (uint32_t *)(condition >> 8);
            if ((condition >> 7 & 1) == 0) {
                channel_state = (uint32_t *)&usb_core_revision;
                if ((usb_ep0_zero_length_packet_pending == '\x01') && (buffer_cursor[1] != 0)) {
                    usb_ep0_state = 5;
                    mww_process_mww_transfer_storage();
                    return 0;
                }
                usb_ep0_out_stalled = 0;
            }
            core_dispatch_usb_control_completion(condition, buffer_cursor, channel_state);
            return 0;
        }
        /* Notify usb data transfer callback snapshot only after the associated state fields have been committed. */
        (*usb_data_transfer_callback_snapshot)(condition);
    }
    return 0;
}

/**
 * @brief Handle transfer device transfer state.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void mww_handle_transfer_device_transfer_state(offset) int offset;

{
    /* Handle transfer device transfer state; persistent state is carried in usb bot csw pointer, core MWW saved request and related record fields. */
    int operation_status;
    uint32_t transfer_length;
    uint32_t working_result;

    operation_status = (int)usb_bot_csw_pointer;
    if (core_mww_saved_request == 0) {
        usb_bot_cbw_received_length = *(uint32_t *)(offset + 4);
        operation_status = core_validate_bot_command_wrapper();
        if (operation_status == 0) {
            mww_process_mww_transfer_storage(0x83);
            mww_process_mww_transfer_storage(3);
            usb_bot_persistent_stall = 1;
            return;
        }
        mww_handle_transfer_mww0_addr_device_transfer_state();
        return;
    }
    if (core_mww_saved_request == 2) {
        *(int *)(usb_bot_csw_pointer + 8) = *(int *)(usb_bot_csw_pointer + 8) - *(int *)(offset + 4);
        transfer_length = *(uint32_t *)(operation_status + 8);
        if (transfer_length != 0) {
            if (transfer_length < 0x401) {
                if (transfer_length == 0) {
                    return;
                }
            } else {
                transfer_length = 0x400;
            }
            mww_submit_usb_transfer(3, &usb_bulk_endpoint_buffer, transfer_length, 0);
            return;
        }
        if (usb_bot_transfer_case == 0xd) {
            mww_transition_selection_state(0xd);
            return;
        }
        if ((usb_bot_transfer_case != 9) && (usb_bot_transfer_case != 0xb)) {
            operation_status = core_is_supported_forward_opcode(*(uint8_t *)(usb_bot_cbw_pointer + 0xf));
            if (operation_status == 0) {
                usb_bot_scsi_data_transfer_length = *(uint32_t *)(offset + 4);
                mww_handle_saved_register_mww_saved_register();
            }
            if (usb_bot_sata_transfer_complete == 0) {
                usb_bot_usb_transfer_complete = 1;
                return;
            }
        }
        working_result = 0;
    } else {
        mww_process_mww_transfer_storage(0x83);
        mww_process_mww_transfer_storage(3);
        working_result = 2;
    }
    mww_process_device_transfer_state(working_result);
    return;
}

/**
 * @brief Handle saved register MWW saved register.
 *
 * @return Result produced by the procedure.
 */
int mww_handle_saved_register_mww_saved_register()

{
    /* Handle saved register MWW saved register; persistent state is carried in core MWW saved request, usb bot current scsi opcode and related record fields. */
    uint32_t operation_status;
    int saved_register_value;

    if ((core_mww_saved_request != 4) && (core_mww_saved_request != 5)) {
        usb_bot_current_scsi_opcode = *usb_bot_scsi_command_block;
        saved_register_value = core_process_word_validity_flag_command_validation_context();
        if (saved_register_value == 8) {
            usb_bot_pending_scsi_opcode = *(uint8_t *)(usb_bot_cbw_pointer + 0xf);
        } else if (saved_register_value == 9) {
            if (usb_bot_scsi_response_data != (uint32_t)&scsi_command_response_buffer) {
                operation_status = usb_bot_scsi_response_byte_count;
                if (0x1013 < usb_bot_scsi_response_byte_count) {
                    operation_status = 0x1014;
                }
                core_copy_memory_bytes(&scsi_command_response_buffer, usb_bot_scsi_response_data, operation_status);
            }
            usb_bot_actual_transfer_length = usb_bot_scsi_response_byte_count;
            operation_status = *(uint32_t *)(usb_bot_cbw_pointer + 8);
            if (usb_bot_scsi_response_byte_count < operation_status) {
                usb_bot_transfer_case = 5;
            } else if (operation_status < usb_bot_scsi_response_byte_count) {
                usb_bot_transfer_case = 7;
                usb_bot_actual_transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
            }
            usb_bot_sata_transfer_complete = 1;
            operation_status = 0x1014;
            if (usb_bot_actual_transfer_length < 0x1014) {
                operation_status = usb_bot_actual_transfer_length;
            }
            if (operation_status == 0) {
                mww_transition_selection_state(4);
            } else {
                mww_submit_usb_transfer(0x83, &scsi_command_response_buffer, operation_status, 0);
            }
        } else {
            if (saved_register_value != 0) {
                core_process_mww_saved_request_for_mww_handle_saved(1);
            }
            mww_process_device_transfer_state(saved_register_value != 0);
        }
    }
    return saved_register_value;
}

/**
 * @brief Send USB endpoint command.
 *
 * @param channel_flags Procedure input.
 * @param command_word Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
int mww_send_usb_endpoint_command(channel_flags, command_word, length, flags)
uint32_t channel_flags;
uint32_t command_word;
uint32_t length;
int flags;

{
    /* Send USB endpoint command; persistent state is carried in MWW window zero read offset register, MWW window one write offset register and related record fields. */
    uint32_t mww_window_zero_read_offset_register_snapshot;
    uint32_t mww_window_one_write_offset_register_snapshot;
    int operation_result;
    int flags_snapshot;

    flags_snapshot = flags;
    operation_result = core_wait_usb_endpoint_command(channel_flags, 1000);
    if (operation_result == 0) {
        operation_result = ((channel_flags & 0x7fffff7f) << 1 | channel_flags >> 7) * 0x10;
        *(int *)(&usb_endpoint_command_parameter_one_register + operation_result) = flags;
        *(uint32_t *)(&usb_endpoint_command_parameter_zero_register + operation_result) = length;
        if (((command_word & 0xff) == 7) && (0x108a < usb_core_revision)) {
            *(uint32_t *)(&usb_endpoint_command_register + operation_result) = command_word;
        } else {
            *(uint32_t *)(&usb_endpoint_command_register + operation_result) = command_word | 0x400;
        }
        if (((command_word & 0xff) == 8) && (mww_update_mww_transfer_storage(channel_flags, &flags_snapshot), *(int *)(flags_snapshot + 0x24) != 0)) {
            if ((channel_flags >> 7 & 1) == 0) {
                /* Snapshot MWW window one write offset register before decoding its status or capability fields. */
                mww_window_one_write_offset_register_snapshot = mww_window_one_write_offset_register;
                /* Program MWW window one read offset register with mww_window_one_write_offset_register_snapshot; write ordering is hardware-significant. */
                mww_window_one_read_offset_register = mww_window_one_write_offset_register_snapshot;
            } else {
                if (usb_active_mass_storage_class == 0) {
                    operation_result = 6;
                } else {
                    operation_result = 5;
                }
                /* Snapshot MWW window zero read offset register before decoding its status or capability fields. */
                mww_window_zero_read_offset_register_snapshot = mww_window_zero_read_offset_register;
                /* Program MWW window zero write offset register with 1 << operation_result + 10 ^ mww_window_zero_read_offset_register_snapshot; write ordering is hardware-significant. */
                mww_window_zero_write_offset_register = 1 << operation_result + 10 ^ mww_window_zero_read_offset_register_snapshot;
            }
        }
        operation_result = 0;
    }
    return operation_result;
}

/**
 * @brief Handle USB endpoint event.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t mww_handle_usb_endpoint_event(condition, value, length, flags)
uint32_t condition;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Handle USB endpoint event; persistent state is carried in USB core revision. */
    uint32_t operation_status;
    uint32_t operation_result;
    uint32_t status_bits;
    uint32_t request_code_or_result;
    uint32_t flags_snapshot;

    if ((condition >> 1 & 1) == 0) {
        request_code_or_result = 0;
    } else {
        request_code_or_result = 0x80;
    }
    operation_status = (condition & 0x3ff) >> 6;
    status_bits = (condition & 0x3f) >> 2;
    flags_snapshot = flags;
    if (operation_status == 1) {
        operation_result = ((uint32_t (*)())mww_complete_usb_transfer)(request_code_or_result | status_bits);
    } else {
        if (operation_status == 2) {
            request_code_or_result = mww_advance_completed_usb_trb(request_code_or_result | status_bits);
            return request_code_or_result;
        }
        if (operation_status == 3) {
            mww_update_mww_transfer_storage(request_code_or_result | status_bits, &flags_snapshot);
            operation_result = *(uint32_t *)(flags_snapshot + 0x10);
            if ((operation_result == 0) && (status_bits == 0)) {
                request_code_or_result = core_prepare_command_transfer_direction(condition, request_code_or_result);
                return request_code_or_result;
            }
        } else if (operation_status == 6) {
            operation_result = condition >> 0xd;
            if ((((condition >> 0xc & 1) != 0) && (operation_result = usb_core_revision, usb_core_revision == 0x120a)) && (request_code_or_result == 0)) {
                mww_update_mww_transfer_storage(status_bits, &flags_snapshot);
                *(uint32_t *)(*(int *)(flags_snapshot + 0x18) + 0xc) = *(uint32_t *)(*(int *)(flags_snapshot + 0x18) + 0xc) | 1;
                request_code_or_result = core_process_mww_update_resource(status_bits);
                return request_code_or_result;
            }
        } else {
            operation_result = operation_status - 7;
            if (operation_status - 7 == 0) {
                mww_update_mww_transfer_storage(request_code_or_result | status_bits, &flags_snapshot);
                operation_result = (condition & 0xfffffff) >> 0x18;
                if (operation_result == 6) {
                    *(uint32_t *)(flags_snapshot + 0x1c) = (condition & 0x7fffff) >> 0x10;
                    return flags_snapshot;
                }
            }
        }
    }
    return operation_result;
}

/**
 * @brief Submit USB transfer.
 *
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t mww_submit_usb_transfer(condition, buffer, length, flags)
uint32_t condition;
uint8_t *buffer;
int length;
uint32_t flags;

{
    /* Submit USB transfer; persistent state is carried in USB trb ring in. */
    int working_result;
    uint32_t mww_configure_transfer_descriptor_result;
    uint32_t data_cursor;
    int length_component;
    uint32_t *transfer_descriptor;
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    mww_update_mww_transfer_storage(condition, &flags_snapshot);
    if (*(int *)(flags_snapshot + 0x20) == 0) {
        if ((buffer == (uint8_t *)&MWW0_ADDR) || (buffer == (uint8_t *)&MWW1_ADDR)) {
            mww_configure_transfer_descriptor_result = 1;
        } else {
            mww_configure_transfer_descriptor_result = 0;
        }
        *(uint32_t *)(flags_snapshot + 0x24) = mww_configure_transfer_descriptor_result;
        if ((condition & 0x80) == 0) {
            data_cursor = (uint32_t)*(uint16_t *)(flags_snapshot + 0x14);
            working_result = -0x40;
            length_component = (((length + data_cursor) - 1) / data_cursor) * data_cursor;
        } else {
            working_result = -0x80;
            length_component = length;
        }
        transfer_descriptor = (uint32_t *)(&usb_trb_ring_in + working_result + (condition & 0xfffff7f) * 0x10);
        if (*(int *)(flags_snapshot + 0x24) == 0) {
            *transfer_descriptor = (uint32_t)buffer;
            transfer_descriptor[1] = 0;
            transfer_descriptor[2] = length_component;
            transfer_descriptor[3] = (flags & 0xffff) << 0xe | 0x813;
        } else {
            transfer_descriptor = (uint32_t *)(uint32_t)core_transfer_descriptor_capacity_previous_descriptor_address_descriptor_queue_context(condition & 0x80, length_component, flags);
        }
        mww_configure_transfer_descriptor_result = mww_configure_transfer_descriptor(condition, transfer_descriptor, length, flags);
    } else {
        mww_configure_transfer_descriptor_result = 4;
    }
    return mww_configure_transfer_descriptor_result;
}

/**
 * @brief Advance completed USB trb.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
uint32_t mww_advance_completed_usb_trb(condition, value, length, buffer)
uint32_t condition;
uint32_t value;
uint32_t length;
int *buffer;

{
    /* Advance completed USB trb; persistent state is carried in usb in trb bytes remaining, usb out trb bytes remaining and related record fields. */
    int *data_cursor;
    int working_result;
    int *descriptor_slot;
    int remaining_length;
    int initial_buffer_address;
    int buffer_offset;
    uint32_t operation_status;
    uint32_t ring_capacity;
    int *buffer_cursor;

    buffer_cursor = buffer;
    mww_update_mww_transfer_storage(condition, &buffer_cursor);
    working_result = 0x11290;
    data_cursor = (int *)&usb_in_trb_bytes_remaining;
    if ((condition >> 7 & 1) == 0) {
        working_result = 0x11340;
        ring_capacity = 8;
        data_cursor = (int *)&usb_out_trb_bytes_remaining;
    } else {
        ring_capacity = 10;
    }
    descriptor_slot = (int *)(working_result + -0x40000000 + data_cursor[0xc] * 0x10);
    working_result = data_cursor[data_cursor[0xc] + 1] - (descriptor_slot[2] & 0xffffffU);
    if ((condition >> 7 & 1) != 0) {
        /* Program MWW window zero read offset register with working_result + *descriptor_slot; write ordering is hardware-significant. */
        mww_window_zero_read_offset_register = working_result + *descriptor_slot;
    }
    remaining_length = buffer_cursor[2];
    buffer_offset = buffer_cursor[1];
    initial_buffer_address = *buffer_cursor;
    data_cursor[0xc] = data_cursor[0xc] + 1;
    operation_status = data_cursor[0xc];
    buffer_cursor[2] = remaining_length - working_result;
    buffer_cursor[1] = working_result + buffer_offset;
    *buffer_cursor = working_result + initial_buffer_address;
    if (ring_capacity <= operation_status) {
        data_cursor[0xc] = 0;
    }
    /* Gate the following state transition on *data_cursor != 0; the handler is skipped when this condition is false. */
    if (*data_cursor != 0) {
        core_transition_queue_capacity_command_queue_capacity_context(condition);
    }
    return 0;
}

/**
 * @brief Transition selection state.
 *
 * @param offset Procedure input.
 * @return No value.
 */
void mww_transition_selection_state(offset) int offset;

{
    /* Transition selection state; persistent state is carried in usb bot actual transfer length, usb bot transfer case and related record fields. */
    uint32_t working_result;
    uint32_t usb_bot_actual_transfer_length_snapshot;

    if (offset - 2U < 2) {
    /* Shared target for mww phase select bulk in stall; incoming paths preserve the same state assumptions. */
    mww_phase_select_bulk_in_stall:
        working_result = 0x83;
    } else {
        if (offset - 4U < 2) {
            mww_process_mww_transfer_storage(0x83);
            working_result = 0;
            goto mww_phase_send_command_status;
        }
        if (offset - 7U < 2)
            goto mww_phase_select_bulk_in_stall;
        if (offset == 9) {
            usb_bot_transfer_case = 9;
            core_mww_saved_request = 2;
            usb_bot_actual_transfer_length = *(uint32_t *)(usb_bot_cbw_pointer + 8);
            if (usb_bot_actual_transfer_length >> 10 == 0) {
                usb_bot_actual_transfer_length_snapshot = usb_bot_actual_transfer_length;
                if (usb_bot_actual_transfer_length == 0) {
                    return;
                }
            } else {
                usb_bot_actual_transfer_length_snapshot = 0x400;
            }
            ((uint32_t (*)())mww_submit_usb_transfer)(3, &usb_bulk_endpoint_buffer, usb_bot_actual_transfer_length_snapshot, 0);
            return;
        }
        if (offset != 10) {
            if (offset == 0xb) {
                usb_bot_transfer_case = 0xb;
                return;
            }
            if (offset != 0xd) {
                return;
            }
        }
        working_result = 3;
    }
    mww_process_mww_transfer_storage(working_result);
    working_result = 2;
/* Shared target for mww phase send command status; incoming paths preserve the same state assumptions. */
mww_phase_send_command_status:
    mww_process_device_transfer_state(working_result);
    return;
}

/**
 * @brief Process device transfer state.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void mww_process_device_transfer_state(condition) int condition;

{
    /* Process device transfer state; persistent state is carried in usb bot cbw pointer, usb bot csw pointer and related record fields. */
    int usb_bot_cbw_pointer_snapshot;
    uint32_t *usb_bot_csw_pointer_snapshot;
    int operation_status;
    uint8_t condition_field;

    core_release_device_command();
    usb_bot_csw_pointer_snapshot = (uint32_t *)usb_bot_csw_pointer;
    usb_bot_cbw_pointer_snapshot = (int)usb_bot_cbw_pointer;
    operation_status = usb_bot_check_condition;
    if ((core_mww_saved_request == 4) || (core_mww_saved_request == 5))
        goto mww_command_status_clear_transfer_context;
    *usb_bot_csw_pointer = 0x53425355;
    usb_bot_csw_pointer_snapshot[1] = *(uint32_t *)(usb_bot_cbw_pointer_snapshot + 4);
    if (operation_status == 0) {
        condition_field = (uint8_t)condition;
    } else {
        condition_field = 1;
    }
    *(uint8_t *)(usb_bot_csw_pointer_snapshot + 3) = condition_field;
    if ((usb_bot_transfer_case == 5) || (usb_bot_transfer_case == 0xb)) {
        operation_status = *(int *)(usb_bot_cbw_pointer_snapshot + 8) - usb_bot_actual_transfer_length;
    /* Shared target for mww command status store residue; incoming paths preserve the same state assumptions. */
    mww_command_status_store_residue:
        usb_bot_csw_pointer_snapshot[2] = operation_status;
    } else if (usb_bot_transfer_case == 9) {
        operation_status = *(int *)(usb_bot_cbw_pointer_snapshot + 8);
        goto mww_command_status_store_residue;
    }
    if (condition == 2) {
        core_mww_saved_request = 5;
    } else {
        core_mww_saved_request = 4;
    }
    ((uint32_t (*)())mww_submit_usb_transfer)(0x83, usb_bot_csw_pointer, 0xd, 0);
/* Shared target for mww command status clear transfer context; incoming paths preserve the same state assumptions. */
mww_command_status_clear_transfer_context:
    core_release_mww_device_command();
    return;
}

/**
 * @brief Build AHCI prd table.
 *
 * @param context Procedure input.
 * @param offset Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t mww_build_ahci_prd_table(context, offset)
int context;
int offset;

{
    /* Build AHCI prd table; persistent state is carried in ahci command table prdt, ATA identify device data. */
    uint32_t *ahci_command_table_prdt_pointer;
    uint32_t operation_result;
    uint32_t offset_field_0x18;
    uint8_t *data_cursor;
    int remaining_length;

    operation_result = 0;
    ahci_command_table_prdt_pointer = (uint32_t *)(&ahci_command_table_prdt + context * 0x100);
    if (*(char *)(offset + 0x16) == '\0') {
        data_cursor = (uint8_t *)&ata_identify_device_data;
    } else if (*(char *)(offset + 0x15) == '\0') {
        data_cursor = (uint8_t *)&MWW0_ADDR;
    } else {
        data_cursor = (uint8_t *)&MWW1_ADDR;
    }
    /* Repeat while remaining_length != 0; the body advances or polls the state needed to leave the loop. */
    do {
        if (7 < operation_result) {
            return operation_result;
        }
        offset_field_0x18 = *(uint32_t *)(offset + 0x18);
        if (0x3dfff < offset_field_0x18) {
            offset_field_0x18 = 0x3e000;
        }
        *ahci_command_table_prdt_pointer = (uint32_t)data_cursor;
        ahci_command_table_prdt_pointer[1] = 0;
        ahci_command_table_prdt_pointer[2] = 0;
        ahci_command_table_prdt_pointer[3] = offset_field_0x18 - 1;
        operation_result = operation_result + 1;
        ahci_command_table_prdt_pointer = ahci_command_table_prdt_pointer + 4;
        remaining_length = *(int *)(offset + 0x18) - offset_field_0x18;
        *(int *)(offset + 0x18) = remaining_length;
    } while (remaining_length != 0);
    return operation_result;
}

/**
 * @brief Configure transfer descriptor.
 *
 * @param context Procedure input.
 * @param transfer_descriptor Procedure input.
 * @param length Procedure input.
 * @param status_context Procedure input.
 * @return No value.
 */
uint32_t mww_configure_transfer_descriptor(context, transfer_descriptor, length, status_context)
uint32_t context;
uint32_t *transfer_descriptor;
uint32_t length;
uint32_t *status_context;

{
    /* Configure transfer descriptor using only caller-provided data. */
    int mww_send_usb_endpoint_command_derived;
    uint32_t *status_bits;

    status_bits = status_context;
    mww_update_mww_transfer_storage(context, &status_bits);
    status_bits[6] = (uint32_t)transfer_descriptor;
    status_bits[3] = transfer_descriptor[2];
    status_bits[2] = length;
    status_bits[1] = 0;
    *status_bits = *transfer_descriptor;
    *(int16_t *)((int)status_bits + 0x16) = (int16_t)(uint32_t)status_context;
    mww_send_usb_endpoint_command_derived = ((int (*)())mww_send_usb_endpoint_command)(context, (int)status_context << 0x10 | 0x106);
    if (mww_send_usb_endpoint_command_derived == 0) {
        status_bits[8] = 1;
    }
    return 0;
}

/**
 * @brief Process MWW transfer storage.
 *
 * @param condition Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void mww_process_mww_transfer_storage(condition, value, length, flags) int condition;
uint32_t value;
uint32_t length;
int flags;

{
    /* Process MWW transfer storage using only caller-provided data. */
    int mww_send_usb_endpoint_command_result;
    int flags_snapshot;

    flags_snapshot = flags;
    mww_update_mww_transfer_storage(condition, &flags_snapshot);
    if (*(int *)(flags_snapshot + 0x24) != 0) {
        core_end_usb_endpoint_transfer(condition);
    }
    mww_send_usb_endpoint_command_result = mww_send_usb_endpoint_command(condition, 4, 0, 0);
    if (mww_send_usb_endpoint_command_result == 0) {
        *(uint32_t *)(flags_snapshot + 0x10) = 1;
    }
    if (condition == 0) {
        core_arm_usb_control_setup_reception();
    }
    return;
}

/**
 * @brief Reset both wrap-window read/write cursors and their USB ring pointers.
 *
 * @return No value.
 */
void mww_reset_rw_offsets()

{
    /* Reset rw offsets; persistent state is carried in MWW window zero write offset register, Usb in trb ring buffer pointer and related record fields. */
    /* Program MWW window zero write offset register with 0; write ordering is hardware-significant. */
    mww_window_zero_write_offset_register = 0;
    usb_in_trb_ring_buffer_pointer = (uint32_t)&MWW0_ADDR;
    /* Program MWW window zero read offset register with 0; write ordering is hardware-significant. */
    mww_window_zero_read_offset_register = 0;
    usb_out_trb_ring_buffer_pointer = (uint32_t)&MWW1_ADDR;
    /* Program MWW window one write offset register with 0; write ordering is hardware-significant. */
    mww_window_one_write_offset_register = 0;
    mww_window_one_read_offset_register = 0;
    return;
}

/**
 * @brief Configure the full wrap window for SATA-to-USB transfers.
 *
 * @return No value.
 */
void mww_init_read_only()

{
    /* Init read only; persistent state is carried in MWW window zero block offset register, MWW window zero block size register and related record fields. */
    /* Program MWW window zero block offset register with 0; write ordering is hardware-significant. */
    mww_window_zero_block_offset_register = 0;
    mww_window_zero_block_size_register = 6;
    /* Program MWW window zero write offset register with 0; write ordering is hardware-significant. */
    mww_window_zero_write_offset_register = 0;
    usb_in_trb_ring_buffer_pointer = (uint32_t)&MWW0_ADDR;
    /* Program MWW window zero read offset register with 0; write ordering is hardware-significant. */
    mww_window_zero_read_offset_register = 0;
    return;
}

/**
 * @brief Configure the full wrap window for USB-to-SATA transfers.
 *
 * @return No value.
 */
void mww_init_write_only()

{
    /* Init write only; persistent state is carried in MWW window one block offset register, MWW window one block size register and related record fields. */
    /* Program MWW window one block offset register with 0; write ordering is hardware-significant. */
    mww_window_one_block_offset_register = 0;
    mww_window_one_block_size_register = 0x80000006;
    /* Program MWW window one write offset register with 0; write ordering is hardware-significant. */
    mww_window_one_write_offset_register = 0;
    usb_out_trb_ring_buffer_pointer = (uint32_t)&MWW1_ADDR;
    /* Program MWW window one read offset register with 0; write ordering is hardware-significant. */
    mww_window_one_read_offset_register = 0;
    return;
}

/**
 * @brief Initialize the MWW subsystem state and required hardware resources.
 *
 * @return No value.
 */
void mww_init()

{
    /* Init; persistent state is carried in MWW window zero block offset register, MWW window zero block size register and related record fields. */
    /* Program MWW window zero block offset register with 0; write ordering is hardware-significant. */
    mww_window_zero_block_offset_register = 0;
    mww_window_zero_block_size_register = 5;
    /* Program MWW window one block offset register with 0x8000; write ordering is hardware-significant. */
    mww_window_one_block_offset_register = 0x8000;
    mww_window_one_block_size_register = 0x80000005;
    mww_reset_rw_offsets();
    return;
}

/**
 * @brief Update MWW transfer storage.
 *
 * @param condition Procedure input.
 * @param buffer Procedure input.
 * @return No value.
 */
void mww_update_mww_transfer_storage(condition, buffer) uint32_t condition;
uint32_t *buffer;

{
    /* Update MWW transfer storage; persistent state is carried in usb in endpoint info, usb out endpoint info. */
    uint8_t *data_cursor;

    data_cursor = (uint8_t *)&usb_in_endpoint_info;
    if ((condition >> 7 & 1) == 0) {
        data_cursor = (uint8_t *)&usb_out_endpoint_info;
    }
    *buffer = (uint32_t)data_cursor + (condition & 0xffffff7f) * 0x28;
    return;
}

/**
 * @brief Abort stalled SATA-side MWW requests and return the resulting status.
 *
 * @return Result produced by the procedure.
 */
uint32_t mww_force_sata_interface_ready()

{
    /* Force SATA interface ready using only caller-provided data. */
    uint32_t mww_status_snapshot;

    /* Program MWW CONTROL REG OFF with 0xf0; write ordering is hardware-significant. */
    MWW_CONTROL_REG_OFF = 0xf0;
    MWW_CONTROL_REG_OFF = 0;
    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    mww_status_snapshot = MWW_STATUS_REG_OFF;
    return mww_status_snapshot;
}
