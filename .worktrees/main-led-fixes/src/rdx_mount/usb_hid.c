/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hid.c
//
// Project     : TUSB926x Firmware.
//
// Description : USB Human Interface Device class module.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//    01/15/09 - Alexis Cortes - Creation.
//    06/25/09 - Kevin Harris - Updated to fit the Boot Loader and Full FW needs.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the USB Human Interface Device class.
 *
 */

#include "usb_hid.h"
#include "gio.h"
#include "reg_io.h"
#include "sci.h"
#include "spi.h"
#include "string.h"
#include "system.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "usb_hal.h"
#include "usb_stack.h"

#define HID_REPORT_LEN  9  /* bytes */

/*Globals*/
/*wIdle_Value contains the Idle Duration and Report ID set by the USB Host.*/
UINT16_T wHID_Idle_Value;

/*wHID_Protocol_Value the Protocol Value(0 = Boot, 1 = Report) set by the USB Host*/
UINT16_T wHID_Protocol_Value;

/*Determine if the code is to be placed into RAM or ROM.*/
BOOLEAN_T RAM_or_Not_Flash;

/*Re-Programming Enabled is used to prevent accidental erasing of the Flash FW.*/
BOOLEAN_T ReProgram_Lock;

/*Total size of FW to be written in to Flash.*/
/*UINT32_T qFW_Size;*/

UINT16_T page_cnt;


/*****************************************************************************
 * Function: handle_HID_Opcodes
 *************************************************************************//**
 * This function handles HID Opcodes sent in Set Report requests.
 *
 * @param bCTRL_INT flag that indicates whether control or interrupt HID is active (0 = Control, 1 = Interrupt).
 *                
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_ERROR when request is not handled.
 *
 ****************************************************************************** 
 */

STATUS_T handle_HID_Opcodes(BOOLEAN_T bCTRL_INT) /*USB_HID_CTRL = 0x00
                                                   USB_HID_INT  = 0x01*/
{
    STATUS_T status = STATUS_OK;
    UINT32_T gio_num;
    UINT32_T dir_mask;
    UINT32_T gio_mask;
    UINT32_T val_mask;
    UINT32_T qAddress;
    UINT32_T qData;

    switch ( datapath_ram->HID_Report_Buf_OUT[0] )
    {
        case USB_HID_SETUP_DOWNLOAD_DATA:
            if ( ReProgram_Lock )
            {
                RAM_or_Not_Flash = datapath_ram->HID_Report_Buf_OUT[1]; /*RAM = 0x01 Flash = 0x00*/

                page_cnt = 0;
            }
            break;

        case USB_HID_RESET_FLASH_BURNER_DEVICE:

            //Ensure that the set_report is completed when issued through an HID Ctrl Xfer.
            if (!bCTRL_INT)
            {
                usb_hal_io_request((EP0 | ENDPT_DIRECTION_IN), NULL, 0, 0);
                while (usb_dev.ep_info_IN[EP0].pTRB->dControl & TRB_CTRL_HWO_BIT)
                {
                    msleep(1);
                }
            }

            if ( ReProgram_Lock )
            {
                usb_hal_disconnect();

                system_reset();
            }
            break;

        case USB_HID_POISON_FLASH:
            if ( ReProgram_Lock )
            {
                /*Erase SPI Flash*/
                SpiOps( OpcodeWriteEnable, 0x00000000, NULL, 0x0, 0 );
                SpiOps( OpcodeChipErase, 0x00000000, NULL, 0x0, 0 );
            }
            break;

        case USB_HID_READ_REG:
            if ( ReProgram_Lock )
            {
                qAddress = (UINT32_T)(datapath_ram->HID_Report_Buf_OUT[1] | (datapath_ram->HID_Report_Buf_OUT[2] << 8) | (datapath_ram->HID_Report_Buf_OUT[3] << 16) | (datapath_ram->HID_Report_Buf_OUT[4] << 24));
                qData = *(volatile UINT32_T *)qAddress;

                datapath_ram->HID_Report_Buf_IN[0] =  USB_HID_READ_REG;
                datapath_ram->HID_Report_Buf_IN[1] =  0x00;
                datapath_ram->HID_Report_Buf_IN[2] =  0x00;
                datapath_ram->HID_Report_Buf_IN[3] =  0x00;
                datapath_ram->HID_Report_Buf_IN[4] =  0x00;
                datapath_ram->HID_Report_Buf_IN[5] =   qData & 0x000000FF;
                datapath_ram->HID_Report_Buf_IN[6] =  (qData & 0x0000FF00) >> 8;
                datapath_ram->HID_Report_Buf_IN[7] =  (qData & 0x00FF0000) >> 16;
                datapath_ram->HID_Report_Buf_IN[8] =  (qData & 0xFF000000) >> 24;

                if ( bCTRL_INT )
                {
                    /*Interrupt HID*/
                    /*Report size is hardcode for Boot Loader*/
                    usb_hal_io_request((USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), (void*)&datapath_ram->HID_Report_Buf_IN, HID_REPORT_LEN, 0);
                }

            }
            break;

        case USB_HID_WRITE_REG:
            if ( ReProgram_Lock )
            {
                qAddress = (UINT32_T)(datapath_ram->HID_Report_Buf_OUT[1] | (datapath_ram->HID_Report_Buf_OUT[2] << 8) | (datapath_ram->HID_Report_Buf_OUT[3] << 16) | (datapath_ram->HID_Report_Buf_OUT[4] << 24));
                qData = (UINT32_T)(datapath_ram->HID_Report_Buf_OUT[5] | (datapath_ram->HID_Report_Buf_OUT[6] << 8) | (datapath_ram->HID_Report_Buf_OUT[7] << 16) | (datapath_ram->HID_Report_Buf_OUT[8] << 24));

                *(volatile UINT32_T *)qAddress = qData;
            }
            break;

/*
        case USB_HID_STATUS_UPDATE:

            status = STATUS_OK;
            break;
*/

        case USB_HID_ENABLE_REPROGRAM:
            ReProgram_Lock = datapath_ram->HID_Report_Buf_OUT[1]; /*Enable = 0x01 Disable = 0x00*/
            break;

        case USB_HID_GET_FIRMWARE_VERSION:
            datapath_ram->HID_Report_Buf_IN[0] = USB_HID_GET_FIRMWARE_VERSION;
            datapath_ram->HID_Report_Buf_IN[1] = 0x00;
            datapath_ram->HID_Report_Buf_IN[2] = 0x00;
            datapath_ram->HID_Report_Buf_IN[3] = 0x00;
            datapath_ram->HID_Report_Buf_IN[4] = 0x00;
            datapath_ram->HID_Report_Buf_IN[5] = 0x00;
            datapath_ram->HID_Report_Buf_IN[6] = 0x00;
            datapath_ram->HID_Report_Buf_IN[7] = FIRMWARE_MINOR_VERSION;
            datapath_ram->HID_Report_Buf_IN[8] = FIRMWARE_MAJOR_VERSION;

            if ( bCTRL_INT )
            {
                /*Interrupt HID*/                   
                usb_hal_io_request((USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), (void*)&datapath_ram->HID_Report_Buf_IN, HID_REPORT_LEN, 0);
            }
            break;

        case USB_HID_GET_GPIO_STATE:
            val_mask = 0;
            for (gio_num = 0; gio_num < TOTAL_NUM_GIOS; gio_num++)
            {
                val_mask |= (gio_get_state(gio_num) << gio_num);
            }
    
            dir_mask = gio_get_direction();
            gio_mask = gio_get_function();
    
            datapath_ram->HID_Report_Buf_IN[0] = USB_HID_GET_GPIO_STATE;
            datapath_ram->HID_Report_Buf_IN[1] = gio_mask & 0xFF;
            datapath_ram->HID_Report_Buf_IN[2] = (gio_mask & 0x0000FF00) >> 8;
            datapath_ram->HID_Report_Buf_IN[3] = dir_mask & 0xFF;
            datapath_ram->HID_Report_Buf_IN[4] = (dir_mask & 0x0000FF00) >> 8;
            datapath_ram->HID_Report_Buf_IN[5] = val_mask & 0xFF;
            datapath_ram->HID_Report_Buf_IN[6] = (val_mask & 0x0000FF00) >> 8;
            datapath_ram->HID_Report_Buf_IN[7] = 0;
            datapath_ram->HID_Report_Buf_IN[8] = 0;

            if ( bCTRL_INT )
            {
                /*Interrupt HID*/
                usb_hal_io_request((USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), (void*)&datapath_ram->HID_Report_Buf_IN, HID_REPORT_LEN, 0);
            }
            break;

        case USB_HID_SET_GPIO_OUTPUT:
            // Input GPIOs will be changed to outputs if they are selected by GIO mask.
            gio_mask = datapath_ram->HID_Report_Buf_OUT[1] | (datapath_ram->HID_Report_Buf_OUT[2] << 8);
            val_mask = datapath_ram->HID_Report_Buf_OUT[3] | (datapath_ram->HID_Report_Buf_OUT[4] << 8);

            for (gio_num = 0; gio_num < TOTAL_NUM_GIOS; gio_num++)
            {
                if (gio_mask & (1 << gio_num))
                {
                    if (val_mask & (1 << gio_num))
                    {
                        gio_high(gio_num);
                    }
                    else
                    {
                        gio_low(gio_num);
                    }

                    // Change GIO direction to output.
                    gio_set_direction(gio_num, GIO_DIR_OUTPUT);
                }
            }
            break;

        case USB_HID_CONFIG_GPIO_INPUT:
            gio_mask = datapath_ram->HID_Report_Buf_OUT[1] | (datapath_ram->HID_Report_Buf_OUT[2] << 8);

            for (gio_num = 0; gio_num < TOTAL_NUM_GIOS; gio_num++)
            {
                if (gio_mask & (1 << gio_num))
                {
                    gio_set_as_input(gio_num);
                }
            }           
            break;

        default:
            status = STATUS_ERROR;
            break;
    }

    return status;
}


/*****************************************************************************
 * Function: HID_EP0_OUT_Control_Callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle HID requests
 * on EP0.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void HID_EP0_OUT_Control_Callback(EP_INFO_T *ep_info)
{
    DEBUG("-> HID_EP0_OUT_Control_Callback()\n");

    if (!((USB_REQ_GET_IDLE | USB_REQ_SET_IDLE  | USB_REQ_GET_PROTOCOL | USB_REQ_SET_PROTOCOL) == usb_dev.setup_packet.bRequest))
    {
        handle_HID_Opcodes(USB_HID_CTRL);
    }

    return;
}

/*****************************************************************************
 * Function: handle_usb_get_report
 *************************************************************************//**
 * This function handles the HID Control Get Report request.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK when the request completes successfully.
 * @retval STATUS_ERROR when the report type is not handled.
 *
 ****************************************************************************** 
 */

STATUS_T handle_usb_get_report(USB_SETUP_PACKET_T* setup_packet)
{
    STATUS_T status = STATUS_OK;

    INFO("-> handle_usb_get_report()\n");

    switch ( ((setup_packet->wValue & 0xFF00) >> 8) )
    {
        case USB_REQ_GET_REPORT_INPUT:          
            usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, (void*)datapath_ram->HID_Report_Buf_IN, MIN(setup_packet->wLength, HID_REPORT_LEN));
            break;

        case USB_REQ_GET_REPORT_FEATURE:
            datapath_ram->HID_Report_Buf_IN[0] = 0x60;
            datapath_ram->HID_Report_Buf_IN[1] = 0x92;

            usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, (void*)datapath_ram->HID_Report_Buf_IN, MIN(setup_packet->wLength, 2));
            break;

        default:
            status = STATUS_ERROR;

    }

    return status;
}


/*****************************************************************************
 * Function: handle_usb_set_report
 *************************************************************************//**
 * This function handles the HID Control Set Report request.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK always.              
 *
 ****************************************************************************** 
 */

STATUS_T handle_usb_set_report(USB_SETUP_PACKET_T* setup_packet)
{
    INFO("-> handle_usb_set_report() - wLength = %u\n", setup_packet->wLength);

    /*Register an EP0 Callback to handle HID Control Transfers.*/
    usb_stack_register_ep0_OUT_data_xfer_callback(HID_EP0_OUT_Control_Callback);

    usb_hal_ep0_io_request(ENDPT_DIRECTION_OUT, (void*)datapath_ram->HID_Report_Buf_OUT, setup_packet->wLength);

    return STATUS_OK;
}


/*****************************************************************************
 * Function: handle_usb_get_idle
 *************************************************************************//**
 * This function handles the HID Get Idle request.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK always.              
 *
 ****************************************************************************** 
 */

STATUS_T handle_usb_get_idle(USB_SETUP_PACKET_T* setup_packet)
{
    INFO("-> handle_usb_get_idle()\n");

    datapath_ram->HID_Report_Buf_IN[0] = (UINT8_T)((wHID_Idle_Value & 0xFF00) >> 8);  

    usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, (void*)datapath_ram->HID_Report_Buf_IN, MIN(setup_packet->wLength, 2));

    return STATUS_OK;
}


/*****************************************************************************
 * Function: handle_usb_set_idle
 *************************************************************************//**
 * This function handles the HID Set Idle request.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK always. 
 *
 ****************************************************************************** 
 */

STATUS_T handle_usb_set_idle(USB_SETUP_PACKET_T* setup_packet)
{
    INFO("-> handle_usb_set_idle()\n");

    wHID_Idle_Value = setup_packet->wValue;

    return STATUS_OK;
}


/*****************************************************************************
 * Function: handle_usb_get_protocol
 *************************************************************************//**
 * This function handles the HID Get Protocol request.
 * For the moment the Protocol value does not actually make any 
 * changes in the HID functionality.  The Get/Set protocol simply receives the
 * Value from the Host and returns that Value when requested.  This is needed to
 * pass USB CV HID testing.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK always.              
 *
 ****************************************************************************** 
 */

STATUS_T handle_usb_get_protocol(USB_SETUP_PACKET_T* setup_packet)
{
    INFO("-> handle_usb_get_protocol()\n");

    datapath_ram->HID_Report_Buf_IN[0] = (UINT8_T)(wHID_Protocol_Value & 0x00FF);
    datapath_ram->HID_Report_Buf_IN[1] = (UINT8_T)((wHID_Protocol_Value & 0xFF00) >> 8);

    usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, (void*)datapath_ram->HID_Report_Buf_IN, MIN(setup_packet->wLength, 2));    

    return STATUS_OK;
}


/*****************************************************************************
 * Function: handle_usb_set_protocol
 *************************************************************************//**
 * This function handles the HID Set Protocol request.
 * For the moment the Protocol value does not actually make any 
 * changes in the HID functionality.  The Get/Set protocol simply receives the
 * Value from the Host and returns that Value when requested.  This is needed to
 * pass USB CV HID testing.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK always.            
 *
 ****************************************************************************** 
 */

STATUS_T handle_usb_set_protocol(USB_SETUP_PACKET_T* setup_packet)
{
    INFO("-> handle_usb_set_protocol()\n");

    wHID_Protocol_Value = setup_packet->wValue;

    return STATUS_OK;
}


/*****************************************************************************
 * Function: hid_class_request_callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle HID class requests.
 *
 * @param[in] setup_packet pointer the setup packet.
 *     
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_TIMEOUT when the EP0 I/O request times out.                             
 * @retval STATUS_ERROR when request is not handled.
 *
 ****************************************************************************** 
 */

STATUS_T hid_class_request_callback(USB_SETUP_PACKET_T* setup_packet)
{
    STATUS_T status = STATUS_ERROR;

    DEBUG("-> hid_class_request_callback()\n");

    /*Determine Request Type.*/
    switch ( setup_packet->bRequest )
    {
        case USB_REQ_GET_REPORT:
            status = handle_usb_get_report(setup_packet);
            break;

        case USB_REQ_GET_IDLE:
            status = handle_usb_get_idle(setup_packet);
            break;

        case USB_REQ_SET_REPORT:
            status = handle_usb_set_report(setup_packet);
            break;

        case USB_REQ_SET_IDLE:
            status = handle_usb_set_idle(setup_packet);
            break;

        case USB_REQ_SET_PROTOCOL:
            status = handle_usb_set_protocol(setup_packet);
            break;

        case USB_REQ_GET_PROTOCOL:
            status = handle_usb_get_protocol(setup_packet);
            break;

        default:
            DEBUG("-> hid_class_request_callback() - No matching request.\n");
            status = STATUS_ERROR;
            break;
    }

    return status;
}


/*****************************************************************************
 * Function: hid_idle
 *************************************************************************//**
 * This function sends an I/O request to recieve report data on the
 * HID interrupt-OUT endpoint.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void hid_idle(void)
{
    DEBUG("-> hid_idle()\n");

    /*Make the HID input ready for Xfer.*/
    usb_hal_io_request((USB_HID_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT),
                       (void *)datapath_ram->HID_Report_Buf_OUT,
                       usb_dev.ep_info_OUT[USB_HID_OUT_ENDPT_NUM].wMaxPktSize,
                       0);

    return;
}



/*****************************************************************************
 * Function: HID_EP1_OUT_Control_Callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle HID requests
 * on the HID interrupt-OUT endpoint.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void HID_EP1_OUT_Interrupt_callback(EP_INFO_T *ep_info)
{
    DEBUG("-> HID_EP1_OUT_Interrupt_callback()\n");

    handle_HID_Opcodes(USB_HID_INT);

    hid_idle();

    return;
}



/*****************************************************************************
 * Function: HID_EP1_IN_Control_Callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle transfer 
 * completions on the HID interrupt-IN endpoint.
 * For now this function does nothing but, it prevents the IN EP1
 * interrupt from causing a Callback to an unknown or NULL pointer.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void HID_EP1_IN_Interrupt_callback(EP_INFO_T *ep_info)
{
    INFO("-> HID_EP1_IN_Interrupt_callback()\n");

    /*If needed load next available report.*/
    return;
}


/*****************************************************************************
 * Function: hid_init
 *************************************************************************//**
 * This function intializes the HID module and registers callbacks with
 * the USB stack.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

#define REPORT_PROTOCOL     0x0001

void hid_init(void)
{
    DEBUG("-> hid_init()\n");

    /*Default Global Variables to False.*/
    ReProgram_Lock = FALSE;
    wHID_Idle_Value = 0x0000;

    /*HID Device should default to Report Protocol*/
    wHID_Protocol_Value = REPORT_PROTOCOL;

    /*Register class request callback.*/
    usb_stack_register_request_callback((USB_TYPE_CLASS | USB_RECIP_INTERFACE), hid_class_request_callback);

    /*Register data xfer callback for HID_EP1_OUT_Interrupt_callback*/
    usb_stack_register_data_xfer_callback((USB_HID_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT), HID_EP1_OUT_Interrupt_callback);

    /*Register data xfer callback for HID_EP1_IN_Interrupt_callback*/
    usb_stack_register_data_xfer_callback((USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), HID_EP1_IN_Interrupt_callback);

    return;
}
