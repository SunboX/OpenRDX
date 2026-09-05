/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : system.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the system module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the system module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * The active mount build compiles system control from `src/rdx_mount`; this
 * translation unit is outside that build filter and contains no duplicate
 * reset, clock, or power-state owner.
 */
