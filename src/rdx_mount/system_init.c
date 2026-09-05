/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : system_init.c
//
// Project     : TUSB926x Firmware.
//
// Description : This file contains the system intialization function.
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
 * This file contains the system intialization function.
 *
 */

#include "system.h"
#include "reg_io.h"


#define SYS_CLKCNTRL_PENA    0x00000100

/*****************************************************************************
 * Function: system_init
 *************************************************************************//**
 * This function resets and enables the system peripherials.
 *
 * @param None.
 *
 * @return None.
 *
 ****************************************************************************** 
 */

void system_init(void)
{
    // Global Reset to the Peripherals.
    WRITE_REG32(CLKCNTL_REG_OFF, 0);

    // Enable the Peripherals.
    WRITE_REG32(CLKCNTL_REG_OFF, SYS_CLKCNTRL_PENA);

    // Take Peripherals Out of Power Down Mode.
    WRITE_REG32(PCR_PSPWRDWNCLR0, 0xFFFFFFFF);
    WRITE_REG32(PCR_PSPWRDWNCLR1, 0xFFFFFFFF);
    WRITE_REG32(PCR_PSPWRDWNCLR2, 0xFFFFFFFF);
    WRITE_REG32(PCR_PSPWRDWNCLR3, 0xFFFFFFFF);

    // Peripheral Protection Clear Register.
    WRITE_REG32(PCR_PPROTCLR0, 0xFFFFFFFF);

    return;
}

