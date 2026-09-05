/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : tusb9260_types.h
//
// Project     : TUSB926x Firmware.
//
// Description : Defines standard types.
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
 * This file defines standard types.
 *
 */

#ifndef _TUSB9260_TYPES_H_
#define _TUSB9260_TYPES_H_

#define FALSE  0
#define TRUE   !FALSE

#ifndef NULL
#define NULL   0x0
#endif

typedef unsigned long long      UINT64_T;
typedef signed long long        INT64_T;
typedef unsigned long           UINT32_T;
typedef signed long             INT32_T;
typedef unsigned short          UINT16_T;
typedef signed short            INT16_T;
typedef unsigned char           UINT8_T;
typedef signed char             INT8_T;
typedef unsigned long           BOOLEAN_T;


#endif
