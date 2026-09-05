/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ums_bot.c
//             
// Project     : TUSB926x Firmware.
//             
// Description : USB Mass Storage Bulk-Only Transport module. 
//               Compliant to BOT Spec Revision 1.0 (Sept 31, 1999).
// 
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   01/30/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the USB Mass Storage (UMS) Class Bulk-Only Transport (BOT) layer.
 *
 * The UMS BOT layer is compliant with the following specifications:
 * - USB Mass Storage Class, Bulk-Only Transport Specification, Revision 1.0 (Sept 31, 1999).
 *
 */

#include "ums_bot.h"
#include "ahci.h"
#include "scsi.h"
#include "sci.h"
#include "mww.h"
#include "usb_stack.h"
#include "usb_hal.h"
#include "string.h"
#include "reg_io.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#if BOT_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif

/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

UINT32_T gBOT_case;
UINT32_T gActualXferLength;

BOOLEAN_T gUSB_xfer_cmplt;
BOOLEAN_T gSATA_xfer_cmplt;
BOOLEAN_T gCheckCondition;  /* for ATA PASS-THROUGH cmds */

UINT32_T gCBW_length;
UMS_BOT_CBW_T *gCBW;  //stores incoming CBW for BOT block.
UMS_BOT_CSW_T *gCSW;  //stores outgoing CSW for BOT block.
UMS_BOT_STATE_T gBOT_state;  //stores the current state of the BOT state machine.

SCSI_CMD_INPUT_T ums_cmd;

UINT8_T gSCSI_cmd_pending;

void ums_bot_case_handler(UINT32_T case_num);
void ums_bot_send_CSW(UINT8_T csw_status);

#if DEBUG_LEVEL >= 1
void ums_bot_debug_dump(void)
{
    UINT32_T i;

    CRIT("BOT CBW:\n");
    CRIT(" Sig = 0x%08x\n", gCBW->dSignature);
    CRIT(" Tag = 0x%08x\n", gCBW->dTag);
    CRIT(" Xfer Len = 0x%08x\n", gCBW->dDataTransferLength);
    CRIT(" Flags = 0x%02x\n", gCBW->bmFlags);
    CRIT(" LUN = 0x%02x\n", gCBW->bLUN);
    CRIT(" CDB = 0x%02x", gCBW->CB[0]);
    for (i = 1; i < 16; i++)
    {
        kprintf(" %02x", gCBW->CB[i]);
    }
    kprintf("\n");

    CRIT("gCSW->dDataResidue = 0x%08x\n", gCSW->dDataResidue);
    CRIT("gBOT_state = %u\n", gBOT_state);
    CRIT("gBOT_case = %u\n", gBOT_case);
    CRIT("gUSB_xfer_cmplt = %u\n", gUSB_xfer_cmplt);
    CRIT("gSATA_xfer_cmplt = %u\n", gSATA_xfer_cmplt);

    return;
}
#endif

/*****************************************************************************
 * Function: ums_bot_tx
 *************************************************************************//**
 * This function sends I/O request to transmit data on the BULK IN 
 * endpoint.
 *
 * @param[in] buffer pointer to data buffer.
 * @param[in] byte_cnt number of bytes to transmit. 
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ums_bot_tx(void *buffer, UINT32_T byte_cnt)
{
    DEBUG("ums_bot_tx() - buff = 0x%08x, byte_cnt = %u.\n", (UINT32_T)buffer, byte_cnt);

    if (byte_cnt != 0)
    {
        usb_hal_io_request((BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), buffer, byte_cnt, 0);
    }
    else
    {
        // Case (4).  Host expects to receive data. Device intends to transfer no data.
        // This is for handling ATAPI devices where the device may return a byte count of zero.
        ums_bot_case_handler(4);
    }

    return;
}


/*****************************************************************************
 * Function: ums_bot_rx
 *************************************************************************//**
 * This function sends I/O request to recieve data on the BULK OUT 
 * endpoint.
 *
 * @param[out] buffer pointer to buffer to recieve data.
 * @param[in] byte_cnt number of bytes to recieve (must be non-zero).
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ums_bot_rx(void *buffer, UINT32_T byte_cnt)
{
    if (byte_cnt != 0)
    {
        usb_hal_io_request((BOT_BULK_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT), buffer, byte_cnt, 0);
    }

    return;
}


/*****************************************************************************
 * Function: ums_bot_stall
 *************************************************************************//**
 * This function stalls one of bulk endpoints based on direction.
 *
 * @param[in] endpt_dir endpoint direction to stall.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_stall(UINT32_T endpt_dir)
{
    DEBUG("-> ums_bot_stall() - %s.\n", (endpt_dir == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

    // Stall appropriate endpoints.
    if (endpt_dir == ENDPT_DIRECTION_OUT)
    {
        usb_hal_set_endpt_stall(BOT_BULK_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT);
    }
    else /* IN */
    {
        usb_hal_set_endpt_stall(BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN);
    }

    return;
}


/*****************************************************************************
 * Function: ums_bot_xfer_cleanup
 *************************************************************************//**
 * This function cancels any active transfers and optionally 
 * can stall the active bulk endpoint.
 *
 * @param[in] stall_active_endpt flag indicating whether or not the active endpoint 
 * should be stalled.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_xfer_cleanup(BOOLEAN_T stall_active_endpt)
{
    EP_INFO_T *pEP = &usb_dev.ep_info_OUT[BOT_BULK_OUT_ENDPT_NUM];

    DEBUG("-> ums_bot_xfer_cleanup() - stall = %u.\n", stall_active_endpt);

    if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
    {
        // Calculate bytes remaining to determine if the OUT transfer is complete.
        pEP->dBytesRemaining -= (pEP->dXferLength - (pEP->pTRB->dStatus & TRB_STATUS_BUFFER_SIZE_MASK));

        if (pEP->dBytesRemaining > 0)
        {
            // Cancel BULK OUT transaction.
            usb_hal_cancel_io_request((BOT_BULK_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT));

            // For OUT direction, only STALL endpoint if transfer is active. 
            if (stall_active_endpt)
            {
                INFO("STALLING BOT Bulk OUT. bytes_remaining = %d, scsi_cmd = 0x%x\n", 
                     pEP->dBytesRemaining, gSCSI_cmd_pending);
                ums_bot_stall(ENDPT_DIRECTION_OUT);
            }
        }
        else
        {
            // Mark transfer as inactive because USB Xfer Complete event may not have been processed
            // yet and we do not want a data-OUT callback while BOT state is UMS_BOT_STATE_SEND_CSW.
            pEP->bXferActive = FALSE;
        }
    }

    // Cancel BULK IN transaction (no effect if there isn't one).
    usb_hal_cancel_io_request((BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN));

    // For IN direction, always STALL endpoint if BOT state is DATA-IN.
    if (stall_active_endpt && (gBOT_state == UMS_BOT_STATE_DATA_IN))
    {
        ums_bot_stall(ENDPT_DIRECTION_IN);
    }

    return;
}


/*****************************************************************************
 * Function: ums_bot_valid_CBW
 *************************************************************************//**
 * This function determines whether a CBW is valid and meaningful.
 *
 * @param None.
 *                    
 * @retval TRUE if the CBW is valid and meaningful.
 * @retval FALSE otherwise.
 *
 ****************************************************************************** 
 */

inline BOOLEAN_T ums_bot_valid_CBW(void)
{
    BOOLEAN_T valid = FALSE;

    // Check for valid CBW. (correct CBW length and CBW signature)
    if ((gCBW_length == BOT_CBW_PACKET_LENGTH) && (gCBW->dSignature == BOT_CBW_SIGNATURE))
    {
        // Check for meaningful CBW. (LUN, CBWCB length, and flags)
        if ((gCBW->bLUN <= BOT_MAX_LUN) &&
            (gCBW->bCBLength <= BOT_CBWCB_MAX_LENGTH) && (gCBW->bCBLength > 0) &&
            ((gCBW->bmFlags == ENDPT_DIRECTION_IN) || (gCBW->bmFlags == ENDPT_DIRECTION_OUT)))
        {
            valid = TRUE;
        }
    }

    INFO("-> ums_bot_valid_CBW() - %s.\n", (valid) ? "TRUE" : "FALSE");

    return valid;   
}

/*****************************************************************************
 * Function: ums_bot_send_CSW
 *************************************************************************//**
 * This function prepares and sends a CSW.  It also sets the
 * BOT state based on the CSW status.
 *
 * @param[in] csw_status status (passed, failed, or phase error).
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_send_CSW(UINT8_T csw_status)
{
    // Return if the BOT state is not valid.
    if ((gBOT_state == UMS_BOT_STATE_SEND_CSW) || (gBOT_state == UMS_BOT_STATE_ERROR))
    {
        CRIT("@Error: ums_bot_send_CSW() - gBOT_state = %u.\n", gBOT_state);
        return;
    }

    if ((csw_status == CSW_CMD_FAILED) || (csw_status == CSW_PHASE_ERROR))
    {
        CRIT("-> ums_bot_send_CSW() - status = %s. CDB[0] = 0x%02x.\n", (csw_status == CSW_CMD_FAILED) ? "FAILED" : "PHASE_ERROR", gCBW->CB[0]);
    }
    else
    {
        DEBUG("-> ums_bot_send_CSW() - status = PASSED.\n");
    }

    // Set CSW signature.
    gCSW->dSignature = BOT_CSW_SIGNATURE;

    TI_PRELIM_ASIC_DPRAM_ACCESS_BUG();

    // Copy CBW Tag to CSW.
    gCSW->dTag = gCBW->dTag;

    // Set CSW status.
    gCSW->bStatus = (gCheckCondition) ? CSW_CMD_FAILED : csw_status;

    if ((gBOT_case == 5) || (gBOT_case == 11))
    {
        // For BOT Case (5) or (11), set CSW data residue equal to the CBW xfer length minus the actual xfer length.
        gCSW->dDataResidue = gCBW->dDataTransferLength - gActualXferLength;
    }
    else if (gBOT_case == 9)
    {
        // For BOT Case (9), set CSW data residue equal to the CBW xfer length.
        gCSW->dDataResidue = gCBW->dDataTransferLength;
    }

    if (csw_status == CSW_PHASE_ERROR)
    {
        // Set BOT error state.
        gBOT_state = UMS_BOT_STATE_ERROR;
    }
    else
    {
        // Go to Send CSW state.
        gBOT_state = UMS_BOT_STATE_SEND_CSW;
    }

    // Send CSW.
    ums_bot_tx(gCSW, BOT_CSW_PACKET_LENGTH);

    return;
}


/*****************************************************************************
 * Function: ums_bot_case_handler
 *************************************************************************//**
 * This function stalls the appropriate endpoint if needed and sends
 * the appropriate CSW for the 13 cases excluding the thin diagonal cases (1,6,12).
 *
 * @param[in] case_num case number [1-13].
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_case_handler(UINT32_T case_num)
{
    CRIT("-> ums_bot_case_handler() - case %u.\n", case_num);

    switch (case_num)
    {
        case 1:
        case 6:
        case 12:
            // Case (1), (6), and (12). Normal cases. No action required.
            break;

        case 2:
        case 3:
            // Case (2).  Host expects no data. Device intends to send data.
            // Case (3).  Host expects no data. Device intends to receive data.
            ums_bot_stall(ENDPT_DIRECTION_IN);        
            ums_bot_send_CSW(CSW_PHASE_ERROR);
            break;

        case 4:
        case 5:
            // Case (4).  Host expects to receive data. Device intends to transfer no data.
            // Case (5).  Device intends to send less data than host expects.
            ums_bot_stall(ENDPT_DIRECTION_IN);
            ums_bot_send_CSW(CSW_CMD_PASSED);  /* Passed because we have no SCSI sense data to report */
            break;

        case 7:
        case 8:
            // Case (7).  Device intends to send more data than host expects.
            // Case (8).  Host expects to receive data. Device intends to receive data.
            ums_bot_stall(ENDPT_DIRECTION_IN);
            ums_bot_send_CSW(CSW_PHASE_ERROR);
            break;

        case 9:
            // Case (9).  Host expects to send data. Device intends to transfer no data.
            // Accept all the data the host wants to send and discard.
            gBOT_case = 9;
            gBOT_state = UMS_BOT_STATE_DATA_OUT;  // BOT state needs to be changed to Data Out.

            // Setup transfer to Rx data which will be discarded.
            gActualXferLength = gCBW->dDataTransferLength;
            ums_bot_rx((void *)datapath_ram->bulk_ep_buffer, MIN(gActualXferLength, sizeof(datapath_ram->bulk_ep_buffer)));
            break;

        case 11:
            // Case (11).  Device intends to recieve less data than host expects to send. 
            // Accept all the data the host wants to send but only write the amount specified in the SCSI command to the SATA drive.
            gBOT_case = 11;
            break;

        case 10:
        case 13:
            // Case (10).  Host expects to send data. Device intends to send data. 
            // Case (13).  Device intends to recieve more data than host expects to send.
            ums_bot_stall(ENDPT_DIRECTION_OUT);
            ums_bot_send_CSW(CSW_PHASE_ERROR);
            break;

        default:
            break;
    }

    return;
}

/*****************************************************************************
 * Function: ums_bot_determine_state
 *************************************************************************//**
 * This function sets the BOT state based on the SCSI command
 * in the CBWCB.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */
inline void ums_bot_determine_state(void)
{
    ENDPT_DIR_T direction;
    STATUS_T status;

    if (ata_dev[gCBW->bLUN].bPacketDevice)
    {
        // Determine state from CBW info.
        if (gCBW->dDataTransferLength == 0)
        {
            gBOT_state = UMS_BOT_STATE_CSW_PENDING;
        }
        else
        {
            gBOT_state = (gCBW->bmFlags == ENDPT_DIRECTION_OUT) ? UMS_BOT_STATE_DATA_OUT : UMS_BOT_STATE_DATA_IN;
        }
    }
    else /* ATA device */
    {
        // This is needed to pass USB-IF CV MSC test.
        status = scsi_get_data_direction(gCBW->CB, &direction);

        if (status == STATUS_OK)
        {
            gBOT_state = (direction == ENDPT_DIRECTION_OUT) ? UMS_BOT_STATE_DATA_OUT : UMS_BOT_STATE_DATA_IN;           
        }
        else if (status == STATUS_NO_DATA)
        {
            gBOT_state = UMS_BOT_STATE_CSW_PENDING;
        }
        else /* SCSI command is unknown so determine state from CBW flags */
        {
            CRIT("@Warning: ums_bot_determine_state() - SCSI cmd 0x%x handled in default case.\n", gCBW->CB[0]);

            // Determine state from CBW info.
            if (gCBW->dDataTransferLength == 0)
            {
                gBOT_state = UMS_BOT_STATE_CSW_PENDING;
            }
            else
            {
                gBOT_state = (gCBW->bmFlags == ENDPT_DIRECTION_OUT) ? UMS_BOT_STATE_DATA_OUT : UMS_BOT_STATE_DATA_IN;
            }

            INFO("SCSI cmd 0x%x is %s.\n", gCBW->CB[0], (gBOT_state == UMS_BOT_STATE_DATA_IN) ? "DATA IN" :
                 (gBOT_state == UMS_BOT_STATE_DATA_OUT) ? "DATA OUT" : "NO DATA");      
        }
    }

#if 0
    // BQ - temp debug.
    if (gCBW->dDataTransferLength > 0)
    {
        if (((gBOT_state == UMS_BOT_STATE_DATA_OUT) && (gCBW->bmFlags != ENDPT_DIRECTION_OUT)) ||
            ((gBOT_state == UMS_BOT_STATE_DATA_IN) && (gCBW->bmFlags != ENDPT_DIRECTION_IN))/* ||
            (((gBOT_state == UMS_BOT_STATE_DATA_OUT) || (gBOT_state == UMS_BOT_STATE_DATA_IN))  && (gCBW->dDataTransferLength == 0))*/)
        {
            CRIT("Direction mismatch. SCSI cmd 0x%x should be data %s.  gBOT_state = %u.\n", gCBW->CB[0], (gCBW->bmFlags == ENDPT_DIRECTION_IN) ? "IN" :
                 (gCBW->bmFlags == ENDPT_DIRECTION_OUT) ? "OUT" : (gCBW->dDataTransferLength == 0) ? "NO DATA" : "?",
                 gBOT_state);      
        }
    }
#endif

    DEBUG("-> ums_bot_determine_state() - BOT_state = %u.\n", gBOT_state);
    return;
}

/*****************************************************************************
 * Function: ums_bot_validate_case
 *************************************************************************//**
 * This function determines whether the CBW information is consistent 
 * with the BOT state determined from the SCSI cmd.
 *
 * @param None.
 *
 * @retval TRUE if the CBW information is consistent with the BOT state.
 * @retval FALSE otherwise.
 *
 ****************************************************************************** 
 */

inline BOOLEAN_T ums_bot_validate_case(void)
{
    BOOLEAN_T bValid = FALSE;

    INFO("-> ums_bot_validate_case()\n");

    if (ata_dev[gCBW->bLUN].bPacketDevice)
    {
        // BQ Workaround - Seems like Windows likes to send invalid cmds for ATAPI devices.
        return TRUE;
    }

    if (gCBW->dDataTransferLength == 0) /* Host expects no data */
    {
        if (gBOT_state == UMS_BOT_STATE_DATA_IN)
        {
            // Case (2).  Device intends to send data.
            ums_bot_case_handler(2);
        }
        else if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
        {
            // Case (3).  Device intends to receive data.
            ums_bot_case_handler(3);
        }
        else
        {
            // Case (1). Device agrees there is no data phase.
            bValid = TRUE;
        }
    }
    else /* Host expects data */
    {
        if (gCBW->bmFlags == ENDPT_DIRECTION_IN) /* Host expects to recieve data */
        {
            if (gBOT_state == UMS_BOT_STATE_DATA_IN)
            {
                // Case (6) likely.  Device intends to send data, but amount is not known until SCSI cmd is processed. 
                bValid = TRUE;
            }
            else if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
            {
                // Case (8).  Device intends to receive data.
                ums_bot_case_handler(8);
            }
            else
            {
                // Case (4).  Device intends to transfer no data.
                ums_bot_case_handler(4);
            }
        }
        else /* Host expects to send data */
        {
            if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
            {
                // Case (12) likely.  Device intends to receive data, but amount is not known until SCSI cmd is processed. 
                bValid = TRUE;
            }
            else if (gBOT_state == UMS_BOT_STATE_DATA_IN)
            {
                // Case (10).  Device intends to send data. 
                ums_bot_case_handler(10);
            }
            else
            {
                // Case (9).  Device intends to transfer no data.
                ums_bot_case_handler(9);
            }
        }
    }

    return bValid;
}

/*****************************************************************************
 * Function: ums_bot_issue_scsi_cmd
 *************************************************************************//**
 * This function calls the SCSI command handler and processes the result.
 *
 * @param None.
 *
 * @return The status from scsi_command_handler().
 *
 ****************************************************************************** 
 */

STATUS_T ums_bot_issue_scsi_cmd(void)
{
    STATUS_T status;

    // Send command to SCSI module.
    status = scsi_command_handler(&ums_cmd);

    if (status == STATUS_SCSI_RESPONSE_PENDING)
    {
        // Store SCSI command with pending response.
        gSCSI_cmd_pending = gCBW->CB[0];
    }
    else if (status == STATUS_SCSI_RESPONSE_READY)
    {
        // Get the transfer length.
        gActualXferLength = ums_cmd.dDataByteCnt;

        if (gActualXferLength < gCBW->dDataTransferLength)
        {
            // Case (5).  Device intends to send less data than host expects.
            gBOT_case = 5;
        }
        else if (gActualXferLength > gCBW->dDataTransferLength)
        {
            // BOT Case (7).  Device intends to send more data than host expects.
            gBOT_case = 7;
            gActualXferLength = gCBW->dDataTransferLength;
        }

        // Set SATA xfer complete flag to TRUE so CSW will be sent in ums_bot_data_xfer_callback_IN().
        gSATA_xfer_cmplt = TRUE;

        // TX response data.
        ums_bot_tx((void *)&datapath_ram->scsi_response_buffer[0],
                   MIN(gActualXferLength, SCSI_RESPONSE_BUFF_SIZE));
    }
    else /* Error */
    {
        if (status != STATUS_OK)
        {
            // Cancel any active xfers and stall appropriate endpoint.
            ums_bot_xfer_cleanup(TRUE);
        }

        if (status != STATUS_SCSI_ATA_ERROR)
        {
            // Send CSW with appropriate status.
            ums_bot_send_CSW((status == STATUS_OK) ? CSW_CMD_PASSED : CSW_CMD_FAILED);
        }
    }

    return status;
}

/*****************************************************************************
 * Function: ums_bot_ata_error_callback
 *************************************************************************//**
 * This callback is called by the AHCI block when a fatal error
 * occurs.
 *
 * @param[in] pAtaCmdCbkData pointer to callback data. This parameter is not 
 * used and may be NULL.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_ata_error_callback(ATA_CMD_CALLBACK_T *pAtaCmdCbkData)
{
    CRIT("-> ums_bot_ata_error_callback() - gBOT_state = %u.\n", gBOT_state);

    // Validate BOT state.
    if ((gBOT_state == UMS_BOT_STATE_DATA_IN) || 
        (gBOT_state == UMS_BOT_STATE_DATA_OUT) || 
        (gBOT_state == UMS_BOT_STATE_CSW_PENDING))
    {
        // Cancel any active xfers and stall appropriate endpoint.
        ums_bot_xfer_cleanup(TRUE);

        if (scsi_is_rw_cmd(gCBW->CB[0]))
        {
            // Reset wrap window R/W offsets.
            mww_reset_rw_offsets();
        }

        // Send error CSW.
        ums_bot_send_CSW(CSW_CMD_FAILED);
    }
    else
    {
        CRIT("@Warning: ums_bot_ata_error_callback() - gBOT_state = %u. CDB[0] = 0x%02x.\n", gBOT_state, gCBW->CB[0]);
    }

    return;
}


/*****************************************************************************
 * Function: ums_bot_ata_cmd_callback
 *************************************************************************//**
 * This callback is called by the AHCI block when a D2H register
 * FIS is received.
 *
 * @param[in] pAtaCmdCbkData pointer to callback data structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_ata_cmd_callback(ATA_CMD_CALLBACK_T *pAtaCmdCbkData)
{
    DEBUG("-> ums_bot_ata_cmd_callback() - cmd_slot = %u, SCSI_cmd_pending = 0x%02x, ata_status = 0x%x, BOT_state = %u, gActualXferLength = %u.\n", 
          pAtaCmdCbkData->bCmdSlot, gSCSI_cmd_pending, pAtaCmdCbkData->bStatus, gBOT_state, gActualXferLength);

    if (gActualXferLength == 0)
    {
        ums_bot_send_CSW((pAtaCmdCbkData->bStatus & ATA_ERROR_STATUS_BIT) ? CSW_CMD_FAILED : CSW_CMD_PASSED); 
    }
    else
    {
        // Verify BOT state is a data state.
        if ((gBOT_state == UMS_BOT_STATE_DATA_IN) || (gBOT_state == UMS_BOT_STATE_DATA_OUT))
        {
            if (pAtaCmdCbkData->bStatus & ATA_ERROR_STATUS_BIT)
            {
                CRIT("@Error: ums_bot_ata_cmd_callback() - ATA error detected!\n");

                // Call error handler.
                ums_bot_ata_error_callback(NULL);
            }
            else /* ERROR status bit not set */
            {
#if DISABLE_WRAP_WINDOW
                // Set SATA xfer complete flag.
                gSATA_xfer_cmplt = TRUE;

                // Setup USB transfer request.
                if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
                {
                    if ((gBOT_case != 11) && (gBOT_case != 13))
                    {
                        // Case (12).  Device received exact amount of data the host intended to send.
                        ums_bot_send_CSW(CSW_CMD_PASSED);
                    }
                }
                else if (gBOT_state == UMS_BOT_STATE_DATA_IN)
                {
                    ums_bot_tx((void *)datapath_ram->normal_data_buffer, gActualXferLength);
                }
#else
                if ((gBOT_state == UMS_BOT_STATE_DATA_IN) &&
                    (scsi_is_rw_cmd(gSCSI_cmd_pending) == FALSE))
                {
                    // Send data without using wrap window.
                    ums_bot_tx((void *)datapath_ram->normal_data_buffer,
                               MIN(pAtaCmdCbkData->dDataByteCnt, gCBW->dDataTransferLength));
                }

                if (gUSB_xfer_cmplt)
                {
                    // Case (6) or (12).  Device sent or received exact amount of data host expected.
                    // Case (11).  Device intends to recieve less data than host expects to send.
                    ums_bot_send_CSW(CSW_CMD_PASSED);
                }
                else
                {
                    // Set SATA xfer complete flag.
                    gSATA_xfer_cmplt = TRUE;
                }

#endif
            } /* END: ERROR status bit not set */
        } /* END: Verify BOT state is a data state.*/
    } /* END: xfer length != 0 */

    return;
}


/*****************************************************************************
 * Function: ums_bot_idle
 *************************************************************************//**
 * This function sets the BOT state to IDLE and sends an I/O 
 * request to the USB HAL to recieve a CBW.
 *
 * @param lun logical unit number.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_idle(UINT32_T lun)
{
    DEBUG("-> ums_bot_idle()\n");

    // Set idle state.
    gBOT_state = UMS_BOT_STATE_IDLE;

    // Reset error case flag.
    gBOT_case = 0;

    // Reset transfer flags.
    gUSB_xfer_cmplt = FALSE; 
    gSATA_xfer_cmplt = FALSE;

    // Reset check condition flag.
    gCheckCondition = FALSE;

    // Prepare to Rx CBW.
    ums_bot_rx(gCBW, BOT_CBW_PACKET_LENGTH);

    return;
}

/*****************************************************************************
 * Function: ums_bot_reset
 *************************************************************************//**
 * This function resets the BOT module by registering ATA callbacks, cancelling
 * active transfers, and returning to BOT IDLE state.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_reset(void)
{
    DEBUG("-> ums_bot_reset().\n");

    // Register callbacks with AHCI module.
    ahci_register_ata_callbacks(ums_bot_ata_cmd_callback, NULL, ums_bot_ata_error_callback);
#if REMOVABLE_MEDIA_DEVICE
    /* The removable LUN stays available while SATA discovery runs. A later
     * insertion completion must not reset DATA/CSW state or queue a second
     * CBW over the host command that is already in progress. */
    ahci_register_port_init_complete_callback(0, NULL);
#else
    ahci_register_port_init_complete_callback(0, ums_bot_idle);
#endif

    // Cancel any active transfers but don't stall any endpoints.
    ums_bot_xfer_cleanup(FALSE);

    // Clear persistent stall.
    usb_dev.bBOT_PersistentStall = FALSE;

    // Set the command slot to zero (since we cannot use NCQ for BOT).
    ums_cmd.bCmdSlotNum = 0;

#if REMOVABLE_MEDIA_DEVICE
    /* Inquiry and readiness queries work even during pending discovery;
     * individual SCSI commands enforce the current media-ready state. */
    ums_bot_idle(0);
#else
    if (ata_dev[0].bDeviceInitComplete || ata_dev[0].bDeviceInitTimedOut)
    {
        // Go to IDLE state.
        ums_bot_idle(0);
    }
#endif

    return;
}


/*****************************************************************************
 * Function: ums_bot_process_CBW
 *************************************************************************//**
 * This function processes an incoming CBW.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ums_bot_process_CBW(void)
{
    BOOLEAN_T issue_scsi_cmd = TRUE;

    DEBUG("-> ums_bot_process_CBW() - xfer_length = %u bytes, cmd = 0x%02x, cmd_length = %u bytes.\n", 
          gCBW->dDataTransferLength, gCBW->CB[0], gCBW->bCBLength);

    // Set the CSW data residue to the CBW xfer length.
    gCSW->dDataResidue = gCBW->dDataTransferLength;

    // Set BOT state based on SCSI command.
    ums_bot_determine_state();

    // Proceed to setup transfer and process SCSI command if BOT case is valid.
    if (ums_bot_validate_case())
    {
        // Process SCSI command.   
        ums_cmd.bLUN = gCBW->bLUN;
        ums_cmd.pCommandBlock = gCBW->CB;
        ums_cmd.bCmdBlkLength = gCBW->bCBLength;
        ums_cmd.dDataXferLength = gCBW->dDataTransferLength;

        gActualXferLength = gCBW->dDataTransferLength;

        if (scsi_is_rw_cmd(gCBW->CB[0]))
        {
            // Audio-CD recording have variable sector sizes so we cannot calculate the transfer length
            // from the SCSI command block.
            if (!ata_dev[gCBW->bLUN].bPacketDevice)
            {
                // Extract transfer length from SCSI command block.
                scsi_get_rw_xfer_length(gCBW->CB, gCBW->bLUN, &gActualXferLength);
            }

#if DISABLE_WRAP_WINDOW
            if (gCBW->dDataTransferLength > sizeof(datapath_ram->normal_data_buffer))
            {
                CRIT("@Error: Xfer length > %u bytes %s not supported w/o wrap window memory!  (length = %u bytes) ######.\n", 
                     sizeof(datapath_ram->normal_data_buffer), scsi_is_write_cmd(gCBW->CB[0]) ? "WRITE" : "READ", gCBW->dDataTransferLength);
                gActualXferLength = sizeof(datapath_ram->normal_data_buffer);
            }
#endif        

            // Setup USB transfer request.
            if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
            {
                if (gActualXferLength == 0)
                {
                    // Case (9).  Device intends to transfer no data.
                    ums_bot_case_handler(9);
                }
                else if (gActualXferLength < gCBW->dDataTransferLength)
                {
                    // Case (11).  Device intends to recieve less data than host expects to send.
                    ums_bot_case_handler(11);
                }
                else if (gActualXferLength > gCBW->dDataTransferLength)
                {
                    // Case (13).  Device intends to recieve more data than host expects to send.
                    gBOT_case = 13;
                    gActualXferLength = gCBW->dDataTransferLength;                   
                }

#if DISABLE_WRAP_WINDOW
                ums_bot_rx((void *)datapath_ram->normal_data_buffer,
                           MIN(gActualXferLength, sizeof(datapath_ram->normal_data_buffer)));
#else
                // Setup memory wrap window for write-only use.
                mww_init_write_only();

                // Send the write command to the SATA device.
                if (ums_bot_issue_scsi_cmd() == STATUS_SCSI_RESPONSE_PENDING)
                {
                    // Issue USB Rx request.
                    ums_bot_rx((void *)USB_TO_SATA_WRAP_WINDOW_ADDR, gActualXferLength);  
                }
                else
                {
                    ums_bot_stall(ENDPT_DIRECTION_OUT);
                }
#endif
            }
            else if (gBOT_state == UMS_BOT_STATE_DATA_IN)
            {
                if (gActualXferLength == 0)
                {
                    // Case (4).  Host expects to receive data. Device intends to transfer no data.
                    ums_bot_case_handler(4);
                }
                else if (gActualXferLength < gCBW->dDataTransferLength)
                {
                    // Case (5).  Device intends to send less data than host expects.
                    gBOT_case = 5;
                }
                else if (gActualXferLength > gCBW->dDataTransferLength)
                {
                    // BOT Case (7).  Device intends to send more data than host expects.
                    gBOT_case = 7;
                    gActualXferLength = gCBW->dDataTransferLength;
                }

                if (gActualXferLength != 0)
                {

#if DISABLE_WRAP_WINDOW

                    ums_bot_issue_scsi_cmd();
#else              
                   
                    // Setup memory wrap window for read-only use.
                    mww_init_read_only();
                   
                    // Send the read command to the SATA device.
                    if (ums_bot_issue_scsi_cmd() == STATUS_SCSI_RESPONSE_PENDING)
                    {
                        // Issue USB Tx request.
                        ums_bot_tx((void *)SATA_TO_USB_WRAP_WINDOW_ADDR, gActualXferLength);
                    }
#endif
                }

            } /* END: gBOT_state == UMS_BOT_STATE_DATA_IN */
        }
        else /* non-RW command */
        {
            // Set check condition flag in case of ATA PASS-THROUGH command.
            gCheckCondition = scsi_is_check_condition_set(&gCBW->CB[0]);

            // Don't use wrap window for non-RW cmds because xfers are not always DWORD multiples.
            if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
            {
                // Assume xfer length is not greater than window memory size.
                ums_bot_rx((void *)datapath_ram->normal_data_buffer,
                           MIN(gActualXferLength, sizeof(datapath_ram->normal_data_buffer)));

//                // Set pointer to data.
//                ums_cmd.pData = (void *)datapath_ram->normal_data_buffer;

                // Issue SCSI cmd only if there is no data to be Rx'd.
                if (gActualXferLength > 0)
                {
                    issue_scsi_cmd = FALSE;
                }
            }

            if ((gBOT_state == UMS_BOT_STATE_DATA_IN) &&
                (gActualXferLength > sizeof(datapath_ram->normal_data_buffer)))
            {
                CRIT("@ERROR: xfer length is > 64 KB for non-wrap window xfer.\n");

                issue_scsi_cmd = FALSE;

//                // Cancel any active xfers and stall appropriate endpoint.
//                ums_bot_xfer_cleanup(TRUE);

                // Stall IN endpoint.
                ums_bot_stall(ENDPT_DIRECTION_IN);

                // Send CSW with failed status.
                ums_bot_send_CSW(CSW_CMD_FAILED);
            }

            if (issue_scsi_cmd)
            {
                ums_bot_issue_scsi_cmd();              
            }

        } /* End non-RW command */
    } /* End if (ums_bot_validate_case) */

    return;
}

/*****************************************************************************
 * Function: ums_bot_data_xfer_callback_OUT
 *************************************************************************//**
 * This function called by USB stack when it receives transfer 
 * complete event on the BOT bulk OUT endpoint.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_data_xfer_callback_OUT(EP_INFO_T *ep_info)
{
    DEBUG("-> ums_bot_data_xfer_callback_OUT() - byte_cnt = %u, BOT_state = %u.\n", ep_info->dByteCount, gBOT_state);

    if (gBOT_state == UMS_BOT_STATE_IDLE)
    {
        // Get CBW length.
        gCBW_length = ep_info->dByteCount;

        // Check for valid & meaningful CBW.
        if (ums_bot_valid_CBW())
        {
            // Process the CBW.
            ums_bot_process_CBW();
        }
        else /* Invalid CBW */
        {
            CRIT("@Error: Invalid CBW! Setting persistent STALL on BOT endpts. CBW ptr = 0x%08x.\n", (UINT32_T)gCBW);
            if (gCBW_length != BOT_CBW_PACKET_LENGTH) CRIT("CBW length: %u bytes. (Should be %u bytes.)\n", gCBW_length, BOT_CBW_PACKET_LENGTH);
            if (gCBW->dSignature != BOT_CBW_SIGNATURE) CRIT("CBW signature: 0x%x. (Should be 0x%x)\n", gCBW->dSignature, BOT_CBW_SIGNATURE);
            if (gCBW->bLUN > BOT_MAX_LUN) CRIT("CBW LUN: %u. (Should be <= %u)\n", gCBW->bLUN, BOT_MAX_LUN);
            if (gCBW->bCBLength > BOT_CBWCB_MAX_LENGTH) CRIT("CBW CB length: %u bytes. (Should be <= %u bytes)\n", gCBW->bCBLength, BOT_CBWCB_MAX_LENGTH);
            if ((gCBW->bmFlags != ENDPT_DIRECTION_IN) && (gCBW->bmFlags != ENDPT_DIRECTION_OUT)) CRIT("CBW Flags: 0x%02x.  (Should be 0x00 or 0x80)\n", gCBW->bmFlags);

            // Stall both BOT endpts until host performs reset recovery.
            ums_bot_stall(ENDPT_DIRECTION_IN);
            ums_bot_stall(ENDPT_DIRECTION_OUT);

            // Set flag to maintain persistent stall until reset recovery.  Do not send CSW.
            usb_dev.bBOT_PersistentStall = TRUE;
        }
    }
    else if (gBOT_state == UMS_BOT_STATE_DATA_OUT)
    {
        // Update CSW data residue.
        gCSW->dDataResidue -= ep_info->dByteCount;

        TI_PRELIM_ASIC_DPRAM_ACCESS_BUG();

        if (gCSW->dDataResidue == 0)
        {

#if DISABLE_WRAP_WINDOW
            if (scsi_command_handler(&ums_cmd) == STATUS_SCSI_RESPONSE_PENDING)
            {
                // Store SCSI command with pending response.
                gSCSI_cmd_pending = gCBW->CB[0];
                INFO("DUMMY - gSCSI_cmd_pending[%u] = 0x%02x.\n", ums_cmd.bCmdSlotNum, gCBW->CB[0]);
            }

            if (gBOT_case == 13)
            {
                // Case (13).  Device intends to recieve more data than host expects to send.
                ums_bot_case_handler(13);
            }
            else if ((gBOT_case == 9) || (gBOT_case == 11))
            {
                ums_bot_send_CSW(CSW_CMD_PASSED);
            }
#else
            if (gBOT_case == 13)
            {
                // Case (13).  Device intends to recieve more data than host expects to send.
                ums_bot_case_handler(13);
            }
            else if (gBOT_case == 9)
            {
                // Case (9).  Host expects to send data. Device intends to transfer no data.
                // Device has accepted all of the data the host expected to send.
                ums_bot_send_CSW(CSW_CMD_PASSED);
            }
            else
            {
                if (!scsi_is_rw_cmd(gCBW->CB[0]))
                {
                    // Set data xfer length to the number of bytes received over USB.
                    ums_cmd.dDataXferLength = ep_info->dByteCount;
                    ums_bot_issue_scsi_cmd();
                }

                if (gSATA_xfer_cmplt)
                {
                    // Case (12).  Device received exact amount of data the host intended to send.
                    ums_bot_send_CSW(CSW_CMD_PASSED);   
                }
                else
                {
                    // Set USB xfer complete flag.
                    gUSB_xfer_cmplt = TRUE;
                }
            }
#endif
        }
        else  /* More data is coming. */
        {
            // Setup transfer to Rx data which will be discarded.
            ums_bot_rx((void *)datapath_ram->bulk_ep_buffer, 
                       MIN(sizeof(datapath_ram->bulk_ep_buffer), gCSW->dDataResidue));
        }
    }
    else
    {
        // This should never happen.  
        CRIT("@Error: ums_bot_data_callback_OUT() - Invalid BOT state = %u.\n", gBOT_state);
        ums_bot_stall(ENDPT_DIRECTION_IN);
        ums_bot_stall(ENDPT_DIRECTION_OUT);
        ums_bot_send_CSW(CSW_PHASE_ERROR);
    }

    return;
}

/*****************************************************************************
 * Function: ums_bot_data_xfer_callback_IN
 *************************************************************************//**
 * This function called by USB stack when it receives transfer 
 * complete event on the BOT bulk IN endpoint.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_data_xfer_callback_IN(EP_INFO_T *ep_info)
{
    DEBUG("-> ums_bot_data_xfer_callback_IN() - byte_cnt = %u, BOT_state = %u.\n", ep_info->dByteCount, gBOT_state);

    if ((gBOT_state == UMS_BOT_STATE_SEND_CSW) || (gBOT_state == UMS_BOT_STATE_ERROR))
    {
        // Return to BOT IDLE state.
        ums_bot_idle(0);
    }
    else if (gBOT_state == UMS_BOT_STATE_DATA_IN)
    {
        // Update CSW data residue.
        gCSW->dDataResidue -= ep_info->dByteCount;

        TI_PRELIM_ASIC_DPRAM_ACCESS_BUG();

        if (gCSW->dDataResidue == 0)
        {
            if (gBOT_case == 7)
            {
                // Case (7).  Device intends to send more data than host expects.
                ums_bot_case_handler(7);
            }
            else
            {
                if (gSATA_xfer_cmplt)
                {
                    // Case (6).  Device sent exact amount of data host expected.
                    ums_bot_send_CSW(CSW_CMD_PASSED);
                }
                else
                {
                    // Set USB xfer complete flag.
                    gUSB_xfer_cmplt = TRUE;
                }
            }
        }
        else
        {
            DEBUG("gCSW->dDataResidue = %u bytes.\n", gCSW->dDataResidue);
            // Case (5).  Device sent less data than host expected.
            ums_bot_case_handler(5);
        }
    }
    else
    {
        // This should never happen.
        CRIT("@Error: ums_bot_data_callback_IN() - Invalid BOT state = %u.\n", gBOT_state);
        ums_bot_stall(ENDPT_DIRECTION_IN);
        ums_bot_stall(ENDPT_DIRECTION_OUT);
        ums_bot_send_CSW(CSW_PHASE_ERROR);
    }

    return;
}


/*****************************************************************************
 * Function: ums_bot_class_request_callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle 
 * BOT class requests (Mass Storage Reset and Get Max LUN).
 *
 * @param[in] setup_packet pointer to setup packet.
 *
 * @retval STATUS_OK when succesful.
 * @retval STATUS_ERROR when request is not handled.
 *
 ****************************************************************************** 
 */

STATUS_T ums_bot_class_request_callback(USB_SETUP_PACKET_T* setup_packet)
{
    STATUS_T status = STATUS_ERROR;

    DEBUG("-> ums_bot_class_request_callback()\n");

    if ((setup_packet->bmRequestType & USB_REQ_TYPE_DIR_MASK) == USB_REQ_TYPE_HOST_TO_DEVICE)
    {
        if ((setup_packet->bRequest == BOT_MASS_STORAGE_RESET_REQUEST) &&
            (setup_packet->wValue == 0x00) &&
            (setup_packet->wLength == 0x00) &&
            (setup_packet->wIndex == BOT_INTERFACE_NUM))
        {
            /* Mass Storage Reset Request */
            CRIT("BOT Reset.\n");

            // Recover SATA.
            ahci_reset_lun(0, FALSE);

            // Reset BOT.
            ums_bot_reset();

            status = STATUS_OK;
        }
    }
    else if ((setup_packet->bmRequestType & USB_REQ_TYPE_DIR_MASK) == USB_REQ_TYPE_DEVICE_TO_HOST)
    {
        if ((setup_packet->bRequest == BOT_GET_MAX_LUN_REQUEST) &&
            (setup_packet->wValue == 0x00) &&
            (setup_packet->wLength == 0x01) &&
            (setup_packet->wIndex == BOT_INTERFACE_NUM))
        {
            /* Get Max LUN Request */
            DEBUG("Get Max LUN.\n");

            // Set EP0 state to Data IN.
            usb_dev.ep0_state = EP0_STATE_DATA_IN;

            /* TX Data (1 byte) */
            datapath_ram->ep0_buffer[0] = BOT_MAX_LUN;
            usb_hal_ep0_io_request(ENDPT_DIRECTION_IN, (void *)datapath_ram->ep0_buffer, 0x1);

            status = STATUS_OK;
        }
    }
    else
    {
        DEBUG("-> ums_bot_class_request_callback() - No matching request.\n");
    }

    return status;
}

/*****************************************************************************
 * Function: ums_bot_init
 *************************************************************************//**
 * This function initializes the Bulk-Only Transport layer.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_bot_init(void)
{
    DEBUG("-> ums_bot_init()\n");

    // Init persistent stall.
    usb_dev.bBOT_PersistentStall = FALSE;

    // Register data xfer callbacks.
    usb_stack_register_BOT_data_xfer_callback((BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), ums_bot_data_xfer_callback_IN);
    usb_stack_register_BOT_data_xfer_callback((BOT_BULK_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT), ums_bot_data_xfer_callback_OUT);

    // Register class request callback.
    usb_stack_register_request_callback((USB_TYPE_CLASS | USB_RECIP_INTERFACE), ums_bot_class_request_callback);

    // Set pointers to command and status buffers.
    gCBW = (UMS_BOT_CBW_T *)&datapath_ram->ums_cmd_buffer[0];
    gCSW = (UMS_BOT_CSW_T *)&datapath_ram->ums_status_buffer[0];

    // Register BOT reset callback.
    usb_stack_register_BOT_reset_callback(ums_bot_reset);

    return;
}



