/*
 * SPDX-FileCopyrightText: 2010 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_vendor.c
//
// Project     : TUSB926x Firmware.
//
// Description : Defines USB vendor type device request callbacks that are registered 
// with the USB stack module.
//
//   (C) Copyright 2010 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   03/25/10 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of vendor type device requests for the USB stack.
 *
 */

#include "usb_stack.h"
#include "sci.h"
#include "spi.h"
#include "system.h"
#include "usb_hal.h"
#include "tusb9260.h"
#include "tusb9260_types.h"


#define TI_VENDOR_REQUEST_INDEX_FIELD   0x55
#define TI_VENDOR_REQUEST_VALUE_FIELD   0x37

/*****************************************************************************
 * Function: vendor_request_callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle 
 * vendor requests.
 *
 * @param[in] setup_packet pointer to setup packet.
 *
 * @retval STATUS_OK when succesful.
 * @retval STATUS_ERROR when request is not handled.
 *
 ****************************************************************************** 
 */

STATUS_T vendor_request_callback(USB_SETUP_PACKET_T* setup_packet)
{
    STATUS_T status = STATUS_ERROR;

    DEBUG("-> vendor_request_callback()\n");

    if ((setup_packet->wIndex != TI_VENDOR_REQUEST_INDEX_FIELD) ||
        (setup_packet->wValue != TI_VENDOR_REQUEST_VALUE_FIELD))
    {
        return STATUS_ERROR;
    }

#if 0 
    /* These TI-specific vendor requests are obsolete since Flash Burner 
     * v2.0.0.5 was released. The code is left here as a sample to aid 
     * customers in creating their own vendor request handler.
     */

    if ((setup_packet->bmRequestType & USB_REQ_TYPE_DIR_MASK) == USB_REQ_TYPE_HOST_TO_DEVICE)
    {
        if ((setup_packet->bRequest == USB_REQ_TI_FLASH_UNLOCK) && (setup_packet->wLength == 0x00))
        {
            // Set flash unlock flag
            flash_unlocked = TRUE;

            status = STATUS_OK;
        }
        else if ((setup_packet->bRequest == USB_REQ_TI_FLASH) && (setup_packet->wLength == 0x00))
        {
            if (flash_unlocked)
            {
                /*Erase SPI Flash*/
                SpiOps( OpcodeWriteEnable, 0x00000000, NULL, 0x0 );
                SpiOps( OpcodeChipErase, 0x00000000, NULL, 0x0 );
            }

            // Send status stage before resetting system.
            usb_hal_io_request((EP0 | ENDPT_DIRECTION_IN), NULL, 0x0, 0x00);

            // Wait until packet is sent.
            while (usb_dev.ep_info_IN[EP0].pTRB->dControl & TRB_CTRL_HWO_BIT)
            {
                usleep(1);
            }

            // Reset system.
            system_reset();

            status = STATUS_OK;
        }
    }
    else 
#endif 

    if ((setup_packet->bmRequestType & USB_REQ_TYPE_DIR_MASK) == USB_REQ_TYPE_DEVICE_TO_HOST)
    {
        if ((setup_packet->bRequest == USB_REQ_TI_GET_PID) && (setup_packet->wLength == 0x02))
        {
            INFO("Get TI PID.\n");

            // Set EP0 state to Data IN.
            usb_dev.ep0_state = EP0_STATE_DATA_IN;

            /* TX Data (2 bytes) */
            datapath_ram->ep0_buffer[0] = 0x60;
            datapath_ram->ep0_buffer[1] = 0x92;
            usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, (void *)datapath_ram->ep0_buffer, 0x2);

            status = STATUS_OK;
        }
    }
    else
    {
        DEBUG("-> vendor_request_callback() - No matching request.\n");
    }

    return status;
}

/*****************************************************************************
 * Function: usb_vendor_init
 *************************************************************************//**
 * This function registers vendor type device request callbacks with the 
 * USB stack.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_vendor_init(void)
{
    DEBUG("-> usb_vendor_init()\n");

    // Register vendor request callback.
    usb_stack_register_request_callback((USB_TYPE_VENDOR | USB_RECIP_DEVICE), vendor_request_callback);

    return;
}
