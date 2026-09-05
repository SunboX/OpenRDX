/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_MECHANISM_H
#define TUSB9261_RDX_MECHANISM_H

/*! @file
 * @brief RDX mechanism state-machine interface.
 */

#include "tusb9260_types.h"

#define RDX_MECHANISM_DIAGNOSTIC_LENGTH 128U

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
 * @return TRUE when boot homing started the low-control drive, otherwise FALSE.
 */
BOOLEAN_T rdx_mechanism_start_boot_homing(UINT32_T now);

/**
 * @brief Start the mechanism low-control drive for one accepted eject.
 *
 * @param[in] now current free-running millisecond counter.
 * @param[in] hardware_profile validated hardware profile code.
 */
void rdx_mechanism_start(UINT32_T now,
                         UINT16_T hardware_profile);

/**
 * @brief Advance one mechanism drive, pause, or completion phase without blocking.
 *
 * @param[in] now current free-running millisecond counter.
 * @return no event, one settled successful eject, or terminal failure.
 */
RDX_MECHANISM_EVENT_T rdx_mechanism_service(UINT32_T now);

/** Stop all implemented drives and return the mechanism state to idle. */
void rdx_mechanism_cancel(void);

/**
 * @brief Report whether a mechanism drive or completion sequence is active.
 *
 * @return TRUE while the mechanism state machine is active, otherwise FALSE.
 */
BOOLEAN_T rdx_mechanism_is_active(void);

/**
 * @brief Read a fixed, little-endian mechanism and register snapshot.
 *
 * The 64-byte header contains RDXM, schema 1, state, retry, event count,
 * profile, GPIO input/output bytes, both RTI counters, deadline, service
 * timing, phase-entry time, four operation counters, and PWM1 CFG/PER/PH1D.
 * Eight committed 8-byte events follow in time order: RTI1 timestamp, state,
 * GPIO input, GPIO output, and retry count. Unused event bytes are zero.
 * The read performs no bus transfer or output write and may run in USB IRQ
 * context; an event being written by foreground code is omitted.
 *
 * @param[out] data Destination for exactly 128 bytes.
 * @param[in] length Must equal RDX_MECHANISM_DIAGNOSTIC_LENGTH.
 * @return TRUE when the complete snapshot was serialized, otherwise FALSE.
 */
BOOLEAN_T rdx_mechanism_read_diagnostics(UINT8_T *data, UINT32_T length);

#endif /* TUSB9261_RDX_MECHANISM_H */
