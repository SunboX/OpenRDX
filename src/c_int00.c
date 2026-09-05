/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : c_int00.c
// Project     : TUSB9261 RDX Firmware
// Description : Reset entry point from 0x0800B944.
//=======================================================================================

#include "../include/rdx_firmware.h"

/**
 * @brief Initialize the Cortex-M3 execution environment and enter firmware main.
 *
 * @details Loads MSP from the vector table, configures VTOR, initializes the
 * system and interrupt controller, and then enters the firmware main routine.
 */
void c_int00(void) {
    /* Load the initial main-stack pointer from vector word zero. */
    asm(" mov r0,#0x08000000");
    asm(" ldr r1,[r0]");
    asm(" msr msp,r1");

    /* Configure the NVIC vector-table offset register. */
    scb_vector_table_offset_register = 0x08000000u;

    system_init();
    VIM_init();
    main();

    /* The firmware main loop is not expected to return. */
    for (;;) {
    }
}
