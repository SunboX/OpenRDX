/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
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
    /* Hid init; persistent state is carried in USB to SATA transfer descriptor, SATA to USB transfer descriptor. */
    ti_memset(&usb_to_sata_transfer_descriptor, 0, 0x120);
    ti_memset(&sata_to_usb_transfer_descriptor, 0, 0x120);
    return;
}
