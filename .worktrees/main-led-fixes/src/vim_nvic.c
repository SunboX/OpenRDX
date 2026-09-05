/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : vim_nvic.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the vim nvic module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the vim nvic module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief VIM init.
 *
 * @return No value.
 */
void VIM_init()

{
    /* VIM init; persistent state is carried in VIM request mask clear zero register, VIM request mask clear one register and related record fields. */
    nvic_init();
    /* Program VIM request mask clear zero register with 0xffffffff; write ordering is hardware-significant. */
    vim_request_mask_clear_zero_register = 0xffffffff;
    vim_request_mask_clear_one_register = 0xffffffff;
    /* Program VIM nmi priority zero register with 0; write ordering is hardware-significant. */
    vim_nmi_priority_zero_register = 0;
    vim_nmi_priority_one_register = 0;
    /* Program VIM nesting control register with 10; write ordering is hardware-significant. */
    vim_nesting_control_register = 10;
    rti_update_masked_register_bits(0xfffffe84, 0xff0000, 0x160000);
    rti_update_masked_register_bits(0xfffffe94, 0xff00, 0x500);
    rti_update_masked_register_bits(0xfffffe84, 0xff00, 0x1400);
    rti_update_masked_register_bits(0xfffffe88, 0xff00, 0x600);
    rti_update_masked_register_bits(0xfffffe94, 0xff000000, 0xa000000);
    /* Program VIM request mask set zero register with 0x600f53; write ordering is hardware-significant. */
    vim_request_mask_set_zero_register = 0x600f53;
    vim_request_mask_set_zero_register = 0x20;
    /* Program VIM request mask set one register with 0x40; write ordering is hardware-significant. */
    vim_request_mask_set_one_register = 0x40;
    return;
}

/**
 * @brief Configure Cortex-M exception priorities and enable the core interrupt lines.
 *
 * @return No value.
 */
void nvic_init()

{
    /* NVIC init using only caller-provided data. */
    /* Program NVIC INT PRIORITY REG0 with 0x80a0c0e0; write ordering is hardware-significant. */
    NVIC_INT_PRIORITY_REG0 = 0x80a0c0e0;
    NVIC_INT_PRIORITY_REG1 = 0x204060;
    /* Program NVIC SYS HAND CTL STATE REG with 0x70000; write ordering is hardware-significant. */
    NVIC_SYS_HAND_CTL_STATE_REG = 0x70000;
    NVIC_IRQ_0_31_SET_EN_REG = 0xff;
    return;
}
