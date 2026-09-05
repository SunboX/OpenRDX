/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ums_bot.h
//             
// Project     : TUSB926x Firmware.
//             
// Description : Header file for BOT module.
// 
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   01/30/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * Header file for the USB Mass Storage (UMS) Class Bulk-Only Transport (BOT) layer.
 *
 */

#include "tusb9260_types.h"


#ifndef _UMS_BOT_H_
#define _UMS_BOT_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define BOT_MAX_LUN                UMS_MAX_LUN
#define BOT_INTERFACE_NUM          UMS_INTERFACE_NUM   

#define BOT_CBW_SIGNATURE          0x43425355
#define BOT_CSW_SIGNATURE          0x53425355
#define BOT_CBW_PACKET_LENGTH      0x1F   /* 31 bytes */
#define BOT_CSW_PACKET_LENGTH      0x0D   /* 13 bytes */

#define BOT_CBWCB_MAX_LENGTH       0x10   /* 16 bytes */

/* CSW Status Definitions */
#define CSW_CMD_PASSED             0x00
#define CSW_CMD_FAILED             0x01  /* Host will request SCSI sense data */
#define CSW_PHASE_ERROR            0x02  /* Host needs to perform reset recovery */

/* Setup Pkt bRequest values */
#define BOT_MASS_STORAGE_RESET_REQUEST   0xFF
#define BOT_GET_MAX_LUN_REQUEST          0xFE


/*----------------------------------------------------------------------------+
| Structures & Enums                                                          |
+----------------------------------------------------------------------------*/


typedef struct _UMS_BOT_CBW_T
{
    UINT32_T dSignature;
    UINT32_T dTag;
    UINT32_T dDataTransferLength;
    UINT8_T  bmFlags;
    UINT8_T  bLUN;
    UINT8_T  bCBLength;
    UINT8_T  CB[16];
} UMS_BOT_CBW_T;


typedef struct _UMS_BOT_CSW_T
{
    UINT32_T dSignature;
    UINT32_T dTag;
    UINT32_T dDataResidue;
    UINT8_T  bStatus;
} UMS_BOT_CSW_T;


typedef enum
{
    UMS_BOT_STATE_IDLE        = 0,  /* Ready to receive CBW */
    UMS_BOT_STATE_DATA_IN     = 1,
    UMS_BOT_STATE_DATA_OUT    = 2,
    UMS_BOT_STATE_CSW_PENDING = 3,
    UMS_BOT_STATE_SEND_CSW    = 4,
    UMS_BOT_STATE_ERROR       = 5   /* stalled, waiting for BOT reset (also sends PHASE ERROR CSW */
} UMS_BOT_STATE_T;


/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

extern UMS_BOT_CBW_T *gCBW;


/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void ums_bot_init(void);

#endif
