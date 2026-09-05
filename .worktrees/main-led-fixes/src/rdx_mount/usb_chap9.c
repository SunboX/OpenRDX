/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_chap9.c
//
// Project     : TUSB926x Firmware.
//
// Description : USB Chapter 9 portion of USB stack.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   05/28/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the USB Chapter 9 portion of USB stack.
 *
 */

#include "usb_stack.h"
#include "ahci.h"  // gSATADeviceCount
#include "gio.h"
#include "reg_io.h"
#include "sci.h"
#include "string.h"
#include "system.h"  // DIE ID reg offsets.
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "usb_hal.h"
#include "usb_hid.h"


/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define USE_EXTERNAL_DESCRIPTORS  0     /* This build embeds its mass-storage descriptors. */

#define MSFT_OS_DESCRIPTOR_SUPPORT  0  /* (Default = 0) Only skeleton code is present. Code must be completed to enable this feature. */

#define FORCE_USB2_UAS_DISABLE   1  /* (Default = 1) Set to 1 to override external descriptor settings and hide UAS interface for USB2. */
#define FORCE_USB3_UAS_DISABLE   1  /* (Default = 1) Set to 1 to override external descriptor settings and hide UAS interface for USB3. */

#define BOT_INTERFACE_PROTCOL_CODE   0x50
#define UAS_INTERFACE_PROTCOL_CODE   0x62


#if !USE_EXTERNAL_DESCRIPTORS

/****************** INTERNAL USB DESCRIPTOR DEFINITION *************************/ 

#define DEVICE_IS_SELF_POWERED    1     
#define REMOTE_WAKEUP_CAPABLE     0     

#define USB3_MAX_POWER_FIELD  0x00  /* Self-powered SuperSpeed descriptor value. */

#define HID_ENABLE 0      
#define BOT_ENABLE 1   /* should always be 1 */
#define UAS_ENABLE 0


#if HID_ENABLE
    #define    TOTAL_NUM_INTERFACES 0x02
#else // MSC ONLY
    #define    TOTAL_NUM_INTERFACES 0x01
#endif 

#define CONFIG_BMATTRIB_FIELD   (0x80 | (DEVICE_IS_SELF_POWERED << 6) | (REMOTE_WAKEUP_CAPABLE << 5))

#define MAX_STREAMS    0x05  /* Num of streams = 2^(val) - should match up w/ NCQ depth */

UINT8_T descriptor_data_buff[] =
{
    /* Device Descriptor */
    0x12,                       /* bLength              */
    USB_DT_DEVICE,              /* DEVICE               */
    0x00,0x03,                  /* USB 3.0              */    // FW will update this value automatically when operation at USB 2.0.
    0x00,                       /* CLASS                */
    0x00,                       /* Subclass             */
    0x00,                       /* Protocol             */
    0x09,                       /* bMaxPktSize0         */    // FW will update this value automatically when operation at USB 2.0.
    0x5A,0x1A,                  /* idVendor: Tandberg   */
    0x05,0x00,                  /* idProduct: RDX       */
    0x83,0x02,                  /* bcdDevice: 0283h     */
    0x01,                       /* iManufacturer        */
    0x02,                       /* iProduct             */
    0x03,                       /* iSerial Number       */
    0x01,                       /* One configuration    */

    /* Device Qualifier (USB 2.0 only) */
    0x0A,                               // Length of this descriptor (10 bytes)
    USB_DT_DEVICE_QUALIFIER,            // Type code of this descriptor (06h)
    0x10,0x02,                          // Release of USB spec (Rev 2.10)
    0x00,                               // Device's base class code - vendor specific
    0x00,                               // Device's sub class code
    0x00,                               // Device's protocol type code
    0x40,                               // End point 0's packet size 64 bytes
    0x01,                               // Number of configurations supported
    0x00,                               // Reserved for future use

    /* Strings */
    0x04,
    USB_DT_STRING,
    0x09, 0x04,                 /* English (U.S.) */

    /* Manufacturer: fixed eight-character fallback string. */
    (2+16),
    USB_DT_STRING,
    'T', 0, 'A', 0, 'N', 0, 'D', 0, 'B', 0, 'E', 0, 'R', 0, 'G', 0,

    /* Product: fixed 16-character field, including required padding. */
    (2+32),
    USB_DT_STRING,
    'R', 0, 'D', 0, 'X', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0,
    ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0,

    /* Fixed fallback serial. The 0xCA marker is consumed by
     * send_string_descriptor() and prevents replacement with the TI die ID. */
    (2+24),
    USB_DT_STRING,
    '0', 0xCA, '0', 0, '9', 0, '8', 0, '7', 0, '6', 0,
    '5', 0, '4', 0, '3', 0, '2', 0, '1', 0, '0', 0,

    /* BOS */
    0x05,       /* bLength */
    USB_DT_BOS,  
    0x2A, 0x00, /* wTotalLength */
    0x03,  /* bNumDeviceCaps */

    /* USB 2.0 Extension Capability */
    0x07,
    USB_DT_DEVICE_CAPABILITY,
    0x02, /* USB_DC_USB2_EXTENSION */
    0x02, 0x00, 0x00, 0x00,   /* LPM support */

    /* SuperSpeed Capability */
    0x0A,
    USB_DT_DEVICE_CAPABILITY,
    0x03, /* USB_DC_SUPERSPEED_USB */
    0x00,        // LTM not supported. BQ - check this later.
    0x0E, 0x00,  /* Full, High, Super speeds supported */
    0x01,        /* Functionality support (FS) */
    0x0A,        /* U1 exit latency < 10us */
    0x0A, 0x00,  /* Configured U2 exit latency: 10 us */

    /* Container ID */
    0x14,
    USB_DT_DEVICE_CAPABILITY,
    0x04, /* USB_DC_CONTAINER_ID */
    0x00,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,     /* Firmware will generate 128-bit UUID */
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,     /* Firmware will generate 128-bit UUID */

    /* Configuration */
    0x09,                       /* bLength              */
    USB_DT_CONFIG,              /* CONFIGURATION        */
    0x26, 0x00,                 /* wTotallength         */   // FW will update this value automatically.
    TOTAL_NUM_INTERFACES,       /* bNumInterfaces       */
    0x01,                       /* bConfigurationValue  */
    0x00,                       /* iConfiguration       */
    CONFIG_BMATTRIB_FIELD,      /* bmAttributes - rsvd (bit 7), self-powered (bit 6), remote wakeup cap (bit 5) */ 
    USB3_MAX_POWER_FIELD,       /* bMaxPower            */

#if BOT_ENABLE
    /* Interface (BOT) */
    0x09,                       /* bLength              */
    USB_DT_INTERFACE,           /* INTERFACE            */
    UMS_INTERFACE_NUM,          /* bInterfaceNumber     */
    0x00,                       /* bAlternateSetting    */
    0x02,                       /* bNumEndpoints        */
    0x08,                       /* bInterfaceClass      */
    0x06,                       /* bInterfaceSubClass   */
    BOT_INTERFACE_PROTCOL_CODE, /* bInterfaceProtocol  BOT = 0x50 */
    0x00,                       /* iInterface           */

    /* Endpoint Descriptor  : BOT Bulk-In */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x83,                       /* bEndpointAddress     */
    0x02,                       /* bmAttributes         */
    0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
    0x00,                       /* bInterval            */

    /* Endpoint Companion */
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    MAX_BURST_SIZE,  /* bMaxBurst */
    0x00,            /* MaxStreams = 0 */
    0x00, 0x00,

    /* Endpoint Descriptor  : BOT Bulk-Out */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x03,                       /* bEndpointAddress     */
    0x02,                       /* bmAttributes         */
    0x00, 0x04,                 /* wMaxPacketSize       */   // FW will update this value automatically.
    0x00,                       /* bInterval            */

    /* Endpoint Companion */
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    MAX_BURST_SIZE,  /* bMaxBurst */
    0x00,            /* MaxStreams = 0 */
    0x00, 0x00,

#endif /* BOT_ENABLE */

#if UAS_ENABLE

    /* Interface (UAS) */
    0x09,                       /* bLength              */
    USB_DT_INTERFACE,           /* INTERFACE            */
    UMS_INTERFACE_NUM,          /* bInterfaceNumber     */
    0x01,                       /* bAlternateSetting    */
    0x04,                       /* bNumEndpoints        */
    0x08,                       /* bInterfaceClass      */
    0x06,                       /* bInterfaceSubClass   */
    UAS_INTERFACE_PROTCOL_CODE, /* bInterfaceProtocol  UAS = 0x62.  */
    0x00,                       /* iInterface           */

    /* Endpoint Descriptor  : UAS Bulk-In STATUS */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x82,                       /* bEndpointAddress     */
    0x02,                       /* bmAttributes         */
    0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
    0x00,                       /* bInterval            */

    /* Endpoint Companion */
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    MAX_BURST_SIZE,         /* bMaxBurst */
    MAX_STREAMS,            /* MaxStreams */
    0x00, 0x00,

    /* Pipe Usage */
    0x04,
    USB_DT_PIPE_USAGE,
    0x02,  /* Pipe ID */
    0x00,  /* RSVD */

    /* Endpoint Descriptor  : UAS Bulk-Out COMMAND */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x02,                       /* bEndpointAddress     */
    0x02,                       /* bmAttributes         */
    0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically. 
    0x00,                       /* bInterval            */

    /* Endpoint Companion */
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    MAX_BURST_SIZE,  /* bMaxBurst */
    0x00,            /* MaxStreams = 0 */
    0x00, 0x00,

    /* Pipe Usage */
    0x04,
    USB_DT_PIPE_USAGE,
    0x01,  /* Pipe ID */
    0x00,  /* RSVD */

    /* Endpoint Descriptor  : UAS Bulk-In DATA */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x83,                       /* bEndpointAddress     */
    0x02,                       /* bmAttributes         */
    0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
    0x00,                       /* bInterval            */

    /* Endpoint Companion */
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    MAX_BURST_SIZE,  /* bMaxBurst */
    MAX_STREAMS,     /* MaxStreams */
    0x00, 0x00,

    /* Pipe Usage */
    0x04,
    USB_DT_PIPE_USAGE,
    0x03,  /* Pipe ID */
    0x00,  /* RSVD */

    /* Endpoint Descriptor  : UAS Bulk-Out DATA */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x03,                       /* bEndpointAddress     */
    0x02,                       /* bmAttributes         */
    0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
    0x00,                       /* bInterval            */

    /* Endpoint Companion */
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    MAX_BURST_SIZE,  /* bMaxBurst */
    MAX_STREAMS,     /* MaxStreams */
    0x00, 0x00,

    /* Pipe Usage */
    0x04,
    USB_DT_PIPE_USAGE,
    0x04,  /* Pipe ID */
    0x00,  /* RSVD */

#endif /* UAS_ENABLE */

#if HID_ENABLE

    /*Interface (HID)*/
    0x09,                       /* bLength              */
    USB_DT_INTERFACE,           /* INTERFACE            */
    HID_INTERFACE_NUM,          /* bInterfaceNumber     */
    0x00,                       /* bAlternateSetting    */
    0x02,                       /* bNumEndpoints        */
    0x03,                       /* bInterfaceClass      */
    0x00,                       /* bInterfaceSubClass   */
    0x00,                       /* bInterfaceProtocol   */
    0x00,                       /* iInterface           */

    /*HID Descriptor*/
    0x09,                       /* bLength              */
    USB_DT_HID,                 /* bDescriptorType      */
    0x10, 0x01,                 /* bcdHID               */
    0x00,                       /* bCountryCode         */
    0x01,                       /* bNumDescriptor       */
    0x22,                       /* bDescriptorType      */
    0x2F, 0x00,                 /* wReportDescriptorLength */

    /*HID Report Descriptor*/
    0x06, 0xB0, 0xFF,           /* Usage page (vendor defined)          */
    0x09, 0x01,                 /* Usage ID (vendor defined)            */
    0xA1, 0x01,                 /* Collection (application)             */

    /*The Input report*/
    0x09, 0x03,                 /* Usage ID - vendor defined            */
    0x15, 0x00,                 /* Logical Minimum (0)                  */
    0x26, 0xFF, 0x00,           /* Logical Maximum (255)                */
    0x75, 0x08,                 /* Report Size (8 bits)                 */
    0x95, 0x09,                 /* Report Count (9 fields)              */
    0x81, 0x02,                 /* Input (Data, Variable, Absolute)     */

    /*The Output report*/
    0x09, 0x04,                 /* Usage ID - vendor defined            */
    0x15, 0x00,                 /* Logical Minimum (0)                  */
    0x26, 0xFF, 0x00,           /* Logical Maximum (255)                */
    0x75, 0x08,                 /* Report Size (8 bits)                 */
    0x95, 0x09,                 /* Report Count (9 fields)              */
    0x91, 0x02,                 /* Output (Data, Variable, Absolute)    */

    /*The Feature report*/
    0x09, 0x05,                 /* Usage ID - vendor defined            */
    0x15, 0x00,                 /* Logical Minimum (0)                  */
    0x26, 0xFF, 0x00,           /* Logical Maximum (255)                */
    0x75, 0x08,                 /* Report Size (8 bits)                 */
    0x95, 0x02,                 /* Report Count (2 fields)              */
    0xB1, 0x02,                 /* Feature (Data, Variable, Absolute)   */

    0xC0,                       /* end collection */

    /*Endpoint Descriptor  : Burner HID Interrupt IN */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x81,                       /* bEndpointAddress     */
    0x03,                       /* bmAttributes         */
    0x09, 0x00,                 /* wMaxPacketSize       */
    0x02,                       /* bInterval            */

    /*Endpoint Companion*/
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    0x00,             /* bMaxBurst */
    0x00,             /* MaxStreams = 0 */
    0x00, 0x00,

    /*Endpoint Descriptor  : Burner HID Interrupt OUT*/
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x01,                       /* bEndpointAddress     */
    0x03,                       /* bmAttributes         */
    0x09, 0x00,                 /* wMaxPacketSize       */
    0x02,                       /* bInterval            */

    /*Endpoint Companion*/
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    0x00,  /* bMaxBurst */
    0x00,  /* MaxStreams = 0 */
    0x00, 0x00,

#endif /* HID_ENABLE */

    0x00  /* Designates the end of the descriptor space */
};

#endif  /* USE_EXTERNAL_DESCRIPTORS == 0 */


#if USE_EXTERNAL_DESCRIPTORS
    #define DESC_BASE_PTR   ((UINT8_T*)0x0800FBFF)
#else
    #define DESC_BASE_PTR   ((UINT8_T*)&descriptor_data_buff[0])
#endif

#if USB_STACK_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif

UINT8_T *response_buff;  // Points to EP0 buffer in datapath RAM.

#if MSFT_OS_DESCRIPTOR_SUPPORT

#define OS_STRING_DESC_LENGTH  0x12

UINT8_T os_string_desc[18] = 
{
    OS_STRING_DESC_LENGTH,   /* length */
    USB_DT_STRING, 
    'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0,  /* qwSignature "MSFT100" */
    USB_REQ_MS_DESCRIPTOR,  /* Vendor code */
    0x02   /* Flags - supports container ID */
};

#endif

/*****************************************************************************
 * Function: get_descriptor_ptr
 *************************************************************************//**
 * This function returns the pointer to the specified descriptor.
 *
 * @param[in] desc_type descriptor type. 
 * @param[out] data pointer to memory to store pointer to descriptor data.
 * @param[in] index zero-based index used to specify a descriptor when there are multiple descriptors of the same type.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T get_descriptor_ptr(UINT8_T desc_type, UINT8_T **data, UINT32_T index)
{
    BOOLEAN_T found = FALSE;
    STATUS_T status;
    UINT8_T *desc_ptr;
    UINT32_T length;
    UINT32_T report_length;
    UINT32_T num_found = 0;

    INFO("-> get_descriptor_ptr() - type = 0x%x, index = %u.\n", desc_type, index);

    desc_ptr = DESC_BASE_PTR;
    length = desc_ptr[0];              

    while (length > 0)
    {
        if (desc_ptr[1] == desc_type)
        {
            if (num_found++ == index)
            {
                found = TRUE;
                break;
            }
        }

        if (desc_ptr[1] == USB_DT_HID)
        {
            if (desc_type == USB_DT_REPORT)
            {
                if (num_found++ == index)
                {
                    // Advance descriptor pointer to start of HID report descriptor.
                    desc_ptr += length;
                    found = TRUE;
                    break;
                }
            }

            // Skip over HID report descriptor
            report_length = (((UINT32_T)desc_ptr[8] << 8) | desc_ptr[7]);
            desc_ptr += report_length;
        }

        // Increment descriptor pointer.
        desc_ptr += length;

        // Get length of next descriptor.
        length = desc_ptr[0]; 
    }

    if (found)
    {
        INFO("Desc_type 0x%x match found. ptr = 0x%08x.\n", desc_type, (UINT32_T)desc_ptr);
        // Set pointer to descriptor data.
        if (data != NULL)
        {
            *data = desc_ptr;
        }
        status = STATUS_OK;
    }
    else
    {
        if (data != NULL)
        {
            CRIT("@Error: No descriptor match found for type = 0x%x, idx = %u!\n", desc_type, index);
        }
        status = STATUS_ERROR;
    }

    return status;
}


/*****************************************************************************
 * Function: process_usb_descriptors
 *************************************************************************//**
 * This function extracts device configuration data from the configuration descriptor.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void process_usb_descriptors(void)
{
    UINT8_T *desc_ptr;
    UINT8_T bmAttributes;

    INFO("-> process_usb_descriptors()\n");

    // Get pointer to device descriptor.
    if (STATUS_OK == get_descriptor_ptr(USB_DT_DEVICE, &desc_ptr, 0))
    {
        // Save serial number string descriptor index.
        usb_dev.bSerialNumStringDescIndex = desc_ptr[16];
    }

    // Get pointer to config descriptor.
    if (STATUS_OK == get_descriptor_ptr(USB_DT_CONFIG, &desc_ptr, 0))
    {
        // Get value of bmAttributes field.
        bmAttributes = desc_ptr[7];

        // Check self-powered bit.
        usb_dev.bSelfPoweredCapable = (bmAttributes & USB_CD_ATTR_SELF_POWERED_BIT) ? TRUE : FALSE;

        // Check remote wakeup bit.
        usb_dev.bRemoteWakeupCapable = (bmAttributes & USB_CD_ATTR_REMOTE_WAKEUP_BIT) ? TRUE : FALSE;
    }
    else
    {
        CRIT("@Error: Config descriptor could not be processed!\n");
    }

    return;
}

/*****************************************************************************
 * Function: handle_usb_get_status_device
 *************************************************************************//**
 * This function generates a response for GET STATUS (DEVICE) request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when setup packet wIndex field is non-zero.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_status_device(void)
{
    UINT8_T response = 0;

    DEBUG("-> handle_usb_get_status_device()\n");

    if (usb_dev.setup_packet.wIndex != 0)
    {
        return STATUS_ERROR;
    }

    // Check GPIO-4 to determine current power source.
    if (gio_is_usb_device_self_powered() && usb_dev.bSelfPoweredCapable)
    {
        response |= USB_DS_SELF_POWERED;
    }

    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        if (usb_dev.bIsU1Enabled)
        {
            response |= USB_DS_U1_ENABLE;
        }

        if (usb_dev.bIsU2Enabled)
        {
            response |= USB_DS_U2_ENABLE;
        }

        if (usb_dev.bIsLTMEnabled)
        {
            response |= USB_DS_LTM_ENABLE;
        }
    }
    else /* USB 2.0 */
    {
        if (usb_dev.bRemoteWakeupCapable && usb_dev.bRemoteWakeupEnabled[0])
        {
            response |= USB_DS_REMOTE_WAKEUP;
        }
    }

    response_buff[0] = response;

    return STATUS_OK;
}

/*****************************************************************************
 * Function: handle_usb_get_status_interface
 *************************************************************************//**
 * This function generates a response for GET STATUS (INTERFACE) request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when interface number is invalid.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_status_interface(void)
{
    UINT32_T interface_num = usb_dev.setup_packet.wIndex;
    UINT8_T response = 0;

    DEBUG("-> handle_usb_get_status_interface()\n");

    // Validate interface number.
    if (interface_num > MAX_INTERFACE_NUM)
    {
        return STATUS_ERROR;
    }

    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        if (usb_dev.bRemoteWakeupEnabled[interface_num])
        {
            response |= USB_IS_REMOTE_WAKEUP_ENABLED;
        }

        // Remote wakeup is only supported for the HID interface.
        if (usb_dev.bRemoteWakeupCapable && (interface_num == HID_INTERFACE_NUM))
        {
            response |= USB_IS_REMOTE_WAKEUP_CAPABLE;
        }
    }

    response_buff[0] = response;

    return STATUS_OK;
}

/*****************************************************************************
 * Function: handle_usb_get_status_endpoint
 *************************************************************************//**
 * This function generates a response for GET STATUS (ENDPOINT) request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when endpoint number is invalid.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_status_endpoint(void)
{
    UINT8_T response = 0;
    UINT32_T ep_num = usb_dev.setup_packet.wIndex;

    DEBUG("-> handle_usb_get_status_endpoint()\n");

    // Validate endpt number.
    if ((ep_num & ~ENDPT_DIRECTION_MASK) > MAX_EP_NUM)
    {
        return STATUS_ERROR;
    }

    if (usb_hal_is_endpt_stalled(ep_num))
    {
        response |= USB_DS_HALT;
    }

    response_buff[0] = response;

    return STATUS_OK;   
}

/*****************************************************************************
 * Function: handle_usb_get_status
 *************************************************************************//**
 * This function handles a GET STATUS request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when setup packet wIndex field is non-zero and device state address,
 * or if device state is not configured, or the recipient is invalid.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_status(void)
{
    STATUS_T status;

    INFO("-> handle_usb_get_status()\n");

    // Clear two bytes in the response buffer.
    response_buff[0] = 0x00;
    response_buff[1] = 0x00;

    if (usb_dev.dev_state == USB_DEVICE_STATE_ADDRESSED)
    {
        if (usb_dev.setup_packet.wIndex != 0)
        {
            return STATUS_ERROR;
        }
    }
    else if (usb_dev.dev_state != USB_DEVICE_STATE_CONFIGURED)
    {
        return STATUS_ERROR;        
    }

    switch (usb_dev.setup_packet.bmRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_DEVICE:
            status = handle_usb_get_status_device();
            break;

        case USB_RECIP_INTERFACE:
            status = handle_usb_get_status_interface();
            break;

        case USB_RECIP_ENDPOINT:
            status = handle_usb_get_status_endpoint();
            break;

        default:
            status = STATUS_ERROR;
            break;
    }

    if (status == STATUS_OK)
    {
        // Send the 2-byte response.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, 0x2);
    }

    return status;
}


/*****************************************************************************
 * Function: handle_usb2_feature_device
 *************************************************************************//**
 * This function handles a SET/CLEAR FEATURE (DEVICE) request for USB 2.0.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when the feature request is not supported.
 *
 ******************************************************************************
 */

STATUS_T handle_usb2_feature_device(void)
{
    STATUS_T status = STATUS_NOT_SUPPORTED;

    DEBUG("-> handle_usb2_feature_device() - feature = %u.\n", usb_dev.setup_packet.wValue);

    switch (usb_dev.setup_packet.wValue)
    {
        case USB_FEATURE_DEVICE_REMOTE_WAKEUP:
            if (usb_dev.bRemoteWakeupCapable)
            {
                usb_dev.bRemoteWakeupEnabled[0] = (usb_dev.setup_packet.bRequest == USB_REQ_SET_FEATURE) ? TRUE : FALSE;
                status = STATUS_OK;  
            }
            break;

        case USB_FEATURE_TEST_MODE: 
            // Note: test mode cannot be cleared by Clear Feature request. 
            if (usb_dev.setup_packet.bRequest == USB_REQ_SET_FEATURE)
            {
                usb_dev.wTestMode = (usb_dev.setup_packet.wIndex & USB_TEST_MODE_MASK) >> USB_TEST_MODE_OFFSET;
                status = STATUS_OK;  
            }
            break;

        default:
            CRIT("@Error: Unhandled USB 2.0 feature %u.\n", usb_dev.setup_packet.wValue);
            break;
    }

    return status;
}

/*****************************************************************************
 * Function: handle_usb3_feature_device
 *************************************************************************//**
 * This function handles a SET/CLEAR FEATURE (DEVICE) for USB 3.0.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when the feature request is not supported.
 * @retval STATUS_ERROR when device is not in the configured state,
 * or setup packet wIndex is non-zero for set feature requests, or the feature is not supported.
 *
 ******************************************************************************
 */

STATUS_T handle_usb3_feature_device(void)
{
    BOOLEAN_T set_flag = (usb_dev.setup_packet.bRequest == USB_REQ_SET_FEATURE) ? TRUE : FALSE;
    STATUS_T status = STATUS_NOT_SUPPORTED;

    DEBUG("-> handle_usb3_feature_device() - feature = %u.\n", usb_dev.setup_packet.wValue);

    // Device must be in configured state and wIndex must be zero for SuperSpeed feature requests.
    if ((usb_dev.dev_state != USB_DEVICE_STATE_CONFIGURED) ||
        (usb_dev.setup_packet.wIndex != 0))
    {
        return STATUS_ERROR;
    }

    switch (usb_dev.setup_packet.wValue)
    {
        case USB_FEATURE_U1_ENABLE:
            CRIT("-> usb_hal_set_U1_initiate_enable() - %u.\n", set_flag);            
            usb_hal_set_U1_initiate_enable(set_flag);
            usb_dev.bIsU1Enabled = set_flag;
            status = STATUS_OK;
            break;

        case USB_FEATURE_U2_ENABLE:
            CRIT("-> usb_hal_set_U2_initiate_enable() - %u.\n", set_flag);                         
            usb_hal_set_U2_initiate_enable(set_flag);
            usb_dev.bIsU2Enabled = set_flag;
            status = STATUS_OK;
            break;

        case USB_FEATURE_LTM_ENABLE:
            // LTM is not supported.
            DEBUG("Warning: LTM feature enable is not supported.\n");
            usb_dev.bIsLTMEnabled = FALSE;
            break;

        default:   
            CRIT("@Error: Unhandled USB 3.0 feature %u.\n", usb_dev.setup_packet.wValue);
            break;
    }

    return status;
}

/*****************************************************************************
 * Function: handle_usb_feature_endpoint
 *************************************************************************//**
 * This function handles a SET/CLEAR FEATURE (ENDPOINT) request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when endpoint number is invalid, or the device is not 
 * in configured state for a non-EP0 request.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_feature_endpoint(void)
{
    STATUS_T status = STATUS_ERROR;
    UINT32_T ep_num = (UINT32_T)(usb_dev.setup_packet.wIndex & 0xFF);

    DEBUG("-> handle_usb_feature_endpoint()\n");

    // Validate endpt number.
    if ((ep_num & ~ENDPT_DIRECTION_MASK) > MAX_EP_NUM)
    {
        return STATUS_ERROR;
    }

    // If non-EP0, only process if device is in configured state.
    if (((ep_num & ~ENDPT_DIRECTION_MASK) > 0) && 
        (usb_dev.dev_state != USB_DEVICE_STATE_CONFIGURED))
    {
        return STATUS_ERROR;
    }

    if (usb_dev.setup_packet.wValue == USB_FEATURE_ENDPOINT_HALT)
    {
        if (usb_dev.setup_packet.bRequest == USB_REQ_SET_FEATURE)
        {
            /* Stall the referenced EP. */
            usb_hal_set_endpt_stall(ep_num);
        }
        else /* Clear feature */
        {
            /* Clear Stall for the referenced EP. */
            usb_hal_clear_endpt_stall(ep_num);

            if ((usb_dev.bBOT_PersistentStall) && (usb_dev.active_mass_storage_class == USB_MSC_BOT) && 
                (ep_num == (BOT_BULK_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT) || 
                 ep_num == (BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN)))
            {
                /* Re-Stall the referenced EP. */
                usb_hal_set_endpt_stall(ep_num);
            }
        }

        status = STATUS_OK;
    }

    return status;
}


#define LOW_POWER_SUSPEND_BIT           0x01
#define FUNCTION_REMOTE_WAKE_ENABLE_BIT 0x02

/*****************************************************************************
 * Function: handle_usb_feature_interface
 *************************************************************************//**
 * This function handles a SET/CLEAR FEATURE (INTERFACE) request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the interface number is invalid or the feature is not supported.
 * @retval STATUS_NOT_SUPPORTED when not connected USB SuperSpeed.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_feature_interface(void)
{
    STATUS_T status = STATUS_OK;
    UINT32_T suspend_options = (usb_dev.setup_packet.wIndex >> 8);
    UINT32_T interface_num = (usb_dev.setup_packet.wIndex & 0xFF);

    DEBUG("-> handle_usb_feature_interface() - num = %u, suspend_opt = 0x%x.\n",
          interface_num, suspend_options);

    // Interface feature requests are not supported for USB 2.0.
    if (usb_dev.dev_speed != USB_SUPER_SPEED)
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (interface_num > MAX_INTERFACE_NUM)
    {
        return STATUS_ERROR;
    }

    if (usb_dev.setup_packet.wValue == USB_FEATURE_FUNCTION_SUSPEND)
    {
        if (suspend_options & LOW_POWER_SUSPEND_BIT)
        {
            // Put interface into low power suspend mode.
            usb_dev.bLowPowerSuspendState[interface_num] = TRUE;
            // BQ - should we power down HDD for MSC interface?
        }
        else
        {
            // Put interface in normal operation mode.
            usb_dev.bLowPowerSuspendState[interface_num] = FALSE;
        }

        if (suspend_options & FUNCTION_REMOTE_WAKE_ENABLE_BIT)
        {
            // Enabling remote wake is only supported on the HID interface.
            if (usb_dev.bRemoteWakeupCapable && (interface_num == HID_INTERFACE_NUM))
            {
                usb_dev.bRemoteWakeupEnabled[interface_num] = TRUE;
            }
            else
            {
                status = STATUS_ERROR;
            }
        }
        else
        {
            // Clear remote wakeup enable.
            usb_dev.bRemoteWakeupEnabled[interface_num] = FALSE;          
        }
    }

    return status;
}


/*****************************************************************************
 * Function: handle_usb_feature
 *************************************************************************//**
 * This function handles a SET/CLEAR FEATURE request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when the recipient is not supported.
 * @retval STATUS_ERROR when the USB device state is not configured and setup 
 * packet wIndex field is non-zero.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_feature(void)
{
    STATUS_T status;

    INFO("-> handle_usb_feature()\n");

    if ((usb_dev.dev_state != USB_DEVICE_STATE_CONFIGURED) && (usb_dev.setup_packet.wIndex != 0))
    {
        return STATUS_ERROR;
    }

    switch (usb_dev.setup_packet.bmRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_DEVICE:
            if (usb_dev.dev_speed == USB_SUPER_SPEED)
            {
                status = handle_usb3_feature_device();
            }
            else /* USB 2.0 */
            {
                status = handle_usb2_feature_device();
            }
            break;

        case USB_RECIP_ENDPOINT:
            status = handle_usb_feature_endpoint();
            break;

        case USB_RECIP_INTERFACE:
            status = handle_usb_feature_interface();
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    return status;
}

/*****************************************************************************
 * Function: handle_usb_set_address
 *************************************************************************//**
 * This function handles a SET ADDRESS request.
 *
 * @param None.
 *
 * @retval STATUS_OK always.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_set_address(void)
{
    UINT32_T addr = usb_dev.setup_packet.wValue;

    INFO("-> handle_usb_set_address()\n");

    usb_hal_set_address(addr);

    // Set configuration number to zero.
    usb_dev.bCurrentConfigNum = 0;

    // Set device state.
    usb_dev.dev_state = (addr == 0) ? USB_DEVICE_STATE_DEFAULT : USB_DEVICE_STATE_ADDRESSED;

    return STATUS_OK;
}


/*****************************************************************************
 * Function: send_device_descriptor
 *************************************************************************//**
 * This function sends the device descriptor.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_device_descriptor(void)
{
    STATUS_T status;
    UINT8_T *desc_ptr;

    DEBUG("-> send_device_descriptor()\n");

    status = get_descriptor_ptr(USB_DT_DEVICE, &desc_ptr, 0);

    if (status == STATUS_OK)
    {
        // Copy device descriptor into datapath RAM.
        ti_memcpy(response_buff, desc_ptr, desc_ptr[0]);

        if ((usb_dev.dev_speed == USB_HIGH_SPEED) || (usb_dev.dev_speed == USB_FULL_SPEED))
        {
            // Modify USB version info to read (2.1).
            response_buff[2] = 0x10;
            response_buff[3] = 0x02;

            // Modify EP0 max pkt size field to read 64 bytes.
            response_buff[7] = 0x40;
        }

        // Send device descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, response_buff[0]));
    }

    return status;
}

#define SELF_POWERED_BIT 0x40
#define BULK_ENDPT_ATTR  0x02

/*****************************************************************************
 * Function: copy_usb2_config_descriptor
 *************************************************************************//**
 * This function copies the USB 2.0 configuration descriptor into
 * the response buffer.
 *
 * @param[in] speed USB connection speed (High or Full).
 * @param[out] desc_length pointer to memory to store length of configuration descriptor in bytes.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T copy_usb2_config_descriptor(eUSB_DEVICE_SPEED_T speed, UINT32_T *desc_length)
{
    UINT8_T *desc_ptr;
    UINT8_T *dst_ptr = response_buff;
    UINT32_T desc_type;
    UINT32_T length;
    UINT32_T total_length = 0;
    UINT32_T report_length;

    INFO("-> copy_usb2_config_descriptor() - %s speed.\n", (speed == USB_HIGH_SPEED) ? "HIGH" : (speed == USB_FULL_SPEED) ? "FULL" : "LOW");

    if (STATUS_OK != get_descriptor_ptr(USB_DT_CONFIG, &desc_ptr, 0))
    {
        return STATUS_ERROR;
    }

    // Get config descriptor length.
    length = desc_ptr[0];

    // Copy config descriptor into response buffer.
    ti_memcpy(dst_ptr, desc_ptr, length);

    // Update destination and descriptor pointers.
    dst_ptr += length;
    desc_ptr += length;
    total_length += length;

    // Get length of next descriptor.
    length = desc_ptr[0];

    while (length > 0)
    {
        // Determine descriptor type.
        desc_type = desc_ptr[1];

        // UAS is not supported for ATAPI devices.
        if ((desc_type == USB_DT_INTERFACE) && (desc_ptr[7] == UAS_INTERFACE_PROTCOL_CODE) &&
            ((ata_dev[0].bDeviceInitComplete && ata_dev[0].bPacketDevice) || 
             FORCE_USB2_UAS_DISABLE || (usb_dev.usb_core_version < 0x120a)))
        {
            // Update descriptor pointer to skip over all UAS-specific descriptors.
            desc_ptr += (4 * (7 + 6 + 4));
        }
        else if ((desc_type == USB_DT_INTERFACE) || (desc_type == USB_DT_ENDPOINT) || 
                 (desc_type == USB_DT_HID) || (desc_type == USB_DT_PIPE_USAGE))
        {
            // Copy interface, endpoint, HID, and pipe usage descriptors. 
            INFO("  copying desc_type 0x%02x  (0x%x bytes)\n", desc_type, length);

            ti_memcpy(dst_ptr, desc_ptr, length);

            if ((desc_type == USB_DT_ENDPOINT) && (dst_ptr[3] == BULK_ENDPT_ATTR))
            {
                // Update bulk endpt max packet size based on speed.
                if (speed == USB_HIGH_SPEED)
                {
                    dst_ptr[4] = 0x00;
                    dst_ptr[5] = 0x02;
                }
                else if (speed == USB_FULL_SPEED)
                {
                    dst_ptr[4] = 0x40;
                    dst_ptr[5] = 0x00;
                }
            }

            // Update total length of config desc.
            total_length += length;

            // Update destination pointer.
            dst_ptr += length;

            if (desc_type == USB_DT_HID)
            {
                // Skip over HID report descriptor
                report_length = (((UINT32_T)desc_ptr[8] << 8) | desc_ptr[7]);
                desc_ptr += report_length;
            }
        }

        // Update descriptor pointer.
        desc_ptr += length;

        // Get length of next descriptor.
        length = desc_ptr[0];
    }

    // Modify config descriptor's total length field.
    response_buff[2] = (UINT8_T)(total_length & 0xFF);
    response_buff[3] = (UINT8_T)((total_length >> 8) & 0xFF);

    // Convert SuperSpeed max power field from 8-mA to 2-mA units.  Max power draw is 500-mA for HS.
    response_buff[8] = MIN(((UINT32_T)response_buff[8] * 4), 0xFA);

    // Check GPIO-4 to determine current power source.
    if (gio_is_usb_device_self_powered() && usb_dev.bSelfPoweredCapable)
    {
        // Modify bMaxPower field if self-powered. (8mA) 
        response_buff[8] = 0x04; 
    }

    INFO(" Total length of USB 2.0 config desc = %u bytes.\n", total_length);

#if 0
    UINT8_T i;
    for (i = 0; i < total_length; i++)
    {
        CRIT("  0x%02x.\n", response_buff[i]);     
    }
#endif

    *desc_length = total_length;

    return STATUS_OK;
}


/*****************************************************************************
 * Function: copy_usb3_config_descriptor
 *************************************************************************//**
 * This function copies the USB 3.0 configuration descriptor into
 * the response buffer.
 *
 * @param[out] desc_length pointer to memory to store length of configuration descriptor in bytes.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */
STATUS_T copy_usb3_config_descriptor(UINT32_T *desc_length)
{
    UINT8_T *desc_ptr;
    UINT8_T *dst_ptr = response_buff;
    UINT32_T desc_type;
    UINT32_T length;
    UINT32_T total_length = 0;
    UINT32_T report_length;

    INFO("-> copy_usb3_config_descriptor() .\n");

    if (STATUS_OK != get_descriptor_ptr(USB_DT_CONFIG, &desc_ptr, 0))
    {
        return STATUS_ERROR;
    }

    // Get config descriptor length.
    length = desc_ptr[0];

    // Copy config descriptor into response buffer.
    ti_memcpy(dst_ptr, desc_ptr, length);

    // Update destination and descriptor pointers.
    dst_ptr += length;
    desc_ptr += length;
    total_length += length;

    // Get length of next descriptor.
    length = desc_ptr[0];

    // Copy descriptors.
    while (length > 0)
    {
        // Determine descriptor type.
        desc_type = desc_ptr[1];

        // UAS is not supported for ATAPI devices.
        if ((desc_type == USB_DT_INTERFACE) && (desc_ptr[7] == UAS_INTERFACE_PROTCOL_CODE) &&
            ((ata_dev[0].bDeviceInitComplete && ata_dev[0].bPacketDevice) || 
             FORCE_USB3_UAS_DISABLE || (usb_dev.usb_core_version < 0x120a)))
        {
            // Update descriptor pointer to skip over all UAS-specific descriptors.
            desc_ptr += (4 * (7 + 6 + 4));
        }
        else if ((desc_type == USB_DT_INTERFACE) || (desc_type == USB_DT_ENDPOINT) || 
                 (desc_type == USB_DT_SUPERSPEED_ENDPT_COMPANION) ||
                 (desc_type == USB_DT_HID) || (desc_type == USB_DT_PIPE_USAGE))
        {
            // Copy interface, endpoint, SS endpt companion, HID, and pipe usage descriptors. 
            INFO("  copying desc_type 0x%02x  (0x%x bytes)\n", desc_type, length);

            // Copy config descriptor into response buffer.
            ti_memcpy(dst_ptr, desc_ptr, length);

            if (desc_type == USB_DT_SUPERSPEED_ENDPT_COMPANION)
            {
                // Set MaxStreams attribute to match supported NCQ depth for UAS endpts.
                if (dst_ptr[3])
                {
                    dst_ptr[3] = (AHCI_NCQ_DEPTH == 32) ? 0x05 : (AHCI_NCQ_DEPTH == 16) ? 0x04 : (AHCI_NCQ_DEPTH == 8) ? 0x03 : (AHCI_NCQ_DEPTH == 4) ? 0x02 : 0x01;
                }
            }

            // Update total length of config desc.
            total_length += length;

            // Update destination pointer.
            dst_ptr += length;

            if (desc_type == USB_DT_HID)
            {
                // Skip over HID report descriptor
                report_length = (((UINT32_T)desc_ptr[8] << 8) | desc_ptr[7]);
                desc_ptr += report_length;
            }
        }

        // Update descriptor pointer.
        desc_ptr += length;

        // Get length of next descriptor.
        length = desc_ptr[0];
    }

    // Modify config descriptor's total length field.
    response_buff[2] = (UINT8_T)(total_length & 0xFF);
    response_buff[3] = (UINT8_T)((total_length >> 8) & 0xFF);

    // Check GPIO-4 to determine current power source.
    if (gio_is_usb_device_self_powered() && usb_dev.bSelfPoweredCapable)
    {
        // Modify bMaxPower field if self-powered. (8mA) 
        response_buff[8] = 0x01; 
    }

    INFO(" Total length of USB 3.0 config desc = %u bytes.\n", total_length);

#if 0
    UINT8_T i;
    for (i = 0; i < total_length; i++)
    {
        CRIT("  0x%02x.\n", response_buff[i]);     
    }
#endif

    *desc_length = total_length;

    return STATUS_OK;
}


/*****************************************************************************
 * Function: send_config_descriptor
 *************************************************************************//**
 * This function sends the configuration descriptor.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_config_descriptor(void)
{
    STATUS_T status;
    UINT32_T desc_length;

    DEBUG("-> send_config_descriptor()\n");

    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        status = copy_usb3_config_descriptor(&desc_length);
    }
    else /* USB 2.0 */
    {
        status = copy_usb2_config_descriptor(usb_dev.dev_speed, &desc_length);
    }

    if (status == STATUS_OK)
    {
        // Send device descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_length));
    }

    return status;
}

/*****************************************************************************
 * Function: send_BOS_descriptor
 *************************************************************************//**
 * This function sends the Binary Object Store (BOS) descriptor.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_BOS_descriptor(void)
{
    STATUS_T status;
    UINT8_T *desc_ptr;
    UINT8_T *bos_desc_ptr;
    UINT32_T *uuid_ptr;
    UINT32_T desc_length;
    UINT32_T die_id_high;
    UINT32_T die_id_low;
    UINT32_T index = 0;

    DEBUG("-> send_BOS_descriptor()\n");

    status = get_descriptor_ptr(USB_DT_BOS, &bos_desc_ptr, 0);

    if (status == STATUS_OK)
    {
        // Determine the BOS descriptor total length.
        desc_length = bos_desc_ptr[2] | (bos_desc_ptr[3] << 8);

        // Search device capability descriptors until container ID type is found.
        while (STATUS_OK == get_descriptor_ptr(USB_DT_DEVICE_CAPABILITY, &desc_ptr, index))
        {
            if (desc_ptr[2] == USB_DC_CONTAINER_ID)
            {
                die_id_low = READ32(DIEIDL_REG_OFF);
                die_id_high = READ32(DIEIDH_REG_OFF);

                if (die_id_low | die_id_high)
                {
                    uuid_ptr = (UINT32_T*)&desc_ptr[4];

                    // Update 128-bit UUID in container ID descriptor using Die ID.          
                    uuid_ptr[3] = die_id_low;
                    uuid_ptr[2] = die_id_high;
                    uuid_ptr[1] = die_id_low ^ ~(die_id_high);
                    uuid_ptr[0] = die_id_low & die_id_high;
                }

                break;
            }

            index++;
        }

        // Copy the BOS descriptor into the response buffer.
        ti_memcpy(response_buff, bos_desc_ptr, desc_length);

        // Send BOS descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_length));
    }

    return status;
}

/*****************************************************************************
 * Function: bcd_to_ascii
 *************************************************************************//**
 * This function converts a Binary-Coded Decimal (BCD) value to ASCII value.
 *
 * @param[in] bcd BCD value.
 *
 * @return The ASCII value of the BCD.
 *
 ******************************************************************************
 */

UINT8_T bcd_to_ascii(UINT8_T bcd)
{
    if (bcd > 9)
    {
        return(bcd + 'A' - 10);
    }
    else
    {
        return(bcd + '0');
    }
}


/*****************************************************************************
 * Function: send_string_descriptor
 *************************************************************************//**
 * This function sends a string descriptor.
 *
 * @param[in] index string descriptor index (zero-based).
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_string_descriptor(UINT8_T index)
{
    STATUS_T status;
    UINT32_T die_id_low;
    UINT32_T die_id_high;
    UINT32_T buff_offset;
    UINT32_T offset;
    UINT8_T *desc_ptr;
    UINT8_T *serial_ptr;

    DEBUG("-> send_string_descriptor() index = %u.\n", index);

    status = get_descriptor_ptr(USB_DT_STRING, &desc_ptr, index);

    if (status == STATUS_OK)
    {
        ti_memcpy(response_buff, desc_ptr, desc_ptr[0]);

        if (index == usb_dev.bSerialNumStringDescIndex)
        {
            // Set pointer to serial number string.
            serial_ptr = &response_buff[2];

            if (serial_ptr[1] == 0xCA)
            {
                // Do not overwrite serial number string with Die ID.
                serial_ptr[1] = 0;
            }
            else 
            {
                die_id_low = READ32(DIEIDL_REG_OFF);
                die_id_high = READ32(DIEIDH_REG_OFF); 

                // Check for non-zero die ID.
                if (die_id_low | die_id_high)
                {
                    // Clear first 16 characters. (32-bytes)
                    ti_memset(serial_ptr, 0, 32);
    
                    for (buff_offset = 0, offset = 0; buff_offset <= 14; buff_offset+=2, offset+=4)
                    {
                        // Fill in characters 0-7 of serial #.
                        serial_ptr[buff_offset] = bcd_to_ascii((die_id_low >> offset) & 0xF);

                        // Fill in characters 8-15 of serial #.
                        serial_ptr[buff_offset + 16] = bcd_to_ascii((die_id_high >> offset) & 0xF);
                    }
                }
            }
        }

        // Send string descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_ptr[0]));
    }
    else
    {
#if MSFT_OS_DESCRIPTOR_SUPPORT
        // Check if OS string descriptor request.
        if ((index == 0xEE) && (usb_dev.setup_packet.wLength == OS_STRING_DESC_LENGTH))
        {
            ti_memcpy(response_buff, os_string_desc, OS_STRING_DESC_LENGTH);

            // Send string descriptor.
            usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, OS_STRING_DESC_LENGTH));

            status = STATUS_OK;
        }
#endif
    }

    return status;
}

/*****************************************************************************
 * Function: send_device_qualifier_descriptor
 *************************************************************************//**
 * This function sends the device qualifier descriptor for USB 2.0. 
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when connected at USB SuperSpeed.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_device_qualifier_descriptor(void)
{
    STATUS_T status;
    UINT8_T *desc_ptr;

    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        return STATUS_NOT_SUPPORTED;
    }

    DEBUG("-> send_device_qualifier_descriptor()\n");

    status = get_descriptor_ptr(USB_DT_DEVICE_QUALIFIER, &desc_ptr, 0);

    if (status == STATUS_OK)
    {
        ti_memcpy(response_buff, desc_ptr, desc_ptr[0]);

        // Send descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_ptr[0]));
    }

    return status;
}

/*****************************************************************************
 * Function: send_other_speed_config_descriptor
 *************************************************************************//**
 * This function sends the other speed configuration descriptor for USB 2.0.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when connected at USB SuperSpeed.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_other_speed_config_descriptor(void)
{
    STATUS_T status = STATUS_ERROR;
    UINT32_T desc_length;

    DEBUG("-> send_other_speed_config_descriptor()\n");

    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        return STATUS_NOT_SUPPORTED;
    }

    // Copy other speed config descriptor.
    if (usb_dev.dev_speed == USB_HIGH_SPEED)
    {
        status = copy_usb2_config_descriptor(USB_FULL_SPEED, &desc_length);
    }
    else if (usb_dev.dev_speed == USB_FULL_SPEED)
    {
        status = copy_usb2_config_descriptor(USB_HIGH_SPEED, &desc_length);
    }

    if (status == STATUS_OK)
    {
        // Change descriptor type to "Other Speed".
        response_buff[1] = USB_DT_OTHER_SPEED_CONFIG;

        // Send descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_length));
    }

    return status;
}

/*****************************************************************************
 * Function: send_HID_report_descriptor
 *************************************************************************//**
 * This function sends the HID report descriptor. 
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_HID_report_descriptor(void)
{
    STATUS_T status;
    UINT8_T *desc_ptr;
    UINT32_T desc_length;

    // Get pointer to HID descriptor.
    status = get_descriptor_ptr(USB_DT_HID, &desc_ptr, 0);

    if (status == STATUS_OK)
    {
        // Get total length of HID report descriptors.
        desc_length = desc_ptr[7] | (desc_ptr[8] << 8);

        // Get pointer to first HID report descriptor.
        status = get_descriptor_ptr(USB_DT_REPORT, &desc_ptr, 0);

        if (status == STATUS_OK)
        {
            // Copy descriptors into response buffer.
            ti_memcpy(response_buff, desc_ptr, desc_length);

            // Send HID Report Descriptors.
            usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_length));
        }
    }

    return status;
}


/*****************************************************************************
 * Function: send_HID_class_descriptor
 *************************************************************************//**
 * This function sends the HID Class descriptor. 
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T send_HID_class_descriptor(void)
{
    STATUS_T status;
    UINT8_T *desc_ptr;
    UINT32_T desc_length;

    // Get pointer to HID descriptor.
    status = get_descriptor_ptr(USB_DT_HID, &desc_ptr, 0);

    if (status == STATUS_OK)
    {
        // Get length of HID class descriptor.
        desc_length = desc_ptr[0];

        // Copy descriptor into response buffer.
        ti_memcpy(response_buff, desc_ptr, desc_length);

        // Send HID class Descriptors.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, MIN(usb_dev.setup_packet.wLength, desc_length));
    }

    return status;
}


/*****************************************************************************
 * Function: handle_usb_get_descriptor
 *************************************************************************//**
 * This function handles a GET DESCRIPTOR request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when descriptor type is not supported.
 * @retval STATUS_ERROR when the descriptor cannot be found.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_descriptor(void)
{
    STATUS_T status = STATUS_OK;

    INFO("-> handle_usb_get_descriptor()\n");

    switch ( usb_dev.setup_packet.wValue >> 8 )
    {
        case USB_DT_DEVICE:
            status = send_device_descriptor();
            break;

        case USB_DT_CONFIG:
            status = send_config_descriptor();
            break;

        case USB_DT_STRING:
            status = send_string_descriptor(usb_dev.setup_packet.wValue & 0xFF);
            break;

        case USB_DT_DEVICE_QUALIFIER:  /* USB 2.0 only */
            status = send_device_qualifier_descriptor();
            break;

        case USB_DT_OTHER_SPEED_CONFIG:  /* USB 2.0 only */
            status = send_other_speed_config_descriptor();
            break;

        case USB_DT_BOS:  /* USB 2.1 & 3.0 */
            status = send_BOS_descriptor();
            break;

        case USB_DT_REPORT:
            status = send_HID_report_descriptor();
            break;

        case USB_DT_HID:
            status = send_HID_class_descriptor();
            break;

        default:
            CRIT("@Error: Descriptor type %u not supported!\n", (usb_dev.setup_packet.wValue >> 8));
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    return status;
}


/*****************************************************************************
 * Function: handle_usb_get_configuration
 *************************************************************************//**
 * This function handles a GET CONFIGURATION request.
 *
 * @param None.
 *
 * @retval STATUS_OK always.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_configuration(void)
{
    DEBUG("-> handle_usb_get_configuration()\n");

    response_buff[0] = usb_dev.bCurrentConfigNum;

    // Send descriptor.
    usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, 0x1);

    return STATUS_OK;
}


#define MAX_CONFIG_VALUE 1   // We only support one configuration.

/*****************************************************************************
 * Function: handle_usb_set_configuration
 *************************************************************************//**
 * This function handles a SET CONFIGURATION request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when configuration value is invalid or device state is not addressed or configured.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_set_configuration(void)
{
    STATUS_T status = STATUS_ERROR;
    UINT32_T config_val = (UINT32_T)(usb_dev.setup_packet.wValue & 0xFF);
    UINT32_T i;

    CRIT("-> handle_usb_set_configuration() - val = %u.\n", config_val);

    if ((usb_dev.dev_state >= USB_DEVICE_STATE_ADDRESSED) && (config_val <= MAX_CONFIG_VALUE))
    {
        if (config_val == 0)
        {
            usb_dev.dev_state = USB_DEVICE_STATE_ADDRESSED;
        }
        else
        {
            // Set BOT interface as the default.
            usb_stack_set_BOT_interface();

            // Ready the HID interface if the device is HID-enabled.
            if (STATUS_OK == get_descriptor_ptr(USB_DT_HID, NULL, 0))
            {
                // Make the HID input ready for Xfer.
                hid_idle();
            }

            if (!gio_is_sata_device_powered())
            {
                // Turn on SATA device power.
                gio_sata_device_power_enable(TRUE);
            }

            // Set device state to Configured.
            usb_dev.dev_state = USB_DEVICE_STATE_CONFIGURED;
        }

        usb_dev.bCurrentConfigNum = config_val;

        // Initialize alternate interface setting numbers to zero.
        for (i = 0; i < (MAX_INTERFACE_NUM + 1); i++)
        {
            usb_dev.bCurrentInterfaceAltSetting[i] = 0;
        }

        status = STATUS_OK;
    }

    return status;
}

/*****************************************************************************
 * Function: handle_usb_set_interface
 *************************************************************************//**
 * This function handles a SET INTERFACE request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when interface value is invalid or device state is not configured.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_set_interface(void)
{
    STATUS_T status = STATUS_ERROR;
    UINT32_T interface_num = usb_dev.setup_packet.wIndex;
    UINT32_T alt_setting = usb_dev.setup_packet.wValue;

    CRIT("-> handle_usb_set_interface() - num = %u, alt = %u.\n", interface_num, alt_setting);

    if (usb_dev.dev_state == USB_DEVICE_STATE_CONFIGURED)
    {
        // Verify interface is valid.
        if (interface_num <= MAX_INTERFACE_NUM)
        {
            if ((interface_num == UMS_INTERFACE_NUM) && (alt_setting <= UMS_MAX_ALT_SETTING_NUM))
            {
                if (alt_setting == UMS_BOT_ALT_SETTING_NUM)
                {
                    usb_stack_set_BOT_interface();
                }
                else if (alt_setting == UMS_UAS_ALT_SETTING_NUM)
                {
                    usb_stack_set_UAS_interface();
                }

                // All I/O requests are cancelled during endpt configuration when setting BOT/UAS interface
                // so we need to reconfigure HID I/O request if device is HID enabled. 
                if (STATUS_OK == get_descriptor_ptr(USB_DT_HID, NULL, 0))
                {
                    // Make the HID input ready for Xfer.
                    hid_idle();
                }

                // Store alternate interface setting.
                usb_dev.bCurrentInterfaceAltSetting[interface_num] = alt_setting;
                status = STATUS_OK;
            }
            else if ((interface_num == HID_INTERFACE_NUM) && (alt_setting == 0))
            {
                // Store alternate interface setting.
                // No alternates are supported for the HID interface.
                usb_dev.bCurrentInterfaceAltSetting[interface_num] = alt_setting;
                status = STATUS_OK;
            }
        }
    }

    return status;
}

/*****************************************************************************
 * Function: handle_usb_get_interface
 *************************************************************************//**
 * This function sends a response to a GET INTERFACE request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when interface value is invalid or device state is not configured.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_interface(void)
{
    STATUS_T status = STATUS_ERROR;
    UINT32_T interface_num = usb_dev.setup_packet.wIndex;

    DEBUG("-> handle_usb_get_interface() - num = %u\n", interface_num);

    if (usb_dev.dev_state != USB_DEVICE_STATE_CONFIGURED)
    {
        return STATUS_ERROR;
    }

    if (interface_num <= MAX_INTERFACE_NUM)
    {
        response_buff[0] = usb_dev.bCurrentInterfaceAltSetting[interface_num];   

        // Send descriptor.
        usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, response_buff, 0x1);

        status = STATUS_OK;
    }

    return status;
}

/*****************************************************************************
 * Function: usb_chap9_set_sel_data_callback
 *************************************************************************//**
 * This function handles the data for a SET SEL request.
 *
 * @param ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_chap9_set_sel_data_callback(EP_INFO_T *ep_info)
{
    // Use U2PEL to set system exit latency params.
    usb_hal_set_sel((datapath_ram->ep0_buffer[4] << 8) | datapath_ram->ep0_buffer[5]);
    return;
}


/*****************************************************************************
 * Function: handle_usb_set_sel
 *************************************************************************//**
 * This function handles a SET SEL request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when device is not in addressed or configured state.
 * @retval STATUS_NOT_SUPPORTED when device is not connected at SuperSpeed.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_set_sel(void)
{
    STATUS_T status = STATUS_ERROR;

    if (usb_dev.dev_state >= USB_DEVICE_STATE_ADDRESSED)
    {
        if (usb_dev.dev_speed == USB_SUPER_SPEED)
        {
            // Register callback to handle SET SEL data.
            usb_stack_register_ep0_OUT_data_xfer_callback(usb_chap9_set_sel_data_callback);

            // Setup to receive the packet containing the latency values.
            usb_hal_ep0_io_request(ENDPT_DIRECTION_OUT, (void*)datapath_ram->ep0_buffer, usb_dev.setup_packet.wLength);

            status = STATUS_OK;
        }
        else
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    return status;
}

#if MSFT_OS_DESCRIPTOR_SUPPORT

/*****************************************************************************
 * Function: handle_usb_get_ms_descriptor
 *************************************************************************//**
 * This function sends a response to a GET MICROSOFT DESCRIPTOR request.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when descriptor is not found.
 *
 ******************************************************************************
 */

STATUS_T handle_usb_get_ms_descriptor(void)
{
    UINT32_T interface_num = (usb_dev.setup_packet.wValue & 0xFF00) >> 8;
    UINT32_T page_num = usb_dev.setup_packet.wValue & 0xFF;
    UINT32_T feature_index = usb_dev.setup_packet.wIndex;

    CRIT("-> handle_usb_get_ms_descriptor() - interface_num = %u, page_num = %u, feature_index = %u.\n",
         interface_num, page_num, feature_index);

    // BQ - need to finish this later.
    return STATUS_ERROR;
}

#endif 

/*****************************************************************************
 * Function: usb_chap9_handle_standard_request
 *************************************************************************//**
 * This function handles standard device requests.
 *
 * @param[in] request value of the bRequest field from the setup packet.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when the request is not supported.
 * @retval STATUS_ERROR when the request is invalid.
 *
 ******************************************************************************
 */

STATUS_T usb_chap9_handle_standard_request(UINT8_T request)
{
    STATUS_T status;

    switch (request)
    {
        case USB_REQ_GET_STATUS:
            status = handle_usb_get_status();
            break;

        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE:
            status = handle_usb_feature();
            break;

        case USB_REQ_SET_ADDRESS:
            status = handle_usb_set_address();
            break;

        case USB_REQ_GET_DESCRIPTOR:
            status = handle_usb_get_descriptor();
            break;

        case USB_REQ_SET_DESCRIPTOR:
            /* We do not support this */
            status = STATUS_NOT_SUPPORTED;
            break;

        case USB_REQ_GET_CONFIGURATION:
            status = handle_usb_get_configuration();
            break;

        case USB_REQ_SET_CONFIGURATION:
            status = handle_usb_set_configuration();
            break;

        case USB_REQ_GET_INTERFACE:
            status = handle_usb_get_interface();
            break;

        case USB_REQ_SET_INTERFACE:
            status = handle_usb_set_interface();
            break;

        case USB_REQ_SYNCH_FRAME:
            /* We do not support this since we have no isochronous endpoints */
            status = STATUS_NOT_SUPPORTED;
            break;

        case USB_REQ_SET_ISOCH_DELAY:   /* USB 3.0 only */
            /* Per spec, this request cannot be STALLed even though we have no isochronous endpoints */
            status = (usb_dev.dev_speed == USB_SUPER_SPEED) ? STATUS_OK : STATUS_NOT_SUPPORTED;
            break;

        case USB_REQ_SET_SEL:   /* USB 3.0 only */
            status = handle_usb_set_sel();
            break;

        default:
            status = STATUS_ERROR;
            CRIT("@Error: Invalid standard request = %u.\n", usb_dev.setup_packet.bRequest);
            break;
    }

    return status;
}


/*****************************************************************************
 * Function: usb_chap9_process_setup_pkt
 *************************************************************************//**
 * This function processes a setup packet (USB device request).
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_NOT_SUPPORTED when the setup packet is not supported.
 * @retval STATUS_ERROR when there is an error processing the setup packet.
 *
 ******************************************************************************
 */

STATUS_T usb_chap9_process_setup_pkt(void)
{
    STATUS_T status = STATUS_NOT_SUPPORTED;
    UINT32_T i = 0;

    INFO("-> usb_chap9_process_setup_pkt()\n");

    /* Determine Request Type. */
    switch (usb_dev.setup_packet.bmRequestType & USB_TYPE_MASK)
    {
        case USB_TYPE_STANDARD:
            status = usb_chap9_handle_standard_request(usb_dev.setup_packet.bRequest);
            break;

        case USB_TYPE_CLASS:
            // Class request are all handled with registered callbacks.
            break;

        case USB_TYPE_VENDOR:
#if MSFT_OS_DESCRIPTOR_SUPPORT
            if (usb_dev.setup_packet.bRequest == USB_REQ_MS_DESCRIPTOR)
            {
                status = handle_usb_get_ms_descriptor();
            }
#endif
            break;

        default:
            status = STATUS_ERROR;
            CRIT("@Error: USB request type 0x%x is invalid.\n", usb_dev.setup_packet.bmRequestType & USB_TYPE_MASK);
            break;
    }

    if (status != STATUS_OK)
    {
        // Try to handle request with externally registered callbacks.
        while ((status != STATUS_OK) && (i < MAX_NUM_USB_REQUEST_CALLBACKS) && 
               (usb_stack_fxn.pRequestCallback[i] != NULL))
        {
            if (usb_stack_fxn.registered_request_type[i] == (usb_dev.setup_packet.bmRequestType & (USB_TYPE_MASK | USB_RECIP_MASK)))
            {
                status = usb_stack_fxn.pRequestCallback[i](&usb_dev.setup_packet);
            }

            i++;
        }
    }

    if (status != STATUS_OK)
    {
        if (status == STATUS_NOT_SUPPORTED)
        {
            CRIT("@Warning: Request is not supported:\n");
        }
        else
        {
            CRIT("@Error: Processing setup pkt failed!\n");
        }

        CRIT("  Setup Pkt: wLength = %u, bmReqType = 0x%02x, bRequest = 0x%02x, wValue = 0x%04x, wIndex = 0x%04x.\n", 
             usb_dev.setup_packet.wLength, usb_dev.setup_packet.bmRequestType, usb_dev.setup_packet.bRequest,
             usb_dev.setup_packet.wValue, usb_dev.setup_packet.wIndex);
    }

    return status;
}
