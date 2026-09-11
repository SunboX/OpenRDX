/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/** @file Bounded extraction of checksum-validated cartridge properties. */
#include "rdx_unlock.h"
#include "string.h"

#define RDX_METADATA_FIRST_OFFSET 4U
#define RDX_METADATA_DATA_BYTES 508U
#define RDX_METADATA_TYPE_VENDOR 0x06U
#define RDX_METADATA_TYPE_MODEL 0x31U
#define RDX_METADATA_TYPE_SERIAL 0x05U
#define RDX_METADATA_TYPE_BARCODE 0x35U

static RDX_MEDIA_IDENTITY_T rdx_media_identity[NUM_AHCI_PORTS];
static RDX_MEDIA_STATISTICS_T rdx_media_statistics[NUM_AHCI_PORTS];

/** Decode a little-endian record length without alignment assumptions. */
static UINT16_T rdx_metadata_read_le16(const volatile UINT8_T *data)
{
    return (UINT16_T)((UINT16_T)data[0] | ((UINT16_T)data[1] << 8U));
}

/** Reset cached advisory fields when a SATA medium is replaced. */
void rdx_reset_media_metadata(UINT32_T port_num)
{
    if (port_num < NUM_AHCI_PORTS)
    {
        ti_memset(&rdx_media_identity[port_num], 0U,
                  sizeof(rdx_media_identity[port_num]));
        ti_memset(&rdx_media_statistics[port_num], 0U,
                  sizeof(rdx_media_statistics[port_num]));
    }
}

/**
 * @brief Copy one byte-string property from an RDX metadata sector.
 *
 * Context-3 identity uses record types 06h, 31h, 05h, and 35h. Text records
 * have flag bits zero and two clear; copy their bytes after the four-byte
 * header into a zero-filled fixed-width destination.
 *
 * @param data Metadata sector containing typed records.
 * @param wanted_type Property record type.
 * @param destination Zero-filled destination field.
 * @param destination_size Size of the destination field.
 * @return TRUE when a structurally valid property record was copied.
 */
static BOOLEAN_T rdx_copy_metadata_text_record(
    const volatile UINT8_T *data,
    UINT8_T wanted_type,
    UINT8_T *destination,
    UINT32_T destination_size)
{
    UINT32_T offset = RDX_METADATA_FIRST_OFFSET;

    ti_memset(destination, 0U, destination_size);
    while (offset < RDX_METADATA_DATA_BYTES)
    {
        UINT8_T type = data[offset];
        UINT8_T flags = data[offset + 1U];
        UINT16_T length = rdx_metadata_read_le16(data + offset + 2U);
        UINT32_T header_size;

        if ((flags & 0x04U) != 0U)
        {
            header_size = 20U;
        }
        else if ((flags & 0x01U) != 0U)
        {
            header_size = 12U;
        }
        else
        {
            header_size = 4U;
        }

        if ((length < header_size) ||
            (length >= RDX_METADATA_DATA_BYTES) ||
            ((offset + length) >= RDX_METADATA_DATA_BYTES))
        {
            return FALSE;
        }

        if ((type == wanted_type) && ((flags & 0x05U) == 0U))
        {
            UINT32_T copy_length = (UINT32_T)length - header_size;

            if (copy_length > destination_size)
            {
                copy_length = destination_size;
            }
            ti_memcpy(destination, (const void *)(data + offset + header_size),
                      copy_length);
            return TRUE;
        }
        offset += length;
    }
    return FALSE;
}

/**
 * @brief Capture the four cartridge identity properties used by VPD page C0h.
 *
 * @param port_num SATA port number.
 * @param data Selected, checksum-validated metadata sector.
 */
void rdx_capture_media_identity(UINT32_T port_num,
                                const volatile UINT8_T *data)
{
    RDX_MEDIA_IDENTITY_T candidate;
    BOOLEAN_T complete;

    if ((port_num >= NUM_AHCI_PORTS) || (data == NULL))
    {
        return;
    }
    ti_memset(&candidate, 0U, sizeof(candidate));
    complete = rdx_copy_metadata_text_record(
                   data, RDX_METADATA_TYPE_VENDOR,
                   candidate.vendor, sizeof(candidate.vendor)) &&
               rdx_copy_metadata_text_record(
                   data, RDX_METADATA_TYPE_MODEL,
                   candidate.model, sizeof(candidate.model)) &&
               rdx_copy_metadata_text_record(
                   data, RDX_METADATA_TYPE_SERIAL,
                   candidate.serial, sizeof(candidate.serial)) &&
               rdx_copy_metadata_text_record(
                   data, RDX_METADATA_TYPE_BARCODE,
                   candidate.barcode, sizeof(candidate.barcode));
    if (complete)
    {
        candidate.valid = TRUE;
        ti_memcpy(&rdx_media_identity[port_num], &candidate,
                  sizeof(candidate));
    }
}

/** Return the cartridge identity loaded during authenticated mount. */
const RDX_MEDIA_IDENTITY_T *rdx_get_media_identity(UINT32_T port_num)
{
    if ((port_num >= NUM_AHCI_PORTS) ||
        !rdx_media_identity[port_num].valid)
    {
        return NULL;
    }
    return &rdx_media_identity[port_num];
}


/**
 * @brief Capture context-4 scalar counters from a validated sector.
 * @param port_num SATA port index.
 * @param data Exactly 512 bytes already checked by the mirrored-sector reader.
 *
 * Context 4 maps 0Ah to loads, 0Bh to read MiB, and 0Ch to written MiB.
 * A malformed or duplicate scalar never becomes a valid counter sample.
 * This reader does not change either persistent copy on the cartridge.
 */
void rdx_capture_media_statistics(UINT32_T port_num,
                                  const volatile UINT8_T *data)
{
    RDX_MEDIA_STATISTICS_T candidate;
    UINT32_T offset = RDX_METADATA_FIRST_OFFSET;

    if ((port_num >= NUM_AHCI_PORTS) || (data == NULL))
    {
        return;
    }
    ti_memset(&candidate, 0U, sizeof(candidate));
    while (offset + 4U <= RDX_METADATA_DATA_BYTES)
    {
        UINT8_T type = data[offset];
        UINT8_T flags = data[offset + 1U];
        UINT16_T length = rdx_metadata_read_le16(data + offset + 2U);
        UINT32_T header_size = (flags & 4U) ? 20U : ((flags & 1U) ? 12U : 4U);

        if ((type == 0U) && (flags == 0U) && (length == 0U))
        {
            break;
        }
        if ((length < header_size) || (offset + length > RDX_METADATA_DATA_BYTES))
        {
            return;
        }
        if ((type >= 0x0AU) && (type <= 0x0CU) && ((flags & 5U) == 0U))
        {
            UINT32_T value = 0U;
            UINT32_T index;
            BOOLEAN_T *valid;
            UINT32_T *destination;
            if ((length <= 4U) || (length > 8U))
            {
                return;
            }
            valid = (type == 0x0AU) ? &candidate.load_count_valid :
                ((type == 0x0BU) ? &candidate.read_mib_valid : &candidate.written_mib_valid);
            destination = (type == 0x0AU) ? &candidate.load_count :
                ((type == 0x0BU) ? &candidate.read_mib : &candidate.written_mib);
            if (*valid)
            {
                return;
            }
            for (index = 0U; index < (UINT32_T)length - 4U; index++)
            {
                value |= (UINT32_T)data[offset + 4U + index] << (index * 8U);
            }
            *destination = value;
            *valid = TRUE;
        }
        offset += length;
    }
    rdx_media_statistics[port_num] = candidate;
}

/** Return independently valid stored cartridge counters for one current port. */
const RDX_MEDIA_STATISTICS_T *rdx_get_media_statistics(UINT32_T port_num)
{
    return (port_num < NUM_AHCI_PORTS) ? &rdx_media_statistics[port_num] : NULL;
}
