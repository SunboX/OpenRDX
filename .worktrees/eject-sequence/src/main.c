/*! @file
 * @brief Standalone RDX eject-button and motor test.
 */

#include "eject_test.h"

static uint32_t motor_pwm_period_ticks;

static void eject_motor_stop(void);

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
 * @brief Wait for a microsecond interval using wrap-safe RTI arithmetic.
 *
 * @param duration_us Interval in microseconds.
 */
static void eject_delay_us(uint32_t duration_us)
{
    uint32_t start;

    start = RTIFRC0;
    while ((uint32_t)(RTIFRC0 - start) < duration_us) {
    }
}

/**
 * @brief Wait for a millisecond interval using wrap-safe RTI arithmetic.
 *
 * @param duration_ms Interval in milliseconds.
 */
static void eject_delay_ms(uint32_t duration_ms)
{
    uint32_t start;

    start = RTIFRC1;
    while ((uint32_t)(RTIFRC1 - start) < duration_ms) {
    }
}

/**
 * @brief Configure the two polling timers used by the eject sequence.
 *
 * @param clock_mhz Peripheral clock frequency in megahertz.
 */
static void eject_timer_init(uint32_t clock_mhz)
{
    RTIGCTRL = 0UL;
    RTISETINT = 0UL;

    RTIUC0 = 0UL;
    RTIFRC0 = 0UL;
    RTICPUC0 = clock_mhz - 1UL;

    RTIUC1 = 0UL;
    RTIFRC1 = 0UL;
    RTICPUC1 = (clock_mhz * 1000UL) - 1UL;

    RTIGCTRL = 3UL;
}

/**
 * @brief Configure only GPIO1, GPIO3, PWM1, and the polling timers.
 */
static void eject_hardware_init(void)
{
    uint32_t clock_mhz;

    clock_mhz = ((DEVICE_ID >> 17) == 0UL) ? 75UL : 40UL;
    motor_pwm_period_ticks = clock_mhz * MOTOR_PWM_PERIOD_US;

    GIOGCR0 = 1UL;
    GIOOUT0 |= MOTOR_CONTROL_MASK;
    GIODIR0 &= ~EJECT_BUTTON_MASK;
    GIODIR0 |= MOTOR_CONTROL_MASK;
    GIOPULDIS0 |= EJECT_BUTTON_MASK | MOTOR_CONTROL_MASK;

    PWM1_PCR = PWM_PCR_FREE_RUN_BIT;
    PWM1_PH1D = 0UL;
    PWM1_CFG = PWM_CFG_PHASE1_OUTPUT_HIGH;
    PWM1_PER = motor_pwm_period_ticks;

    eject_timer_init(clock_mhz);
}

/**
 * @brief Apply the recovered GPIO3-low then PWM1-full motor-start pattern.
 */
static void eject_motor_start(void)
{
    GIOOUT0 &= ~MOTOR_CONTROL_MASK;
    eject_delay_us(MOTOR_DEAD_TIME_US);
    PWM1_CFG = PWM_CFG_PHASE1_OUTPUT_HIGH | PWM_CFG_MODE_CONTINUOUS;
    PWM1_START = PWM_START_BIT;
    PWM1_PER = motor_pwm_period_ticks;
    PWM1_PH1D = motor_pwm_period_ticks;
}

/**
 * @brief Run one fixed two-second motor pulse without mechanism checks.
 */
static void eject_run_motor_pulse(void)
{
    eject_motor_start();
    eject_delay_ms(MOTOR_RUN_TIME_MS);
    eject_motor_stop();
}

/**
 * @brief Apply the recovered PWM1-off then GPIO3-high motor-stop pattern.
 */
static void eject_motor_stop(void)
{
    PWM1_PH1D = 0UL;
    PWM1_CFG &= ~PWM_CFG_MOTOR_BITS;
    eject_delay_us(MOTOR_DEAD_TIME_US);
    GIOOUT0 |= MOTOR_CONTROL_MASK;
}

/**
 * @brief Poll the button and run one eject pulse for each new press.
 */
void main(void)
{
    eject_hardware_init();
    for (;;) {
        while (eject_button_pressed() == 0UL) {
        }
        eject_run_motor_pulse();
        while (eject_button_pressed() != 0UL) {
        }
    }
}
