/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*! @file
 * @brief Compare-and-swap restore of the bounded manufacturing flash record.
 */

#include "rdx_manufacturing.h"

#include "ahci.h"
#include "gio.h"
#include "rdx_hardware.h"
#include "rdx_mechanism.h"
#include "reg_io.h"
#include "spi.h"
#include "string.h"
#include "vim_nvic.h"

#define RDX_MFG_ADDRESS                  0x3E000U
#define RDX_MFG_SECTOR_LENGTH             4096U
#define RDX_MFG_RECORD_LENGTH              256U
#define RDX_MFG_EXPECTED_OFFSET             16U
#define RDX_MFG_DESIRED_OFFSET             272U
#define RDX_MFG_INTERRUPT_MASK       0x00200060U

static BOOLEAN_T rdx_mfg_mutation_started;

/** Report a started manufacturing write, including uncertain completion, until reboot. */
BOOLEAN_T rdx_manufacturing_mutation_started(void)
{
    return rdx_mfg_mutation_started;
}

/** Compare complete byte spans without interpreting identity or padding. */
static BOOLEAN_T rdx_mfg_equal(const UINT8_T *left, const UINT8_T *right,
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

/** Decode the little-endian CRC without relying on pointer alignment. */
static UINT32_T rdx_mfg_load_le32(const UINT8_T *bytes)
{
    return (UINT32_T)bytes[0] | ((UINT32_T)bytes[1] << 8U) |
           ((UINT32_T)bytes[2] << 16U) | ((UINT32_T)bytes[3] << 24U);
}

/** Compute CRC-32/ISO-HDLC over every byte in the captured erase sector. */
static UINT32_T rdx_mfg_sector_crc(const UINT8_T *sector)
{
    UINT32_T crc = 0xFFFFFFFFUL;
    UINT32_T index;
    UINT32_T bit;
    for (index = 0U; index < RDX_MFG_SECTOR_LENGTH; index++)
    {
        crc ^= sector[index];
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = (crc >> 1U) ^ ((crc & 1U) ? 0xEDB88320UL : 0U);
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

/** Reject erased data and require the exact four-lane checksum. */
static BOOLEAN_T rdx_mfg_desired_valid(const UINT8_T *record)
{
    UINT8_T lanes[4] = {0x78U, 0x56U, 0x34U, 0x12U};
    UINT8_T erased = 0xFFU;
    UINT32_T index;
    for (index = 0U; index < RDX_MFG_RECORD_LENGTH; index++)
    {
        erased &= record[index];
        if (index >= 4U)
        {
            lanes[(index - 4U) & 3U] += record[index];
        }
    }
    return (erased != 0xFFU) && rdx_mfg_equal(record, lanes, sizeof(lanes));
}

/** Require fresh physical absence and idle mechanism/SATA work, even after unload. */
static BOOLEAN_T rdx_mfg_bay_is_safe(void)
{
    UINT32_T index;
    if (gio_rdx_cartridge_present() || rdx_mechanism_is_active() ||
        rdx_hardware_eject_in_progress() || ata_dev[0].bDeviceInitComplete ||
        ((READ32(PxSSTS(0)) & PSSTS_DET_MASK) != 0U) ||
        ((READ32(PxCI(0)) | READ32(PxSACT(0))) != 0U))
    {
        return FALSE;
    }
    for (index = 0U; index < ATA_CALLBACK_QUEUE_DEPTH; index++)
    {
        if (ata_dev[0].callback_pending[index])
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * @brief Restore an exclusively owned sector and verify all 4096 resulting bytes.
 *
 * @param payload Validated immutable request in scsi_response_buffer.
 * @return SCSI success after full verification; failure may leave a partially
 *         programmed sector after erase, so no automatic retry/reset is done.
 */
static STATUS_T rdx_mfg_restore_locked(const UINT8_T *payload)
{
    UINT8_T verify[RDX_MFG_RECORD_LENGTH];
    UINT8_T *sector = (UINT8_T *)datapath_ram->normal_data_buffer;
    UINT32_T offset;

    /* BOT received the request outside ATA scratch memory. USB and SATA
     * sources are now masked and ATA is idle, so its normal buffer can hold
     * the full sector without a 4-KiB stack allocation or request overlap. */
    if (SpiOpsBounded(OpcodeReadData, RDX_MFG_ADDRESS, sector,
                     RDX_MFG_SECTOR_LENGTH, 0U) != STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    if (!rdx_mfg_equal(sector, &payload[RDX_MFG_EXPECTED_OFFSET],
                       RDX_MFG_RECORD_LENGTH) ||
        (rdx_mfg_sector_crc(sector) != rdx_mfg_load_le32(&payload[8])))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    /* GPIO5 is sampled again after the full flash read and CRC work. Logical
     * no-media and GPIO2's mechanism endpoint are not physical absence. */
    if (!rdx_mfg_bay_is_safe())
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    if (rdx_mfg_equal(sector, &payload[RDX_MFG_DESIRED_OFFSET],
                      RDX_MFG_RECORD_LENGTH))
    {
        return STATUS_OK;
    }
    ti_memcpy(sector, &payload[RDX_MFG_DESIRED_OFFSET], RDX_MFG_RECORD_LENGTH);

    /* Even uncertain write-enable/erase completion requires a manual restart.
     * Never admit another identity write or an automatic update activation. */
    rdx_mfg_mutation_started = TRUE;
    if (SpiOpsBounded(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    /* WREN can itself take time: check the physical boundary immediately
     * before issuing the irreversible erase command. */
    if (!rdx_mfg_bay_is_safe())
    {
        if (SpiOpsBounded(OpcodeWriteDisable, 0U, NULL, 0U, 0U) != STATUS_OK)
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    if (SpiOpsBounded(OpcodeSectorErase, RDX_MFG_ADDRESS, NULL, 0U, 0U) !=
        STATUS_OK)
    {
        return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    for (offset = 0U; offset < RDX_MFG_SECTOR_LENGTH;
         offset += RDX_MFG_RECORD_LENGTH)
    {
        if ((SpiOpsBounded(OpcodeWriteEnable, 0U, NULL, 0U, 0U) != STATUS_OK) ||
            (SpiOpsBounded(OpcodePageProgram, RDX_MFG_ADDRESS + offset,
                          &sector[offset], RDX_MFG_RECORD_LENGTH, 0U) != STATUS_OK))
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
    }
    for (offset = 0U; offset < RDX_MFG_SECTOR_LENGTH;
         offset += RDX_MFG_RECORD_LENGTH)
    {
        if ((SpiOpsBounded(OpcodeReadData, RDX_MFG_ADDRESS + offset,
                          verify, sizeof(verify), 0U) != STATUS_OK) ||
            !rdx_mfg_equal(verify, &sector[offset], sizeof(verify)))
        {
            return STATUS_SCSI_INTERNAL_TARGET_FAILURE;
        }
    }
    return STATUS_OK;
}

/** Validate one exact capability or compare-and-swap request and preserve ownership. */
STATUS_T rdx_manager_handle_manufacturing(const UINT8_T *cdb,
                                         UINT32_T host_length,
                                         const UINT8_T *payload)
{
    static const UINT8_T probe[10] = {0x3BU, 2U, 0x4DU, 0U, 0U, 1U, 0U, 0U, 0U, 0U};
    static const UINT8_T restore[10] = {0x3BU, 2U, 0x4DU, 0U, 0U, 0U, 0U, 2U, 0x10U, 0U};
    static const UINT8_T magic[8] = {'R', 'D', 'X', 'M', 'F', 'G', '0', '1'};
    UINT32_T saved_interrupts;
    STATUS_T status;

    if (cdb == NULL)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    if (rdx_mfg_equal(cdb, probe, sizeof(probe)) && (host_length == 0U))
    {
        return STATUS_OK;
    }
    if (!rdx_mfg_equal(cdb, restore, sizeof(restore)) ||
        (host_length != RDX_MANUFACTURING_TRANSFER_LENGTH) ||
        (payload == NULL) || !rdx_mfg_equal(payload, magic, sizeof(magic)) ||
        payload[12] || payload[13] || payload[14] || payload[15] ||
        !rdx_mfg_desired_valid(&payload[RDX_MFG_DESIRED_OFFSET]))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (rdx_mfg_mutation_started)
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    saved_interrupts = READ_REG32(VIM_REQMASKSET0) & RDX_MFG_INTERRUPT_MASK;
    WRITE_REG32(VIM_REQMASKCLR0, RDX_MFG_INTERRUPT_MASK);
    status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    if (rdx_mfg_bay_is_safe() && rdx_spi_acquire())
    {
        status = rdx_mfg_restore_locked(payload);
        rdx_spi_release();
    }
    /* Restore only sources enabled on entry; never revive discovery's masked
     * SATA sources, and do not publish changed identity/profile mid-boot. */
    WRITE_REG32(VIM_REQMASKSET0, saved_interrupts);
    return status;
}
