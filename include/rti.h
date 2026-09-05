/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_RTI_H_H
#define TUSB9261_RDX_RTI_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void rti_suspend_compare_interrupt_for_timer();
/** @brief Firmware procedure. */
void rti_restore_compare_interrupt_for_timer();
/** @brief Firmware procedure. */
void rti_delay_microseconds_with_watchdog();
/** @brief Firmware procedure. */
void rti_init();
/** @brief Firmware procedure. */
void rti_start_periodic_interrupts();
/** @brief Firmware procedure. */
void rti_set_software_timer();
/** @brief Firmware procedure. */
void rti_config_compare_interrupt();
/** @brief Firmware procedure. */
void rti_delay_microseconds();
/** @brief Firmware procedure. */
void rti_delay_milliseconds();
/** @brief Firmware procedure. */
void rti_suspend_compare_interrupt();
/** @brief Firmware procedure. */
void rti_restore_compare_interrupt();
/** @brief Firmware procedure. */
void rti_update_masked_register_bits();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_RTI_H_H */
