/*
 * SPDX-FileCopyrightText: 2009-2010 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

/*******************************************************************************************/
/* TUSB9261 OpenRDX memory map and required controller layout.                             */
/* Validate every section and limit for the target hardware before use.                    */
/* TI linker-command content follows.                                                       */

/*******************************************************************************************/
/* TUSB9260_LINK.CMD - COMMAND FILE FOR LINKING TUSB9260 Firmware                          */
/*                                                                                         */
/*   Usage:  cl470 <src files...> -z -o <out file> -m <map file> tusb9260_link.cmd         */
/*                                                                                         */
/* Revision History:                                                                       */
/*   MM/DD/YY                                                                              */
/*   02/25/10 - Brian Quach - Creation.                                                    */
/*                                                                                         */
/*******************************************************************************************/

--rom_model   /* Autoinit vars at run time */
--fill_value=0x0
--stack_size=0x800  /* 2KB */
--heap_size=0x0


/* SYSTEM MEMORY MAP */

MEMORY /* 64 KB total */
{
    VECTORS     : org = 0x08000000   len = 0x00000040   /* INTERRUPT VECTOR MEMORY */
    VIM_TABLE   : org = 0x08000040   len = 0x000000c0   /* M3 VIM TABLE MEMORY */  
    PD_MEM      : org = 0x08000100   len = 0x0000faff   /* TI bridge program/data memory */
    RESERVED    : org = 0x0800fbff   len = 0x00000401   /* upper RAM retained by boot environment */
}

/* SECTIONS ALLOCATION INTO MEMORY */

SECTIONS
{
    /* PROGRAM MEMORY */
    .intvecs     : {} > VECTORS          /* INTERRUPT VECTORS */
    .m3vim_table : {} > VIM_TABLE        /* M3 VIM Table   */
   
    .text        : {} > PD_MEM           /* CODE */
    .cinit       : {} > PD_MEM           /* INITIALIZATION TABLES */
    .const       : {} > PD_MEM           /* CONSTANT DATA */

    /* DATA MEMORY */
    .stack       : { _Stack_pv = . + __STACK_SIZE; } > PD_MEM ALIGN(8) (HIGH) /* SOFTWARE SYSTEM STACK  */
                    /* initial stack pointer value to top of stack */

    .bss         : {} > PD_MEM (HIGH)    /* GLOBAL & STATIC VARS */   
}
