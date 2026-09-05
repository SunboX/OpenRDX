/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : system_init.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the system init module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the system init module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief System init.
 *
 * @return No value.
 */
void system_init()

{
    /* System init using only caller-provided data. */
    /* Program CLKCNTL REG OFF with 0; write ordering is hardware-significant. */
    CLKCNTL_REG_OFF = 0;
    CLKCNTL_REG_OFF = 0x100;
    PCR_PSPWRDWNCLR0 = 0xffffffff;
    PCR_PSPWRDWNCLR1 = 0xffffffff;
    PCR_PSPWRDWNCLR2 = 0xffffffff;
    PCR_PSPWRDWNCLR3 = 0xffffffff;
    PCR_PPROTCLR0 = 0xffffffff;
    return;
}
