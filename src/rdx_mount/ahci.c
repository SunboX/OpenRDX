/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ahci.c
//
// Project     : TUSB926x Firmware.
//
// Description : SATA AHCI driver.
//               Compliant to ATA8-ACS Rev 6, AHCI Rev 1.1, and SATA Rev 2.6 specifications.
//               NOTE: Does not support hot plug, port multiplier, or staggered spin-up.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   03/04/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the Serial ATA Advanced Host 
 * Controller Interface (AHCI) driver.
 * 
 * The SATA AHCI driver is compliant with the following specifications:
 * - Serial ATA Advanced Host Controller Interface, Revision 1.1.
 * - Serial ATA Specification, Revision 2.6.
 * - AT Attachment 8 - ATA/ATAPI Command Set, Revision 6.
 * 
 * The driver does not support port multiplier or staggered
 * spin-up. When link power management is enabled, hot plug
 * operation is not supported because device removal 
 * cannot be properly detected.
 *  
 */

#include "ahci.h"
#include "gio.h"
#include "mww.h"
#include "pwm.h"
#include "reg_io.h"
#include "rdx_hardware.h"
#include "rdx_mechanism.h"
#include "sata_media.h"
#include "rti.h"  // usleep()
#include "sci.h"
#include "scsi_data.h"
#include "string.h"
#include "system.h"  // for global reset.
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "usb_hal.h"  // usb_dev
#include "usb_stack.h"
#include "vim_nvic.h" 
#include "wdt.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#if AHCI_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif

#define AHCI_SATA_RX_ERROR_INTERRUPT_MASK 0x00000020U
#define AHCI_CONTROLLER_INTERRUPT_MASK    0x00000040U
#define AHCI_USB_INTERRUPT_MASK           0x00200000U
#define AHCI_SATA_INTERRUPT_MASK \
    (AHCI_SATA_RX_ERROR_INTERRUPT_MASK | AHCI_CONTROLLER_INTERRUPT_MASK)
#define AHCI_MEDIA_PUBLICATION_INTERRUPT_MASK \
    (AHCI_SATA_INTERRUPT_MASK | AHCI_USB_INTERRUPT_MASK)


/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

ATA_DEVICE_INFO_T  ata_dev[NUM_AHCI_PORTS];  // Only 1 port so just one device struct.

UINT32_T gSATADeviceCount = 0;

BOOLEAN_T gDiskActivity = FALSE;

/* Connect-change ISR state. The interrupt path makes media unavailable and
 * records the event; foreground service performs every potentially blocking
 * stop, link-settle, discovery, and authentication operation. */
static volatile BOOLEAN_T ahci_hotplug_pending[NUM_AHCI_PORTS];
static volatile BOOLEAN_T ahci_hotplug_quiesced[NUM_AHCI_PORTS];
static volatile BOOLEAN_T ahci_reinit_wait_for_callbacks[NUM_AHCI_PORTS];

STATUS_T ahci_set_features_xfer_mode(UINT32_T port_num, BOOLEAN_T dma);
STATUS_T ahci_set_features_dma_auto_activate(UINT32_T port_num, BOOLEAN_T enable);
STATUS_T ahci_stop(UINT32_T port_num);
STATUS_T ahci_port_reset(UINT32_T port_num);
inline void ahci_ata_cbk_queue_add(
    UINT32_T port_num,
    void (*pCallback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk));

/**
 * @brief Return whether any deferred ATA callback remains on one port.
 *
 * @param port_num SATA port number.
 * @return TRUE while at least one callback slot is pending.
 */
static BOOLEAN_T ahci_callbacks_are_pending(UINT32_T port_num)
{
    UINT32_T callback_index;

    for (callback_index = 0U;
         callback_index < ATA_CALLBACK_QUEUE_DEPTH;
         callback_index++)
    {
        if (ata_dev[port_num].callback_pending[callback_index])
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Fail media closed and queue complete foreground rediscovery.
 *
 * @param port_num SATA port number.
 * @param wait_for_callbacks Preserve already queued completion callbacks first.
 */
static void ahci_schedule_media_discovery(
    UINT32_T port_num,
    BOOLEAN_T wait_for_callbacks)
{
    ata_dev[port_num].bDeviceInitComplete = FALSE;
    ata_dev[port_num].bDeviceInitTimedOut = TRUE;
    sata_media_reset(port_num);
#if REMOVABLE_MEDIA_DEVICE
    rdx_hardware_media_reinitializing(port_num);
#endif
    WRITE32(PxIE(port_num), 0U);
    WRITE_REG32(VIM_REQMASKCLR0, 0x00000020);
    if (!ahci_hotplug_pending[port_num])
    {
        ahci_hotplug_quiesced[port_num] = FALSE;
        ahci_reinit_wait_for_callbacks[port_num] = wait_for_callbacks;
    }
    else if (wait_for_callbacks)
    {
        ahci_reinit_wait_for_callbacks[port_num] = TRUE;
    }
    ahci_hotplug_pending[port_num] = TRUE;
}

/**
 * @brief Queue discovery for a debounced physical insertion if still unready.
 *
 * An empty RDX bay may retain PHY-ready while a discovery attempt fails.
 * The next GPIO-confirmed insertion must therefore be able to restart the
 * deferred path without requiring another SATA connect-change interrupt.
 * Keep an already admitted device and any queued callbacks intact.
 *
 * @param port_num SATA port number.
 */
void ahci_media_inserted(UINT32_T port_num)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return;
    }

    /* Exclude USB submission and both SATA handlers only while deciding who
     * owns discovery. All stop, reset, and admission work stays deferred. */
    WRITE_REG32(VIM_REQMASKCLR0,
                AHCI_MEDIA_PUBLICATION_INTERRUPT_MASK);
    if (!ata_dev[port_num].bDeviceInitComplete)
    {
        ahci_schedule_media_discovery(port_num, TRUE);
        WRITE_REG32(VIM_REQMASKSET0,
                    AHCI_CONTROLLER_INTERRUPT_MASK);
    }
    else
    {
        WRITE_REG32(VIM_REQMASKSET0, AHCI_SATA_INTERRUPT_MASK);
    }
    WRITE_REG32(VIM_REQMASKSET0, AHCI_USB_INTERRUPT_MASK);
}

/**
 * @brief Invalidate media after a locally completed command needs recovery.
 *
 * This path deliberately does not enqueue an ATA callback: its caller owns
 * completion of the active SCSI or foreground request. A requested reset
 * first stops DMA, releases a wrap-window stall, and performs COMRESET. The
 * port then remains isolated until foreground discovery admits media again.
 *
 * @param port_num SATA port number.
 * @param reset_port TRUE to abort an active slot with COMRESET.
 */
void ahci_recover_local_command(
    UINT32_T port_num,
    BOOLEAN_T reset_port)
{
    if (port_num >= NUM_AHCI_PORTS)
    {
        return;
    }

    WRITE_REG32(VIM_REQMASKCLR0, AHCI_SATA_INTERRUPT_MASK);
    WRITE32(PxIE(port_num), 0U);
    if (reset_port)
    {
        mww_force_sata_interface_ready();
        (void)ahci_stop(port_num);
        (void)ahci_port_reset(port_num);
    }
    ahci_schedule_media_discovery(port_num, FALSE);

    /* Keep receiver-error recovery isolated with the invalid media epoch.
     * AHCI must run so any already-latched connect change can be consumed. */
    WRITE_REG32(VIM_REQMASKSET0, AHCI_CONTROLLER_INTERRUPT_MASK);
}

/**
 * @brief Handle one terminal SATA link-change notification.
 *
 * @param port_num SATA port number.
 */
static void ahci_handle_media_link_change(UINT32_T port_num)
{
#if REMOVABLE_MEDIA_DEVICE
    BOOLEAN_T media_was_ready = ata_dev[port_num].bDeviceInitComplete;
#endif

    CRIT("AHCI port connect change.\n");
    WRITE32(PxSERR(port_num), PSERR_DIAG_X_BIT);

#if REMOVABLE_MEDIA_DEVICE
    if ((READ32(PxSSTS(port_num)) & PSSTS_DET_MASK) !=
        PSSTS_DET_PHY_READY)
    {
        sata_media_link_disconnected(port_num);
    }
    ahci_schedule_media_discovery(port_num, media_was_ready);
    if (media_was_ready &&
        (usb_dev.dev_state == USB_DEVICE_STATE_CONFIGURED) &&
        !ahci_callbacks_are_pending(port_num))
    {
        /* Complete transport cleanup before foreground discovery reuses the
         * command slot or waits for a later insertion. */
        ahci_ata_cbk_queue_add(
            port_num, ata_dev[port_num].pAtaErrorCallback);
    }
#else
    usb_hal_disconnect();
    ahci_schedule_media_discovery(port_num, FALSE);
#endif
}

#if DEBUG_LEVEL >= 1
void ahci_debug_dump(void)
{
    CRIT("SATA Reg Dump:\n");
    CRIT(" CLB  = 0x%08x\n", READ32(PxCLB(0)));                                       
    CRIT(" CLBU = 0x%08x\n", READ32(PxCLBU(0)));                                        
    CRIT(" FB   = 0x%08x\n", READ32(PxFB(0)));
    CRIT(" IS   = 0x%08x\n", READ32(PxIS(0)));
    CRIT(" IE   = 0x%08x\n", READ32(PxIE(0)));
    CRIT(" CMD  = 0x%08x\n", READ32(PxCMD(0)));
    CRIT(" TFD  = 0x%08x\n", READ32(PxTFD(0)));
    CRIT(" SCTL = 0x%08x\n", READ32(PxSCTL(0)));
    CRIT(" SERR = 0x%08x\n", READ32(PxSERR(0)));
    CRIT(" SSTS = 0x%08x\n", READ32(PxSSTS(0)));
    CRIT(" SACT = 0x%08x\n", READ32(PxSACT(0)));
    CRIT(" CI   = 0x%08x\n", READ32(PxCI(0)));

    return;
}
#endif 

/*****************************************************************************
 * Function: ahci_ata_cbk_queue_add
 *************************************************************************//**
 * This function adds a callback to the ATA callback queue.
 *
 * @param[in] port_num SATA port number.
 * @param[in] pCallback pointer to the function callback.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_ata_cbk_queue_add(UINT32_T port_num, void (*pCallback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk))
{
    if (pCallback)
    {
        if (ata_dev[(port_num)].callback_pending[ata_dev[(port_num)].callback_index])
        {
            CRIT("@Error: ATA Cbk Queue Full!\n"); 
        }

        ata_dev[(port_num)].pAtaCallbackQueue[ata_dev[(port_num)].callback_index] = pCallback;
        ata_dev[(port_num)].callback_pending[ata_dev[(port_num)].callback_index] = TRUE;
        ata_dev[(port_num)].callback_index++;

        if (ata_dev[(port_num)].callback_index >= ATA_CALLBACK_QUEUE_DEPTH)
        {
            ata_dev[(port_num)].callback_index = 0;
        }
    }

    return;
}


/*****************************************************************************
 * Function: ahci_clear_callback_queue
 *************************************************************************//**
 * This function re-initializes the callback queue.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_clear_callback_queue(UINT32_T port_num)
{
    UINT32_T i;

    DEBUG("-> ahci_clear_callback_queue()\n");

    for (i = 0; i < ATA_CALLBACK_QUEUE_DEPTH; i++)
    {
        ata_dev[port_num].callback_pending[i] = FALSE;
    }

    ata_dev[port_num].callback_index = 0;
    ata_dev[port_num].callback_processing_index = 0;
}


/*****************************************************************************
 * Function: ahci_wait_complete
 *************************************************************************//**
 * This function waits up to a specified number of milliseconds 
 * for a register's masked value to equal the completion value.
 *
 * @param[in] addr       register address.
 * @param[in] cmplt_mask completion mask.
 * @param[in] cmplt_val  completion value.
 * @param[in] timeout_ms timeout value in milliseconds.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when wait times out.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_wait_complete(UINT32_T addr, UINT32_T cmplt_mask, UINT32_T cmplt_val, INT32_T timeout_ms)
{
    UINT32_T reg_val;
    STATUS_T status = STATUS_OK;

    /* Loop until completion or timeout */
    while (timeout_ms > 0)
    {
        reg_val = READ_REG32(addr) & cmplt_mask;
        if (reg_val == cmplt_val) break;

        usleep(1000);
        timeout_ms--;

        // Reset watchdog.
        wdt_reset();
    }

    if (timeout_ms <= 0)
    {
        CRIT("-> ahci_wait_complete() timed out! Rd: 0x%08x = 0x%08x, cmplt_val = 0x%08x.\n", addr, reg_val, cmplt_val);
        status = STATUS_TIMEOUT;
    }
    else
    {
        INFO("-> ahci_wait_complete() - %d ms remaining waiting for addr[0x%08x] = 0x%08x.\n", timeout_ms, addr, cmplt_val);
    }

    return status;

}

/*****************************************************************************
 * Function: ahci_stop
 *************************************************************************//**
 * This function stops HBA command list processing for the specified port.
 *
 * @param[in] port_num SATA port number.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when command list fails to stop running.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_stop(UINT32_T port_num)
{
    INFO("-> ahci_stop()\n");

    // Clear PxCMD.ST
    MODIFY32(PxCMD(port_num), PCMD_ST_BIT, 0);

    // Wait for command list to stop running (at least 500 ms per AHCI 1.1).
    return ahci_wait_complete(PxCMD(port_num), PCMD_CR_BIT, 0x0, 2000);
}

/*****************************************************************************
 * Function: ahci_start
 *************************************************************************//**
 * This function starts HBA command list processing for the specified port.
 *
 * @param[in] port_num SATA port number.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_start(UINT32_T port_num)
{
    INFO("-> ahci_start()\n");

    // Clear port error register by writing ones.
    WRITE32(PxSERR(port_num), 0xFFFFFFFF);

    // Set Tx/Rx transaction size to 64 DWORDS. (It is cleared upon port reset).
    WRITE32(PxDMACR(port_num), 0x66);

    // Set PxCMD.ST
    MODIFY32(PxCMD(port_num), PCMD_ST_BIT, PCMD_ST_BIT);

    if (ata_dev[port_num].bPortReset && ata_dev[port_num].bDeviceInitComplete)
    {
        // Restore settings that were cleared upon port reset. 

        if (!ata_dev[port_num].bSoftwareSettingsPreservation)
        {
            // Send SET FEATURES command to set PIO transfer mode.
            ahci_set_features_xfer_mode(port_num, FALSE);

            // Send SET FEATURES command to set DMA transfer mode.
            ahci_set_features_xfer_mode(port_num, TRUE);
        }

        if (ata_dev[port_num].bDMA_SetupAutoActivateSupport)
        {
            // Enable DMA Setup FIS Auto-Activate feature.
            ahci_set_features_dma_auto_activate(port_num, TRUE);
        }

        // Clear D2H Reg FIS interrupt set during SET FEATURES command(s).
        WRITE32(PxIS(port_num), D2H_REGISTER_FIS_INTR);
    }

    // Clear port reset flag.
    ata_dev[port_num].bPortReset = FALSE;

    // Clear SActive flags.
    ata_dev[port_num].dSActive = 0;

    return;
}


/*****************************************************************************
 * Function: ahci_hba_reset
 *************************************************************************//**
 * This function resets the entire HBA.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when reset fails to complete.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_hba_reset(void)
{
    STATUS_T status;

    CRIT("-> ahci_hba_reset()\n");

    // Global Host Reset.
    WRITE32(AHCI_REG_OFF(GHC_REG_OFF), GHC_HBA_RESET_BIT);

    // Wait up to 1 sec per AHCI 1.1 for reset to complete.
    status = ahci_wait_complete(AHCI_REG_OFF(GHC_REG_OFF), GHC_HBA_RESET_BIT, 0x0, 1000);

    if (status != STATUS_OK)
    {
        CRIT("@Error: HBA global reset failed!\n");
    }

    return status;
}


#define MAX_NUM_PORT_RESET_RETRIES    12

/*****************************************************************************
 * Function: ahci_port_reset
 *************************************************************************//**
 * This function performs a COMRESET and waits for communication to be 
 * established.  If device presence is not detected, the process is retried
 * up to MAX_NUM_PORT_RESET_RETRIES times. ahci_stop() must be called before 
 * using this function if PxCMD.ST is set.
 *
 * @param[in] port_num SATA port number.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when COMINIT is never detected.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_port_reset(UINT32_T port_num)
{
    BOOLEAN_T pccs_restore;
    STATUS_T status;
    UINT32_T retry_cnt = 0;

    // Save the current state of the port connect change status interrupt enable.
    pccs_restore = READ32(PxIE(port_num)) & PORT_CONNECT_CHANGE_STATUS;    

    // Clear the port connect change status interrupt enable.
    MODIFY32(PxIE(port_num), PORT_CONNECT_CHANGE_STATUS, 0); 

    do
    {
        CRIT("-> ahci_port_reset(%u)\n", port_num);

        /* Port Reset (COMRESET) */
        MODIFY32(PxSCTL(port_num), PSCTL_DET_MASK, PSCTL_DET_RESET);
        msleep(5);  /* wait at least 1 ms for COMRESET signal to be sent */
        MODIFY32(PxSCTL(port_num), PSCTL_DET_MASK, 0);

        // Wait for communication re-establishment (up to 10 ms per SATA 2.6).
        // PxSSTS.DET = 0x1 (Device presence detected) or 0x3 (PHY ready). (just check bit 0).
        //status = ahci_wait_complete(PxSSTS(port_num), PSSTS_DET_BIT0_MASK, PSSTS_DET_BIT0_MASK, 50);

        // Wait for PHY Ready instead of Device Presence because sometimes a device will get
        // to Device Presence state but never make it to PHY Ready state.
        status = ahci_wait_complete(PxSSTS(port_num), PSSTS_DET_MASK, PSSTS_DET_PHY_READY, 50);

    } while ((status != STATUS_OK) && (retry_cnt++ < MAX_NUM_PORT_RESET_RETRIES));

    // Check if COMRESET times out and device init was completed previously. 
    if ((status != STATUS_OK) && ata_dev[port_num].bDeviceInitComplete)
    {
        // Clear device init complete flag.
        ata_dev[port_num].bDeviceInitComplete = FALSE;
        // Set device init timed out flag.
        ata_dev[port_num].bDeviceInitTimedOut = TRUE;
        // Clear packet device flag.
        ata_dev[port_num].bPacketDevice = FALSE;

#if !REMOVABLE_MEDIA_DEVICE
        // Disconnect USB.
        usb_hal_disconnect();

        // Reconnect as removable media device with no media.
        usb_hal_connect();
#endif
    }

    // Clear Exchanged bit in PxSERR.
    WRITE32(PxSERR(port_num), PSERR_DIAG_X_BIT);

    if (pccs_restore)
    {
        // Restore the port connect change status interrupt enable.
        MODIFY32(PxIE(port_num), PORT_CONNECT_CHANGE_STATUS, PORT_CONNECT_CHANGE_STATUS); 
    }

    // Set port reset flag so setting will be restored after command processing is started.
    ata_dev[port_num].bPortReset = TRUE;

    return status;
}



/*****************************************************************************
 * Function: ahci_classify_device
 *************************************************************************//**
 * This function reads the device signature to determine 
 * whether a device is ATA or ATAPI.
 *
 * @param[in] port_num SATA port number.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when signature is not ATA or ATAPI.
 *
 ****************************************************************************** 
 */

inline STATUS_T ahci_classify_device(UINT32_T port_num)
{
    UINT32_T sig;
    UINT8_T LBA_high;
    UINT8_T LBA_mid;
    UINT8_T LBA_low;
    UINT8_T sect_cnt;
    STATUS_T status = STATUS_ERROR;
    UINT8_T *d2h_fis = (UINT8_T*)datapath_ram->ahci_mem[port_num].ahci_rfis.D2H_Register_FIS;

    sig = READ32(PxSIG(port_num));

    LBA_high = (sig & 0xFF000000) >> 24;
    LBA_mid  = (sig & 0x00FF0000) >> 16;
    LBA_low  = (sig & 0x0000FF00) >> 8;
    sect_cnt = (sig & 0x000000FF);

    // LBA_low and sect_cnt must be 0x01 for ATA and ATAPI devices.
    if ((LBA_low == 0x01) || (sect_cnt == 0x01))
    {
        if ((LBA_high == 0x00) && (LBA_mid == 0x00))
        {
            /* ATA */
            ata_dev[port_num].bPacketDevice = FALSE;
            status = STATUS_OK;
        }
        else if ((LBA_high == 0xEB) && (LBA_mid == 0x14))
        {
            /* ATAPI */
            ata_dev[port_num].bPacketDevice = TRUE;
            status = STATUS_OK;
        }
    }

    if (status == STATUS_OK)
    {
        // Save device signature.
        ti_memcpy(ata_dev[port_num].bDeviceSignature, d2h_fis, sizeof(ata_dev[port_num].bDeviceSignature));
    }
    else
    {
        CRIT("@Error: ahci_classify_device() failed.\n");
    }

    return status;
}



#define CMD_HEADER_PRDTL_OFFSET      16
#define CMD_HEADER_PREFETCHABLE_BIT  0x00000080  /* not supported by host */
#define CMD_HEADER_WRITE_BIT         0x00000040 
#define CMD_HEADER_ATAPI_BIT         0x00000020 

#define CMD_FIS_LENGTH   5  /* D2H Register FIS is 5 DWORDS */

/*****************************************************************************
 * Function: ahci_create_cmd_header
 *************************************************************************//**
 * This function generates a command header for the specified
 * port and command slot. Prefetch is not supported by the host.
 *
 * @param[in] port_num SATA port number (same as LUN).
 * @param[in] cmd_slot command slot.                       
 * @param[in] prdt_entries number of scatter/gather entries in PRDT.   
 * @param[in] ata_cmd pointer to ATA command structure.                 
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_create_cmd_header(UINT32_T port_num, UINT32_T cmd_slot, UINT32_T prdt_entries, ATA_COMMAND_T *ata_cmd)
{
    AHCI_CMD_HEADER_T* header = (AHCI_CMD_HEADER_T*)&datapath_ram->ahci_mem[port_num].ahci_cmd_list[cmd_slot];

    // Set Command FIS Length (CFL).
    header->dDescInfo = CMD_FIS_LENGTH;

    // Set PRD table length (PRDTL).
    header->dDescInfo |= (prdt_entries << CMD_HEADER_PRDTL_OFFSET);

    // Set Write bit if necessary.
    if (ata_cmd->bIsWriteCmd)
    {
        header->dDescInfo |= CMD_HEADER_WRITE_BIT;
    }

    // Save write command flag so we can set correct sense data later if there's an error.
    ata_dev[port_num].bWriteCmd[cmd_slot] = ata_cmd->bIsWriteCmd;

    // Set ATAPI bit for packet commands.
    if (ata_cmd->fis.command == ATA_CMD_PACKET)
    {
        header->dDescInfo |= CMD_HEADER_ATAPI_BIT;
    }

    // Clear PRDBC field.
    header->dPRDByteCnt = 0;

    // Program command table descriptor address.
    header->dCmdTableBaseAddr = (UINT32_T)&datapath_ram->ahci_mem[port_num].ahci_cmd_table[cmd_slot];
    header->dCmdTableBaseAddrHi = 0;

    return;
}

/*****************************************************************************
 * Function: ahci_get_PRD_byte_count
 *************************************************************************//**
 * This function returns the byte count field from a command header.
 *
 * @param[in] port_num SATA port number.
 * @param[in] cmd_slot command slot.                       
 *
 * @return The byte count.
 *
 ****************************************************************************** 
 */

inline UINT32_T ahci_get_PRD_byte_count(UINT32_T port_num, UINT32_T cmd_slot)
{
    AHCI_CMD_HEADER_T* cmd_header = (AHCI_CMD_HEADER_T*)&datapath_ram->ahci_mem[port_num].ahci_cmd_list[cmd_slot];

    INFO("Cmd slot %u PRD byte_cnt = %u.\n", cmd_slot, cmd_header->dPRDByteCnt);

    // Return byte count (DW[1]).
    return cmd_header->dPRDByteCnt;   
}


/*****************************************************************************
 * Function: ahci_setup_PRDT
 *************************************************************************//**
 * This function populates the Physical Region Descriptor Table for a command.
 *
 * @param[in] port_num SATA port number.
 * @param[in] cmd_slot command slot.                       
 * @param[in] ata_cmd pointer to ATA command structure.                     
 * @param[out] prdt_entry_count number of populated descriptors.
 *
 * @retval STATUS_OK when the full transfer fits the selected data window.
 * @retval STATUS_ERROR when the byte count cannot be represented safely.
 *
 ****************************************************************************** 
 */

inline STATUS_T ahci_setup_PRDT(
    UINT32_T port_num,
    UINT32_T cmd_slot,
    ATA_COMMAND_T *ata_cmd,
    UINT32_T *prdt_entry_count)
{
    volatile UINT32_T *pDW = &datapath_ram->ahci_mem[port_num].ahci_cmd_table[cmd_slot].PRDT[0];
    UINT32_T byte_cnt;
    UINT32_T remaining_byte_cnt;
    UINT32_T maximum_byte_cnt;
    UINT32_T buff_ptr;

    if ((ata_cmd == NULL) || (prdt_entry_count == NULL))
    {
        return STATUS_ERROR;
    }
    *prdt_entry_count = 0U;

    if (ata_cmd->bUseMemoryWrapWindow)
    {
        // Set data pointer to appropriate wrap window memory address.
        if (ata_cmd->bIsWriteCmd)
        {
            //ata_dev[port_num].pData[cmd_slot] = (void*)USB_TO_SATA_WRAP_WINDOW_ADDR;
            buff_ptr = USB_TO_SATA_WRAP_WINDOW_ADDR;
        }
        else
        {
            //ata_dev[port_num].pData[cmd_slot] = (void*)SATA_TO_USB_WRAP_WINDOW_ADDR;
            buff_ptr = SATA_TO_USB_WRAP_WINDOW_ADDR;
        }
    }
    else
    {
        // Don't use wrap window memory.
        //ata_dev[port_num].pData[cmd_slot] = (void*)datapath_ram->normal_data_buffer;
        buff_ptr = (UINT32_T)datapath_ram->normal_data_buffer;
    }

    maximum_byte_cnt = ata_cmd->bUseMemoryWrapWindow ?
        ((UINT32_T)AHCI_MAX_SCAT_GATH *
         (UINT32_T)MWW_VIRTUAL_WINDOW_SIZE) :
        (UINT32_T)sizeof(datapath_ram->normal_data_buffer);

#if DISABLE_WRAP_WINDOW
    //ata_dev[port_num].pData[cmd_slot] = (void*)datapath_ram->normal_data_buffer;
    buff_ptr = (UINT32_T)datapath_ram->normal_data_buffer;
    maximum_byte_cnt = (UINT32_T)sizeof(datapath_ram->normal_data_buffer);
#endif

    if (ata_cmd->dDataByteCnt > maximum_byte_cnt)
    {
        CRIT("@Error: Xfer length 0x%x exceeds data-window capacity 0x%x!\n",
             ata_cmd->dDataByteCnt, maximum_byte_cnt);
        return STATUS_ERROR;
    }

    remaining_byte_cnt = ata_cmd->dDataByteCnt;
    while (remaining_byte_cnt > 0U)
    {
        // Transfer length must be no larger than virtual wrap window size for each scatter/gather entry.
        byte_cnt = MIN(remaining_byte_cnt, MWW_VIRTUAL_WINDOW_SIZE);

        pDW[0] = buff_ptr;//(UINT32_T)ata_dev[port_num].pData[cmd_slot];  /* Data base addr */
        pDW[1] = 0;  /* Data base addr upper 32-bit (for 64-bit addressing) */
        pDW[2] = 0;  /* RSVD */
        pDW[3] = (byte_cnt - 1);  /* Data block length (subtract 1 because this field is 0-based), max length is 4 MB */

        // Calculate the remaining byte count.
        remaining_byte_cnt -= byte_cnt;

        // Increment pointer to next entry in the scatter/gather list.
        pDW += 4;

        // Increment number of PRDT entries.
        (*prdt_entry_count)++;

    }

    return STATUS_OK;
}


/*****************************************************************************
 * Function: ahci_build_cmd
 *************************************************************************//**
 * This function builds the AHCI memory structures required to issue a command.
 *
 * @param[in] port_num SATA port number.
 * @param[in] ata_cmd pointer to ATA command structure.                     
 * @param[out] cmd_slot command slot.                       
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the command slot is out of range.
 * @retval STATUS_ATA_CMD_SLOT_BUSY when the command slot is already in use
 * @retval STATUS_ATA_DEVICE_FAULT when there is a device fault.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_build_cmd(UINT32_T port_num, ATA_COMMAND_T *ata_cmd, UINT32_T cmd_slot)
{
    UINT32_T dev_state;
    UINT32_T prdt_entries = 0;
    STATUS_T status = STATUS_OK;

    DEBUG("-> ahci_build_cmd() - cmd_slot = %u.\n", cmd_slot);

    // Check for device fault.
    if (ata_dev[port_num].bDeviceFault)
    {
        CRIT("@Error: Device fault!\n", cmd_slot);
        return STATUS_ATA_DEVICE_FAULT;
    }

    if (cmd_slot > ata_dev[port_num].bQueueDepth)
    {
        // Error.
        CRIT("@Error: Cmd slot %u is out of range.  Aborting.\n", cmd_slot);
        return STATUS_ERROR;
    }

    // An empty command slot has its respective bit cleared in both the PxCI and PxSACT registers.
    dev_state = READ32(PxSACT(port_num)) | READ32(PxCI(port_num));   

    if (dev_state & (1 << cmd_slot))
    {
        CRIT("@Error: Cmd slot %u busy. SACT = 0x%08x, CI = 0x%08x.\n", cmd_slot, READ32(PxSACT(port_num)), READ32(PxCI(port_num))); 
        status = STATUS_ATA_CMD_SLOT_BUSY;
    }

    if (status == STATUS_OK)
    {
        // Save check condition flag (for ATA PASS-THROUGH cmds).
        ata_dev[port_num].bCheckCondition[cmd_slot] = ata_cmd->bCheckCondition;

        // Copy command FIS into command table.
        ti_memcpy((void*)&datapath_ram->ahci_mem[port_num].ahci_cmd_table[cmd_slot].Command_FIS[0], &ata_cmd->fis, sizeof(REGISTER_FIS_H2D_T));

        // Check if this is a Packet command.
        if (ata_cmd->fis.command == ATA_CMD_PACKET)
        {
            // Copy ATAPI command into command table.
            ti_memcpy((void*)&datapath_ram->ahci_mem[port_num].ahci_cmd_table[cmd_slot].ATAPI_CMD[0], &ata_cmd->atapi_cdb[0], sizeof(ata_cmd->atapi_cdb));           
        }

        if (ata_cmd->dDataByteCnt > 0)
        {
            // Setup PRDT.
            status = ahci_setup_PRDT(
                port_num, cmd_slot, ata_cmd, &prdt_entries);
        }

        if (status == STATUS_OK)
        {
            // Create command header.
            ahci_create_cmd_header(
                port_num, cmd_slot, prdt_entries, ata_cmd);
        }
    }

    return status;
}


/*****************************************************************************
 * Function: ahci_issue_cmd
 *************************************************************************//**
 * This function issues an AHCI command after it has been built
 * using ahci_build_cmd().
 *
 * @param[in] port_num SATA port number (same as LUN).
 * @param[in] cmd_slot command slot.                       
 * @param[in] is_queued_cmd flag indicating whether a queued command will be issued.   
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when PxCMD.ST bit is not set.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_issue_cmd(UINT32_T port_num, UINT32_T cmd_slot, BOOLEAN_T is_queued_cmd)
{
    STATUS_T status = STATUS_OK;

    DEBUG("-> ahci_issue_cmd() - P%u, cmd_slot = %u, queued_cmd = %u.\n", port_num, cmd_slot, is_queued_cmd);

    // Verify P#CMD.ST is set.
    if (READ32(PxCMD(port_num)) & PCMD_ST_BIT)
    {
        /* If NCQ is used, write cmd_slot to the PxSACT.DS register to 
        indicate a command with that TAG = cmd_slot is outstanding. */
        if (is_queued_cmd)
        {
            WRITE32(PxSACT(port_num), (1 << cmd_slot));

            // Set SActive flag.
            ata_dev[port_num].dSActive |= (1 << cmd_slot);
        }
        else
        {
            // Save current command slot to be returned to mass storage module when D2H register FIS interrupt occurs.
            ata_dev[port_num].bCurrentCmdSlot = cmd_slot;
        }

        // Set bit for appropriate cmd_slot in P#CI register to indicate command is ready to send to the device.
        WRITE32(PxCI(port_num), (1 << cmd_slot));
    }
    else
    {
        // Cannot set CI if P#CMD.ST is not 1.
        CRIT("@Error: Failed to issue command slot %u. P%uCMD.ST not set.\n", cmd_slot, port_num);
        status = STATUS_ERROR;
    }

    return status;
}

/*****************************************************************************
 * Function: ahci_register_ata_callbacks
 *************************************************************************//**
 * This function is used by mass storage layers to register callback functions
 * for ATA command completions and errors.  The handlers must be low latency to
 * avoid causing missed interrupts.
 *
 * @param[in] ata_cmd_callback pointer to function to handle D2H Register FIS or Set Device Bits FIS interrupts.
 * @param[in] ata_queued_cmd_callback pointer to function to handle DMA Setup FIS interrupts.                      
 * @param[in] ata_error_callback pointer to function for handling fatal ATA error interrupts.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_register_ata_callbacks(void (*ata_cmd_callback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk),
                                 void (*ata_queued_cmd_callback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk),
                                 void (*ata_error_callback)(ATA_CMD_CALLBACK_T *pAtaCmdCbk))
{
    UINT32_T port_num;

    INFO("-> ahci_register_ata_callbacks() - 0x%08x, 0x%08x, 0x%08x.\n", 
         (UINT32_T)ata_cmd_callback, (UINT32_T)ata_error_callback, (UINT32_T)ata_queued_cmd_callback);

    for (port_num = 0; port_num < NUM_AHCI_PORTS; port_num++)
    {
        // Set callbacks.
        ata_dev[port_num].pAtaCmdCallback = ata_cmd_callback; /* Called upon D2H Register FIS or Set Device Bits FIS interrupt */
        ata_dev[port_num].pAtaQueuedCmdCallback = ata_queued_cmd_callback; /* Called upon DMA Setup FIS interrupt */
        ata_dev[port_num].pAtaErrorCallback = ata_error_callback; /* Called upon fatal error interrupt */
    }

    return;
}


/*****************************************************************************
 * Function: ahci_register_port_init_complete_callback
 *************************************************************************//**
 * This function is used by mass storage layers to register callback functions
 * for port initialization completion.
 *
 * @param[in] port_num SATA port number.
 * @param[in] sata_port_init_callback pointer to function to handle SATA port initialization completion.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_register_port_init_complete_callback(UINT32_T port_num, void (*sata_port_init_callback)(UINT32_T port_num))
{
    INFO("-> ahci_register_port_init_complete_callback() - 0x%08x.\n", (UINT32_T)sata_port_init_callback);

    // Set callbacks.
    ata_dev[port_num].pSATAPortInitCallback = sata_port_init_callback; 

    return;
}



#define UDMA_XFER_TYPE  0x40
#define MDMA_XFER_TYPE  0x20
#define PIO_XFER_TYPE   0x08

/*****************************************************************************
 * Function: ahci_get_xfer_mode
 *************************************************************************//**
 * This function returns the value of the highest transfer 
 * transfer mode supported (used for SET FEATURES command).
 *
 * @param[in] port_num SATA port number.
 * @param[in] dma flag indicating whether to return DMA or PIO xfer mode.
 *
 * @return The transfer mode value.
 *
 ****************************************************************************** 
 */

inline UINT8_T ahci_get_xfer_mode(UINT32_T port_num, BOOLEAN_T dma)
{
    UINT32_T features = 0;
    UINT32_T bit_offset;

    if (dma)
    {
        if (ata_dev[port_num].wUDMA_ModesSupported)
        {
            // Find highest supported UDMA mode.
            bit_offset = 6;  /* 6 = Ultra DMA mode 6 */
            while ((ata_dev[port_num].wUDMA_ModesSupported & (0x1 << bit_offset)) == 0)
            {
                bit_offset--;
            }

            features = UDMA_XFER_TYPE + bit_offset;
        }
        else if (ata_dev[port_num].wMDMA_ModesSupported)
        {
            // Find highest supported MDMA mode.
            bit_offset = 2;
            while ((ata_dev[port_num].wMDMA_ModesSupported & (0x1 << bit_offset)) == 0)
            {
                bit_offset--;
            }

            features = MDMA_XFER_TYPE + bit_offset;
        }
    }
    else
    {
        // Find highest supported PIO mode. 
        // Bit 0 indicates support for PIO mode 3.
        // Bit 1 indicates support for PIO mode 4.
        if (ata_dev[port_num].wPIO_ModesSupported & 0x02)
        {
            // PIO mode 4.
            features = PIO_XFER_TYPE + 0x4;
        }
        else if (ata_dev[port_num].wPIO_ModesSupported & 0x01)
        {
            // PIO mode 3.
            features = PIO_XFER_TYPE + 0x3;
        }
        else
        {
            // PIO default mode.
            features = 0;  
        }
    }

    DEBUG("  SATA %s xfer mode: 0x%02x.\n", (dma) ? "DMA" : "PIO", features);

    return(UINT8_T)features;
}


#define SET_FEATURES_XFER_MODE  0x03

/*****************************************************************************
 * Function: ahci_set_features_xfer_mode
 *************************************************************************//**
 * This function sends a SET FEATURES command to set the device transfer mode.
 *
 * @param[in] port_num SATA port number.
 * @param[in] dma flag indicating whether to set DMA or PIO xfer mode.
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when the command times out.
 * @retval STATUS_ERROR when the device rejects the command.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_set_features_xfer_mode(UINT32_T port_num, BOOLEAN_T dma)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;

    DEBUG("-> ahci_set_features_xfer_mode() on port %u, DMA = %u.\n", port_num, dma);

    // Clear ATA command info.
    ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

    // Populate command FIS.
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
    ata_cmd.fis.command = ATA_CMD_SET_FEATURES;
    ata_cmd.fis.features = SET_FEATURES_XFER_MODE;
    ata_cmd.fis.sector_cnt = ahci_get_xfer_mode(port_num, dma);

    status = ahci_build_cmd(port_num, &ata_cmd, 0);

    if (status == STATUS_OK)
    {
        status = ahci_issue_cmd(port_num, 0, FALSE);
    }

    if (status == STATUS_OK)
    {
        // Wait for command completion.
        status = ahci_wait_complete(PxCI(port_num), 0x1, 0x0, 2000);
        if ((status == STATUS_OK) &&
            ((READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK) != 0U))
        {
            status = STATUS_ERROR;
        }
    }

    return status;
}

#define SET_FEATURES_ENABLE_FEATURE  0x10
#define SET_FEATURES_DISABLE_FEATURE 0x90

#define SET_FEATURES_DMA_SETUP_FIS_AUTO_ACTIVATE  0x02

/*****************************************************************************
 * Function: ahci_set_features_dma_auto_activate
 *************************************************************************//**
 * This function sends a SET FEATURES command to enable or disable
 * DMA Setup FIS Auto-Activate optimization feature.
 *
 * @param[in] port_num SATA port number.
 * @param[in] enable flag indicating whether to enable or disable DMA Setup FIS Auto-Activate feature.
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when the command times out.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_set_features_dma_auto_activate(UINT32_T port_num, BOOLEAN_T enable)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;

    DEBUG("-> ahci_set_features_dma_auto_activate() on port %u.\n", port_num);

    // Clear ATA command info.
    ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

    // Populate command FIS.
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
    ata_cmd.fis.command = ATA_CMD_SET_FEATURES;
    ata_cmd.fis.features = (enable) ? SET_FEATURES_ENABLE_FEATURE : SET_FEATURES_DISABLE_FEATURE;
    ata_cmd.fis.sector_cnt = SET_FEATURES_DMA_SETUP_FIS_AUTO_ACTIVATE;

    status = ahci_build_cmd(port_num, &ata_cmd, 0);

    if (status == STATUS_OK)
    {
        status = ahci_issue_cmd(port_num, 0, FALSE);
    }

    if (status == STATUS_OK)
    {
        // Wait for command completion.
        status = ahci_wait_complete(PxCI(port_num), 0x1, 0x0, 2000);
        if ((status == STATUS_OK) &&
            ((READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK) != 0U))
        {
            status = STATUS_ERROR;
        }
    }

    return status;
}


#if DEBUG_LEVEL >= 1

void ahci_dump_dev_info(UINT32_T port_num)
{
    UINT32_T specv;
    UINT16_T buff[21];

    CRIT("\n");
    CRIT("================================================\n");
    CRIT("             IDENTIFY DEVICE INFO\n");
    CRIT("================================================\n");
    CRIT("\n");

    ti_memset(buff, 0, sizeof(buff));
    ti_memcpy(buff, ata_dev[port_num].wModelNum, sizeof(ata_dev[port_num].wModelNum));
    CRIT("  Model:  %s\n", (char*)buff);

    ti_memset(buff, 0, sizeof(buff));
    ti_memcpy(buff, ata_dev[port_num].wFirmwareRev, sizeof(ata_dev[port_num].wFirmwareRev));
    CRIT("  FW Rev: %s\n", (char*)buff);

    ti_memset(buff, 0, sizeof(buff));
    ti_memcpy(buff, ata_dev[port_num].wSerialNum, sizeof(ata_dev[port_num].wSerialNum));
    CRIT("  Serial: %s\n", (char*)buff);
    if (ata_dev[port_num].wMediaRotationRate == 0x0001)
    {
//        CRIT("  Disk Type: Solid State\n");
        CRIT("  TRIM Support: %s %s%s\n", ata_dev[port_num].bTRIMSupport ? "Yes" : "No", 
             ata_dev[port_num].bDRATSupport ? "[DRAT]" : "", ata_dev[port_num].bRZATSupport ? "[RZAT]" : "");
    }
    CRIT("\n");

    for (specv = 8; specv >= 4; specv--)
    {
        if (ata_dev[port_num].bATA_MajorVersionNum & (0x1 << specv))
            break;
    }

    CRIT("  Spec Compliance: ATA%s-%u\n", ata_dev[port_num].bPacketDevice ? "PI" : "", specv);
    CRIT("  Removable Media: %s\n", ata_dev[port_num].bRemovableMediaDevice ? "Yes" : "No");  
//    CRIT("  Multi Sector Num = %u\n", ata_dev[port_num].bMultiSectorNum);   /* number of sectors transferred per interrupt */
    if (ata_dev[port_num].wMediaRotationRate > 0x0400)
    {
        CRIT("  Rotational Speed = %u RPM\n", ata_dev[port_num].wMediaRotationRate);
    }
//    CRIT("  MDMA Modes = 0x%04x\n", ata_dev[port_num].wMDMA_ModesSupported);
    CRIT("  UDMA Modes = 0x%04x\n", ata_dev[port_num].wUDMA_ModesSupported);
    CRIT("  PIO Modes = 0x%04x\n", ata_dev[port_num].wPIO_ModesSupported);
    CRIT("\n");
    CRIT("  LBA48: %s\n", ata_dev[port_num].bLBA48 ? "Yes" : "No");     /* indicates if we are dealing w/ 28-bit or 48-bit addressing */    
    CRIT("  Max LBA = 0x%08x %08x\n", (UINT32_T)(ata_dev[port_num].ddTrueMaxLBA >> 32), (UINT32_T)(ata_dev[port_num].ddTrueMaxLBA & 0xFFFFFFFF));   /* derived from IDENTIFY DEVICE words 60-61 (28-bit) or 100-103 (48-bit) */
    CRIT("  Write FUA: %s\n", ata_dev[port_num].bFUA  ? "Yes" : "No");

    if (ata_dev[port_num].bWorldWideNameValid)
    {
        CRIT("  World Wide Name = 0x%08x %08x\n", 
             (UINT32_T)(ata_dev[port_num].wWorldWideName[0] << 16) | ata_dev[port_num].wWorldWideName[1], 
             (UINT32_T)(ata_dev[port_num].wWorldWideName[2] << 16) | ata_dev[port_num].wWorldWideName[3]);
    }
    else
    {
        CRIT("  World Wide Name: N/A\n");
    }

    CRIT("\n");
    CRIT("  SATA Speed: Gen%u\n", ata_dev[port_num].bSATA_Gen);
    CRIT("  NCQ Support: %s\n", ata_dev[port_num].bNCQ ? "Yes" : "No");
    CRIT("  Queue Depth = %u\n", ata_dev[port_num].bActualQueueDepth);  /* 0 = depth of 1 */
//    CRIT("  DMA Auto-Activate Optimization: %s\n", ata_dev[port_num].bDMA_SetupAutoActivateSupport ? "Yes" : "No"); 
//    CRIT("  SW Settings Preservation: %s\n", ata_dev[port_num].bSoftwareSettingsPreservation ? "Yes" : "No"); 
//    CRIT("  Host-initiated LPM Support: %s\n", ata_dev[port_num].bLPM ? "Yes" : "No"); 
    CRIT("\n");
    CRIT("  Logical Sector Size = %u bytes\n", ata_dev[port_num].dTrueSectorSize);  /* bytes */
//    CRIT("  Physical Sector Exp = %u\n", ata_dev[port_num].bPhysicalSectorExp);  /* Word 106 bits 3:0 */
    CRIT("  Physical Sector Size = %u bytes\n", ata_dev[port_num].dPhysicalSectorSize);  /* bytes */
    CRIT("  Logical Sector Offset = %u\n", ata_dev[port_num].wTrueLogicalSectorOffset);  /* location of logical sector zero */


    CRIT("\n");
    CRIT("================================================\n\n");

    return;
}

#endif /* DEBUG_LEVEL >= 1 */



/*****************************************************************************
 * Function: ahci_save_device_info
 *************************************************************************//**
 * This function saves the required information from the IDENTIFY
 * (PACKET) DEVICE command.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_save_device_info(UINT32_T port_num)
{
    UINT32_T i;
    UINT32_T logical_sector_words;
    UINT16_T *id_info;

    // Save IDENTIFY DEVICE information.
    ti_memcpy(ata_dev[port_num].wIdentifyDeviceInfo, (void*)datapath_ram->normal_data_buffer, 512);

    // Get pointer to ata_dev info.
    id_info = (UINT16_T*)datapath_ram->normal_data_buffer;

    /* Clear fields that are populated only when matching IDENTIFY validity
     * bits are set. A cartridge swap or PUIS re-IDENTIFY must not inherit
     * capabilities, geometry, or identity flags from the previous disk. */
    ata_dev[port_num].bDMADIR = FALSE;
    ata_dev[port_num].wPIO_ModesSupported = 0U;
    ata_dev[port_num].wUDMA_ModesSupported = 0U;
    ata_dev[port_num].bSATA_Gen = 0U;
    ata_dev[port_num].bATA_MajorVersionNum = 0U;
    ata_dev[port_num].bWorldWideNameValid = FALSE;
    ti_memset(ata_dev[port_num].wWorldWideName, 0U,
              sizeof(ata_dev[port_num].wWorldWideName));
    ata_dev[port_num].bPhysicalSectorExp = 0U;
    ata_dev[port_num].wLogicalSectorOffset = 0U;
    ata_dev[port_num].bLargeSectorEmulation = FALSE;
    ata_dev[port_num].wLowestAlignedLBA = 0U;
    ata_dev[port_num].wDataSetMgmtMaxBlocks = 0U;

    INFO("Identify Device Info:  (addr: 0x%08x)\n", (UINT32_T)id_info);
    for (i = 0; i < 256; i += 8)
    {
        INFO("%04x: %04x %04x %04x %04x %04x %04x %04x %04x\n", i*2, id_info[i], id_info[i+1], id_info[i+2], 
             id_info[i+3], id_info[i+4], id_info[i+5], id_info[i+6], id_info[i+7]);   
    }

    // Check if removeable media device. (bit 7)
    ata_dev[port_num].bRemovableMediaDevice = (id_info[0] & 0x0080) ? TRUE : FALSE;

    // Store serial, firmware rev, and model num info. 
    // (Byte swap values for proper ASCII string format)  
    for (i = 0; i < (sizeof(ata_dev[port_num].wSerialNum) / 2); i++)
    {
        ata_dev[port_num].wSerialNum[i] = BSWAP_16(id_info[10 + i]);
    }

    for (i = 0; i < (sizeof(ata_dev[port_num].wFirmwareRev) / 2); i++)
    {
        ata_dev[port_num].wFirmwareRev[i] = BSWAP_16(id_info[23 + i]);
    }

    for (i = 0; i < (sizeof(ata_dev[port_num].wModelNum) / 2); i++)
    {
        ata_dev[port_num].wModelNum[i] = BSWAP_16(id_info[27 + i]);
    }

//    // Save max num of logical sectors for R/W multiple cmds.
//    ata_dev[port_num].bMultiSectorNum = id_info[47] & 0x00FF;   

    // Save the max LBA for LBA28.
    ata_dev[port_num].ddMaxLBA = *((UINT32_T *)&id_info[60]);

    if (ata_dev[port_num].bPacketDevice)
    {
        // Check if DMADIR is required (bit 15).
        ata_dev[port_num].bDMADIR = (id_info[62] & 0x8000) ? TRUE : FALSE;
    }

    // Save the multiword DMA modes supported.
    ata_dev[port_num].wMDMA_ModesSupported = id_info[63];

    // Check if Words (70:64) are valid.
    if (id_info[53] & 0x02)
    {
        // Save the PIO modes supported.
        ata_dev[port_num].wPIO_ModesSupported = id_info[64];
    }

    // Check DRAT SUPPORTED bit. (Bit 14 of word 69)
    ata_dev[port_num].bDRATSupport = (id_info[69] & 0x4000) ? TRUE : FALSE;
    // Check RZAT SUPPORTED bit. (Bit 5 of word 69)
    ata_dev[port_num].bRZATSupport = (id_info[69] & 0x0020) ? TRUE : FALSE;

    // Save the max queue depth (save the min value supported by both host and device).
    ata_dev[port_num].bActualQueueDepth = (id_info[75] & 0x001F);
    ata_dev[port_num].bQueueDepth = MIN((id_info[75] & 0x001F), (AHCI_NCQ_DEPTH - 1));    // Word 75 is (max queue depth - 1)

    // Check for LPM request support.
    ata_dev[port_num].bLPM = (id_info[76] & 0x0200) ? TRUE : FALSE;  

    // Check for NCQ support.
    ata_dev[port_num].bNCQ = (id_info[76] & 0x0100) ? TRUE : FALSE;    

    // Check for signaling rate support.
    for (i = 3; i > 0; i--)
    {
        if (id_info[76] & (1 << i))
        {
            ata_dev[port_num].bSATA_Gen = i;
            break;
        }
    }

    // Check for DMA Setup Auto-Activation support.
    ata_dev[port_num].bDMA_SetupAutoActivateSupport = (id_info[78] & 0x0004) ? TRUE : FALSE;

    // Check for Software Settings Preservation enabled (enabled by default if supported).
    ata_dev[port_num].bSoftwareSettingsPreservation = (id_info[79] & 0x0040) ? TRUE : FALSE;

    // Get ATA major version number.
    if (id_info[80] != 0xFFFF && id_info[80] != 0x0000)
    {
        ata_dev[port_num].bATA_MajorVersionNum = id_info[80];  /* bits may indicate all ATA versions that device is compliant to */
    }

    // Check if LBA48 supported.
    ata_dev[port_num].bLBA48 = (id_info[83] & 0x0400) ? TRUE : FALSE;

    // Check if Write FUA supported.
    ata_dev[port_num].bFUA = (id_info[84] & 0x0040) ? TRUE : FALSE;

    // Word 88 (UDMA modes) is valid if bit 2 of Word 53 is set.
    if (id_info[53] & 0x0004)
    {
        // Save Ultra DMA modes supported.
        ata_dev[port_num].wUDMA_ModesSupported = id_info[88];
    }

    // Save max LBA if LBA48 is supported.
    if (ata_dev[port_num].bLBA48 && (ata_dev[port_num].ddMaxLBA == 0x0FFFFFFF))
    {
        ti_memcpy(&ata_dev[port_num].ddMaxLBA, &id_info[100], sizeof(ata_dev[port_num].ddMaxLBA));
    }

    // World wide name field is valid if bit 8 of Word 84 is set.
    if (id_info[84] & 0x0100)
    {
        ata_dev[port_num].bWorldWideNameValid = TRUE;
        ti_memcpy(&ata_dev[port_num].wWorldWideName[0], &id_info[108], sizeof(ata_dev[port_num].wWorldWideName));   

        if ((id_info[108] & 0xF000) != 0x5000)
        {
            // Some devices have bit 8 of Word 84 set but do not have valid World Wide Name info.
            CRIT("@Error: Naming authority for WWN must be IEEE!\n");
            ata_dev[port_num].bWorldWideNameValid = FALSE;
        }
    }

    // Set default num of logical sectors per physical sector.
    ata_dev[port_num].dLogicalSectorsPerPhysicalSector = 1;

    // Word 106 is valid if bit 14 is set and bit 15 is cleared.
    if ((id_info[106] & 0x4000) && ((id_info[106] & 0x8000) != 0x8000))
    {
        // Check if logical sector size > 256 words (512 bytes).  (bit 12 set)
        if (id_info[106] & 0x1000)
        {
            /* ATA words 117 and 118 hold the low and high halves,
             * respectively, of the logical-sector size measured in words. */
            logical_sector_words =
                ((UINT32_T)id_info[118] << 16) |
                (UINT32_T)id_info[117];
            if ((logical_sector_words == 0U) ||
                (logical_sector_words > 0x7FFFFFFFU))
            {
                /* Fail admission closed when doubling the word count would
                 * wrap the byte-size field or yield an empty sector. */
                ata_dev[port_num].dSectorSize = 0U;
            }
            else
            {
                ata_dev[port_num].dSectorSize =
                    logical_sector_words * 2U;
            }
        }
        else
        {
            ata_dev[port_num].dSectorSize = DEFAULT_ATA_SECTOR_SIZE;
        }

        // Is the physical sector size a multiple of the logical. (bit 13 set)
        if (id_info[106] & 0x2000)
        {
            ata_dev[port_num].bPhysicalSectorExp = id_info[106] & 0x000F;

            // Determine number of logical sectors per physical sector.
            for (i = 0; i < ata_dev[port_num].bPhysicalSectorExp; i++)
            {
                ata_dev[port_num].dLogicalSectorsPerPhysicalSector *= 2;
            }

            ata_dev[port_num].dPhysicalSectorSize = ata_dev[port_num].dSectorSize * ata_dev[port_num].dLogicalSectorsPerPhysicalSector;

            // Word 209 is valid if bit 14 is set and bit 15 is cleared.
            if ((id_info[209] & 0x4000) && ((id_info[209] & 0x8000) != 0x8000))
            {
                ata_dev[port_num].wLogicalSectorOffset = id_info[209] & 0x3FFF;
            }
        }
        else
        {
            ata_dev[port_num].dPhysicalSectorSize = ata_dev[port_num].dSectorSize;
        }
    }
    else
    {
        ata_dev[port_num].dSectorSize = (ata_dev[port_num].bPacketDevice) ? DEFAULT_ATAPI_SECTOR_SIZE : DEFAULT_ATA_SECTOR_SIZE;
        ata_dev[port_num].dPhysicalSectorSize = ata_dev[port_num].dSectorSize;
    }

    // Save the true logical sector size, Max LBA, and logical sector offset (in case we are emulating 4KB logical sectors).
    ata_dev[port_num].dTrueSectorSize = ata_dev[port_num].dSectorSize;
    ata_dev[port_num].ddTrueMaxLBA = ata_dev[port_num].ddMaxLBA;
    ata_dev[port_num].wTrueLogicalSectorOffset = ata_dev[port_num].wLogicalSectorOffset;

#if ENABLE_LARGE_SECTOR_EMULATION
    // Check if the drive is > 2.2TB or if it has 4KB physical sectors.
//    if (!ata_dev[port_num].bPacketDevice && 
//        ((ata_dev[port_num].dLogicalSectorsPerPhysicalSector >= 8) || ((ata_dev[port_num].ddMaxLBA - 1) >= 0xFFFFFFFF)) &&
//        (ata_dev[port_num].dSectorSize == DEFAULT_ATA_SECTOR_SIZE))

    // Check if the drive is > 2.2TB.
    if (!ata_dev[port_num].bPacketDevice && 
        ((ata_dev[port_num].ddMaxLBA - 1) >= 0xFFFFFFFF) &&
        (ata_dev[port_num].dSectorSize == DEFAULT_ATA_SECTOR_SIZE))
    {
        /* Emulate 4KB logical sectors so that drives larger than 2.2TB can be used under WinXP which
         * does not support 16-byte SCSI commands.  This scheme fails for drives larger than 17.6TB. 
         * Logical sector alignment is also handled by the firmware. */

        if ((ata_dev[port_num].ddMaxLBA >> 3) <= 0xFFFFFFFF)
        {
            ata_dev[port_num].bLargeSectorEmulation = TRUE;

            /* Lowest Aligned LBA */
            ata_dev[port_num].wLowestAlignedLBA = ata_dev[port_num].dLogicalSectorsPerPhysicalSector - ata_dev[port_num].wLogicalSectorOffset;

            /* Perform a psuedo-modulus to determine Lowest Aligned LBA (according to SAT-2) */
            if (ata_dev[port_num].wLowestAlignedLBA == ata_dev[port_num].dLogicalSectorsPerPhysicalSector)
            {
                ata_dev[port_num].wLowestAlignedLBA = 0;
            }

            INFO("Lowest Aligned LBA = %u.\n", ata_dev[port_num].wLowestAlignedLBA);

            /* Subtract lowest aligned LBA from max LBA and Divide by 8 */
            ata_dev[port_num].ddMaxLBA = (ata_dev[port_num].ddMaxLBA - ata_dev[port_num].wLowestAlignedLBA) >> 3; 

            /* Set new emulated values */
            ata_dev[port_num].dSectorSize = (DEFAULT_ATA_SECTOR_SIZE * 8);  /* 4KB */
            ata_dev[port_num].bPhysicalSectorExp = 0;
            ata_dev[port_num].dLogicalSectorsPerPhysicalSector = 1;
            ata_dev[port_num].wLogicalSectorOffset = 0;
        }
    }

#endif

    // Save TRIM support.
    ata_dev[port_num].bTRIMSupport = id_info[169] & 0x1;

    if (ata_dev[port_num].bTRIMSupport)
    {
        // A value of 0 indicates that the maximum number of 512-byte blocks of LBA Range Entries is not specified.  
        ata_dev[port_num].wDataSetMgmtMaxBlocks =
            ata_dev[port_num].wIdentifyDeviceInfo[105] ?
            ata_dev[port_num].wIdentifyDeviceInfo[105] : 0xFFFFU;
    }

    // Save rotational speed.
    ata_dev[port_num].wMediaRotationRate = id_info[217];

    return;
}

/*****************************************************************************
 * Function: ahci_standby_immediate
 *************************************************************************//**
 * This function sends a STANDBY IMMEDIATE command.  The SATA drive will
 * be spun down. The device is still capable of responding to commands 
 * but responses may take longer (up to 30 sec).
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when the command times out.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_standby_immediate(UINT32_T port_num)
{
    STATUS_T status = STATUS_OK;

#if ENABLE_SATA_STANDBY_POWER_MODE

    UINT32_T command_slot = 0;
    UINT32_T dev_state;
    ATA_COMMAND_T ata_cmd;

    if (!ata_dev[port_num].bDeviceInitComplete)
    {
        return status;
    }

    CRIT("-> ahci_standby_immediate() - port %u.\n", port_num);

    // Clear ATA command info.
    ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

    // Populate command FIS.
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
    ata_cmd.fis.command = ATA_CMD_STANDBY_IMMEDIATE;

    // Find empty command slot.
    // An empty command slot has its respective bit cleared in both the PxCI and PxSACT registers. 
    dev_state = READ32(PxSACT(port_num)) | READ32(PxCI(port_num));

    // Search for the first empty slot.
    while (dev_state & (1 << command_slot))
    {
        command_slot++;

        // Give up after checking all possible slots.
        if (command_slot > ata_dev[port_num].bQueueDepth)
        {
            // Error.
            CRIT("@Error: All command slots full! P%uSACT|CI = 0x%x.  Aborting.\n", port_num, dev_state);
            status = STATUS_ERROR;
            break;
        }
    }

    if (status == STATUS_OK)
    {
        status = ahci_build_cmd(port_num, &ata_cmd, command_slot);

        if (status == STATUS_OK)
        {
            // Disable D2H Register port interrupt since we do not want an ATA cmd callback
            // for this command which will confuse mass storage layers.
            MODIFY32(PxIE(port_num), D2H_REGISTER_FIS_INTR, 0);

            status = ahci_issue_cmd(port_num, command_slot, FALSE);
        }

        if (status == STATUS_OK)
        {
            // Wait for command completion.
            status = ahci_wait_complete(PxCI(0), 0x1, 0x0, 1000);

            // Clear the D2H interrupt status.
            WRITE32(PxIS(port_num), D2H_REGISTER_FIS_INTR);
        }

        // Re-enable D2H Register port interrupt.
        MODIFY32(PxIE(port_num), D2H_REGISTER_FIS_INTR, D2H_REGISTER_FIS_INTR);
    }

#endif
    return status;
}


/*****************************************************************************
 * Function: ahci_identify_device
 *************************************************************************//**
 * This function sends an IDENTIFY (PACKET) DEVICE command and waits for 
 * command completion before returning.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when the command times out.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_identify_device(UINT32_T port_num)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;

    DEBUG("-> ahci_identify_device() on port %u.\n", port_num);

    if (ata_dev[port_num].bDeviceInitComplete)
    {
        // Disable PIO Setup port interrupt.
        MODIFY32(PxIE(port_num), PIO_SETUP_FIS_INTR, 0);
    }

    // Clear ATA command info.
    ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

    // Populate command FIS.
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
    ata_cmd.fis.command = (ata_dev[port_num].bPacketDevice) ? ATA_CMD_IDENTIFY_PACKET_DEVICE : ATA_CMD_IDENTIFY_DEVICE;

    ata_cmd.dDataByteCnt = 0x200;  /* Identify Device returns 512-bytes of data */

    /* Never allow a rejected IDENTIFY to reuse device data left by the
     * previous cartridge in the shared command buffer. */
    ti_memset((void *)datapath_ram->normal_data_buffer, 0U, 0x200U);

    status = ahci_build_cmd(port_num, &ata_cmd, 0);

    if (status == STATUS_OK)
    {
        status = ahci_issue_cmd(port_num, 0, FALSE);
    }

    if (status == STATUS_OK)
    {
        // Wait for command completion. (Increased timeout to 7 sec for InnoDisk EverGreen SSD)
        status = ahci_wait_complete(PxCI(port_num), 0x1, 0x0, 7000);
        if ((status == STATUS_OK) &&
            ((READ32(PxTFD(port_num)) & PTFD_STS_FAILURE_MASK) != 0U))
        {
            status = STATUS_ERROR;
        }
    }

    if (status == STATUS_OK)
    {
        // Clear the PIO Setup FIS interrupt status.
        WRITE32(PxIS(port_num), PIO_SETUP_FIS_INTR);

        // Store the ATA device info.
        ahci_save_device_info(port_num);
    }
    else
    {
        /* A failed IDENTIFY is a valid empty-media result for an RDX bay. */
#if REMOVABLE_MEDIA_DEVICE
        ata_dev[port_num].bDeviceInitComplete = FALSE;
        ata_dev[port_num].bDeviceInitTimedOut = TRUE;
#else
        // Drive must be hosed.  Just reset the system.
        system_reset();  // BQ - This causes issues with running SATA compliance tests.
#endif
    }

    if (ata_dev[port_num].bDeviceInitComplete)
    {
        // Re-enable PIO Setup port interrupt
        MODIFY32(PxIE(port_num), PIO_SETUP_FIS_INTR, PIO_SETUP_FIS_INTR);
    }

    return status;
}

/*****************************************************************************
 * Function: ahci_get_TFD_info
 *************************************************************************//**
 * This function reads the Task File Data register and returns the status
 * and error register data.
 *
 * @param[in] port_num SATA port number.
 * @param[in] status pointer to memory location to store status info.
 * @param[in] error pointer to memory location to store error info.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_get_TFD_info(UINT32_T port_num, UINT8_T *status, UINT8_T *error)
{
    UINT32_T tfd_reg = READ32(PxTFD(port_num));

    INFO("-> ahci_get_TFD_info() - P%uTFD = 0x%x.\n", port_num, tfd_reg);

    if (status) *status = (UINT8_T)(tfd_reg & 0xFF);
    if (error)  *error  = (UINT8_T)((tfd_reg & 0xFF00) >> 8);

    return;
}


#define ATA_ICRC_ERROR_BIT  0x80
#define ATA_UNC_WP_ERROR_BIT   0x40  /* for read or write commands */
#define ATA_MC_ERROR_BIT    0x20  /* obsolete in ATA-8 */
#define ATA_IDNF_ERROR_BIT  0x10
#define ATA_MCR_ERROR_BIT   0x08  /* obsolete in ATA-8 */
#define ATA_ABRT_ERROR_BIT  0x04
#define ATA_NM_ERROR_BIT    0x02

/*****************************************************************************
 * Function: ahci_translate_error_to_sense_data
 *************************************************************************//**
 * This function translates ATA device errors (read from TFD error register)
 * to SCSI sense data according to SAT-2 specification clause 11.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_translate_error_to_sense_data(UINT32_T port_num)
{
    UINT8_T status;
    UINT8_T error;

    ahci_get_TFD_info(port_num, &status, &error);

    if (ata_dev[port_num].bDeviceFault)
    {
        scsi_set_sense_data(HARDWARE_ERROR, INTERNAL_TARGET_FAILURE, NO_ASCQ);       
    }
    else if (status & ATA_ERROR_STATUS_BIT)
    {
        if (error & ATA_ICRC_ERROR_BIT)
        {
            scsi_set_sense_data(ABORTED_COMMAND, IU_CRC_ERROR_DETECTED, ASCQ_IU_CRC_ERROR_DETECTED);
        }
        else if (error & ATA_UNC_WP_ERROR_BIT)
        {
            if (ata_dev[port_num].bWriteCmd[ata_dev[port_num].bCurrentCmdSlot])
            {
                scsi_set_sense_data(DATA_PROTECT, WRITE_PROTECTED, NO_ASCQ);
            }
            else
            {
                scsi_set_sense_data(MEDIUM_ERROR, UNCORRECTABLE_READ_ERROR, NO_ASCQ);
            }
        }
        else if (error & ATA_MC_ERROR_BIT)
        {
            scsi_set_sense_data(UNIT_ATTENTION, MEDIUM_MAY_HAVE_CHANGED, NO_ASCQ);
        }
        else if (error & ATA_IDNF_ERROR_BIT)
        {
            scsi_set_sense_data(ILLEGAL_REQUEST, LBA_OUT_OF_RANGE, NO_ASCQ);
        }
        else if (error & ATA_MCR_ERROR_BIT)
        {
            scsi_set_sense_data(UNIT_ATTENTION, OPERATOR_MEDIUM_REMOVAL_REQUEST, ASCQ_OPERATOR_MEDIUM_REMOVAL_REQUEST);
        }
        else if (error & ATA_NM_ERROR_BIT)
        {
            scsi_set_sense_data(NOT_READY, MEDIUM_NOT_PRESENT, NO_ASCQ);
        }
        else if (error & ATA_ABRT_ERROR_BIT)  /* must be checked last because ABRT bit is ignored if any other bits are set */
        {
            scsi_set_sense_data(ABORTED_COMMAND, NO_ADDITIONAL_SENSE_INFO, NO_ASCQ);                
        }
    }

    return;
}


/*****************************************************************************
 * Function: ahci_fatal_error_recovery
 *************************************************************************//**
 * This function performs recovery from fatal errors defined in 
 * ACHI 1.1 Section 6.2.2 and notifies the mass storage layer of uncompleted commands.
 *
 * @param[in] port_num SATA port number.
 * @param[in] force_comreset flag to force a COMRESET.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_fatal_error_recovery(UINT32_T port_num, BOOLEAN_T force_comreset)
{
    UINT32_T dev_status;
    UINT32_T outstanding_cmd_slots = 0;
    BOOLEAN_T media_was_ready = ata_dev[port_num].bDeviceInitComplete;
    BOOLEAN_T port_was_reset = FALSE;

    DEBUG("-> ahci_fatal_error_recovery()\n");

    if (ata_dev[port_num].bNCQ)
    {
        outstanding_cmd_slots = READ32(PxSACT(port_num));
    }

    // Stop the DMA engine.
    ahci_stop(port_num);

    // Check the device status.
    dev_status = READ32(PxTFD(port_num));

    if ((dev_status & (PTFD_STS_BSY_BIT | PTFD_STS_DRQ_BIT)) ||
        outstanding_cmd_slots || force_comreset)
    {
        port_was_reset = TRUE;
        CRIT("-> ahci_fatal_error_recovery() - dev_status = 0x%08x, outstanding_cmd_slots = 0x%08x, force_comreset = %u.\n", 
             dev_status, outstanding_cmd_slots, force_comreset);
        // Device is not in stable state if BSY or DRQ is set.  Issue COMRESET.

        // AHCI spec says to to send READ LOG EXT after restarting command list
        // processing, but this results in a OVERFLOW interrupt and port failure
        // so just reset the port instead.
        if (STATUS_OK != ahci_port_reset(port_num))
        {
#if REMOVABLE_MEDIA_DEVICE
            /* Leave the dock online and expose no media until hot-plug. */
            ata_dev[port_num].bDeviceInitComplete = FALSE;
            ata_dev[port_num].bDeviceInitTimedOut = TRUE;
#else
            // System reset if the port reset fails.
            system_reset();
#endif
        }
    }

#if REMOVABLE_MEDIA_DEVICE
    /* COMRESET can revoke RDX access and always invalidates per-link media
     * classification. Repeat IDENTIFY and admission from foreground context
     * before exposing media again. */
    if (port_was_reset && media_was_ready)
    {
        ahci_schedule_media_discovery(port_num, TRUE);
    }
#endif

    // Restart command list processing.
    ahci_start(port_num);

    // Queue ATA error callback to notify mass storage layer if USB is in configured state.
    // Disconnecting USB during active transfer can cause fatal errors which cannot be reported by the MSC layer.
    if ((usb_dev.dev_state == USB_DEVICE_STATE_CONFIGURED) &&
        media_was_ready &&
        !ahci_callbacks_are_pending(port_num))
    {
        ahci_ata_cbk_queue_add(port_num, ata_dev[port_num].pAtaErrorCallback);
    }

    return;
}


/*****************************************************************************
 * Function: ahci_set_port_speed
 *************************************************************************//**
 * This function limits the speed negotiation to the specified Generation rate.
 * ahci_stop() must be called before using this function if PxCMD.ST is set.
 *
 * @param[in] port_num SATA port number.
 * @param[in] gen generation: [0] = no speed restriction, [1] = 1.5 Gb/s, [2] = 3 Gb/s)
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_set_port_speed(UINT32_T port_num, UINT32_T gen)
{
    INFO("ahci_set_port_speed() - Gen-%u.\n", gen);

    // Limit speed and start device detection.
    MODIFY32(PxSCTL(port_num), (PSCTL_DET_MASK | PSCTL_SPD_MASK), (gen << 4) | PSCTL_DET_RESET);

    // Wait for at least 1 ms to ensure COMRESET is sent.
    msleep(5);

    // Stop device detection.
    MODIFY32(PxSCTL(port_num), PSCTL_DET_MASK, 0);

    return;
}


/*****************************************************************************
 * Function: ahci_current_interface_speed
 *************************************************************************//**
 * This function returns the current SATA interface speed.
 *
 * @param[in] port_num SATA port number.
 *
 * @retval 0 when communication has not been established.
 * @retval 1 when Gen-1 (1.5 Gbps) rate negotiated.
 * @retval 2 when Gen-2 (3.0 Gbps) rate negotiated.
 *
 ******************************************************************************
 */
UINT32_T ahci_current_interface_speed(UINT32_T port_num)
{
    return(READ32(PxSSTS((port_num))) & PSSTS_SPD_MASK) >> PSSTS_SPD_OFFSET;
}


#define PHY_READY_RETRY_LIMIT  2

/*****************************************************************************
 * Function: ahci_init_port
 *************************************************************************//**
 * This function performs initialization for a port.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_init_port(UINT32_T port_num)
{
    UINT32_T retry_cnt = 0;
#if REMOVABLE_MEDIA_DEVICE
    UINT32_T discovery_epoch;
#endif
    STATUS_T status = STATUS_OK;

    // Clear device init flag.
    ata_dev[port_num].bDeviceInitComplete = FALSE;
    ata_dev[port_num].bDeviceInitTimedOut = FALSE;
    sata_media_reset(port_num);
#if REMOVABLE_MEDIA_DEVICE
    discovery_epoch = sata_media_get_epoch(port_num);
    rdx_hardware_media_reinitializing(port_num);
#endif

    /* The interface-error source is independent of PxIE hot-plug events.
     * Keep it masked until a cartridge has completed discovery.  An empty RDX
     * bay can report transient receiver errors while no usable SATA target is
     * present; fixed-disk recovery would interpret a stuck init command as a
     * fatal fault and reset the entire USB controller. */
    WRITE_REG32(VIM_REQMASKCLR0, 0x00000020);

    // Disable all port interrupts.
    WRITE32(PxIE(port_num), 0);

    /* Establish a clean connect-change baseline before blocking discovery.
     * Any later transition stays latched and is checked before readiness. */
    WRITE32(PxIS(port_num), PORT_CONNECT_CHANGE_STATUS);
    WRITE32(AHCI_REG_OFF(IS_REG_OFF), (0x01U << port_num));

    // Set port HW init regs.  
    // (Hot plug capability. No cold presence detection, no mechanical presence switch)
    WRITE32(PxCMD(port_num), PCMD_HPCP_BIT | PCMD_SUD_BIT);

#if FORCE_GEN1_SPEED
    // Set the port speed negotiation limit to Gen-1 (1.5Gb/s).
    ahci_set_port_speed(port_num, 1);
#else
    // Set the port speed negotiation limit to Gen-2 (3.0Gb/s).
    ahci_set_port_speed(port_num, 2);
#endif 

    // Verify PxCMD.ST, PxCMD.CR, PxCMD.FRE, and PxCMD.FR are all cleared. (port is idle).
    if (READ32(PxCMD(port_num)) & (PCMD_ST_BIT | PCMD_FRE_BIT | PCMD_FR_BIT | PCMD_CR_BIT))
    {
        // Port is NOT idle.
        CRIT("@Warning: SATA port is not idle on init!\n");

        // If port is not idle, clear PxCMD.ST, wait at least 500ms for PxCMD.CR to read 0.  
        status = ahci_stop(port_num);

        if (status == STATUS_OK)
        {
            // If PxCMD.FRE is 1, then clear it and wait at least 500ms for PxCMD.FR to read 0. 
            MODIFY32(PxCMD(port_num), PCMD_FRE_BIT, 0);
            status = ahci_wait_complete(PxCMD(port_num), PCMD_FR_BIT , 0x0, 2000);
        }

        // If either PxCMD.CR or PxCMD.FR do not clear, then attempt port reset.
        if (status != STATUS_OK)
        {
            status = ahci_port_reset(port_num);
        }
    }

    if ((status == STATUS_OK) &&
        ((READ32(PxCI(port_num)) | READ32(PxSACT(port_num))) != 0U))
    {
        /* A prior timed-out command can leave ownership bits set even after
         * command-list processing stops. COMRESET must retire that slot
         * before a new discovery epoch clears its device-fault latch. */
        status = ahci_port_reset(port_num);
    }

    /* Device-fault status belongs to the prior command/link epoch. Clear it
     * only after the port has been stopped or confirmed idle, before the first
     * command of this fresh discovery pass is built. */
    if (status == STATUS_OK)
    {
        ata_dev[port_num].bDeviceFault = FALSE;
    }

    // Clear data in ACHI memory structs referenced by P#CLB and P#FB as recommended by databook.
    ti_memset((void*)&datapath_ram->ahci_mem[port_num].ahci_cmd_list[0], 0, sizeof(datapath_ram->ahci_mem[port_num].ahci_cmd_list));
    ti_memset((void*)&datapath_ram->ahci_mem[port_num].ahci_rfis, 0, sizeof(datapath_ram->ahci_mem[port_num].ahci_rfis));

    // Program base addresses for command list and FIS.
    WRITE32(PxCLB(port_num), (UINT32_T)datapath_ram->ahci_mem[port_num].ahci_cmd_list);   /* Must be 1-KB aligned */
    WRITE32(PxCLBU(port_num), 0x0);
    WRITE32(PxFB(port_num), (UINT32_T)&datapath_ram->ahci_mem[port_num].ahci_rfis);  /* Must be 256-byte aligned */

    // Set PxCMD.FRE to 1 (FIS receive enabled).
    MODIFY32(PxCMD(port_num), PCMD_FRE_BIT, PCMD_FRE_BIT);

    DEBUG("Waiting for SATA PHY Ready...\n");
    do
    {
        if (status != STATUS_OK)
        {
            INFO("P%uSERR = 0x%08x.\n", port_num, READ32(PxSERR(port_num)));
            ahci_port_reset(port_num);
        }

        // Wait for PHY ready (PxSSTS.DET = 0x3).
        status = ahci_wait_complete(PxSSTS(port_num), PSSTS_DET_MASK, PSSTS_DET_PHY_READY, 250);

        if (++retry_cnt > PHY_READY_RETRY_LIMIT) break;

    } while (status != STATUS_OK);

    if (status == STATUS_OK)
    {
        CRIT("SATA Gen-%u speed negotiated.\n", ahci_current_interface_speed(port_num));

        DEBUG("Waiting for SATA device to become ready...\n");
        // Wait until PxTFD.STS.BSY, PxTFD.STS.DRQ, and PxTFD.STS.ERR are all 0.  (SATA drive ready)
        // ATA-6 pg 314 indicates 30 secs timeout.
        status = ahci_wait_complete(PxTFD(port_num), (PTFD_STS_BSY_BIT | PTFD_STS_DRQ_BIT | PTFD_STS_ERR_BIT), 0, 30000);

        // Check PxSIG register for device signature (reset value = 0xFFFFFFFF). 
        // Both ATA and ATAPI devices have 0x0101 in lower word of their signatures. 
        if (status == STATUS_OK)
        {
            // We should have received signature before PxTFD.STS bits were cleared.
            status = ahci_wait_complete(PxSIG(port_num), 0x0000FFFF, 0x0101, 10);
        }
        else
        {
            CRIT("@Error: Timeout waiting for SATA device ready!\n");
        }
    }
    else
    {
        CRIT("@Error: Timeout waiting for PHY Ready. P%uSSTS = 0x%08x, P%uSERR = 0x%08x.\n", port_num, READ32(PxSSTS(port_num)), port_num, READ32(PxSERR(port_num)));
        ata_dev[port_num].bDeviceInitTimedOut = TRUE;
    }

    if (status == STATUS_OK)
    {
        // Start processing command list.
        ahci_start(port_num);

        // Determine from signature whether device is ATA or ATAPI type.
        status = ahci_classify_device(port_num);
    }

    if (status == STATUS_OK)
    {
        // If ATAPI device, set PxCMD.ATAPI to 1. (this is for activity LED signal output).
        if (ata_dev[port_num].bPacketDevice)
        {
            MODIFY32(PxCMD(port_num), PCMD_ATAPI_BIT, PCMD_ATAPI_BIT); 
        }

        // Send IDENTIFY DEVICE or IDENTIFY PACKET DEVICE command.
        status = ahci_identify_device(port_num);

        if ((status == STATUS_OK) &&
            !sata_media_start_after_identify(
                port_num, &ata_dev[port_num]))
        {
            status = STATUS_ERROR;
        }

#if AHCI_LINK_POWER_MGMT_ENABLE 
        if (ata_dev[port_num].bLPM)
        {
            // Disable PHY ready change interrupt. (not required since we never enable this interrupt.)
            //MODIFY32(PxIE(port_num), PHY_READY_CHANGE_STATUS, 0);

            // Set Aggressive Partial state (exit time < 10us).  (Slumber is < 10ms)
            MODIFY32(PxCMD(port_num), PCMD_ASP_BIT, 0x0);

//            // Enabled automatic Partial to Slumber transition.
//            MODIFY32(PxCMD(port_num), PCMD_APSTE_BIT, PCMD_APSTE_BIT);

            // Enable power management.
            MODIFY32(PxCMD(port_num), PCMD_ALPE_BIT, PCMD_ALPE_BIT);
        }
#else
        // Disable IPM transitions. (By default devices are not permitted to attempt IPM transistions but we set this to make sure we don't accept).
        MODIFY32(PxSCTL(port_num), PSCTL_IPM_MASK, (PSCTL_IPM_DISABLE_BOTH << PSCTL_IPM_OFFSET));
#endif

    }

#if !FORCE_GEN1_SPEED
    if (status == STATUS_OK)
    {
        // If device supports Gen2 interface speed and our current connection speed is lower,
        // attempt to renegotiate at higher speed.
        if ((ata_dev[port_num].bSATA_Gen >= 2) && (ahci_current_interface_speed(port_num) < 2))
        {
            CRIT("Retrying SATA Gen-2 speed...\n");
            ahci_stop(port_num);

            // First, try a port reset to renegotiate higher speed.
            ahci_port_reset(port_num);

            // If speed is still not Gen-2, try a more aggressive HBA reset.
            if (ahci_current_interface_speed(port_num) < 2)
            {
                ahci_hba_reset();

                // Turn on global interrupt enable.
                WRITE32(AHCI_REG_OFF(GHC_REG_OFF), GHC_INT_EN_BIT);

                /* HBA reset can clear the port command register. Restore
                 * hot-plug capability and staggered spin-up before the next
                 * COMRESET/PHY negotiation. */
                WRITE32(PxCMD(port_num), PCMD_HPCP_BIT | PCMD_SUD_BIT);

                // Workaround for drives that send COMINIT after Gen3 negotiation failure for Intel ICH10 southbridge.
                // A delay between 50 and 900 us is required here to allow time for device to send COMINIT 
                // before we start COMRESET so we can link at Gen2. (Fix for Toshiba MQ01ABD100 - FW:AX0P2D)
                usleep(250);

                // Initiate COMRESET at Gen2.
                ahci_set_port_speed(port_num, 2);

                // Set PxCMD.FRE to 1 (FIS receive enabled).
                MODIFY32(PxCMD(port_num), PCMD_FRE_BIT, PCMD_FRE_BIT);

                status = ahci_wait_complete(PxSSTS(port_num), PSSTS_DET_MASK, PSSTS_DET_PHY_READY, 50);
            }

            if (status == STATUS_OK)
            {
                CRIT("SATA Gen-%u speed negotiated.\n", ahci_current_interface_speed(port_num));

                // Wait until PxTFD.STS.BSY, PxTFD.STS.DRQ, and PxTFD.STS.ERR are all 0.  (SATA drive ready)
                // ATA-6 pg 314 indicates 30 secs timeout.
                status = ahci_wait_complete(PxTFD(port_num), (PTFD_STS_BSY_BIT | PTFD_STS_DRQ_BIT | PTFD_STS_ERR_BIT), 0, 30000);

                if (status == STATUS_OK)
                {
                    ahci_start(port_num);
                }
            }
        }
    }
#endif

    if (status == STATUS_OK)
    {
#if DEBUG_LEVEL >= 1
        ahci_dump_dev_info(port_num);
#endif

        // Send SET FEATURES command to set PIO transfer mode.
        status = ahci_set_features_xfer_mode(port_num, FALSE);

        if (status == STATUS_OK)
        {
            // Send SET FEATURES command to set DMA transfer mode.
            status = ahci_set_features_xfer_mode(port_num, TRUE);
        }
    }

    if (status == STATUS_OK)
    {
        /* The published USB transport is BOT, which cannot use DMA Setup FIS
         * Auto-Activate. Keeping it disabled also avoids optional-device
         * startup commands that are unrelated to BOT transfers. */
        ata_dev[port_num].bDMA_SetupAutoActivateSupport = FALSE;
    }

    /* Select either authenticated RDX extent mapping or validated generic
     * direct-LBA access. Both paths converge on the same readiness, slider,
     * LED, eject, and hot-plug lifecycle below. */
    if (status == STATUS_OK)
    {
        if (!sata_media_prepare(port_num, &ata_dev[port_num]))
        {
            status = STATUS_ERROR;
        }
#if REMOVABLE_MEDIA_DEVICE
        else
        {
            /* sata_media_prepare() opens the admitted epoch after its own
             * reset boundary. Track that exact epoch through final publish. */
            discovery_epoch = sata_media_get_epoch(port_num);
        }
#endif
    }

    // Clear port error register by writing ones.
    WRITE32(PxSERR(port_num), 0xFFFFFFFF);

    /* Clear command/error status but preserve a connect change that arrived
     * during the blocking discovery sequence. */
    WRITE32(PxIS(port_num),
            0xFFFFFFFFU & ~PORT_CONNECT_CHANGE_STATUS);

    // Clear global interrupt status by writing one.
    WRITE32(AHCI_REG_OFF(IS_REG_OFF), (0x01 << port_num));

    if (status == STATUS_OK)
    {
        // Set medium change flag.
        ata_dev[port_num].bMediumChanged = TRUE;

#if REMOVABLE_MEDIA_DEVICE
        /* Cache the physical lock-slider state before publishing readiness.
         * This potentially blocking SPI sample therefore finishes before any
         * host write command can pass the ready and write-protect gates. */
        rdx_hardware_prepare_media_ready(port_num);

        /* USB submission and both SATA handlers must stay outside the final
         * check/publish window. Enabling SATA before USB on exit guarantees a
         * newly latched link event invalidates this epoch before a host command
         * can observe readiness. */
        WRITE_REG32(VIM_REQMASKCLR0,
                    AHCI_MEDIA_PUBLICATION_INTERRUPT_MASK);
        if ((sata_media_get_epoch(port_num) != discovery_epoch) ||
            ((READ32(PxSSTS(port_num)) & PSSTS_DET_MASK) !=
             PSSTS_DET_PHY_READY) ||
            ((READ32(PxIS(port_num)) &
              PORT_CONNECT_CHANGE_STATUS) != 0U))
        {
            /* Do not publish capacity or mapping from a disk that changed
             * while IDENTIFY, admission, or the slider sample was running. */
            if ((READ32(PxSSTS(port_num)) & PSSTS_DET_MASK) !=
                PSSTS_DET_PHY_READY)
            {
                sata_media_link_disconnected(port_num);
            }
            ahci_schedule_media_discovery(port_num, FALSE);
            status = STATUS_ERROR;
        }
        else
        {
            ata_dev[port_num].bDeviceInitComplete = TRUE;
            WRITE32(PxIE(port_num), PORT_DEFAULT_INTR_ENABLE);
        }

        if (status == STATUS_OK)
        {
            WRITE_REG32(VIM_REQMASKSET0, AHCI_SATA_INTERRUPT_MASK);
        }
        else
        {
            WRITE_REG32(VIM_REQMASKSET0,
                        AHCI_CONTROLLER_INTERRUPT_MASK);
        }
        WRITE_REG32(VIM_REQMASKSET0, AHCI_USB_INTERRUPT_MASK);
#endif
    }

#if !REMOVABLE_MEDIA_DEVICE
    if (status == STATUS_OK)
    {
        // Set Device Ready flag.
        ata_dev[port_num].bDeviceInitComplete = TRUE;

        // Enable required port interrupts.
        WRITE32(PxIE(port_num), PORT_DEFAULT_INTR_ENABLE);

        // Receiver-error recovery is meaningful only for an admitted target.
        WRITE_REG32(VIM_REQMASKSET0, AHCI_SATA_RX_ERROR_INTERRUPT_MASK);
    }
#endif

    if (status != STATUS_OK)
    {
        /* Any unsuccessful discovery leaves a valid empty RDX dock, not a
         * transport that is still waiting for SATA initialization.  The
         * fixed-disk path only set this flag when PHY negotiation itself timed
         * out. An empty RDX bay can instead reach PHY-ready and fail
         * while waiting for a device signature/TFD; if USB configuration
         * completes after that callback, BOT reset otherwise never enters
         * IDLE and Windows cannot issue the INQUIRY that creates its disk
         * PDO. */
        ata_dev[port_num].bDeviceInitTimedOut = TRUE;

        // Enable connect change only when foreground rediscovery is not
        // already responsible for the latched event.
        WRITE32(PxIE(port_num), ahci_hotplug_pending[port_num] ?
                0U : PORT_CONNECT_CHANGE_STATUS);
    }

    // Call port init callback.
    if (ata_dev[port_num].pSATAPortInitCallback)
    {
        ata_dev[port_num].pSATAPortInitCallback(port_num);
    }

    return status;
}


/*****************************************************************************
 * Function: ahci_handle_sdb_fis
 *************************************************************************//**
 * This function handles a Set Device Bits FIS by adding the appropriate ATA 
 * command callback to the callback queue.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_handle_sdb_fis(UINT32_T port_num)
{
    ATA_CMD_CALLBACK_T* ata_cmd_cbk_param = &ata_dev[port_num].ata_callback_data[ata_dev[port_num].callback_index];
    volatile UINT8_T *sdb_fis = datapath_ram->ahci_mem[port_num].ahci_rfis.Set_Device_Bits_FIS;

    ata_cmd_cbk_param->bPortNum = port_num;
    // Set dummy command slot value to differentiate this SDB FIS callback from D2H Register FIS callback.
    ata_cmd_cbk_param->bCmdSlot = SDB_FIS_CMD_SLOT_NUM;
    //ata_cmd_cbk_param->bError = sdb_fis[3];
    // Save status from SDB FIS.
    ata_cmd_cbk_param->bStatus = sdb_fis[2];

    // Save SActive from SDB FIS.
//    ata_cmd_cbk_param->dSActive = *(UINT32_T*)&sdb_fis[4];  

    // SDB FIS interrupts may be missed for short transfer length queued R/W with disks such as WDC WD5000HHTZ VelociRaptor disk
    // so we must determine the completed commands from the SACT register instead of the SActive field in the SDB FIS.
    ata_cmd_cbk_param->dSActive = ata_dev[port_num].dSActive ^ READ32(PxSACT(port_num));
    // Update SActive flags.
    ata_dev[port_num].dSActive &= ~ata_cmd_cbk_param->dSActive;

    INFO("-> ahci_handle_sdb_fis() - SActive = 0x%08x.\n", ata_cmd_cbk_param->dSActive);

    // Add callback to queue.
    ahci_ata_cbk_queue_add(port_num, ata_dev[port_num].pAtaCmdCallback);

    return;
}

#define DMA_SETUP_FIS_DIRECTION_BIT 0x20
#define DMA_SETUP_TAG_MASK          0x1F

/*****************************************************************************
 * Function: ahci_handle_dma_setup_fis
 *************************************************************************//**
 * This function handles a DMA Setup FIS by adding the appropriate ATA 
 * command callback to the callback queue.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_handle_dma_setup_fis(UINT32_T port_num)
{
    ATA_CMD_CALLBACK_T* ata_cmd_cbk_param = &ata_dev[port_num].ata_callback_data[ata_dev[port_num].callback_index];
    volatile UINT8_T *dma_setup_fis = datapath_ram->ahci_mem[port_num].ahci_rfis.DMA_Setup_FIS;

    INFO("-> ahci_handle_dma_setup_fis()\n");

    ata_cmd_cbk_param->bPortNum = port_num;

    // Read command slot from DMA Setup FIS.
    ata_cmd_cbk_param->bCmdSlot = dma_setup_fis[4] & DMA_SETUP_TAG_MASK;

    // Read DMA transfer count from DMA Setup FIS.
    ata_cmd_cbk_param->dDataByteCnt = *(UINT32_T*)&dma_setup_fis[20];

    /**** This data is not required by the UAS layer so avoid reading it to make ISR faster. ****/
    //ata_cmd_cbk_param->pData = ata_dev[port_num].pData[ata_cmd_cbk_param->bCmdSlot];
    // // Read transfer direction from DMA Setup FIS.
    // ata_cmd_cbk_param->bDirection = (dma_setup_fis[1] & DMA_SETUP_FIS_DIRECTION_BIT) ? ENDPT_DIRECTION_IN : ENDPT_DIRECTION_OUT;   

    // Add callback to queue.
    ahci_ata_cbk_queue_add(port_num, ata_dev[port_num].pAtaQueuedCmdCallback);

    gDiskActivity = TRUE;

    return;
}

#define ATA_RETURN_DESC_EXTEND_BIT   0x01

/*****************************************************************************
 * Function: ahci_set_scsi_sense_data
 *************************************************************************//**
 * This function populates SCSI sense information with the status from a 
 * Device to Host Register FIS or PIO Setup FIS.  
 *
 * @param[in] port_num SATA port number.
 * @param[in] fis pointer to D2H Register FIS or PIO Setup FIS.
 * @param[in] d2h_reg_fis flag indicating whether the FIS is a D2H Register FIS (TRUE) or PIO Setup FIS (FALSE.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_set_scsi_sense_data(UINT32_T port_num, UINT8_T *fis, BOOLEAN_T d2h_reg_fis)
{
    BOOLEAN_T set_ata_return_data = FALSE;
    UINT8_T status_reg;
    UINT8_T lba_upper_nonzero;
    UINT8_T sector_cnt_upper_nonzero;

    // Read Status field for D2H Register FIS or E_Status field for PIO Setup FIS.
    status_reg = (d2h_reg_fis) ? fis[2] : fis[15];

    if (ata_dev[port_num].bCheckCondition[ata_dev[port_num].bCurrentCmdSlot] && 
        !ata_dev[port_num].bDeviceFault && !(status_reg & ATA_ERROR_STATUS_BIT))
    {
        // Set sense data according to SAT-2 specification.
        scsi_set_sense_data(SCSI_CORRECTED_ERROR, ATA_PASS_THROUGH_INFO_AVAIL, ASCQ_ATA_PASS_THROUGH_INFO_AVAIL);
        set_ata_return_data = TRUE;
    }
    else if (ata_dev[port_num].bDeviceFault || (status_reg & ATA_ERROR_STATUS_BIT))
    {
        // Sense data set according to clause 11 in SAT-2 spec.
        ahci_translate_error_to_sense_data(port_num);
        set_ata_return_data = TRUE;
    }
    else
    {
        // No sense info.
        scsi_set_sense_data(NO_SENSE, NO_ADDITIONAL_SENSE_INFO, NO_ASCQ);           
    }

    if (set_ata_return_data)
    {
        // Populate ATA return descriptor data.
        /* ata_return_descriptor_data[2] - Extend bit was set when processing the ATA PASS-THROUGH command */
        ata_return_descriptor_data[3]  = fis[3];     /* error          */
        ata_return_descriptor_data[4]  = fis[13];    /* sector_cnt_exp */
        ata_return_descriptor_data[5]  = fis[12];    /* sector_cnt     */
        ata_return_descriptor_data[6]  = (ata_return_descriptor_data[2] & ATA_RETURN_DESC_EXTEND_BIT) ? fis[8] : 0;     /* LBA_low_exp    */
        ata_return_descriptor_data[7]  = fis[4];     /* LBA_low        */
        ata_return_descriptor_data[8]  = (ata_return_descriptor_data[2] & ATA_RETURN_DESC_EXTEND_BIT) ? fis[9] : 0;     /* LBA_mid_exp    */
        ata_return_descriptor_data[9]  = fis[5];     /* LBA_mid        */
        ata_return_descriptor_data[10] = (ata_return_descriptor_data[2] & ATA_RETURN_DESC_EXTEND_BIT) ? fis[10] : 0;    /* LBA_high_exp   */
        ata_return_descriptor_data[11] = fis[6];     /* LBA_high       */
        ata_return_descriptor_data[12] = fis[7];     /* device         */
        ata_return_descriptor_data[13] = status_reg; /* status         */

        // Populate fixed format sense info.
        // Information field:
        fixed_format_sense_data[0]  = 0xF0;       /* Valid = 1, response code = 0x70 */
        fixed_format_sense_data[3]  = fis[3];     /* error      */
        fixed_format_sense_data[4]  = status_reg; /* status     */
        fixed_format_sense_data[5]  = fis[7];     /* device     */
        fixed_format_sense_data[6]  = fis[12];    /* sector_cnt */

        sector_cnt_upper_nonzero = (fis[13]) ? 0x40 : 0x00;
        lba_upper_nonzero = (fis[8] + fis[9] + fis[10]) ? 0x20 : 0x00;

        // Command specific info field:
        fixed_format_sense_data[8]  = (ata_return_descriptor_data[2] << 7) | sector_cnt_upper_nonzero | lba_upper_nonzero;  /* Log Index = 0 */
        fixed_format_sense_data[9]  = fis[6];     /* LBA_high */
        fixed_format_sense_data[10] = fis[5];     /* LBA_mid  */
        fixed_format_sense_data[11] = fis[4];     /* LBA_low  */
    }

    return;
}


#define ATA_DEVICE_FAULT_STATUS_BIT  0x20
#define ATA_BUSY_STATUS_BIT          0x80    
#define ATA_DATA_REQUEST_BIT         0x08

/*****************************************************************************
 * Function: ahci_handle_d2h_reg_fis
 *************************************************************************//**
 * This function handles a Device to Host Register FIS by setting sense 
 * data if necessary and calling the appropriate ATA command callback.  
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_handle_d2h_reg_fis(UINT32_T port_num)
{
    ATA_CMD_CALLBACK_T *ata_cmd_cbk_param = &ata_dev[port_num].ata_callback_data[ata_dev[port_num].callback_index];
    REGISTER_FIS_D2H_T *d2h_fis = (REGISTER_FIS_D2H_T*)datapath_ram->ahci_mem[port_num].ahci_rfis.D2H_Register_FIS;

    INFO("-> ahci_handle_d2h_reg_fis()\n");

    ata_dev[port_num].bDeviceFault = (d2h_fis->status & ATA_DEVICE_FAULT_STATUS_BIT) ? TRUE : FALSE;

    ahci_set_scsi_sense_data(port_num, (UINT8_T *)d2h_fis, TRUE);

    ata_cmd_cbk_param->bPortNum = port_num;
    ata_cmd_cbk_param->bCmdSlot = ata_dev[port_num].bCurrentCmdSlot;
    //ata_cmd_cbk_param->bError = d2h_fis->error;
    ata_cmd_cbk_param->bStatus = d2h_fis->status;

//    ata_cmd_cbk_param->pData = ata_dev[port_num].pData[ata_dev[port_num].bCurrentCmdSlot];

    // Get PRD byte count from command header.
    ata_cmd_cbk_param->dDataByteCnt = ahci_get_PRD_byte_count(port_num, ata_dev[port_num].bCurrentCmdSlot);

    // Check byte count so we don't blink disk activity LED for TEST UNIT READY commands.
    if (ata_cmd_cbk_param->dDataByteCnt)
    {
        gDiskActivity = TRUE;
    }

    // Add callback to queue.
    ahci_ata_cbk_queue_add(port_num, ata_dev[port_num].pAtaCmdCallback);

    return;
}

#define PIO_SETUP_FIS_DIRECTION_BIT 0x20

/*****************************************************************************
 * Function: ahci_handle_pio_setup_fis
 *************************************************************************//**
 * This function handles a PIO Setup FIS by adding the appropriate ATA 
 * command callback to the callback queue if the data transfer is complete.
 * PIO Setup FIS transfer count must be even and cannot exceed 8192 bytes.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_handle_pio_setup_fis(UINT32_T port_num)
{
    ATA_CMD_CALLBACK_T *ata_cmd_cbk_param = &ata_dev[port_num].ata_callback_data[ata_dev[port_num].callback_index];
    volatile UINT8_T *pio_setup_fis = datapath_ram->ahci_mem[port_num].ahci_rfis.PIO_Setup_FIS;

    INFO("-> ahci_handle_pio_setup_fis()\n");

    gDiskActivity = TRUE;

    // Return if BSY bit is set in E_Status as there are more data blocks to be transferred.
    if (pio_setup_fis[15] & ATA_BUSY_STATUS_BIT)
        return;

    // Wait for command completion for accurate PRD byte count info.
    while (READ32(PxCI(0)) & ata_dev[port_num].bCurrentCmdSlot);

    ahci_set_scsi_sense_data(port_num, (UINT8_T *)pio_setup_fis, FALSE);

    ata_cmd_cbk_param->bPortNum = port_num;
    ata_cmd_cbk_param->bCmdSlot = ata_dev[port_num].bCurrentCmdSlot;
    //ata_cmd_cbk_param->bError = pio_setup_fis[3];
    ata_cmd_cbk_param->bStatus = pio_setup_fis[2];

//    ata_cmd_cbk_param->pData = ata_dev[port_num].pData[ata_cmd_cbk_param->bCmdSlot];
    // Get PRD byte count. 
    ata_cmd_cbk_param->dDataByteCnt = ahci_get_PRD_byte_count(port_num, ata_dev[port_num].bCurrentCmdSlot);
    INFO("PIO byte cnt = %u\n", ata_cmd_cbk_param->dDataByteCnt);
    // Read transfer direction from PIO SETUP FIS.
    //ata_cmd_cbk_param->bDirection = (pio_setup_fis[1] & PIO_SETUP_FIS_DIRECTION_BIT) ? ENDPT_DIRECTION_IN : ENDPT_DIRECTION_OUT;   

    // Add callback to queue.
    ahci_ata_cbk_queue_add(port_num, ata_dev[port_num].pAtaCmdCallback);

    // Clear the interrupt to make sure we don't get multiple callbacks.
    WRITE32(PxIS(port_num), PIO_SETUP_FIS_INTR);

    return;
}


/*****************************************************************************
 * Function: ahci_port_intr_handler
 *************************************************************************//**
 * This function handles a port interrupt.
 *
 * @param[in] port_num SATA port number.
 * @param[in] port_status port interrupt status.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ahci_port_intr_handler(UINT32_T port_num, UINT32_T port_status)
{
    UINT8_T status;
    UINT8_T error;

    /* A link change is terminal for every completion bit captured in the
     * same interrupt snapshot. Queue one transport error for the old medium
     * and let foreground discovery establish the next command epoch. */
    if ((port_status & PORT_CONNECT_CHANGE_STATUS) != 0U)
    {
        ahci_handle_media_link_change(port_num);
        return;
    }

    if (port_status & PORT_FATAL_ERROR_INTR)
    {
        if (port_status & INTERFACE_FATAL_ERROR_STATUS)
        {
            CRIT("AHCI interface fatal error!  P%uSERR = 0x%08x.\n", port_num, READ32(PxSERR(port_num)));
        }
        else if (port_status & TASK_FILE_ERROR_STATUS)
        {
            ahci_get_TFD_info(port_num, &status, &error);
            CRIT("AHCI task file error! P%uTFD error = 0x%02x, status = 0x%02x.\n", port_num, error, status);
            ahci_translate_error_to_sense_data(port_num);
        }
        else
        {
            // Other fatal errors.
            CRIT("AHCI fatal error! P%uIS = 0x%08x.\n", port_num, port_status);
        }

        // Perform recovery.
        ahci_fatal_error_recovery(port_num, FALSE);
        return;
    }

    if (port_status & SET_DEVICE_BITS_FIS_INTR)
    {
        /* For NCQ Cmds */
        ahci_handle_sdb_fis(port_num);
    }

    if (port_status & DMA_SETUP_FIS_INTR)
    {
        /* For NCQ Cmds */
        ahci_handle_dma_setup_fis(port_num);
    }

    if (port_status & D2H_REGISTER_FIS_INTR)
    {
        /* For non-queued Cmds */
        ahci_handle_d2h_reg_fis(port_num);
    }

    if (port_status & PIO_SETUP_FIS_INTR)
    {
        ahci_handle_pio_setup_fis(port_num);     
    }

    if (port_status & OVERFLOW_STATUS)
    {
        CRIT(" AHCI overflow status interrupt.\n");
        // Restart the DMA engine per databook.
        ahci_stop(port_num);
        ahci_start(port_num);
    }

#if 0  // PHY_READY_CHANGE_STATUS interrupt is not compatible with TUSB926x PHY.
    // Use AHCI interface errors to determine device disconnect.
    if (port_status & PHY_READY_CHANGE_STATUS)
    {
        CRIT("SATA PHY ready status change.\n");
        // Clear COMM Wake and PHY ready bits in PxSERR.
        WRITE32(PxSERR(port_num), (PSERR_DIAG_W_BIT | PSERR_DIAG_N_BIT));
    }
#endif

    return;
}

/**
 * @brief Service deferred SATA connect changes from foreground context.
 *
 * The first pass isolates interrupted DMA and callback state exactly once.
 * A request remains pending while DET is below PHY-ready, covering early
 * insertion notifications without repeatedly entering discovery for an empty
 * bay. Once DET reaches 3, the normal port initialization and RDX admission
 * path runs once.
 */
void ahci_service(void)
{
    UINT32_T port_num;
    STATUS_T status;

#if REMOVABLE_MEDIA_DEVICE
    /* Never enter a blocking port operation while PWM1 has an active
     * deadline. The mechanism service must regain foreground control until
     * motion has settled or failed. */
    if (rdx_hardware_eject_in_progress())
    {
        return;
    }
#endif

    for (port_num = 0U; port_num < NUM_AHCI_PORTS; port_num++)
    {
        if (!ahci_hotplug_pending[port_num])
        {
            continue;
        }

        if (ahci_reinit_wait_for_callbacks[port_num])
        {
            if (ahci_callbacks_are_pending(port_num))
            {
                continue;
            }
            ahci_reinit_wait_for_callbacks[port_num] = FALSE;
        }

        /* Never discard a callback merely because the first event did not
         * know one was already queued. Promote it to the same drain barrier. */
        if (ahci_callbacks_are_pending(port_num))
        {
            ahci_reinit_wait_for_callbacks[port_num] = TRUE;
            continue;
        }

        if (!ahci_hotplug_quiesced[port_num])
        {
            /* Commit the one-time quiesce state before any blocking work. A
             * concurrently delivered terminal event can then raise the drain
             * barrier without its callback being cleared or its state lost. */
            ahci_hotplug_quiesced[port_num] = TRUE;
            ahci_stop(port_num);
            mww_force_sata_interface_ready();
            gSATADeviceCount = 0U;
            continue;
        }

        if ((READ32(PxSSTS(port_num)) & PSSTS_DET_MASK) !=
            PSSTS_DET_PHY_READY)
        {
#if REMOVABLE_MEDIA_DEVICE
            /* PxIE may already be masked by deferred receiver recovery.
             * Observe later physical removal here so the next cartridge
             * cannot inherit the previous cartridge's failed-access limit. */
            sata_media_link_disconnected(port_num);
#endif
            continue;
        }

        ahci_hotplug_pending[port_num] = FALSE;
        ahci_hotplug_quiesced[port_num] = FALSE;
        ahci_reinit_wait_for_callbacks[port_num] = FALSE;
        status = ahci_init_port(port_num);
        if (status == STATUS_OK)
        {
            gSATADeviceCount++;
        }
#if !REMOVABLE_MEDIA_DEVICE
        // Reinitialize USB Core after foreground discovery.
        usb_hal_init(NULL, NULL, NULL);
        usb_hal_connect();
#endif
    }
}



/*****************************************************************************
 * Function: ahci_isr
 *************************************************************************//**
 * Interrupt service routine for the AHCI SATA controller.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_isr(void)
{
    UINT32_T IPS;
    UINT32_T IE_mask;
    UINT32_T port_intr;

#if (NUM_AHCI_PORTS == 1)

    // Read IS.IPS register to determine which ports have interrupts pending.  
    IPS = READ32(AHCI_REG_OFF(IS_REG_OFF));

    // Check if port 0 has interrupts pending.
    if (IPS & PI_PORT0)
    {
        // Check PxIS register to determine which interrupt occurred.
        port_intr = READ32(PxIS(0));

        do
        {
            /* A terminal event can isolate the port while this ISR loops.
             * Re-read PxIE so newly latched completion bits cannot be
             * dispatched with the previous media epoch's mask. */
            IE_mask = READ32(PxIE(0));
            DEBUG("AHCI P0IS = 0x%08x, IE = 0x%08x.\n", port_intr, IE_mask);

            // Write 1's to clear the interrupt status bits that are not read-only.
            WRITE32(PxIS(0), port_intr);

            // Call port interrupt handler. (mask out any interrupts that are not currently enabled)
            ahci_port_intr_handler(0, (port_intr & IE_mask));

            // Recheck for any new interrupts.
            port_intr = READ32(PxIS(0));

        } while (port_intr);

        // Write 1 to clear the port interrupt.
        WRITE32(AHCI_REG_OFF(IS_REG_OFF), PI_PORT0);
    }

#else

    UINT32_T port_num;     

    // Read IS.IPS register to determine which ports have interrupts pending.  
    IPS = READ32(AHCI_REG_OFF(IS_REG_OFF));

    // Check each port.
    for (port_num = 0; port_num < NUM_AHCI_PORTS; port_num++)
    {
        // Check if port has interrupts pending.
        if (IPS & (0x1 << port_num))
        {
            // Check PxIS register to determine which interrupt occurred.
            port_intr = READ32(PxIS(port_num));

            DEBUG("AHCI P%uIS = 0x%08x, IE = 0x%08x.\n", port_num, port_intr, READ32(PxIE(port_num)));

            // Write 1's to clear the interrupt status bits that are not read-only.
            WRITE32(PxIS(port_num), port_intr);

            // Mask out any interrupts that are not currently enabled.
            port_intr &= READ32(PxIE(port_num));

            // Call port interrupt handler.
            ahci_port_intr_handler(port_num, port_intr);

            // Write 1 to clear the port interrupt.
            WRITE32(AHCI_REG_OFF(IS_REG_OFF), (0x1 << port_num));
        }
    }

#endif

    return;
}


/*****************************************************************************
 * Function: ahci_reset_lun
 *************************************************************************//**
 * This function stops a port, performs a COMRESET if necessary or if force 
 * flag is set, and restarts command list processing.
 *
 * @param[in] lun logical unit number.
 * @param[in] force_comreset flag to indicate whether a COMRESET should be forced.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_reset_lun(UINT32_T lun, BOOLEAN_T force_comreset)
{
    UINT32_T dev_status;
    UINT32_T port_num;

    CRIT("-> ahci_reset_lun(%u)\n", lun);

    if (!ata_dev[lun].bDeviceInitComplete) return;

    // Disable all port interrupts.
    WRITE32(PxIE(lun), 0);

    // Force SATA interface ready so DMA engine won't get stuck accessing wrap window memory if a transfer was interrupted.
    mww_force_sata_interface_ready();

    /* Discard stale completions before recovery. If recovery queues a fresh
     * error callback, leave it intact so foreground service can drain it
     * before rediscovery reuses command slot zero. */
    ahci_clear_callback_queue(lun);

    // Reset the port if necessary.
    ahci_fatal_error_recovery(lun, force_comreset);

    // Check the device status.
    dev_status = READ32(PxTFD(lun));

    // Re-enable command interrupts only if recovery did not require a fresh
    // foreground media admission pass.
    WRITE32(PxIE(lun), ata_dev[lun].bDeviceInitComplete ?
            PORT_DEFAULT_INTR_ENABLE : 0U);

    if (dev_status & (PTFD_STS_BSY_BIT | PTFD_STS_DRQ_BIT))
    {
#if REMOVABLE_MEDIA_DEVICE
        if (ahci_hotplug_pending[lun])
        {
            /* A COMRESET already invalidated this medium and queued any
             * terminal callback. Foreground service must drain that callback
             * before it reuses slot zero for media discovery. */
            CRIT("ATA device busy after link reset; deferring media discovery.\n");
        }
        else
#endif
        {
        // Device is stuck on a pending data transfer because the HDD was in the process of spinning-up.  Reset system to recover.
#if 0  // 12/09/11 - BQ - attempting a more gentle recovery method to prevent OS from detecting a detach/attach.
        system_reset();
#else
        CRIT("@Error: ATA device busy! Re-initializing HBA.\n");
        // Clear all device init complete flags.
        for (port_num = 0; port_num < gSATADeviceCount; port_num++)
        {
            ata_dev[port_num].bDeviceInitComplete = FALSE;
        }

        // Re-initialize AHCI.
        ahci_init();
#endif
        }
    }

    // Establish unit attention condition.
    scsi_set_sense_data(UNIT_ATTENTION, BUS_DEVICE_RESET_FUNCTION_OCCURRED, ASCQ_BUS_DEVICE_RESET_FUNCTION_OCCURRED);

    return;
}


/* Interface Communication Control values */
#define ICC_SLUMBER 0x6
#define ICC_PARTIAL 0x2
#define ICC_ACTIVE  0x1
#define ICC_IDLE    0x0

/*****************************************************************************
 * Function: ahci_power_mgmt_callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle SATA power
 * management.
 *
 * @param[in] pm_state current USB power state.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_power_mgmt_callback(eUSB_DEVICE_PM_STATE_T pm_state)
{
    UINT32_T port_num;

    DEBUG("-> ahci_power_mgmt_callback() - pm_state = %u.\n", pm_state);

    // Disable AHCI SATA Interrupt to prevent SATA ISR from stomping on any USB power management
    // callbacks that might be acting on SATA drive.
    WRITE_REG32(VIM_REQMASKCLR0, 0x00000040);   /* AHCI SATA Interrupt (Remapped to Ch 6) */   

    for (port_num = 0; port_num < gSATADeviceCount; port_num++)
    {
        if ((pm_state == USB_PM_RESET) || (pm_state == USB_PM_DISCONNECT))
        {
            // Reset the LUN to make sure it's in a good state.
            ahci_reset_lun(port_num, FALSE);
        }
    }

    // Re-enable AHCI SATA Interrupt.
    WRITE_REG32(VIM_REQMASKSET0, 0x00000040);   /* AHCI SATA Interrupt (Remapped to Ch 6) */   

    INFO("<- ahci_power_mgmt_callback() - pm_state = %u.\n", pm_state);

    return;
}

#if 0
/*****************************************************************************
 * Function: ahci_power_down
 *************************************************************************//**
 * This function turns off power to the SATA device.
 *
 * @param[in] port_num SATA port number.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_power_down(UINT32_T port_num)
{
    // If bus-powered or macro option is enabled, power down drive.
    if (ENABLE_SATA_POWER_DOWN_WHEN_USB_NOT_CONFIGURED || !gio_is_usb_device_self_powered())
    {
        // Is power ON?
        if (gio_is_sata_device_powered())
        {
            CRIT("-> ahci_power_down()\n");

            // Disable SATA interface error interrupt.
            WRITE_REG32(VIM_REQMASKCLR0, 0x00000020);   /* SATA interface error (Remapped to Ch 5) */   

            // Turn off SATA device power.
            gio_sata_device_power_enable(FALSE);

            // Clear out device info.
            ti_memset(&ata_dev[port_num], 0, sizeof(ata_dev[0]));

            // Clear device count.
            gSATADeviceCount = 0;
        }
    }

    return;
}
#endif 

#define AHCI_MAX_NUM_PORTS  8   /* max num supported by HW */

typedef enum SATA_TX_MARGIN_LOW_SWING
{
    TX_LOW_SWING_1000MV = 0,
    TX_LOW_SWING_625MV  = 1,
    TX_LOW_SWING_500MV  = 2,
    TX_LOW_SWING_250MV  = 3,
    TX_LOW_SWING_125MV  = 4
} SATA_TX_MARGIN_LOW_SWING_T;

/*****************************************************************************
 * Function: ahci_init
 *************************************************************************//**
 * This function initializes the AHCI SATA controller.
 *
 * @param None.
 *                    
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when no devices are initialized.
 *
 ****************************************************************************** 
 */

STATUS_T ahci_init(void)
{
    UINT32_T pi;
    UINT32_T port_num;

    CRIT("-> ahci_init()\n");

    // Initialize device count.
    gSATADeviceCount = 0;
    for (port_num = 0U; port_num < NUM_AHCI_PORTS; port_num++)
    {
        ahci_hotplug_pending[port_num] = FALSE;
        ahci_hotplug_quiesced[port_num] = FALSE;
        ahci_reinit_wait_for_callbacks[port_num] = FALSE;
    }

    // Register power management callback.
    usb_stack_register_PM_callback(ahci_power_mgmt_callback);

    // Set global HW init regs.
    WRITE32(AHCI_REG_OFF(PI_REG_OFF), PI_PORT0);     /* Port 0 only */
    WRITE32(AHCI_REG_OFF(CAP_REG_OFF), 0x00000000);  /* No additional capabilities */

    // Ignore 'I' bit in DMA SETUP FIS and always trigger interrupt.  (FW requires this for NCQ).
    MODIFY32(AHCI_REG_OFF(DIAGNR1_REG_OFF), DIAGNR1_IGNORE_I_BIT_ENABLE_BIT, DIAGNR1_IGNORE_I_BIT_ENABLE_BIT);

#if SATA_NO_POLARITY_SWAP
    // Set default SATA PHY Tx and Rx polarity. 
    MODIFY32(PxPHYCTRL(0), (PPHY_CTRL_RX_POLARITY | PPHY_CTRL_TX_POLARITY), 0);
#elif SATA_POLARITY_SWAP_RX
    // Swap SATA PHY Rx polarity.
    MODIFY32(PxPHYCTRL(0), PPHY_CTRL_RX_POLARITY, PPHY_CTRL_RX_POLARITY);
#elif SATA_POLARITY_SWAP_BOTH
    // Swap SATA PHY Rx and Tx polarity.
    MODIFY32(PxPHYCTRL(0), (PPHY_CTRL_RX_POLARITY | PPHY_CTRL_TX_POLARITY), (PPHY_CTRL_RX_POLARITY | PPHY_CTRL_TX_POLARITY));
#else
    // Swap SATA PHY Tx polarity. (This is the required setting for TI FPGA platform and EVM)
    MODIFY32(PxPHYCTRL(0), PPHY_CTRL_TX_POLARITY, PPHY_CTRL_TX_POLARITY);
#endif

    // Set Tx swing to low-swing mode (output de-emphasis is disabled).
    MODIFY32(PxPHYCTRL(0), PPHY_CTRL_TX_SWING, PPHY_CTRL_TX_SWING);

    // Set Tx margin to 500 mV.  Per SATA spec, V_diff_TX range is 325-600mV for Gen1u and 275-750mV for Gen2u.
    MODIFY32(PxPHYCTRL(0), PPHY_TX_MARGIN_MASK, TX_LOW_SWING_500MV);

    // Reset controller.
    if (ahci_hba_reset() == STATUS_OK)
    {
        // Turn on global interrupt enable.
        WRITE32(AHCI_REG_OFF(GHC_REG_OFF), GHC_INT_EN_BIT);

        // Read ports implemented.
        pi = READ32(AHCI_REG_OFF(PI_REG_OFF));

        // CAP.NCS value is fixed to 32 so we don't need to check it.

        INFO("AHCI Version = 0x%08x.\n", READ32(AHCI_REG_OFF(VS_REG_OFF)));
        INFO("AHCI ports implemented = 0x%08x.\n", pi);

        // Initialize all implemented ports.
        for (port_num = 0; port_num < NUM_AHCI_PORTS; port_num++)
        {
            if (pi & (0x01 << port_num))
            {
                if (ahci_init_port(port_num) == STATUS_OK)
                {
                    gSATADeviceCount++;
                }
#if REMOVABLE_MEDIA_DEVICE
                else if ((READ32(PxSSTS(port_num)) & PSSTS_DET_MASK) !=
                         PSSTS_DET_PHY_READY)
                {
                    /* Discovery can finish before a boot-time link reaches
                     * DET=3 while connect-change interrupts are masked. Keep
                     * polling in foreground so that transition is not lost. */
                    ahci_hotplug_pending[port_num] = TRUE;
                    ahci_hotplug_quiesced[port_num] = FALSE;
                }
#endif
            }
        }
    }

    CRIT("Connected to %u AHCI device(s).\n", gSATADeviceCount);

    // Return good status if at least 1 device was detected.
    return(gSATADeviceCount > 0) ? STATUS_OK : STATUS_ERROR;
}


#define ERROR_CNT_THRESHOLD       0x2 /* Range is 0x0-0xF */
#define COUNTER_THRESHOLD         8   /* number of 8b10b errors > ERROR_CNT_THRESHOLD before port reset is issued. */
#define PORT_RESET_THRESHOLD_MS   3   /* msecs */

/** Clear the receiver-error count without waiting for SATA link recovery. */
static void ahci_clear_rx_error_count(void)
{
    MODIFY32(PxPHYCTRL(0), PPHY_CTRL_CLR_RX_8B10B_ERR_CNT,
             PPHY_CTRL_CLR_RX_8B10B_ERR_CNT);
    MODIFY32(PxPHYCTRL(0), PPHY_CTRL_CLR_RX_8B10B_ERR_CNT, 0);
}


/*****************************************************************************
 * Function: ahci_rx_error_isr
 *************************************************************************//**
 * This function handles the AHCI receive error interrupt for port 0 and 
 * resets the port or device if needed. 
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ahci_rx_error_isr(void)
{
    static UINT32_T counter = 0;
    static UINT32_T num_port_resets = 0;
    static UINT32_T t1 = 0;
    UINT32_T dev_detect_status;
    UINT32_T err_cnt;
    BOOLEAN_T media_was_ready;

    // Reset watchdog.
    wdt_reset();

    err_cnt = (READ32(PxPHYSTAT(0)) & PPHYSTAT_RX_8B10B_ERR_CNT_MASK) >> PPHYSTAT_RX_8B10B_ERR_CNT_OFFSET;

    if (err_cnt == 0)
    {
        // If LPM is enabled, this interrupt may occur with 0 error count when entering low power state.
        return;
    }

#if REMOVABLE_MEDIA_DEVICE
    if (rdx_mechanism_is_active())
    {
        /* Receiver errors have higher interrupt priority than the normal
         * connect-change handler. Mechanical removal can therefore enter
         * recovery before that handler isolates the disappearing target.
         * Keep stop/COMRESET waits out of powered travel and retain the
         * event for the foreground discovery/callback drain after settling.
         * WAIT_FOR_IO is deliberately excluded: outstanding host commands
         * still need normal recovery before the mechanism can start. */
        media_was_ready = ata_dev[0].bDeviceInitComplete;
        if ((READ32(PxSSTS(0)) & PSSTS_DET_MASK) !=
            PSSTS_DET_PHY_READY)
        {
            /* Isolation below can suppress the normal connect-change ISR.
             * Preserve its physical-removal handling before masking PxIE. */
            sata_media_link_disconnected(0U);
        }
        ahci_schedule_media_discovery(0U, TRUE);
        ahci_clear_rx_error_count();
        counter = 0;
        if (media_was_ready &&
            !ahci_callbacks_are_pending(0U))
        {
            /* Boot homing can still own an admitted medium; normal eject
             * has already withdrawn readiness and needs no ATA callback. */
            ahci_ata_cbk_queue_add(
                0U, ata_dev[0].pAtaErrorCallback);
        }
        return;
    }
#endif

    // Check device detect status.
    dev_detect_status = (READ32(PxSSTS(0)) & PSSTS_DET_MASK);

    // Ignore interface errors if in BIST mode.
    if (dev_detect_status != PSSTS_DET_PHY_OFFLINE)
    {
        CRIT("-> ahci_rx_error_isr() - err_cnt = 0x%x.\n", err_cnt);

        if (err_cnt > ERROR_CNT_THRESHOLD)
        {
            if ((rti_get_time() - t1) > PORT_RESET_THRESHOLD_MS)
            {
                t1 = rti_get_time();
                counter = 0;
            }

            counter++;
        }
        else
        {
            // Reset count.
            counter = 0;
        }

        if (counter >= COUNTER_THRESHOLD)
        {
            // Reset count.
            counter = 0;

            if ((rti_get_time() - t1) <= PORT_RESET_THRESHOLD_MS)
            {
                // Workaround for ASUS DRW-21B1ST CD/DVD Drivewhere 8b10b errors can occur 
                // while IDENTIFY DEVICE command is being executed.
                if (!ata_dev[0].bDeviceInitComplete && READ32(PxCI(0)))
                {
#if REMOVABLE_MEDIA_DEVICE
                    /* A pending discovery command in an empty bay is not a
                     * reason to reset the USB dock.  Mark the SATA target as
                     * absent; the connect-change path retries on a real
                     * PHY-ready insertion. */
                    ata_dev[0].bDeviceInitTimedOut = TRUE;
                    ahci_stop(0);
#else
                    system_reset();
#endif
                }
                else
                {
                    media_was_ready = ata_dev[0].bDeviceInitComplete;
                    ahci_stop(0);

                    // Device is not in stable state.  Issue COMRESET.
                    if (STATUS_OK != ahci_port_reset(0))
                    {
                        // Delay required to pass ASR-03 SATA compliance test.
                        msleep(100);
#if !REMOVABLE_MEDIA_DEVICE
                        // Global reset if port reset fails.
                        system_reset();
#endif
                    }

#if REMOVABLE_MEDIA_DEVICE
                    if (media_was_ready)
                    {
                        ahci_schedule_media_discovery(0U, TRUE);
                    }
#endif
                    ahci_start(0);

                    num_port_resets++;

                    CRIT("Num of port resets = %u.\n", num_port_resets);

                    if (media_was_ready &&
                        !ahci_callbacks_are_pending(0U))
                    {
                        /* Notify the transport once. Discovery must not be
                         * skipped merely because another callback already
                         * owns the queue. */
                        ahci_ata_cbk_queue_add(
                            0U, ata_dev[0].pAtaErrorCallback);
                    }
                }
            }

        } /* END: counter >= MAX_ERROR_THRESHOLD */
    } /* END: dev_detect_status != PSSTS_DET_PHY_OFFLINE */

    ahci_clear_rx_error_count();

    return;
}


