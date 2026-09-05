/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hal_isr.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the usb hal isr module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the usb hal isr module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void usb_hal_isr()

{
    /* USB HAL isr; persistent state is carried in USB event count register, USB event queue cursor and related record fields. */
    uint32_t pending_byte_count;
    uint32_t event_word;
    uint32_t *next_queue_entry;
    uint32_t queued_word_count;

    /* Snapshot USB event count register before decoding its status or capability fields. */
    pending_byte_count = usb_event_count_register;
    /* Iterate while pending_byte_count = pending_byte_count & 0xffff, pending_byte_count != 0, preserving the defined cursor and bound. */
    while (pending_byte_count = pending_byte_count & 0xffff, pending_byte_count != 0) {
        queued_word_count = pending_byte_count + 3 >> 2;
        /* Repeat while queued_word_count != 0; the body advances or polls the state needed to leave the loop. */
        do {
            next_queue_entry = (uint32_t *)usb_event_queue_cursor + 1;
            event_word = *usb_event_queue_cursor;
            usb_event_queue_cursor = next_queue_entry;
            if (usb_event_queue_base + 0x80 <= (uint32_t)next_queue_entry) {
                usb_event_queue_cursor = (volatile uint32_t *)usb_event_queue_base;
            }
            if ((event_word & 1) == 0) {
                mww_handle_usb_endpoint_event(event_word, event_word >> 1);
            } else if ((event_word & 0xfe) == 0) {
                usb_hal_handle_device_event();
            }
            queued_word_count = queued_word_count - 1;
        } while (queued_word_count != 0);
        /* Program USB event count register with pending_byte_count; write ordering is hardware-significant. */
        usb_event_count_register = pending_byte_count;
        /* Snapshot USB event count register before decoding its status or capability fields. */
        pending_byte_count = usb_event_count_register;
    }
    return;
}
