/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : pmw.c
//             
// Project     : TUSB926x Firmware.
//             
// Description : Header file for Pulse-Width Modulation module driver.
// 
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   07/29/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * Header file for Pulse-Width Modulation (PWM) module driver.
 *  
 */

#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define PWM_PID_REG_OFF(pwm_num)     (0xFFF78800 + (0x400 * pwm_num))
#define PWM_PCR_REG_OFF(pwm_num)     (0xFFF78804 + (0x400 * pwm_num))
#define PWM_CFG_REG_OFF(pwm_num)     (0xFFF78808 + (0x400 * pwm_num))
#define PWM_START_REG_OFF(pwm_num)   (0xFFF7880C + (0x400 * pwm_num))
#define PWM_RPT_REG_OFF(pwm_num)     (0xFFF78810 + (0x400 * pwm_num))
#define PWM_PER_REG_OFF(pwm_num)     (0xFFF78814 + (0x400 * pwm_num))
#define PWM_PH1D_REG_OFF(pwm_num)    (0xFFF78818 + (0x400 * pwm_num))

#define PWM_PCR_FREE_RUN_BIT  0x1

#define PWM_CFG_MODE_DISABLE     0x0
#define PWM_CFG_MODE_ONE_SHOT    0x1
#define PWM_CFG_MODE_CONTINUOUS  0x2
#define PWM_CFG_INTR_ENABLE      0x40  /* interrupt enable.  interupt occurs once a period */
#define PWM_CFG_ACTIVE_HIGH      0x20  /* inactive output level */
#define PWM_CFG_P1OUT_LEVEL_BIT  0x10  /* phase 1 output level */

/* Keep both PWM pins phase-one-high while the generators are disabled. */
#define PWM_CFG_RDX_IDLE         (PWM_CFG_P1OUT_LEVEL_BIT | PWM_CFG_MODE_DISABLE)
#define PWM_CFG_RDX_RUNNING      (PWM_CFG_P1OUT_LEVEL_BIT | PWM_CFG_MODE_CONTINUOUS)

#define PWM_START_BIT            0x01

/* Fixed periods used by the fan and mechanism control paths. */
#define RDX_FAN_PWM_PERIOD_US     16000U
#define RDX_MOTOR_PWM_PERIOD_US   50U

typedef enum PWM_NUM
{
    RDX_FAN_PWM_NUM             = 0,
    RDX_MOTOR_PWM_NUM           = 1,

    /* Shared USB support code uses these descriptive compatibility aliases. */
    HDD_ACTIVITY_LED_PWM_NUM    = RDX_FAN_PWM_NUM,
    SW_HEARTBEAT_PWM_NUM        = RDX_MOTOR_PWM_NUM
} PWM_NUM_T;

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/


void pwm_init(UINT32_T pwm_num);
void pwm_disable(UINT32_T pwm_num);
void pwm_run(UINT32_T pwm_num, UINT32_T duty_cycle_percentage, UINT32_T period_us);
void pwm_fade(UINT32_T pwm_num);
void pwm_toggle(UINT32_T pwm_num);

void pwm_set_duty_cycle(UINT32_T pwm_num, UINT32_T duty_cycle_percentage);


