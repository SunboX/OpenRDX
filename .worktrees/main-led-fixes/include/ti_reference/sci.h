/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : sci.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for Serial Communication Interface module driver.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   06/04/09 - Brian Quach - Added BRSR calculation based on baud rate and RTI clock.
//
//=======================================================================================

/*! @file
 * 
 * Header file for the Serial Communication Interface module driver
 *  
 */

#ifndef _SCI_H_
#define _SCI_H_

#include "tusb9260_types.h"
#include "rti.h" // rti_clock_mhz

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define BAUD_RATE 115200
#define SCI_BRSR_VALUE (UINT32_T)((((rti_clock_mhz * 125000) + BAUD_RATE) / (BAUD_RATE * 2)) - 1)

// SCI registers.
#define SCI_GCR0 0xFFF7E500
#define SCI_GCR1 0xFFF7E504
#define SCI_SETINT 0xFFF7E50C
#define SCI_CLRINT 0xFFF7E510

#define SCI_FLR 0xFFF7E51C

#define SCI_INTVEC0 0xFFF7E520
#define SCI_INTVEC1 0xFFF7E524

#define SCI_CHAR 0xFFF7E528
#define SCI_BAUD 0xFFF7E52C

#define SCI_RD 0xFFF7E534
#define SCI_TD 0xFFF7E538
#define SCI_PIO0 0xFFF7E53C
#define SCI_PIO1 0xFFF7E540
#define SCI_PIO2 0xFFF7E544
#define SCI_PIO3 0xFFF7E548
#define SCI_PIO4 0xFFF7E54C
#define SCI_PIO5 0xFFF7E550
#define SCI_PIO6 0xFFF7E554
#define SCI_PIO7 0xFFF7E558
#define SCI_PIO8 0xFFF7E55C

// GCR0 Bits.
#define SCI_GCR0_RESET (1UL)

// GCR1 Bits.
#define SCI_GCR1_TXENA (1UL << 25)
#define SCI_GCR1_RXENA (1UL << 24)
#define SCI_GCR1_CONT (1UL << 17)
#define SCI_GCR1_LOOPBACK (1UL << 16)
#define SCI_GCR1_POWERDOWN (1UL << 9)
#define SCI_GCR1_SLEEP (1UL << 8)
#define SCI_GCR1_SWNRST (1UL << 7)
#define SCI_GCR1_CLOCK (1UL << 5)
#define SCI_GCR1_STOP (1UL << 4)
#define SCI_GCR1_PARITY (1UL << 3)
#define SCI_GCR1_PARITYENA (1UL << 2)
#define SCI_GCR1_TIMINGMODE (1UL << 1)
#define SCI_GCR1_COMMMODE (1UL)

// PIO Bits.
#define SCI_PIO_TX_GPIO9 (1UL << 2)
#define SCI_PIO_RX_GPIO8 (1UL << 1)
#define SCI_PIO_CLK (1UL)

// Interrupt clear/enable bits.
#define SCI_TX_INT_BIT (1UL << 8)

// Flag register bits.
#define SCI_FLR_TX_RDY_BIT (1UL << 8)
#define SCI_FLR_RX_RDY_BIT (1UL << 9)

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void sci_init(void);

#if DEBUG_LEVEL >= 1

UINT32_T getchar(char *pcInChar);
UINT32_T kprintf(const char *format, ...);
void sci_isr(void);

#endif /* DEBUG_LEVEL >= 1 */

#endif
