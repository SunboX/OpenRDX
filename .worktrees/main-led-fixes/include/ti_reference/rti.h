/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : rti.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for Real Time Interrupt module driver.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   05/12/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * Header file for the Real-Time Interrupt module driver.
 *  
 */

#ifndef _RTI_H_
#define _RTI_H_

#include "tusb9260.h"
#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define RTI0_PRESCALE (rti_clock_mhz - 1)          /* yields 1 MHz frequency */
#define RTI1_PRESCALE ((rti_clock_mhz * 1000) - 1) /* yields 1 kHz frequency */

#define RTI_COMP0_INTERRUPT_PERIOD 500 /* in ms */
#define RTI_COMP1_INTERRUPT_PERIOD 250 /* in ms */
#define RTI_COMP2_INTERRUPT_PERIOD 100 /* in ms */

#define RTI_COMP0_INTERRUPT_ENABLE 1
#define RTI_COMP1_INTERRUPT_ENABLE 1
#define RTI_COMP2_INTERRUPT_ENABLE 1

#define RTI_COMPARE0_INT_FLAG 0x1
#define RTI_COMPARE1_INT_FLAG 0x2
#define RTI_COMPARE2_INT_FLAG 0x4

#define RTIGCTRL_REG_OFF 0xFFFFFC00
#define RTITBCTRL_REG_OFF 0xFFFFFC04
#define RTICAPCTRL_REG_OFF 0xFFFFFC08
#define RTICOMPCTRL_REG_OFF 0xFFFFFC0C
#define RTIFRC0_REG_OFF 0xFFFFFC10
#define RTIUC0_REG_OFF 0xFFFFFC14
#define RTICPUC0_REG_OFF 0xFFFFFC18
#define RTICAFRC0_REG_OFF 0xFFFFFC20
#define RTICAUC0_REG_OFF 0xFFFFFC24
#define RTIFRC1_REG_OFF 0xFFFFFC30
#define RTIUC1_REG_OFF 0xFFFFFC34
#define RTICPUC1_REG_OFF 0xFFFFFC38
#define RTICAFRC1_REG_OFF 0xFFFFFC40
#define RTICAUC1_REG_OFF 0xFFFFFC44
#define RTICOMP0_REG_OFF 0xFFFFFC50
#define RTIUDCP0_REG_OFF 0xFFFFFC54

#define RTI_COMP_REG_OFF(num) (RTICOMP0_REG_OFF + ((num) * 0x08))
#define RTI_UDCP_REG_OFF(num) (RTIUDCP0_REG_OFF + ((num) * 0x08))

#define RTITBLCOMP_REG_OFF 0xFFFFFC70
#define RTITBHCOMP_REG_OFF 0xFFFFFC74
#define RTISETINT_REG_OFF 0xFFFFFC80
#define RTICLRINT_REG_OFF 0xFFFFFC84 // Auto-clear of interrupts is non-functional. Do not use.
#define RTIINTFLAG_REG_OFF 0xFFFFFC88
#define RTIDWDCTRL_REG_OFF 0xFFFFFC90
#define RTIDWDPRLD_REG_OFF 0xFFFFFC94
#define RTIWDSTATUS_REG_OFF 0xFFFFFC98
#define RTIWDKEY_REG_OFF 0xFFFFFC9C
#define RTIDWDCNTR_REG_OFF 0xFFFFFCA0
#define RTIWWDRXNCTRL_REG_OFF 0xFFFFFCA4
#define RTIWWDSIZECTRL_REG_OFF 0xFFFFFCA8
#define RTIINTCLRENABLE_REG_OFF 0xFFFFFCAC
#define RTICOMP0CLR_REG_OFF 0xFFFFFCB0
#define RTICOMP1CLR_REG_OFF 0xFFFFFCB4
#define RTICOMP2CLR_REG_OFF 0xFFFFFCB8
#define RTICOMP3CLR_REG_OFF 0xFFFFFCBC

#define RTIGCTRL_CNT1_ENABLE_BIT 0x2
#define RTIGCTRL_CNT0_ENABLE_BIT 0x1

#define RTI_FRC0_OVERFLOW_INT_BIT 0x00020000
#define RTI_FRC1_OVERFLOW_INT_BIT 0x00040000

/*----------------------------------------------------------------------------+
| Externs                                                                     |
+----------------------------------------------------------------------------*/

/*****************************************************************************
 * Function: rti_get_time
 *************************************************************************//**
 * This function returns the current free-running counter value of timer 1.
 *
 * @param None.
 *                    
 * @return The counter value (millisecond resolution).
 *
 ****************************************************************************** 
 */

extern inline UINT32_T rti_get_time(void) { return *(volatile UINT32_T *)(RTIFRC1_REG_OFF); }

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void rti_init(void);
void rti_config_compare_interrupt(UINT32_T compare_num, UINT32_T period);

void usleep(UINT32_T usec);
void msleep(UINT32_T msec);

void rti_overflow0_isr(void);
void rti_overflow1_isr(void);

void rti_compare0_isr(void);
void rti_compare1_isr(void);
void rti_compare2_isr(void);

#endif
