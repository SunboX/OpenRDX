/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_vendor.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the usb vendor module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the usb vendor module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Define an intentionally empty subsystem boundary.
 *
 * Vendor-request behavior is owned by the active mount implementation. This
 * translation unit is outside that build filter and contains no duplicate
 * request dispatcher or persistent state.
 */
