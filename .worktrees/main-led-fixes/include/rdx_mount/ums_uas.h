/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ums_uas.h
//             
// Project     : TUSB926x Firmware.
//             
// Description : Header file for UAS module.
// 
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   07/23/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * Header file for the USB Mass Storage (UMS) Class USB Attached SCSI (UAS) layer.
 *
 */

#include "tusb9260.h"
#include "tusb9260_types.h"

#ifndef _UMS_UAS_H_
#define _UMS_UAS_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define UAS_REVISION_4_IU    1   /* Set to 1 to use UAS spec v4 IUs */

/* Data-IN pacing prevents the SATA drive from completing multiple reads before the 
 * data previous read can be transmitted over USB.  It will reduce the performance
 * when issuing multiple queued reads with transfer length < 16KB, but will ensure
 * the system does not miss any DMA Setup FIS interrupts. This is required for 
 * FPGA platforms running at slower clock rates. */
#define ENABLE_UAS_DATA_IN_PACING  0  // (Default = 0)


/* Information Unit IDs */
#define UAS_COMMAND_IU           0x01
#define UAS_SENSE_IU             0x03
#define UAS_RESPONSE_IU          0x04
#define UAS_TASK_MANAGEMENT_IU   0x05
#define UAS_READ_READY_IU        0x06
#define UAS_WRITE_READY_IU       0x07


// SCSI Status Codes. (SAM-5)
#define SCSI_STATUS_GOOD                  0x00
#define SCSI_STATUS_CHECK_CONDITION       0x02
#define SCSI_STATUS_CONDITION_MET         0x04
#define SCSI_STATUS_BUSY                  0x08
#define SCSI_STATUS_RESERVATION_CONFLICT  0x18
#define SCSI_STATUS_TASK_SET_FULL         0x28
#define SCSI_STATUS_ACA_ACTIVE            0x30
#define SCSI_STATUS_TASK_ABORTED          0x40


// Response Codes.
#define RC_TASK_MGMT_FUNCTION_CMPLT           0x00
#define RC_INVALID_INFORMATION_UNIT           0x02  /* if IU_ID field is not valid */
#define RC_TASK_MGMT_FUNCTION_NOT_SUPPORTED   0x04
#define RC_TASK_MGMT_FUNCTION_FAILED          0x05
//#define RC_TASK_MGMT_FUNCTION_SUCCEEDED       0x08   /* for Query task (not supported) */
#define RC_INCORRECT_LUN                      0x09
#define RC_OVERLAPPED_TAG_ATTEMPTED           0x0A  

// Task Management Functions.
#define TMF_ABORT_TASK           0x01
#define TMF_ABORT_TASK_SET       0x02
#define TMF_CLEAR_TASK_SET       0x04
#define TMF_LOGICAL_UNIT_RESET   0x08
#define TMF_I_T_NEXUS_RESET      0x10
#define TMF_CLEAR_ACA            0x40  /* ACA - auto contingent allegiance */
#define TMF_QUERY_TASK           0x80
#define TMF_QUERY_TASK_SET       0x81
#define TMF_QUERY_ASYNC_EVENT    0x82



/*----------------------------------------------------------------------------+
| Structures & Enums                                                          |
+----------------------------------------------------------------------------*/

typedef struct _UAS_COMMAND_IU_T
{
    UINT8_T  bIU_ID;
    UINT8_T  rsvd;
    UINT16_T wTAG;     /* used as USB 3.0 stream ID */
#if UAS_REVISION_4_IU
    UINT8_T  bCmdPriority;  /* Priority [6:3] and Task attribute [2:0] */
    UINT8_T  rsvd1;
    UINT8_T  bAdditionalCDBLength;  /* bits [7:2] only (DWORDS) */
#else
    UINT16_T wLength;   /* bytes */
    UINT8_T  bCmdPriority;  /* and Task attribute */
#endif
    UINT8_T  rsvd2;
    UINT8_T  bLUN[8];
    UINT8_T  CDB[16];
    /* no additional CDB bytes req'd because we only support 16-byte SCSI cmds. */
} UAS_COMMAND_IU_T;

typedef struct _UAS_RW_READY_IU_T
{
    UINT8_T  bIU_ID;
    UINT8_T  rsvd;
    UINT16_T wTAG;
} UAS_RW_READY_IU_T;


typedef struct _UAS_SENSE_IU_T
{
    UINT8_T  bIU_ID;
    UINT8_T  rsvd;
    UINT16_T wTAG;
#if UAS_REVISION_4_IU
    UINT16_T wStatusQualifier;
    UINT8_T  bStatus;
    UINT8_T  rsvd1[7];
#else
    UINT8_T  bStatus;
    UINT8_T  rsvd1;
#endif
    UINT16_T wLength;      /* length of sense data */
    UINT8_T  bSenseData[18];   /* fixed format sense data is 18-bytes. */
} UAS_SENSE_IU_T;

typedef struct _UAS_RESPONSE_IU_T
{
    UINT8_T  bIU_ID;
    UINT8_T  rsvd;
    UINT16_T wTAG;
    UINT8_T  bAdditionalResponseInfo[3];
    UINT8_T  bResponseCode;
} UAS_RESPONSE_IU_T;


typedef struct _UAS_TASK_MGMT_IU_T
{
    UINT8_T  bIU_ID;
    UINT8_T  rsvd;
    UINT16_T wTAG;
    UINT8_T  bTaskMgmtFunction;
    UINT8_T  rsvd1;
    UINT16_T wTaskTAG;
    UINT8_T  bLUN[8];
} UAS_TASK_MGMT_IU_T;


typedef struct _UAS_CMD_PIPE_IU_T
{
    UINT8_T  bIU_ID;
    UINT8_T  rsvd;
    UINT16_T wTAG;
    UINT32_T rsvd1;
    UINT8_T  bLUN[8];
    UINT8_T  CDB[16];   /* only valid for Command IU */
} UAS_CMD_PIPE_IU_T;



typedef enum
{
    UMS_UAS_STATE_IDLE                = 0,  /* Not in use */
    UMS_UAS_STATE_RX_CMD              = 1,  /* Ready to Rx command IU */
    UMS_UAS_STATE_CMD_RECEIVED        = 2,  /* Indicates a command has been received in the command buffer */
    UMS_UAS_STATE_CMD_WAITING         = 3,  /* When a non-queued command is waiting to be sent to the SATA device */
    UMS_UAS_STATE_QUEUED_CMD_WAITING  = 4,  /* When a Queued R/W command is waiting to be sent to the SATA device */
    UMS_UAS_STATE_DATA_IN_PENDING     = 5,  /* when SATA device is ready but data pipe is busy when trying to Read data */
    UMS_UAS_STATE_DATA_OUT_PENDING    = 6,  /* when SATA device is ready but data pipe is busy when trying to Write data */
    UMS_UAS_STATE_DATA_IN_QUEUED      = 7,  /* when Queued Read command has been issued to SATA drive */
    UMS_UAS_STATE_DATA_OUT_QUEUED     = 8,  /* when Queued Write command has been issued to SATA drive */
    UMS_UAS_STATE_DATA_IN             = 9,  /* there can only be 1 command buffer with this state at any time */
    UMS_UAS_STATE_DATA_OUT            = 10, /* there can only be 1 command buffer with this state at any time */
    UMS_UAS_STATE_READ_READY_PENDING  = 11, /* when status pipe is busy when trying to send Read Ready IU */
    UMS_UAS_STATE_WRITE_READY_PENDING = 12, /* when status pipe is busy when trying to send Write Ready IU */
    UMS_UAS_STATE_SENSE_PENDING       = 13, /* when status pipe is busy when trying to send Sense IU */
    UMS_UAS_STATE_RESPONSE_PENDING    = 14, /* when status pipe is busy when trying to send Response IU */
    UMS_UAS_STATE_SEND_SENSE          = 15, /* when sense IU can be sent upon ATA callback (valid for data and non-data commands) */
    UMS_UAS_STATE_STATUS_SENT         = 16, /* when sense IU or response IU has been sent */

} UMS_UAS_STATE_T;


typedef struct _UMS_UAS_CMD_RESPONSE_ORDER_T
{
    UINT32_T cmd_index[AHCI_NCQ_DEPTH];
    INT32_T index;
    INT32_T processing_index;

} UMS_UAS_CMD_RESPONSE_ORDER_T;

typedef struct
{
    void*     data_buff[AHCI_NCQ_DEPTH];
    UINT32_T  byte_cnt[AHCI_NCQ_DEPTH];
    BOOLEAN_T xfer_pending[AHCI_NCQ_DEPTH];
    UINT32_T  cmd_index[AHCI_NCQ_DEPTH];

    INT32_T  index;
    INT32_T  processing_index;
} UMS_UAS_PENDING_DATA_IN_LIST_T;

typedef struct
{
    void*     data_buff;
    UINT32_T  byte_cnt;
    BOOLEAN_T xfer_pending;
    UINT32_T  cmd_index;

} UMS_UAS_PENDING_DATA_OUT_INFO_T;

typedef struct
{
    BOOLEAN_T valid_flag[AHCI_NCQ_DEPTH];
    UINT32_T  cmd_index[AHCI_NCQ_DEPTH];

    INT32_T  index;             // list tail
    INT32_T  processing_index;  // list head
} UMS_UAS_INDEX_LIST_T;

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void ums_uas_init(void);

#endif

