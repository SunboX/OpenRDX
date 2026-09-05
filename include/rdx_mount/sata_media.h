/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*! @file
 * @brief Dual-mode admission policy for RDX and generic SATA media.
 */

#ifndef SATA_MEDIA_H
#define SATA_MEDIA_H

#include "ahci.h"

/** Media layout selected for one initialized SATA port. */
typedef enum _SATA_MEDIA_KIND_T
{
    SATA_MEDIA_KIND_NONE = 0,
    SATA_MEDIA_KIND_RDX,
    SATA_MEDIA_KIND_GENERIC
} SATA_MEDIA_KIND_T;

/**
 * @brief Clear the selected layout and cached RDX context for one SATA port.
 *
 * Call this at every discovery boundary before the new device is inspected.
 *
 * @param port_num SATA port number.
 */
void sata_media_reset(UINT32_T port_num);

/**
 * @brief Forget a failed RDX access attempt after a physical disconnect.
 *
 * @param port_num SATA port number.
 */
void sata_media_link_disconnected(UINT32_T port_num);

/**
 * @brief Start a disk whose IDENTIFY data requests Power-Up In Standby.
 *
 * This runs immediately after IDENTIFY and before transfer-mode selection.
 * Disks without either standardized PUIS signature are left unchanged.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 * @return TRUE when no wake is required or the required wake completes.
 */
BOOLEAN_T sata_media_start_after_identify(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device);

/**
 * @brief Select and validate the media layout behind one SATA port.
 *
 * Locked media follows the authenticated RDX path. Directly readable media is
 * inspected for RDX metadata before generic SATA admission is considered.
 * Packet devices, inaccessible disks, ambiguous layouts, and invalid RDX
 * metadata remain unavailable.
 *
 * @param port_num SATA port number.
 * @param device Parsed ATA IDENTIFY information for that port.
 * @return TRUE only when one complete media policy admitted the disk.
 */
BOOLEAN_T sata_media_prepare(
    UINT32_T port_num,
    ATA_DEVICE_INFO_T *device);

/**
 * @brief Return the selected layout for one SATA port.
 *
 * @param port_num SATA port number.
 * @return Selected media layout, or SATA_MEDIA_KIND_NONE for an invalid port.
 */
SATA_MEDIA_KIND_T sata_media_get_kind(UINT32_T port_num);

/**
 * @brief Return the current discovery epoch for one SATA port.
 *
 * The value changes whenever media policy is invalidated, allowing a
 * synchronous command owner to detect removal or reset before restoring
 * interrupt state.
 *
 * @param port_num SATA port number.
 * @return Current epoch, or zero for an invalid port.
 */
UINT32_T sata_media_get_epoch(UINT32_T port_num);

/**
 * @brief Report whether runtime SMART polling is available for one port.
 *
 * @param port_num SATA port number.
 * @return TRUE when the admitted media supports enabled SMART telemetry.
 */
BOOLEAN_T sata_media_smart_is_available(UINT32_T port_num);

/**
 * @brief Translate one host LBA according to the admitted media layout.
 *
 * @param port_num SATA port number.
 * @param logical_lba Host-visible logical block address.
 * @param physical_lba Receives the translated physical block address.
 * @return TRUE only while an admitted media layout is selected.
 */
BOOLEAN_T sata_media_translate_lba(
    UINT32_T port_num,
    UINT64_T logical_lba,
    UINT64_T *physical_lba);

#endif
