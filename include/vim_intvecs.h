/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_VIM_INTVECS_H_H
#define TUSB9261_RDX_VIM_INTVECS_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void _Bus_fault();
/** @brief Firmware procedure. */
void _Debug_monitor();
/** @brief Firmware procedure. */
void _Hard_fault();
/** @brief Firmware procedure. */
void _IO_PHANTOM_INT();
/** @brief Firmware procedure. */
void _MPU();
/** @brief Firmware procedure. */
void _NMI();
/** @brief Firmware procedure. */
void _PSR();
/** @brief Firmware procedure. */
void _SWI();
/** @brief Firmware procedure. */
void _SYSTick();
/** @brief Firmware procedure. */
void _Usage_fault();
/** @brief Firmware procedure. */
void ahci_isr();
/** @brief Firmware procedure. */
void ahci_rx_error_isr();
/** @brief Firmware procedure. */
void default_isr();
/** @brief Firmware procedure. */
void gio_isr();
/** @brief Firmware procedure. */
void mww_error_isr();
/** @brief Firmware procedure. */
void rti_compare0_isr();
/** @brief Firmware procedure. */
void rti_compare1_isr();
/** @brief Firmware procedure. */
void rti_compare2_isr();
/** @brief Firmware procedure. */
void rti_overflow0_isr();
/** @brief Firmware procedure. */
void rti_overflow1_isr();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_VIM_INTVECS_H_H */
