/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*! @file
 * @brief Authenticated RDX cartridge mount and ATA access initialization.
 */

#ifndef RDX_UNLOCK_H
#define RDX_UNLOCK_H

#include "ahci.h"
#include "rdx_runtime_ata.h"

#define RDX_MEDIA_VENDOR_LENGTH   32U
#define RDX_MEDIA_MODEL_LENGTH    32U
#define RDX_MEDIA_SERIAL_LENGTH   16U
#define RDX_MEDIA_BARCODE_LENGTH  16U

/** Cartridge identity fields stored in the authenticated RDX metadata tree. */
typedef struct _RDX_MEDIA_IDENTITY_T
{
    UINT8_T vendor[RDX_MEDIA_VENDOR_LENGTH];
    UINT8_T model[RDX_MEDIA_MODEL_LENGTH];
    UINT8_T serial[RDX_MEDIA_SERIAL_LENGTH];
    UINT8_T barcode[RDX_MEDIA_BARCODE_LENGTH];
    BOOLEAN_T valid;
} RDX_MEDIA_IDENTITY_T;

/** Result of inspecting an already accessible SATA medium for RDX metadata. */
typedef enum _RDX_MEDIA_INSPECTION_T
{
    RDX_MEDIA_INSPECTION_UNREADABLE = 0,
    RDX_MEDIA_INSPECTION_NOT_RECOGNIZED,
    RDX_MEDIA_INSPECTION_READY,
    RDX_MEDIA_INSPECTION_INVALID
} RDX_MEDIA_INSPECTION_T;

/** Result of one cartridge access and metadata-validation attempt. */
typedef enum _RDX_ACCESS_RESULT_T
{
    RDX_ACCESS_RESULT_READY = 0,
    RDX_ACCESS_RESULT_REJECTED,
    RDX_ACCESS_RESULT_RETRYABLE_FAILURE
} RDX_ACCESS_RESULT_T;

/**
 * @brief Clear cached RDX layout and identity state for one SATA port.
 *
 * @param port_num SATA port number.
 */
void rdx_reset_media_context(UINT32_T port_num);

/**
 * @brief Run the cartridge-access F2 operation for a locked RDX candidate.
 *
 * The caller selects this path when the new link reports a locked disk. The
 * operation itself does not reinterpret ATA IDENTIFY security fields; it
 * issues the model/capacity/serial-derived command and then requires valid
 * RDX metadata. Already accessible media is classified separately so a
 * redundant access command cannot reject a usable cartridge.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 * @return Structured result distinguishing a completed access rejection from
 *         a retryable transport or metadata failure.
 */
RDX_ACCESS_RESULT_T rdx_unlock_media(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device);

/**
 * @brief Inspect directly readable media for the authenticated RDX layout.
 *
 * This performs no ATA SECURITY UNLOCK command. It verifies physical sector
 * zero is readable, validates the mirrored metadata and data extent, loads
 * cartridge identity, and enables advisory SMART monitoring.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 * @return Structured result distinguishing generic, valid RDX, corrupt RDX,
 *         and inaccessible media.
 */
RDX_MEDIA_INSPECTION_T rdx_inspect_accessible_media(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device);

/**
 * @brief Return the cartridge identity loaded during authenticated mount.
 *
 * @param port_num SATA port number.
 * @return Identity for a genuine mounted cartridge, or NULL when the metadata
 * fields are not available.
 */
const RDX_MEDIA_IDENTITY_T *rdx_get_media_identity(UINT32_T port_num);

/**
 * @brief Translate a host-visible cartridge LBA to the physical SATA LBA.
 *
 * Authenticated cartridge metadata supplies the start of the user-data extent,
 * which is added to every SCSI read/write LBA.
 *
 * @param port_num SATA port number.
 * @param logical_lba Host-visible logical block address.
 * @return Physical SATA logical block address.
 */
UINT64_T rdx_translate_media_lba(UINT32_T port_num, UINT64_T logical_lba);

#endif
