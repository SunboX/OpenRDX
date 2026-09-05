/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : reg_io.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the reg io module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the reg io module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * The active mount build compiles register access from `src/rdx_mount`; this
 * translation unit is outside that build filter and contains no duplicate
 * register helpers or hardware ownership.
 */
