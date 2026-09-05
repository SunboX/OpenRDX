/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : one_touch.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the one touch module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the one touch module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * The active mount build compiles its button implementation from
 * `src/rdx_mount`; this translation unit is outside that build filter and
 * therefore contains no runtime procedure or duplicated hardware owner.
 */
