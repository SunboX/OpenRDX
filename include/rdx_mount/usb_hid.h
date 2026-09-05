/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

/*****************************************************************************
* Filename    : usb_hid.h
*
* Project     : TUSB926x Boot Loader.
*
* Description : HID Init Opcodes Defines.
*
*   (C) Copyright 2009 by Texas Instruments Incorporated.
*   All rights reserved.
*
* Revision History:
*   MM/DD/YY
*    01/15/09 - Alexis Cortes - Creation.
*    06/25/09 - Kevin Harris - Updated to fit the Boot Loader and Full FW needs.
*
*****************************************************************************/

/*! @file
 * 
 * Header file for the USB Human Interface Device class module.
 *  
 */

#ifndef __USB_HID_H__
#define __USB_HID_H__

#define USB_HID_CTRL                        FALSE
#define USB_HID_INT                         TRUE

// HID CLASS Requests
#define USB_REQ_GET_REPORT                  0x01
#define USB_REQ_GET_IDLE                    0x02
#define USB_REQ_GET_PROTOCOL                0x03
#define USB_REQ_SET_REPORT                  0x09
#define USB_REQ_SET_IDLE                    0x0A
#define USB_REQ_SET_PROTOCOL                0x0B

#define USB_REQ_GET_REPORT_INPUT            0x01
#define USB_REQ_GET_REPORT_FEATURE          0x03

// HID Report IDs
#define USB_HID_SETUP_DOWNLOAD_DATA         0x01
#define USB_HID_RESET_FLASH_BURNER_DEVICE   0x02
#define USB_HID_POISON_FLASH                0x03
#define USB_HID_READ_REG                    0x04
#define USB_HID_WRITE_REG                   0x05
#define USB_HID_STATUS_UPDATE               0x06
#define USB_HID_ENABLE_REPROGRAM            0x07
#define USB_HID_GET_FIRMWARE_VERSION        0x09

#define USB_HID_GET_GPIO_STATE              0x0A
#define USB_HID_SET_GPIO_OUTPUT             0x0B
#define USB_HID_CONFIG_GPIO_INPUT           0x0C


/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void hid_init(void); 
void hid_idle(void);
                                           

#if UNIT_TEST
void HID_EP1_OUT_Interrupt_callback(EP_INFO_T *ep_info);
#endif


#endif

