/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hal.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the usb hal module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the usb hal module.
 */

#include "../include/rdx_firmware.h"

/*
 * The USB pull-up must remain disabled until the defined RDX ATA-security
 * handshake succeeds. Keeping this state outside the fixed-address firmware
 * records avoids changing their defined packed layout.
 */
static uint8_t rdx_sata_authenticated;
static uint8_t rdx_sata_ready;

/**
 * @brief Configure and enable endpoints for the negotiated speed and storage transport.
 *
 * @return No value.
 */
void usb_hal_configure_endpts()

{
    /* USB HAL configure endpts; persistent state is carried in usb device speed, USB endpoint enable register and related record fields. */
    uint32_t controller_value;
    uint32_t endpoint_bank_count;

    if (usb_device_speed != 4) {
        /* Program USB endpoint enable register with 3; write ordering is hardware-significant. */
        usb_endpoint_enable_register = 3;
        /* Repeat while (controller_value >> 0x11 & 1) == 0; the body advances or polls the state needed to leave the loop. */
        do {
            /* Snapshot USB device status register before decoding its status or capability fields. */
            controller_value = usb_device_status_register;
        } while ((controller_value >> 0x11 & 1) == 0);
        usb_hal_cancel_all_io_requests(&usb_device_control_register, controller_value >> 0x12);
        if (0x108a < usb_core_revision) {
            mww_send_usb_endpoint_command(0, 0x20009, 0, 0);
        }
        core_configure_usb_endpoint(1, *(uint16_t *)(&usb_endpoint_one_max_packet_size_by_speed + usb_device_speed * 2), 0);
        core_configure_usb_endpoint(0x81, *(uint16_t *)(&usb_endpoint_one_max_packet_size_by_speed + usb_device_speed * 2), 1);
        core_process_mww_process_command(1);
        core_process_mww_process_command(0x81);
        controller_value = 0xf;
        if (usb_device_speed != 0) {
            endpoint_bank_count = 1;
            if (usb_active_mass_storage_class == 1) {
                if (usb_device_speed == 3) {
                    endpoint_bank_count = 2;
                }
                core_process_channel_completion_usb_transfer_state(2, *(uint16_t *)(&usb_endpoint_two_max_packet_size_by_speed + usb_device_speed * 2), 1, 0);
                core_process_mww_process_command(2);
                core_process_channel_completion_usb_transfer_state(0x82, *(uint16_t *)(&usb_endpoint_two_max_packet_size_by_speed + usb_device_speed * 2), endpoint_bank_count, 2);
                core_process_mww_process_command(0x82);
                controller_value = 0x3f;
            }
            core_process_channel_completion_usb_transfer_state(3, *(uint16_t *)(&usb_endpoint_three_max_packet_size_by_speed + usb_device_speed * 2), endpoint_bank_count, 0);
            core_process_mww_process_command(3);
            core_process_channel_completion_usb_transfer_state(0x83, *(uint16_t *)(&usb_endpoint_three_max_packet_size_by_speed + usb_device_speed * 2), endpoint_bank_count, 3);
            core_process_mww_process_command(0x83);
            controller_value = controller_value | 0xc0;
        }
        /* Program USB endpoint enable register with controller_value; write ordering is hardware-significant. */
        usb_endpoint_enable_register = controller_value;
    }
    return;
}

/**
 * @brief Connect state or data used by the USB subsystem.
 *
 * @return No value.
 */
void usb_hal_connect()

{
    /* Never publish an unauthenticated or ordinary SATA device to the host. */
    if ((rdx_sata_authenticated == 0U) || (rdx_sata_ready == 0U)) {
        return;
    }

    /* USB HAL connect; persistent state is carried in USB device control register. */
    core_update_masked_register_bits(&usb_device_control_register, 0x80000000, 0x80000000);
    return;
}

/**
 * @brief Record a successful RDX ATA-security handshake.
 *
 * @return No value.
 */
void rdx_sata_usb_authenticate()

{
    rdx_sata_authenticated = 1U;
    return;
}

/**
 * @brief Publish a fully initialized and authenticated RDX cartridge to USB.
 *
 * @return No value.
 */
void rdx_sata_usb_publish()

{
    if (rdx_sata_authenticated == 0U) {
        return;
    }
    rdx_sata_ready = 1U;
    usb_hal_connect();
    return;
}

/**
 * @brief Withdraw the bridge before probing or after rejecting a SATA device.
 *
 * @return No value.
 */
void rdx_sata_usb_reject()

{
    uint8_t was_authenticated;

    was_authenticated = rdx_sata_authenticated;
    rdx_sata_authenticated = 0U;
    rdx_sata_ready = 0U;
    if (was_authenticated != 0U) {
        usb_hal_disconnect();
    }
    return;
}
