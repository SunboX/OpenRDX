/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_MECHANISM_H
#define TUSB9261_RDX_MECHANISM_H

/*! @file
 * @brief RDX mechanism state-machine interface.
 */

#include "tusb9260_types.h"

typedef enum _RDX_MECHANISM_EVENT_T
{
    RDX_MECHANISM_EVENT_NONE = 0,
    RDX_MECHANISM_EVENT_SUCCEEDED,
    RDX_MECHANISM_EVENT_FAILED
} RDX_MECHANISM_EVENT_T;

/**
 * @brief Initialize the mechanism state with every motor output inactive.
 *
 * This function never starts timed motion. Boot homing is deliberately split
 * into rdx_mechanism_start_boot_homing() so synchronous startup work cannot
 * prevent the foreground service from enforcing motor deadlines.
 *
 * @param[in] hardware_profile validated hardware profile code.
 */
void rdx_mechanism_init(UINT16_T hardware_profile);

/**
 * @brief Start boot homing from the sensed mechanism position when required.
 *
 * @param[in] now current free-running millisecond counter.
 * @return TRUE when boot homing entered state 1, otherwise FALSE.
 */
BOOLEAN_T rdx_mechanism_start_boot_homing(UINT32_T now);

/**
 * @brief Enter mechanism state 1 for one accepted eject.
 *
 * @param[in] now current free-running millisecond counter.
 * @param[in] hardware_profile validated hardware profile code.
 */
void rdx_mechanism_start(UINT32_T now,
                         UINT16_T hardware_profile);

/**
 * @brief Advance states 1 through 8 once without blocking.
 *
 * @param[in] now current free-running millisecond counter.
 * @return no event, one settled successful eject, or terminal failure.
 */
RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now);

/** Stop all implemented drives and return the mechanism state to idle. */
void rdx_mechanism_cancel(void);

/**
 * @brief Report whether mechanism states 1 through 8 are active.
 *
 * @return TRUE while the mechanism state machine is active, otherwise FALSE.
 */
BOOLEAN_T rdx_mechanism_is_active(void);

#endif /* TUSB9261_RDX_MECHANISM_H */
