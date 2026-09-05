/*! @file
 * @brief Minimal reset and clock startup for the standalone fan test.
 */

#include "fan_test.h"

/**
 * @brief Enable the peripheral clock domains needed by GPIO, PWM, and RTI.
 */
void fan_test_system_init(void)
{
    CLKCNTL = 0UL;
    CLKCNTL = 0x100UL;
    PCR_PSPWRDWNCLR0 = 0xFFFFFFFFUL;
    PCR_PSPWRDWNCLR1 = 0xFFFFFFFFUL;
    PCR_PSPWRDWNCLR2 = 0xFFFFFFFFUL;
    PCR_PSPWRDWNCLR3 = 0xFFFFFFFFUL;
    PCR_PPROTCLR0 = 0xFFFFFFFFUL;
}

/**
 * @brief Initialize the Cortex-M3 stack, vector base, and required clocks.
 */
void c_int00(void)
{
    asm(" mov r0,#0x08000000");
    asm(" ldr r1,[r0]");
    asm(" msr msp,r1");

    SCB_VTOR = 0x08000000UL;
    SYSTEM_CTRL = 0UL;
    fan_test_system_init();
    main();

    for (;;) {
    }
}
