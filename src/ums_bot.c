/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ums_bot.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the ums bot module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the ums bot module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Initialize the UMS subsystem state and required hardware resources.
 *
 * @return No value.
 */
void usb_mass_storage_init()

{
    /* Mass storage init using only caller-provided data. */
    uint32_t device_context;
    int device_context_address;

    core_initialize_usb_reconnect_state();
    core_allocate_software_timer(0x800f2cc);
    rti_set_software_timer(0x800f2cc, 800);
    core_initialize_led_controller_registry();
    core_process_flash_validation_status();
    /* Controller F29C uses logical outputs 8 then 9. Its boot selector is
     * zero; with no cartridge it illuminates dock/eject amber, so this pair's
     * physical order is amber then green while its state remains generic. */
    core_process_clear_memory_spi_response_context(
        0x800f29c,
        RDX_DOCK_AMBER_LOGICAL_OUTPUT,
        RDX_DOCK_GREEN_LOGICAL_OUTPUT);
    device_context = (uint32_t)core_get_cartridge_monitor_context(0);
    core_store_record_field_14_value(0x800f29c, device_context);
    core_store_context_enable_byte(0x800f29c, 1);
    core_process_lookup_arguments();
    core_handle_cartridge_monitor_context();
    device_context_address = (int)core_get_cartridge_monitor_context();
    *(uint8_t *)(device_context_address + 0x10) = 1;
    return;
}
