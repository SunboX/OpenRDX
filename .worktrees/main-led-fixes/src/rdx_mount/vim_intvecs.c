/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : vim_intvecs.c
//
// Project     : TUSB926x Firmware.
//
// Description : VIM interrupt vectors.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   07/15/09 - Brian Quach - Updated interrupt assignments.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the interrupt vector assignments for the Vectored Interrupt Manager (VIM).
 *
 */

#include "ahci.h"
#include "gio.h"
#include "mww.h"
#include "rti.h"
#include "sci.h"
#include "system.h"
#include "tusb9260_types.h"
#include "usb_hal.h"


void default_isr(void)
{
#if DEBUG_LEVEL >= 1
    /* Copy stacked PC into R0 */
//    asm(" mrs r0, msp");
//    asm(" ldr r0, [r0,#24]");
    
    CRIT("@Error: Default ISR!\n");
    while (1);
#else
    system_reset();
#endif
}

/* This will place the VIM Table at address specified in the linker file (0x00000040) */
#pragma DATA_SECTION( M3_VIM_Table, ".m3vim_table" );

/** This array defines the interrupt vector assignments for
 *  the Vectored Interrupt Manager (VIM) */
const UINT32_T M3_VIM_Table[48] =
{
    (UINT32_T) &default_isr,         /* 0  Reserved for ESM High Level Interrupt*/
    (UINT32_T) &default_isr,         /* 1  Reserved for NMI Interrupt */
    (UINT32_T) &default_isr,         /* 2  Reserved for ESM Low Level Interrupt*/
    (UINT32_T) &default_isr,         /* 3  System Software Interrupt (SSI)*/

    (UINT32_T) &rti_compare0_isr,    /* 4  RTI Compare 0 */
    (UINT32_T) &ahci_rx_error_isr,   /* 22 SATA Interface Error Interrupt */

    (UINT32_T) &ahci_isr,            /* 20 AHCI SATA Interrupt */    
    (UINT32_T) &default_isr,         /* 7  RTI Compare 3 */
    (UINT32_T) &rti_overflow0_isr,   /* 8  RTI Overflow 0 */
    (UINT32_T) &rti_overflow1_isr,   /* 9  RTI Overflow 1 */
    (UINT32_T) &rti_compare2_isr,    /* 6  RTI Compare 2 */
                                    
    (UINT32_T) &gio_isr,             /* 11 GIO Interrupt A */
    (UINT32_T) &default_isr,         /* 12 GIO Interrupt B */
                                    
    (UINT32_T) &default_isr,         /* 13 RSVD */
    (UINT32_T) &default_isr,         /* 14 RSVD */                                     
    (UINT32_T) &default_isr,         /* 15 RSVD */
    (UINT32_T) &default_isr,         /* 16 RSVD */
    (UINT32_T) &default_isr,         /* 17 RSVD */

#if DEBUG_LEVEL >= 1
    (UINT32_T) &sci_isr,             /* 18 SCI Interrupt 0 */
#else
    (UINT32_T) &default_isr,         /* 18 SCI Interrupt 0 */
#endif    

    (UINT32_T) &default_isr,         /* 19 SCI Interrupt 1 */
    (UINT32_T) &default_isr,         /* 10 RSVD */
    (UINT32_T) &usb_hal_isr,         /* 21 USB Interrupt */
    (UINT32_T) &rti_compare1_isr,    /* 5  RTI Compare 1 */

    (UINT32_T) &default_isr,         /* 23 RSVD */
    (UINT32_T) &default_isr,         /* 24 RSVD */
    (UINT32_T) &default_isr,         /* 25 RSVD */
    (UINT32_T) &default_isr,         /* 26 RSVD */
    (UINT32_T) &default_isr,         /* 27 RSVD */
    (UINT32_T) &default_isr,         /* 28 RSVD */
    (UINT32_T) &default_isr,         /* 29 RSVD */
    (UINT32_T) &default_isr,         /* 30 RSVD */
    (UINT32_T) &default_isr,         /* 31 RSVD */

    (UINT32_T) &default_isr,         /* 32 SPI Interrupt 0 */
    (UINT32_T) &default_isr,         /* 33 SPI Interrupt 1 */
                                    
    (UINT32_T) &default_isr,         /* 34 RSVD */
    (UINT32_T) &default_isr,         /* 35 RSVD */
                                    
    (UINT32_T) &default_isr,         /* 36 PWM0 Interrupt */
    (UINT32_T) &default_isr,         /* 37 PWM1 Interrupt */

    (UINT32_T) &mww_error_isr,       /* 38 Memory Wrap Window Error Interrupt. */
    (UINT32_T) &default_isr,         /* 39 RSVD */
    (UINT32_T) &default_isr,         /* 40 RSVD */
    (UINT32_T) &default_isr,         /* 41 RSVD */
    (UINT32_T) &default_isr,         /* 42 RSVD */
    (UINT32_T) &default_isr,         /* 43 RSVD */
    (UINT32_T) &default_isr,         /* 44 RSVD */
    (UINT32_T) &default_isr,         /* 45 RSVD */
    (UINT32_T) &default_isr,         /* 46 RSVD */
    (UINT32_T) &default_isr          /* 47 RSVD */

};

