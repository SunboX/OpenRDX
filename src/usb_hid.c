/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hid.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the usb hid module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the usb hid module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Initialize the HID subsystem state and required hardware resources.
 *
 * @return No value.
 */
void hid_init()

{
    /* Hid init; persistent state is carried in usb mass storage command buffers, usb mass storage status buffers. */
    ti_memset(&usb_mass_storage_command_buffers, 0, 0x120);
    ti_memset(&usb_mass_storage_status_buffers, 0, 0x120);
    return;
}
