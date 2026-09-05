/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_USB_STACK_H_H
#define TUSB9261_RDX_USB_STACK_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void usb_hal_init();
/** @brief Firmware procedure. */
void usb_hal_handle_device_event();
/** @brief Firmware procedure. */
uint8_t *usb_get_device_descriptor();
/** @brief Firmware procedure. */
void usb_hal_disconnect();
/** @brief Firmware procedure. */
void usb_stack_init();
/** @brief Firmware procedure. */
void usb_hal_recover_stalled_event_interrupt();
/** @brief Firmware procedure. */
uint32_t usb_hal_get_connect_speed();
/** @brief Firmware procedure. */
void usb_hal_handle_usb_reset();
/** @brief Firmware procedure. */
uint32_t usb_hal_get_link_readiness();
/** @brief Firmware procedure. */
void usb_hal_cancel_all_io_requests();
/** @brief Firmware procedure. */
void usb_hal_handle_link_state_change();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_USB_STACK_H_H */
