/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_GIO_H_H
#define TUSB9261_RDX_GIO_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void gio_configure_logical_pin_interrupt();
/** @brief Configure GPIO10 as the active-low RDX SATA power-enable output. */
void gio_enable_rdx_sata_power();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_GIO_H_H */
