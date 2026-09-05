/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_chap9.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the usb chap9 module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the usb chap9 module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * The active mount build compiles USB Chapter 9 handling from
 * `src/rdx_mount`; this translation unit is outside that build filter and
 * contains no duplicate descriptor or endpoint-zero state.
 */
