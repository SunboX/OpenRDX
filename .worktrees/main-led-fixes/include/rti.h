/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
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
void rti_forward_rtisetint_reg_off_rtisetint();
/** @brief Firmware procedure. */
void rti_enable_counter_when_requested();
/** @brief Firmware procedure. */
void rti_delay_microseconds_with_watchdog();
/** @brief Firmware procedure. */
void rti_init();
/** @brief Firmware procedure. */
void rti_start_periodic_interrupts();
/** @brief Firmware procedure. */
void rti_process_thermal_timer_state();
/** @brief Firmware procedure. */
void rti_config_compare_interrupt();
/** @brief Firmware procedure. */
void rti_delay_microseconds();
/** @brief Firmware procedure. */
void rti_delay_milliseconds();
/** @brief Firmware procedure. */
void rti_update_interrupt_enabled_rtisetint_reg_off();
/** @brief Firmware procedure. */
void rti_enable_counter_if_requested();
/** @brief Firmware procedure. */
void rti_update_masked_register_bits();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_RTI_H_H */
