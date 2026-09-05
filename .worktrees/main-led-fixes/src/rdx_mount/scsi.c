/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : scsi.c
//
// Project     : TUSB926x Firmware.
//
// Description : SCSI layer handles SCSI commands from USB mass storage blocks and
//               translates applicable SCSI commands to SATA commands.
//               Compliant to ATA8-ACS Rev 6, SBC-3 Rev 16, SPC-4 Rev 16, MMC-5 Rev 4,
//               and SAT-3 Rev 2 where applicable.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   03/04/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 *
 * This file contains the implementation of the SCSI layer.
 *
 * The SCSI layer handles SCSI commands from the active mass storage class (BOT or UAS)
 * by generating responses directly or by translating SCSI commands to their
 * ATA counterparts to be executed by the SATA AHCI block.
 *
 * The SCSI layer is compliant to the following specifications unless otherwise noted:
 * - AT Attachment 8 - ATA/ATAPI Command Set (ATA8-ACS), Revision 6.
 * - Multi-Media Commands 5 (MMC-5), Revision 4.
 * - SCSI/ATA Translation 3 (SAT-3), Revision 2.
 * - SCSI Block Commands 3 (SBC-3), Revision 16.
 * - SCSI Primary Commands 4 (SPC-4), Revision 16.
 *
 */

#include "scsi.h"
#include "ahci.h"
#include "mww.h"
#include "one_touch.h"
#include "rdx_hardware.h"
#include "rdx_manager_protocol.h"
#include "sata_media.h"
#include "reg_io.h"
#include "sci.h"
#include "scsi_data.h"
#include "spi.h"
#include "string.h"
#include "system.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "ums_bot.h"
#include "usb_hal.h"
#include "vim_nvic.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

/** (Default = 0) Set to 1 to allow host OS to disable the SATA 
 *  device write cache via Mode Select command. Write
 *  performance will be significantly degraded if the host
 *  disables the write cache. */
#define ENABLE_WRITE_CACHE_ENABLE_CHANGEABLE  0

/** (Default = 0) Set to 1 to allow sending ATA SMART Execute
 *  Offline Immediate in response to a SCSI Send Diagnostic
 *  command for self-test. This command may take several minutes
 *  to complete when enabled. */
#define ENABLE_SMART_EXEC_OFFLINE_IMMED  0

#if SCSI_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif

/* NACA (Normal ACA) bit specifies whether an auto contingent 
allegiance (ACA) is established if the command terminates with 
CHECK CONDITION status.  ACA is not supported in firmware. */
#define NACA_CONTROL_BIT   0x04

#define SCSI_SATA_RX_ERROR_INTERRUPT_MASK 0x00000020U
#define SCSI_AHCI_INTERRUPT_MASK          0x00000040U
#define SCSI_CONTROLLER_INTERRUPT_MASK \
    (SCSI_SATA_RX_ERROR_INTERRUPT_MASK | SCSI_AHCI_INTERRUPT_MASK)
#define SCSI_INTERMEDIATE_COMPLETION_STATUS \
    (D2H_REGISTER_FIS_INTR | TASK_FILE_ERROR_STATUS)

/*----------------------------------------------------------------------------+
| Globals                                                                     |
+----------------------------------------------------------------------------*/

SCSI_CMD_INFO_T scsi_cmd;
volatile UINT8_T* scsi_resp_buff = NULL;
UINT32_T scsi_resp_buff_sz;
static BOOLEAN_T scsi_medium_removal_prevented = FALSE;

/**
 * @brief Report the persisted PREVENT/ALLOW MEDIUM REMOVAL state.
 *
 * @return TRUE when medium removal is prohibited, otherwise FALSE.
 */
BOOLEAN_T scsi_medium_removal_is_prevented(void)
{
    return scsi_medium_removal_prevented;
}

/** Clear PREVENT state for the hidden-button mode-two callback only. */
void scsi_clear_medium_removal_prevented(void)
{
    /* This is the same scoped state read before an eject request returns
     * MEDIUM REMOVAL PREVENTED; no sense data is injected asynchronously. */
    scsi_medium_removal_prevented = FALSE;
}

/*****************************************************************************
 * Function: scsi_init
 *************************************************************************//**
 * This function initializes the SCSI module.
 *
 * @param None.
 *
 * @return TRUE when the media layout remained admitted while the FIS was built.
 *
 ******************************************************************************
 */

void scsi_init(void)
{
    scsi_resp_buff = datapath_ram->scsi_response_buffer;
    scsi_resp_buff_sz = SCSI_RESPONSE_BUFF_SIZE;
    scsi_medium_removal_prevented = FALSE;
    rdx_manager_protocol_init();

    return;
}

/*****************************************************************************
 * Function: scsi_is_rw_cmd
 *************************************************************************//**
 * This function determines whether SCSI command is a Read or Write command.
 *
 * @param[in] scsi_cmd 8-bit SCSI command.
 *
 * @retval TRUE when SCSI command is a Read or Write command.
 * @retval FALSE otherwise.
 *
 ******************************************************************************
 */

BOOLEAN_T scsi_is_rw_cmd(UINT8_T scsi_cmd)
{
    switch (scsi_cmd)
    {
        case SCSI_READ6:
        case SCSI_READ10:
        case SCSI_READ12:
        case SCSI_READ16:
        case SCSI_WRITE6:
        case SCSI_WRITE10:
        case SCSI_WRITE12:
        case SCSI_WRITE16:
        case SCSI_READ_CD:
            return TRUE;

        default:
            return FALSE;
    }
}

#define ATA_PASS_THROUGH_CK_COND_BIT  0x20

/*****************************************************************************
 * Function: scsi_is_check_condition_set
 *************************************************************************//**
 * This function determines whether the Check Condition bit is set for
 * and ATA PASS-THROUGH command.
 *
 * @param[in] cdb pointer to the command descriptor block.
 *
 * @retval TRUE when SCSI command is ATA PASS-THROUGH and the Check Condition bit is set.
 * @retval FALSE otherwise.
 *
 ******************************************************************************
 */

BOOLEAN_T scsi_is_check_condition_set(const UINT8_T* cdb)
{
    BOOLEAN_T flag = FALSE;

    switch (cdb[0])
    {
        case ATA_PASS_THROUGH12:
        case ATA_PASS_THROUGH16:
            if (cdb[2] & ATA_PASS_THROUGH_CK_COND_BIT)
            {
                flag = TRUE;
            }
            break;

        default:
            break;
    }

    return flag;
}


/*****************************************************************************
 * Function: scsi_is_write_cmd
 *************************************************************************//**
 * This function determines whether SCSI command is a Write command.
 *
 * @param[in] scsi_cmd 8-bit SCSI command.
 *
 * @retval TRUE when SCSI command is a Write command.
 * @retval FALSE otherwise.
 *
 ******************************************************************************
 */

BOOLEAN_T scsi_is_write_cmd(UINT8_T scsi_cmd)
{
    switch (scsi_cmd)
    {
        case SCSI_WRITE6:
        case SCSI_WRITE10:
        case SCSI_WRITE12:
        case SCSI_WRITE16:
            return TRUE;

        default:
            return FALSE;
    }
}

#define RDX_SCSI_WRITE_AND_VERIFY10 0x2EU
#define RDX_SCSI_WRITE_AND_VERIFY12 0xAEU
#define RDX_SCSI_WRITE_AND_VERIFY16 0x8EU

/**
 * @brief Match the exact write-protect command class.
 *
 * WRITE(6/10/12/16), WRITE AND VERIFY(10/12/16), and UNMAP consult the
 * physical write-protect state. UNMAP is destructive media mutation even
 * though it carries range descriptors instead of ordinary write payload.
 * FORMAT UNIT and unrestricted ATA PASS-THROUGH retain their separately
 * defined command policies.
 *
 * @param cdb Command descriptor block to classify.
 * @return TRUE only for a write-class opcode covered by the physical lock.
 */
static BOOLEAN_T scsi_is_media_write_class_cmd(const UINT8_T *cdb)
{
    switch (cdb[0])
    {
        case SCSI_WRITE6:
        case SCSI_WRITE10:
        case SCSI_WRITE12:
        case SCSI_WRITE16:
        case RDX_SCSI_WRITE_AND_VERIFY10:
        case RDX_SCSI_WRITE_AND_VERIFY12:
        case RDX_SCSI_WRITE_AND_VERIFY16:
        case SCSI_UNMAP:
            return TRUE;

        default:
            return FALSE;
    }
}

#define SCSI_AHCI_MAX_TRANSFER_BYTES \
    ((UINT32_T)AHCI_MAX_SCAT_GATH * \
     (UINT32_T)MWW_VIRTUAL_WINDOW_SIZE)
#define SCSI_LBA28_MAX_TRANSFER_BLOCKS 256U

/**
 * @brief Calculate the largest read/write request representable by AHCI.
 *
 * @param lun SATA logical unit.
 * @param sector_size Bytes per block in the address space being described.
 * @return Maximum block count, or zero for an invalid sector size.
 */
static UINT32_T scsi_get_max_rw_blocks(
    UINT32_T lun,
    UINT32_T sector_size)
{
    UINT32_T maximum_blocks;

    if ((lun >= NUM_AHCI_PORTS) || (sector_size == 0U))
    {
        return 0U;
    }
    maximum_blocks = SCSI_AHCI_MAX_TRANSFER_BYTES / sector_size;
    if (!ata_dev[lun].bLBA48 &&
        !(ata_dev[lun].bNCQ &&
          (usb_dev.active_mass_storage_class == USB_MSC_UAS)) &&
        (maximum_blocks > SCSI_LBA28_MAX_TRANSFER_BLOCKS))
    {
        maximum_blocks = SCSI_LBA28_MAX_TRANSFER_BLOCKS;
    }
    return maximum_blocks;
}


/*****************************************************************************
 * Function: scsi_get_rw_xfer_length
 *************************************************************************//**
 * This function returns the transfer length of a SCSI read
 * or write command in bytes.
 *
 * @param[in] cdb pointer to SCSI command descriptor block.
 * @param[in] lun logical unit number.
 * @param[out] scsi_xfer_length pointer to memory to store transfer length in bytes.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the SCSI command is not supported.
 *
 ******************************************************************************
 */

STATUS_T scsi_get_rw_xfer_length(const UINT8_T *cdb, UINT8_T lun, UINT32_T *scsi_xfer_length)
{
    STATUS_T status = STATUS_OK;
    UINT32_T xfer_blocks = 0;
    UINT32_T sector_size;

    switch (cdb[0])
    {
        case SCSI_WRITE6:
        case SCSI_READ6:
            xfer_blocks = cdb[4];
            if (xfer_blocks == 0) xfer_blocks = 256;
            break;

        case SCSI_READ10:
        case SCSI_WRITE10:
            xfer_blocks = (cdb[7] << 8) | cdb[8];
            break;

        case SCSI_READ12:
        case SCSI_WRITE12:
            xfer_blocks = (cdb[6] << 24) | (cdb[7] << 16) | (cdb[8] << 8) | cdb[9];
            break;

        case SCSI_READ16:
        case SCSI_WRITE16:
            xfer_blocks = (cdb[10] << 24) | (cdb[11] << 16) | (cdb[12] << 8) | cdb[13];
            break;

        default:
            status = STATUS_ERROR;
            break;
    }

    /* This check is not valid because LBA-28 drives that support NCQ can use
     * 48-bit Read/Write FPDMA commands */
    //if (!ata_dev[lun].bLBA48 && (xfer_blocks > 256))
    //{
    //    CRIT("@Error: xfer length of %u blocks not supported by LBA-28!\n");
    //    xfer_blocks = 256;
    //    status = STATUS_NOT_SUPPORTED;
    //}

    sector_size = ata_dev[lun].bDeviceInitComplete ?
        ata_dev[lun].dSectorSize : DEFAULT_ATA_SECTOR_SIZE;
    if ((sector_size == 0U) ||
        (xfer_blocks > (0xFFFFFFFFU / sector_size)) ||
        (ata_dev[lun].bDeviceInitComplete &&
         (xfer_blocks > scsi_get_max_rw_blocks(lun, sector_size))))
    {
        *scsi_xfer_length = 0U;
        status = STATUS_NOT_SUPPORTED;
    }
    else
    {
        *scsi_xfer_length = xfer_blocks * sector_size;
    }

    INFO("-> scsi_get_rw_xfer_length() - %u bytes.\n", *scsi_xfer_length);

    return status;
}



/*****************************************************************************
 * Function: scsi_set_sense_data
 *************************************************************************//**
 * This function sets SCSI sense data.
 *
 * @param[in] key sense key.
 * @param[in] asc additional sense code.
 * @param[in] ascq additional sense code qualifier.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void scsi_set_sense_data(UINT8_T key, ASC_CODE_T asc, ASCQ_CODE_T ascq)
{
    DEBUG("-> scsi_set_sense_data() - Key = 0x%x, ASC = 0x%x, ASCQ = 0x%x.\n", key, asc, ascq);

    // Reset data fields which may have been set by ATA PASS-THROUGH command.
    fixed_format_sense_data[0]  = 0x70;            /* Valid = 0, response code = 0x70 */
    ti_memset(&fixed_format_sense_data[3], 0, 4);  /* Command Information */
    ti_memset(&fixed_format_sense_data[8], 0, 4);  /* Command-specific Info */

    fixed_format_sense_data[2]  = key;
    fixed_format_sense_data[12] = asc;
    fixed_format_sense_data[13] = ascq;

    return;
}


/*****************************************************************************
 * Function: scsi_validate_LBA_range
 *************************************************************************//**
 * This function verifies that the range of LBAs for a transfer
 * is within the bounds of the logical unit.
 *
 * @param None.
 *
 * @retval STATUS_OK when range is valid.
 * @retval STATUS_SCSI_INVALID_ADDRESS_RANGE when address range is invalid.
 *
 ******************************************************************************
 */

STATUS_T scsi_validate_LBA_range(void)
{
    UINT64_T *lba = (UINT64_T *)&scsi_cmd.bLBA[0];

    INFO("-> scsi_validate_LBA_range()\n");

    // Verify LBA and xfer length yield ending block number that is in range.
    /* RDX presents only its metadata-defined user-data extent; generic SATA
     * presents native capacity. ddMaxLBA is the selected host-visible limit. */
    if ((*lba + scsi_cmd.dXferLength) >
        ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA)
    {
        CRIT("@Error: Invalid LBA range specified!\n");
        CRIT("LBA:  0x%08x 0x%08x  Xfer Length = %u.\n", (UINT32_T)(*lba >> 32), (UINT32_T)(*lba & 0xFFFFFFFF), scsi_cmd.dXferLength);
        return STATUS_SCSI_INVALID_ADDRESS_RANGE;
    }
    else
    {
        return STATUS_OK;
    }

// BQ TODO:  When a command attempts to access or reference an invalid LBA, the device server shall return the first
//invalid LBA in the INFORMATION field of the sense data (see SBC-3 pg 23).

}


#define SCSI_FUA_BIT  0x08   /* 2nd byte in command block */

/*****************************************************************************
 * Function: scsi_get_rw_params
 *************************************************************************//**
 * This function extracts the LBA and transfer length information
 * for a SCSI read or write command.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when tranfer length is zero.
 * @retval STATUS_ERROR when the SCSI command is not supported.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_get_rw_params(void)
{
    STATUS_T status = STATUS_OK;

    INFO("-> scsi_get_rw_params()\n");

    switch (scsi_cmd.pCmdInput->pCommandBlock[0])
    {
        case SCSI_WRITE6:
        case SCSI_READ6:
            // Check control byte.
            if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
            {
                status = STATUS_SCSI_INVALID_CMD_FIELD;
            }

            scsi_cmd.bLBA[2] = scsi_cmd.pCmdInput->pCommandBlock[1] & 0x1F;
            scsi_cmd.bLBA[1] = scsi_cmd.pCmdInput->pCommandBlock[2];
            scsi_cmd.bLBA[0] = scsi_cmd.pCmdInput->pCommandBlock[3];
            scsi_cmd.dXferLength = scsi_cmd.pCmdInput->pCommandBlock[4];
            if (scsi_cmd.dXferLength == 0) scsi_cmd.dXferLength = 256;
            break;

        case SCSI_READ10:
        case SCSI_WRITE10:
        case SCSI_READ12:
        case SCSI_WRITE12:
            if (scsi_cmd.pCmdInput->pCommandBlock[1] & SCSI_FUA_BIT)
            {
                scsi_cmd.bFUA = TRUE;
            }

            scsi_cmd.bLBA_exp[0] = scsi_cmd.pCmdInput->pCommandBlock[2];
            scsi_cmd.bLBA[2] = scsi_cmd.pCmdInput->pCommandBlock[3];
            scsi_cmd.bLBA[1] = scsi_cmd.pCmdInput->pCommandBlock[4];
            scsi_cmd.bLBA[0] = scsi_cmd.pCmdInput->pCommandBlock[5];
            if ((scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_READ12) || (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_WRITE12))
            {
                // Check control byte.
                if (scsi_cmd.pCmdInput->pCommandBlock[11] & NACA_CONTROL_BIT)
                {
                    status = STATUS_SCSI_INVALID_CMD_FIELD;
                }

                scsi_cmd.dXferLength = (scsi_cmd.pCmdInput->pCommandBlock[6] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[7] << 16) | (scsi_cmd.pCmdInput->pCommandBlock[8] << 8) | scsi_cmd.pCmdInput->pCommandBlock[9];
            }
            else /* 10-byte cmd */
            {
                // Check control byte.
                if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
                {
                    status = STATUS_SCSI_INVALID_CMD_FIELD;
                }
                scsi_cmd.dXferLength = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) | scsi_cmd.pCmdInput->pCommandBlock[8];
            }
            break;

        case SCSI_READ16:
        case SCSI_WRITE16:
            // Check control byte.
            if (scsi_cmd.pCmdInput->pCommandBlock[15] & NACA_CONTROL_BIT)
            {
                status = STATUS_SCSI_INVALID_CMD_FIELD;
            }

            if (scsi_cmd.pCmdInput->pCommandBlock[1] & SCSI_FUA_BIT)
            {
                scsi_cmd.bFUA = TRUE;
            }

            /* ATA commands carry at most 48 LBA bits. Reject the upper two
             * SCSI bytes instead of silently aliasing them into low media. */
            if ((scsi_cmd.pCmdInput->pCommandBlock[2] != 0U) ||
                (scsi_cmd.pCmdInput->pCommandBlock[3] != 0U))
            {
                status = STATUS_SCSI_INVALID_ADDRESS_RANGE;
            }

            scsi_cmd.bLBA_exp[2] = scsi_cmd.pCmdInput->pCommandBlock[4];
            scsi_cmd.bLBA_exp[1] = scsi_cmd.pCmdInput->pCommandBlock[5];
            scsi_cmd.bLBA_exp[0] = scsi_cmd.pCmdInput->pCommandBlock[6];
            scsi_cmd.bLBA[2] = scsi_cmd.pCmdInput->pCommandBlock[7];
            scsi_cmd.bLBA[1] = scsi_cmd.pCmdInput->pCommandBlock[8];
            scsi_cmd.bLBA[0] = scsi_cmd.pCmdInput->pCommandBlock[9];
            scsi_cmd.dXferLength = (scsi_cmd.pCmdInput->pCommandBlock[10] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[11] << 16) | (scsi_cmd.pCmdInput->pCommandBlock[12] << 8) | scsi_cmd.pCmdInput->pCommandBlock[13];
            break;

        default:
            status = STATUS_ERROR;
            break;
    }

    if (status == STATUS_OK)
    {
#if ENABLE_LARGE_SECTOR_EMULATION
        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bLargeSectorEmulation)
        {
            UINT64_T *lba = (UINT64_T *)&scsi_cmd.bLBA[0];

            // Translate LBA for native logical sector size.
            *lba = (*lba << 3) + ata_dev[scsi_cmd.pCmdInput->bLUN].wLowestAlignedLBA;  // Multiply by 8 and add offset to first aligned LBA.

            // Translate transfer length for native logical sector size.
            scsi_cmd.dXferLength = scsi_cmd.dXferLength << 3;  // Multiply by 8.
        }
#endif
        if (scsi_cmd.dXferLength == 0)
        {
            CRIT("@Warning: Zero transfer length specified in SCSI R/W cmd block.\n");
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }
    }

    return status;
}



/*****************************************************************************
 * Function: scsi_get_verify_params
 *************************************************************************//**
 * This function extracts LBA and verfication length info from
 * a SCSI VERIFY command.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the SCSI command is not supported.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_get_verify_params(void)
{
    STATUS_T status = STATUS_OK;

    switch (scsi_cmd.pCmdInput->pCommandBlock[0])
    {
        case SCSI_VERIFY10:
        case SCSI_VERIFY12:
            scsi_cmd.bLBA_exp[2] = 0;
            scsi_cmd.bLBA_exp[1] = 0;
            scsi_cmd.bLBA_exp[0] = scsi_cmd.pCmdInput->pCommandBlock[2];
            scsi_cmd.bLBA[2] = scsi_cmd.pCmdInput->pCommandBlock[3];
            scsi_cmd.bLBA[1] = scsi_cmd.pCmdInput->pCommandBlock[4];
            scsi_cmd.bLBA[0] = scsi_cmd.pCmdInput->pCommandBlock[5];

            if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_VERIFY12)
            {
                // Check control byte.
                if (scsi_cmd.pCmdInput->pCommandBlock[11] & NACA_CONTROL_BIT)
                {
                    status = STATUS_SCSI_INVALID_CMD_FIELD;
                }

                scsi_cmd.dXferLength = (scsi_cmd.pCmdInput->pCommandBlock[6] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[7] << 16) | (scsi_cmd.pCmdInput->pCommandBlock[8] << 8) | scsi_cmd.pCmdInput->pCommandBlock[9];
            }
            else /* 10-byte cmd */
            {
                // Check control byte.
                if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
                {
                    status = STATUS_SCSI_INVALID_CMD_FIELD;
                }

                scsi_cmd.dXferLength = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) | scsi_cmd.pCmdInput->pCommandBlock[8];
            }
            break;

        case SCSI_VERIFY16:
            // Check control byte.
            if (scsi_cmd.pCmdInput->pCommandBlock[15] & NACA_CONTROL_BIT)
            {
                status = STATUS_SCSI_INVALID_CMD_FIELD;
            }
            if ((scsi_cmd.pCmdInput->pCommandBlock[2] != 0U) ||
                (scsi_cmd.pCmdInput->pCommandBlock[3] != 0U))
            {
                status = STATUS_SCSI_INVALID_ADDRESS_RANGE;
            }
            scsi_cmd.bLBA_exp[2] = scsi_cmd.pCmdInput->pCommandBlock[4];
            scsi_cmd.bLBA_exp[1] = scsi_cmd.pCmdInput->pCommandBlock[5];
            scsi_cmd.bLBA_exp[0] = scsi_cmd.pCmdInput->pCommandBlock[6];
            scsi_cmd.bLBA[2] = scsi_cmd.pCmdInput->pCommandBlock[7];
            scsi_cmd.bLBA[1] = scsi_cmd.pCmdInput->pCommandBlock[8];
            scsi_cmd.bLBA[0] = scsi_cmd.pCmdInput->pCommandBlock[9];
            scsi_cmd.dXferLength = (scsi_cmd.pCmdInput->pCommandBlock[10] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[11] << 16) | (scsi_cmd.pCmdInput->pCommandBlock[12] << 8) | scsi_cmd.pCmdInput->pCommandBlock[13];
            break;

        default:
            CRIT("@Error: scsi_get_verify_params() - Unsupported command = 0x%x.\n", scsi_cmd.pCmdInput->pCommandBlock[0]);
            status = STATUS_ERROR;
            break;
    }

#if ENABLE_LARGE_SECTOR_EMULATION
    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bLargeSectorEmulation)
    {
        UINT64_T *lba = (UINT64_T *)&scsi_cmd.bLBA[0];

        // Translate LBA for native logical sector size.
        *lba = (*lba << 3) + ata_dev[scsi_cmd.pCmdInput->bLUN].wLowestAlignedLBA;  // Multiply by 8 and add offset to first aligned LBA.

        // Translate transfer length for native logical sector size.
        scsi_cmd.dXferLength = scsi_cmd.dXferLength << 3;  // Multiply by 8.
    }
#endif

    return status;

}


/*****************************************************************************
 * Function: scsi_get_ata_rw_cmd
 *************************************************************************//**
 * This function translates a SCSI read or write command to its
 * non-queued ATA counterpart.
 *
 * @param[in] is_write_cmd flag to indicate whether command is a write.
 * @param[in] bFUA flag to indicate whether FUA (Forced Unit Access) is used for a write.
 *
 * @returns The ATA command.
 *
 ******************************************************************************
 */

inline UINT8_T scsi_get_ata_rw_cmd(BOOLEAN_T is_write_cmd, BOOLEAN_T bFUA)
{
    if (is_write_cmd)
    {
        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48)
        {
            return (bFUA) ? ATA_CMD_WRITE_DMA_FUA_EXT : ATA_CMD_WRITE_DMA_EXT;
        }
        else
        {
            return ATA_CMD_WRITE_DMA;
        }
    }
    else /* Read command */
    {
        return(ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48) ? ATA_CMD_READ_DMA_EXT : ATA_CMD_READ_DMA;
    }
}



#define ATA_LBA_BIT    0x40
#define ATA_FUA_BIT    0x80  /* for FPDMA cmds only */

/*****************************************************************************
 * Function: scsi_build_ata_rw_cmd
 *************************************************************************//**
 * This function builds an ATA read or write command using
 * globally stored SCSI info.
 *
 * @param[in] ata_cmd pointer to ATA command structure.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline BOOLEAN_T scsi_build_ata_rw_cmd(ATA_COMMAND_T *ata_cmd)
{
    BOOLEAN_T write_cmd;
    UINT64_T physical_lba;

    INFO("-> scsi_build_ata_rw_cmd() - xfer_length = %u.\n", scsi_cmd.dXferLength);

    ti_memset(ata_cmd, 0U, sizeof(*ata_cmd));

    // Check if WRITE cmd.
    write_cmd = scsi_is_write_cmd(scsi_cmd.pCmdInput->pCommandBlock[0]);

    // Build command FIS.
    ata_cmd->fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd->fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */

    /* RDX media maps through its authenticated data extent. Generic SATA
     * media keeps the host LBA unchanged, including all upper address bits. */
    physical_lba = ((UINT64_T)scsi_cmd.bLBA_exp[2] << 40) |
                   ((UINT64_T)scsi_cmd.bLBA_exp[1] << 32) |
                   ((UINT64_T)scsi_cmd.bLBA_exp[0] << 24) |
                   ((UINT64_T)scsi_cmd.bLBA[2] << 16) |
                   ((UINT64_T)scsi_cmd.bLBA[1] << 8) |
                   (UINT64_T)scsi_cmd.bLBA[0];
    if (!sata_media_translate_lba(
            scsi_cmd.pCmdInput->bLUN, physical_lba, &physical_lba))
    {
        return FALSE;
    }

    ata_cmd->fis.LBA_low = (UINT8_T)physical_lba;
    ata_cmd->fis.LBA_mid = (UINT8_T)(physical_lba >> 8);
    ata_cmd->fis.LBA_high = (UINT8_T)(physical_lba >> 16);
    ata_cmd->fis.device = ATA_LBA_BIT;
    ata_cmd->fis.LBA_low_exp = (UINT8_T)(physical_lba >> 24);
    ata_cmd->fis.LBA_mid_exp = (UINT8_T)(physical_lba >> 32);
    ata_cmd->fis.LBA_high_exp = (UINT8_T)(physical_lba >> 40);

    ata_cmd->fis.control = 0x00;

    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bNCQ && (usb_dev.active_mass_storage_class == USB_MSC_UAS))
    {
        // Issue FPDMA cmd.
        ata_cmd->fis.command = (write_cmd) ? ATA_CMD_WRITE_FPDMA_QUEUED : ATA_CMD_READ_FPDMA_QUEUED;
        ata_cmd->fis.features = scsi_cmd.dXferLength & 0xFF;
        ata_cmd->fis.features_exp = (scsi_cmd.dXferLength >> 8) & 0xFF;
        ata_cmd->fis.sector_cnt = (scsi_cmd.pCmdInput->bCmdSlotNum << 3);  // Populate TAG value.
        ata_cmd->fis.sector_cnt_exp = 0x00;  /* Normal priority only */

        if (scsi_cmd.bFUA)
        {
            ata_cmd->fis.device |= ATA_FUA_BIT;
        }
    }
    else /* BOT or non-NCQ */
    {
        ata_cmd->fis.command = scsi_get_ata_rw_cmd(write_cmd, scsi_cmd.bFUA);
        ata_cmd->fis.features = 0x00;
        ata_cmd->fis.features_exp = 0x00;
        ata_cmd->fis.sector_cnt = scsi_cmd.dXferLength & 0xFF;

        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48)
        {
            ata_cmd->fis.sector_cnt_exp = (scsi_cmd.dXferLength >> 8) & 0xFF;
        }
        else /* LBA28 */
        {
            ata_cmd->fis.sector_cnt_exp = 0x00;
            ata_cmd->fis.device |= (UINT8_T)(physical_lba >> 24) & 0x0F;
        }
    }

    // Set reserved fields to zero.
    ata_cmd->fis.rsvd0 = 0x00;
    ata_cmd->fis.rsvd1 = 0x00000000;

    // Set other data req'd for ATA cmd.
    ata_cmd->bIsWriteCmd = write_cmd;
    ata_cmd->bUseMemoryWrapWindow = TRUE;
    ata_cmd->dDataByteCnt = scsi_cmd.dXferLength * ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize;
    ata_cmd->bCheckCondition = FALSE;

    return TRUE;
}


/**
 * @brief Verify that SCSI submission still targets its captured media epoch.
 *
 * Call this only with both SATA interrupt sources masked. Besides software
 * readiness and classification, the live PHY and latched connect-change bit
 * are checked so build/issue cannot knowingly cross a physical link boundary.
 *
 * @param lun SATA logical unit.
 * @return TRUE while the CDB's captured media remains physically current.
 */
static BOOLEAN_T scsi_submission_media_is_current(UINT32_T lun)
{
    return ((lun < NUM_AHCI_PORTS) &&
            ata_dev[lun].bDeviceInitComplete &&
            (sata_media_get_kind(lun) != SATA_MEDIA_KIND_NONE) &&
            (sata_media_get_epoch(lun) == scsi_cmd.dMediaEpoch) &&
            ((READ32(PxSSTS(lun)) & PSSTS_DET_MASK) ==
             PSSTS_DET_PHY_READY) &&
            ((READ32(PxIS(lun)) & PORT_CONNECT_CHANGE_STATUS) == 0U)) ?
           TRUE : FALSE;
}

/*****************************************************************************
 * Function: scsi_send_ata_cmd
 *************************************************************************//**
 * This function builds an ATA read or write command using
 * globally stored SCSI info.
 *
 * @param[in] ata_cmd pointer to ATA command structure.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_ERROR when no empty command slot is found.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 *
 ******************************************************************************
 */

STATUS_T scsi_send_ata_cmd(ATA_COMMAND_T *ata_cmd)
{
    BOOLEAN_T queued_cmd_flag = FALSE;
#if REMOVABLE_MEDIA_DEVICE
    BOOLEAN_T recovery_started = FALSE;
    UINT32_T lun = scsi_cmd.pCmdInput->bLUN;
#endif
    STATUS_T status;

#if REMOVABLE_MEDIA_DEVICE
    /* SATA handlers can preempt the USB command path. Mask both sources across
     * final epoch/link validation, command construction, and slot issue. */
    WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);
    if (!scsi_submission_media_is_current(lun))
    {
        ahci_recover_local_command(lun, FALSE);
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
#endif

    // Build AHCI command.
    status = ahci_build_cmd(
        scsi_cmd.pCmdInput->bLUN,
        ata_cmd,
        scsi_cmd.pCmdInput->bCmdSlotNum);

    if (status == STATUS_OK)
    {
        if (ata_cmd->fis.command == ATA_CMD_WRITE_FPDMA_QUEUED ||
            ata_cmd->fis.command == ATA_CMD_READ_FPDMA_QUEUED)
        {
            queued_cmd_flag = TRUE;
        }

#if REMOVABLE_MEDIA_DEVICE
        if (!scsi_submission_media_is_current(lun))
        {
            ahci_recover_local_command(lun, FALSE);
            recovery_started = TRUE;
            status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
        }
        else
#endif
        {
            // Enable AHCI command slot.
            status = ahci_issue_cmd(
                scsi_cmd.pCmdInput->bLUN,
                scsi_cmd.pCmdInput->bCmdSlotNum,
                queued_cmd_flag);
        }

        if (status == STATUS_OK)
        {
            status = STATUS_SCSI_RESPONSE_PENDING;
        }
        else
        {
            // Port command start bit wasn't set. This should never happen.
            status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
        }
    }

#if REMOVABLE_MEDIA_DEVICE
    if (!recovery_started && !scsi_submission_media_is_current(lun))
    {
        /* A link event can latch while the SATA sources are masked. If issue
         * already claimed the slot, COMRESET retires it before local failure;
         * otherwise simple invalidation is sufficient. */
        ahci_recover_local_command(
            lun,
            ((READ32(PxCI(lun)) &
              (0x1U << scsi_cmd.pCmdInput->bCmdSlotNum)) != 0U) ?
            TRUE : FALSE);
        recovery_started = TRUE;
        status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    if (!recovery_started)
    {
        WRITE_REG32(VIM_REQMASKSET0, SCSI_CONTROLLER_INTERRUPT_MASK);
    }
#endif

    if (status == STATUS_ATA_DEVICE_FAULT)
    {
        // SAT-2 spec says to return internal target failure.
        status = STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    else if (status == STATUS_ATA_CMD_SLOT_BUSY)
    {
        // Command slot was in use. This should never happen.
        status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }

    return status;
}


/*****************************************************************************
 * Function: scsi_handle_rw_cmd
 *************************************************************************//**
 * This function handles SCSI read or write commands.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_ERROR when the SCSI command is not supported or no empty command slot is found.
 * @retval STATUS_SCSI_INVALID_ADDRESS_RANGE when address range is invalid.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when tranfer length is zero.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_rw_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;

    DEBUG("-> scsi_handle_rw_cmd()\n");

    // Extract LBA and transfer length info.
    status = scsi_get_rw_params();

    if (status == STATUS_OK)
    {
        // Verify address is in range.
        status = scsi_validate_LBA_range();
    }

    if ((status == STATUS_OK) &&
        (scsi_cmd.dXferLength > scsi_get_max_rw_blocks(
            scsi_cmd.pCmdInput->bLUN,
            ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize)))
    {
        /* Hosts may ignore block-limits VPD. Reject before byte-count
         * multiplication or PRDT construction instead of truncating DMA. */
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (status == STATUS_OK)
    {
        if (usb_dev.active_mass_storage_class == USB_MSC_BOT)
        {
            if ((scsi_cmd.dXferLength * ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize) > gCBW->dDataTransferLength)
            {
                // Set SCSI transfer length (blocks) to the length specified in the CBW.
                // This is required to pass BOT MSC CV tests.
                scsi_cmd.dXferLength = (gCBW->dDataTransferLength / ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize);
            }

#if DISABLE_WRAP_WINDOW
            if ((scsi_cmd.dXferLength * ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize) > sizeof(datapath_ram->normal_data_buffer))
            {
                /* Derive the diagnostic transfer cap from the actual 4-KiB
                 * ordinary-data buffer and sector size. This prevents the AHCI
                 * PRDT from advertising a transfer larger than the storage
                 * reserved for the command. */
                scsi_cmd.dXferLength =
                    sizeof(datapath_ram->normal_data_buffer) /
                    ata_dev[scsi_cmd.pCmdInput->bLUN].dTrueSectorSize;
            }
#endif
        }

        // Build and send the ATA command only while media remains admitted.
        if (scsi_build_ata_rw_cmd(&ata_cmd))
        {
            status = scsi_send_ata_cmd(&ata_cmd);
        }
        else
        {
            status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
        }
    }

    return status;
}


/*****************************************************************************
 * Function: scsi_build_ata_read_verify_sectors_cmd
 *************************************************************************//**
 * This function builds a ATA READ VERIFY SECTORS command.
 *
 * @param[in] ata_cmd pointer to ATA command structure.
 *
 * @return TRUE when the media layout remained admitted while the FIS was built.
 *
 ******************************************************************************
 */

BOOLEAN_T scsi_build_ata_read_verify_sectors_cmd(ATA_COMMAND_T* ata_cmd)
{
    UINT64_T logical_lba;
    UINT64_T physical_lba;

    ti_memset(ata_cmd, 0, sizeof(ATA_COMMAND_T));

    // Build command FIS.
    ata_cmd->fis.command = (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48) ? ATA_CMD_READ_VERIFY_SECTORS_EXT : ATA_CMD_READ_VERIFY_SECTORS;
    ata_cmd->fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd->fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */

    logical_lba = ((UINT64_T)scsi_cmd.bLBA_exp[2] << 40) |
                  ((UINT64_T)scsi_cmd.bLBA_exp[1] << 32) |
                  ((UINT64_T)scsi_cmd.bLBA_exp[0] << 24) |
                  ((UINT64_T)scsi_cmd.bLBA[2] << 16) |
                  ((UINT64_T)scsi_cmd.bLBA[1] << 8) |
                  (UINT64_T)scsi_cmd.bLBA[0];
    if (!sata_media_translate_lba(
            scsi_cmd.pCmdInput->bLUN, logical_lba, &physical_lba))
    {
        return FALSE;
    }

    /* Apply the same selected media mapping used by READ and WRITE before
     * issuing ATA READ VERIFY. */
    ata_cmd->fis.LBA_low = (UINT8_T)physical_lba;
    ata_cmd->fis.LBA_mid = (UINT8_T)(physical_lba >> 8);
    ata_cmd->fis.LBA_high = (UINT8_T)(physical_lba >> 16);
    ata_cmd->fis.device = ATA_LBA_BIT;
    ata_cmd->fis.LBA_low_exp = (UINT8_T)(physical_lba >> 24);
    ata_cmd->fis.LBA_mid_exp = (UINT8_T)(physical_lba >> 32);
    ata_cmd->fis.LBA_high_exp = (UINT8_T)(physical_lba >> 40);
    if (!ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48)
    {
        ata_cmd->fis.device |= (UINT8_T)(physical_lba >> 24) & 0x0FU;
    }
    ata_cmd->fis.sector_cnt = scsi_cmd.dXferLength & 0xFF;
    ata_cmd->fis.sector_cnt_exp = (scsi_cmd.dXferLength >> 8) & 0xFF;

    return TRUE;
}


/*****************************************************************************
 * Function: scsi_handle_verify_cmd
 *************************************************************************//**
 * This function handles SCSI verify command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_ERROR when the SCSI command is not supported or no empty command slot is found.
 * @retval STATUS_SCSI_INVALID_ADDRESS_RANGE when address range is invalid.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when tranfer length is zero.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_verify_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;

    // Extract command params.
    status = scsi_get_verify_params();

    if (status == STATUS_OK)
    {
        // Verify address is in range.
        status = scsi_validate_LBA_range();
    }

    if (status == STATUS_OK)
    {
        if (scsi_cmd.dXferLength != 0)
        {
            // Build and send only while media remains admitted.
            if (scsi_build_ata_read_verify_sectors_cmd(&ata_cmd))
            {
                status = scsi_send_ata_cmd(&ata_cmd);
            }
            else
            {
                status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
            }
        }
    }

    return status;
}



#define LUN_REPORT_LENGTH_WITHOUT_LIST  8   /* bytes */
#define LUN_ENTRY_LENGTH                8   /* bytes */
#define MAX_SELECT_REPORT_CODE          2
#define MIN_REPORT_LUNS_ALLOC_LENGTH    16  /* bytes */

/*****************************************************************************
 * Function: scsi_handle_report_luns_cmd
 *************************************************************************//**
 * This function handles SCSI REPORT LUNS command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_report_luns_cmd(void)
{
    STATUS_T status = STATUS_OK;
    UINT32_T lun;
    UINT32_T select_report;
    UINT32_T alloc_length;

    select_report = scsi_cmd.pCmdInput->pCommandBlock[2];

    alloc_length = ((scsi_cmd.pCmdInput->pCommandBlock[6] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[7] << 16) |
                    (scsi_cmd.pCmdInput->pCommandBlock[8] << 8) | scsi_cmd.pCmdInput->pCommandBlock[9]);

    DEBUG("-> scsi_handle_report_luns_cmd() - report = %u, alloc_len = %u.\n", select_report, alloc_length);

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[11] & NACA_CONTROL_BIT)
    {
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    // Check if any rsvd fields are set.
    if ((scsi_cmd.pCmdInput->pCommandBlock[1] | scsi_cmd.pCmdInput->pCommandBlock[3] |
         scsi_cmd.pCmdInput->pCommandBlock[4] | scsi_cmd.pCmdInput->pCommandBlock[5]))
    {
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    // Validate report code and allocation length.
    if ((select_report > MAX_SELECT_REPORT_CODE) || (alloc_length < MIN_REPORT_LUNS_ALLOC_LENGTH))
    {
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (status == STATUS_OK)
    {
        // Populate LUN list length (bytes).
        scsi_resp_buff[0] = 0x00;
        scsi_resp_buff[1] = 0x00;
        scsi_resp_buff[2] = 0x00;
        scsi_resp_buff[3] = (LUN_ENTRY_LENGTH * (UMS_MAX_LUN + 1));  /* Add 1 since LUN is zero-based */

        // Populate RSVD field.
        *((UINT32_T*)&scsi_resp_buff[4]) = 0x00000000;

        // Populate LUN list using Single level LUN structure using peripheral device addressing method.
        for (lun = 0; lun <= UMS_MAX_LUN; lun++)
        {
            scsi_resp_buff[(lun * 8) + 8] = 0x00;             /* addr method & bus */
            scsi_resp_buff[(lun * 8) + 9] = lun;              /* LUN */
            ti_memset((void*)&scsi_resp_buff[(lun * 8) + 10], 0, 6);  /* RSVD */
        }

        // Set data pointer and byte count.
        scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
        scsi_cmd.pCmdInput->dDataByteCnt = MIN(alloc_length, (LUN_REPORT_LENGTH_WITHOUT_LIST + scsi_resp_buff[3]));

        status = STATUS_SCSI_RESPONSE_READY;
    }

    return status;
}

#define PREVENT_BIT_MASK  0x3
#define MEDIUM_REMOVAL_ALLOWED    0x0
#define MEDIUM_REMOVAL_PROHIBITED 0x1

/*****************************************************************************
 * Function: scsi_handle_prevent_allow_medium_removal_cmd
 *************************************************************************//**
 * This function handles SCSI prevent/allow medium removal command.
 *
 * @param None.
 *
 * @retval STATUS_OK when either accepted ALLOW form is applied or either
 *         accepted PREVENT form is applied to ready media.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when the unsupported NACA bit is set.
 * @retval STATUS_SCSI_LOGICAL_UNIT_NOT_READY when PREVENT is requested for
 *         unavailable or logically unloaded media.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_prevent_allow_medium_removal_cmd(void)
{
    UINT8_T prevent = (scsi_cmd.pCmdInput->pCommandBlock[4] & PREVENT_BIT_MASK);
    BOOLEAN_T removal_prohibited;

    INFO("-> scsi_handle_prevent_allow_medium_removal_cmd() - val = 0x%x.\n", prevent);

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    /* The two-bit field has paired forms: zero/two allow removal and one/three
     * prohibit it.  Bit zero is the policy value; bit one is accepted context
     * and must not turn the paired forms into invalid-field failures. */
    removal_prohibited = ((prevent & MEDIUM_REMOVAL_PROHIBITED) != 0U) ?
                         TRUE : FALSE;

    /* ALLOW applies unconditionally.  PREVENT requires command-visible ready
     * media; a page 31h logical unload is unavailable even though the
     * underlying ATA device deliberately remains initialized. */
    if (removal_prohibited &&
        (!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete ||
         rdx_hardware_is_logically_unloaded()))
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    scsi_medium_removal_prevented = removal_prohibited;
    return STATUS_OK;
}


#define LOEJ_BIT 0x02
#define START_BIT 0x01
#define NO_FLUSH_BIT 0x04

#define ATA_IDLE_IMMEDIATE_SUPPORT_BIT (1 << 13)

typedef enum
{
    PWR_CONDITION_START_VALID   = 0x0,
    PWR_CONDITION_ACTIVE        = 0x1,
    PWR_CONDITION_IDLE          = 0x2,
    PWR_CONDITION_STANDBY       = 0x3,
    PWR_CONDITION_LU_CONTROL    = 0x7,
    PWR_CONDITION_FORCE_STANDBY = 0xB,

} pwr_condition_t;

#define PWR_CONDITION_START_VALID 0x0
/** Media token for a polled command inside a multi-command SCSI request. */
typedef struct _SCSI_INTERMEDIATE_SESSION_T
{
    UINT32_T media_epoch;
    SATA_MEDIA_KIND_T media_kind;
} SCSI_INTERMEDIATE_SESSION_T;

/**
 * @brief Test whether a polled SCSI command still owns the admitted media.
 *
 * @param lun SATA logical unit.
 * @param session Media token captured before command issue.
 * @return TRUE only while readiness, kind, and epoch remain unchanged.
 */
static BOOLEAN_T scsi_intermediate_session_is_current(
    UINT32_T lun,
    const SCSI_INTERMEDIATE_SESSION_T *session)
{
    return (ata_dev[lun].bDeviceInitComplete &&
            (session->media_kind != SATA_MEDIA_KIND_NONE) &&
            (sata_media_get_kind(lun) == session->media_kind) &&
            (sata_media_get_epoch(lun) == session->media_epoch)) ?
           TRUE : FALSE;
}

/**
 * @brief Reserve completion interrupts for an intermediate ATA command.
 *
 * Connect-change and transport interrupts stay enabled during the wait. The
 * short controller-wide mask closes the check/modify race with both ISRs.
 *
 * @param lun SATA logical unit.
 * @param session Receives the current media token.
 * @return TRUE when intermediate command submission may proceed.
 */
static BOOLEAN_T scsi_begin_intermediate_ata_command(
    UINT32_T lun,
    SCSI_INTERMEDIATE_SESSION_T *session)
{
    if ((lun >= NUM_AHCI_PORTS) || (session == NULL) ||
        !ata_dev[lun].bDeviceInitComplete)
    {
        return FALSE;
    }
    session->media_kind = sata_media_get_kind(lun);
    session->media_epoch = sata_media_get_epoch(lun);
    WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);
    if (!scsi_intermediate_session_is_current(lun, session))
    {
        WRITE32(PxIE(lun), 0U);
        WRITE_REG32(VIM_REQMASKSET0, SCSI_AHCI_INTERRUPT_MASK);
        return FALSE;
    }
    WRITE32(PxIS(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS);
    MODIFY32(PxIE(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS, 0U);
    WRITE_REG32(VIM_REQMASKSET0, SCSI_CONTROLLER_INTERRUPT_MASK);
    return TRUE;
}

/**
 * @brief Complete an intermediate ATA command without reviving stale masks.
 *
 * @param lun SATA logical unit.
 * @param session Media token captured before command issue.
 * @param submission_status Result returned by scsi_send_ata_cmd().
 * @param timeout_ms Completion timeout for a pending command.
 * @return Local completion status; ATA ERR/DF becomes target failure.
 */
static STATUS_T scsi_finish_intermediate_ata_command(
    UINT32_T lun,
    const SCSI_INTERMEDIATE_SESSION_T *session,
    STATUS_T submission_status,
    INT32_T timeout_ms)
{
    STATUS_T status = submission_status;
    BOOLEAN_T media_is_current;

    if (status == STATUS_SCSI_RESPONSE_PENDING)
    {
        status = ahci_wait_complete(
            PxCI(lun),
            (0x1U << scsi_cmd.pCmdInput->bCmdSlotNum),
            0U,
            timeout_ms);
    }
    if ((status != STATUS_OK) &&
        ((READ32(PxCI(lun)) &
          (0x1U << scsi_cmd.pCmdInput->bCmdSlotNum)) != 0U))
    {
        /* A timed-out polled slot still owns any later completion FIS. Leave
         * D2H/TFES masked, retire the slot, and invalidate the captured epoch
         * before returning a locally completed not-ready result. */
        ahci_recover_local_command(lun, TRUE);
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }

    WRITE_REG32(VIM_REQMASKCLR0, SCSI_CONTROLLER_INTERRUPT_MASK);
    media_is_current = scsi_intermediate_session_is_current(lun, session);
    if (media_is_current && (status == STATUS_OK) &&
        ((READ32(PxTFD(lun)) & PTFD_STS_FAILURE_MASK) != 0U))
    {
        status = STATUS_SCSI_INTERNAL_TARGET_FAILURE;
    }
    WRITE32(PxIS(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS);
    if (media_is_current)
    {
        MODIFY32(PxIE(lun), SCSI_INTERMEDIATE_COMPLETION_STATUS,
                 SCSI_INTERMEDIATE_COMPLETION_STATUS);
    }
    else
    {
        WRITE32(PxIE(lun), 0U);
        status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    WRITE_REG32(VIM_REQMASKSET0, SCSI_AHCI_INTERRUPT_MASK);
    if (media_is_current)
    {
        WRITE_REG32(VIM_REQMASKSET0,
                    SCSI_SATA_RX_ERROR_INTERRUPT_MASK);
    }
    return status;
}

/*****************************************************************************
 * Function: scsi_handle_start_stop_unit_cmd
 *************************************************************************//**
 * This function handles SCSI start/stop unit command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_ERROR when the SCSI command is not supported or no empty command slot is found.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 * @retval STATUS_SCSI_INVALID_ADDRESS_RANGE when address range is invalid.
 * @retval STATUS_SCSI_INVALID_CMD when command parameters are not supported.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 * @retval STATUS_SCSI_LOGICAL_UNIT_NOT_READY when a non-eject operation is
 *         requested without initialized media.
 * @retval STATUS_SCSI_MEDIUM_REMOVAL_PREVENTED when LOEJ is interlocked.
 * @retval STATUS_TIMEOUT when ATA command times out.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_start_stop_unit_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
    SCSI_INTERMEDIATE_SESSION_T intermediate_session;
    STATUS_T status = STATUS_SCSI_INVALID_CMD_FIELD;
    UINT32_T pwr_condition = scsi_cmd.pCmdInput->pCommandBlock[4] >> 4;

#if REMOVABLE_MEDIA_DEVICE
    /* The removable-media contract dispatches all four LOEJ/START forms by
     * those two bits alone.  Ignore the high-nibble power-condition field so
     * it cannot redirect an RDX request into the non-removable ACTIVE, IDLE,
     * or STANDBY translation below. */
    pwr_condition = PWR_CONDITION_START_VALID;
#endif

    DEBUG("-> scsi_handle_start_stop_unit_cmd() - CDB[4] = 0x%x.\n", scsi_cmd.pCmdInput->pCommandBlock[4]);

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    /* LOEJ has its own media-state topology and must be handled before the
     * ordinary ATA-readiness gate.  START=1 reports the current state without
     * moving or reloading media.  START=0 tests PREVENT first, then either
     * consumes the request through page 31h logical-unload policy or queues
     * the physical coordinator.  Queueing with an empty bay is intentional:
     * the coordinator reduces that accepted request to a harmless no-op. */
    if ((pwr_condition == PWR_CONDITION_START_VALID) &&
        ((scsi_cmd.pCmdInput->pCommandBlock[4] & LOEJ_BIT) != 0U))
    {
        if ((scsi_cmd.pCmdInput->pCommandBlock[4] & START_BIT) != 0U)
        {
            return (ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete &&
                    !rdx_hardware_is_logically_unloaded()) ?
                   STATUS_OK : STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
        }

        if (scsi_medium_removal_is_prevented())
        {
            /* This rejection completes locally.  The dedicated status makes
             * scsi_command_handler() install removal-prevented sense and lets
             * BOT/UAS finish without waiting for an ATA callback. */
            return STATUS_SCSI_MEDIUM_REMOVAL_PREVENTED;
        }

#if REMOVABLE_MEDIA_DEVICE
        if (!rdx_manager_apply_host_eject_policy())
        {
            rdx_hardware_request_eject();
        }
        return STATUS_OK;
#else
        if (!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete)
        {
            return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
        }
        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bRemovableMediaDevice)
        {
            /* The generic bridge has no defined ATA media-eject transport. */
            return STATUS_NOT_SUPPORTED;
        }
        return STATUS_SCSI_INVALID_CMD_FIELD;
#endif
    }

    /* Logical unload preserves the ATA-ready flag, so both conditions form
     * the command-visible readiness gate.  Every non-LOEJ power transition
     * returns before allocating an ATA slot while either condition is false. */
    if (!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete ||
        rdx_hardware_is_logically_unloaded())
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }

    // Initialize ATA command.
    ti_memset(&ata_cmd, 0, sizeof(ata_cmd));
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */

    if (pwr_condition == PWR_CONDITION_START_VALID)
    {
        if (scsi_cmd.pCmdInput->pCommandBlock[4] & START_BIT)
        {
            /* LOEJ was completed above, so START here is the ordinary
             * LOEJ=0 media-start verification path. */
            scsi_cmd.bLBA_exp[2] = 0;
            scsi_cmd.bLBA_exp[1] = 0;
            scsi_cmd.bLBA_exp[0] = 0;
            scsi_cmd.bLBA[2] = 0;
            scsi_cmd.bLBA[1] = 0;
            scsi_cmd.bLBA[0] = 0xF;

            // Set count field to 1 per SAT-2 spec.
            scsi_cmd.dXferLength = 1;

            // Build and send only while media remains admitted.
            if (scsi_build_ata_read_verify_sectors_cmd(&ata_cmd))
            {
                status = scsi_send_ata_cmd(&ata_cmd);
            }
            else
            {
                status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
            }
        }
        else  /* START = 0 */
        {
            /* LOEJ returned above, so STOP here is strictly the LOEJ=0
             * ordered FLUSH CACHE then STANDBY IMMEDIATE transaction. */
            // Send ATA FLUSH CACHE and then send ATA STANDBY IMMEDIATE.
            // Ignore IMMED bit because there isn't a good way to complete these commands
            // after returning GOOD status.

            // Build ATA FLUSH CACHE command.
            ata_cmd.fis.command = (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48) ? ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;

            if (!scsi_begin_intermediate_ata_command(
                    scsi_cmd.pCmdInput->bLUN, &intermediate_session))
            {
                return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
            }
            status = scsi_finish_intermediate_ata_command(
                scsi_cmd.pCmdInput->bLUN,
                &intermediate_session,
                scsi_send_ata_cmd(&ata_cmd),
                30000);
            if (status == STATUS_OK)
            {
                // Build ATA STANDBY IMMEDIATE command.
                ata_cmd.fis.command = ATA_CMD_STANDBY_IMMEDIATE;

                // Send ATA command to device.
                status = scsi_send_ata_cmd(&ata_cmd);
            }
        }
    }
    else if (pwr_condition == PWR_CONDITION_ACTIVE)
    {
        // Send ATA IDLE and then send ATA VERIFY SECTORS.
        // Ignore IMMED bit because there isn't a good way to complete these commands
        // after returning GOOD status.

        // Build ATA IDLE command.
        ata_cmd.fis.command = ATA_CMD_IDLE;

        if (!scsi_begin_intermediate_ata_command(
                scsi_cmd.pCmdInput->bLUN, &intermediate_session))
        {
            return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
        }
        status = scsi_finish_intermediate_ata_command(
            scsi_cmd.pCmdInput->bLUN,
            &intermediate_session,
            scsi_send_ata_cmd(&ata_cmd),
            10000);
        if (status == STATUS_OK)
        {
            // Setup an arbitrary LBA address.
            scsi_cmd.bLBA_exp[2] = 0;
            scsi_cmd.bLBA_exp[1] = 0;
            scsi_cmd.bLBA_exp[0] = 0;
            scsi_cmd.bLBA[2] = 0;
            scsi_cmd.bLBA[1] = 0;
            scsi_cmd.bLBA[0] = 0xF;

            // Set count field to 1 per SAT-2 spec.
            scsi_cmd.dXferLength = 1;

            // Build and send only while media remains admitted.
            if (scsi_build_ata_read_verify_sectors_cmd(&ata_cmd))
            {
                status = scsi_send_ata_cmd(&ata_cmd);
            }
            else
            {
                status = STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
            }
        }
    }
    else if (pwr_condition == PWR_CONDITION_IDLE)
    {
        // Send ATA FLUSH CACHE if NO_FLUSH bit is cleared, 
        // then send ATA IDLE, 
        // then send ATA IDLE IMMEDIATE with UNLOAD if supported.
        // Ignore IMMED bit because there isn't a good way to complete these commands
        // after returning GOOD status.

        if (!(scsi_cmd.pCmdInput->pCommandBlock[4] & NO_FLUSH_BIT))
        {
            // Build ATA FLUSH CACHE command.
            ata_cmd.fis.command = (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48) ? ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;

            if (!scsi_begin_intermediate_ata_command(
                    scsi_cmd.pCmdInput->bLUN, &intermediate_session))
            {
                return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
            }
            status = scsi_finish_intermediate_ata_command(
                scsi_cmd.pCmdInput->bLUN,
                &intermediate_session,
                scsi_send_ata_cmd(&ata_cmd),
                30000);
        }
        else
        {
            status = STATUS_OK;
        }

        if (status == STATUS_OK)
        {
            // Check if POWER CONDITION MODIFIER is non-zero and IDLE IMMEDIATE with UNLOAD is supported.
            if ((scsi_cmd.pCmdInput->pCommandBlock[3] & 0x0F) &&
                (ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[84] & ATA_IDLE_IMMEDIATE_SUPPORT_BIT))
            {
                // Build ATA IDLE command.
                ata_cmd.fis.command = ATA_CMD_IDLE;

                if (!scsi_begin_intermediate_ata_command(
                        scsi_cmd.pCmdInput->bLUN, &intermediate_session))
                {
                    return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
                }
                status = scsi_finish_intermediate_ata_command(
                    scsi_cmd.pCmdInput->bLUN,
                    &intermediate_session,
                    scsi_send_ata_cmd(&ata_cmd),
                    10000);
                if (status == STATUS_OK)
                {
                    // Build ATA IDLE IMMEDIATE command for UNLOAD feature.
                    ata_cmd.fis.features = 0x44;
                    ata_cmd.fis.LBA_low = 0x4C;
                    ata_cmd.fis.LBA_mid = 0x4E;
                    ata_cmd.fis.LBA_high = 0x55;
                    ata_cmd.fis.command = ATA_CMD_IDLE_IMMEDIATE;

                    // Send ATA command to device.
                    status = scsi_send_ata_cmd(&ata_cmd);
                }
            }
            else
            {
                // Build ATA IDLE command.
                ata_cmd.fis.command = ATA_CMD_IDLE;

                // Send ATA command to device.
                status = scsi_send_ata_cmd(&ata_cmd);
            }
        }
    }
    else if (pwr_condition == PWR_CONDITION_STANDBY)
    {
        // Flush cache if NO_FLUSH bit is cleared and then send ATA STANDBY.
        // Ignore IMMED bit because there isn't a good way to complete these commands
        // after returning GOOD status.

        if (!(scsi_cmd.pCmdInput->pCommandBlock[4] & NO_FLUSH_BIT))
        {
            // Build ATA FLUSH CACHE command.
            ata_cmd.fis.command = (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48) ? ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;

            if (!scsi_begin_intermediate_ata_command(
                    scsi_cmd.pCmdInput->bLUN, &intermediate_session))
            {
                return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
            }
            status = scsi_finish_intermediate_ata_command(
                scsi_cmd.pCmdInput->bLUN,
                &intermediate_session,
                scsi_send_ata_cmd(&ata_cmd),
                30000);
            if (status == STATUS_OK)
            {
                // Build ATA STANDBY command.
                ata_cmd.fis.command = ATA_CMD_STANDBY;

                // Send ATA command to device.
                status = scsi_send_ata_cmd(&ata_cmd);
            }
        }
        else /* NO_FLUSH = 1 */
        {
            // Build ATA STANDBY command.
            ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

            ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
            ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
            ata_cmd.fis.command = ATA_CMD_STANDBY;

            // Send ATA command to device.
            status = scsi_send_ata_cmd(&ata_cmd);
        }
    }

    return status;
}

#define CODE_SET_BINARY  0x01
#define CODE_SET_ASCII   0x02
#define DESIGNATOR_TYPE_NAA  0x03
#define DESIGNATOR_TYPE_T10  0x01
#define VPD_NAA_DEVICE_ID_PAGE_DATA_LENGTH     16
#define VPD_T10_DEVICE_ID_PAGE_DATA_LENGTH     76

/*****************************************************************************
 * Function: scsi_build_dev_id_vpd_page
 *************************************************************************//**
 * This function builds the device ID vital product data page.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_dev_id_vpd_page(UINT32_T *buff_size)
{
    DEBUG("-> scsi_build_dev_id_vpd_page()\n");
    /* RDX Manager's SCInqDevIdPg parser consumes a single fixed T10
     * designator as adapter vendor/product/serial fields. Returning the
     * ordinary disk's binary NAA WWN here makes its first byte (often 50h,
     * ASCII 'P') appear as the drive manufacturer. */
    *buff_size = rdx_manager_build_drive_id_vpd(
        (UINT8_T *)scsi_resp_buff, scsi_resp_buff_sz);

    return;
}

#define MAX_UNMAP_DESC_SIZE   (2 * 1024)
#define BLOCK_LIMITS_VPD_PAGE_LENGTH  0x3C

/*****************************************************************************
 * Function: scsi_build_block_limits_vpd_page
 *************************************************************************//**
 * This function builds the block limits vital product data page.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_block_limits_vpd_page(UINT32_T *buff_size)
{
    UINT32_T max_transfer_blocks;
    UINT32_T unmap_desc_cnt = 0;

    DEBUG("-> scsi_build_block_limits_vpd_page()\n");

    ti_memset((void*)scsi_resp_buff, 0, (BLOCK_LIMITS_VPD_PAGE_LENGTH + 4));

    //scsi_resp_buff[0] = 0x00; /* Direct access block device connected */
    scsi_resp_buff[1] = VPD_BLOCK_LIMITS_PAGE_CODE; /* Page code */
    //scsi_resp_buff[2] = 0; /* RSVD */
    scsi_resp_buff[3] = BLOCK_LIMITS_VPD_PAGE_LENGTH; /* Page length */

    /* Optimal Xfer length granularity */
    //scsi_resp_buff[6] = 0x00;
    scsi_resp_buff[7] = 0x01;

    max_transfer_blocks = scsi_get_max_rw_blocks(
        scsi_cmd.pCmdInput->bLUN,
        ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize);

    /* Max and optimal transfer lengths share the hardware PRDT byte limit. */
    scsi_resp_buff[8] = (UINT8_T)(max_transfer_blocks >> 24U);
    scsi_resp_buff[9] = (UINT8_T)(max_transfer_blocks >> 16U);
    scsi_resp_buff[10] = (UINT8_T)(max_transfer_blocks >> 8U);
    scsi_resp_buff[11] = (UINT8_T)max_transfer_blocks;
    scsi_resp_buff[12] = (UINT8_T)(max_transfer_blocks >> 24U);
    scsi_resp_buff[13] = (UINT8_T)(max_transfer_blocks >> 16U);
    scsi_resp_buff[14] = (UINT8_T)(max_transfer_blocks >> 8U);
    scsi_resp_buff[15] = (UINT8_T)max_transfer_blocks;

    // Set Max UNMAP LBA count to 0x003FFFC0 x Max number of 512-byte LBA entry blocks per DATA SET MANAGEMENT (DSM) ATA command 
    // to limit the maximum number of LBAs that may be unmapped by an UNMAP command to prevent host from timing out the command.  
    // (0xFFFF x 40) = 0x003FFFC0, 0xFFFF is the LBA range limit for each DATA SET MANAGEMENT (DSM) 
    // ATA commmand entry and 40 is the max number of entries per command in a 512-byte block.
    *((UINT32_T*)&scsi_resp_buff[20]) = BSWAP_32(
        0x003FFFC0U *
        (UINT32_T)MIN(
            ata_dev[scsi_cmd.pCmdInput->bLUN].wDataSetMgmtMaxBlocks,
            0x0400U));

    // Set Max UNMAP block descriptor count.  (0x100 x 16 bytes/descriptor = 4KB)
    // SAT-3 rev02 spec says to set to zero, but we are limiting it to ensure there is enough memory left
    // to convert to ATA command. SBC-3 rev27 spec also says this field must be non-zero if UNMAP command is
    // supported.
    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bTRIMSupport)
    {
        unmap_desc_cnt = (MAX_UNMAP_DESC_SIZE - 8U) >> 4;
    }

    *((UINT32_T*)&scsi_resp_buff[24]) = BSWAP_32(unmap_desc_cnt);

    *buff_size = BLOCK_LIMITS_VPD_PAGE_LENGTH + 4;

    return;
}


#define SERIAL_NUM_VPD_PAGE_LENGTH  20

/*****************************************************************************
 * Function: scsi_build_serial_num_vpd_page
 *************************************************************************//**
 * This function builds the unit serial number vital product data page.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_serial_num_vpd_page(UINT32_T *buff_size)
{
    DEBUG("-> scsi_build_serial_num_vpd_page()\n");

    scsi_resp_buff[0] = 0x00; /* Direct access block device connected */
    scsi_resp_buff[1] = VPD_UNIT_SERIAL_NUMBER_PAGE_CODE; /* Page code */
    scsi_resp_buff[2] = 0; /* RSVD */
    scsi_resp_buff[3] = SERIAL_NUM_VPD_PAGE_LENGTH; /* Page length */

    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete)
    {
        /* Copy Serial Number */
        ti_memcpy((void*)&scsi_resp_buff[4], &ata_dev[scsi_cmd.pCmdInput->bLUN].wSerialNum[0], SERIAL_NUM_VPD_PAGE_LENGTH);
    }
    else
    {
        /* Copy ASCII spaces */
        ti_memset((void*)&scsi_resp_buff[4], ' ', SERIAL_NUM_VPD_PAGE_LENGTH);
    }

    *buff_size = (SERIAL_NUM_VPD_PAGE_LENGTH + 4);

    return;
}

#define VPD_BLOCK_DEVICE_CHAR_PAGE_LENGTH   0x3C

/*****************************************************************************
 * Function: scsi_build_block_dev_char_vpd_page
 *************************************************************************//**
 * This function builds the block device characteristics vital product data page.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_block_dev_char_vpd_page(UINT32_T *buff_size)
{
    DEBUG("-> scsi_build_block_dev_char_vpd_page()\n");

    ti_memset((void*)scsi_resp_buff, 0, (VPD_BLOCK_DEVICE_CHAR_PAGE_LENGTH + 4));

    scsi_resp_buff[0] = 0x00; /* Direct access block device connected */
    scsi_resp_buff[1] = VPD_BLOCK_DEVICE_CHAR_PAGE_CODE; /* Page code */
    scsi_resp_buff[2] = 0x00; /* RSVD */
    scsi_resp_buff[3] =  VPD_BLOCK_DEVICE_CHAR_PAGE_LENGTH; /* Page length */

    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete)
    {
        // Medium Rotation Rate.
        *((UINT16_T*)&scsi_resp_buff[4]) = ata_dev[scsi_cmd.pCmdInput->bLUN].wMediaRotationRate;
        // Nominal Form Factor.
        *((UINT16_T*)&scsi_resp_buff[6]) = ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[168];
    }

    *buff_size = VPD_BLOCK_DEVICE_CHAR_PAGE_LENGTH + 4;

    return;
}


#define LOGICAL_BLOCK_PROVISIONING_VPD_PAGE_LENGTH  4
#define LOGICAL_BLOCK_PROV_LBPU_BIT    0x80
#define LOGICAL_BLOCK_PROV_LBPRZ_BIT   0x04
#define LOGICAL_BLOCK_PROV_ANC_SUP_BIT 0x02

/*****************************************************************************
 * Function: scsi_build_logical_block_provisioning_vpd_page
 *************************************************************************//**
 * This function builds the logical block provisioning vital product data page.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_logical_block_provisioning_vpd_page(UINT32_T *buff_size)
{
    DEBUG("-> scsi_build_logical_block_provisioning_vpd_page()\n");

    ti_memset((void*)scsi_resp_buff, 0, (LOGICAL_BLOCK_PROVISIONING_VPD_PAGE_LENGTH + 4));

    //scsi_resp_buff[0] = 0x00; /* Direct access block device connected */
    scsi_resp_buff[1] = VPD_LOGICAL_BLOCK_PROVISIONING_PAGE_CODE, /* Page code */
    //scsi_resp_buff[2] = 0; /* Page length MSB */
    scsi_resp_buff[3] = LOGICAL_BLOCK_PROVISIONING_VPD_PAGE_LENGTH; /* Page length LSB */
    //scsi_resp_buff[4] = 0; /* Threshold exponent */
    //scsi_resp_buff[5] = 0; /* LBPU, LBPWS, LBPWS10, RSVD[2], LBPRZ, ANC_SUP, DP */
    //scsi_resp_buff[6] = 0; /* Provisioning type */
    //scsi_resp_buff[7] = 0; /* RSVD */

    // Settings per SAT-4 spec.
    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bTRIMSupport)
    {
        scsi_resp_buff[5] = LOGICAL_BLOCK_PROV_LBPU_BIT;

        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDRATSupport)
        {
            scsi_resp_buff[5] |= LOGICAL_BLOCK_PROV_ANC_SUP_BIT;

            if (ata_dev[scsi_cmd.pCmdInput->bLUN].bRZATSupport)
            {
                scsi_resp_buff[5] |= LOGICAL_BLOCK_PROV_LBPRZ_BIT;
            }
        }
    }

    *buff_size = LOGICAL_BLOCK_PROVISIONING_VPD_PAGE_LENGTH + 4;

    return;
}


static const UINT8_T bSAT_VendorID[] = {'T','I','-','D','S','G'};
static const UINT8_T bSAT_ProductID[] = {'T','U','S','B','9','2','6','x'};
static const UINT8_T bSAT_VersionID[] = {'0','1','0','0'};

#define ATA_INFO_VPD_PAGE_LENGTH   0x238
#define TRANSPORT_ID_SATA          0x34

/*****************************************************************************
 * Function: scsi_build_ata_info_vpd_page
 *************************************************************************//**
 * This function builds the ATA Information vital product data page.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_ata_info_vpd_page(UINT32_T *buff_size)
{
    DEBUG("-> scsi_build_ata_info_vpd_page()\n");

    ti_memset((void*)scsi_resp_buff, 0, (ATA_INFO_VPD_PAGE_LENGTH + 4));

    scsi_resp_buff[0] = 0x00; /* Direct access block device connected */
    scsi_resp_buff[1] =  VPD_ATA_INFO_PAGE_CODE; /* Page code */
    *((UINT16_T*)&scsi_resp_buff[2]) = BSWAP_16(ATA_INFO_VPD_PAGE_LENGTH); /* Page length */

    /* SAT Vendor ID - "TI-DSG" per www.t10.org */
    ti_memcpy((void*)&scsi_resp_buff[8], bSAT_VendorID, sizeof(bSAT_VendorID));

    /* SAT Product ID - "TUSB926x" */
    ti_memcpy((void*)&scsi_resp_buff[16], bSAT_ProductID, sizeof(bSAT_ProductID));

    /* SAT Product Revision - "0100" */
    ti_memcpy((void*)&scsi_resp_buff[32], bSAT_VersionID, sizeof(bSAT_VersionID));

    /* ATA device signature */
    scsi_resp_buff[36] = TRANSPORT_ID_SATA;  /* Transport Identifier - SATA */

    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete)
    {
        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceSignature[1] & 0x40)
        {
            scsi_resp_buff[37] = 0x40;  /* Set I bit.  No PM supported. */
        }

        ti_memcpy((void*)&scsi_resp_buff[38], &ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceSignature[2], 12);

        /* Command Code */
        scsi_resp_buff[56] = (ata_dev[scsi_cmd.pCmdInput->bLUN].bPacketDevice) ? 0xA1 : 0xEC;

        /* Copy ATA IDENTIFY DEVICE info */
        ti_memcpy((void*)&scsi_resp_buff[60], ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo, 512);
    }

    *buff_size = ATA_INFO_VPD_PAGE_LENGTH + 4;

    return;
}

#define SPC_DEFINED_DATA_FORMAT  0x02
#define RMB_BIT                  0x80
#define STANDARD_INQUIRY_DATA_LENGTH  64U

/* Stable dock identity exposed instead of the model string reported by the
 * ordinary SATA disk inside the cartridge. */
static const UINT8_T rdx_vendor_id[8] =
{
    'T', 'A', 'N', 'D', 'B', 'E', 'R', 'G'
};
static const UINT8_T rdx_product_id[16] =
{
    'R', 'D', 'X', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
};

/*****************************************************************************
 * Function: scsi_build_std_inquiry_data
 *************************************************************************//**
 * This function builds the response for a standard inquiry.
 *
 * @param[out] buff_size pointer to memory to store buffer size.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_std_inquiry_data(UINT32_T *buff_size)
{
    DEBUG("-> scsi_build_std_inquiry_data()\n");

    ti_memset((void*)scsi_resp_buff, 0, STANDARD_INQUIRY_DATA_LENGTH);

    /* Keep a fixed RDX inquiry shape. Bytes 36-42 are the private extension
     * RDX Manager uses to distinguish this dock from an ordinary SAT bridge.
     * Revision 0001 identifies this OpenRDX build for update-policy checks. */
    scsi_resp_buff[1] = RMB_BIT;
    scsi_resp_buff[2] = 0x06U;
    scsi_resp_buff[3] = SPC_DEFINED_DATA_FORMAT;
    scsi_resp_buff[4] = STANDARD_INQUIRY_DATA_LENGTH - 5U;
    ti_memcpy((void*)&scsi_resp_buff[8], rdx_vendor_id,
              sizeof(rdx_vendor_id));
    ti_memcpy((void*)&scsi_resp_buff[16], rdx_product_id,
              sizeof(rdx_product_id));
    ti_memcpy((void*)&scsi_resp_buff[32], "0001", 4U);
    scsi_resp_buff[36] = 0x38U;
    ti_memcpy((void*)&scsi_resp_buff[37], "RDX", 3U);
    scsi_resp_buff[40] = 0x02U;
    scsi_resp_buff[41] = 0x57U;
    scsi_resp_buff[42] = 0x50U;
    scsi_resp_buff[58] = 0x04U;
    scsi_resp_buff[59] = 0x60U;
    scsi_resp_buff[60] = 0x04U;
    scsi_resp_buff[61] = 0xC0U;
    scsi_resp_buff[62] = 0x17U;
    scsi_resp_buff[63] = 0x30U;

    *buff_size = STANDARD_INQUIRY_DATA_LENGTH;

    return;
}


#define INQUIRY_EVPD_BIT   0x01

/*****************************************************************************
 * Function: scsi_handle_inquiry_cmd
 *************************************************************************//**
 * This function handles a INQUIRY command.
 * ATA information VPD page is not supported although req'd by (SAT-2)
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_inquiry_cmd(void)
{
    UINT16_T alloc_length;
    UINT8_T  page_code;
    UINT32_T buffer_size;
    STATUS_T status = STATUS_OK;

    page_code = scsi_cmd.pCmdInput->pCommandBlock[2];
    alloc_length = (scsi_cmd.pCmdInput->pCommandBlock[3] << 8) | scsi_cmd.pCmdInput->pCommandBlock[4];

    DEBUG("-> scsi_handle_inquiry_cmd() - page_code = 0x%02x.\n", page_code);

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (scsi_cmd.pCmdInput->pCommandBlock[1] & INQUIRY_EVPD_BIT)
    {
        // VPD Page specified by page code.
        switch (page_code)
        {
            case VPD_SUPPORTED_PAGES_PAGE_CODE:
                buffer_size = MIN(sizeof(inquiry_vpd_page00_data), scsi_resp_buff_sz);
                // Copy data into response buffer.                                  
                ti_memcpy((void*)scsi_resp_buff, inquiry_vpd_page00_data, buffer_size);
                break;

            case VPD_BLOCK_LIMITS_PAGE_CODE:
                scsi_build_block_limits_vpd_page(&buffer_size);
                break;

            case VPD_LOGICAL_BLOCK_PROVISIONING_PAGE_CODE:
                scsi_build_logical_block_provisioning_vpd_page(&buffer_size);

                break;

            case VPD_BLOCK_DEVICE_CHAR_PAGE_CODE:
                scsi_build_block_dev_char_vpd_page(&buffer_size);
                break;

            case VPD_DEVICE_ID_PAGE_CODE:
                scsi_build_dev_id_vpd_page(&buffer_size);
                break;

            case VPD_UNIT_SERIAL_NUMBER_PAGE_CODE:
                scsi_build_serial_num_vpd_page(&buffer_size);
                break;

            case VPD_ATA_INFO_PAGE_CODE:
                scsi_build_ata_info_vpd_page(&buffer_size);
                break;

            case VPD_RDX_MEDIA_ID_PAGE_CODE:
                buffer_size = rdx_manager_build_media_id_vpd(
                    (UINT8_T *)scsi_resp_buff, scsi_resp_buff_sz,
                    scsi_cmd.pCmdInput->bLUN);
                if (buffer_size == 0U)
                {
                    status = STATUS_SCSI_INTERNAL_TARGET_FAILURE;
                }
                break;

            case VPD_RDX_MEDIA_IDENTIFY_PAGE_CODE:
                buffer_size = rdx_manager_build_media_identify_vpd(
                    (UINT8_T *)scsi_resp_buff, scsi_resp_buff_sz,
                    scsi_cmd.pCmdInput->bLUN);
                if (buffer_size == 0U)
                {
                    status = STATUS_SCSI_INTERNAL_TARGET_FAILURE;
                }
                break;

            default:
                CRIT("@Warning: scsi_handle_inquiry_cmd() - VPD page 0x%02x not implemented.\n", page_code);
                // Requested page is not implemented.
                status = STATUS_SCSI_INVALID_CMD_FIELD;
                break;
        }
    }
    else /* Standard Inquiry */
    {
        if (page_code == 0)
        {
            scsi_build_std_inquiry_data(&buffer_size);
        }
        else
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

    } /* End Standard Inquiry */

    if (status == STATUS_OK)
    {
        // Set data pointer and byte count.
        scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
        scsi_cmd.pCmdInput->dDataByteCnt = MIN(alloc_length, buffer_size);
        status = STATUS_SCSI_RESPONSE_READY;
    }

    return status;
}



#define READ_CAPACITY10_DATA_LENGTH   8  /* bytes */
#define READ_CAPACITY16_DATA_LENGTH   32  /* bytes */

#define READ_CAPACITY_PMI_BIT 0x1
#define READ_CAPACITY_LBPME_BIT 0x80
#define READ_CAPACITY_LBPRZ_BIT 0x40

/*****************************************************************************
 * Function: scsi_handle_read_capacity_cmd
 *************************************************************************//**
 * This function handles 10 & 16-byte READ CAPACITY commands.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when successful and there is data to be sent.
 * @retval STATUS_OK when allocation length field is zero.
 * @retval STATUS_SCSI_INVALID_CMD when command is not supported.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_read_capacity_cmd(void)
{
    UINT64_T num_blocks;
    INT16_T  lowest_aligned_lba;
    UINT32_T alloc_length;
    UINT32_T buffer_size = 0;
    STATUS_T status = STATUS_SCSI_RESPONSE_READY;

    DEBUG("-> scsi_handle_read_capacity_cmd()\n");

    if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_READ_CAPACITY10)
    {
        // Check control byte.
        if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        // Check PMI bit.
        if (scsi_cmd.pCmdInput->pCommandBlock[8] & READ_CAPACITY_PMI_BIT)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        // Check for non-zero LBA.
        if (scsi_cmd.pCmdInput->pCommandBlock[2] | scsi_cmd.pCmdInput->pCommandBlock[3] |
            scsi_cmd.pCmdInput->pCommandBlock[4] | scsi_cmd.pCmdInput->pCommandBlock[5])
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        if (status == STATUS_SCSI_RESPONSE_READY)
        {
            num_blocks = MIN((ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA - 1), 0xFFFFFFFF);

            /* Last Logical Block */
            scsi_resp_buff[0] = (num_blocks >> 24) & 0xFF;
            scsi_resp_buff[1] = (num_blocks >> 16) & 0xFF;
            scsi_resp_buff[2] = (num_blocks >> 8) & 0xFF;
            scsi_resp_buff[3] = num_blocks & 0xFF;

            /* Block Length */
            scsi_resp_buff[4] = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 24) & 0xFF;
            scsi_resp_buff[5] = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 16) & 0xFF;
            scsi_resp_buff[6] = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 8) & 0xFF;
            scsi_resp_buff[7] = ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize & 0xFF;

            buffer_size = READ_CAPACITY10_DATA_LENGTH;
        }
    }
    else if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_READ_CAPACITY16)
    {
        // Check control byte.
        if (scsi_cmd.pCmdInput->pCommandBlock[15] & NACA_CONTROL_BIT)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        // Check service action.
        if (scsi_cmd.pCmdInput->pCommandBlock[1] != 0x10)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        if (status == STATUS_SCSI_RESPONSE_READY)
        {
            alloc_length = ((scsi_cmd.pCmdInput->pCommandBlock[10] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[11] << 16) |
                            (scsi_cmd.pCmdInput->pCommandBlock[12] << 8) | scsi_cmd.pCmdInput->pCommandBlock[13]);

            if (alloc_length == 0)
            {
                status = STATUS_OK;
            }
            else
            {
                num_blocks = ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA - 1;

                /* Last Logical Block */
                scsi_resp_buff[0] = (num_blocks >> 56) & 0xFF;
                scsi_resp_buff[1] = (num_blocks >> 48) & 0xFF;
                scsi_resp_buff[2] = (num_blocks >> 40) & 0xFF;
                scsi_resp_buff[3] = (num_blocks >> 32) & 0xFF;
                scsi_resp_buff[4] = (num_blocks >> 24) & 0xFF;
                scsi_resp_buff[5] = (num_blocks >> 16) & 0xFF;
                scsi_resp_buff[6] = (num_blocks >> 8) & 0xFF;
                scsi_resp_buff[7] = num_blocks & 0xFF;

                /* Block Length */
                scsi_resp_buff[8]  = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 24) & 0xFF;
                scsi_resp_buff[9]  = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 16) & 0xFF;
                scsi_resp_buff[10] = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 8) & 0xFF;
                scsi_resp_buff[11] = ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize & 0xFF;

                /* Protection */
                scsi_resp_buff[12] = 0x00;  /* Type 0 */

                /* Logical blocks per physical block exponent */
                scsi_resp_buff[13] = ata_dev[scsi_cmd.pCmdInput->bLUN].bPhysicalSectorExp;

                /* Lowest Aligned LBA */
                lowest_aligned_lba = ata_dev[scsi_cmd.pCmdInput->bLUN].dLogicalSectorsPerPhysicalSector - ata_dev[scsi_cmd.pCmdInput->bLUN].wLogicalSectorOffset;

                /* Perform a psuedo-modulus to calculate Lowest Aligned LBA */
                if ((lowest_aligned_lba == ata_dev[scsi_cmd.pCmdInput->bLUN].dLogicalSectorsPerPhysicalSector) ||
                    (lowest_aligned_lba < 0))
                {
                    lowest_aligned_lba = 0;
                }

                scsi_resp_buff[14] = (lowest_aligned_lba >> 8) & 0x3F;
                scsi_resp_buff[15] = lowest_aligned_lba & 0xFF;

                if (ata_dev[scsi_cmd.pCmdInput->bLUN].bTRIMSupport && ata_dev[scsi_cmd.pCmdInput->bLUN].bDRATSupport)
                {
                    /* Logical block provisioning management enabled */
                    scsi_resp_buff[14] |= READ_CAPACITY_LBPME_BIT;

                    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bRZATSupport)
                    {
                        scsi_resp_buff[14] |= READ_CAPACITY_LBPRZ_BIT;
                    }
                }

                // Last 16 bytes are reserved.
                ti_memset((void*)&scsi_resp_buff[16], 0, 16);

                buffer_size = MIN(alloc_length, READ_CAPACITY16_DATA_LENGTH);
            }
        }
    }
    else
    {
        status = STATUS_SCSI_INVALID_CMD;
    }

    // Set data pointer and byte count.
    scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
    scsi_cmd.pCmdInput->dDataByteCnt = buffer_size;

    return status;
}



#define READ_FORMAT_CAPACITY_DATA_LENGTH    20  /* bytes */

/*****************************************************************************
 * Function: scsi_handle_read_format_capacities_cmd
 *************************************************************************//**
 * This function handles a READ FORMAT CAPACITIES command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when successful.
 * @retval STATUS_OK if allocation length is zero.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_read_format_capacities_cmd(void)
{
    UINT16_T alloc_length;
    UINT32_T reported_blocks;

    DEBUG("-> scsi_handle_read_format_capacities_cmd()\n");

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    alloc_length = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) | scsi_cmd.pCmdInput->pCommandBlock[8];

    if (alloc_length == 0)
    {
        /* This is not an error per MMC-5 pg 469 */
        return STATUS_OK;
    }

    // Capacity list header.
    scsi_resp_buff[0] = 0x00; /* RSVD */
    scsi_resp_buff[1] = 0x00; /* RSVD */
    scsi_resp_buff[2] = 0x00; /* RSVD */
    scsi_resp_buff[3] = 0x10; /* Capacity List Length (must be a multiple of 8) */

    /* Current/Max capacity descriptor */
    // Populate Number of Blocks without wrapping a capacity above 32 bits.
    reported_blocks =
        ((ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA >> 32) != 0U) ?
        0xFFFFFFFFU :
        (UINT32_T)ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA;
    scsi_resp_buff[4] = (reported_blocks >> 24) & 0xFF;
    scsi_resp_buff[5] = (reported_blocks >> 16) & 0xFF;
    scsi_resp_buff[6] = (reported_blocks >> 8) & 0xFF;
    scsi_resp_buff[7] = reported_blocks & 0xFF;

    scsi_resp_buff[8] = 0x02;  /* Descriptor Type: Formatted Media */

    // Populate Block Size.  MMC-5 says 2048 for all devices execept BD-R.
    scsi_resp_buff[9] = 0x00;
    scsi_resp_buff[10] = (ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 8) & 0xFF;
    scsi_resp_buff[11] = ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize & 0xFF;

    /* Formattable capacity descriptor 
     * (same as Current/Max capacity descriptor except descriptor type field is reserved. */
    ti_memcpy((void*)&scsi_resp_buff[12], (void*)&scsi_resp_buff[4], 8);
    scsi_resp_buff[16] = 0x00; /* RSVD */

    // Set data pointer and byte count.
    scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
    scsi_cmd.pCmdInput->dDataByteCnt = MIN(alloc_length, READ_FORMAT_CAPACITY_DATA_LENGTH);

    return STATUS_SCSI_RESPONSE_READY;
}

#define FIXED_FORMAT_SENSE_DATA_ADDITIONAL_LENGTH  0x0A  /* bytes */
#define SENSE_DATA_HEADER_LENGTH   8  /* bytes */

/*****************************************************************************
 * Function: scsi_copy_sense_data
 *************************************************************************//**
 * This function copies sense data into the specified buffer using either
 * descriptor format or fixed format.
 *
 * @param[in] dst pointer to the memory destination where sense data is to be copied.
 * @param[in] descriptor_format flag to indicate whether descriptor format or
 * fixed format sense data should be copied.
 *
 * @return The total length of the sense data in bytes.
 *
 ******************************************************************************
 */

UINT32_T scsi_copy_sense_data(void *dst, BOOLEAN_T descriptor_format)
{
    UINT8_T *sense_buff = (UINT8_T*)dst;

    if (descriptor_format)
    {
        // Descriptor format sense data request.
        sense_buff[0] = DESCRIPTOR_FORMAT_SENSE_DATA_RESPONSE_CODE;
        sense_buff[1] = fixed_format_sense_data[2] & 0xF; /* Sense Key */
        sense_buff[2] = fixed_format_sense_data[12]; /* ASC */
        sense_buff[3] = fixed_format_sense_data[13]; /* ASCQ */
        sense_buff[4] = 0x00;   /* RSVD */
        sense_buff[5] = 0x00;   /* RSVD */
        sense_buff[6] = 0x00;   /* RSVD */

        if ((fixed_format_sense_data[12] == ATA_PASS_THROUGH_INFO_AVAIL) &&
            (fixed_format_sense_data[13] == ASCQ_ATA_PASS_THROUGH_INFO_AVAIL))
        {
            sense_buff[7] = sizeof(ata_return_descriptor_data);   /* Additional sense length */

            // Copy ATA return descriptor data.
            ti_memcpy((void*)&sense_buff[8], ata_return_descriptor_data, sizeof(ata_return_descriptor_data));
        }
        else
        {
            sense_buff[7] = 0;   /* Additional sense length */
        }
    }
    else /* Fixed format sense data request */
    {
        ti_memcpy((void*)&sense_buff[0], fixed_format_sense_data, sizeof(fixed_format_sense_data));
        sense_buff[7] = FIXED_FORMAT_SENSE_DATA_ADDITIONAL_LENGTH;   /* Additional sense length */
    }

    DEBUG("-> scsi_copy_sense_data() - %u bytes.\n", (sense_buff[7] + SENSE_DATA_HEADER_LENGTH));

    TI_PRELIM_ASIC_DPRAM_ACCESS_BUG();

    return(sense_buff[7] + SENSE_DATA_HEADER_LENGTH);
}


#define DESC_BIT 0x01  /* indicates request for descriptor based sense info */

/*****************************************************************************
 * Function: scsi_handle_request_sense_cmd
 *************************************************************************//**
 * This function handles a REQUEST SENSE command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when successful.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_request_sense_cmd(void)
{
    UINT8_T alloc_length = scsi_cmd.pCmdInput->pCommandBlock[4];
    UINT32_T sense_length;

    DEBUG("-> scsi_handle_request_sense_cmd()\n");

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    // Verify no reserved fields are set.
    if ((scsi_cmd.pCmdInput->pCommandBlock[1] & ~DESC_BIT) || scsi_cmd.pCmdInput->pCommandBlock[2] || scsi_cmd.pCmdInput->pCommandBlock[3])
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    // Copy sense data into response buffer.
    sense_length = scsi_copy_sense_data((void*)scsi_resp_buff, (scsi_cmd.pCmdInput->pCommandBlock[1] & DESC_BIT));

    // Set data pointer and byte count.
    scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
    scsi_cmd.pCmdInput->dDataByteCnt = MIN(alloc_length, sense_length);

    return STATUS_SCSI_RESPONSE_READY;
}

/*****************************************************************************
 * Function: scsi_handle_log_sense_cmd
 *************************************************************************//**
 * Build the vendor LOG SENSE pages consumed during RDX Manager discovery.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when the requested page is supported.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD for an unsupported or malformed page.
 *
 *****************************************************************************
 */
inline STATUS_T scsi_handle_log_sense_cmd(void)
{
    UINT8_T *cdb = scsi_cmd.pCmdInput->pCommandBlock;
    UINT8_T page_code = cdb[2] & 0x3FU;
    UINT16_T alloc_length = ((UINT16_T)cdb[7] << 8U) | cdb[8];
    UINT32_T response_length;

    if ((cdb[1] != 0U) || (cdb[3] != 0U) ||
        (cdb[4] != 0U) || (cdb[5] != 0U) ||
        (cdb[6] != 0U) || (cdb[9] & NACA_CONTROL_BIT))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    response_length = rdx_manager_build_log_sense(
        (UINT8_T *)scsi_resp_buff, scsi_resp_buff_sz,
        scsi_cmd.pCmdInput->bLUN, page_code);
    if (response_length == 0U)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    scsi_cmd.pCmdInput->pData = (void *)scsi_resp_buff;
    scsi_cmd.pCmdInput->dDataByteCnt = MIN((UINT32_T)alloc_length,
                                           response_length);
    return STATUS_SCSI_RESPONSE_READY;
}

/*****************************************************************************
 * Function: scsi_handle_write_buffer_cmd
 *************************************************************************//**
 * Pass an RDX Manager firmware download chunk to the guarded update receiver.
 *
 * @retval STATUS_OK when the chunk is accepted.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD for an invalid sequence or image.
 *
 *****************************************************************************
 */
inline STATUS_T scsi_handle_write_buffer_cmd(void)
{
    const UINT8_T *payload = NULL;

    if (scsi_cmd.pCmdInput->dDataXferLength != 0U)
    {
        payload = (const UINT8_T *)datapath_ram->normal_data_buffer;
    }
    return rdx_manager_handle_write_buffer(
        scsi_cmd.pCmdInput->pCommandBlock,
        scsi_cmd.pCmdInput->dDataXferLength, payload);
}


#define DBD_BIT    0x08
#define DPOFUA_BIT 0x10 /* DPO and FUA supported - block device specific param */
#define WP_BIT     0x80 /* Write Protect - block device specific param  */
#define PAGE_CODE_HEADER_BLOCK_DESC 0x00
#define	PAGE_CODE_CACHING_MODE	0x08
#define PAGE_CODE_ALL_PAGES		0x3F

#define CACHING_MODE_PAGE_LENGTH 20
#define WCE_BIT 0x04
#define DRA_BIT 0x20

#define PC_CHANGEABLE 0x01

/*****************************************************************************
 * Function: scsi_handle_mode_sense_cmd
 *************************************************************************//**
 * This function handles a MODE SENSE command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_READY when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_mode_sense_cmd(void)
{
    STATUS_T status = STATUS_OK;
    UINT32_T alloc_len;
    UINT8_T index;
    UINT8_T page_code;
    UINT8_T LLBAA = 0;
    UINT8_T data_length;
    UINT8_T page_ctrl;
    UINT8_T *mode_sense_data = (UINT8_T*)scsi_resp_buff;

    DEBUG("-> scsi_handle_mode_sense_cmd() - %u.\n", (scsi_cmd.pCmdInput->pCommandBlock[0] >> 4) + 5);

    if (scsi_cmd.pCmdInput->pCommandBlock[1] & ~DBD_BIT)
    {
        // Reserved fields are set illegally.
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    page_ctrl = scsi_cmd.pCmdInput->pCommandBlock[2] >> 6;
    page_code = scsi_cmd.pCmdInput->pCommandBlock[2] & 0x3F;

    if ((page_code == PAGE_CODE_HEADER_BLOCK_DESC) ||
        (page_code == RDX_MODE_PAGE_DRIVE_CONTROL) ||
        (page_code == RDX_MODE_PAGE_VENDOR_SHORT) ||
        (page_code == RDX_MODE_PAGE_OPERATION))
    {
        if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_MODE_SENSE6)
        {
            if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
            {
                return STATUS_SCSI_INVALID_CMD_FIELD;
            }
            alloc_len = scsi_cmd.pCmdInput->pCommandBlock[4];
            ti_memset(mode_sense_data, 0, 4U);
            mode_sense_data[2] =
                rdx_hardware_is_write_protected() ? WP_BIT : 0U;
            index = 4U;
            if (!(scsi_cmd.pCmdInput->pCommandBlock[1] & DBD_BIT))
            {
                mode_sense_data[3] = 8U;
                ti_memset(&mode_sense_data[index], 0, 8U);
                mode_sense_data[index + 5U] =
                    (UINT8_T)(ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 16U);
                mode_sense_data[index + 6U] =
                    (UINT8_T)(ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 8U);
                mode_sense_data[index + 7U] =
                    (UINT8_T)ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize;
                index += 8U;
            }
        }
        else
        {
            if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
            {
                return STATUS_SCSI_INVALID_CMD_FIELD;
            }
            alloc_len = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) |
                        scsi_cmd.pCmdInput->pCommandBlock[8];
            ti_memset(mode_sense_data, 0, 8U);
            mode_sense_data[3] =
                rdx_hardware_is_write_protected() ? WP_BIT : 0U;
            mode_sense_data[4] = (scsi_cmd.pCmdInput->pCommandBlock[1] &
                                  0x10U) >> 4U;
            index = 8U;
            if (!(scsi_cmd.pCmdInput->pCommandBlock[1] & DBD_BIT))
            {
                mode_sense_data[7] = 8U;
                ti_memset(&mode_sense_data[index], 0, 8U);
                mode_sense_data[index + 5U] =
                    (UINT8_T)(ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 16U);
                mode_sense_data[index + 6U] =
                    (UINT8_T)(ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize >> 8U);
                mode_sense_data[index + 7U] =
                    (UINT8_T)ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize;
                index += 8U;
            }
        }

        if (page_code == PAGE_CODE_HEADER_BLOCK_DESC)
        {
            /* SCModeBlockDesc asks for page zero specifically. Return only
             * the header and optional block descriptor for this request;
             * rejecting it leaves Manager to parse stale vendor-page bytes as
             * the logical block size. */
            data_length = index;
        }
        else
        {
            data_length = (UINT8_T)(index + rdx_manager_build_mode_page(
                &mode_sense_data[index], scsi_resp_buff_sz - index,
                page_code, page_ctrl));
            if (data_length == index)
            {
                status = STATUS_SCSI_INVALID_CMD_FIELD;
            }
        }
        if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_MODE_SENSE6)
        {
            mode_sense_data[0] = data_length - 1U;
        }
        else
        {
            mode_sense_data[0] = 0U;
            mode_sense_data[1] = data_length - 2U;
        }
    }
    else if ((page_code == PAGE_CODE_ALL_PAGES) || (page_code == PAGE_CODE_CACHING_MODE))
    {
        // Refresh ATA device info.
        ahci_identify_device(scsi_cmd.pCmdInput->bLUN);

        if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_MODE_SENSE6)
        {
            // Check control byte.
            if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
            {
                status = STATUS_SCSI_INVALID_CMD_FIELD;
            }

            alloc_len = scsi_cmd.pCmdInput->pCommandBlock[4];

            // Program mode parameter header.
            mode_sense_data[0] = CACHING_MODE_PAGE_LENGTH + 3;   /* Mode data length */
            mode_sense_data[1] = 0x00;   /* Medium Type (SBC-3) */
            mode_sense_data[2] =
                (ata_dev[scsi_cmd.pCmdInput->bLUN].bFUA ? DPOFUA_BIT : 0U) |
                (rdx_hardware_is_write_protected() ? WP_BIT : 0U);   /* Device specific param (SBC-3) */
            mode_sense_data[3] = 0x00;   /* Block descriptor length */

            index = 4;
        }
        else /* MODE SENSE 10 */
        {
            // Check control byte.
            if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
            {
                status = STATUS_SCSI_INVALID_CMD_FIELD;
            }

            // Get long LBA bit.
            LLBAA = (scsi_cmd.pCmdInput->pCommandBlock[1] & 0x10) >> 4;

            alloc_len = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) | scsi_cmd.pCmdInput->pCommandBlock[8];

            // Program mode parameter header
            mode_sense_data[0] = 0x00;   /* Mode data length */
            mode_sense_data[1] = CACHING_MODE_PAGE_LENGTH + 6;   /* Mode data length */
            mode_sense_data[2] = 0x00;   /* Medium Type (SBC-3) */
            mode_sense_data[3] =
                (ata_dev[scsi_cmd.pCmdInput->bLUN].bFUA ? DPOFUA_BIT : 0U) |
                (rdx_hardware_is_write_protected() ? WP_BIT : 0U);   /* Device specific param (SBC-3) */
            mode_sense_data[4] = LLBAA;  /* LONG LBA */
            mode_sense_data[5] = 0x00;   /* RSVD */
            mode_sense_data[6] = 0x00;   /* Block descriptor length */
            mode_sense_data[7] = 0x00;   /* Block descriptor length */

            index = 8;
        }

        // Populate Caching Mode page.
        if (status == STATUS_OK)
        {
            ti_memset(&mode_sense_data[index], 0, CACHING_MODE_PAGE_LENGTH);

            mode_sense_data[index + 0]  = PAGE_CODE_CACHING_MODE;   /* Mode Page */
            mode_sense_data[index + 1]  = CACHING_MODE_PAGE_LENGTH - 2; /* Page length */

            if (page_ctrl == PC_CHANGEABLE)
            {
#if ENABLE_WRITE_CACHE_ENABLE_CHANGEABLE
                mode_sense_data[index + 2]  = (ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[82] & 0x20) ? WCE_BIT : 0;
#else
                mode_sense_data[index + 2]  = 0;
#endif
                mode_sense_data[index + 12] = (ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[82] & 0x40) ? DRA_BIT : 0;
            }
            else /* Current values */
            {
                mode_sense_data[index + 2]  = (ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[85] & 0x20) ? WCE_BIT : 0;
                mode_sense_data[index + 12] = (ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[85] & 0x40) ? 0 : DRA_BIT;
            }

            data_length = index + CACHING_MODE_PAGE_LENGTH;
        }
    }
    else
    {
        CRIT("@Warning: scsi_handle_mode_sense_cmd() - page code 0x%02x not supported.\n", page_code);
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (status == STATUS_OK)
    {
        // Set data pointer and byte count.
        scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
        scsi_cmd.pCmdInput->dDataByteCnt = MIN(data_length, alloc_len);

        status = STATUS_SCSI_RESPONSE_READY;
    }

    return status;
}

#define PF_BIT 0x10
#define SP_BIT 0x01
#define PAGE_CODE_MASK 0x3F

#define SET_FEATURES_ENABLE_WRITE_CACHE  0x02
#define SET_FEATURES_DISABLE_WRITE_CACHE 0x82
#define SET_FEATURES_ENABLE_READ_LOOK_AHEAD  0xAA
#define SET_FEATURES_DISABLE_READ_LOOK_AHEAD 0x55

/*****************************************************************************
 * Function: scsi_handle_mode_select_cmd
 *************************************************************************//**
 * This function handles a MODE SELECT command.
 *
 * @param None.
 *
 * @retval STATUS_OK or STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 * @retval STATUS_TIMEOUT when ATA command times out.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_mode_select_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
#if ENABLE_WRITE_CACHE_ENABLE_CHANGEABLE
    SCSI_INTERMEDIATE_SESSION_T intermediate_session;
#endif
    STATUS_T status;
    UINT8_T ctrl_byte;
    UINT8_T page_len;
    INT32_T param_list_len;
    UINT32_T block_desc_len;
    UINT32_T header_len;
    UINT8_T *mode_param_list = (UINT8_T*)datapath_ram->normal_data_buffer;
    UINT8_T *mode_page;

    DEBUG("-> scsi_handle_mode_select_cmd() - CDB[0] = 0x%02x.\n", scsi_cmd.pCmdInput->pCommandBlock[0]);

    if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_MODE_SELECT6)
    {
        param_list_len = scsi_cmd.pCmdInput->pCommandBlock[4];
        ctrl_byte = scsi_cmd.pCmdInput->pCommandBlock[5];
    }
    else /* SCSI_MODE_SELECT10 */
    {
        param_list_len = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) | scsi_cmd.pCmdInput->pCommandBlock[8];
        ctrl_byte = scsi_cmd.pCmdInput->pCommandBlock[9];
    }

    INFO("param_list_len = %u\n", param_list_len);

    status = rdx_manager_handle_mode_select(
        scsi_cmd.pCmdInput->pCommandBlock, mode_param_list,
        (UINT32_T)param_list_len);
    if (status != STATUS_NOT_SUPPORTED)
    {
        return status;
    }

    // Check control byte.
    if (ctrl_byte & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (scsi_cmd.pCmdInput->pCommandBlock[1] & SP_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (scsi_cmd.pCmdInput->pCommandBlock[1] & PF_BIT)
    {
        status = STATUS_OK;

        if (param_list_len)
        {
            if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_MODE_SELECT6)
            {
                block_desc_len = mode_param_list[3]; 
                header_len = 4;
            }
            else /* SCSI_MODE_SELECT10 */
            {
                block_desc_len = (mode_param_list[6] << 8) | mode_param_list[7]; 
                header_len = 8;
            }

            INFO("block_desc_len = %u.\n", block_desc_len);

            mode_page = mode_param_list + block_desc_len + header_len;

            param_list_len -= (block_desc_len + header_len);

            while (param_list_len > 0)
            {
                page_len = mode_page[1];
                INFO("page_code = 0x%x.\n", mode_page[0] & PAGE_CODE_MASK);

                switch (mode_page[0] & PAGE_CODE_MASK)
                {
                    case PAGE_CODE_CACHING_MODE:
                        ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

                        // Build ATA Set feature for read look-ahead. 
                        ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
                        ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
                        ata_cmd.fis.command = ATA_CMD_SET_FEATURES;
                        ata_cmd.fis.features = (mode_page[12] & DRA_BIT) ? SET_FEATURES_DISABLE_READ_LOOK_AHEAD : SET_FEATURES_ENABLE_READ_LOOK_AHEAD;

#if ENABLE_WRITE_CACHE_ENABLE_CHANGEABLE
                        if (!scsi_begin_intermediate_ata_command(
                                scsi_cmd.pCmdInput->bLUN,
                                &intermediate_session))
                        {
                            return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
                        }
#endif

                        // Send ATA command to device.
                        status = scsi_send_ata_cmd(&ata_cmd);

#if ENABLE_WRITE_CACHE_ENABLE_CHANGEABLE
                        status = scsi_finish_intermediate_ata_command(
                            scsi_cmd.pCmdInput->bLUN,
                            &intermediate_session,
                            status,
                            1000);
                        if (status == STATUS_OK)
                        {
                            // Build ATA Set feature for write cache.
                            ata_cmd.fis.features =
                                (mode_page[2] & WCE_BIT) ?
                                SET_FEATURES_ENABLE_WRITE_CACHE :
                                SET_FEATURES_DISABLE_WRITE_CACHE;

                            if (!(mode_page[2] & WCE_BIT))
                            {
                                CRIT("@Warning: Write cache has been disabled!\n");
                            }

                            // Send ATA command to device.
                            status = scsi_send_ata_cmd(&ata_cmd);
                        }
#endif
                        break;

                    default:
                        break;
                }

                param_list_len -= (page_len + 2);
            }
        }
    }
    else
    {
        // Set status according to SAT-3 spec.
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    return status;
}


/*****************************************************************************
 * Function: scsi_handle_test_unit_ready_cmd
 *************************************************************************//**
 * This function handles a TEST UNIT READY command and sends an
 * ATA check power mode command to the device.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_SCSI_MEDIUM_CHANGE when removeable media has changed (if REMOVABLE_MEDIA_DEVICE macro is enabled).
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 * @retval STATUS_OK if allocation length is zero.
 * @retval STATUS_ERROR when no empty command slot is found.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_test_unit_ready_cmd(void)
{
    ATA_COMMAND_T ata_cmd;

    DEBUG("-> scsi_handle_test_unit_ready_cmd()\n");

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

#if REMOVABLE_MEDIA_DEVICE
    if (ata_dev[scsi_cmd.pCmdInput->bLUN].bMediumChanged)
    {
        // Clear medium change flag.
        ata_dev[scsi_cmd.pCmdInput->bLUN].bMediumChanged = FALSE;
        return STATUS_SCSI_MEDIUM_CHANGE;
    }
#endif

    // Build ATA command.
    ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
    ata_cmd.fis.command = ATA_CMD_CHECK_POWER_MODE;

    // Send ATA command to device.
    return scsi_send_ata_cmd(&ata_cmd);
}


#define ATA_PASS_THROUGH_T_LENGTH_MASK  0x03
#define ATA_PASS_THROUGH_T_DIR_BIT      0x08

#define FORMAT_UNIT_FMTDATA_BIT         0x10

/*****************************************************************************
 * Function: scsi_get_data_direction
 *************************************************************************//**
 * This function returns the data direction for a SCSI command.
 *
 * @param[in] cdb pointer to SCSI command descriptor block.
 * @param[out] direction pointer to memory to store data direction (ENDPT_DIRECTION_OUT or ENDPT_DIRECTION_IN).
 *
 * @retval STATUS_OK when there is a data direction reported.
 * @retval STATUS_NO_DATA when there is no data or direction.
 * @retval STATUS_ERROR when the SCSI command is not supported or direction pointer is NULL.
 *
 ******************************************************************************
 */

STATUS_T scsi_get_data_direction(const UINT8_T *cdb, ENDPT_DIR_T *direction)
{
    STATUS_T status = STATUS_OK;

    if (direction == NULL)
    {
        return STATUS_ERROR;
    }

    switch (cdb[0])
    {
        // Most frequently used commands (Read & Write) first.
        case SCSI_READ6:
        case SCSI_READ10:
        case SCSI_READ12:
        case SCSI_READ16:
            *direction = ENDPT_DIRECTION_IN;
            break;

        case SCSI_WRITE6:
        case SCSI_WRITE10:
        case SCSI_WRITE12:
        case SCSI_WRITE16:
            *direction = ENDPT_DIRECTION_OUT;
            break;

        case SCSI_READ_CAPACITY16:
            // Check alloc length.
            if (cdb[10] | cdb[11] | cdb[12] | cdb[13])
            {
                *direction = ENDPT_DIRECTION_IN;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        case SCSI_INQUIRY:
        case SCSI_READ_FORMAT_CAPACITIES:
        case SCSI_MODE_SENSE6:
        case SCSI_MODE_SENSE10:
        case SCSI_LOG_SENSE:
        case SCSI_READ_CAPACITY10:
        case SCSI_REQUEST_SENSE:
        case SCSI_REPORT_LUNS:
        case SCSI_TI_ONE_TOUCH_BACKUP_QUERY:
        case SCSI_TI_GET_FW_VERSION:
        case SCSI_TI_GET_PID:
        case SCSI_TI_GET_USB_SPEED:
            *direction = ENDPT_DIRECTION_IN;
            break;

        case SCSI_WRITE_BUFFER:
            if (cdb[6] | cdb[7] | cdb[8])
            {
                *direction = ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        case SCSI_TEST_UNIT_READY:
        case SCSI_VERIFY10:
        case SCSI_VERIFY12:
        case SCSI_VERIFY16:
        case SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL:
        case SCSI_START_STOP_UNIT:
        case SCSI_SYNCHRONIZE_CACHE10:
        case SCSI_SYNCHRONIZE_CACHE16:
        case SCSI_TI_FLASH_UNLOCK:
        case SCSI_TI_FLASH_ERASE:
        case SCSI_TI_DEVICE_RESET:
            // No data stage.
            status = STATUS_NO_DATA;
            break;

        case ATA_PASS_THROUGH12:
        case ATA_PASS_THROUGH16:
            if (cdb[2] & ATA_PASS_THROUGH_T_LENGTH_MASK)
            {
                *direction = (cdb[2] & ATA_PASS_THROUGH_T_DIR_BIT) ? ENDPT_DIRECTION_IN : ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            DEBUG("ATA pass-through cmd direction = 0x%02x. status = %u\n", *direction, status);
            break;

        case SCSI_UNMAP:
        case SCSI_MODE_SELECT10:
            // Check parameter list length.
            if (cdb[7] | cdb[8])
            {
                *direction = ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        case SCSI_FORMAT_UNIT:
            if (cdb[1] & FORMAT_UNIT_FMTDATA_BIT)
            {
                *direction = ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        case SCSI_SECURITY_PROTOCOL_IN:
        case SCSI_SECURITY_PROTOCOL_OUT:
            // Check allocation length.
            if (cdb[6] | cdb[7] | cdb[8] | cdb[9])
            {
                *direction = (cdb[0] == SCSI_SECURITY_PROTOCOL_IN) ? ENDPT_DIRECTION_IN : ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        case SCSI_MODE_SELECT6:
            // Check parameter list length.
            if (cdb[4])
            {
                *direction = ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        case SCSI_SEND_DIAGNOSTIC:
            if (cdb[3] | cdb[4])
            {
                *direction = ENDPT_DIRECTION_OUT;
            }
            else
            {
                status = STATUS_NO_DATA;
            }
            break;

        default:
            CRIT("-> scsi_get_data_direction() - unknown cmd 0x%02x.\n", cdb[0]);
            status = STATUS_ERROR;
            break;
    }

    return status;
}


#define ATA_PASS_THROUGH_BYTE_BLOCK_BIT 0x04
#define ATA_PASS_THROUGH_EXTEND_BIT     0x01

#define T_LENGTH_NO_DATA    0x00
#define T_LENGTH_FEATURES   0x01
#define T_LENGTH_SECTOR_CNT 0x02
#define T_LENGTH_TPSIU      0x03

/*****************************************************************************
 * Function: scsi_build_ata_pass_through_cmd
 *************************************************************************//**
 * This function builds a ATA PASS-THROUGH command.
 *
 * @param[in] ata_cmd pointer to ATA command structure.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_ata_pass_through_cmd(ATA_COMMAND_T *ata_cmd)
{
    UINT32_T sector_cnt_index;
    UINT32_T features_index;

    ti_memset(ata_cmd, 0, sizeof(ATA_COMMAND_T));

    // Build ATA command.
    ata_cmd->fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd->fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */

    // Clear EXTEND bit in ATA Return Desciptor.
    ata_return_descriptor_data[2] = 0;

    if (scsi_cmd.pCmdInput->pCommandBlock[0] == ATA_PASS_THROUGH12)
    {
        // Store CDB index for sector count and features.
        sector_cnt_index = 4;
        features_index = 3;

        ata_cmd->fis.features   = scsi_cmd.pCmdInput->pCommandBlock[3];
        ata_cmd->fis.sector_cnt = scsi_cmd.pCmdInput->pCommandBlock[4];
        ata_cmd->fis.LBA_low    = scsi_cmd.pCmdInput->pCommandBlock[5];
        ata_cmd->fis.LBA_mid    = scsi_cmd.pCmdInput->pCommandBlock[6];
        ata_cmd->fis.LBA_high   = scsi_cmd.pCmdInput->pCommandBlock[7];
        ata_cmd->fis.device     = scsi_cmd.pCmdInput->pCommandBlock[8];
        ata_cmd->fis.command    = scsi_cmd.pCmdInput->pCommandBlock[9];
        // pCommandBlock[10] is RSVD.
        ata_cmd->fis.control    = scsi_cmd.pCmdInput->pCommandBlock[11];
    }
    else /* ATA_PASS_THROUGH16 */
    {
        // Store CDB index for sector count and features.
        sector_cnt_index = 6;
        features_index = 4;

        ata_cmd->fis.features       = scsi_cmd.pCmdInput->pCommandBlock[4];
        ata_cmd->fis.sector_cnt     = scsi_cmd.pCmdInput->pCommandBlock[6];
        ata_cmd->fis.LBA_low        = scsi_cmd.pCmdInput->pCommandBlock[8];
        ata_cmd->fis.LBA_mid        = scsi_cmd.pCmdInput->pCommandBlock[10];
        ata_cmd->fis.LBA_high       = scsi_cmd.pCmdInput->pCommandBlock[12];
        ata_cmd->fis.device         = scsi_cmd.pCmdInput->pCommandBlock[13];
        ata_cmd->fis.command        = scsi_cmd.pCmdInput->pCommandBlock[14];
        ata_cmd->fis.control        = scsi_cmd.pCmdInput->pCommandBlock[15];

        if (scsi_cmd.pCmdInput->pCommandBlock[1] & ATA_PASS_THROUGH_EXTEND_BIT)
        {
            ata_cmd->fis.features_exp   = scsi_cmd.pCmdInput->pCommandBlock[3];
            ata_cmd->fis.sector_cnt_exp = scsi_cmd.pCmdInput->pCommandBlock[5];
            ata_cmd->fis.LBA_low_exp    = scsi_cmd.pCmdInput->pCommandBlock[7];
            ata_cmd->fis.LBA_mid_exp    = scsi_cmd.pCmdInput->pCommandBlock[9];
            ata_cmd->fis.LBA_high_exp   = scsi_cmd.pCmdInput->pCommandBlock[11];

            // Set EXTEND bit in ATA Return Desciptor.
            ata_return_descriptor_data[2] = ATA_PASS_THROUGH_EXTEND_BIT;
        }
    }

//    // Set wrap window and write flags to FALSE.
//    ata_cmd->bIsWriteCmd = FALSE;        // BQ - do we need to look at ATA command to determine this to set AHCI command header 'W' bit?
//    ata_cmd->bUseMemoryWrapWindow = FALSE;

    // Set check condition flag.
    ata_cmd->bCheckCondition = (scsi_cmd.pCmdInput->pCommandBlock[2] & ATA_PASS_THROUGH_CK_COND_BIT) ? TRUE : FALSE;

    switch (scsi_cmd.pCmdInput->pCommandBlock[2] & ATA_PASS_THROUGH_T_LENGTH_MASK)
    {
        case T_LENGTH_NO_DATA:
            ata_cmd->dDataByteCnt = 0;
            break;

        case T_LENGTH_FEATURES:
            if (scsi_cmd.pCmdInput->pCommandBlock[1] & ATA_PASS_THROUGH_EXTEND_BIT)
            {
                ata_cmd->dDataByteCnt = (scsi_cmd.pCmdInput->pCommandBlock[features_index - 1] << 8) | scsi_cmd.pCmdInput->pCommandBlock[features_index];
            }
            else
            {
                ata_cmd->dDataByteCnt = scsi_cmd.pCmdInput->pCommandBlock[features_index];
            }

            if (scsi_cmd.pCmdInput->pCommandBlock[2] & ATA_PASS_THROUGH_BYTE_BLOCK_BIT)
            {
                // Convert Blocks to bytes.
                ata_cmd->dDataByteCnt *= ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize;
            }
            break;

        case T_LENGTH_SECTOR_CNT:
            if (scsi_cmd.pCmdInput->pCommandBlock[1] & ATA_PASS_THROUGH_EXTEND_BIT)
            {
                ata_cmd->dDataByteCnt = (scsi_cmd.pCmdInput->pCommandBlock[sector_cnt_index - 1] << 8) | scsi_cmd.pCmdInput->pCommandBlock[sector_cnt_index];
            }
            else
            {
                ata_cmd->dDataByteCnt = scsi_cmd.pCmdInput->pCommandBlock[sector_cnt_index];
            }

            if (scsi_cmd.pCmdInput->pCommandBlock[2] & ATA_PASS_THROUGH_BYTE_BLOCK_BIT)
            {
                // Convert Blocks to bytes.
                ata_cmd->dDataByteCnt *= ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize;
            }
            break;

        default: /* T_LENGTH_TPSIU */
            // Get xfer length from CBW.
            ata_cmd->dDataByteCnt = scsi_cmd.pCmdInput->dDataXferLength;
            break;
    }

    if ((ata_cmd->fis.command == ATA_CMD_IDENTIFY_DEVICE) ||
        (ata_cmd->fis.command == ATA_CMD_IDENTIFY_PACKET_DEVICE))
    {
        // Manually set byte count because hdparm application specifies zero transfer length
        // and an AHCI overflow will occur if the PRDT is not configured for the data.
        ata_cmd->dDataByteCnt = 0x200;
    }

    /* AHCI's command-header W bit describes the ATA data phase, not the
     * enclosing SCSI opcode.  PASS-THROUGH opcodes therefore derive it from
     * T_DIR after the complete transfer length has been decoded. */
    ata_cmd->bIsWriteCmd =
        ((ata_cmd->dDataByteCnt != 0U) &&
         ((scsi_cmd.pCmdInput->pCommandBlock[2] &
           ATA_PASS_THROUGH_T_DIR_BIT) == 0U)) ? TRUE : FALSE;

    CRIT("-> scsi_build_ata_pass_through_cmd() - cmd = 0x%x, byte_cnt = %u, chk_cond = %u.\n",
         ata_cmd->fis.command, ata_cmd->dDataByteCnt, ata_cmd->bCheckCondition);

    return;
}


#define PROTOCOL_FIELD_MASK    0x1E
#define PROTOCOL_FIELD_OFFSET  1

// Protocol field
#define ATA_HW_RESET_CODE               0
#define SRST_CODE                       1
#define SEND_ATA_CMD_MIN_PROTOCOL_NUM   3
#define SEND_ATA_CMD_MAX_PROTOCOL_NUM   12
#define RETURN_RESPONSE_INFO            15

#define ATA_NON_DATA_PROTOCOL_CODE        3U
#define ATA_PIO_DATA_IN_PROTOCOL_CODE     4U
#define ATA_DMA_PROTOCOL_CODE             6U
#define ATA_UDMA_DATA_IN_PROTOCOL_CODE   10U

#define SCSI_ATA_READ_SECTORS             0x20U
#define SCSI_ATA_READ_SECTORS_EXT         0x24U
#define SCSI_ATA_READ_NATIVE_MAX_EXT      0x27U
#define SCSI_ATA_READ_MULTIPLE_EXT        0x29U
#define SCSI_ATA_READ_MULTIPLE            0xC4U
#define SCSI_ATA_READ_NATIVE_MAX          0xF8U

#define SCSI_ATA_SMART_READ_DATA          0xD0U
#define SCSI_ATA_SMART_READ_THRESHOLDS    0xD1U
#define SCSI_ATA_SMART_READ_LOG           0xD5U
#define SCSI_ATA_SMART_RETURN_STATUS      0xDAU
#define SCSI_ATA_SMART_LBA_MID            0x4FU
#define SCSI_ATA_SMART_LBA_HIGH           0xC2U

/**
 * @brief Validate a bounded ATA PASS-THROUGH data-in phase.
 *
 * @param protocol Decoded SAT protocol field.
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only for an explicitly declared, bounded data-in transfer.
 */
static BOOLEAN_T scsi_ata_pass_through_has_data_in(
    UINT8_T protocol,
    const ATA_COMMAND_T *ata_cmd)
{
    UINT8_T transfer_flags = scsi_cmd.pCmdInput->pCommandBlock[2];

    if ((ata_cmd == NULL) ||
        ((transfer_flags & ATA_PASS_THROUGH_T_DIR_BIT) == 0U) ||
        ((transfer_flags & ATA_PASS_THROUGH_T_LENGTH_MASK) ==
         T_LENGTH_NO_DATA) ||
        (ata_cmd->dDataByteCnt == 0U) ||
        (ata_cmd->dDataByteCnt >
         (UINT32_T)sizeof(datapath_ram->normal_data_buffer)))
    {
        return FALSE;
    }

    return ((protocol == ATA_PIO_DATA_IN_PROTOCOL_CODE) ||
            (protocol == ATA_DMA_PROTOCOL_CODE) ||
            (protocol == ATA_UDMA_DATA_IN_PROTOCOL_CODE)) ? TRUE : FALSE;
}

/**
 * @brief Validate an ATA PASS-THROUGH command with no data phase.
 *
 * @param protocol Decoded SAT protocol field.
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only when both SAT and AHCI descriptions carry no data.
 */
static BOOLEAN_T scsi_ata_pass_through_has_no_data(
    UINT8_T protocol,
    const ATA_COMMAND_T *ata_cmd)
{
    UINT8_T transfer_flags = scsi_cmd.pCmdInput->pCommandBlock[2];

    return ((ata_cmd != NULL) &&
            (protocol == ATA_NON_DATA_PROTOCOL_CODE) &&
            ((transfer_flags &
              (ATA_PASS_THROUGH_T_DIR_BIT |
               ATA_PASS_THROUGH_BYTE_BLOCK_BIT |
               ATA_PASS_THROUGH_T_LENGTH_MASK)) == 0U) &&
            (ata_cmd->dDataByteCnt == 0U)) ? TRUE : FALSE;
}

/**
 * @brief Match a data transfer to its ATA sector-count registers.
 *
 * Raw ATA read commands determine their device-side transfer from the task
 * file. Requiring SAT's transfer-length source to be the same sector count,
 * expressed in blocks, prevents a short PRDT from being paired with a larger
 * device transfer. Zero is rejected because ATA interprets it as a large
 * special count rather than an empty operation.
 *
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only when ATA and AHCI describe the same nonzero byte count.
 */
static BOOLEAN_T scsi_ata_pass_through_data_count_matches_sector_count(
    const ATA_COMMAND_T *ata_cmd)
{
    UINT8_T transfer_flags = scsi_cmd.pCmdInput->pCommandBlock[2];
    UINT32_T sector_count;
    UINT32_T sector_size;

    if ((ata_cmd == NULL) ||
        ((transfer_flags & ATA_PASS_THROUGH_T_LENGTH_MASK) !=
         T_LENGTH_SECTOR_CNT) ||
        ((transfer_flags & ATA_PASS_THROUGH_BYTE_BLOCK_BIT) == 0U))
    {
        return FALSE;
    }

    sector_count = ata_cmd->fis.sector_cnt;
    if ((scsi_cmd.pCmdInput->pCommandBlock[0] == ATA_PASS_THROUGH16) &&
        ((scsi_cmd.pCmdInput->pCommandBlock[1] &
          ATA_PASS_THROUGH_EXTEND_BIT) != 0U))
    {
        sector_count |= (UINT32_T)ata_cmd->fis.sector_cnt_exp << 8U;
    }
    sector_size = ata_dev[scsi_cmd.pCmdInput->bLUN].dSectorSize;
    if ((sector_count == 0U) || (sector_size == 0U) ||
        (sector_count > (0xFFFFFFFFU / sector_size)))
    {
        return FALSE;
    }
    return (ata_cmd->dDataByteCnt == (sector_count * sector_size)) ?
           TRUE : FALSE;
}

/**
 * @brief Recognize a tightly formed IDENTIFY DEVICE telemetry request.
 *
 * Packet devices are never admitted by the removable-media policy, and the
 * packet IDENTIFY opcode is intentionally not included here.
 *
 * @param protocol Decoded SAT protocol field.
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only for one 512-byte PIO data-in IDENTIFY DEVICE operation.
 */
static BOOLEAN_T scsi_ata_pass_through_is_identify_command(
    UINT8_T protocol,
    const ATA_COMMAND_T *ata_cmd)
{
    return ((ata_cmd != NULL) &&
            (ata_cmd->fis.command == ATA_CMD_IDENTIFY_DEVICE) &&
            (protocol == ATA_PIO_DATA_IN_PROTOCOL_CODE) &&
            scsi_ata_pass_through_has_data_in(protocol, ata_cmd) &&
            (ata_cmd->dDataByteCnt == 0x200U)) ? TRUE : FALSE;
}

/**
 * @brief Recognize explicitly read-only SMART telemetry subcommands.
 *
 * @param protocol Decoded SAT protocol field.
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only for bounded SMART reads or exact RETURN STATUS.
 */
static BOOLEAN_T scsi_ata_pass_through_is_smart_read_command(
    UINT8_T protocol,
    const ATA_COMMAND_T *ata_cmd)
{
    if ((ata_cmd == NULL) || (ata_cmd->fis.command != ATA_CMD_SMART) ||
        (ata_cmd->fis.LBA_mid != SCSI_ATA_SMART_LBA_MID) ||
        (ata_cmd->fis.LBA_high != SCSI_ATA_SMART_LBA_HIGH))
    {
        return FALSE;
    }

    switch (ata_cmd->fis.features)
    {
        case SCSI_ATA_SMART_READ_DATA:
        case SCSI_ATA_SMART_READ_THRESHOLDS:
        case SCSI_ATA_SMART_READ_LOG:
            return ((protocol == ATA_PIO_DATA_IN_PROTOCOL_CODE) &&
                    scsi_ata_pass_through_has_data_in(protocol, ata_cmd) &&
                    (ata_cmd->fis.sector_cnt == 1U) &&
                    (ata_cmd->dDataByteCnt == 0x200U)) ? TRUE : FALSE;

        case SCSI_ATA_SMART_RETURN_STATUS:
            return scsi_ata_pass_through_has_no_data(protocol, ata_cmd);

        default:
            return FALSE;
    }
}

/**
 * @brief Recognize the conservative read-only ATA command set.
 *
 * This set is used for generic media while the physical slider is locked.
 * Unknown commands and unclassified no-data commands are denied because ATA
 * contains media-mutating operations in both data-out and no-data forms.
 *
 * @param protocol Decoded SAT protocol field.
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only for a command proven read-only with a matching protocol.
 */
static BOOLEAN_T scsi_ata_pass_through_is_read_only_command(
    UINT8_T protocol,
    const ATA_COMMAND_T *ata_cmd)
{
    if (scsi_ata_pass_through_is_identify_command(protocol, ata_cmd) ||
        scsi_ata_pass_through_is_smart_read_command(protocol, ata_cmd))
    {
        return TRUE;
    }

    switch (ata_cmd->fis.command)
    {
        case ATA_CMD_CHECK_POWER_MODE:
        case ATA_CMD_READ_VERIFY_SECTORS:
        case ATA_CMD_READ_VERIFY_SECTORS_EXT:
        case SCSI_ATA_READ_NATIVE_MAX:
        case SCSI_ATA_READ_NATIVE_MAX_EXT:
            return scsi_ata_pass_through_has_no_data(protocol, ata_cmd);

        case SCSI_ATA_READ_SECTORS:
        case SCSI_ATA_READ_SECTORS_EXT:
        case SCSI_ATA_READ_MULTIPLE:
        case SCSI_ATA_READ_MULTIPLE_EXT:
            return ((protocol == ATA_PIO_DATA_IN_PROTOCOL_CODE) &&
                    scsi_ata_pass_through_has_data_in(
                        protocol, ata_cmd) &&
                    scsi_ata_pass_through_data_count_matches_sector_count(
                        ata_cmd)) ? TRUE : FALSE;

        case ATA_CMD_READ_DMA:
        case ATA_CMD_READ_DMA_EXT:
            return (((protocol == ATA_DMA_PROTOCOL_CODE) ||
                     (protocol == ATA_UDMA_DATA_IN_PROTOCOL_CODE)) &&
                    scsi_ata_pass_through_has_data_in(
                        protocol, ata_cmd) &&
                    scsi_ata_pass_through_data_count_matches_sector_count(
                        ata_cmd)) ? TRUE : FALSE;

        default:
            return FALSE;
    }
}

/**
 * @brief Apply the admitted-media ATA PASS-THROUGH policy.
 *
 * RDX media permits only non-media telemetry, keeping raw physical LBAs and
 * reserved metadata unreachable. Writable generic media retains the full SAT
 * command surface. A locked generic disk is restricted to the explicit
 * read-only set above.
 *
 * @param lun SATA logical unit.
 * @param protocol Decoded SAT protocol field.
 * @param ata_cmd Fully decoded ATA command.
 * @return TRUE only when the selected media policy permits submission.
 */
static BOOLEAN_T scsi_ata_pass_through_is_allowed(
    UINT32_T lun,
    UINT8_T protocol,
    const ATA_COMMAND_T *ata_cmd)
{
    SATA_MEDIA_KIND_T media_kind;

    if ((lun >= NUM_AHCI_PORTS) || (ata_cmd == NULL))
    {
        return FALSE;
    }
    media_kind = sata_media_get_kind(lun);
    if (media_kind == SATA_MEDIA_KIND_RDX)
    {
        return scsi_ata_pass_through_is_identify_command(
                   protocol, ata_cmd) ||
               scsi_ata_pass_through_is_smart_read_command(
                   protocol, ata_cmd) ||
               ((ata_cmd->fis.command == ATA_CMD_CHECK_POWER_MODE) &&
                scsi_ata_pass_through_has_no_data(protocol, ata_cmd));
    }
    if (media_kind != SATA_MEDIA_KIND_GENERIC)
    {
        return FALSE;
    }
    if (!rdx_hardware_is_write_protected())
    {
        return TRUE;
    }
    return scsi_ata_pass_through_is_read_only_command(protocol, ata_cmd);
}

/*****************************************************************************
 * Function: scsi_handle_ata_pass_through_cmd
 *************************************************************************//**
 * This function handles ATA PASS-THROUGH commands.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful and no response is pending.
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful and waiting for a response.
 * @retval STATUS_ERROR when no empty command slot is found.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 *
 ******************************************************************************
 */

STATUS_T scsi_handle_ata_pass_through_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;
    UINT8_T protocol;

    protocol = (scsi_cmd.pCmdInput->pCommandBlock[1] & PROTOCOL_FIELD_MASK) >> PROTOCOL_FIELD_OFFSET;

    DEBUG("-> scsi_handle_ata_pass_through_cmd() - protocol = 0x%x.\n", protocol);

    if ((protocol == ATA_HW_RESET_CODE) || (protocol == SRST_CODE))
    {
        /* Reset is completed locally. Recovery invalidates media and performs
         * fresh foreground admission without creating an ATA callback. */
        ahci_recover_local_command(scsi_cmd.pCmdInput->bLUN, TRUE);
        status = STATUS_OK;
    }
    else if ((protocol >= SEND_ATA_CMD_MIN_PROTOCOL_NUM) && (protocol <= SEND_ATA_CMD_MAX_PROTOCOL_NUM))
    {
        // Build the ATA command.
        scsi_build_ata_pass_through_cmd(&ata_cmd);

        if (scsi_ata_pass_through_is_allowed(
                scsi_cmd.pCmdInput->bLUN, protocol, &ata_cmd))
        {
            // Send ATA command to device.
            status = scsi_send_ata_cmd(&ata_cmd);
        }
        else if ((sata_media_get_kind(scsi_cmd.pCmdInput->bLUN) ==
                  SATA_MEDIA_KIND_GENERIC) &&
                 rdx_hardware_is_write_protected())
        {
            status = STATUS_SCSI_WRITE_PROTECTED;
        }
        else
        {
            status = STATUS_SCSI_INVALID_CMD;
        }
    }
    else if (protocol == RETURN_RESPONSE_INFO)
    {
        // Copy data into response buffer.
        ti_memcpy((void*)scsi_resp_buff, ata_return_descriptor_data, sizeof(ata_return_descriptor_data));

        // Set data pointer and byte count for ATA return descriptor.
        scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
        scsi_cmd.pCmdInput->dDataByteCnt = sizeof(ata_return_descriptor_data);

        status = STATUS_SCSI_RESPONSE_READY;
    }
    else
    {
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    return status;
}


#define DMA_FEATURE_BIT  0x01
#define DMA_DIR_BIT      0x04

/*****************************************************************************
 * Function: scsi_build_atapi_cmd
 *************************************************************************//**
 * This function builds a ATA PACKET command. Only valid for BOT transfers.
 *
 * @param[in] ata_cmd pointer to ATA command structure.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void scsi_build_atapi_cmd(ATA_COMMAND_T *ata_cmd)
{
    DEBUG("-> scsi_build_atapi_cmd()\n");

    ti_memset(ata_cmd, 0, sizeof(ATA_COMMAND_T));

    // Build ATA command.
    ata_cmd->fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd->fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
    ata_cmd->fis.command = ATA_CMD_PACKET;

    ata_cmd->fis.LBA_mid = 0xFF;    /* byte count limit (7:0) for PIO */
    ata_cmd->fis.LBA_high = 0xFF;   /* byte count limit (15:8) for PIO */

    // Set DMA bit only for transfers 32-bytes and larger. (workaround for customer device)
    if (gCBW->dDataTransferLength >= 32)
    {
        ata_cmd->fis.features = DMA_FEATURE_BIT;  

        if (ata_dev[scsi_cmd.pCmdInput->bLUN].bDMADIR && (gCBW->bmFlags == ENDPT_DIRECTION_IN))
        {
            // Set DMADIR bit for read commands (transfer to host).
            ata_cmd->fis.features |= DMA_DIR_BIT;
        }
    }

    // Set other data for ATA cmd.
    ata_cmd->bIsWriteCmd = scsi_is_write_cmd(scsi_cmd.pCmdInput->pCommandBlock[0]);

    // Use memory wrap window for R/W commands.
    ata_cmd->bUseMemoryWrapWindow = scsi_is_rw_cmd(scsi_cmd.pCmdInput->pCommandBlock[0]);

    // Set ATA cmd byte count to transfer length specified by BOT CBW.  UAS is not supported for ATAPI 
    // because the Command IU does not contain transfer length info and block sizes are variable.
    ata_cmd->dDataByteCnt = gCBW->dDataTransferLength;

    if (!ata_cmd->bUseMemoryWrapWindow)
    {
        if (ata_cmd->dDataByteCnt > sizeof(datapath_ram->normal_data_buffer))
        {
            // Non-RW commands do not use the memory wrap window therefore are limited to the memory window size.
            ata_cmd->dDataByteCnt = sizeof(datapath_ram->normal_data_buffer);

            CRIT("@Error: ATAPI command 0x%x transfer length %u is > %u bytes.\n", scsi_cmd.pCmdInput->pCommandBlock[0],
                 gCBW->dDataTransferLength, sizeof(datapath_ram->normal_data_buffer));
        }
        else if (gCBW->bmFlags == ENDPT_DIRECTION_IN)
        {
            // Workaround to prevent SATA hang if transfer length from CBW is less than SATA transfer size.
            ata_cmd->dDataByteCnt = sizeof(datapath_ram->normal_data_buffer);
        }
    }

    INFO("ATAPI scsi_cmd = 0x%x, byte_cnt = %u.\n", scsi_cmd.pCmdInput->pCommandBlock[0], ata_cmd->dDataByteCnt);
    return;
}


/*****************************************************************************
 * Function: scsi_atapi_cmd_handler
 *************************************************************************//**
 * This function handles SCSI commands for ATAPI devices.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_ERROR when no empty command slot is found.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_atapi_cmd_handler(void)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status;

    INFO("-> scsi_atapi_cmd_handler()\n");

    if (!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete)
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;

    if (((scsi_cmd.pCmdInput->pCommandBlock[0] == ATA_PASS_THROUGH12) && (scsi_cmd.pCmdInput->pCommandBlock[9])) ||  /* SCSI command 0xA1 BLANK is used to erase CD. */
        (scsi_cmd.pCmdInput->pCommandBlock[0] == ATA_PASS_THROUGH16))
    {
        status = scsi_handle_ata_pass_through_cmd();
    }
    else
    {
        // Build ATAPI command.
        scsi_build_atapi_cmd(&ata_cmd);

        // Copy SCSI command into ATA cmd stucture.
        ti_memcpy(ata_cmd.atapi_cdb, scsi_cmd.pCmdInput->pCommandBlock, scsi_cmd.pCmdInput->bCmdBlkLength);

        // Send ATAPI command to device.
        status = scsi_send_ata_cmd(&ata_cmd);
    }

    return status;
}



#define IMMED_BIT 0x02

/*****************************************************************************
 * Function: scsi_handle_synchronize_cache_cmd
 *************************************************************************//**
 * This function handles SCSI synchronize cache commands and sends
 * and ATA flush command to the device.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful and waiting for a response.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 * @retval STATUS_SCSI_INVALID_CMD when the command is invalid.
 * @retval STATUS_ERROR when no empty command slot is found.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_synchronize_cache_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
    BOOLEAN_T immed;
    STATUS_T status = STATUS_OK;

    DEBUG("-> scsi_handle_synchronize_cache_cmd()\n");

    if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_SYNCHRONIZE_CACHE10)
    {
        // Check control byte.
        if (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }
    }
    else if (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_SYNCHRONIZE_CACHE16)
    {
        // Check control byte.
        if (scsi_cmd.pCmdInput->pCommandBlock[15] & NACA_CONTROL_BIT)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }
    }
    else
    {
        status = STATUS_SCSI_INVALID_CMD;
    }

    if (status == STATUS_OK)
    {
        immed = (scsi_cmd.pCmdInput->pCommandBlock[1] & IMMED_BIT) ? TRUE : FALSE;

        // Build ATA command.
        ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

        ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
        ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
        ata_cmd.fis.command = (ata_dev[scsi_cmd.pCmdInput->bLUN].bLBA48) ? ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE;

        // Send ATA command to device.
        status = scsi_send_ata_cmd(&ata_cmd);

        if (immed)
        {
            //status = STATUS_OK;

            // Treat as if immed = 0 so that mass storage layers won't send two responses for this command.
        }
    }

    return status;
}


#define DSM_BLOCK_SIZE   0x200
#define ATA_TRIM_BIT     0x1
#define UNMAP_ANCHOR_BIT 0x1
#define UNMAP_HEADER_SIZE       8U
#define UNMAP_DESCRIPTOR_SIZE  16U
#define DSM_RANGE_ENTRY_SIZE    8U
#define DSM_RANGES_PER_BLOCK   (DSM_BLOCK_SIZE / DSM_RANGE_ENTRY_SIZE)

/**
 * @brief Send one validated ATA TRIM payload from the normal data buffer.
 *
 * @param dsm_data_block_cnt Number of populated 512-byte DSM blocks.
 * @param wait_for_completion TRUE for an intermediate synchronous chunk.
 * @return SCSI/ATA submission result.
 */
static STATUS_T scsi_send_trim_data(
    UINT32_T dsm_data_block_cnt,
    BOOLEAN_T wait_for_completion)
{
    ATA_COMMAND_T ata_cmd;
    SCSI_INTERMEDIATE_SESSION_T intermediate_session;
    STATUS_T status;
    UINT32_T lun = scsi_cmd.pCmdInput->bLUN;

    if (dsm_data_block_cnt == 0U)
    {
        return STATUS_OK;
    }
    if (!ata_dev[lun].bDeviceInitComplete ||
        (sata_media_get_kind(lun) != SATA_MEDIA_KIND_GENERIC))
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }

    ti_memset(&ata_cmd, 0U, sizeof(ata_cmd));
    ata_cmd.fis.command = ATA_CMD_DATA_SET_MGMT;
    ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
    ata_cmd.fis.CRRR_PMP = 0x80U;
    ata_cmd.fis.features = ATA_TRIM_BIT;
    ata_cmd.fis.sector_cnt = (UINT8_T)dsm_data_block_cnt;
    ata_cmd.fis.sector_cnt_exp =
        (UINT8_T)(dsm_data_block_cnt >> 8);
    ata_cmd.fis.device = ATA_LBA_BIT;
    ata_cmd.dDataByteCnt = dsm_data_block_cnt * DSM_BLOCK_SIZE;
    ata_cmd.bIsWriteCmd = TRUE;

    if (wait_for_completion &&
        !scsi_begin_intermediate_ata_command(lun, &intermediate_session))
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }
    status = scsi_send_ata_cmd(&ata_cmd);
    if (wait_for_completion)
    {
        status = scsi_finish_intermediate_ata_command(
            lun, &intermediate_session, status, 10000);
    }
    return status;
}

/*****************************************************************************
 * Function: scsi_handle_unmap_cmd
 *************************************************************************//**
 * This function handles SCSI UNMAP commands.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when the anchor bit is set.
 * @retval STATUS_ERROR when no empty command slot is found.
 * @retval STATUS_SCSI_ATA_ERROR when the ATA command fails.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_unmap_cmd(void)
{
    UINT32_T param_list_len;
    UINT32_T descriptor_data_len;
    UINT32_T num_unmap_block_desc;
    UINT32_T num_blocks;
    UINT32_T dsm_range_len;
    UINT32_T dsm_data_block_cnt;
    UINT32_T dsm_entry_count = 0U;
    UINT32_T max_dsm_blocks;
    UINT32_T max_dsm_entries;
    UINT32_T lun = scsi_cmd.pCmdInput->bLUN;
    UINT64_T lba;
    UINT8_T *unmap_param_list;
    UINT8_T *dsm_data;
    UINT8_T *block_desc;
    STATUS_T status;

    // Get parameter list length.
    param_list_len = (scsi_cmd.pCmdInput->pCommandBlock[7] << 8) | scsi_cmd.pCmdInput->pCommandBlock[8];

    DEBUG("-> scsi_handle_unmap_cmd() - anchor = %u, list_len = 0x%x.\n", 
          (scsi_cmd.pCmdInput->pCommandBlock[1] & UNMAP_ANCHOR_BIT), param_list_len);

    if (!ata_dev[lun].bTRIMSupport)
    {
        return STATUS_SCSI_INVALID_CMD;
    }

    // Check ANCHOR bit and control byte.
    if ((scsi_cmd.pCmdInput->pCommandBlock[1] & UNMAP_ANCHOR_BIT) || 
        (scsi_cmd.pCmdInput->pCommandBlock[9] & NACA_CONTROL_BIT))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    if (param_list_len == 0U)
    {
        return STATUS_OK;
    }
    if ((param_list_len < UNMAP_HEADER_SIZE) ||
        (param_list_len > MAX_UNMAP_DESC_SIZE) ||
        (scsi_cmd.pCmdInput->dDataXferLength < param_list_len))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    /* Preserve the received parameter list in the upper half of the 4-KiB
     * scratch region. The lower half remains available for generated DSM
     * entries and is the fixed AHCI data pointer for this command class. */
    unmap_param_list = (UINT8_T*)(datapath_ram->normal_data_buffer +
                                  (sizeof(datapath_ram->normal_data_buffer) - MAX_UNMAP_DESC_SIZE));
    ti_memcpy(unmap_param_list, (void*)datapath_ram->normal_data_buffer,
              param_list_len);

    descriptor_data_len = ((UINT32_T)unmap_param_list[2] << 8) |
                          (UINT32_T)unmap_param_list[3];
    if (((descriptor_data_len % UNMAP_DESCRIPTOR_SIZE) != 0U) ||
        (descriptor_data_len > (param_list_len - UNMAP_HEADER_SIZE)) ||
        (descriptor_data_len >
         (scsi_cmd.pCmdInput->dDataXferLength - UNMAP_HEADER_SIZE)))
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }
    num_unmap_block_desc = descriptor_data_len / UNMAP_DESCRIPTOR_SIZE;
    if (num_unmap_block_desc == 0U)
    {
        return STATUS_OK;
    }

    dsm_data = (UINT8_T*)datapath_ram->normal_data_buffer;
    max_dsm_blocks = MIN(
        (UINT32_T)ata_dev[lun].wDataSetMgmtMaxBlocks,
        (UINT32_T)(sizeof(datapath_ram->normal_data_buffer) -
                   MAX_UNMAP_DESC_SIZE) / DSM_BLOCK_SIZE);
    if (max_dsm_blocks == 0U)
    {
        return STATUS_SCSI_INVALID_CMD;
    }
    max_dsm_entries = max_dsm_blocks * DSM_RANGES_PER_BLOCK;
    ti_memset(dsm_data, 0U, max_dsm_blocks * DSM_BLOCK_SIZE);

    block_desc = &unmap_param_list[8];
    while (num_unmap_block_desc-- != 0U)
    {
        /* Decode in host-independent byte order, then validate the complete
         * range before adding any destructive ATA entry. */
        lba = ((UINT64_T)block_desc[0] << 56) | ((UINT64_T)block_desc[1] << 48) |
              ((UINT64_T)block_desc[2] << 40) | ((UINT64_T)block_desc[3] << 32) |
              ((UINT64_T)block_desc[4] << 24) | ((UINT64_T)block_desc[5] << 16) |
              ((UINT64_T)block_desc[6] << 8) | (UINT64_T)block_desc[7];
        num_blocks = ((UINT32_T)block_desc[8] << 24) |
                     ((UINT32_T)block_desc[9] << 16) |
                     ((UINT32_T)block_desc[10] << 8) |
                     (UINT32_T)block_desc[11];
        if ((num_blocks != 0U) &&
            ((lba >= ata_dev[lun].ddMaxLBA) ||
             ((UINT64_T)num_blocks > (ata_dev[lun].ddMaxLBA - lba))))
        {
            return STATUS_SCSI_INVALID_ADDRESS_RANGE;
        }

        while (num_blocks != 0U)
        {
            if (dsm_entry_count == max_dsm_entries)
            {
                status = scsi_send_trim_data(max_dsm_blocks, TRUE);
                if (status != STATUS_OK)
                {
                    return status;
                }
                dsm_entry_count = 0U;
                ti_memset(dsm_data, 0U,
                          max_dsm_blocks * DSM_BLOCK_SIZE);
            }

            dsm_range_len = MIN(num_blocks, 0xFFFFU);
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 0U] =
                (UINT8_T)lba;
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 1U] =
                (UINT8_T)(lba >> 8);
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 2U] =
                (UINT8_T)(lba >> 16);
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 3U] =
                (UINT8_T)(lba >> 24);
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 4U] =
                (UINT8_T)(lba >> 32);
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 5U] =
                (UINT8_T)(lba >> 40);
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 6U] =
                (UINT8_T)dsm_range_len;
            dsm_data[dsm_entry_count * DSM_RANGE_ENTRY_SIZE + 7U] =
                (UINT8_T)(dsm_range_len >> 8);
            dsm_entry_count++;
            num_blocks -= dsm_range_len;
            lba += dsm_range_len;
        }
        block_desc += UNMAP_DESCRIPTOR_SIZE;
    }

    if (dsm_entry_count == 0U)
    {
        return STATUS_OK;
    }
    dsm_data_block_cnt =
        (dsm_entry_count + DSM_RANGES_PER_BLOCK - 1U) /
        DSM_RANGES_PER_BLOCK;
    return scsi_send_trim_data(dsm_data_block_cnt, FALSE);
}

#define INC_512_BIT 0x80

/*****************************************************************************
 * Function: scsi_handle_security_protocol_cmd
 *************************************************************************//**
 * This function handles SCSI SECURITY PROTOCOL IN/OUT command.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 * @retval STATUS_ERROR when no empty command slot is found.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_security_protocol_cmd(void)
{
    ATA_COMMAND_T ata_cmd;
    STATUS_T status = STATUS_OK;
    UINT32_T alloc_len = ((scsi_cmd.pCmdInput->pCommandBlock[6] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[7] << 16) | (scsi_cmd.pCmdInput->pCommandBlock[8] << 8) | scsi_cmd.pCmdInput->pCommandBlock[9]);
    UINT32_T ata_data_len;
    UINT32_T response_length;
    UINT16_T protocol_specific;

    DEBUG("-> scsi_handle_security_protocol_cmd() - cmd = 0x%02x, protocol = 0x%02x, specific = 0x%04x.\n",
          scsi_cmd.pCmdInput->pCommandBlock[0], scsi_cmd.pCmdInput->pCommandBlock[1],
          (scsi_cmd.pCmdInput->pCommandBlock[2] << 8) | scsi_cmd.pCmdInput->pCommandBlock[3]);

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[11] & NACA_CONTROL_BIT)
    {
        status = STATUS_SCSI_INVALID_CMD_FIELD;
    }

    protocol_specific =
        ((UINT16_T)scsi_cmd.pCmdInput->pCommandBlock[2] << 8U) |
        scsi_cmd.pCmdInput->pCommandBlock[3];
    if ((status == STATUS_OK) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_SECURITY_PROTOCOL_IN) &&
        (scsi_cmd.pCmdInput->pCommandBlock[1] ==
         RDX_SECURITY_PROTOCOL_TANDBERG) &&
        !(scsi_cmd.pCmdInput->pCommandBlock[4] & INC_512_BIT))
    {
        response_length = rdx_manager_build_security_protocol_in(
            (UINT8_T *)scsi_resp_buff, scsi_resp_buff_sz,
            protocol_specific);
        if (response_length != 0U)
        {
            scsi_cmd.pCmdInput->pData = (void *)scsi_resp_buff;
            scsi_cmd.pCmdInput->dDataByteCnt = MIN(alloc_len,
                                                   response_length);
            return STATUS_SCSI_RESPONSE_READY;
        }
    }

    if (scsi_cmd.pCmdInput->pCommandBlock[4] & INC_512_BIT)
    {
        if (alloc_len > 0x0000FFFF)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        ata_data_len = alloc_len;
    }
    else
    {
        if (alloc_len > 0x01FFFE00)
        {
            status = STATUS_SCSI_INVALID_CMD_FIELD;
        }

        ata_data_len = ((alloc_len + 511) / 512);
    }

    if (status == STATUS_OK)
    {
        // Clear ATA response buffer.
        ti_memset((void*)datapath_ram->normal_data_buffer, 0, (ata_data_len * 512));

        // Build ATA command.
        ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

        // Build command FIS.
        ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
        ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */

        if (alloc_len == 0)
        {
            ata_cmd.fis.command = ATA_CMD_TRUSTED_NON_DATA;
            // Set non-data receive bit if SCSI SECURITY PROTOCOL IN.
            ata_cmd.fis.LBA_low_exp = (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_SECURITY_PROTOCOL_IN) ? 0x01 : 0x00;
        }
        else
        {
            ata_cmd.fis.command = (scsi_cmd.pCmdInput->pCommandBlock[0] == SCSI_SECURITY_PROTOCOL_IN) ? ATA_CMD_TRUSTED_RECEIVE_DMA : ATA_CMD_TRUSTED_SEND_DMA;
            // Set transfer length.
            ata_cmd.fis.LBA_low = (ata_data_len >> 8) & 0xFF;
            ata_cmd.fis.sector_cnt = ata_data_len & 0xFF;
        }

        // Set security protocol.
        ata_cmd.fis.features = scsi_cmd.pCmdInput->pCommandBlock[1];

        // Set security protocol specific data field.
        ata_cmd.fis.LBA_mid = scsi_cmd.pCmdInput->pCommandBlock[3];
        ata_cmd.fis.LBA_high = scsi_cmd.pCmdInput->pCommandBlock[2];

        // Set ATA data transfer length.
        ata_cmd.dDataByteCnt = ata_data_len * 512;

        // Send ATA command to device.
        status = scsi_send_ata_cmd(&ata_cmd);
    }

    return status;
}

#define FMT_DATA_BIT  0x10
#define DEFECT_LIST_FORMAT_BITS  0x07

/*****************************************************************************
 * Function: scsi_handle_format_unit_cmd
 *************************************************************************//**
 * This function handles SCSI FORMAT UNIT command.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_format_unit_cmd(void)
{
    STATUS_T status = STATUS_SCSI_INVALID_CMD_FIELD;

    DEBUG("-> scsi_handle_format_unit_cmd()\n");

    // Check control byte.
    if (!(scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT))
    {
        if ((scsi_cmd.pCmdInput->pCommandBlock[1] & (FMT_DATA_BIT | DEFECT_LIST_FORMAT_BITS)) == 0)
        {
            status = STATUS_OK;
        }
    }

    return status;
}

#define UNITOFFL_BIT 0x1
#define DEVOFFL_BIT  0x2
#define SELFTEST_BIT 0x4
#define SELFTEST_CODE_BITS 0xE

#define ATA_SMART_SUPPORT_BIT 0x0002
#define ATA_SMART_ENABLED_BIT 0x0001

#define ATA_CMD_FEATURE_SMART_EXEC_OFFLINE_IMMED    0xD4

/**
 * @brief Run one polled READ VERIFY step for the default self-test.
 *
 * The shared intermediate-command guard prevents a completion interrupt from
 * reaching the USB layer and refuses to restore command-owned interrupt bits
 * after the admitted media changes.
 *
 * @param logical_lba Host-visible block to verify.
 * @return STATUS_OK on success, logical-unit-not-ready after media invalidation,
 *         or STATUS_SCSI_SELF_TEST_FAILURE for another command failure.
 */
static STATUS_T scsi_run_diagnostic_verify(UINT64_T logical_lba)
{
    ATA_COMMAND_T ata_cmd;
    SCSI_INTERMEDIATE_SESSION_T intermediate_session;
    UINT64_T *lba = (UINT64_T *)&scsi_cmd.bLBA[0];
    UINT32_T lun = scsi_cmd.pCmdInput->bLUN;
    STATUS_T status;

    *lba = logical_lba;
    scsi_cmd.dXferLength = 1U;
    if (!scsi_build_ata_read_verify_sectors_cmd(&ata_cmd) ||
        !scsi_begin_intermediate_ata_command(lun, &intermediate_session))
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }

    status = scsi_finish_intermediate_ata_command(
        lun,
        &intermediate_session,
        scsi_send_ata_cmd(&ata_cmd),
        2000);
    if ((status != STATUS_OK) &&
        (status != STATUS_SCSI_LOGICAL_UNIT_NOT_READY))
    {
        status = STATUS_SCSI_SELF_TEST_FAILURE;
    }
    return status;
}

/*****************************************************************************
 * Function: scsi_handle_send_diagnostic_cmd
 *************************************************************************//**
 * This function handles SCSI SEND DIAGNOSTIC command. Only the default self-test
 * is supported.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_SCSI_INVALID_CMD_FIELD when any command params are invalid.
 * @retval STATUS_SCSI_SELF_TEST_FAILURE if the self-test fails.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_send_diagnostic_cmd(void)
{
#if ENABLE_SMART_EXEC_OFFLINE_IMMED
    ATA_COMMAND_T ata_cmd;
#endif
    STATUS_T status = STATUS_SCSI_INVALID_CMD_FIELD;

    DEBUG("-> scsi_handle_send_diagnostic_cmd()\n");

    // Check control byte.
    if (scsi_cmd.pCmdInput->pCommandBlock[5] & NACA_CONTROL_BIT)
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    status = rdx_manager_handle_send_diagnostic(
        scsi_cmd.pCmdInput->pCommandBlock,
        (UINT8_T *)datapath_ram->normal_data_buffer,
        ((UINT32_T)scsi_cmd.pCmdInput->pCommandBlock[3] << 8U) |
        scsi_cmd.pCmdInput->pCommandBlock[4]);
    if (status != STATUS_NOT_SUPPORTED)
    {
        return status;
    }

    // Check param list length is zero.
    if (scsi_cmd.pCmdInput->pCommandBlock[3] | scsi_cmd.pCmdInput->pCommandBlock[4])
    {
        return STATUS_SCSI_INVALID_CMD_FIELD;
    }

    // Only default self-test is supported.
    if ((scsi_cmd.pCmdInput->pCommandBlock[1] & (SELFTEST_CODE_BITS | DEVOFFL_BIT | UNITOFFL_BIT | SELFTEST_BIT)) == SELFTEST_BIT)
    {
        // NOTE: SMART Execute offline immediate may take several minutes to complete so this
        // feature is not currently enabled.
#if ENABLE_SMART_EXEC_OFFLINE_IMMED
        // Verify the drive supports SMART and that it is enabled.
        if ((ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[84] & ATA_SMART_SUPPORT_BIT) && 
            (ata_dev[scsi_cmd.pCmdInput->bLUN].wIdentifyDeviceInfo[85] & ATA_SMART_ENABLED_BIT))
        {
            INFO("sending ATA_CMD_FEATURE_SMART_EXEC_OFFLINE_IMMED.\n");
            ti_memset(&ata_cmd, 0, sizeof(ata_cmd));

            // Build ATA Set feature for write cache. 
            ata_cmd.fis.FIS_type = H2D_REGISTER_FIS_TYPE;
            ata_cmd.fis.CRRR_PMP = 0x80;  /* Set C bit, no port multiplier */
            ata_cmd.fis.command = ATA_CMD_SMART;
            ata_cmd.fis.features = ATA_CMD_FEATURE_SMART_EXEC_OFFLINE_IMMED;

            ata_cmd.fis.LBA_high = 0xC2;
            ata_cmd.fis.LBA_mid  = 0x4F;
            ata_cmd.fis.LBA_low  = 0x81;  // Short self-test in captive mode.

            // Send ATA command to device.
            status = scsi_send_ata_cmd(&ata_cmd);
        }
        else
#endif
        {
            /* Verify the first, last, and midpoint host-visible blocks through
             * the selected media mapping. Each intermediate command is guarded
             * independently so a hot-plug boundary stops the sequence. */
            status = scsi_run_diagnostic_verify(0U);
            if (status == STATUS_OK)
            {
                status = scsi_run_diagnostic_verify(
                    ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA - 1U);
            }
            if (status == STATUS_OK)
            {
                status = scsi_run_diagnostic_verify(
                    ata_dev[scsi_cmd.pCmdInput->bLUN].ddMaxLBA >> 1);
            }
        }
    }

    return status;
}



/*****************************************************************************
 * Function: scsi_handle_ti_defined_cmd
 *************************************************************************//**
 * This function handles TI-defined SCSI commands for ATA devices.
 *
 * @param None.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_SCSI_RESPONSE_READY when successful and there is data to return.
 * @retval STATUS_SCSI_INVALID_CMD when the command is not handled.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_handle_ti_defined_cmd(void)
{
    static BOOLEAN_T flash_unlocked = FALSE;
    STATUS_T status;
    UINT32_T readStartAddress;

    switch (scsi_cmd.pCmdInput->pCommandBlock[0])
    {
        case SCSI_TI_ONE_TOUCH_BACKUP_QUERY:
            ti_memcpy((void*)scsi_resp_buff, one_touch_button_state_query(), ONE_TOUCH_STATE_SIZE);
            scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
            scsi_cmd.pCmdInput->dDataByteCnt = ONE_TOUCH_STATE_SIZE;
            status = STATUS_SCSI_RESPONSE_READY;
            break;

        case SCSI_TI_FLASH_UNLOCK:
            DEBUG("Flash unlocked.\n");
            flash_unlocked = TRUE;
            status = STATUS_OK;
            break;

        case SCSI_TI_FLASH_ERASE:
            if (flash_unlocked)
            {
                /*Erase SPI Flash*/
                SpiOps(OpcodeWriteEnable, 0x00000000, NULL, 0x0, 0);
                SpiOps(OpcodeChipErase, 0x00000000, NULL, 0x0, 0);
                status = STATUS_OK;
            }
            else
            {
                status = STATUS_SCSI_INVALID_CMD;
            }
            break;

        case SCSI_TI_GET_PID:
            /* TX Data (2 bytes) */
            scsi_resp_buff[0] = 0x60;
            scsi_resp_buff[1] = 0x92;
            scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
            scsi_cmd.pCmdInput->dDataByteCnt = 0x2;
            status = STATUS_SCSI_RESPONSE_READY;
            break;

        case SCSI_TI_GET_FW_VERSION:
            scsi_resp_buff[0] = FIRMWARE_MINOR_VERSION;
            scsi_resp_buff[1] = FIRMWARE_MAJOR_VERSION;
            scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
            scsi_cmd.pCmdInput->dDataByteCnt = 0x2;
            status = STATUS_SCSI_RESPONSE_READY;
            break;

        case SCSI_TI_GET_USB_SPEED:
            scsi_resp_buff[0] = usb_dev.dev_speed;
            scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
            scsi_cmd.pCmdInput->dDataByteCnt = 0x1;
            status = STATUS_SCSI_RESPONSE_READY;
            break;

        case SCSI_TI_DEVICE_RESET:
            usb_hal_disconnect();
            system_reset();
            status = STATUS_OK;
            break;

        case SCSI_TI_READ_FLASH:
            readStartAddress = ((scsi_cmd.pCmdInput->pCommandBlock[1] << 24) | (scsi_cmd.pCmdInput->pCommandBlock[2] << 16) |
                                (scsi_cmd.pCmdInput->pCommandBlock[3] << 8) | scsi_cmd.pCmdInput->pCommandBlock[4]);
            SpiOps(OpcodeReadData, readStartAddress, (UINT8_T*)&scsi_resp_buff[0], scsi_cmd.pCmdInput->pCommandBlock[5], 0);
            scsi_cmd.pCmdInput->pData = (void*)scsi_resp_buff;
            scsi_cmd.pCmdInput->dDataByteCnt = scsi_cmd.pCmdInput->pCommandBlock[5];
            status = STATUS_SCSI_RESPONSE_READY; 
            break;

        default:
            status = STATUS_SCSI_INVALID_CMD;
            break;
    }

    DEBUG("-> scsi_handle_ti_defined_cmd() - cmd = 0x%x, status = 0x%x.\n", scsi_cmd.pCmdInput->pCommandBlock[0], status);

    return status;
}


/*****************************************************************************
 * Function: scsi_ata_cmd_handler
 *************************************************************************//**
 * This function handles SCSI commands for ATA devices.
 *
 * @param None.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when successful.
 * @retval STATUS_ERROR when no empty command slot is found.
 * @retval STATUS_SCSI_INTERNAL_TARGET_FAILURE when there is a device fault.
 * @retval STATUS_<?> refer to "scsi_handle_" command handler functions.
 *
 ******************************************************************************
 */

inline STATUS_T scsi_ata_cmd_handler(void)
{
    STATUS_T status;

    INFO("-> scsi_ata_cmd_handler()\n");

    // Windows will give up enumerating the disk if INQUIRY command is failed
    // more than three times.
    if ((!ata_dev[scsi_cmd.pCmdInput->bLUN].bDeviceInitComplete ||
         rdx_hardware_is_logically_unloaded()) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_REQUEST_SENSE) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_INQUIRY) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_LOG_SENSE) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_MODE_SENSE6) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_MODE_SENSE10) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_MODE_SELECT6) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_MODE_SELECT10) &&
        /* PREVENT/ALLOW must inspect ALLOW before its media-state gate, while
         * START STOP owns branch-specific readiness and page 31h policy.  Both
         * therefore bypass this generic visibility gate and perform their
         * asymmetric checks in their handlers. */
        (scsi_cmd.pCmdInput->pCommandBlock[0] !=
             SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_START_STOP_UNIT) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_SECURITY_PROTOCOL_IN) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_WRITE_BUFFER) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_SEND_DIAGNOSTIC) &&
        (scsi_cmd.pCmdInput->pCommandBlock[0] != SCSI_REPORT_LUNS))
    {
        return STATUS_SCSI_LOGICAL_UNIT_NOT_READY;
    }

    /* Test the exact write class before evaluating the physical lock.  Reject before an
     * AHCI slot is allocated, but use a locally-completed status: there is no
     * ATA callback to finish BOT/UAS on this path. */
    if (scsi_is_media_write_class_cmd(scsi_cmd.pCmdInput->pCommandBlock) &&
        rdx_hardware_is_write_protected())
    {
        return STATUS_SCSI_WRITE_PROTECTED;
    }

    // Call appropriate command handler.
    switch (scsi_cmd.pCmdInput->pCommandBlock[0])
    {
        case SCSI_READ6:
        case SCSI_READ10:
        case SCSI_READ12:
        case SCSI_READ16:
        case SCSI_WRITE6:
        case SCSI_WRITE10:
        case SCSI_WRITE12:
        case SCSI_WRITE16:
            status = scsi_handle_rw_cmd();
            break;

        case SCSI_REPORT_LUNS:
            status = scsi_handle_report_luns_cmd();
            break;

        case SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL:
            status = scsi_handle_prevent_allow_medium_removal_cmd();
            break;

        case SCSI_MODE_SELECT6:
        case SCSI_MODE_SELECT10:
            status = scsi_handle_mode_select_cmd();
            break;

        case SCSI_START_STOP_UNIT:
            status = scsi_handle_start_stop_unit_cmd();
            break;

        case SCSI_INQUIRY:
            status = scsi_handle_inquiry_cmd();
            break;

        case SCSI_READ_FORMAT_CAPACITIES:
            status = scsi_handle_read_format_capacities_cmd();
            break;

        case SCSI_MODE_SENSE6:
        case SCSI_MODE_SENSE10:
//            status = STATUS_SCSI_INVALID_CMD;  /* Change for Mikroimage to improve write performance under Linux */
            status = scsi_handle_mode_sense_cmd();
            break;

        case SCSI_LOG_SENSE:
            status = scsi_handle_log_sense_cmd();
            break;

        case SCSI_WRITE_BUFFER:
            status = scsi_handle_write_buffer_cmd();
            break;

        case SCSI_READ_CAPACITY10:
        case SCSI_READ_CAPACITY16:
            status = scsi_handle_read_capacity_cmd();
            break;

        case SCSI_VERIFY10:
        case SCSI_VERIFY12:
        case SCSI_VERIFY16:
            status = scsi_handle_verify_cmd();
            break;

        case SCSI_REQUEST_SENSE:
            status = scsi_handle_request_sense_cmd();
            break;

        case SCSI_TEST_UNIT_READY:
            status = scsi_handle_test_unit_ready_cmd();
            break;

        case SCSI_SYNCHRONIZE_CACHE10:
        case SCSI_SYNCHRONIZE_CACHE16:
            status = scsi_handle_synchronize_cache_cmd();
            break;

        case ATA_PASS_THROUGH12:
        case ATA_PASS_THROUGH16:
            status = scsi_handle_ata_pass_through_cmd();
            break;

        case SCSI_UNMAP:
            status = scsi_handle_unmap_cmd();
            break;

        case SCSI_SECURITY_PROTOCOL_IN:
        case SCSI_SECURITY_PROTOCOL_OUT:
            status = scsi_handle_security_protocol_cmd();
            break;

        case SCSI_FORMAT_UNIT:
            status = scsi_handle_format_unit_cmd();
            break;

        case SCSI_SEND_DIAGNOSTIC:
            status = scsi_handle_send_diagnostic_cmd();
            break;

        default:
            CRIT("@Error: Invalid SCSI command = 0x%x.\n", scsi_cmd.pCmdInput->pCommandBlock[0]);
            status = STATUS_SCSI_INVALID_CMD;
            break;
    }

    return status;
}



/*****************************************************************************
 * Function: scsi_command_handler
 *************************************************************************//**
 * This function handles SCSI commands from USB mass storage layer.
 *
 * @param[in,out] cmd_input pointer to SCSI command input structure.
 *
 * @retval STATUS_SCSI_RESPONSE_PENDING when response is pending and command slot value in input structure is updated.
 * @retval STATUS_SCSI_RESPONSE_READY when response is ready and data pointer and byte count in input structure are updated.
 * @retval STATUS_<?> refer to scsi_atapi_cmd_handler() or scsi_atapi_cmd_handler() functions.
 *
 ******************************************************************************
 */

STATUS_T scsi_command_handler(SCSI_CMD_INPUT_T *cmd_input)
{
    STATUS_T status = STATUS_OK;

    DEBUG("-> scsi_command_handler() - cmd = 0x%02x.\n", cmd_input->pCommandBlock[0]);

    if (cmd_input->bLUN > UMS_MAX_LUN)
    {
        return STATUS_ERROR;
    }

    ti_memset(&scsi_cmd, 0, sizeof(scsi_cmd));

    // Copy pointer to input params to global struct.
    scsi_cmd.pCmdInput = cmd_input;
    /* Bind every builder and later submission to the media generation that
     * existed before this CDB was parsed or any physical LBA was derived. */
    scsi_cmd.dMediaEpoch = sata_media_get_epoch(cmd_input->bLUN);

    if ((cmd_input->pCommandBlock[0] >= SCSI_TI_OPCODE_MIN) &&
        (cmd_input->pCommandBlock[0] <= SCSI_TI_OPCODE_MAX) &&
        (cmd_input->bCmdBlkLength == 0x06))
    {
        status = scsi_handle_ti_defined_cmd();
    }
    else
    {
        if (ata_dev[cmd_input->bLUN].bPacketDevice)
        {
            status = scsi_atapi_cmd_handler();
        }
        else /* ATA device */
        {
            status = scsi_ata_cmd_handler();
        }
    }

    // Set sense data.
    switch (status)
    {
        case STATUS_OK:
        case STATUS_SCSI_RESPONSE_READY:
            scsi_set_sense_data(NO_SENSE, NO_ADDITIONAL_SENSE_INFO, NO_ASCQ);
            break;

        case STATUS_SCSI_INVALID_CMD_FIELD:
            scsi_set_sense_data(ILLEGAL_REQUEST, INVALID_FIELD_IN_COMMAND, NO_ASCQ);
            break;

        case STATUS_SCSI_INVALID_CMD:
            scsi_set_sense_data(ILLEGAL_REQUEST, INVALID_COMMAND, NO_ASCQ);
            break;

        case STATUS_SCSI_INVALID_ADDRESS_RANGE:
            scsi_set_sense_data(ILLEGAL_REQUEST, LBA_OUT_OF_RANGE, NO_ASCQ);
            break;

        case STATUS_SCSI_INTERNAL_TARGET_FAILURE:
            scsi_set_sense_data(HARDWARE_ERROR, INTERNAL_TARGET_FAILURE, NO_ASCQ);
            break;

        case STATUS_SCSI_LOGICAL_UNIT_NOT_READY:
            //scsi_set_sense_data(NOT_READY, LUN_NOT_READY, ASCQ_LUN_BECOMING_READY);  
            scsi_set_sense_data(NOT_READY, MEDIUM_NOT_PRESENT, NO_ASCQ);
            break;

        case STATUS_SCSI_MEDIUM_CHANGE:
            scsi_set_sense_data(UNIT_ATTENTION, MEDIUM_MAY_HAVE_CHANGED, NO_ASCQ);
            break;

        case STATUS_SCSI_RESPONSE_PENDING:
        case STATUS_SCSI_ATA_ERROR:
            // No sense data to set here.
            break;

        case STATUS_SCSI_SELF_TEST_FAILURE:
            scsi_set_sense_data(HARDWARE_ERROR, LOGICAL_UNIT_FAILED_SELF_TEST, ASCQ_LOGICAL_UNIT_FAILED_SELF_TEST);
            status = STATUS_SCSI_ATA_ERROR;
            break;

        case STATUS_SCSI_AUTHENTICATION_FAILURE:
            scsi_set_sense_data(DATA_PROTECT, SECURITY_ERROR,
                                ASCQ_DIGITAL_SIGNATURE_VALIDATION_FAILED);
            break;

        case STATUS_SCSI_MEDIUM_REMOVAL_PREVENTED:
            scsi_set_sense_data(ILLEGAL_REQUEST,
                                MEDIUM_REMOVAL_PREVENTED,
                                ASCQ_MEDIUM_REMOVAL_PREVENTED);
            break;

        case STATUS_SCSI_WRITE_PROTECTED:
            scsi_set_sense_data(DATA_PROTECT, WRITE_PROTECTED, NO_ASCQ);
            break;

        default:
            CRIT("@Warning: scsi_ata_cmd_handler() status 0x%x for cmd 0x%02x not handled.\n", status, scsi_cmd.pCmdInput->pCommandBlock[0]);
            break;
    }

    return status;
}
