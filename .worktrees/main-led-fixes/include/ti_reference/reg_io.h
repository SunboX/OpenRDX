/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : reg_io.h
//
// Project     : TUSB926x Firmware.
//
// Description : Register Read/Write functions header file.
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
 * Header file for register Read/Write functions.
 *  
 */

#include "tusb9260_types.h"

#ifndef _REG_IO_H_
#define _REG_IO_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define REG_IO_DEBUG_LEVEL 4 /* Debug level at which register R/W info should be printed */

/* These macros should be used when accessing registers before the SCI module is initialized or  
 * to prevent register access debug output.  Typically, READ32() and WRITE32() should be used */
#define READ_REG32(addr) (*(volatile UINT32_T *)(addr))
#define WRITE_REG32(addr, val) (*(volatile UINT32_T *)(addr) = val)

#if DEBUG_LEVEL >= REG_IO_DEBUG_LEVEL
#define WRITE32(addr, val) dbg_write_reg32(addr, val)
#define READ32(addr) dbg_read_reg32(addr)
#else
#define WRITE32(addr, val) WRITE_REG32(addr, val)
#define READ32(addr) READ_REG32(addr)
#endif

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void MODIFY32(UINT32_T addr, UINT32_T clear_mask, UINT32_T set_mask);
void MODIFY_REG32(UINT32_T addr, UINT32_T clear_mask, UINT32_T set_mask);
void dbg_write_reg32(UINT32_T addr, UINT32_T val);
UINT32_T dbg_read_reg32(UINT32_T addr);

#endif
