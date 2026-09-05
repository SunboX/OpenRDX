/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : scsi.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for SCSI layer.
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
 * Header file for the SCSI layer.
 *  
 */

#include "usb_hal.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

#ifndef _SCSI_H_
#define _SCSI_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define SCSI_MAX_STD_OPCODE_VALUE 0xBF // Opcodes 0xC0 - 0xFF are vendor specific.

/* Supported SCSI Commands */
#define SCSI_FORMAT_UNIT 0x04
#define SCSI_START_STOP_UNIT 0x1B // no data

#define SCSI_INQUIRY 0x12
#define SCSI_READ_FORMAT_CAPACITIES 0x23

#define SCSI_MODE_SENSE6 0x1A
#define SCSI_MODE_SENSE10 0x5A

#define SCSI_READ_CAPACITY10 0x25
#define SCSI_READ_CAPACITY16 0x9E

#define SCSI_READ6 0x08
#define SCSI_READ10 0x28
#define SCSI_READ12 0xA8
#define SCSI_READ16 0x88

#define SCSI_REQUEST_SENSE 0x03
#define SCSI_TEST_UNIT_READY 0x00

#define SCSI_WRITE6 0x0A
#define SCSI_WRITE10 0x2A
#define SCSI_WRITE12 0xAA
#define SCSI_WRITE16 0x8A

#define SCSI_VERIFY10 0x2F
#define SCSI_VERIFY12 0xAF
#define SCSI_VERIFY16 0x8F

#define SCSI_UNMAP 0x42

#define SCSI_SYNCHRONIZE_CACHE10 0x35 // no data
#define SCSI_SYNCHRONIZE_CACHE16 0x91 // no data

#define SCSI_REPORT_LUNS 0xA0
#define SCSI_SEND_DIAGNOSTIC 0x1D
#define SCSI_READ_DIAGNOSTIC 0x1C // Not currently supported.

#define SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1E // no data

#define SCSI_MODE_SELECT6 0x15
#define SCSI_MODE_SELECT10 0x55

#define ATA_PASS_THROUGH12 0xA1
#define ATA_PASS_THROUGH16 0x85

#define SCSI_SECURITY_PROTOCOL_IN 0xA2
#define SCSI_SECURITY_PROTOCOL_OUT 0xB5

#define SCSI_READ_CD 0xBE

/* TI defined SCSI commands */
#define SCSI_TI_OPCODE_MIN 0xE0
#define SCSI_TI_OPCODE_MAX 0xE7

#define SCSI_TI_ONE_TOUCH_BACKUP_QUERY 0xE0
#define SCSI_TI_FLASH_UNLOCK 0xE1
#define SCSI_TI_FLASH_ERASE 0xE2
#define SCSI_TI_GET_PID 0xE3
#define SCSI_TI_GET_FW_VERSION 0xE4
#define SCSI_TI_GET_USB_SPEED 0xE5
#define SCSI_TI_DEVICE_RESET 0xE6
#define SCSI_TI_READ_FLASH 0xE7 /* for reading the SPI flash */

#define DESCRIPTOR_FORMAT_SENSE_DATA_RESPONSE_CODE 0x72

#define SCSI_REPORT_LUNS_WLUN 0x01 /* Well known LUN for REPORT LUNS command */

/* Sense Keys */
#define NO_SENSE 0x00
#define SCSI_CORRECTED_ERROR 0x01
#define NOT_READY 0x02
#define MEDIUM_ERROR 0x03
#define HARDWARE_ERROR 0x04
#define ILLEGAL_REQUEST 0x05
#define UNIT_ATTENTION 0x06
#define DATA_PROTECT 0x07
//#define BLANK_CHECK         0x08
//#define COPY_ABORTED        0x0A
#define ABORTED_COMMAND 0x0B
//#define VOLUME_OVERFLOW     0x0D
//#define MISCOMPARE          0x0E

/* Additional Sense Code */
typedef enum _ASC_CODE_T {
    ATA_PASS_THROUGH_INFO_AVAIL = 0x00,
    NO_ADDITIONAL_SENSE_INFO = 0x00,
    INVALID_COMMAND = 0x20,
    INVALID_FIELD_IN_COMMAND = 0x24,
    //PARAMETER_LIST_LENGTH_ERROR                  = 0x1A,
    //INVALID_FIELD_IN_PARAMETER_LIST              = 0x26,
    LBA_OUT_OF_RANGE = 0x21,
    MEDIUM_NOT_PRESENT = 0x3A,
    MEDIUM_MAY_HAVE_CHANGED = 0x28,
    //INVALID_INFORMATION_UNIT                     = 0x0E,
    //LOGICAL_UNIT_DOES_NOT_RESPOND_TO_SELECTION   = 0x05,
    //LOGICAL_UNIT_FAILURE                         = 0x3E,
    //COMMANDS_CLEARED_BY_DEVICE_SERVER            = 0x2F,
    //OVERLAPPED_COMMANDS_ATTEMPTED                = 0x4E,
    DATA_PHASE_ERROR = 0x4B,
    LOGICAL_UNIT_NOT_SUPPORTED = 0x25,
    IU_CRC_ERROR_DETECTED = 0x47,
    UNCORRECTABLE_READ_ERROR = 0x11,
    INTERNAL_TARGET_FAILURE = 0x44,
    OPERATOR_MEDIUM_REMOVAL_REQUEST = 0x5A,
    WRITE_PROTECTED = 0x27,
    OVERLAPPED_CMDS_ATTEMPTED = 0x4E,
    BUS_DEVICE_RESET_FUNCTION_OCCURRED = 0x29,
    LUN_NOT_READY = 0x04,
    LOGICAL_UNIT_FAILED_SELF_TEST = 0x3E,
    //    INQUIRY_DATA_HAS_CHANGED                     = 0x3F

} ASC_CODE_T;

/* Additional Sense Code Qualifiers */
typedef enum _ASCQ_CODE_T {
    NO_ASCQ = 0x00,
    //ASCQ_LOGICAL_UNIT_FAILURE              = 0x01,
    //ASCQ_COMMANDS_CLEARED_BY_DEVICE_SERVER = 0x02,
    //ASCQ_MEDIUM_NOT_PRESENT_LOADABLE       = 0x03,
    ASCQ_IU_CRC_ERROR_DETECTED = 0x03,
    ASCQ_ATA_PASS_THROUGH_INFO_AVAIL = 0x1D,
    ASCQ_OPERATOR_MEDIUM_REMOVAL_REQUEST = 0x01,
    ASCQ_BUS_DEVICE_RESET_FUNCTION_OCCURRED = 0x03,
    ASCQ_LUN_BECOMING_READY = 0x01,
    ASCQ_LOGICAL_UNIT_FAILED_SELF_TEST = 0x03,
    //    ASCQ_INQUIRY_DATA_HAS_CHANGED           = 0x03

} ASCQ_CODE_T;

/* Vital Product Data Page codes */
#define VPD_SUPPORTED_PAGES_PAGE_CODE 0x00
#define VPD_UNIT_SERIAL_NUMBER_PAGE_CODE 0x80
#define VPD_DEVICE_ID_PAGE_CODE 0x83
#define VPD_ATA_INFO_PAGE_CODE 0x89
#define VPD_BLOCK_LIMITS_PAGE_CODE 0xB0
#define VPD_BLOCK_DEVICE_CHAR_PAGE_CODE 0xB1
#define VPD_LOGICAL_BLOCK_PROVISIONING_PAGE_CODE 0xB2

/*----------------------------------------------------------------------------+
| Structures                                                                  |
+----------------------------------------------------------------------------*/

typedef struct {
    UINT8_T bPortNum; /* directly maps to LUN */
    UINT8_T bCmdSlot; /* used to my UAS layer to look up TAG to send ERDY or R/W READY */
    UINT8_T bStatus;  /* Valid for Set Device Bits FIS and D2H Register FIS (bit 0 indicates error) */
    //UINT8_T bError;  /* may not need this data in UMS layer */

    UINT32_T dSActive; /* Valid for Set Device Bit FIS only (bit set to one indicates completion of a command slot) */

    //void *pData;   /* Valid for DMA Setup FIS and D2H Register FIS (ptr for UMS layer to Tx/Rx data over USB) */
    UINT32_T dDataByteCnt; /* Valid for PIO Setup FIS, D2H Register FIS, and DMA Setup FIS */
    //UINT8_T  bDirection;  /* Valid for PIO Setup FIS only */

} ATA_CMD_CALLBACK_T;

typedef struct _SCSI_CMD_INPUT_T {
    UINT8_T *pCommandBlock;

    UINT32_T dDataXferLength; // Used for all non-RW cmds and ATAPI write cmds (USB data-OUT). (also used for NO_STALL testing)

    /* Valid if status = STATUS_RESPONSE_READY */
    void *pData;           /* for UMS layer to send cmd response via USB */
    UINT32_T dDataByteCnt; /* for UMS layer to send cmd response via USB */

    UINT8_T bLUN;
    UINT8_T bCmdBlkLength;
    UINT8_T bCmdSlotNum; /* valid if status = STATUS_RESPONSE_PENDING */

} SCSI_CMD_INPUT_T;

typedef struct _SCSI_CMD_INFO_T {
    /* Incoming parameters */
    SCSI_CMD_INPUT_T *pCmdInput;

    /* Parameters */
    UINT32_T dXferLength; /* blocks */ /* also serves a verification length for VERIFY cmds. */
    BOOLEAN_T bFUA;
    UINT8_T bLBA[3];     /* LBA High, Mid, Low */
    UINT8_T bLBA_exp[3]; /* LBA High, Mid, Low (expanded) */
    UINT8_T bRSVD[2];    /* space to allow casting LBA to UINT64_T */
} SCSI_CMD_INFO_T;

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

BOOLEAN_T scsi_is_rw_cmd(UINT8_T scsi_cmd);
BOOLEAN_T scsi_is_write_cmd(UINT8_T scsi_cmd);

BOOLEAN_T scsi_is_check_condition_set(const UINT8_T *cdb);

UINT32_T scsi_copy_sense_data(void *dst, BOOLEAN_T descriptor_format);

STATUS_T scsi_get_rw_xfer_length(const UINT8_T *cdb, UINT8_T lun, UINT32_T *scsi_xfer_length);

STATUS_T scsi_get_data_direction(const UINT8_T *cdb, ENDPT_DIR_T *direction);

void scsi_set_sense_data(UINT8_T key, ASC_CODE_T asc, ASCQ_CODE_T ascq);

STATUS_T scsi_command_handler(SCSI_CMD_INPUT_T *pCmdInput);

void scsi_init(void);

#endif /*_SCSI_H_*/
