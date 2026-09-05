/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : string.c
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for manipulating C strings.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   04/02/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This header file defines functions for manipulating C strings and arrays.
 *
 */

#ifndef _STRING_H_
#define _STRING_H_

#include "tusb9260_types.h"

void *ti_memset(void *ptr, UINT8_T val, UINT32_T size);
void *ti_memcpy(void *dst, const void *src, UINT32_T size);

#endif
