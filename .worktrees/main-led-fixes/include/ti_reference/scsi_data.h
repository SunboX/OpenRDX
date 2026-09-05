/*
 * SPDX-FileCopyrightText: 2010 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : scsi_data.h
//
// Project     : TUSB9260 Firmware.
//
// Description : Header file for SCSI data.
//
//   (C) Copyright 2010 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   09/27/10 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * Header file for the SCSI data.
 *  
 */

#ifndef _SCSI_DATA_H_
#define _SCSI_DATA_H_

#include "tusb9260_types.h"

#define NUM_SUPPORTED_VPD_PAGES 7

typedef UINT8_T VPD_PAGE00_DATA_T[NUM_SUPPORTED_VPD_PAGES + 4];

/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

extern UINT8_T fixed_format_sense_data[18];
extern VPD_PAGE00_DATA_T inquiry_vpd_page00_data;
extern UINT8_T ata_return_descriptor_data[14];

#endif
