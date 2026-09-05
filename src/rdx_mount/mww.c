/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : mww.c
//
// Project     : TUSB926x Firmware.
//
// Description : Memory Wrap Window driver
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   08/04/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the Memory Wrap Window driver.
 * 
 * The memory wrap windows allows a fixed (1, 2, 4, 8, 16, 32, or 64-KB) block of datapath RAM 
 * to appear as a larger 256-KB virtual block of memory to the USB and SATA controller DMAs. 
 * Each window has a fixed direction for data transmission.
 *  
 */

#include "mww.h"
#include "reg_io.h"
#include "sci.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "usb_hal.h"

#define MWW_STATUS_ERROR_MASK  0xF

/*****************************************************************************
 * Function: mww_error_isr
 *************************************************************************//**
 * Interrupt service routine for memory wrap window errors.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void mww_error_isr(void)
{
    UINT32_T status;

    status = READ32(MWW_STATUS_REG_OFF);

    CRIT("-> mww_error_isr()\n");
    CRIT(" status = 0x%x.\n", status);
    CRIT(" control = 0x%x.\n", READ32(MWW_CONTROL_REG_OFF));
    CRIT(" usb_AHB_addr = 0x%x.\n", READ32(MWW_USB_MASTER_ADDR_REG_OFF));
    CRIT(" sata_AHB_addr = 0x%x.\n", READ32(MWW_SATA_MASTER_ADDR_REG_OFF));

    // Clear error bits.
    WRITE32(MWW_STATUS_REG_OFF, status);

    return;
}

/*****************************************************************************
 * Function: mww_reset_rw_offsets
 *************************************************************************//**
 * This function resets the read and write offsets for the SATA-to-USB and
 * USB-to-SATA datapath RAM wrap windows. It also initializes the USB TRB
 * ring buffer pointers.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void mww_reset_rw_offsets(void)
{
    // Init ring TRB buffer pointer.
    usb_dev.trb_ring_info_IN.buffer_ptr  = (UINT8_T*)SATA_TO_USB_WRAP_WINDOW_ADDR;
    usb_dev.trb_ring_info_OUT.buffer_ptr = (UINT8_T*)USB_TO_SATA_WRAP_WINDOW_ADDR;

    // Reset SATA-to-USB datapath wrap window read & write offsets.
    WRITE32(MWWxWRTOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), 0x0);
    WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), 0x0);

    // Reset USB-to-SATA datapath wrap window read & write offsets.
    WRITE32(MWWxWRTOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), 0x0);
    WRITE32(MWWxRDOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), 0x0);

    return;
}

/*****************************************************************************
 * Function: mww_init
 *************************************************************************//**
 * This function configures the SATA-to-USB and USB-to-SATA datapath RAM 
 * wrap windows (32KB each). It also initializes the USB TRB ring buffer pointers.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void mww_init(void)
{
    // Configure SATA-to-USB datapath wrap window.  (USB IN)
    WRITE32(MWWxBLKOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), SATA_TO_USB_WRAP_WINDOW_OFFSET);
    WRITE32(MWWxBLKSIZE(SATA_TO_USB_WRAP_WINDOW_NUM), (WRAP_WINDOW_BLOCK_SIZE | MWW_DIR_SATA_TO_USB));  

    // Configure USB-to-SATA datapath wrap window.  (USB OUT)
    WRITE32(MWWxBLKOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), USB_TO_SATA_WRAP_WINDOW_OFFSET);
    WRITE32(MWWxBLKSIZE(USB_TO_SATA_WRAP_WINDOW_NUM), (WRAP_WINDOW_BLOCK_SIZE | MWW_DIR_USB_TO_SATA));  

    // Initialize ring TRB buffer pointers and wrap window Rd/Wr offsets.
    mww_reset_rw_offsets();

    return;
}

/*****************************************************************************
 * Function: mww_init_read_only
 *************************************************************************//**
 * This function configures the datapath RAM wrap window (64KB) for SATA-to-USB
 * direction use only. It also initializes the USB TRB ring (IN direction) 
 * buffer pointer.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void mww_init_read_only(void)
{
    // Init ring TRB buffer pointer.
    usb_dev.trb_ring_info_IN.buffer_ptr  = (UINT8_T*)SATA_TO_USB_WRAP_WINDOW_ADDR;

    // Configure SATA-to-USB datapath wrap window 0.  (USB IN)
    WRITE32(MWWxBLKOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), 0);
    WRITE32(MWWxBLKSIZE(SATA_TO_USB_WRAP_WINDOW_NUM), (MWW_BLK_SIZE_64KB | MWW_DIR_SATA_TO_USB));  
    WRITE32(MWWxWRTOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), 0x0);
    WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), 0x0);

    return;
}

/*****************************************************************************
 * Function: mww_init_write_only
 *************************************************************************//**
 * This function configures the datapath RAM wrap window (64KB) for USB-to-SATA
 * direction use only. It also initializes the USB TRB ring (OUT direction) 
 * buffer pointer.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void mww_init_write_only(void)
{
    // Init ring TRB buffer pointer.
    usb_dev.trb_ring_info_OUT.buffer_ptr = (UINT8_T*)USB_TO_SATA_WRAP_WINDOW_ADDR;

    // Configure USB-to-SATA datapath wrap window 1.  (USB OUT)
    WRITE32(MWWxBLKOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), 0);
    WRITE32(MWWxBLKSIZE(USB_TO_SATA_WRAP_WINDOW_NUM), (MWW_BLK_SIZE_64KB | MWW_DIR_USB_TO_SATA));  
    WRITE32(MWWxWRTOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), 0x0);
    WRITE32(MWWxRDOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), 0x0);

    return;
}

/*****************************************************************************
 * Function: mww_check_usb_interface_ready
 *************************************************************************//**
 * This function checks the USB AHB bus status.
 *
 * @param None.
 *                    
 * @retval TRUE when USB AHB Ready bit is set.
 * @retval FALSE otherwise.
 *
 ****************************************************************************** 
 */

BOOLEAN_T mww_check_usb_interface_ready(void)
{
    if ((READ32(MWW_STATUS_REG_OFF) & MWW_STATUS_USB_AHB_RDY_BIT) == MWW_STATUS_USB_AHB_RDY_BIT)
    {
        return TRUE;
    }
    else
    {
        DEBUG("-> mww_check_usb_interface_ready() - MMW status = 0x%08x.\n", READ32(MWW_STATUS_REG_OFF));        
        return FALSE;
    }
}


/*****************************************************************************
 * Function: mww_check_sata_interface_ready
 *************************************************************************//**
 * This function checks the SATA AHB bus status.
 *
 * @param None.
 *                    
 * @retval TRUE when SATA AHB Ready bit is set.
 * @retval FALSE otherwise.
 *
 ****************************************************************************** 
 */

BOOLEAN_T mww_check_sata_interface_ready(void)
{
    if ((READ32(MWW_STATUS_REG_OFF) & MWW_STATUS_SATA_AHB_RDY_BIT) == MWW_STATUS_SATA_AHB_RDY_BIT)
    {
        return TRUE;
    }
    else
    {
        DEBUG("-> mww_check_sata_interface_ready() - MMW status = 0x%08x.\n", READ32(MWW_STATUS_REG_OFF));        
        return FALSE;
    }
}

#if 0  /* BQ - do not use this function.  USB core cannot handle AHB error generated by forcing windows ready. */
void mww_force_usb_interface_ready(void)
{
    CRIT("-> mww_force_usb_interface_ready()\n");

    // Force all USB and SATA interfaces to be ready.
    WRITE32(MWW_CONTROL_REG_OFF, 0x0F);
    WRITE32(MWW_CONTROL_REG_OFF, 0x0);

    if ((READ32(MWW_STATUS_REG_OFF) & MWW_STATUS_USB_AHB_RDY_BIT) != MWW_STATUS_USB_AHB_RDY_BIT)
    {
        CRIT("@Error: USB MWW status = 0x%08x!\n", READ32(MWW_STATUS_REG_OFF));       
    }

    return;
}
#endif 

/*****************************************************************************
 * Function: mww_force_sata_interface_ready
 *************************************************************************//**
 * This function causes all SATA DMA requests for all wrap windows to be
 * completed with an error response.  It is used to recover when the DMA 
 * interface has become stalled.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void mww_force_sata_interface_ready(void)
{
    DEBUG("-> mww_force_sata_interface_ready\n");

    // Force all SATA interface windows to be ready.
    WRITE32(MWW_CONTROL_REG_OFF, 0xF0);
    WRITE32(MWW_CONTROL_REG_OFF, 0x0);

    if ((READ32(MWW_STATUS_REG_OFF) & MWW_STATUS_SATA_AHB_RDY_BIT) != MWW_STATUS_SATA_AHB_RDY_BIT)
    {
        CRIT("@Error: SATA MWW status = 0x%08x!\n", READ32(MWW_STATUS_REG_OFF));       
    }

    return;
}


#if DEBUG_LEVEL >= 1

#define NUM_WRAP_WINDOWS_IN_USE  2  

void mww_print_debug_info(void)
{
    UINT32_T window;

    CRIT("MWW status = 0x%08x.\n", READ32(MWW_STATUS_REG_OFF));
    CRIT("MWW cntl = 0x%08x.\n", READ32(MWW_CONTROL_REG_OFF));         
    CRIT("MWW usb addr = 0x%08x.\n", READ32(MWW_USB_MASTER_ADDR_REG_OFF)); 
    CRIT("MWW sata addr = 0x%08x.\n", READ32(MWW_SATA_MASTER_ADDR_REG_OFF));

    for (window = 0; window < NUM_WRAP_WINDOWS_IN_USE; window++)
    {
        CRIT("WIN%u blk offset = 0x%08x.\n", window, READ32(MWWxBLKOFFSET(window)));
        CRIT("WIN%u blk size = 0x%08x.\n",   window, READ32(MWWxBLKSIZE(window)));  
        CRIT("WIN%u wrt offset = 0x%08x.\n", window, READ32(MWWxWRTOFFSET(window)));
        CRIT("WIN%u rd offset = 0x%08x.\n",  window, READ32(MWWxRDOFFSET(window))); 
    }

    CRIT("\n");
    return;
}

#endif
