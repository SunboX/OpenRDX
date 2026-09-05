/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rdx_manager_identity.h
//
// Description : Checksum-protected adapter identity and hardware profile.
//=======================================================================================

#ifndef TUSB9261_RDX_MANAGER_IDENTITY_H
#define TUSB9261_RDX_MANAGER_IDENTITY_H

/*! @file
 * @brief Access to the validated manufacturing record at SPI flash 3E000h.
 */

#include "tusb9260_types.h"

/** Fixed-width fields used by the Manager adapter-identity VPD designator. */
typedef struct _RDX_DRIVE_IDENTITY_T
{
    UINT8_T serial[10];
    UINT8_T vendor[8];
    UINT8_T product[16];
} RDX_DRIVE_IDENTITY_T;

/** Load adapter identity and profile from the checksum-protected record. */
void rdx_manager_identity_init(void);

/**
 * @brief Return the initialized fixed-width adapter identity.
 *
 * @return immutable identity record valid until the next initialization.
 */
const RDX_DRIVE_IDENTITY_T *rdx_manager_get_drive_identity(void);

/**
 * @brief Return the checksum-validated effective hardware profile identifier.
 *
 * The raw 3E000h flash record stores the profile at offset 0xA4; offset 0x78
 * belongs to a separate decoded wire representation. Raw 0, 0xFFFF, and 1
 * normalize to profile 0x37. An unreadable or checksum-invalid record uses
 * profile 0x38 so the runtime remains on the ordinary enabled path.
 *
 * @return effective hardware profile, or 0x38 after fallback.
 */
UINT16_T rdx_manager_get_hardware_profile(void);

#endif /* TUSB9261_RDX_MANAGER_IDENTITY_H */
