/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_VIM_NVIC_H_H
#define TUSB9261_RDX_VIM_NVIC_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void VIM_init();
/** @brief Firmware procedure. */
void nvic_init();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_VIM_NVIC_H_H */
