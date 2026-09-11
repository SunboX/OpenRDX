/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/** @file Read-only Manager status from stored metadata and live inputs. */
#include "rdx_manager_protocol.h"
#include "rdx_manager_identity.h"
#include "rdx_hardware.h"
#include "rdx_unlock.h"
#include "gio.h"
#include "string.h"

/** Store one big-endian 16-bit value. */
static void rdx_store_be16(UINT8_T *buffer, UINT16_T value)
{
    buffer[0] = (UINT8_T)(value >> 8U);
    buffer[1] = (UINT8_T)value;
}

/** Store one big-endian 32-bit value. */
static void rdx_store_be32(UINT8_T *buffer, UINT32_T value)
{
    buffer[0] = (UINT8_T)(value >> 24U);
    buffer[1] = (UINT8_T)(value >> 16U);
    buffer[2] = (UINT8_T)(value >> 8U);
    buffer[3] = (UINT8_T)value;
}

/** Append one LOG SENSE integer parameter record. */
static UINT32_T rdx_append_log_parameter(UINT8_T *buffer, UINT32_T offset,
                                         UINT16_T code, UINT32_T value,
                                         UINT8_T value_length)
{
    rdx_store_be16(&buffer[offset], code);
    buffer[offset + 2U] = 0x60U;
    buffer[offset + 3U] = value_length;
    if (value_length == 2U)
    {
        rdx_store_be16(&buffer[offset + 4U], (UINT16_T)value);
    }
    else
    {
        rdx_store_be32(&buffer[offset + 4U], value);
    }
    return offset + 4U + value_length;
}

/** Build one RDX LOG SENSE page used by RDX Manager. */
UINT32_T rdx_manager_build_status_log(UINT8_T *buffer, UINT32_T buffer_size,
                                     UINT8_T lun, UINT8_T page_code)
{
    UINT32_T offset;
    const RDX_MEDIA_STATISTICS_T *statistics;
    UINT32_T code;

    if ((buffer == NULL) || (lun >= NUM_AHCI_PORTS))
    {
        return 0U;
    }
    if (page_code == RDX_LOG_SENSE_DRIVE_PAGE_CODE)
    {
        if (buffer_size < 56U)
        {
            return 0U;
        }
        ti_memset(buffer, 0, 56U);
        buffer[0] = page_code;
        rdx_store_be16(&buffer[2], 52U);
        offset = 4U;
        offset = rdx_append_log_parameter(buffer, offset, 0U, 0U, 2U);
        offset = rdx_append_log_parameter(
            buffer, offset, 1U, rdx_manager_get_drive_load_count(), 4U);
        offset = rdx_append_log_parameter(buffer, offset, 2U, 0U, 4U);
        offset = rdx_append_log_parameter(buffer, offset, 3U,
            gio_rdx_mechanism_input_asserted() ? 1U : 0U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 4U, 1U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 5U,
            ((rdx_manager_get_hardware_profile() == 0x38U) &&
             !gio_is_usb_device_self_powered()) ? 1U : 0U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 6U, 7U, 2U);
        offset = rdx_append_log_parameter(buffer, offset, 7U, 0U, 2U);
        return offset;
    }
    if (page_code == RDX_LOG_SENSE_CARTRIDGE_PAGE_CODE)
    {
        if (buffer_size < 116U)
        {
            return 0U;
        }
        ti_memset(buffer, 0, 116U);
        buffer[0] = page_code;
        rdx_store_be16(&buffer[2], 112U);
        offset = 4U;
        offset = rdx_append_log_parameter(buffer, offset, 0U, 0U, 2U);
        statistics = rdx_get_media_statistics(lun);
        for (code = 1U; code <= 13U; code++)
        {
            UINT32_T value = 0U;
            /* Types 0Ah/0Bh/0Ch in context 4 are the cartridge's
             * stored load/read/write counters. Absence is not a zero sample. */
            if ((code == 1U) || (code == 5U) || (code == 6U))
            {
                if ((statistics == NULL) ||
                    ((code == 1U) && !statistics->load_count_valid) ||
                    ((code == 5U) && !statistics->written_mib_valid) ||
                    ((code == 6U) && !statistics->read_mib_valid))
                {
                    continue;
                }
                value = (code == 1U) ? statistics->load_count :
                    ((code == 5U) ? statistics->written_mib : statistics->read_mib);
            }
            else if (code == 2U)
            {
                /* The capacity field uses decimal MB and an overflow
                 * sentinel. Filesystem free space is not known here. */
                value = (ata_dev[lun].ddTrueMaxLBA > 0xFFFFFFFFULL) ?
                    0xFFFFFFFFUL :
                    (((UINT32_T)ata_dev[lun].ddTrueMaxLBA / 1000U) << 9U) / 1000U;
            }
            else if (code == 8U)
            {
                value = rdx_hardware_is_write_protected() ? 1U : 0U;
            }
            else if (code == 10U)
            {
                value = ata_dev[lun].bSATA_Gen;
            }
            offset = rdx_append_log_parameter(buffer, offset,
                                              (UINT16_T)code, value, 4U);
        }
        /* Include the two reserved tail bytes in a complete page; optional
         * counter records can be absent without inventing zero samples. */
        if (offset == 114U)
        {
            offset = 116U;
        }
        rdx_store_be16(&buffer[2], (UINT16_T)(offset - 4U));
        return offset;
    }
    if (page_code == RDX_LOG_SENSE_TEMPERATURE_PAGE_CODE)
    {
        if (buffer_size < 16U)
        {
            return 0U;
        }
        ti_memset(buffer, 0, 16U);
        buffer[0] = page_code;
        rdx_store_be16(&buffer[2], 12U);
        rdx_store_be16(&buffer[4], 0U);
        buffer[6] = 0x43U;
        buffer[7] = 2U;
        /* Current sample is parameter 0; 0xFF marks an unavailable sensor. */
        buffer[9] = rdx_hardware_get_temperature_celsius();
        rdx_store_be16(&buffer[10], 1U);
        buffer[12] = 0x43U;
        buffer[13] = 2U;
        buffer[15] = 0xFFU;
        return 16U;
    }
    return 0U;
}
