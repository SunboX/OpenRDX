/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_SCI_H_H
#define TUSB9261_RDX_SCI_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void sci_init();
/** @brief Firmware procedure. */
uint32_t sci_read_logical_input();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_SCI_H_H */
