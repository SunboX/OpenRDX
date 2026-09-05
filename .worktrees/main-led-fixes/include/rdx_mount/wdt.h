/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : wdt.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for watchdog timer.
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
 * Header file for the watchdog timer.
 *
 */

#include "rti.h"
#include "tusb9260_types.h"

#ifndef _WDT_H_
#define _WDT_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define WDT_TIMEOUT_MS   445    /* max value is 447 ms w/ RTI clock of 75 MHz. */


#define WDT_MAX_PRELOAD_VAL  0xFFF
#define WDT_PRELOAD_VAL  (((WDT_TIMEOUT_MS * 1000 * rti_clock_mhz) / 0x2000) - 1)

/* WDT Registers */
#define WDT_CTRL_REG_OFF        0xFFFFFC90
#define WDT_PRLD_REG_OFF        0xFFFFFC94
#define WDT_STATUS_REG_OFF      0xFFFFFC98
#define WDT_KEY_REG_OFF         0xFFFFFC9C
#define WDT_CNTR_REG_OFF        0xFFFFFCA0


/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void wdt_start(void);
void wdt_reset(void);


#endif

