/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_button_mode.h
//
// Description : OpenRDX no-cartridge eject-button mode gesture.
//=======================================================================================

#ifndef TUSB9261_RDX_BUTTON_MODE_H
#define TUSB9261_RDX_BUTTON_MODE_H

/*! @file
 * @brief Foreground state machine for the hidden OpenRDX button menu.
 */

#include "tusb9260_types.h"

/** Initialize the hidden no-cartridge button-menu and reconnect state. */
void rdx_button_mode_init(void);

/**
 * @brief Advance the hidden button menu from debounced input state.
 *
 * The caller owns physical-input debounce. This service owns the
 * continuous five-second entry hold, click window, repeating LED menu,
 * delayed confirmation callback, and deferred USB reconnect sequence.
 *
 * @param[in] now current free-running millisecond counter.
 * @param[in] button_pressed TRUE while the debounced eject button is held.
 * @param[in] cartridge_present TRUE while a debounced cartridge is present.
 */
void rdx_button_mode_service(UINT32_T now, BOOLEAN_T button_pressed,
                             BOOLEAN_T cartridge_present);

#endif /* TUSB9261_RDX_BUTTON_MODE_H */
