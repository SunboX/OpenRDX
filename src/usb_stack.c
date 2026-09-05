/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
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
    /* USB HAL init; persistent state is carried in USB device control register, usb device speed and related record fields. */
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
    usb_device_speed = 4;
    usb_device_connection_state = 0;
    /* Snapshot USB core id register before decoding its status or capability fields. */
    controller_value = usb_core_id_register;
    usb_ep0_state = 0;
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
    endpoint_state = (uint32_t *)&usb_self_powered_capable;
    usb_setup_packet_callback = (firmware_callback_t)primary_callback;
    usb_data_transfer_callback = (firmware_callback_t)secondary_callback;
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
    usb_device_configuration_register = 0x5e0004;
    /* Program USB device event enable register with 0x1f; write ordering is hardware-significant. */
    usb_device_event_enable_register = 0x1f;
    usb_event_queue_base = (uint32_t)usb_event_queue_cursor;
    if (emulation_platform == 0) {
        if (0x1319 < usb_core_revision)
            goto usb_init_apply_revision_configuration;
        configuration_mask = 0x400;
        configuration_register = (uint8_t *)&usb3_pipe_control_register;
        configuration_bits = 0x400;
    } else {
        configuration_mask = 0xfff80000;
        configuration_bits = 0x75300000;
        configuration_register = (uint8_t *)&usb_global_control_register;
    }
    core_update_masked_register_bits(configuration_register, configuration_mask, configuration_bits);
/* Shared target for usb init apply revision configuration; incoming paths preserve the same state assumptions. */
usb_init_apply_revision_configuration:
    if (0x1109 < usb_core_revision) {
        core_update_masked_register_bits(&usb3_pipe_control_register, 0x40000, 0x40000);
        core_update_masked_register_bits(&usb_global_control_register, 1, 0);
        core_update_masked_register_bits(&usb3_pipe_control_register, 0x20000, 0x20000);
        core_update_masked_register_bits(&usb2_phy_configuration_register, 0x40, 0);
        core_update_masked_register_bits(&usb_global_control_register, 0x10000, 0x10000);
    }
    if (0x108a < usb_core_revision) {
        mww_send_usb_endpoint_command(0, 9, 0, 0);
    }
    core_update_usb_transfer_state_for_usb_state(0, 1);
    usb_ep0_setup_configuration_pending = 1;
    core_clear_usb_event_counters();
    usb_reset_initialization_complete = 1;
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
    /* Shared target for usb link update usb2 phy state; incoming paths preserve the same state assumptions. */
    usb_link_update_usb2_phy_state:
        if ((usb_core_revision < 0x110a) && (controller_status_bits = usb_device_speed, usb_device_speed == 3))
            goto usb_link_dispatch_event_state;
        if (link_state != 4) {
            if (link_state != 3) {
                core_update_masked_register_bits(&usb2_phy_configuration_register, 0x40, 0);
                goto usb_link_disable_dynamic_clock_gating;
            }
            core_update_masked_register_bits(&usb2_phy_configuration_register, 0x40, 0x40);
            usb_suspend_countdown = 8;
            usb_power_management_state = 0;
            if (usb_device_connection_state < 2)
                goto usb_link_enable_sleep_clock_gating;
            goto usb_invoke_link_state_callback;
        }
    } else {
        link_state = (controller_status & 0xfffff) >> 0x10;
        controller_status_bits = controller_status >> 0x15;
        if ((controller_status >> 0x14 & 1) == 0)
            goto usb_link_update_usb2_phy_state;
    /* Shared target for usb link dispatch event state; incoming paths preserve the same state assumptions. */
    usb_link_dispatch_event_state:
        if (link_state == 2)
            goto usb_link_enable_sleep_clock_gating;
        if (link_state == 3) {
            usb_suspend_countdown = 1;
        /* Shared target for USB invoke link state callback; incoming paths preserve the same state assumptions. */
        usb_invoke_link_state_callback:
            usb_power_management_state = 0;
            if (core_usb_event_callback != (firmware_callback_t)0) {
                usb_power_management_state = 0;
                /* Notify core USB event callback only after the associated state fields have been committed. */
                (*core_usb_event_callback)();
            }
            goto usb_link_enable_sleep_clock_gating;
        }
        controller_status_bits = 0;
        if (link_state != 4) {
            if (link_state == 8) {
                core_disable_bus_dynamic_clock_gating();
                if (usb_function_wake_notification_pending == '\0') {
                    return;
                }
                /* Program USB generic command parameter register with 0; write ordering is hardware-significant. */
                usb_generic_command_parameter_register = 0;
                usb_function_wake_notification_pending = 0;
                /* Program USB generic command register with 0x403; write ordering is hardware-significant. */
                usb_generic_command_register = 0x403;
                return;
            }
        /* Shared target for usb link disable dynamic clock gating; incoming paths preserve the same state assumptions. */
        usb_link_disable_dynamic_clock_gating:
            core_disable_bus_dynamic_clock_gating();
            return;
        }
    }
    if (usb_core_revision < 0x109c) {
        core_notify_usb_disconnect(controller_status_bits);
    }
/* Shared target for usb link enable sleep clock gating; incoming paths preserve the same state assumptions. */
usb_link_enable_sleep_clock_gating:
    core_enable_processor_sleep_clock_gating();
    return;
}

/**
 * @brief Initialize the USB subsystem state and required hardware resources.
 *
 * @return No value.
 */
void usb_stack_init()

{
    /* Stack init; persistent state is carried in usb bot persistent stall, usb bot cbw pointer and related record fields. */
    usb_bot_persistent_stall = 0;
    core_register_usb_bot_transfer_callback(0x83, &usb_bot_in_transfer_complete_callback_entry);
    core_register_usb_bot_transfer_callback(3, 0x8004ac5);
    core_register_usb_request_callback(0x21, 0x80074a1);
    usb_bot_cbw_pointer = &usb_mass_storage_command_buffers;
    usb_bot_csw_pointer = &usb_mass_storage_status_buffers;
    core_register_usb_bot_reset_callback(0x8009ec5);
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
            core_notify_usb_disconnect();
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
                core_notify_usb_resume();
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
    if (((usb_previous_endpoint_enable_mask == 0xcf) && (controller_state == 3)) && (core_usb_event_counter == usb_previous_event_sequence)) {
        usb_stalled_event_sequence = core_usb_event_counter;
        usb_event_stall_pending = 1;
    }
    if ((usb_event_stall_pending != 0) && (pending_interrupt_count = usb_event_count_register, pending_interrupt_count == 0)) {
        if (core_usb_event_counter == usb_stalled_event_sequence) {
            usb_hal_handle_usb_reset();
        }
        usb_event_stall_pending = 0;
    }
    usb_previous_endpoint_enable_mask = controller_state;
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
    /* USB HAL handle USB reset; persistent state is carried in USB endpoint enable register, usb u1 enabled and related record fields. */
    bool reset_required;

    /* Program USB endpoint enable register with 3; write ordering is hardware-significant. */
    usb_endpoint_enable_register = 3;
    usb_u1_enabled = 0;
    usb_u2_enabled = 0;
    reset_required = usb_ep0_state == 0;
    if (reset_required) {
        core_handle_usb_bus_reset();
    }
    usb_reset_initialization_complete = reset_required;
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
        readiness_check = core_check_usb_reconnect_timer(4);
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
        core_end_usb_endpoint_transfer(endpoint_number);
        core_end_usb_endpoint_transfer(endpoint_number | 0x80);
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
    /* USB HAL disconnect; persistent state is carried in core usb disconnect requested, USB device control register and related record fields. */
    int disconnect_requested;
    uint32_t controller_status;

    core_usb_disconnect_requested = 1;
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
