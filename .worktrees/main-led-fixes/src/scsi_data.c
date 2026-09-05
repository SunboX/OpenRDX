/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : scsi_data.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the scsi data module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the scsi data module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * The active mount build compiles SCSI data handling from `src/rdx_mount`;
 * this translation unit is outside that build filter and contains no second
 * response table or command-state owner.
 */
