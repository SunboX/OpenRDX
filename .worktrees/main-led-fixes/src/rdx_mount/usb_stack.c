/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_stack.c
//
// Project     : TUSB926x Firmware.
//
// Description : USB stack module.
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
 * This file contains the implementation of the USB stack.
 *
 */

#include "usb_stack.h"
#include "sci.h"
#include "string.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "usb_hal.h"


/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#if USB_STACK_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif


/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

USB_STACK_FXN usb_stack_fxn;


/*****************************************************************************
 * Function: usb_stack_check_ep0_error
 *************************************************************************//**
 * This function stalls EP0 OUT if the EP0 state machine is in error state.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_check_ep0_error(void)
{
    if (usb_dev.ep0_state == EP0_STATE_ERROR)
    {
        CRIT("-> usb_stack_check_ep0_error() - STALLING EP0 OUT.\n");

        // Cancel any active transfers.
        usb_hal_cancel_io_request(EP0 | ENDPT_DIRECTION_OUT);
        usb_hal_cancel_io_request(EP0 | ENDPT_DIRECTION_IN);

        // Stall EP0 OUT.
        usb_hal_set_endpt_stall(EP0 | ENDPT_DIRECTION_OUT);
    }

    return;
}


/*****************************************************************************
 * Function: usb_stack_setup_pkt_callback
 *************************************************************************//**
 * This callback is registered with the HAL layer to handle
 * setup packets.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_setup_pkt_callback(void)
{
    DEBUG("-> usb_stack_setup_pkt_callback(): wLength = %u, bmReqType = 0x%x, bRequest = 0x%x, wValue = 0x%x, wIndex = 0x%x.\n", 
          usb_dev.setup_packet.wLength, usb_dev.setup_packet.bmRequestType, usb_dev.setup_packet.bRequest,
          usb_dev.setup_packet.wValue, usb_dev.setup_packet.wIndex);

    if (usb_dev.setup_packet.wLength == 0)
    {
        // No data stage.
        usb_dev.ep0_state = EP0_STATE_STATUS_IN;
    }
    else
    {
        // Setup appropriate EP state for request.
        if ((usb_dev.setup_packet.bmRequestType & USB_REQ_TYPE_DIR_MASK) == USB_REQ_TYPE_DEVICE_TO_HOST)
        {
            usb_dev.ep0_state = EP0_STATE_DATA_IN;
        }
        else
        {
            usb_dev.ep0_state = EP0_STATE_DATA_OUT;
        }
    }

    DEBUG("   ep0_state = %u.\n", usb_dev.ep0_state);

    // Call USB Chapter 9 request handler.
    if (usb_chap9_process_setup_pkt() != STATUS_OK)
    {
        usb_dev.ep0_state = EP0_STATE_ERROR;
    }

    usb_stack_check_ep0_error();

    return;
}


/*****************************************************************************
 * Function: usb_stack_process_ep0_IN
 *************************************************************************//**
 * This function processes IN data for EP0.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_process_ep0_IN(EP_INFO_T *ep_info)
{
    DEBUG("-> usb_stack_process_ep0_IN(): ep0_state = %u.\n", usb_dev.ep0_state);   

    if (usb_dev.ep0_state == EP0_STATE_DATA_IN)
    {
        // Check if there is more data to be transferred and if the completed transfer was successful.
        if ((ep_info->dBytesRemaining > 0) && (ep_info->dByteCount == ep_info->dXferLength))
        {
            // Continue the xfer using Normal TRB.
            usb_hal_ep0_io_continue(ENDPT_DIRECTION_IN, ep_info->pBuffer, ep_info->dBytesRemaining);
        }
        else
        {
            usb_dev.ep0_state = EP0_STATE_STATUS_OUT;   
        }
    }
    else if (usb_dev.ep0_state == EP0_STATE_STATUS_IN)
    {
        if (usb_dev.wTestMode)
        {
            // Device will remain in Test Mode until a Power On Reset is received.
            usb_hal_set_test_mode(usb_dev.wTestMode);
        }

        usb_dev.ep0_state = EP0_STATE_IDLE;
    }
    else
    {
        usb_dev.ep0_state = EP0_STATE_ERROR;
    }

    DEBUG("<- usb_stack_process_ep0_IN(): ep0_state = %u.\n", usb_dev.ep0_state);

    usb_stack_check_ep0_error();

    return;
}


/*****************************************************************************
 * Function: usb_stack_process_ep0_OUT
 *************************************************************************//**
 * This function processes OUT data for EP0.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_process_ep0_OUT(EP_INFO_T *ep_info)
{
    DEBUG("-> usb_stack_process_ep0_OUT(): ep0_state = %u.\n", usb_dev.ep0_state);   

    // Verify this data is expected in current state.
    if (usb_dev.ep0_state == EP0_STATE_DATA_OUT)
    {
        // Check if there is more data to be transferred and if the completed transfer was successful.
        if ((ep_info->dBytesRemaining > 0) && (ep_info->dByteCount == ep_info->dXferLength))
        {
            // Continue the xfer using Normal TRB.
            usb_hal_ep0_io_continue(ENDPT_DIRECTION_OUT, ep_info->pBuffer, ep_info->dBytesRemaining);
        }
        else
        {
            // Execute callback to process data.
            if (usb_stack_fxn.pEP0DataXferCallback_OUT)
            {
                usb_stack_fxn.pEP0DataXferCallback_OUT(ep_info);
            }

            // Set EP0 state to Status IN so a ZLP will be sent.
            usb_dev.ep0_state = EP0_STATE_STATUS_IN;
        }
    }
    else if (usb_dev.ep0_state == EP0_STATE_STATUS_OUT)
    {
        usb_dev.ep0_state = EP0_STATE_IDLE;
    }
    else
    {
        usb_dev.ep0_state = EP0_STATE_ERROR;
    }

    DEBUG("<- usb_stack_process_ep0_OUT(): ep0_state = %u.\n", usb_dev.ep0_state);

    usb_stack_check_ep0_error();

    return;
}


/*****************************************************************************
 * Function: usb_stack_register_ep0_OUT_data_xfer_callback
 *************************************************************************//**
 * This function is used by external modules to register a callback
 * which will executed when an EP0 OUT data transfer is completed. 
 * This function should be called prior to the issuing the EP0 OUT I/O 
 * request. The callback is responsible for verifying the amount of 
 * data recieved.
 *
 * @param[in] data_xfer_callback function pointer to data transfer complete callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_register_ep0_OUT_data_xfer_callback(void (*data_xfer_callback)(EP_INFO_T *ep_info))
{
    usb_stack_fxn.pEP0DataXferCallback_OUT = data_xfer_callback;
    return;
}


/*****************************************************************************
 * Function: usb_stack_data_xfer_callback
 *************************************************************************//**
 * This callback is registered with the USB HAL to handle data tranfer completions.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_data_xfer_callback(UINT32_T ep_num, EP_INFO_T *ep_info)
{
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        usb_stack_fxn.pDataXferCallback_OUT[ep_num & ~ENDPT_DIRECTION_MASK](ep_info);      
    }
    else
    {
        usb_stack_fxn.pDataXferCallback_IN[ep_num & ~ENDPT_DIRECTION_MASK](ep_info);      
    }

    return;
}

/*****************************************************************************
 * Function: usb_stack_pwr_mngmt_callback
 *************************************************************************//**
 * This callback is registered with the USB HAL to handle
 * power management events. 
 *
 * @param[in] pm_state USB power management state.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_pwr_mngmt_callback(eUSB_DEVICE_PM_STATE_T pm_state)
{
    UINT32_T i = 0;

    DEBUG("-> usb_stack_pwr_mngmt_callback()\n");

    // Call PM callbacks for USB class blocks.
    while ((i < MAX_NUM_PM_REQ_CALLBACKS) && (usb_stack_fxn.pPwrMngmtCallback[i] != NULL))
    {
        usb_stack_fxn.pPwrMngmtCallback[i](pm_state);
        i++;
    }

    return;
}


/*****************************************************************************
 * Function: usb_stack_register_data_xfer_callback
 *************************************************************************//**
 * This function is used by USB class blocks to register callbacks
 * for their required EPs.  Only use this function for endpoints with a 
 * static configuration (i.e. HID). Mass storage blocks share endpoints so must 
 * call UAS or BOT specific functions.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] data_xfer_callback function pointer to data transfer complete callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_register_data_xfer_callback(UINT32_T ep_num, void (*data_xfer_callback)(EP_INFO_T *ep_info))
{
    DEBUG("-> usb_stack_register_data_xfer_callback() - EP%u %s, fxn_ptr = 0x%08x.\n", 
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          (UINT32_T)data_xfer_callback);

    // Set appropriate data xfer callback in array.
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        usb_stack_fxn.pDataXferCallback_OUT[(ep_num & ~ENDPT_DIRECTION_MASK)] = data_xfer_callback;
    }
    else
    {
        usb_stack_fxn.pDataXferCallback_IN[(ep_num & ~ENDPT_DIRECTION_MASK)] = data_xfer_callback;
    }

    return;
}



/*****************************************************************************
 * Function: usb_stack_register_request_callback
 *************************************************************************//**
 * This function is used by other modules to register callbacks
 * for handling requests outside the scope of the USB Chap 9 standard requests.
 * Callbacks should return STATUS_OK if the request is properly handled.
 *
 * @param[in] request_type specifies bits [6:0] of bmRequestType.
 * @param[in] request_callback function pointer to request callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

STATUS_T usb_stack_register_request_callback(UINT8_T request_type, STATUS_T (*request_callback)(USB_SETUP_PACKET_T* setup_packet))
{
    STATUS_T status = STATUS_ERROR;
    static UINT32_T req_callback_index = 0;

    DEBUG("-> usb_stack_register_request_callback() - fxn_ptr[%u] = 0x%08x.\n", req_callback_index, 
          (UINT32_T)request_callback);

    if (req_callback_index < MAX_NUM_USB_REQUEST_CALLBACKS)
    {
        usb_stack_fxn.pRequestCallback[req_callback_index] = request_callback;
        usb_stack_fxn.registered_request_type[req_callback_index] = request_type & (USB_TYPE_MASK | USB_RECIP_MASK);
        req_callback_index++;
        status = STATUS_OK;
    }

    return status;
}

/*****************************************************************************
 * Function: usb_stack_register_PM_callback
 *************************************************************************//**
 * This function is used by other modules to register callbacks
 * for handling power management requests.  Mass storage callbacks should
 * verify that they are active interface before taking action.
 *
 * @param[in] pwr_mngmt_callback function pointer to power management callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

STATUS_T usb_stack_register_PM_callback(void (*pwr_mngmt_callback)(eUSB_DEVICE_PM_STATE_T pm_state))
{
    STATUS_T status = STATUS_ERROR;
    static UINT32_T pm_callback_index = 0;

    DEBUG("-> usb_stack_register_PM_callback() - fxn_ptr[%u] = 0x%08x.\n", pm_callback_index, (UINT32_T)pwr_mngmt_callback);

    if (pm_callback_index < MAX_NUM_PM_REQ_CALLBACKS)
    {
        usb_stack_fxn.pPwrMngmtCallback[pm_callback_index++] = pwr_mngmt_callback;
        status = STATUS_OK;
    }

    return status;
}


/*****************************************************************************
 * Function: usb_stack_register_BOT_reset_callback
 *************************************************************************//**
 * This function is used by the BOT class block to register a callback
 * for handling class-specific reset initialization and must be called after 
 * usb_stack_init().
 *
 * @param[in] reset_callback function pointer to reset callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_register_BOT_reset_callback(void (*reset_callback)(void))
{
    DEBUG("-> usb_stack_register_BOT_reset_callback() - fxn_ptr = 0x%08x.\n", (UINT32_T)reset_callback);

    usb_stack_fxn.pBOTResetCallback = reset_callback;

    return;
}


/*****************************************************************************
 * Function: usb_stack_register_UAS_reset_callback
 *************************************************************************//**
 * This function is used by the UAS class block to register a callback
 * for handling class-specific reset initialization and must be called after 
 * usb_stack_init().
 *
 * @param[in] reset_callback function pointer to reset callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_register_UAS_reset_callback(void (*reset_callback)(void))
{
    DEBUG("-> usb_stack_register_UAS_reset_callback() - fxn_ptr = 0x%08x.\n", (UINT32_T)reset_callback);

    usb_stack_fxn.pUASResetCallback = reset_callback;

    return;
}



#define USB_MS_CALLBACK_ARRAY_OFFSET 2

/*****************************************************************************
 * Function: usb_stack_register_BOT_data_xfer_callbacks
 *************************************************************************//**
 * This function is used by BOT block to register callbacks
 * for required EPs.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] data_xfer_callback function pointer to data transfer complete callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_register_BOT_data_xfer_callback(UINT32_T ep_num, void (*data_xfer_callback)(EP_INFO_T *ep_info))
{
    DEBUG("-> usb_stack_register_BOT_data_xfer_callback() - EP%u %s, fxn_ptr = 0x%08x.\n", 
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          (UINT32_T)data_xfer_callback);

    // Set appropriate data pkt callback in array.
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        usb_stack_fxn.pBOTDataXferCallback_OUT[(ep_num & ~ENDPT_DIRECTION_MASK) - USB_MS_CALLBACK_ARRAY_OFFSET] = data_xfer_callback;
    }
    else
    {
        usb_stack_fxn.pBOTDataXferCallback_IN[(ep_num & ~ENDPT_DIRECTION_MASK) - USB_MS_CALLBACK_ARRAY_OFFSET] = data_xfer_callback;
    }

    return;
}

/*****************************************************************************
 * Function: usb_stack_register_UAS_data_xfer_callbacks
 *************************************************************************//**
 * This function is used by UAS block to register callbacks
 * for required EPs.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] data_xfer_callback function pointer to data transfer complete callback.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_register_UAS_data_xfer_callback(UINT32_T ep_num, void (*data_xfer_callback)(EP_INFO_T *ep_info))
{
    DEBUG("-> usb_stack_register_UAS_data_xfer_callback() - EP%u %s, fxn_ptr = 0x%08x.\n", 
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          (UINT32_T)data_xfer_callback);

    // Set appropriate data pkt callback in array.
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        usb_stack_fxn.pUASDataXferCallback_OUT[(ep_num & ~ENDPT_DIRECTION_MASK) - USB_MS_CALLBACK_ARRAY_OFFSET] = data_xfer_callback;
    }
    else
    {
        usb_stack_fxn.pUASDataXferCallback_IN[(ep_num & ~ENDPT_DIRECTION_MASK) - USB_MS_CALLBACK_ARRAY_OFFSET] = data_xfer_callback;
    }

    return;
}


/*****************************************************************************
 * Function: usb_stack_set_UAS_interface
 *************************************************************************//**
 * This function is called when a set interface request for UAS is recieved.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_set_UAS_interface(void)
{
    UINT8_T i;

    DEBUG("-> usb_stack_set_UAS_interface()\n");

    // Program function pointers.
    for (i = 0; i < NUM_MASS_STORAGE_ENDPTS; i++)
    {
        usb_stack_fxn.pDataXferCallback_IN[i + USB_MS_CALLBACK_ARRAY_OFFSET] = usb_stack_fxn.pUASDataXferCallback_IN[i];
        usb_stack_fxn.pDataXferCallback_OUT[i + USB_MS_CALLBACK_ARRAY_OFFSET] = usb_stack_fxn.pUASDataXferCallback_OUT[i];
    }

    // Set active MSC.
    usb_dev.active_mass_storage_class = USB_MSC_UAS;

    // Configure endpoints.
    usb_hal_configure_endpts();

    // Call Idle callback.
    usb_stack_fxn.pUASResetCallback();

    return;
}

/*****************************************************************************
 * Function: usb_stack_set_BOT_interface
 *************************************************************************//**
 * This function is called when a set interface request for BOT is recieved.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_set_BOT_interface(void)
{
    UINT8_T i;

    DEBUG("-> usb_stack_set_BOT_interface()\n");

    // Program function pointers.
    for (i = 0; i < NUM_MASS_STORAGE_ENDPTS; i++)
    {
        usb_stack_fxn.pDataXferCallback_IN[i + USB_MS_CALLBACK_ARRAY_OFFSET] = usb_stack_fxn.pBOTDataXferCallback_IN[i];
        usb_stack_fxn.pDataXferCallback_OUT[i + USB_MS_CALLBACK_ARRAY_OFFSET] = usb_stack_fxn.pBOTDataXferCallback_OUT[i];
    }

    // Set active MSC.
    usb_dev.active_mass_storage_class = USB_MSC_BOT;

    // Configure endpoints.
    usb_hal_configure_endpts();

    // Call Idle callback.
    usb_stack_fxn.pBOTResetCallback();

    return;
}


/*****************************************************************************
 * Function: usb_stack_init
 *************************************************************************//**
 * This function is called by main() to intialize the USB stack.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void usb_stack_init(void)
{
    DEBUG("-> usb_stack_init()\n");

    // Clear usb dev struct.
    ti_memset(&usb_dev, 0, sizeof(usb_dev));

    // Set all function pointers to NULL.
    ti_memset(&usb_stack_fxn, 0, sizeof(usb_stack_fxn));

    // Set EP0 data xfer callbacks.
    usb_stack_register_data_xfer_callback((EP0 | ENDPT_DIRECTION_OUT), usb_stack_process_ep0_OUT);
    usb_stack_register_data_xfer_callback((EP0 | ENDPT_DIRECTION_IN), usb_stack_process_ep0_IN);

    // Set response buffer pointer.
    response_buff = (UINT8_T*)datapath_ram->ep0_buffer;

    // Process USB descriptors to extract configuration info.
    process_usb_descriptors();

    // Register vendor device requests.
    usb_vendor_init();

    // Initialize USB HAL and register stack callbacks.
    usb_hal_init(usb_stack_setup_pkt_callback, usb_stack_data_xfer_callback, usb_stack_pwr_mngmt_callback);

    return;
}





