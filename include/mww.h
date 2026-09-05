/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_MWW_H_H
#define TUSB9261_RDX_MWW_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
uint32_t mww_configure_transfer_descriptor();
/** @brief Firmware procedure. */
void mww_handle_transfer_mww0_addr_device_transfer_state();
/** @brief Firmware procedure. */
void mww_handle_transfer_device_transfer_state();
/** @brief Firmware procedure. */
int mww_handle_saved_register_mww_saved_register();
/** @brief Firmware procedure. */
void mww_init();
/** @brief Firmware procedure. */
int mww_send_usb_endpoint_command();
/** @brief Firmware procedure. */
uint32_t mww_submit_usb_transfer();
/** @brief Firmware procedure. */
uint32_t mww_advance_completed_usb_trb();
/** @brief Firmware procedure. */
void mww_process_device_transfer_state();
/** @brief Firmware procedure. */
void mww_process_mww_transfer_storage();
/** @brief Firmware procedure. */
uint32_t mww_build_ahci_prd_table();
/** @brief Firmware procedure. */
void mww_reset_rw_offsets();
/** @brief Firmware procedure. */
void mww_init_read_only();
/** @brief Firmware procedure. */
void mww_init_write_only();
/** @brief Firmware procedure. */
void mww_update_mww_transfer_storage();
/** @brief Firmware procedure. */
uint32_t mww_force_sata_interface_ready();
/** @brief Firmware procedure. */
uint32_t mww_complete_usb_transfer();
/** @brief Firmware procedure. */
uint32_t mww_handle_usb_endpoint_event();
/** @brief Firmware procedure. */
void mww_transition_selection_state();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_MWW_H_H */
