/*! @file
 * @brief Standalone RDX eject-button and fan test.
 */

#include "fan_test.h"

static uint32_t fan_pwm_period_ticks;

/**
 * @brief Return nonzero while the active-low eject button is pressed.
 *
 * @return Nonzero when GPIO1 is low.
 */
static uint32_t eject_button_pressed(void)
{
    return (GIOIN0 & EJECT_BUTTON_MASK) == EJECT_BUTTON_ASSERTED;
}

/**
 * @brief Wait for a millisecond interval using wrap-safe RTI arithmetic.
 *
 * @param duration_ms Interval in milliseconds.
 */
static void fan_delay_ms(uint32_t duration_ms)
{
    uint32_t start;

    start = RTIFRC1;
    while ((uint32_t)(RTIFRC1 - start) < duration_ms) {
    }
}

/**
 * @brief Configure RTI counter one to advance once per millisecond.
 *
 * @param clock_mhz Peripheral clock frequency in megahertz.
 */
static void fan_timer_init(uint32_t clock_mhz)
{
    RTIGCTRL = 0UL;
    RTISETINT = 0UL;
    RTIUC1 = 0UL;
    RTIFRC1 = 0UL;
    RTICPUC1 = (clock_mhz * 1000UL) - 1UL;
    RTIGCTRL = RTI_COUNTER_ONE_ENABLE;
}

/**
 * @brief Configure only GPIO1, PWM0, and the polling timer.
 */
static void fan_hardware_init(void)
{
    uint32_t clock_mhz;

    clock_mhz = ((DEVICE_ID >> 17) == 0UL) ? 75UL : 40UL;
    fan_pwm_period_ticks = clock_mhz * FAN_PWM_PERIOD_US;

    GIOGCR0 = 1UL;
    GIODIR0 &= ~EJECT_BUTTON_MASK;
    GIOPULDIS0 |= EJECT_BUTTON_MASK;

    PWM0_PCR = PWM_PCR_FREE_RUN_BIT;
    PWM0_PER = fan_pwm_period_ticks - 1UL;
    PWM0_PH1D = 0UL;
    PWM0_CFG = 0UL;

    fan_timer_init(clock_mhz);
}

/**
 * @brief Drive the recovered active-high PWM0 fan output at full duty.
 */
static void fan_start(void)
{
    PWM0_PER = fan_pwm_period_ticks - 1UL;
    PWM0_PH1D = fan_pwm_period_ticks;
    PWM0_CFG = PWM_CFG_FAN_RUNNING;
    PWM0_START = PWM_START_BIT;
}

/**
 * @brief Turn the PWM0 fan output off.
 */
static void fan_stop(void)
{
    PWM0_PH1D = 0UL;
    PWM0_CFG &= ~PWM_CFG_FAN_BITS;
}

/**
 * @brief Run one fixed one-second fan pulse without additional checks.
 */
static void fan_run_pulse(void)
{
    fan_start();
    fan_delay_ms(FAN_RUN_TIME_MS);
    fan_stop();
}

/**
 * @brief Poll the button and run one fan pulse for each new press.
 */
void main(void)
{
    fan_hardware_init();
    for (;;) {
        while (eject_button_pressed() == 0UL) {
        }
        fan_run_pulse();
        while (eject_button_pressed() != 0UL) {
        }
    }
}
