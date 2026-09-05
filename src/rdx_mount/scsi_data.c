/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : scsi_data.c
//
// Project     : TUSB926x Firmware.
//
// Description : Data structures for SCSI module.
//                
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
 * This file contains data structures for the SCSI layer.
 *
 */

#include "scsi_data.h"
#include "scsi.h"


/** Supported VPD pages page */
VPD_PAGE00_DATA_T inquiry_vpd_page00_data =
{
    0x00, /* Direct access block device connected */
    VPD_SUPPORTED_PAGES_PAGE_CODE, /* Page code */
    0x00, /* RSVD */
    NUM_SUPPORTED_VPD_PAGES, /* Page length (# of supported VPD pages) */

    /* Supported VPD Pages */
    VPD_SUPPORTED_PAGES_PAGE_CODE,
    VPD_UNIT_SERIAL_NUMBER_PAGE_CODE,
    VPD_DEVICE_ID_PAGE_CODE,
    VPD_ATA_INFO_PAGE_CODE,
    VPD_BLOCK_LIMITS_PAGE_CODE,
    VPD_BLOCK_DEVICE_CHAR_PAGE_CODE,
    VPD_LOGICAL_BLOCK_PROVISIONING_PAGE_CODE,
    VPD_RDX_MEDIA_ID_PAGE_CODE,
    VPD_RDX_MEDIA_IDENTIFY_PAGE_CODE
};


UINT8_T fixed_format_sense_data[18] =
{
    0x70,     /* Response Code */
    0x00,     /* Obsolete */
    NO_SENSE, /* Sense Key */

    /* Command Information */
    0x00,
    0x00,
    0x00,
    0x00,

    0x0A,  /* Additional Sense Length */

    /* Command-specific Info */
    0x00,
    0x00,
    0x00,
    0x00, 

    NO_SENSE, /* Additional Sense Code */
    0x00, /* ASCQ */
    0x00, /* FRUC */

    /* Sense Key Specific */
    0x00, 
    0x00,
    0x00 
};


UINT8_T ata_return_descriptor_data[14] =
{
    0x09,   /* Desc Code */
    0x0C,   /* Additional Length */
    0x00,   /* Extend */

    0x00,   /* Error */
    0x00,   /* Sector count */
    0x00,   /* Sector count */
    0x00,   /* LBA_LOW  */
    0x00,   /* LBA_LOW  */
    0x00,   /* LBA_MID  */
    0x00,   /* LBA_MID  */
    0x00,   /* LBA_HIGH */
    0x00,   /* LBA_HIGH */
    0x00,   /* Device */
    0x00    /* status */
};


