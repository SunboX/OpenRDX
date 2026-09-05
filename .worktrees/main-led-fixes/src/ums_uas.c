/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ums_uas.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the ums uas module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the ums uas module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * The active mount build compiles its UAS support from `src/rdx_mount`; this
 * translation unit is outside that build filter and contains no duplicate
 * transport state or callback table.
 */
