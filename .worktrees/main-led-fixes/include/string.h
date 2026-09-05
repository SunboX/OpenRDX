/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_STRING_H_H
#define TUSB9261_RDX_STRING_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
uint32_t *ti_memset();
/** @brief Firmware procedure. */
uint32_t *ti_memset_entry_wrapper();
/** @brief Firmware procedure. */
uint32_t *ti_memset_alternate_entry_wrapper();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_STRING_H_H */
