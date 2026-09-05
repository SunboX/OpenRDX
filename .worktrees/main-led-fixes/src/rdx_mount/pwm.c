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
// Description : Pulse-Width Modulation module driver.
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
 * This file contains the implementation of the Pulse-Width Modulation (PWM) module driver.
 * 
 * There are 2 independantly configurable PWMs.
 *  
 */

#include "pwm.h"
#include "reg_io.h"
#include "rti.h"
#include "tusb9260_types.h"


#define PWM_PERIOD_MS  4  /* 4 ms = 250 Hz (good rate for LEDs) */ 
#define PWM_TOTAL_PERIOD  ((rti_clock_mhz * 1000) * PWM_PERIOD_MS)


/*****************************************************************************
 * Function: pwm_set_duty_cycle
 *************************************************************************//**
 * This function sets the duty cycle for a PWM.
 *
 * @param[in] pwm_num number of the PWM [0-1].
 * @param[in] duty_cycle_percentage the duty cycle in percent [0-100].
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void pwm_set_duty_cycle(UINT32_T pwm_num, UINT32_T duty_cycle_percentage)
{
    UINT32_T period_ticks;

    if (duty_cycle_percentage > 100U)
    {
        duty_cycle_percentage = 100U;
    }

    /* Preserve the period selected by the caller instead of assuming an LED. */
    period_ticks = READ_REG32(PWM_PER_REG_OFF(pwm_num));
    WRITE_REG32(PWM_PH1D_REG_OFF(pwm_num),
                (period_ticks / 100U) * duty_cycle_percentage);

    return;
}

/*****************************************************************************
 * Function: pwm_start
 *************************************************************************//**
 * This function starts a PMW. It can also be used to restart PWM for one-shot usage.
 *
 * @param[in] pwm_num number of the PWM [0-1].
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void pwm_start(UINT32_T pwm_num)
{
    // Turn on PWM.
    WRITE32(PWM_START_REG_OFF(pwm_num), PWM_START_BIT);

    return;
}

/*****************************************************************************
 * Function: pwm_disable
 *************************************************************************//**
 * This function disables a PWM and drives its programmed high duration to
 * zero. On the Tandberg RDX board PWM0 is the fan control output.
 *
 * @param[in] pwm_num number of the PWM [0-1].
 *
 * @retval None.
 *
 *****************************************************************************
 */

void pwm_disable(UINT32_T pwm_num)
{
    WRITE32(PWM_PH1D_REG_OFF(pwm_num), 0U);
    WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_IDLE);

    return;
}

/*****************************************************************************
 * Function: pwm_run
 *************************************************************************//**
 * Configure and start an RDX PWM output with period-aware duty calculation.
 * The period is converted from microseconds to peripheral-clock ticks without
 * subtracting one. PH1D is calculated after integer division by 100 so the
 * register programming stays deterministic for periods that are not an exact
 * multiple of one hundred ticks.
 *
 * @param[in] pwm_num number of the PWM [0-1].
 * @param[in] duty_cycle_percentage requested duty cycle in percent [0-100].
 * @param[in] period_us PWM period in microseconds.
 *
 * @retval None.
 *
 *****************************************************************************
 */

void pwm_run(UINT32_T pwm_num, UINT32_T duty_cycle_percentage, UINT32_T period_us)
{
    UINT32_T period_ticks;

    if (duty_cycle_percentage > 100U)
    {
        duty_cycle_percentage = 100U;
    }

    period_ticks = rti_clock_mhz * period_us;

    WRITE32(PWM_PCR_REG_OFF(pwm_num), PWM_PCR_FREE_RUN_BIT);
    WRITE32(PWM_CFG_REG_OFF(pwm_num), PWM_CFG_RDX_RUNNING);
    WRITE32(PWM_START_REG_OFF(pwm_num), PWM_START_BIT);
    WRITE32(PWM_PER_REG_OFF(pwm_num), period_ticks);
    WRITE32(PWM_PH1D_REG_OFF(pwm_num),
            (period_ticks / 100U) * duty_cycle_percentage);

    return;
}

/*****************************************************************************
 * Function: pwm_init
 *************************************************************************//**
 * This function initializes a PWM for continuous mode usage.
 *
 * @param[in] pwm_num number of the PWM [0-1].
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void pwm_init(UINT32_T pwm_num)
{
    pwm_run(pwm_num, 100U, PWM_PERIOD_MS * 1000U);

    return;
}

#define DUTY_TABLE_SZ  30
UINT32_T duty_tbl[DUTY_TABLE_SZ] = 
{
    4096, 3950, 3800, 3600, 3300, 
    2896, 2500, 2048, 1448, 1024, 
    724, 512, 362, 255, 180, 
    128, 90, 64, 45, 32, 
    23, 16, 12, 8, 6, 
    4, 3, 2, 1, 0
};

/*****************************************************************************
 * Function: pwm_fade
 *************************************************************************//**
 * This function increments or decrements the duty cycle for a PWM
 * causing it to rise and fall when called periodically. It is used to 
 * make an LED fade in and out. Percieved LED brightness vs. PWM duty cycle   
 * will vary depending on the LED and resistor used so this function may have 
 * to be modified depending on the application.
 *
 * @param[in] pwm_num number of the PWM [0-1].
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void pwm_fade(UINT32_T pwm_num)
{
    static UINT32_T index = 0;
    static BOOLEAN_T increase_duty_cycle = FALSE;
    UINT32_T max_val = duty_tbl[0];
    UINT32_T high_duration;
    UINT32_T period_ticks;

    period_ticks = READ_REG32(PWM_PER_REG_OFF(pwm_num));

    if (increase_duty_cycle)
    {
        high_duration = ((max_val - duty_tbl[--index]) * period_ticks) / max_val;
        if (index == 0) increase_duty_cycle = FALSE;
    }
    else /* decrease duty cycle */
    {
        high_duration = ((max_val - duty_tbl[index++]) * period_ticks) / max_val;
        if (index == DUTY_TABLE_SZ) increase_duty_cycle = TRUE;
    }

    // Set duty cycle.
    WRITE_REG32(PWM_PH1D_REG_OFF(pwm_num), high_duration);

    return;
}


/*****************************************************************************
 * Function: pwm_toggle
 *************************************************************************//**
 * This function flips the duty cycle of a PWM to 100% if currently 0% and 
 * to 0% if non-zero.  
 *
 * @param[in] pwm_num number of the PWM [0-1].
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void pwm_toggle(UINT32_T pwm_num)
{
    UINT32_T high_period;

    high_period = READ_REG32(PWM_PH1D_REG_OFF(pwm_num));

    if (high_period == 0)
    {
        pwm_set_duty_cycle(pwm_num, 100);
    }
    else
    {
        pwm_set_duty_cycle(pwm_num, 0);
    }

    return;
}

