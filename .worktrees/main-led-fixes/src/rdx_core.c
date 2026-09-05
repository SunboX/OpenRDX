/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_core.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the RDX core module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the RDX core module.
 */

#include "../include/rdx_firmware.h"

#include "rdx_core/01_image_validation.inc"
#include "rdx_core/02_management_descriptors.inc"
#include "rdx_core/03_identity_and_mechanism.inc"
#include "rdx_core/04_hash_flash_usb_thermal.inc"
#include "rdx_core/05_device_descriptor_dispatch.inc"
#include "rdx_core/06_record_transfer_helpers.inc"
#include "rdx_core/07_eject_reconnect_smart.inc"
#include "rdx_core/08_thermal_and_record_validation.inc"
#include "rdx_core/09_command_phase_and_ata.inc"
#include "rdx_core/10_spi_flash_and_response_dispatch.inc"
#include "rdx_core/11_usb_ata_payload_transfer.inc"
#include "rdx_core/12_ahci_smart_spi_control.inc"
#include "rdx_core/13_runtime_transfer_helpers.inc"
#include "rdx_core/14_mww_usb_command_control.inc"
#include "rdx_core/15_control_transfer_crc_records.inc"
#include "rdx_core/16_record_builders_and_capacity.inc"
#include "rdx_core/17_cartridge_state_and_validation.inc"
#include "rdx_core/18_firmware_transfer_and_persistence.inc"
#include "rdx_core/19_identity_sense_and_mww.inc"
#include "rdx_core/20_command_state_accessors.inc"
#include "rdx_core/21_persistence_and_state_setters.inc"
#include "rdx_core/22_state_queries_and_hardware_helpers.inc"
#include "rdx_core/23_low_level_accessors.inc"
#include "rdx_core/24_reconnect_state_machine.inc"
