/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_AHCI_H_H
#define TUSB9261_RDX_AHCI_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
int ahci_init_port();
/** @brief Firmware procedure. */
uint32_t ahci_port_intr_handler();
/** @brief Firmware procedure. */
int ahci_run_port_initialization_state_machine();
/** @brief Firmware procedure. */
bool ahci_is_link_active();
/** @brief Firmware procedure. */
bool ahci_init();
/** @brief Firmware procedure. */
bool ahci_execute_taskfile_command();
/** @brief Firmware procedure. */
void ahci_fatal_error_recovery();
/** @brief Firmware procedure. */
uint32_t ahci_port_reset();
/** @brief Firmware procedure. */
void ahci_set_port_speed();
/** @brief Firmware procedure. */
uint32_t ahci_stop();
/** @brief Firmware procedure. */
uint32_t ahci_wait_complete();
/** @brief Firmware procedure. */
bool ahci_continue_port_initialization();
/** @brief Firmware procedure. */
void ahci_get_TFD_info();
/** @brief Firmware procedure. */
void ahci_start();
/** @brief Firmware procedure. */
uint32_t ahci_hba_reset();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_AHCI_H_H */
