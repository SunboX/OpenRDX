/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_USB_HAL_H_H
#define TUSB9261_RDX_USB_HAL_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void usb_hal_connect();
void rdx_sata_usb_authenticate();
void rdx_sata_usb_publish();
void rdx_sata_usb_reject();
/** @brief Firmware procedure. */
void usb_hal_configure_endpts();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_USB_HAL_H_H */
