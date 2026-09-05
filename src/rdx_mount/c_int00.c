/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : c_int00.c
//
// Project     : TUSB926x Firmware.
//
// Description : Initial entry point for firmware.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   06/05/09 - Brian Quach - Added global variable initialization.
//
//=======================================================================================
 
/*! @file
 * 
 * This file contains the initial entry point for firmware execution.
 * 
 */

#include "system.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "vim_nvic.h"


// Extern references to symbols defined by the linker command file.
extern UINT32_T *__cinit__;
extern UINT32_T *_STACK_SIZE;
extern UINT32_T *_stack;

#define STACK_SIZE ((UINT32_T)&_STACK_SIZE)
#define STACK_START ((void *)&_stack)       // This is the bottom of the stack (not the initial stack pointer).


/*****************************************************************************
 * Function: init_globals.
 *************************************************************************//**
 * This function initializes global variables by coping data 
 * from .cinit initialization tables into .bss section.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void init_globals(void)
{
    UINT32_T *table = (UINT32_T *)&__cinit__;
    UINT32_T length;
    UINT8_T *address; 
    UINT8_T *data;

    // No initialization data if start of table is -1.
    if (table != (UINT32_T *)0xFFFFFFFF)
    {
        length = *table++;

        // Copy data from .cinit initialization tables into .bss section.
        while (length != 0)
        {
            address = (UINT8_T *)*table++;
            data = (UINT8_T *)table;

            while (length > 0)
            {
                *address++ = *data++; 
                length--;
            }

            /* Realign cinit pointer to point to next entry */
            table = (UINT32_T *)(((UINT32_T)data + 3) & ~3);
            length = *table++;
        } 
    }

    return;
}


/*****************************************************************************
 * Function: c_int00
 *************************************************************************//**
 * This function is the intial entry point for firmware. It initializes
 * global variables, enables peripherials & interrupts, and calls main().
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void c_int00(void)
{   
#if DEBUG_LEVEL >= 1   
    UINT32_T i;
    UINT32_T *pStack = (UINT32_T*)STACK_START;

    // Fill stack space with fixed patternt to allow us to check for stack overflow.
    for (i = 0; i < (STACK_SIZE / 4); i++)
    {
        pStack[i] = 0xBBBBBBBB;
    }
#endif

    // Set MSP.
    asm(" mov r0,#0x08000000");
    asm(" ldr r1,[r0]");
    asm(" msr msp,r1");
 
    // Set NVIC offset.
    *(volatile UINT32_T *)0xE000ED08 = 0x08000000;

    // Initialize global variables.
    init_globals();

    // Enable system peripherials
    system_init(); 

    // Enable system interrupts (NVIC and VIM init)
    VIM_init();

    /*Switch to Unprivileged Mode*/  // BQ - we need to be in priviledged mode for power management.
    //asm(" mov   r0,#0x01");
    //asm(" msr   CONTROL,r0");

    // Call main().  
    main();

}
