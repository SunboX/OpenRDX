/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : main.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the main module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the main module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Main.
 *
 * @return No value.
 */
void main()

{
    /* Main using only caller-provided data. */
    uint32_t device_id;

    ti_memset(ATA_DEVICES_ADDRESS, 0, 0x1d0);
    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    device_id = DEVID_REG_OFF;
    emulation_platform = (uint32_t)(device_id >> 0x11 != 0);
    if (device_id >> 0x11 == 0) {
        rti_clock_mhz = 0x4b;
    } else {
        rti_clock_mhz = 0x28;
    }
    sci_init();
    SPI_Init();
    /* Program SYSTEM CTRL REG OFF with 0; write ordering is hardware-significant. */
    SYSTEM_CTRL_REG_OFF = 0;
    rti_init();
    /* The Tandberg pin table maps cartridge power to active-high GPIO11. */
    gio_enable_rdx_sata_power();
    rti_delay_milliseconds(500);
    ti_memset(DATAPATH_RAM_OFFSET, 0xee, 0x14000);
    mww_init();
    pwm_init_activity_led(0);
    pwm_init_heartbeat_led(0);
    scsi_init();
    usb_stack_init();
    usb_mass_storage_init();
    /* USB remains electrically disconnected until RDX ATA authentication succeeds. */
    ahci_init();
    wdt_start();
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
        wdt_reset();
        if ((&ata_callback_pending)[ata_callback_processing_index] != '\0') {
            core_mask_usb_interrupt();
            /* Invoke the callback selected by the active queue entry. */
            (**(firmware_callback_t **)(ata_callback_processing_index * 4 + ATA_CALLBACK_QUEUE_ADDRESS))(
                ata_callback_processing_index * 0xc + ATA_CALLBACK_DATA_ADDRESS);
            core_unmask_usb_interrupt_when_allowed(0);
            (&ata_callback_pending)[ata_callback_processing_index] = 0;
            ata_callback_processing_index = ata_callback_processing_index + 1;
            if (0xf < ata_callback_processing_index) {
                ata_callback_processing_index = 0;
            }
        }
        core_mask_usb_interrupt();
        core_process_prepare_runtime_services();
        core_resume_deferred_mww_dispatch();
        core_update_flash_update_context();
        core_unmask_usb_interrupt_when_allowed(0);
    } while (true);
}
