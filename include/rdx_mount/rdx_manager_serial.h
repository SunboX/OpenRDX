/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef RDX_MANAGER_SERIAL_H
#define RDX_MANAGER_SERIAL_H

#include "tusb9260.h"

#define RDX_SERIAL_BUFFER_ID       0x53U
#define RDX_SERIAL_REQUEST_LENGTH  280U

/**
 * @brief Select the same bounded USB receive buffer for BOT and SCSI dispatch.
 *
 * Serial and full-record requests use the SCSI response allocation, not an ATA
 * DMA target. Other data-out commands retain the normal 4-KiB buffer. Both
 * allocations hold at least 4096 bytes; this selector does not admit commands.
 *
 * @param[in] cdb Command bytes, or NULL for the ordinary receive buffer.
 * @return Buffer whose ownership lasts through this serialized BOT command.
 */
UINT8_T *rdx_manager_write_buffer_data(const UINT8_T *cdb);

/**
 * @brief Preserve exact framing for serial and full-record manufacturing writes.
 *
 * BOT discards surplus non-RW bytes after its first buffer. The final discard
 * fragment must never hide an oversized serial or full-record transfer.
 * Full-record requests also reject a short final receive when the CBW is 528 bytes.
 * Other commands retain their existing completed-fragment length contract.
 *
 * @param[in] cdb Command bytes, or NULL for the ordinary length contract.
 * @param[in] total_length Initial CBW transfer length.
 * @param[in] completed_length Last completed USB receive length.
 * @return Complete CBW length for manufacturing requests; the final received length
 *         for exact-size full-record requests and other command families.
 */
UINT32_T rdx_manager_write_buffer_host_length(const UINT8_T *cdb,
                                              UINT32_T total_length,
                                              UINT32_T completed_length);

/**
 * @brief Apply a serial-only manufacturing edit from an exact sector snapshot.
 *
 * The foreground SCSI caller must exclude firmware downloads and supply an
 * exclusively owned normal_data_buffer. This routine reuses that 4-KiB DMA
 * buffer after verifying ATA quiescence, preserving every neighboring byte.
 * A host must durably back up the full sector before invoking this operation.
 * Fallback initialization is limited to completely erased records and the documented
 * fallback profile; a partially corrupt record is never replaced.
 *
 * @param[in] payload Fixed RDXSER01 request described in serial-number-repair.md.
 * @param[in] length Exactly RDX_SERIAL_REQUEST_LENGTH bytes.
 * @param[out] mutation_started TRUE once programming may have changed flash.
 * @return STATUS_OK only after complete-sector readback, or a SCSI error.
 */
STATUS_T rdx_manager_serial_write(const UINT8_T *payload, UINT32_T length,
                                  BOOLEAN_T *mutation_started);

#endif
