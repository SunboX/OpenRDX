/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
 */

//=======================================================================================
// Filename    : system.h
//
// Project     : TUSB926x Firmware.
//
// Description : This header file defines global peripherial and system control registers.
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
 * This header file defines global peripherial and system control registers.
 *
 */

#include "usb_hal.h"

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define ENABLE_CLOCK_GATING 1 // (Default = 1) Set to 0 to disable dynamic clock gating.

/* System Regs */
#define SYS_CLKCNTRL 0xFFFFFFD0
#define CDDIS_REG_OFF 0xFFFFFF3C
#define DIEIDL_REG_OFF 0xFFFFFF7C
#define DIEIDH_REG_OFF 0xFFFFFF80
#define CLKCNTL_REG_OFF 0xFFFFFFD0
#define SYSECR_REG_OFF 0xFFFFFFE0
#define SYSESR_REG_OFF 0xFFFFFFE4
#define DEVID_REG_OFF 0xFFFFFFF0

/* Peripherial Regs */
#define PCR_PPROTCLR0 0xFFFFE040
#define PCR_PSPWRDWNCLR0 0xFFFFE0A0
#define PCR_PSPWRDWNCLR1 0xFFFFE0A4
#define PCR_PSPWRDWNCLR2 0xFFFFE0A8
#define PCR_PSPWRDWNCLR3 0xFFFFE0AC

/* General Purpose Reg 1 */
#define GPREG1_REG_OFF 0xFFFFFFA0
#define GPREG1_USB2_DATA_POLARITY_BIT 0x00000008
#define GPREG1_USB3_POWER_PRESENT_DISABLE_BIT 0x00000004
#define GPREG1_USB3_SSC_DISABLE_BIT 0x00000002
#define GPREG1_SATA_SSC_DISABLE_BIT 0x00000001

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void system_init(void);

void system_reset(void);

/*----------------------------------------------------------------------------+
| Inline Functions                                                            |
+----------------------------------------------------------------------------*/

#define CDDIS_GCLK_OFF                                                                                                                     \
    0x01 /* The GCLK domain is disabled when processor is in sleep mode
and no interrupt event is active */
#define CDDIS_HCLK_OFF                                                                                                                     \
    0x02                     /* The HCLK domain is disabled when processor is in sleep mode
and no interrupt event is active */
#define CDDIS_VCLKP_OFF 0x04 /* VCLKP domain is disabled */

#define CDDIS_M3RDG_OFF 0x08   /* Cortex-M3 RAM/ROM dynamic clock gating disabled */
#define CDDIS_M3BMMDG_OFF 0x10 /* Cortex-M3 RAM/ROM BMM2 dynamic clock gating disabled */
#define CDDIS_A2VDG_OFF 0x20   /* AHB to VBUS dynamic clock gating disabled */

#define CDDIS_PWM_SLEEP_OFF 0x200 /* PWM will always run (even when processor is in sleep state) */

#define CDDIS_DEFAULT_CLOCK_GATING_BITS (CDDIS_M3RDG_OFF | CDDIS_M3BMMDG_OFF | CDDIS_A2VDG_OFF | CDDIS_PWM_SLEEP_OFF)
#define CDDIS_ENABLE_CLOCK_GATING_BITS (CDDIS_GCLK_OFF | CDDIS_HCLK_OFF | CDDIS_PWM_SLEEP_OFF) // Leave VCLKP on.

/* Note: VCLKP can be disabled if the heartbeat LED, GIO inputs, and debug output are disabled.
 * However, USB 2.0 hot-plug causes a fault and corresponding watchdog timeout. This needs further debug. */

/*****************************************************************************
 * Function: system_enable_clock_gating
 *************************************************************************//**
 * This function enables dynamic clock gating and disables the GCLK and HCLK domains.
 * PCR, PWM, GIO, SPI, and SCI cannot be accessed if VCLKP domain is disabled.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void system_enable_clock_gating(void) {
#if ENABLE_CLOCK_GATING
    // Resume from U3 is broken if clock gating is enabled for TUSB9260.
    if (usb_dev.usb_core_version > 0x101A) {
        WRITE32(CDDIS_REG_OFF, CDDIS_ENABLE_CLOCK_GATING_BITS);
    }
#endif
    return;
}

/*****************************************************************************
 * Function: system_disable_clock_gating
 *************************************************************************//**
 * This function disables clock gating for GCLK, HCLK, and VCLKP domains.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void system_disable_clock_gating(void) {
#if ENABLE_CLOCK_GATING
    WRITE32(CDDIS_REG_OFF, CDDIS_DEFAULT_CLOCK_GATING_BITS);
#endif
    return;
}

#endif
