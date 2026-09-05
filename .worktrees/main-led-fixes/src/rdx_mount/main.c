/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : main.c
//
// Project     : TUSB926x Firmware.
//
// Description : Main function.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   01/15/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 *
 * This file contains the main function.
 *
 */

#include "tusb9260.h"
#include "ahci.h"
#include "gio.h"
#include "mww.h"
#include "reg_io.h"
#include "rdx_hardware.h"
#include "rti.h"
#include "sci.h"
#include "spi.h"
#include "string.h"
#include "system.h"
#include "tusb9260_types.h"
#include "ums_bot.h"
#include "ums_uas.h"
#include "usb_hid.h"
#include "usb_stack.h"
#include "vim_nvic.h"
#include "wdt.h"

/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

volatile DATAPATH_RAM_T  *datapath_ram = (DATAPATH_RAM_T *)DATAPATH_RAM_OFFSET;  /* Set pointer to DP RAM */
UINT32_T rti_clock_mhz;
BOOLEAN_T emulation_platform;
BOOLEAN_T wfi_enable = FALSE;

#define CPU_ID_REG_OFF      0xE000ED00
#define SYSTEM_CTRL_REG_OFF 0xE000ED10
#define SLEEP_ON_EXIT_BIT   0x00000002

typedef enum 
{
    RESET_EXTERNAL = 0x0008,
    RESET_SW       = 0x0010,
    RESET_CPU      = 0x0020,
    RESET_WATCHDOG = 0x2000,
    RESET_POWERUP  = 0x8000

} RESET_FLAGS_T;

/*****************************************************************************
 * Function: main
 *************************************************************************//**
 * This function is starting point for the firmware called by c_int00().
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

int main(void)
{
    UINT32_T dev_id;
#if (NUM_AHCI_PORTS > 1)
    UINT32_T port_num;
#endif

    // Set default clock gating configuration.
    system_disable_clock_gating();

    // Disable MCU power management.
    WRITE32(SYSTEM_CTRL_REG_OFF, 0x0);

    // Clear ATA device structure.
    ti_memset(ata_dev, 0, sizeof(ata_dev));

    // Read device ID.
    dev_id = (READ32(DEVID_REG_OFF) & 0xFFFE0000) >> 17;

    // ASIC has a a device ID of zero.
    emulation_platform = (dev_id) ? TRUE : FALSE;

    // Set RTI clock rate.
    rti_clock_mhz = (emulation_platform) ? CPU_CLOCK_MHZ_FPGA : CPU_CLOCK_MHZ_ASIC;

    // Initialize UART.
    sci_init();

    // Initialize SPI.
    SPI_Init();

    // Initialize real-time interrupt module.
    rti_init();

    // Initialize GIOs.
    gio_init();


#if DEBUG_LEVEL >= 1
    UINT32_T exception_status;

    kprintf("\n========================================================\n");
    kprintf("||   TUSB926x Firmware v%u.%02u [%s %s]   ||\n", FIRMWARE_MAJOR_VERSION, FIRMWARE_MINOR_VERSION, __DATE__, __TIME__);
    kprintf("||              %s: 0x%04X                  ||\n", (dev_id == 0x0) ? "   Device ID" : "FPGA Version", dev_id );
    kprintf("========================================================\n\n");

    exception_status = READ32(SYSESR_REG_OFF);

    kprintf(" Reset Flag(s):");
    if (exception_status & RESET_EXTERNAL)
        kprintf(" [External]");
    if (exception_status & RESET_SW)
        kprintf(" [SW]");
    if (exception_status & RESET_CPU)
        kprintf(" [CPU]");
    if (exception_status & RESET_WATCHDOG)
        kprintf(" [Watchdog]");
    if (exception_status & RESET_POWERUP)
        kprintf(" [Power-Up]");

    kprintf("\n\n");

#endif

    /* Initialize the complete 80-KiB datapath RAM before MWW, USB, or AHCI.
     * This is functional initialization rather than debug decoration: the
     * region contains USB TRBs, BOT command/status buffers, AHCI command
     * lists, received FISes, and the physical MWW backing store. Leaving a
     * reset-persistent value in any of those structures can allow descriptor
     * traffic while the first wrapped sector transfer stalls indefinitely. */
    ti_memset((void *)datapath_ram, 0xEE, DATAPATH_RAM_SIZE);

    CRIT("Datapath RAM Usage: %u / %u bytes.\n", sizeof(DATAPATH_RAM_T), DATAPATH_RAM_SIZE);
    CRIT("Supported NCQ Depth: %u\n", AHCI_NCQ_DEPTH);
//    CRIT("Memory wrap window is %s\n", (DISABLE_WRAP_WINDOW == 1) ? "DISABLED" : "ENABLED");
//    CRIT("Setup pkt TRB is at 0x%08x\n", (UINT32_T)&datapath_ram->trb_ep0_setup_packet);
//    CRIT("EP2-IN TRB ptr = 0x%08x\n", (UINT32_T)&datapath_ram->trb_data_IN[EP2]);
//    CRIT("EP2-OUT TRB ptr = 0x%08x\n", (UINT32_T)&datapath_ram->trb_data_OUT[EP2]);
//    CRIT("EP3-IN TRB Ring ptr = 0x%08x\n", (UINT32_T)datapath_ram->trb_ring_IN);
//    CRIT("EP3-OUT TRB Ring ptr = 0x%08x\n", (UINT32_T)datapath_ram->trb_ring_OUT);
//    CRIT(" EP3-IN xfer active ptr = 0x%08x.\n", &usb_dev.ep_info_IN[EP3].bXferActive);
//    CRIT(" EP3-OUT xfer active ptr = 0x%08x.\n", &usb_dev.ep_info_OUT[EP3].bXferActive);
//    CRIT("Scrambling: %s\n", (DISABLE_SCRAMBLING) ? "OFF" : "ON");
    CRIT("U1/U2 Transistions: %s\n", (ENABLE_U1_U2_TRANSITIONS) ? "ON" : "OFF");
    CRIT("USB PHY Suspend: %s\n", (ENABLE_USB_PHY_SUSPEND) ? "ON" : "OFF");
    CRIT("SATA LPM: %s\n", (AHCI_LINK_POWER_MGMT_ENABLE) ? "ON" : "OFF");
//    CRIT("System Clock Gating: %s\n", (ENABLE_CLOCK_GATING) ? "ON" : "OFF");

    CRIT("Device is %s-powered.\n", (gio_is_usb_device_self_powered()) ? "Self" : "Bus");

    // Initialize memory wrap windows.
    mww_init();

    // Initialize SCSI.
    scsi_init();

    /* Claim all RDX inputs and outputs after Manager state has created its LED
     * controllers. This starts with the mechanism outputs in their idle
     * pattern, write protection fail-closed, and PWM0 owned by the fan
     * controller. */
    rdx_hardware_init();

    // Initialize USB stack (and USB HAL).
    usb_stack_init();

    // Initialize the USB Bulk-Only mass-storage transport.
    ums_bot_init();

    /* Initialize UAS immediately after BOT even though the product descriptor
     * publishes only the BOT alternate setting. The call clears shared
     * command/status buffers and installs the complete mass-storage callback
     * table; it does not publish a UAS interface. */
    ums_uas_init();

    /* Complete bounded synchronous SATA discovery before publishing USB.
     * A slow disk can otherwise block the foreground while the host is
     * waiting for its first descriptor response. A failed or empty bay still
     * publishes the same stable removable LUN in the not-ready state. */
    ahci_init();

    /* USB publication does not depend on media admission succeeding. */
    usb_hal_connect();

    /* Timed mechanism motion may begin only after blocking startup discovery
     * has returned and the foreground loop can enforce every deadline. */
    rdx_hardware_start();

    // Initialize watchdog timer.
    wdt_start();

    while (1)
    {
        // Reset watchdog.
        wdt_reset();

        /* A connect-change interrupt only records and isolates the event.
         * Perform link settling and cartridge discovery in foreground context
         * before processing any possibly stale ATA completion. */
        ahci_service();

#if (NUM_AHCI_PORTS == 1)

        if (ata_dev[0].callback_pending[ata_dev[0].callback_processing_index])
        {
            // Disable USB interrupt.
            WRITE_REG32(VIM_REQMASKCLR0, 0x00200000);

            /* Service exactly one queued ATA callback per foreground pass so
             * USB/BOT runs between SATA completions. Draining the whole queue
             * would keep the USB IRQ masked across multiple streaming
             * completions and could starve the MWW ring while the host waits
             * for READ(10) data or its CSW. */
            ata_dev[0].pAtaCallbackQueue[ata_dev[0].callback_processing_index](&ata_dev[0].ata_callback_data[ata_dev[0].callback_processing_index]);

            // Re-enable USB interrupt.
            WRITE_REG32(VIM_REQMASKSET0, 0x00200000);

            // Clear callback pending flag.
            ata_dev[0].callback_pending[ata_dev[0].callback_processing_index] = FALSE;

            // Increment processing index.
            ata_dev[0].callback_processing_index++;

            // Check for rollover.
            if (ata_dev[0].callback_processing_index >= ATA_CALLBACK_QUEUE_DEPTH)
            {
                ata_dev[0].callback_processing_index = 0;
            }
        }

#else 
        for (port_num = 0; port_num < NUM_AHCI_PORTS; port_num++)
        {
            if (ata_dev[port_num].callback_pending[ata_dev[port_num].callback_processing_index])
            {
                // Disable USB interrupt.
                WRITE_REG32(VIM_REQMASKCLR0, 0x00200000);

                // Execute callback.
                ata_dev[port_num].pAtaCallbackQueue[ata_dev[port_num].callback_processing_index](&ata_dev[port_num].ata_callback_data[ata_dev[port_num].callback_processing_index]);

                // Re-enable USB interrupt.
                WRITE_REG32(VIM_REQMASKSET0, 0x00200000);

                // Clear callback pending flag.
                ata_dev[port_num].callback_pending[ata_dev[port_num].callback_processing_index] = FALSE;

                // Increment processing index.
                ata_dev[port_num].callback_processing_index++;

                // Check for rollover.
                if (ata_dev[port_num].callback_processing_index >= ATA_CALLBACK_QUEUE_DEPTH)
                {
                    ata_dev[port_num].callback_processing_index = 0;
                }
            }
        }
#endif

        /* ADC and ATA SMART/flush operations belong in the foreground rather
         * than an RTI or USB ISR.  One pass also advances the non-blocking
         * eject motor and thermal/fan state machines. */
        rdx_hardware_service();

#if AHCI_LINK_POWER_MGMT_ENABLE
        // Poll for SATA interface errors.
        ahci_interface_error_isr();
#else
        // Wait For Interrupt to save power. (~3 mA @ 1.1V)
        // Note: Comment this out if you are using serial wire debugger or disable clock gating in system.h
        //       to prevent the clock from being disabled when processor is sleeping.
        // WFI negatively impacts UAS queued write performance so only enable when flag is set. - BQ 4/28/15
        if (wfi_enable)
        {
            wfi_enable = FALSE;
            asm(" wfi");  
        }
#endif

    } /* End while(1) */

}

