/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : string.c
//
// Project     : TUSB926x Firmware.
//
// Description : Defines functions for manipulating C strings.
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
 * This file contains functions for manipulating C strings and arrays.
 *
 */
  
#include "string.h"
#include "sci.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#if STRING_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif


/*****************************************************************************
 * Function: ti_memset
 *************************************************************************//**
 * This function fills a block of memory with a specified value.
 *
 * @param[in] ptr pointer to the block of memory to fill.
 * @param[in] val 1-byte value to be set.
 * @param[in] size number of bytes to be set to the value.
 *
 * @return \a ptr (Pointer to the block of memory).
 *
 ****************************************************************************** 
 */

void *ti_memset(void *ptr, UINT8_T val, UINT32_T size)
{
    UINT8_T *pBuf = (UINT8_T *)ptr;
    UINT8_T *pBufEnd = pBuf + size;

    INFO("   memset(): addr = 0x%x, val = 0x%x, size = %u bytes.\n", (UINT32_T)ptr, val, size);

    if (((UINT32_T)ptr & 0x3) == 0)
    {
        // If address is aligned, use as many 32-bit writes as possible.
        UINT32_T *pBufEnd32 = (UINT32_T *)((UINT32_T)pBufEnd & ~0x3); 
        UINT32_T *pBuf32 = (UINT32_T *)ptr;    
        UINT32_T val32 = ((UINT32_T)val << 24) | ((UINT32_T)val << 16) | ((UINT32_T)val << 8) | (UINT32_T)val;

        while (pBuf32 != pBufEnd32)
        {
            *pBuf32++ = val32;         
        }        

        pBuf = (UINT8_T *)pBuf32;
    }

    // Write 1-byte at a time for remaining bytes or if address is not aligned.
    while (pBuf != pBufEnd)
    {
        *pBuf++ = val;         
    }                        

    return ptr;
}

/*****************************************************************************
 * Function: ti_memcpy
 *************************************************************************//**
 * This function copies a block of memory.
 *
 * @param[in] dst pointer to the memory destination where content is to be copied.
 * @param[in] src pointer to the source of data to be copied.
 * @param[in] size number of bytes to be copied.
 *
 * @return \a dst (Pointer to the destination).
 *
 ****************************************************************************** 
 */

void *ti_memcpy(void *dst, const void *src, UINT32_T size)
{
    UINT8_T *pSrc = (UINT8_T *)src;
    UINT8_T *pDst = (UINT8_T *)dst;
    UINT8_T *pEnd = pSrc + size;

    INFO("   memcpy(): dst = 0x%x, src = 0x%x, size = %u bytes.\n", (UINT32_T)dst, (UINT32_T)src, size);

    if ((((UINT32_T)dst | (UINT32_T)src) & 0x3) == 0)
    {
        // If addresses are aligned, use as many 32-bit writes as possible.
        UINT32_T *pSrc32 = (UINT32_T *)src;
        UINT32_T *pDst32 = (UINT32_T *)dst;
        UINT32_T *pEnd32 = (UINT32_T *)((UINT32_T)pEnd & ~0x3);

        while (pSrc32 != pEnd32)
        {
            *pDst32++ = *pSrc32++;         
        }                       

        pSrc = (UINT8_T *)pSrc32;
        pDst = (UINT8_T *)pDst32;
    }

    // Write 1-byte at a time for remaining bytes or if addresses are not aligned.
    while (pSrc != pEnd)
    {
        *pDst++ = *pSrc++;         
    }                        

    return dst;
}

