/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : vim_nvic.c
//
// Project     : TUSB926x Firmware.
//
// Description : VIM & NVIC intialization.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   07/15/09 - Brian Quach - Removed unnecessary register settings & updated comments.
//   10/25/09 - Brian Quach - Masked out unused interrupts.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the initialization functions for the Vectored Interrupt Manager (VIM)
 * and Nested Vectored Interrupt Controller (NVIC) - an integrated feature of the ARM Cortex-M3 microcontroller.
 *
 */

#include "reg_io.h"
#include "tusb9260_types.h"
#include "vim_nvic.h"

#define ENABLE_NESTING  1

/*****************************************************************************
 * Function: NVIC_init
 *************************************************************************//**
 * This function initializes the Nested Vectored Interrupt Controller (NVIC)
 * for use with the Vectored Interrupt Manager (VIM).
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void NVIC_init(void)
{
    /* Setup NVIC Interrupt Priority Requirements */
    /* This is hard-coded per the VIM documentation */
    // Note: These settings are only relevant when nesting is enabled in VIM.
    WRITE_REG32(NVIC_INT_PRIORITY_REG0, 0x80A0C0E0);
    WRITE_REG32(NVIC_INT_PRIORITY_REG1, 0x00204060);

    /* Enable Fault Interrupts */
    WRITE_REG32(NVIC_SYS_HAND_CTL_STATE_REG, 0x00070000);

    /* Enable the 8 INTISR inputs to the NVIC */
    WRITE_REG32(NVIC_IRQ_0_31_SET_EN_REG, 0x000000FF);

    return;
}

#define DEFAULT_INTR_ENABLE_SET0  0x00600F53  /* RTI compare 0 - 2, RTI Overflow 0 & 1, GIO A, AHCI, and USB. (Channel 0 and 1 are non-maskable) */
#define DEFAULT_INTR_ENABLE_SET1  0x00000040  /* Memory Wrap Window Error */

#define VIM_NEST_CTRL_ENABLE    0xA

/*****************************************************************************
 * Function: VIM_init
 *************************************************************************//**
 * This function initializes the Vectored Interrupt Manager (VIM).
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void VIM_init(void)
{
    /* Setup NVIC */
    NVIC_init();

    /* Disable all VIM Channels */
    WRITE_REG32(VIM_REQMASKCLR0, 0xFFFFFFFF);
    WRITE_REG32(VIM_REQMASKCLR1, 0xFFFFFFFF);

    /* Set all Interrupts as INTISR type. (Default)  Channels [1:0] are read-only (always INTNMI type)  */
    WRITE_REG32(VIM_NMIPR0, 0x00000000);
    WRITE_REG32(VIM_NMIPR1, 0x00000000);

#if ENABLE_NESTING
    // Enable nesting.
    WRITE_REG32(VIM_NEST_CTRL, VIM_NEST_CTRL_ENABLE);
#endif

#if 0
    // Remap interrupt request channel 4 to channel 20.
    MODIFY_REG32(VIM_CHANCTRL(1), 0xFF000000, 0x14000000);       
    MODIFY_REG32(VIM_CHANCTRL(5), 0xFF000000, 0x04000000);      

#endif

    // Remap interrupt request channel 22 (SATA Interface Error Interrupt) to channel 5.
    MODIFY_REG32(VIM_CHANCTRL(1), 0x00FF0000, 0x00160000);       
    // Remap interrupt request channel 5 (RTI Compare 1) to channel 22.
    MODIFY_REG32(VIM_CHANCTRL(5), 0x0000FF00, 0x00000500);  

    // Remap interrupt request channel 20 (AHCI SATA Interrupt) to channel 6.
    MODIFY_REG32(VIM_CHANCTRL(1), 0x0000FF00, 0x00001400);  
    // Remap interrupt request channel 6 (RTI Compare 2) to channel 10.
    MODIFY_REG32(VIM_CHANCTRL(2), 0x0000FF00, 0x00000600);       
    // Remap interrupt request channel 10 (RSVD) to channel 20.
    MODIFY_REG32(VIM_CHANCTRL(5), 0xFF000000, 0x0A000000);       

    /* Enable required VIM Channels (Channel 0 and 1 are non-maskable) */
    WRITE_REG32(VIM_REQMASKSET0, DEFAULT_INTR_ENABLE_SET0);   /* INT  0 - 31 */

#if DEBUG_LEVEL >= 1
    WRITE_REG32(VIM_REQMASKSET0, 0x00040000);   /* SCI 0 (Ch 18) */   
#endif

#if !AHCI_LINK_POWER_MGMT_ENABLE
    WRITE_REG32(VIM_REQMASKSET0, 0x00000020);   /* SATA interface error (Remapped to Ch 5) */   
#endif

    WRITE_REG32(VIM_REQMASKSET1, DEFAULT_INTR_ENABLE_SET1);   /* INT 32 - 63 */

    return;
}


