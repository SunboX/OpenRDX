/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_stack.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the usb stack module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the usb stack module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Register the stack callbacks and initialize the USB controller.
 *
 * @param primary_callback Primary USB state callback.
 * @param secondary_callback Secondary USB state callback.
 * @param event_callback USB controller event callback.
 * @return No value.
 */
void usb_hal_init(primary_callback, secondary_callback, event_callback) int primary_callback;
int secondary_callback;
int event_callback;

{
    /* USB HAL init; persistent state is carried in USB device control register, USB device state and related record fields. */
    uint32_t descriptor_selector_high_bit;
    uint8_t *configuration_register;
    uint32_t *endpoint_state;
    uint32_t configuration_mask;
    uint32_t controller_value;
    uint32_t configuration_bits;
    uint32_t descriptor_selector_low_bits;

    usb_hal_cancel_all_io_requests();
    core_update_masked_register_bits(&usb_device_control_register, DATAPATH_RAM_OFFSET, 0x40000000);
    /* Repeat while (controller_value >> 0x1e & 1) != 0; the body advances or polls the state needed to leave the loop. */
    do {
        controller_value = usb_device_control_register;
    } while ((controller_value >> 0x1e & 1) != 0);
    usb_device_state = 4;
    core_endpoint_context = 0;
    /* Snapshot USB core id register before decoding its status or capability fields. */
    controller_value = usb_core_id_register;
    core_firmware_transfer_context = 0;
    usb_core_revision = controller_value & 0xffff;
    if (primary_callback == 0) {
        primary_callback = 0;
    }
    if (secondary_callback == 0) {
        secondary_callback = 0;
    }
    if (event_callback == 0) {
        event_callback = 0;
    }
    controller_value = 0;
    endpoint_state = (uint32_t *)&core_state_055;
    usb_primary_state_callback = (firmware_callback_t)primary_callback;
    usb_secondary_state_callback = (firmware_callback_t)secondary_callback;
    core_usb_event_callback = (firmware_callback_t)event_callback;
    /* Repeat while controller_value < 4; the body advances or polls the state needed to leave the loop. */
    do {
        endpoint_state[10] = 0;
        descriptor_selector_low_bits = controller_value << 1;
        endpoint_state[0xe] = 0;
        descriptor_selector_high_bit = controller_value >> 7;
        endpoint_state[0xf] = 0;
        endpoint_state[0x32] = 0;
        controller_value = controller_value + 1;
        endpoint_state[0x36] = 0;
        endpoint_state[0x37] = 0;
        *(uint32_t *)(&usb_endpoint_command_register + (descriptor_selector_low_bits & 0xfffffeff | descriptor_selector_high_bit) * 0x10) = 0;
        *(uint32_t *)(&usb_endpoint_command_register + ((descriptor_selector_high_bit | descriptor_selector_low_bits & 0xffffeff) << 4 | 0x10)) = 0;
        endpoint_state = endpoint_state + 10;
    } while (controller_value < 4);
    /* Program USB event buffer address high register with 0; write ordering is hardware-significant. */
    usb_event_buffer_address_high_register = 0;
    usb_event_queue_cursor = &usb_event_buffer;
    /* Program USB event buffer address low register with 0xc0010a00; write ordering is hardware-significant. */
    usb_event_buffer_address_low_register = 0xc0010a00;
    usb_event_buffer_size_register = 0x200;
    /* Program USB event count register with 0; write ordering is hardware-significant. */
    usb_event_count_register = 0;
    core_command_status_update = 0x5e0004;
    /* Program USB device event enable register with 0x1f; write ordering is hardware-significant. */
    usb_device_event_enable_register = 0x1f;
    usb_event_queue_base = (uint32_t)usb_event_queue_cursor;
    if (emulation_platform == 0) {
        if (0x1319 < usb_core_revision)
            goto usb_hal_init_before_update_masked_register_bits;
        configuration_mask = 0x400;
        configuration_register = (uint8_t *)&usb3_pipe_control_register;
        configuration_bits = 0x400;
    } else {
        configuration_mask = 0xfff80000;
        configuration_bits = 0x75300000;
        configuration_register = (uint8_t *)&usb_global_control_register;
    }
    core_update_masked_register_bits(configuration_register, configuration_mask, configuration_bits);
/* Shared target for USB HAL init before update masked register bits; incoming paths preserve the same state assumptions. */
usb_hal_init_before_update_masked_register_bits:
    if (0x1109 < usb_core_revision) {
        core_update_masked_register_bits(&usb3_pipe_control_register, 0x40000, 0x40000);
        core_update_masked_register_bits(&usb_global_control_register, 1, 0);
        core_update_masked_register_bits(&usb3_pipe_control_register, 0x20000, 0x20000);
        core_update_masked_register_bits(&usb2_phy_configuration_register, 0x40, 0);
        core_update_masked_register_bits(&usb_global_control_register, 0x10000, 0x10000);
    }
    if (0x108a < usb_core_revision) {
        mww_process_command_word_channel_mww_command_channel(0, 9, 0, 0);
    }
    core_update_usb_transfer_state_for_usb_state(0, 1);
    core_state_051 = 1;
    core_update_clear_memory_usb_event_counter();
    usb_stack_ready = 1;
    rti_delay_milliseconds(0x32);
    return;
}

/**
 * @brief Handle a USB link-state-change device event.
 *
 * @param controller_status USB controller status word.
 * @return No value.
 */
void usb_hal_handle_link_state_change(controller_status) uint32_t controller_status;

{
    /* USB HAL handle link state change; persistent state is carried in USB core revision, USB device status register and related record fields. */
    uint32_t controller_status_bits;
    uint32_t link_state;

    if (usb_core_revision < 0x110a) {
        /* Snapshot USB device status register before decoding its status or capability fields. */
        controller_status_bits = usb_device_status_register;
        link_state = (controller_status_bits & 0x3fffff) >> 0x12;
    /* Shared target for USB HAL handle link state change before update masked register bits; incoming paths preserve the same state assumptions. */
    usb_hal_handle_link_state_change_before_update_masked_register_bits:
        if ((usb_core_revision < 0x110a) && (controller_status_bits = usb_device_state, usb_device_state == 3))
            goto usb_hal_handle_link_state_change_before_updating_link_state;
        if (link_state != 4) {
            if (link_state != 3) {
                core_update_masked_register_bits(&usb2_phy_configuration_register, 0x40, 0);
                goto usb_hal_handle_link_state_change_before_set_CDDIS_REG_OFF_to_hex_38_handler;
            }
            core_update_masked_register_bits(&usb2_phy_configuration_register, 0x40, 0x40);
            usb_link_event_code = 8;
            usb_event_callback_status = 0;
            if (core_endpoint_context < 2)
                goto usb_hal_handle_link_state_change_before_update_cddis_reg_off_cartridge_eject_control;
            goto usb_invoke_link_state_callback;
        }
    } else {
        link_state = (controller_status & 0xfffff) >> 0x10;
        controller_status_bits = controller_status >> 0x15;
        if ((controller_status >> 0x14 & 1) == 0)
            goto usb_hal_handle_link_state_change_before_update_masked_register_bits;
    /* Shared target for USB HAL handle link state change before updating link state; incoming paths preserve the same state assumptions. */
    usb_hal_handle_link_state_change_before_updating_link_state:
        if (link_state == 2)
            goto usb_hal_handle_link_state_change_before_update_cddis_reg_off_cartridge_eject_control;
        if (link_state == 3) {
            usb_link_event_code = 1;
        /* Shared target for USB invoke link state callback; incoming paths preserve the same state assumptions. */
        usb_invoke_link_state_callback:
            usb_event_callback_status = 0;
            if (core_usb_event_callback != (firmware_callback_t)0) {
                usb_event_callback_status = 0;
                /* Notify core USB event callback only after the associated state fields have been committed. */
                (*core_usb_event_callback)();
            }
            goto usb_hal_handle_link_state_change_before_update_cddis_reg_off_cartridge_eject_control;
        }
        controller_status_bits = 0;
        if (link_state != 4) {
            if (link_state == 8) {
                core_set_CDDIS_REG_OFF_to_hex_38_handler();
                if (usb_generic_command_pending == '\0') {
                    return;
                }
                /* Program USB generic command parameter register with 0; write ordering is hardware-significant. */
                usb_generic_command_parameter_register = 0;
                usb_generic_command_pending = 0;
                /* Program USB generic command register with 0x403; write ordering is hardware-significant. */
                usb_generic_command_register = 0x403;
                return;
            }
        /* Shared target for USB HAL handle link state change before set CDDIS REG OFF to hex 38 handler; incoming paths preserve the same state assumptions. */
        usb_hal_handle_link_state_change_before_set_CDDIS_REG_OFF_to_hex_38_handler:
            core_set_CDDIS_REG_OFF_to_hex_38_handler();
            return;
        }
    }
    if (usb_core_revision < 0x109c) {
        core_update_firmware_context_usb_transfer_state(controller_status_bits);
    }
/* Shared target for USB HAL handle link state change before update cddis reg off cartridge eject control; incoming paths preserve the same state assumptions. */
usb_hal_handle_link_state_change_before_update_cddis_reg_off_cartridge_eject_control:
    core_update_cddis_reg_off_cartridge_eject_control();
    return;
}

/**
 * @brief Initialize the USB subsystem state and required hardware resources.
 *
 * @return No value.
 */
void usb_stack_init()

{
    /* Stack init; persistent state is carried in MWW transfer active, MWW source descriptor and related record fields. */
    mww_transfer_active = 0;
    core_update_firmware_record_update(0x83, &usb_stack_mww_initialization_entry);
    core_update_firmware_record_update(3, 0x8004ac5);
    core_update_device_base_device_base_context(0x21, 0x80074a1);
    mww_source_descriptor = &usb_to_sata_transfer_descriptor;
    mww_active_descriptor = &sata_to_usb_transfer_descriptor;
    core_update_firmware_context_primary_firmware_callback(0x8009ec5);
    return;
}

/**
 * @brief Dispatch one non-endpoint USB device event.
 *
 * @param event_status USB controller event status word.
 * @return No value.
 */
void usb_hal_handle_device_event(event_status) uint32_t event_status;

{
    /* USB HAL handle device event; persistent state is carried in USB core revision. */
    uint32_t event_code;

    event_code = (event_status & 0xfff) >> 8;
    if (event_code == 0) {
        if (0x109b < usb_core_revision) {
            core_update_firmware_context_usb_transfer_state();
            return;
        }
    } else {
        if (event_code != 1) {
            if (event_code == 2) {
                core_process_usb_transfer_state();
                return;
            }
            if (event_code != 3) {
                if (event_code != 4) {
                    return;
                }
                core_update_firmware_context_usb_completion_state();
                return;
            }
            usb_hal_handle_link_state_change(event_status);
            return;
        }
        core_increment_usb_event_counter();
        usb_hal_handle_usb_reset();
    }
    return;
}

/**
 * @brief Recover a USB event interrupt that remains asserted with an empty event count.
 *
 * @return No value.
 */
void usb_hal_recover_stalled_event_interrupt()

{
    /* USB HAL recover stalled event interrupt; persistent state is carried in core USB event counter, USB endpoint enable register and related record fields. */
    int pending_interrupt_count;
    int controller_state;
    int current_sequence;

    current_sequence = core_usb_event_counter;
    /* Snapshot USB endpoint enable register before decoding its status or capability fields. */
    controller_state = usb_endpoint_enable_register;
    if (((usb_previous_controller_state == 0xcf) && (controller_state == 3)) && (core_usb_event_counter == usb_previous_event_sequence)) {
        usb_stalled_event_sequence = core_usb_event_counter;
        usb_event_stall_pending = 1;
    }
    if ((usb_event_stall_pending != 0) && (pending_interrupt_count = usb_event_count_register, pending_interrupt_count == 0)) {
        if (core_usb_event_counter == usb_stalled_event_sequence) {
            usb_hal_handle_usb_reset();
        }
        usb_event_stall_pending = 0;
    }
    usb_previous_controller_state = controller_state;
    usb_previous_event_sequence = current_sequence;
    return;
}

/**
 * @brief Decode the negotiated USB connection speed from DSTS.
 *
 * @return Result produced by the procedure.
 */
uint32_t usb_hal_get_connect_speed()

{
    /* USB HAL get connect speed; persistent state is carried in USB device status register. */
    uint32_t raw_link_state;
    uint32_t normalized_link_state;

    /* Snapshot USB device status register before decoding its status or capability fields. */
    raw_link_state = usb_device_status_register;
    raw_link_state = raw_link_state & 7;
    if (raw_link_state == 0) {
        return 2;
    }
    if (raw_link_state != 1) {
        if (raw_link_state == 2) {
            return 0;
        }
        if (raw_link_state != 3) {
            if (raw_link_state == 4) {
                normalized_link_state = 3;
            } else {
                normalized_link_state = 4;
            }
            return normalized_link_state;
        }
    }
    return 1;
}

/**
 * @brief Reset endpoint and device state after a USB reset event.
 *
 * @return No value.
 */
void usb_hal_handle_usb_reset()

{
    /* USB HAL handle USB reset; persistent state is carried in USB endpoint enable register, core state 062 and related record fields. */
    bool reset_required;

    /* Program USB endpoint enable register with 3; write ordering is hardware-significant. */
    usb_endpoint_enable_register = 3;
    core_state_062 = 0;
    core_state_063 = 0;
    reset_required = core_firmware_transfer_context == 0;
    if (reset_required) {
        core_process_firmware_context_endpoint_context();
    }
    usb_stack_ready = reset_required;
    return;
}

/**
 * @brief Report whether the current USB link state is ready for the requested transition.
 *
 * @return Result produced by the procedure.
 */
uint32_t usb_hal_get_link_readiness()

{
    /* USB HAL get link readiness; persistent state is carried in USB device status register. */
    uint32_t controller_status;
    uint32_t readiness_state;
    int readiness_check;

    /* Snapshot USB device status register before decoding its status or capability fields. */
    controller_status = usb_device_status_register;
    if ((controller_status & 0x3fffff) >> 0x12 == 4) {
        readiness_check = core_update_pin_update_context(4);
        if (readiness_check == 0) {
            readiness_state = 2;
        } else {
            readiness_state = 0;
        }
    } else {
        readiness_state = 1;
    }
    return readiness_state;
}

/**
 * @brief Cancel active transfers on every non-control endpoint.
 *
 * @return No value.
 */
void usb_hal_cancel_all_io_requests()

{
    /* Hal cancel all io requests using only caller-provided data. */
    uint32_t endpoint_number;

    endpoint_number = 1;
    /* Repeat while endpoint_number < 4; the body advances or polls the state needed to leave the loop. */
    do {
        core_process_wdt_reset(endpoint_number);
        core_process_wdt_reset(endpoint_number | 0x80);
        endpoint_number = endpoint_number + 1;
    } while (endpoint_number < 4);
    return;
}

/**
 * @brief Stop the USB controller and force the link back to receive-detect.
 *
 * @return No value.
 */
void usb_hal_disconnect()

{
    /* USB HAL disconnect; persistent state is carried in core pin update context, USB device control register and related record fields. */
    int disconnect_requested;
    uint32_t controller_status;

    core_pin_update_context = 1;
    disconnect_requested = usb_device_control_register;
    if (disconnect_requested < 0) {
        /* Call USB hal cancel all io requests through the defined unprototyped function-pointer signature. */
        ((void (*)())usb_hal_cancel_all_io_requests)(0);
        core_update_masked_register_bits(&usb_device_control_register, 0x80000000, 0);
        core_update_masked_register_bits(&usb_device_control_register, 0x1e0000, 0xa0000);
        if (usb_core_revision < 0x108a) {
            core_update_masked_register_bits(&usb_device_control_register, 0x40000000, 0x40000000);
            /* Repeat while (controller_status >> 0x1e & 1) != 0; the body advances or polls the state needed to leave the loop. */
            do {
                controller_status = usb_device_control_register;
            } while ((controller_status >> 0x1e & 1) != 0);
        }
        rti_delay_milliseconds(0x32);
    }
    return;
}

/**
 * @brief Get device descriptor.
 *
 * @return Result produced by the procedure.
 */
uint8_t *usb_get_device_descriptor()

{
    /* Get device descriptor using only caller-provided data. */
    return (uint8_t *)&usb_device_descriptor;
}
