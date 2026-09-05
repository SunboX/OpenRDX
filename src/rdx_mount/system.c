/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : system.c
//
// Project     : TUSB926x Firmware.
//
// Description : This file contains the system level functions.
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
 * This file contains the system functions.
 *
 */

#include "system.h"
#include "reg_io.h"
#include "sci.h"
#include "tusb9260.h"

#define SYSECR_SW_RESET1_BIT   0x00008000

/*****************************************************************************
 * Function: system_reset
 *************************************************************************//**
 * This function performs a global device reset.
 *
 * @param None.
 *
 * @return None.
 *
 ****************************************************************************** 
 */

void system_reset(void)
{
    CRIT("### SYSTEM RESET ###\n");

    // Perform a Global Device Reset
    WRITE_REG32(SYSECR_REG_OFF, SYSECR_SW_RESET1_BIT);

    return;
}




