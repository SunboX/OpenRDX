/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : wdt.c
//
// Project     : TUSB926x Firmware.
//
// Description : Watchdog timer.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   04/01/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file implements functions for the watchdog timer.
 *
 */

#include "wdt.h"
#include "reg_io.h"


#define WDT_CTRL_ACTIVATE_TIMER   0xACED5312
#define WDT_STATUS_BIT_MASK       0x0000001F

/*****************************************************************************
 * Function: wdt_start
 *************************************************************************//**
 * This function starts the watchdog timer.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void wdt_start(void)
{
    UINT32_T preload_val = WDT_PRELOAD_VAL;

    // Make sure preload value is within bounds.
    if (preload_val > WDT_MAX_PRELOAD_VAL)
    {
        preload_val = WDT_MAX_PRELOAD_VAL;
    }

    // Set Digital WD preload value.
    WRITE_REG32(WDT_PRLD_REG_OFF, preload_val);

    // Activate Timer.
    WRITE_REG32(WDT_CTRL_REG_OFF, WDT_CTRL_ACTIVATE_TIMER);

    // Clear all WD status bits.
    WRITE_REG32(WDT_STATUS_REG_OFF, WDT_STATUS_BIT_MASK);

    return;
}


#define WDT_RESET_KEY1     0xE51A
#define WDT_RESET_KEY2     0xA35C

/*****************************************************************************
 * Function: wdt_reset
 *************************************************************************//**
 * This function resets the watchdog timer.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void wdt_reset(void)
{
    // Key 1 and Key 2 must be written in sequence to reset the WDT.
    WRITE_REG32(WDT_KEY_REG_OFF, WDT_RESET_KEY1);
    WRITE_REG32(WDT_KEY_REG_OFF, WDT_RESET_KEY2);
}


