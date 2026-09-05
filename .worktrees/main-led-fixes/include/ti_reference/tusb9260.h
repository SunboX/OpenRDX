/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : tusb9260.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for TUSB926x main function.
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
 * Header file for the TUSB926x main function.
 *
 */

#include "mww.h" // wrap window defines.
#include "rti.h"
#include "tusb9260_types.h"

#ifndef _TUSB9260_H_
#define _TUSB9260_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define FIRMWARE_MAJOR_VERSION 1
#define FIRMWARE_MINOR_VERSION 06

#define CPU_CLOCK_MHZ_FPGA 40
#define CPU_CLOCK_MHZ_ASIC 75

/* Please verify SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH macro in gio.h before enabling SATA device power switch control */
/* SATA device power switch control should only be enabled if the power switch is present and the end product is Bus-powered */
/* Power switch must have a strong external pull to disable power after reset */
#ifndef ENABLE_SATA_POWER_SWITCH_CONTROL
#define ENABLE_SATA_POWER_SWITCH_CONTROL 0 // (Default = 0) Set to 1 to allow firmware to control SATA drive power switch.
#endif

#define TI_PRELIM_ASIC_COMPATIBILITY 1 // (Default = 1) Set to one to enable compatibility with TUSB9260 silicon.

// Datapath RAM access LDR/STR bug work-around.
#if TI_PRELIM_ASIC_COMPATIBILITY
#define TI_PRELIM_ASIC_DPRAM_ACCESS_BUG() asm(" nop")
#else
#define TI_PRELIM_ASIC_DPRAM_ACCESS_BUG()                                                                                                  \
    {                                                                                                                                      \
    }
#endif

#define DISABLE_WRAP_WINDOW 0 // (Default = 0) Set to 1 to use store-and-forward instead of Wrap Window memory function.

/* These disables override the global debug level setting */
#define STRING_DEBUG_DISABLE 0
#define USB_HAL_DEBUG_DISABLE 0
#define USB_STACK_DEBUG_DISABLE 0
#define BOT_DEBUG_DISABLE 0
#define UAS_DEBUG_DISABLE 0
#define SCSI_DEBUG_DISABLE 0
#define AHCI_DEBUG_DISABLE 0

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#if DEBUG_LEVEL >= 1
// Variadic macro requires "--gcc" compiler switch.
#define CRIT(str, args...) kprintf("[%010d] " str, rti_get_time(), ##args)
#else
#define CRIT(str, args...)                                                                                                                 \
    {                                                                                                                                      \
    }
#endif

#if DEBUG_LEVEL >= 2
// Variadic macro requires "--gcc" compiler switch.
#define DEBUG(str, args...) kprintf("[%010d] " str, rti_get_time(), ##args)
#else
#define DEBUG(str, args...)                                                                                                                \
    {                                                                                                                                      \
    }
#endif

#if DEBUG_LEVEL >= 3
// Variadic macro requires "--gcc" compiler switch.
#define INFO(str, args...) kprintf("[%010d] " str, rti_get_time(), ##args)
#else
#define INFO(str, args...)                                                                                                                 \
    {                                                                                                                                      \
    }
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define DATAPATH_RAM_OFFSET 0xC0000000

#define SATA_TO_USB_WRAP_WINDOW_ADDR MWW0_ADDR
#define USB_TO_SATA_WRAP_WINDOW_ADDR MWW1_ADDR

#define TRB_RING_IN_SIZE 11 /* Size of TRB ring for wrap window IN xfers */
#define TRB_RING_OUT_SIZE 9 /* Size of TRB ring for wrap window OUT xfers */

#define DATAPATH_RAM_SIZE 0x14000 /* 80 KB */

#define SCSI_RESPONSE_BUFF_SIZE 640

#define NUM_EVNT_BUFFS 1
#define EVNT_BUFFER_SIZE 256 /* bytes */
#define AHCI_NUM_PORTS 1
#define AHCI_NCQ_DEPTH 32                      /* Max Streams value must be updated to correspond to this value */
#define AHCI_MAX_SCAT_GATH 8                   /* max number entries in the scatter/gather list */
#define UMS_UAS_CMD_DEPTH (AHCI_NCQ_DEPTH + 2) /* we need extra buffers to handle command when queue is full */
#define UMS_MAX_LUN 0

/* Inteface Numbers */
#define UMS_INTERFACE_NUM 0
#define HID_INTERFACE_NUM 1
#define MAX_INTERFACE_NUM 1

/* USB Mass Storage Alternate Inteface Numbers */
#define UMS_BOT_ALT_SETTING_NUM 0x00
#define UMS_UAS_ALT_SETTING_NUM 0x01
#define UMS_MAX_ALT_SETTING_NUM 0x01

/* Endpoint Number definitions */
#define USB_HID_IN_ENDPT_NUM 1  /* INTERRUPT */
#define USB_HID_OUT_ENDPT_NUM 1 /* INTERRUPT */

#define UMS_UAS_CMD_ENDPT_NUM 2    /* BULK */
#define UMS_UAS_STATUS_ENDPT_NUM 2 /* BULK */

#define UMS_UAS_DATA_IN_ENDPT_NUM 3  /* BULK */
#define UMS_UAS_DATA_OUT_ENDPT_NUM 3 /* BULK */

#define BOT_BULK_IN_ENDPT_NUM UMS_UAS_DATA_IN_ENDPT_NUM
#define BOT_BULK_OUT_ENDPT_NUM UMS_UAS_DATA_OUT_ENDPT_NUM

#define MAX_EP_NUM 3 /* the max logical endpt number used for this application */

/* Tx FIFO Numbers */
#define EP0_TX_FIFO 0                /* for physical EP 1 */
#define USB_HID_IN_ENDPT_FIFO 1      /* for physical EP 3 */
#define UMS_UAS_STATUS_ENDPT_FIFO 2  /* for physical EP 5 */
#define UMS_UAS_DATA_IN_ENDPT_FIFO 3 /* for physical EP 7 */
#define MAX_TX_FIFO_NUM 3

/*----------------------------------------------------------------------------+
| Enumerations                                                                |
+----------------------------------------------------------------------------*/

typedef enum {
    /* General */
    STATUS_OK = 0,
    STATUS_ERROR,
    STATUS_TIMEOUT,
    STATUS_NOT_SUPPORTED,
    STATUS_XFER_ACTIVE,
    STATUS_NO_DATA,

    /* AHCI SATA module */
    STATUS_ATA_DEVICE_FAULT,
    STATUS_ATA_CMD_SLOT_BUSY,

    /* SCSI module */
    STATUS_SCSI_RESPONSE_PENDING,
    STATUS_SCSI_RESPONSE_READY,
    STATUS_SCSI_INVALID_CMD,
    STATUS_SCSI_INVALID_CMD_FIELD,
    STATUS_SCSI_INVALID_ADDRESS_RANGE,
    STATUS_SCSI_INTERNAL_TARGET_FAILURE,
    STATUS_SCSI_LOGICAL_UNIT_NOT_READY,
    STATUS_SCSI_MEDIUM_CHANGE,
    STATUS_SCSI_SELF_TEST_FAILURE,
    STATUS_SCSI_ATA_ERROR, /* error code when no status should be sent to host by mass storage layer because an ATA error callback will handle status */
    STATUS_INQUIRY_DATA_CHANGED
} STATUS_T;

/*----------------------------------------------------------------------------+
| Structures                                                                  |
+----------------------------------------------------------------------------*/

typedef volatile struct _TRANSFER_REQUEST_BLOCK_T /* 16-bytes */
{
    /* DWORD 1 */
    UINT32_T dBufferPtrLow; /* Pointer to data buffer - low 32-bit addr */

    /* DWORD 2 */
    UINT32_T dBufferPtrHigh; /* Pointer to data buffer - high 32-bit addr for 64-bit addressing */

    /* DWORD 3 */
    //    UINT32_T     buffer_size  :24;
    //    UINT32_T                  :3;
    //    UINT32_T     TRBSTS       :5;
    UINT32_T dStatus;

    /* DWORD 4 */
    //    UINT32_T     HWO          :1;      /* HW owner of descriptor (SW should set to 1, HW clears it unless Link TRB or Short packet on OUT endpt) */
    //    UINT32_T     LST          :1;      /* Last TRB (Do not set for ISOCH endpts) */
    //    UINT32_T     CHN          :1;      /* Chain buffers */
    //    UINT32_T     CSP          :1;      /* Continue on Short Packet */
    //    UINT32_T     TRBCTL       :6;      /* TRB Control */
    //    UINT32_T     ISP          :1;      /* Interrupt on Short Packet */
    //    UINT32_T     IOC          :1;      /* Interrupt on Complete */
    //    UINT32_T                  :2;
    //    UINT32_T     stream_ID    :16;     /* Stream ID for USB 3.0 bulk endpts (Frame Num/Index for ISOCH endpts) */
    //    UINT32_T                  :2;
    UINT32_T dControl;

} TRANSFER_REQUEST_BLOCK_T;

typedef volatile struct _AHCI_RFIS_T /* 256 bytes */
{
    UINT8_T DMA_Setup_FIS[28];
    UINT8_T rsvd0[4];

    UINT8_T PIO_Setup_FIS[20];
    UINT8_T rsvd1[12];

    UINT8_T D2H_Register_FIS[20];
    UINT8_T rsvd2[4];
    UINT8_T Set_Device_Bits_FIS[8];

    UINT8_T Unknown_FIS[64];
    UINT8_T rsvd3[96];
} AHCI_RFIS_T;

typedef volatile struct _AHCI_CMD_TABLE_T /* 256 bytes */
{
    UINT8_T Command_FIS[64];
    UINT8_T ATAPI_CMD[16];
    UINT8_T rsvd[48];
    UINT32_T PRDT[AHCI_MAX_SCAT_GATH * 4]; /* 4 DWORDS per scatter/gather entry */
} AHCI_CMD_TABLE_T;

typedef volatile struct _AHCI_CMD_HEADER_T /* 32 bytes */
{
    UINT32_T dDescInfo;
    UINT32_T dPRDByteCnt; /* byte count updated before PxCI bit for the command is cleared and is only valid for non-NCQ commands */
    UINT32_T dCmdTableBaseAddr;
    UINT32_T dCmdTableBaseAddrHi;
    UINT32_T rsvd[4];
} AHCI_CMD_HEADER_T;

typedef volatile struct _AHCI_MEMORY_T /* 3 KB for NCQ_depth = 8, 5KB for NCQ_depth = 16 */
/* (Must be multiple of 1KB to maintain command list alignment for multiple SATA ports) */
{
    /* AHCI Command List */
    /* Must be 1-KB aligned */
    AHCI_CMD_HEADER_T ahci_cmd_list[AHCI_NCQ_DEPTH]; /* 32 bytes * depth = 512 bytes */

    /* ACHI Recieve FIS */
    /* Must be 256-byte aligned */
    AHCI_RFIS_T ahci_rfis; /* 256 bytes */

    /* AHCI Command Table */
    AHCI_CMD_TABLE_T ahci_cmd_table[AHCI_NCQ_DEPTH]; /* 256 bytes * depth = 4 KB */

    /* Alignment Hole in case we want to add additional AHCI ports */
    /* Size depends on the NCQ depth for the above structs and max number of PRDT scatter/gather entries */
#if AHCI_NCQ_DEPTH == 8
    UINT8_T alignment_hole[512]; /* 512 bytes */
#elif AHCI_NCQ_DEPTH == 16
    UINT8_T alignment_hole[256]; /* 256 bytes */
#endif

} AHCI_MEMORY_T;

typedef volatile struct _UMS_COMMAND_T {
    UINT8_T command[32]; // UAS cmd header is 16 bytes, longest supported SCSI cmd is 16 bytes = 32 bytes.
} UMS_COMMAND_T;

typedef volatile struct _UMS_STATUS_T {
    UINT8_T status[32]; // We need 26-bytes for UAS SENSE IU, rounding up to 32 bytes.
} UMS_STATUS_T;

typedef volatile struct _DATAPATH_RAM_T /* this struct will be placed in datapath RAM */
{
    /* Wrap Window Memory */
    UINT8_T wrap_window_memory_placeholder[64 * 1024];

    /* AHCI Memory Structures */
    AHCI_MEMORY_T ahci_mem[AHCI_NUM_PORTS];

    /* USB Event Buffer */
    UINT32_T event_buffer_0[EVNT_BUFFER_SIZE / 4];

    /* EP0 buffer */
    UINT8_T ep0_buffer[512];

    /* HID Report Buffers */
    UINT8_T HID_Report_Buf_OUT[64];
    UINT8_T HID_Report_Buf_IN[64];

    /* Bulk EP buffer */
    UINT8_T bulk_ep_buffer[1024];

    /* USB Transfer Request Block Descriptors */
    /* Must be 16-byte aligned (last 4 bits are zero) */
    TRANSFER_REQUEST_BLOCK_T trb_ep0_setup_packet;
    TRANSFER_REQUEST_BLOCK_T trb_data_IN[MAX_EP_NUM + 1];
    TRANSFER_REQUEST_BLOCK_T trb_data_OUT[MAX_EP_NUM + 1];

    TRANSFER_REQUEST_BLOCK_T trb_ring_IN[TRB_RING_IN_SIZE];   /* Circular TRB to use for wrap window IN xfers */
    TRANSFER_REQUEST_BLOCK_T trb_ring_OUT[TRB_RING_OUT_SIZE]; /* Circular TRB to use for wrap window IN xfers */

    /* USB Mass Storage command buffer */
    UMS_COMMAND_T ums_cmd_buffer[UMS_UAS_CMD_DEPTH];

    /* USB Mass Storage status buffer */
    UMS_STATUS_T ums_status_buffer[UMS_UAS_CMD_DEPTH];

    /* SCSI command response buffer (for sending responses generated by the SCSI block) */
    UINT8_T scsi_cmd_response_buffer[SCSI_RESPONSE_BUFF_SIZE];

} DATAPATH_RAM_T;

/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

extern UINT32_T rti_clock_mhz;
extern BOOLEAN_T emulation_platform;
extern volatile DATAPATH_RAM_T *datapath_ram;

/*----------------------------------------------------------------------------+
| Functions                                                                   |
+----------------------------------------------------------------------------*/

inline UINT32_T BSWAP_32(UINT32_T val) {
    return ((((val) & 0xff) << 24) | (((val) & 0xff00) << 8) | (((val) & 0xff0000) >> 8) | (((val) >> 24) & 0xff));
}

inline UINT16_T BSWAP_16(UINT16_T val) { return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8)); }

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

int main(void);

#endif /*_TUSB9260_H_*/
