/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/** @file Serial-only manufacturing edits with stale-snapshot protection. */

#include "rdx_manager_serial.h"
#include "rdx_manufacturing.h"

#include "ahci.h"
#include "gio.h"
#include "spi.h"
#include "string.h"

#define RDX_SERIAL_FLASH_ADDRESS  0x3E000U
#define RDX_SERIAL_SECTOR_LENGTH  0x1000U
#define RDX_SERIAL_RECORD_LENGTH  0x0100U
#define RDX_SERIAL_VALUE_OFFSET   8U
#define RDX_SERIAL_VALUE_LENGTH   10U
#define RDX_SERIAL_EXPECTED_OFFSET 24U

/** Select USB input storage that ATA discovery DMA cannot overwrite. */
UINT8_T *rdx_manager_write_buffer_data(const UINT8_T *cdb)
{
    if ((cdb != NULL) && (cdb[0] == 0x3BU) &&
        ((cdb[2] == RDX_SERIAL_BUFFER_ID) ||
         (cdb[2] == RDX_MANUFACTURING_BUFFER_ID)))
    {
        return (UINT8_T *)datapath_ram->scsi_response_buffer;
    }
    return (UINT8_T *)datapath_ram->normal_data_buffer;
}

/** Keep oversized and incomplete manufacturing-data transfers visible to validation. */
UINT32_T rdx_manager_write_buffer_host_length(const UINT8_T *cdb,
                                              UINT32_T total_length,
                                              UINT32_T completed_length)
{
    if ((cdb != NULL) && (cdb[0] == 0x3BU) &&
        (cdb[2] == RDX_SERIAL_BUFFER_ID))
    {
        return total_length;
    }
    if ((cdb != NULL) && (cdb[0] == 0x3BU) &&
        (cdb[2] == RDX_MANUFACTURING_BUFFER_ID))
    {
        /* Retain oversized CBWs but also reject a short final receive when
         * the declared total alone matches this fixed restore frame. */
        return (total_length == RDX_MANUFACTURING_TRANSFER_LENGTH) ?
            completed_length : total_length;
    }
    return completed_length;
}

/** Require every ATA slot idle before borrowing its shared scratch buffer. */
static BOOLEAN_T rdx_serial_ata_idle(void)
{
    UINT32_T port;
    for (port = 0U; port < NUM_AHCI_PORTS; port++)
    {
        if ((READ32(PxCI(port)) | READ32(PxSACT(port))) != 0U)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/** Compare fixed-length byte strings without libc or alignment assumptions. */
static BOOLEAN_T rdx_serial_equal(const UINT8_T *left, const UINT8_T *right,
                                   UINT32_T length)
{
    UINT32_T index;
    for (index = 0U; index < length; index++)
    {
        if (left[index] != right[index])
        {
            return FALSE;
        }
    }
    return TRUE;
}

/** Calculate the IEEE CRC32 of a host-backed-up complete manufacturing sector. */
static UINT32_T rdx_serial_sector_crc(const UINT8_T *sector)
{
    UINT32_T crc = 0xFFFFFFFFUL;
    UINT32_T index;
    UINT32_T bit;
    for (index = 0U; index < RDX_SERIAL_SECTOR_LENGTH; index++)
    {
        crc ^= sector[index];
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = (crc >> 1U) ^ ((crc & 1U) ? 0xEDB88320UL : 0U);
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

/** Calculate the manufacturing four-lane checksum, excluding the stored checksum bytes. */
static void rdx_serial_checksum(const UINT8_T *record, UINT8_T *checksum)
{
    UINT32_T index;
    checksum[0] = 0x78U;
    checksum[1] = 0x56U;
    checksum[2] = 0x34U;
    checksum[3] = 0x12U;
    for (index = 4U; index < RDX_SERIAL_RECORD_LENGTH; index++)
    {
        checksum[index & 3U] += record[index];
    }
}

/** Return TRUE only for a completely erased 256-byte manufacturing record. */
static BOOLEAN_T rdx_serial_record_erased(const UINT8_T *record)
{
    UINT32_T index;
    for (index = 0U; index < RDX_SERIAL_RECORD_LENGTH; index++)
    {
        if (record[index] != 0xFFU)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * @brief Initialize the documented documented fallback profile, without a serial.
 *
 * This is explicitly a fallback initialization, not restored calibration or
 * manufacturing history. Values match the decoded vendor fallback structure
 * and the existing invalid-record identity/profile defaults. The legacy USB
 * PID stored in the record remains 0006h; OpenRDX enumerates as 0005h.
 */
static void rdx_serial_initialize_fallback(UINT8_T *record)
{
    static const UINT8_T vendor[8] =
        { 'T', 'A', 'N', 'D', 'B', 'E', 'R', 'G' };
    static const UINT8_T product[16] =
        { 'R', 'D', 'X', ' ', ' ', ' ', ' ', ' ',
          ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
    static const UINT8_T date[8] =
        { '1', '0', '1', '0', '2', '0', '1', '0' };
    ti_memset(record, 0U, RDX_SERIAL_RECORD_LENGTH);
    record[4] = 6U;
    ti_memcpy(&record[0x12U], vendor, sizeof(vendor));
    ti_memcpy(&record[0x1AU], product, sizeof(product));
    record[0x30U] = 1U;
    record[0x76U] = 0x5AU;
    record[0x77U] = 0x1AU;
    record[0x78U] = 6U;
    ti_memcpy(&record[0x7AU], vendor, sizeof(vendor));
    ti_memcpy(&record[0x82U], product, sizeof(product));
    ti_memcpy(&record[0x9CU], date, sizeof(date));
    record[0xA4U] = 0x38U;
}

/** Program exactly one aligned page, retaining all other flash bytes. */
static STATUS_T rdx_serial_program_page(UINT32_T offset, UINT8_T *sector)
{
    if (SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    if (SpiOps(OpcodePageProgram, RDX_SERIAL_FLASH_ADDRESS + offset,
               &sector[offset], RDX_SERIAL_RECORD_LENGTH, 0U) != STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    return STATUS_OK;
}

/** Apply a bounded serial edit; retain the current USB identity until reset. */
STATUS_T rdx_manager_serial_write(const UINT8_T *payload, UINT32_T length,
                                  BOOLEAN_T *mutation_started)
{
    static const UINT8_T magic[8] =
        { 'R', 'D', 'X', 'S', 'E', 'R', '0', '1' };
    UINT8_T request[RDX_SERIAL_REQUEST_LENGTH];
    UINT8_T readback[RDX_SERIAL_RECORD_LENGTH];
    UINT8_T checksum[4];
    UINT8_T *sector = (UINT8_T *)datapath_ram->normal_data_buffer;
    UINT32_T expected_crc;
    UINT32_T index;
    BOOLEAN_T erased;

    if (mutation_started == NULL)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    *mutation_started = FALSE;
    if ((payload == NULL) || (length != RDX_SERIAL_REQUEST_LENGTH))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    /* The USB ISR preempts foreground ATA discovery. No data-bearing ATA
     * command can start until it returns, but an already issued DMA must have
     * completed before the normal buffer can become the sector snapshot. */
    if (gio_rdx_cartridge_present() || !rdx_serial_ata_idle())
    {
        return STATUS_SCSI_INVALID_CMD;
    }
    /* BOT receives serial requests in the separate SCSI response allocation.
     * Retain a private request copy while the normal buffer holds the sector. */
    ti_memcpy(request, payload, sizeof(request));
    if (!rdx_serial_equal(request, magic, sizeof(magic)) ||
        (request[12] > 1U) || (request[13] != 0U))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    for (index = 14U; index < RDX_SERIAL_EXPECTED_OFFSET; index++)
    {
        if ((request[index] < '0') || (request[index] > '9'))
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
    }
    if (gio_rdx_cartridge_present() || !rdx_serial_ata_idle())
    {
        return STATUS_SCSI_INVALID_CMD;
    }
    if (SpiOps(OpcodeReadData, RDX_SERIAL_FLASH_ADDRESS, sector,
               RDX_SERIAL_SECTOR_LENGTH, 0U) != STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    expected_crc = (UINT32_T)request[8] | ((UINT32_T)request[9] << 8U) |
                   ((UINT32_T)request[10] << 16U) | ((UINT32_T)request[11] << 24U);
    if ((rdx_serial_sector_crc(sector) != expected_crc) ||
        !rdx_serial_equal(sector, &request[RDX_SERIAL_EXPECTED_OFFSET],
                          RDX_SERIAL_RECORD_LENGTH))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    erased = rdx_serial_record_erased(sector);
    rdx_serial_checksum(sector, checksum);
    if (erased)
    {
        if (request[12] != 1U)
        {
            return STATUS_SCSI_INVALID_CMD_FIELD;
        }
        rdx_serial_initialize_fallback(sector);
    }
    else if (!rdx_serial_equal(sector, checksum, sizeof(checksum)) ||
             (request[12] != 0U))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    else if (rdx_serial_equal(&sector[RDX_SERIAL_VALUE_OFFSET], &request[14],
                              RDX_SERIAL_VALUE_LENGTH))
    {
        return STATUS_OK;
    }
    ti_memcpy(&sector[RDX_SERIAL_VALUE_OFFSET], &request[14],
              RDX_SERIAL_VALUE_LENGTH);
    rdx_serial_checksum(sector, checksum);
    ti_memcpy(sector, checksum, sizeof(checksum));

    /* A newly inserted cartridge must also stop a prepared write. A successful
     * erased-record repair only programs one page; an existing record requires
     * a 4-KiB erase, so retain and restore every byte in that entire sector. */
    if (gio_rdx_cartridge_present() || !rdx_serial_ata_idle())
    {
        return STATUS_SCSI_INVALID_CMD;
    }
    *mutation_started = TRUE;
    if (!erased)
    {
        if ((SpiOps(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
            (SpiOps(OpcodeSectorErase, RDX_SERIAL_FLASH_ADDRESS,
                    NULL, 0U, 0U) != STATUS_OK))
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
    }
    for (index = 0U; index < (erased ? RDX_SERIAL_RECORD_LENGTH :
                              RDX_SERIAL_SECTOR_LENGTH);
         index += RDX_SERIAL_RECORD_LENGTH)
    {
        if (rdx_serial_program_page(index, sector) != STATUS_OK)
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
    }
    for (index = 0U; index < RDX_SERIAL_SECTOR_LENGTH;
         index += RDX_SERIAL_RECORD_LENGTH)
    {
        if ((SpiOps(OpcodeReadData, RDX_SERIAL_FLASH_ADDRESS + index,
                    readback, sizeof(readback), 0U) != STATUS_OK) ||
            !rdx_serial_equal(readback, &sector[index], sizeof(readback)))
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
    }
    return STATUS_OK;
}
