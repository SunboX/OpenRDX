/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_stack.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for USB stack.
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
 * Header file for the USB stack.
 *
 */

#ifndef _USB_STACK_H_
#define _USB_STACK_H_

#include "usb_hal.h"
#include "tusb9260.h"
#include "tusb9260_types.h"


/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define NUM_MASS_STORAGE_ENDPTS        2  /* 2 IN, 2 OUT*/
#define MAX_NUM_PM_REQ_CALLBACKS       4
#define MAX_NUM_USB_REQUEST_CALLBACKS  4


/* Standard Requests */
#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07 /*We do not support this*/
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C /*We do not support this*/
#define USB_REQ_SET_SEL                 0x30
#define USB_REQ_SET_ISOCH_DELAY         0x31

/* Vendor Requests */
#define USB_REQ_MS_DESCRIPTOR           0x01

#define USB_REQ_TI_GET_PID              0x70
//#define USB_REQ_TI_GET_FW_VERSION       0x71
//#define USB_REQ_TI_FLASH_UNLOCK         0x72
//#define USB_REQ_TI_FLASH                0x73

/* USB Request Rypes */
#define USB_TYPE_MASK                   0x60
#define USB_TYPE_STANDARD               0x00
#define USB_TYPE_CLASS                  0x20
#define USB_TYPE_VENDOR                 0x40
#define USB_TYPE_RESERVED               0x60

/* USB Recipients */
#define USB_RECIP_MASK                  0x1F
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

/* USB Request Xfer direction */
#define USB_REQ_TYPE_DIR_MASK           0x80
#define USB_REQ_TYPE_DEVICE_TO_HOST     0x80
#define USB_REQ_TYPE_HOST_TO_DEVICE     0x00

/* Get Status Device Recipients Bit Fields */
#define USB_DS_SELF_POWERED    0x01
#define USB_DS_REMOTE_WAKEUP   0x02
#define USB_DS_U1_ENABLE       0x04
#define USB_DS_U2_ENABLE       0x08
#define USB_DS_LTM_ENABLE      0x10

/* Get Status Interface Recipients Bit Fields */
#define USB_IS_REMOTE_WAKEUP_ENABLED   0x02
#define USB_IS_REMOTE_WAKEUP_CAPABLE   0x01

/* Get Status Interface Recipients Bit Fields */
#define USB_DS_FUNCTION_REMOTE_WAKEUP_CAP  0x01
#define USB_DS_FUNCTION_REMOTE_WAKEUP      0x02

/* Get Status Endpoint Recipients Bit Fields */
#define USB_DS_HALT   0x01

/* USB endpoint Features */
#define USB_FEATURE_ENDPOINT_HALT           0x00   

/* USB device features */
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP    0x01   /* USB 2.0 only */
#define USB_FEATURE_TEST_MODE               0x02   /* USB 2.0 only */

#define USB_FEATURE_U1_ENABLE               0x30   /* USB 3.0 only */
#define USB_FEATURE_U2_ENABLE               0x31   /* USB 3.0 only */
#define USB_FEATURE_LTM_ENABLE              0x32   /* USB 3.0 only */

/* USB interface features */
#define USB_FEATURE_FUNCTION_SUSPEND        0x00   

#define USB_TEST_MODE_MASK                  0xFF00  /* USB 2.0 only */
#define USB_TEST_MODE_OFFSET                8       /* USB 2.0 only */

/* Descriptor Types  USB 2.0/3.0 spec table 9.5 */
#define USB_DT_DEVICE                       0x01
#define USB_DT_CONFIG                       0x02
#define USB_DT_STRING                       0x03
#define USB_DT_INTERFACE                    0x04
#define USB_DT_ENDPOINT                     0x05
#define USB_DT_DEVICE_QUALIFIER             0x06
#define USB_DT_OTHER_SPEED_CONFIG           0x07
#define USB_DT_INTERFACE_POWER              0x08
#define USB_DT_OTG                          0x09
#define USB_DT_DEBUG                        0x0A
#define USB_DT_INTERFACE_ASSOCIATION        0x0B
#define USB_DT_BOS                          0x0F
#define USB_DT_DEVICE_CAPABILITY            0x10
#define USB_DT_HID                          0x21
#define USB_DT_REPORT                       0x22
#define USB_DT_PHYSICAL_DESCRIPTOR          0x23   /* Not used */
#define USB_DT_PIPE_USAGE                   0x24
#define USB_DT_SUPERSPEED_ENDPT_COMPANION   0x30

/* Device capability types */
#define USB_DC_WIRELESS_USB      0x01
#define USB_DC_USB2_EXTENSION    0x02
#define USB_DC_SUPERSPEED_USB    0x03
#define USB_DC_CONTAINER_ID      0x04

/* Config descriptor bmAttribute bits */
#define USB_CD_ATTR_SELF_POWERED_BIT  0x40
#define USB_CD_ATTR_REMOTE_WAKEUP_BIT 0x20

/*----------------------------------------------------------------------------+
| Structures                                                                  |
+----------------------------------------------------------------------------*/


typedef struct _USB_STACK_FXN
{
    // Callbacks for data xfers indexed by endpoint number.
    // This array is used for all EPs.
    void (*pDataXferCallback_IN[MAX_EP_NUM + 1])(EP_INFO_T *ep_info);
    void (*pDataXferCallback_OUT[MAX_EP_NUM + 1])(EP_INFO_T *ep_info);

    // Callbacks for BOT and UAS data xfers indexed by endpoint number.
    // Will be copied into pDataXferCallback[2-3] upon set interface.
    void (*pBOTDataXferCallback_IN[NUM_MASS_STORAGE_ENDPTS])(EP_INFO_T *ep_info);  // One pointer will be unused for BOT.
    void (*pBOTDataXferCallback_OUT[NUM_MASS_STORAGE_ENDPTS])(EP_INFO_T *ep_info);  // One pointer will be unused for BOT.

    void (*pUASDataXferCallback_IN[NUM_MASS_STORAGE_ENDPTS])(EP_INFO_T *ep_info);
    void (*pUASDataXferCallback_OUT[NUM_MASS_STORAGE_ENDPTS])(EP_INFO_T *ep_info);

    // Power management callbacks.
    void (*pPwrMngmtCallback[MAX_NUM_PM_REQ_CALLBACKS])(eUSB_DEVICE_PM_STATE_T pm_state);

    void (*pBOTResetCallback)(void);
    void (*pUASResetCallback)(void);

    void (*pEP0DataXferCallback_OUT)(EP_INFO_T *ep_info);    

    // USB class request callbacks.
    STATUS_T (*pRequestCallback[MAX_NUM_USB_REQUEST_CALLBACKS])(USB_SETUP_PACKET_T* setup_packet);
    UINT8_T  registered_request_type[MAX_NUM_USB_REQUEST_CALLBACKS]; /* specifies bits [6:0] of bmRequestType */

} USB_STACK_FXN;


/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

extern USB_STACK_FXN usb_stack_fxn;
extern UINT8_T *response_buff;

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void usb_stack_register_data_xfer_callback(UINT32_T ep_num, void (*data_xfer_callback)(EP_INFO_T *ep_info));
STATUS_T usb_stack_register_request_callback(UINT8_T request_type, STATUS_T (*request_callback)(USB_SETUP_PACKET_T* setup_packet));
STATUS_T usb_stack_register_PM_callback(void (*pwr_mngmt_callback)(eUSB_DEVICE_PM_STATE_T pm_state));
void usb_stack_register_BOT_data_xfer_callback(UINT32_T ep_num, void (*data_xfer_callback)(EP_INFO_T *ep_info));
void usb_stack_register_UAS_data_xfer_callback(UINT32_T ep_num, void (*data_xfer_callback)(EP_INFO_T *ep_info));
void usb_stack_init(void);
void usb_stack_set_UAS_interface(void);
void usb_stack_set_BOT_interface(void);
void usb_stack_register_BOT_reset_callback(void (*reset_callback)(void));
void usb_stack_register_UAS_reset_callback(void (*reset_callback)(void));
void usb_stack_register_ep0_OUT_data_xfer_callback(void (*data_xfer_callback)(EP_INFO_T *ep_info));

/* For usb_chap9.c */
STATUS_T usb_chap9_process_setup_pkt(void);
void process_usb_descriptors(void);

/* For usb_vendor.c */
void usb_vendor_init(void);


#endif
