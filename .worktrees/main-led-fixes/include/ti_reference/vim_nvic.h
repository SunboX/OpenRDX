/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : vim_nvic.h
//
// Project     : TUSB926x Firmware.
//
// Description : This file defines registers for the Vectored Interrupt Manager (VIM)
// and Nested Vectored Interrupt Controller (NVIC).
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
 * This file defines registers for the Vectored Interrupt Manager (VIM)
 * and Nested Vectored Interrupt Controller (NVIC).
 *
 */

#ifndef _VIM_NVIC_H_
#define _VIM_NVIC_H_

#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

/* NVIC Regs */
#define NVIC_IRQ_0_31_SET_EN_REG 0xE000E100

#define NVIC_INT_PRIORITY_REG0 0xE000E400
#define NVIC_INT_PRIORITY_REG1 0xE000E404

#define NVIC_SYS_HAND_CTL_STATE_REG 0xE000ED24

/* VIM Regs */
#define VIM_NEST_CTRL 0xFFFFFE08
#define VIM_NMIPR0 0xFFFFFE10
#define VIM_NMIPR1 0xFFFFFE14

#define VIM_REQMASKSET0 0xFFFFFE30
#define VIM_REQMASKSET1 0xFFFFFE34

#define VIM_REQMASKCLR0 0xFFFFFE40
#define VIM_REQMASKCLR1 0xFFFFFE44

#define VIM_CHANCTRL0 0xFFFFFE80

#define VIM_CHANCTRL(block_num) (VIM_CHANCTRL0 + (0x04 * (block_num)))

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void VIM_init(void);

#endif
