/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_MANUFACTURING_H
#define TUSB9261_RDX_MANUFACTURING_H

#include "tusb9260.h"

#define RDX_MANUFACTURING_BUFFER_ID 0x4DU
#define RDX_MANUFACTURING_TRANSFER_LENGTH 528U

/** Return TRUE after a started restore until a manual restart clears RAM. */
BOOLEAN_T rdx_manufacturing_mutation_started(void);

/**
 * @brief Probe support or restore the 256-byte manufacturing record.
 *
 * @param cdb Exact ten-byte WRITE BUFFER command, validated by the caller's
 *            command-length gate before this function may dereference it.
 * @param host_length Actual USB payload byte count; must match the CDB.
 * @param payload Read-only RDXMFG01 request, or NULL for the capability probe.
 * @return STATUS_OK only after a nonmutating probe/no-op or full-sector
 *         readback verification; a SCSI failure status for every rejection.
 * @note Caller must reject this command during firmware update/reset. Restore
 *       preserves neighboring bytes and runtime identity until a manual reset.
 */
STATUS_T rdx_manager_handle_manufacturing(const UINT8_T *cdb,
                                         UINT32_T host_length,
                                         const UINT8_T *payload);

#endif
