/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : reg_io.c
//             
// Project     : TUSB926x Firmware.
//             
// Description : Register Read/Write functions.
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
 * This file contains register Read/Write functions.
 *  
 */

#include "reg_io.h"
#include "sci.h"  // kprintf()
#include "tusb9260.h"
#include "tusb9260_types.h"



#if DEBUG_LEVEL >= REG_IO_DEBUG_LEVEL

/*****************************************************************************
 * Function: dbg_read_reg32
 *************************************************************************//**
 * This function reads a memory location and prints debug information.  
 *
 * @param[in] addr address of memory location.
 *                    
 * @return The value of the register.
 *
 ****************************************************************************** 
 */

UINT32_T dbg_read_reg32(UINT32_T addr)
{
    UINT32_T val;

    val = *(volatile UINT32_T*)addr;
    kprintf("   Rd: 0x%08x = 0x%08x.\n", addr, val);
    return val;
}


/*****************************************************************************
 * Function: dbg_write_reg32
 *************************************************************************//**
 * This function writes a 32-bit value into a memory location and prints
 * debug information.
 *
 * @param[in] addr address of memory location.
 * @param[in] val DWORD value to write.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void dbg_write_reg32(UINT32_T addr, UINT32_T val)
{
    UINT32_T new_val;

    *(volatile UINT32_T*)addr = val;
    new_val = *(volatile UINT32_T*)addr;

    if (new_val != val)
    {
        kprintf("   Wr: 0x%08x = 0x%08x  <Rd>: 0x%08x.\n", addr, val, new_val);
    }
    else
    {
        kprintf("   Wr: 0x%08x = 0x%08x.\n", addr, val);
    }
}

#endif



/*****************************************************************************
 * Function: MODIFY32
 *************************************************************************//**
 * This function modifies the value of a memory location and prints debug
 * information depending on the DEBUG_LEVEL.
 *
 * @param[in] addr address of memory location.
 * @param[in] clear_mask mask of bits to clear. (must be non-zero)
 * @param[in] set_mask mask of bits to set. (this value is ORed with clear mask before setting)
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void MODIFY32(UINT32_T addr, UINT32_T clear_mask, UINT32_T set_mask)
{
    UINT32_T reg;

    reg = READ32(addr);
    reg &= ~clear_mask;
    reg |= (set_mask & clear_mask);
    WRITE32(addr, reg);

    return;
}


/*****************************************************************************
 * Function: MODIFY_REG32
 *************************************************************************//**
 * This function modifies the value of a memory location without printing
 * any debug info.
 *
 * @param[in] addr address of memory location.
 * @param[in] clear_mask mask of bits to clear. (must be non-zero)
 * @param[in] set_mask mask of bits to set. (this value is ORed with clear mask before setting)
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void MODIFY_REG32(UINT32_T addr, UINT32_T clear_mask, UINT32_T set_mask)
{
    UINT32_T reg;

    reg = READ_REG32(addr);
    reg &= ~clear_mask;
    reg |= (set_mask & clear_mask);
    WRITE_REG32(addr, reg);

    return;
}









