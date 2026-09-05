/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : wdt.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the wdt module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the wdt module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Start state or data used by the WDT subsystem.
 *
 * @return No value.
 */
void wdt_start()

{
    /* Watchdog start using only caller-provided data. */
    uint32_t watchdog_preload;

    watchdog_preload = ((uint32_t)(rti_clock_mhz * 0x6ca48) >> 0xd) - 1;
    if (0xfff < watchdog_preload) {
        watchdog_preload = 0xfff;
    }
    /* Program RTIDWDPRLD REG OFF with watchdog_preload; write ordering is hardware-significant. */
    RTIDWDPRLD_REG_OFF = watchdog_preload;
    RTIDWDCTRL_REG_OFF = 0xaced5312;
    /* Program RTIWDSTATUS REG OFF with 0x1f; write ordering is hardware-significant. */
    RTIWDSTATUS_REG_OFF = 0x1f;
    return;
}

/**
 * @brief Reset state or data used by the WDT subsystem.
 *
 * @return No value.
 */
void wdt_reset()

{
    /* Watchdog reset using only caller-provided data. */
    /* Program RTIWDKEY REG OFF with 0xe51a; write ordering is hardware-significant. */
    RTIWDKEY_REG_OFF = 0xe51a;
    RTIWDKEY_REG_OFF = 0xa35c;
    return;
}
