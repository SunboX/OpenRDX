/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_RUNTIME_ATA_H
#define TUSB9261_RDX_RUNTIME_ATA_H

/*! @file
 * @brief Synchronous ATA commands used after USB startup.
 */

#include "ahci.h"

/**
 * @brief Read the cartridge temperature from its SMART attribute table.
 *
 * The caller must mask USB command submission.  This helper independently
 * verifies port quiescence, saves and masks command-owned PxIE bits, owns
 * command PxIS statuses, and restores the saved value only while the same
 * admitted media epoch remains current.
 *
 * @param port_num SATA port number.
 * @param temperature_celsius Receives SMART attribute C2h, or BEh when C2h
 *                            is absent. Receives 0xFF on failure.
 * @return TRUE when a supported temperature attribute was read; FALSE when
 *         unavailable, failed, or absent.
 */
BOOLEAN_T rdx_read_smart_temperature(UINT32_T port_num,
                                     UINT8_T *temperature_celsius);

/**
 * @brief Flush the initialized cartridge and place it in standby for STOP.
 *
 * The caller must mask USB command submission.  This helper reserves idle
 * AHCI slot zero, issues FLUSH CACHE/EXT followed by STANDBY IMMEDIATE, and
 * restores interrupt ownership after a command failure only if the media
 * epoch remains current. Mechanical eject uses
 * rdx_prepare_mechanism_eject() because that path must not add a cache-flush
 * command.
 *
 * @param port_num SATA port number.
 * @return TRUE only when both commands complete without an ATA error.
 */
BOOLEAN_T rdx_prepare_media_stop(UINT32_T port_num);

/**
 * @brief Place an initialized cartridge in standby before mechanism motion.
 *
 * The caller must mask USB command submission.  This helper reserves idle
 * AHCI slot zero, issues only STANDBY IMMEDIATE, consumes its completion
 * status, and restores the interrupt-enable value only for the same admitted
 * media epoch.
 *
 * @param port_num SATA port number.
 * @return TRUE when STANDBY IMMEDIATE completes without an ATA error.
 */
BOOLEAN_T rdx_prepare_mechanism_eject(UINT32_T port_num);

#endif /* TUSB9261_RDX_RUNTIME_ATA_H */
