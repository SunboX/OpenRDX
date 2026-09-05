/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : scsi.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the scsi module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the scsi module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Initialize the SCSI subsystem state and required hardware resources.
 *
 * @return No value.
 */
void scsi_init()

{
    /* SCSI init; persistent state is carried in core device base context, USB core revision and related record fields. */
    ti_memset(&core_device_base_context, 0, 2);
    ti_memset(&usb_core_revision, 0, 0x204);
    ti_memset(&scsi_state_001, 0, 0x70);
    scsi_state_002 = 0x8009415;
    scsi_state_001 = 0x8009971;
    core_response_buffer = &usb_ep0_buffer;
    core_update_word_update_context();
    usb_hal_init(0x800a68d, &usb_secondary_state_entry, 0x800c081);
    return;
}
