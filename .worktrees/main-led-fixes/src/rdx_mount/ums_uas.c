/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ums_uas.c
//             
// Project     : TUSB926x Firmware.
//             
// Description : USB Mass Storage USB Attached SCSI module. 
//               Compliant to UASP Spec Revision 1.0 (June 24, 2009).
//               and UAS Spec Revision 4. (March 9, 2010).
// 
// 
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   09/28/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the USB Mass Storage (UMS) Class USB Attached SCSI (UAS) layer.
 *
 * The UMS UAS layer is compliant with the following specifications:
 * - USB Attached SCSI (UAS) [T10/2095-D], Revision 4 (March 9, 2010).
 * - USB Mass Storage Class USB Attached SCSI Protocol (UASP), Revision 1.0  (June 24, 2009).
 *
 * Note: Multiple LUNs and Task Attributes are not supported. 
 */

#include "ums_uas.h"
#include "ahci.h"
#include "gio.h"
#include "reg_io.h"
#include "sci.h"
#include "scsi.h"
#include "string.h"
#include "system.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "usb_hal.h"
#include "usb_stack.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#if UAS_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif

#if 0
    #define DUMP_UAS_STATE()   ums_uas_dump_state()
#else
    #define DUMP_UAS_STATE()   {}
#endif

#define TAG_HASH_INDEX_MASK    0x0000003F  /* lower 6-bits */
#define TAG_HASH_TABLE_SIZE    64          /* 2^(# of bits in hash index mask) */

#define CMD_INDEX_LIST_MASK    0x0000003F  /* lower 6-bits */
#define CMD_INDEX_LIST_SIZE    64          /* 2^(# of bits in hash index mask) Must be at least 2x NCQ depth */

#define DUMMY_SCSI_OPCODE      0x01  /* This is an obsolete SCSI opcode */

/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

INT32_T gCmdWaitingCount;
BOOLEAN_T gNonQueuedCmdInProgress;

UMS_UAS_CMD_RESPONSE_ORDER_T gResponse_order;

UMS_UAS_STATE_T gUAS_state[UMS_UAS_CMD_DEPTH];  /* indexed by command index */

UINT8_T gLUN[AHCI_NCQ_DEPTH];   /* indexed by command index */

BOOLEAN_T gCheckCond[UMS_UAS_CMD_DEPTH];

BOOLEAN_T gRW_Ready_locked;

UMS_UAS_PENDING_DATA_IN_LIST_T gPendingData_IN;
UMS_UAS_PENDING_DATA_OUT_INFO_T gPendingData_OUT;

UINT32_T gCurrentDataOutIndex;
UINT32_T gCurrentDataInIndex;

UAS_COMMAND_IU_T *gCommand_IU[UMS_UAS_CMD_DEPTH];  /* indexed by command index */

INT32_T gTagUseCnt[TAG_HASH_TABLE_SIZE];  /* indexed by last 6-bits of TAG value */ 

BOOLEAN_T gSATA_xfer_cmpltd[UMS_UAS_CMD_DEPTH];     /* indexed by command index */

UINT32_T gCmdQueueDepth; /* 1-based */

UINT32_T gCurrent_cmd_buff_overflow_index;

SCSI_CMD_INPUT_T ums_uas_cmd;

UINT32_T gCurrent_cmd_buffer_index;
UINT32_T gCurrent_status_index;

INT32_T gCmd_list_input_index;
INT32_T gCmd_list_processing_index;
UINT32_T gCmd_index_list[CMD_INDEX_LIST_SIZE];

INT32_T gProcessing_index_decrement_count;

UMS_UAS_INDEX_LIST_T gFree_cmd_buffer_list;
UMS_UAS_INDEX_LIST_T gStatus_pending_list;
UMS_UAS_INDEX_LIST_T gResponse_pending_list;


STATUS_T ums_uas_send_rw_ready_iu(UINT32_T cmd_index);
void ums_uas_send_sense_iu(UINT32_T cmd_index, UINT8_T scsi_status);
void ums_uas_process_pending_cmds(void);

void ums_uas_dump_state(void)
{
    UINT32_T cmd_index;

    CRIT("[Index] UAS_state wTag CDB[0] gSATA_xfer_cmplt\n");
    for (cmd_index = 0; cmd_index < UMS_UAS_CMD_DEPTH; cmd_index++)
    {
        CRIT(" [%02u] %02u 0x%02x 0x%02x %s\n", cmd_index, gUAS_state[cmd_index], BSWAP_16(gCommand_IU[cmd_index]->wTAG), gCommand_IU[cmd_index]->CDB[0], gSATA_xfer_cmpltd[cmd_index] ? "Y" : "N");
//        CRIT(" UAS_state[%u] = %u, wTag = 0x%x, scsi_cmd = 0x%x, gSATA_xfer_cmpltd = %u.\n", cmd_index, gUAS_state[cmd_index], BSWAP_16(gCommand_IU[cmd_index]->wTAG), gCommand_IU[cmd_index]->CDB[0], gSATA_xfer_cmpltd[cmd_index]);
    }

    CRIT("gCmdWaitingCount = %u\n", gCmdWaitingCount);
    CRIT("gRW_Ready_locked = %u\n", gRW_Ready_locked);

    CRIT("P0SACT = 0x%08x\n", READ32(PxSACT(0)));
    CRIT("P0CI = 0x%08x\n", READ32(PxCI(0)));

    CRIT("gCurrentDataOutIndex = %u\n", gCurrentDataOutIndex);
    CRIT("gCurrentDataInIndex = %u\n", gCurrentDataInIndex);

//    CRIT(" gStatus_pending_list.processing_index = %u\n", gStatus_pending_list.processing_index);
//    CRIT(" gStatus_pending_list.index = %u\n", gStatus_pending_list.index);
//    CRIT(" gStatus_pending_list.cmd_index[%u] = %u\n", gStatus_pending_list.processing_index, gStatus_pending_list.cmd_index[gStatus_pending_list.processing_index]);
//    CRIT(" gStatus_pending_list.valid[%u] = %u\n", gStatus_pending_list.processing_index, gStatus_pending_list.valid_flag[gStatus_pending_list.processing_index]);

//    for (cmd_index = 0; cmd_index < CMD_INDEX_LIST_SIZE; cmd_index++)
//    {
//        CRIT("gCmd_index_list[%u] = %u\n", cmd_index, gCmd_index_list[cmd_index]);
//    }

    CRIT("gCmd_list_processing_index = %u\n", gCmd_list_processing_index); 
    CRIT("gCmd_index_list[%u] = %u\n", gCmd_list_processing_index, gCmd_index_list[gCmd_list_processing_index]);
    CRIT("gProcessing_index_decrement_count = %u\n", gProcessing_index_decrement_count);

    CRIT("gPendingData_OUT.xfer_pending = %u\n", gPendingData_OUT.xfer_pending);
    CRIT("gPendingData_OUT.cmd_index = %u\n", gPendingData_OUT.cmd_index);

//    for (cmd_index = 0; cmd_index < AHCI_NCQ_DEPTH; cmd_index++)
//    {
//        CRIT(" gPendingData_IN.xfer_pending[%u] = %u, cmd_index = %u\n", cmd_index, gPendingData_IN.xfer_pending[cmd_index], gPendingData_IN.cmd_index[cmd_index]);
//    }

//    CRIT("gPendingData_IN.xfer_pending[%u] = %u, cmd_index = %u.\n", gPendingData_IN.processing_index-1, gPendingData_IN.xfer_pending[gPendingData_IN.processing_index-1], gPendingData_IN.cmd_index[gPendingData_IN.processing_index-1]);
    CRIT("gPendingData_IN.xfer_pending[%u] = %u, cmd_index = %u.\n", gPendingData_IN.processing_index, gPendingData_IN.xfer_pending[gPendingData_IN.processing_index], gPendingData_IN.cmd_index[gPendingData_IN.processing_index]);
    CRIT("gPendingData_IN.processing_index = %u\n", gPendingData_IN.processing_index);
    CRIT("gPendingData_IN.index = %u\n", gPendingData_IN.index);

//    for (cmd_index = 0; cmd_index < AHCI_NCQ_DEPTH; cmd_index++)
//    {
//        CRIT(" gFree_cmd_buffer_list.valid[%u] = %u, cmd_index = %u\n", cmd_index, gFree_cmd_buffer_list.valid_flag[cmd_index], gFree_cmd_buffer_list.cmd_index[cmd_index]);
//    }

//    CRIT(" gFree_cmd_buffer_list.processing_index = %u\n", gFree_cmd_buffer_list.processing_index);
//    CRIT(" gFree_cmd_buffer_list.index = %u\n", gFree_cmd_buffer_list.index);

    return;
}


/*****************************************************************************
 * Function: increment_index
 *************************************************************************//**
 * This function increments an index value and handles rollover.
 *
 * @param[out] index pointer to index to increment.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */
inline void increment_index(INT32_T *index)
{
    INT32_T val = *index;

    if (++val >= gCmdQueueDepth)
    {
        val = 0;
    }

    *index = val;

    return;
}

inline void increment_index_plus(INT32_T *index)
{
    INT32_T val = *index;

    if (++val >= AHCI_NCQ_DEPTH)
    {
        val = 0;
    }

    *index = val;

    return;
}

/*****************************************************************************
 * Function: decrement_index
 *************************************************************************//**
 * This function decrements an index value and handles rollunder.
 *
 * @param[out] index pointer to index to increment.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */
inline void decrement_index(INT32_T *index)
{
    INT32_T val = *index;

    if (--val < 0)
    {
        val = gCmdQueueDepth - 1;
    }

    *index = val;

    return;
}

inline void decrement_index_plus(INT32_T *index)
{
    INT32_T val = *index;

    if (--val < 0)
    {
        val = AHCI_NCQ_DEPTH - 1;
    }

    *index = val;

    return;
}

/*****************************************************************************
 * Function: ums_uas_get_cmd_index
 *************************************************************************//**
 * This function returns the next valid index from the index list specified.
 *
 * @param[in] index_list pointer to the index list.
 * @param[out] cmd_index pointer to memory to store index.
 *
 * @retval STATUS_OK when a valid index is returned.
 * @retval STATUS_ERROR otherwise.
 *
 ****************************************************************************** 
 */

inline STATUS_T ums_uas_get_cmd_index(UMS_UAS_INDEX_LIST_T *index_list, UINT32_T *cmd_index)
{
    STATUS_T status = STATUS_OK;

    if (index_list->valid_flag[index_list->processing_index])
    {
        *cmd_index = index_list->cmd_index[index_list->processing_index];
        index_list->valid_flag[index_list->processing_index] = FALSE;
        increment_index(&index_list->processing_index);
    }
    else
    {
        status = STATUS_ERROR;
    }

    return status;
}

/*****************************************************************************
 * Function: ums_uas_get_cmd_index
 *************************************************************************//**
 * This function returns the next valid index from the index list specified.
 *
 * @param[in] index_list pointer to the index list.
 * @param[out] cmd_index pointer to memory to store index.
 *
 * @retval STATUS_OK when a valid index is returned.
 * @retval STATUS_ERROR otherwise.
 *
 ****************************************************************************** 
 */

inline STATUS_T ums_uas_get_cmd_index_plus(UMS_UAS_INDEX_LIST_T *index_list, UINT32_T *cmd_index)
{
    STATUS_T status = STATUS_OK;

    if (index_list->valid_flag[index_list->processing_index])
    {
        *cmd_index = index_list->cmd_index[index_list->processing_index];
        index_list->valid_flag[index_list->processing_index] = FALSE;
        increment_index_plus(&index_list->processing_index);
    }
    else
    {
        status = STATUS_ERROR;
    }

    return status;
}

/*****************************************************************************
 * Function: find_empty_cmd_overflow_buffer_index
 *************************************************************************//**
 * This function returns the an empty command buffer index.
 *
 * @param[out] cmd_index pointer to memory to store command index.
 *
 * @retval STATUS_OK when command index is found.
 * @retval STATUS_ERROR otherwise.
 *
 ****************************************************************************** 
 */

STATUS_T find_empty_cmd_overflow_buffer_index(UINT32_T *cmd_index)
{
    STATUS_T status = STATUS_ERROR;
    UINT32_T i;

    for (i = gCmdQueueDepth; i < UMS_UAS_CMD_DEPTH; i++)
    {
        if (gUAS_state[i] == UMS_UAS_STATE_IDLE)
        {
            *cmd_index = i;
            status = STATUS_OK;
            break;
        }
    }

    return status;
}

/*****************************************************************************
 * Function: ums_uas_rx_command
 *************************************************************************//**
 * This function sends an I/O request to recieve a packet on the 
 * command pipe.
 *
 * @param[in] buffer_index index of command buffer.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ums_uas_rx_command(UINT32_T buffer_index)
{
    DEBUG("-> ums_uas_rx_command - index = %u.\n", buffer_index);

    // Send I/O request.
    usb_hal_io_request((UMS_UAS_CMD_ENDPT_NUM | ENDPT_DIRECTION_OUT), (void *)&datapath_ram->ums_cmd_buffer[buffer_index], 
                       usb_dev.ep_info_OUT[UMS_UAS_CMD_ENDPT_NUM].wMaxPktSize, 0);

    gUAS_state[buffer_index] = UMS_UAS_STATE_RX_CMD;

    return;
}

/*****************************************************************************
 * Function: ums_uas_tx_data
 *************************************************************************//**
 * This function sends an I/O request to transmit data on the 
 * Data-IN pipe.
 *
 * @param[in] buffer pointer to data buffer.
 * @param[in] byte_cnt number of bytes to transmit.
 * @param[in] cmd_index index of command for this data transfer.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_XFER_ACTIVE when the data pipe is already in use.
 * @retval STATUS_TIMEOUT when the USB command times out.
 *
 ****************************************************************************** 
 */

STATUS_T ums_uas_tx_data(void *buffer, UINT32_T byte_cnt, UINT32_T cmd_index)
{
    STATUS_T status = STATUS_ERROR;

    DEBUG("-> ums_uas_tx_data() - buff = 0x%08x, byte_cnt = %u, cmd_idx = %u, sid = 0x%x.\n", 
          (UINT32_T)buffer, byte_cnt, cmd_index, BSWAP_16(gCommand_IU[cmd_index]->wTAG));

    if (byte_cnt == 0)
    {
        return STATUS_OK;
    }

    // Send I/O request.
    status = usb_hal_io_request((UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), buffer, byte_cnt,
                                (usb_dev.dev_speed == USB_SUPER_SPEED) ? BSWAP_16(gCommand_IU[cmd_index]->wTAG) : 0);

    if (status == STATUS_OK)
    {
        // Save data-IN command index.
        gCurrentDataInIndex = cmd_index;

        // Set UAS state to DATA IN.
        gUAS_state[cmd_index] = UMS_UAS_STATE_DATA_IN; 

#if ENABLE_UAS_DATA_IN_PACING
        if (buffer == (void *)SATA_TO_USB_WRAP_WINDOW_ADDR)
        {
            // Free enough room in wrap window for SATA transfer by incrementing the read offset.
            WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), (READ32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM)) + MIN(byte_cnt, WRAP_WINDOW_BLOCK_SIZE_BYTES)));
        }
#endif 

    }
    else if (status == STATUS_XFER_ACTIVE)
    {
        if (gUAS_state[cmd_index] != UMS_UAS_STATE_DATA_IN_PENDING)
        {
            INFO(" data-in pending.\n");
            // Add transfer to pending Data-IN queue.
            gPendingData_IN.data_buff[gPendingData_IN.index] = buffer;    
            gPendingData_IN.byte_cnt[gPendingData_IN.index] = byte_cnt;
            gPendingData_IN.cmd_index[gPendingData_IN.index] = cmd_index;
            gPendingData_IN.xfer_pending[gPendingData_IN.index] = TRUE;

            // Increment array index.
            increment_index(&gPendingData_IN.index);

            // Set UAS state to DATA IN Pending.
            gUAS_state[cmd_index] = UMS_UAS_STATE_DATA_IN_PENDING;
        }
    }
    else
    {
        CRIT("@Error - UAS Tx status = %u.\n", status);
    }

    return status;
}

/*****************************************************************************
 * Function: ums_uas_rx_data
 *************************************************************************//**
 * This function sends an I/O request to recieve data on the 
 * Data-OUT pipe.
 *
 * @param[out] buffer pointer to buffer to receive data.
 * @param[in] byte_cnt number of bytes to receive.
 * @param[in] cmd_index index of command for this data transfer.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_XFER_ACTIVE when the data pipe is already in use.
 * @retval STATUS_TIMEOUT when the USB command times out.
 *
 ****************************************************************************** 
 */

STATUS_T ums_uas_rx_data(void *buffer, UINT32_T byte_cnt, UINT32_T cmd_index)
{
    STATUS_T status = STATUS_ERROR;

    DEBUG("-> ums_uas_rx_data() - buff = 0x%08x, byte_cnt = %u, cmd_idx = %u, sid = 0x%x.\n", 
          (UINT32_T)buffer, byte_cnt, cmd_index, BSWAP_16(gCommand_IU[cmd_index]->wTAG));

    // Send I/O request.
    status = usb_hal_io_request((UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT), buffer, byte_cnt, 
                                (usb_dev.dev_speed == USB_SUPER_SPEED) ? BSWAP_16(gCommand_IU[cmd_index]->wTAG) : 0);

    if (status == STATUS_OK)
    {
        // Save data-OUT command index.
        gCurrentDataOutIndex = cmd_index;

        // Set UAS state to DATA OUT.
        gUAS_state[cmd_index] = UMS_UAS_STATE_DATA_OUT;
    }
    else if (status == STATUS_XFER_ACTIVE)
    {
        if (gUAS_state[cmd_index] != UMS_UAS_STATE_DATA_OUT_PENDING)
        {
            INFO(" data-OUT pending.\n");
            // Add transfer to pending Data-OUT.
            gPendingData_OUT.data_buff = buffer;    
            gPendingData_OUT.byte_cnt = byte_cnt;
            gPendingData_OUT.cmd_index = cmd_index;
            gPendingData_OUT.xfer_pending = TRUE;

            // Set UAS state to DATA OUT Pending.
            gUAS_state[cmd_index] = UMS_UAS_STATE_DATA_OUT_PENDING;
        }
    }

    return status;
}

/*****************************************************************************
 * Function: ums_uas_tx_status
 *************************************************************************//**
 * This function sends an I/O request to send a packet on the status pipe.
 *
 * @param[in] buffer pointer to data buffer.
 * @param[in] byte_cnt number of bytes to transmit.
 * @param[in] cmd_index index of command for this data transfer.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_XFER_ACTIVE when the status pipe is already in use.
 * @retval STATUS_TIMEOUT when the USB command times out.
 *
 ****************************************************************************** 
 */

inline STATUS_T ums_uas_tx_status(void *buffer, UINT32_T byte_cnt, UINT32_T cmd_index)
{
    STATUS_T status = STATUS_ERROR;

    // Send I/O request.
    status = usb_hal_io_request((UMS_UAS_STATUS_ENDPT_NUM | ENDPT_DIRECTION_IN), buffer, byte_cnt,
                                (usb_dev.dev_speed == USB_SUPER_SPEED) ? BSWAP_16(gCommand_IU[cmd_index]->wTAG) : 0);

    if (status == STATUS_OK)
    {
        gCurrent_status_index = cmd_index;
    }
    else if (status == STATUS_XFER_ACTIVE)
    {
        DEBUG("-> ums_uas_tx_status() - pipe busy, sid = 0x%x.\n", stream_id);
    }

    return status;
}

/*****************************************************************************
 * Function: ums_uas_ata_error_callback
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

void ums_uas_ata_error_callback(ATA_CMD_CALLBACK_T *pAtaCmdCbkData)
{
    UINT32_T cmd_index;

    // Get command index from response order list.
    cmd_index = gResponse_order.cmd_index[gResponse_order.processing_index];

    CRIT("-> ums_uas_ata_error_callback() - gUAS_state[%u] = %u.\n", cmd_index, gUAS_state[cmd_index]);

    if (gUAS_state[cmd_index] == UMS_UAS_STATE_IDLE)
    {
        // Error callback may be due to a LUN reset.
        return;
    }

    // Increment response order processing index.
    increment_index(&gResponse_order.processing_index);

    // Cancel all data transfers.
    usb_hal_cancel_io_request((UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT));
    usb_hal_cancel_io_request((UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN));

    // Reset wrap window R/W offsets.
    mww_reset_rw_offsets();

    ums_uas_send_sense_iu(cmd_index, SCSI_STATUS_CHECK_CONDITION);

#if 0
    // Send error status for all unfinished cmds.
    for (cmd_index = 0; cmd_index < gCmdQueueDepth; cmd_index++)
    {
        if ((gUAS_state[cmd_index] != UMS_UAS_STATE_IDLE) &&
            (gUAS_state[cmd_index] != UMS_UAS_STATE_SENSE_PENDING) &&
            (gUAS_state[cmd_index] != UMS_UAS_STATE_RESPONSE_PENDING))
        {
            // BQ - is data phase error the correct response?
            scsi_set_sense_data(MEDIUM_ERROR, DATA_PHASE_ERROR, NO_ASCQ);
            ums_uas_send_sense_iu(cmd_index, SCSI_STATUS_CHECK_CONDITION);
        }
    }

    //gRW_Ready_locked = FALSE; 
//    gNonQueuedCmdInProgress = FALSE;

    // BQ - I expect the host to reset UAS.  If I reset here the sense IU's sent above
    // may have not been sent yet and will be cancelled.
    //ums_uas_reset();
#endif

    return;
}


/*****************************************************************************
 * Function: ums_uas_ata_cmd_callback
 *************************************************************************//**
 * This callback is called by the AHCI block when a D2H register
 * FIS or Set Device Bits FIS is received.
 *
 * @param[in] pAtaCmdCbkData pointer to callback data structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_ata_cmd_callback(ATA_CMD_CALLBACK_T *pAtaCmdCbkData)
{
    STATUS_T status = STATUS_OK;
    UINT32_T cmd_index;
    UINT32_T start_index;

    if (pAtaCmdCbkData->bCmdSlot == SDB_FIS_CMD_SLOT_NUM)
    {
        // This callback was due to a Set Device Bits FIS.
        if (pAtaCmdCbkData->bStatus & ATA_ERROR_STATUS_BIT)
        {
            // All outstanding commands aborted by drive.
            // Send failed status for all outstanding commands so host will re-issue.
            // BQ - fix me.  I'm not sure that this can occur.  We may only get ums_uas_ata_error_callback() due to task file error.
            // If that is the case, how do we know that a queued command failed?

            CRIT("@Error: ums_uas_ata_cmd_callback() - ATA error detected in SDB FIS!\n");

            // Decrement response order processing index since it will be incremented in ums_uas_ata_error_callback.
//            decrement_index(&gResponse_order.processing_index);
//
//            ums_uas_ata_error_callback(0);
        }
        else /* successful completion */
        {
            // Narrow down the range of command slots to search.
            if (pAtaCmdCbkData->dSActive & 0xFF)
            {
                start_index = 0;
            }
            else if (pAtaCmdCbkData->dSActive & 0xFF00)
            {
                start_index = 8;
            }
            else if (pAtaCmdCbkData->dSActive & 0xFF0000)
            {
                start_index = 16;
            }
            else
            {
                start_index = 24;
            }

            // Successful status returns may be aggregated so we need to check SActive to determine completed commands.
            for (cmd_index = start_index; cmd_index < gCmdQueueDepth; cmd_index++)
            {
                if (pAtaCmdCbkData->dSActive & (1 << cmd_index))
                {
                    DEBUG("-> ums_uas_ata_cmd_callback() - SDB FIS - SActive = 0x%08x, CDB[0] = 0x%02x, gUAS_state[%u] = %u.\n", 
                          pAtaCmdCbkData->dSActive, gCommand_IU[cmd_index]->CDB[0], cmd_index, gUAS_state[cmd_index]);

                    // Must verify UAS state here since status may have been sent in queued command callback.
                    if (gUAS_state[cmd_index] == UMS_UAS_STATE_SEND_SENSE)
                    {
                        // Set SATA xfer complete flag.
                        gSATA_xfer_cmpltd[cmd_index] = TRUE;

                        // Send status.
                        ums_uas_send_sense_iu(cmd_index, SCSI_STATUS_GOOD);
                    }
                    else if ((gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN) || 
                             (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN_PENDING) ||
                             (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_OUT) ||
                             (gUAS_state[cmd_index] == UMS_UAS_STATE_READ_READY_PENDING))
                    {
                        // Set SATA xfer complete flag.
                        gSATA_xfer_cmpltd[cmd_index] = TRUE;
                    }

                    // Mask off processed status bit.
                    pAtaCmdCbkData->dSActive ^= (1 << cmd_index);

                    if (pAtaCmdCbkData->dSActive == 0)
                    {
                        // Exit for loop.
                        break;
                    }
                }
            }
        }
    }
    else /* Callback was due to D2H Register FIS or PIO Setup FIS. */
    {
        // Get command index from response order list.
        cmd_index = gResponse_order.cmd_index[gResponse_order.processing_index];

        if (gUAS_state[cmd_index] == UMS_UAS_STATE_IDLE)
        {
            // Commands may have been cancelled.

            CRIT("@Warning: gUAS_state[%u] is IDLE! gCommand_IU[%u]->CDB[0] = 0x%02x. wTag = 0x%x\n", cmd_index, 
                 gResponse_order.processing_index,
                 gCommand_IU[gResponse_order.cmd_index[gResponse_order.processing_index]]->CDB[0],
                 BSWAP_16(gCommand_IU[cmd_index]->wTAG));

            if (gResponse_order.processing_index < gResponse_order.index)
            {
                // Increment response order processing index.
                increment_index(&gResponse_order.processing_index);
            }

            return;
        }

        // Increment response order processing index.
        increment_index(&gResponse_order.processing_index);

        CRIT("-> ums_uas_ata_cmd_callback() - D2H Reg or PIO Setup FIS - CDB[0] = 0x%02x, gUAS_state[%u] = %u.\n", 
              gCommand_IU[cmd_index]->CDB[0], cmd_index, gUAS_state[cmd_index]);

        if (pAtaCmdCbkData->bStatus & ATA_ERROR_STATUS_BIT)
        {
            CRIT("@Error: ums_uas_ata_cmd_callback() - ATA error detected!\n");

            // Decrement response order processing index since it will be incremented in ums_uas_ata_error_callback.
            decrement_index(&gResponse_order.processing_index);

            // Call error handler.
            ums_uas_ata_error_callback(NULL);
        }
        else /* successful completion */
        {
            // Set SATA xfer complete flag.
            gSATA_xfer_cmpltd[cmd_index] = TRUE;

            if (gUAS_state[cmd_index] == UMS_UAS_STATE_SEND_SENSE)
            {
                // Send Sense IU.                
                ums_uas_send_sense_iu(cmd_index, SCSI_STATUS_GOOD);
            }
            else if ((gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN) &&
                     !scsi_is_rw_cmd(gCommand_IU[cmd_index]->CDB[0]))
            {
                if (usb_dev.dev_speed == USB_HIGH_SPEED)
                {
                    // Send Read Ready IU.
                    status = ums_uas_send_rw_ready_iu(cmd_index);
                }

                if (status == STATUS_OK)
                {
                    CRIT("pAtaCmdCbkData->dDataByteCnt = %u\n", pAtaCmdCbkData->dDataByteCnt);
                    // Send data without using wrap window.
                    ums_uas_tx_data((void *)datapath_ram->normal_data_buffer,
                                    pAtaCmdCbkData->dDataByteCnt, cmd_index);
                }
            }
        }
    } /* END: D2H Register FIS */

    // In case a command is waiting because PxSACT or PxCI was busy and status IU was already sent. 
    if (gCmdWaitingCount)
    {
        ums_uas_process_pending_cmds();
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_send_rw_ready_iu
 *************************************************************************//**
 * This function sends a Read Ready IU or Write Ready IU on the status pipe.
 *
 * @param[in] cmd_index index of command for this ready IU.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_XFER_ACTIVE when the status pipe is already in use.
 * @retval STATUS_TIMEOUT when the USB command times out.
 * @retval STATUS_ERROR when the UAS state is invalid.
 *
 ****************************************************************************** 
 */

STATUS_T ums_uas_send_rw_ready_iu(UINT32_T cmd_index)
{
    STATUS_T status = STATUS_OK;
    UAS_RW_READY_IU_T *rw_ready_iu;

    // Set pointer to status buffer.
    rw_ready_iu = (UAS_RW_READY_IU_T *)&datapath_ram->ums_status_buffer[cmd_index];

    // Populate IU info.
    rw_ready_iu->rsvd = 0;
    rw_ready_iu->wTAG = gCommand_IU[cmd_index]->wTAG;

    if ((gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN) || 
        (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN_QUEUED) ||
        (gUAS_state[cmd_index] == UMS_UAS_STATE_READ_READY_PENDING))
    {
        rw_ready_iu->bIU_ID = UAS_READ_READY_IU;
    }
    else if ((gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_OUT) || 
             (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_OUT_QUEUED) ||
             (gUAS_state[cmd_index] == UMS_UAS_STATE_WRITE_READY_PENDING))
    {
        rw_ready_iu->bIU_ID = UAS_WRITE_READY_IU;
    }
    else
    {
        CRIT("@Error: Invalid gUAS_state[%u] = (%u) for ums_uas_send_rw_ready_iu().\n", cmd_index, gUAS_state[cmd_index]);
        return STATUS_ERROR;
    }

    DEBUG("-> ums_uas_send_rw_ready_iu() - %s READY, cmd_index = %u.\n",
          (rw_ready_iu->bIU_ID == UAS_READ_READY_IU) ? "READ" : (rw_ready_iu->bIU_ID == UAS_WRITE_READY_IU) ? "WRITE" : "?", cmd_index);

    if (!gRW_Ready_locked)
    {
        // Check for any pending Read Ready IUs to make sure we keep USB transfers in order of completion by SATA drive.
        if ((rw_ready_iu->bIU_ID == UAS_READ_READY_IU) &&
            (gUAS_state[gPendingData_IN.cmd_index[gPendingData_IN.processing_index]] == UMS_UAS_STATE_READ_READY_PENDING) && 
            (gPendingData_IN.cmd_index[gPendingData_IN.processing_index] != cmd_index))
        {
            status = STATUS_XFER_ACTIVE;
        }
        else
        {
            // Send RW Ready IU on status pipe.
            status = ums_uas_tx_status(rw_ready_iu, sizeof(UAS_RW_READY_IU_T), cmd_index);   

            if (status == STATUS_OK)
            {
                gRW_Ready_locked = TRUE; 
            }
        }
    }
    else
    {
        status = STATUS_XFER_ACTIVE;
    }

    if (status == STATUS_XFER_ACTIVE)
    {
        if (rw_ready_iu->bIU_ID == UAS_READ_READY_IU)
        {
            gUAS_state[cmd_index] = UMS_UAS_STATE_READ_READY_PENDING;
        }
        else /* Write Ready IU */
        {
            gUAS_state[cmd_index] = UMS_UAS_STATE_WRITE_READY_PENDING;
        }
    }

    return status;
}


/*****************************************************************************
 * Function: ums_uas_start_data_xfer
 *************************************************************************//**
 * This function starts a data transfer for a SCSI Read or Write command 
 * using the memory wrap window.
 *
 * @param[in] cmd_index index of command for this ready IU.
 * @param[in] byte_cnt transfer length in bytes.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_start_data_xfer(UINT32_T cmd_index, UINT32_T byte_cnt)
{
    STATUS_T status = STATUS_OK;

    INFO("-> ums_uas_start_data_xfer() - cmd_index = %u.\n", cmd_index);

    if (usb_dev.dev_speed == USB_HIGH_SPEED)
    {
        // Send R/W Ready IU.
        status = ums_uas_send_rw_ready_iu(cmd_index);
    }

    if (status == STATUS_OK)
    {
        if ((gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_OUT_QUEUED) ||
            (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_OUT))
        {
            // Rx data over USB.
            ums_uas_rx_data((void *)USB_TO_SATA_WRAP_WINDOW_ADDR, byte_cnt, cmd_index);
        }
        else if ((gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN_QUEUED) ||
                 (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN)) /* DATA-IN */
        {
            // Tx data over USB.
            ums_uas_tx_data((void *)SATA_TO_USB_WRAP_WINDOW_ADDR, byte_cnt, cmd_index);   
        }
        else
        {
            CRIT("ums_uas_start_data_xfer w/ UAS_state[%u] = %u.\n", cmd_index, gUAS_state[cmd_index]);
        }
    }
    else if (status == STATUS_XFER_ACTIVE)
    {
        if (gUAS_state[cmd_index] == UMS_UAS_STATE_WRITE_READY_PENDING)
        {
            // Pending Data-OUT.
            gPendingData_OUT.data_buff = (void *)USB_TO_SATA_WRAP_WINDOW_ADDR;   
            gPendingData_OUT.byte_cnt = byte_cnt;
            gPendingData_OUT.cmd_index = cmd_index;
            gPendingData_OUT.xfer_pending = TRUE;
        }
        else if (gUAS_state[cmd_index] == UMS_UAS_STATE_READ_READY_PENDING)
        {
            // Add transfer to pending Data-IN queue.
            gPendingData_IN.data_buff[gPendingData_IN.index] = (void *)SATA_TO_USB_WRAP_WINDOW_ADDR;    
            gPendingData_IN.byte_cnt[gPendingData_IN.index] = byte_cnt;
            gPendingData_IN.cmd_index[gPendingData_IN.index] = cmd_index;
            gPendingData_IN.xfer_pending[gPendingData_IN.index] = TRUE;

            // Increment array index.
            increment_index(&gPendingData_IN.index);
        }
        else
        {
            CRIT("XFER ACTIVE w/ UAS_state[%u] = %u.\n", cmd_index, gUAS_state[cmd_index]);
        }
    }
    else
    {
        CRIT("ums_uas_start_data_xfer() - send RWIU status = %u\n", status);
        ums_uas_dump_state();
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_ata_queued_cmd_callback
 *************************************************************************//**
 * This function serves as a callback to be used by the AHCI 
 * module when a DMA Setup FIS interrupt is received.
 *
 * @param[in] pAtaCmdCbkData pointer to callback data structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_ata_queued_cmd_callback(ATA_CMD_CALLBACK_T *pAtaCmdCbkData)
{
    DEBUG("-> ums_uas_ata_queued_cmd_callback() - bCmdSlot = %u.\n", pAtaCmdCbkData->bCmdSlot);

    // For High-Speed operation, we must assume that any previous NCQ R/W operation was successfully completed
    // if we get a DMA Setup FIS for the next transfer before the SDB FIS for the previous R/W. This is required
    // because the Windows UAS driver expects to recieve Status IU before the next R/W Ready IU.
    if (usb_dev.dev_speed == USB_HIGH_SPEED)
    {
        if ((gUAS_state[gCurrentDataInIndex] == UMS_UAS_STATE_SEND_SENSE) &&
            scsi_is_rw_cmd(gCommand_IU[gCurrentDataInIndex]->CDB[0]))
        {
            // Set SATA xfer complete flag.
            gSATA_xfer_cmpltd[gCurrentDataInIndex] = TRUE;

            // Send status.
            ums_uas_send_sense_iu(gCurrentDataInIndex, SCSI_STATUS_GOOD);
        }
        else if ((gUAS_state[gCurrentDataOutIndex] == UMS_UAS_STATE_SEND_SENSE) &&
                 scsi_is_rw_cmd(gCommand_IU[gCurrentDataOutIndex]->CDB[0]))
        {
            // Set SATA xfer complete flag.
            gSATA_xfer_cmpltd[gCurrentDataOutIndex] = TRUE;

            // Send status.
            ums_uas_send_sense_iu(gCurrentDataOutIndex, SCSI_STATUS_GOOD);
        }
    }

    // Start the USB data transfer.
    ums_uas_start_data_xfer(pAtaCmdCbkData->bCmdSlot, pAtaCmdCbkData->dDataByteCnt);

    return;
}


/*****************************************************************************
 * Function: ums_uas_determine_state
 *************************************************************************//**
 * This function sets the UAS state based on the SCSI command
 * in the CDB.
 *
 * @param[in] cdb pointer to SCSI command descriptor block.
 * @param[in] cmd_index index of the command.
 *
 * @retval STATUS_OK when the data direction has been successfully determined.
 * @retval STATUS_NO_DATA when there is no data phase.
 * @retval STATUS_ERROR when the SCSI command is not supported.
 *
 ****************************************************************************** 
 */

inline STATUS_T ums_uas_determine_state(UINT8_T *cdb, UINT32_T cmd_index)
{
    ENDPT_DIR_T direction;
    STATUS_T status;

    status = scsi_get_data_direction(cdb, &direction);

    if (status == STATUS_OK)
    {
        if (ata_dev[gLUN[cmd_index]].bNCQ && scsi_is_rw_cmd(cdb[0]))
        {
            gUAS_state[cmd_index] = (direction == ENDPT_DIRECTION_OUT) ? UMS_UAS_STATE_DATA_OUT_QUEUED : UMS_UAS_STATE_DATA_IN_QUEUED;                   
        }
        else
        {
            gUAS_state[cmd_index] = (direction == ENDPT_DIRECTION_OUT) ? UMS_UAS_STATE_DATA_OUT : UMS_UAS_STATE_DATA_IN;           
        }
    }
    else if (status == STATUS_NO_DATA)
    {
        gUAS_state[cmd_index] = UMS_UAS_STATE_SEND_SENSE;
    }
    else /* ERROR */
    {
        CRIT("@Error: Unable to determine UAS state for scsi_cmd = 0x%02x!\n", cdb[0]);
    }

    DEBUG("-> ums_uas_determine_state() - CDB[0] = 0x%x, UAS_state[%u] = %u.\n", cdb[0], cmd_index, gUAS_state[cmd_index]);

    return status;
}


#define MAX_SCSI_COMMAND_LENGTH  16  /* bytes */

/*****************************************************************************
 * Function: ums_uas_issue_scsi_cmd
 *************************************************************************//**
 * This function calls the SCSI command handler and processes
 * the result.
 *
 * @param[in] cmd_index index of the command.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_issue_scsi_cmd(UINT32_T cmd_index)
{
    STATUS_T status;

    DEBUG("-> ums_uas_issue_scsi_cmd() - cmd_index = %u, CDB[0] = 0x%02x.\n", cmd_index, gCommand_IU[cmd_index]->CDB[0]);

    // Populate command parameters.
    ums_uas_cmd.bLUN = gLUN[cmd_index];
    ums_uas_cmd.pCommandBlock = &gCommand_IU[cmd_index]->CDB[0]; 
    ums_uas_cmd.bCmdSlotNum = (ata_dev[gLUN[cmd_index]].bNCQ) ? cmd_index : 0;  /* Set command slot to command index value */

    if ((gCommand_IU[cmd_index]->CDB[0] >= SCSI_TI_OPCODE_MIN) &&
        (gCommand_IU[cmd_index]->CDB[0] <= SCSI_TI_OPCODE_MAX))
    {
        ums_uas_cmd.bCmdBlkLength = 0x06;
    }
    else
    {
        ums_uas_cmd.bCmdBlkLength = MAX_SCSI_COMMAND_LENGTH;  /* could be between 6 and 16 bytes */  
    }

    // Clear xfer complete flag.
    gSATA_xfer_cmpltd[cmd_index] = FALSE;

    // Send command to SCSI module.
    status = scsi_command_handler(&ums_uas_cmd);

    if (status == STATUS_SCSI_RESPONSE_PENDING)
    {
        if (!ata_dev[gLUN[cmd_index]].bNCQ || !scsi_is_rw_cmd(gCommand_IU[cmd_index]->CDB[0]))
        {
            // Save command index for ATA command callback.
            gResponse_order.cmd_index[gResponse_order.index] = cmd_index;
            increment_index(&gResponse_order.index);
        }
    }
    else if (status == STATUS_SCSI_RESPONSE_READY)
    {
        // Set SATA xfer complete flag to TRUE so status will be sent in ums_uas_data_IN_pipe_callback().
        gSATA_xfer_cmpltd[cmd_index] = TRUE;

        if (usb_dev.dev_speed == USB_HIGH_SPEED)
        {
            // Send Read Ready IU.
            status = ums_uas_send_rw_ready_iu(cmd_index);
        }
        else
        {
            status = STATUS_OK;
        }

        if (status == STATUS_OK)
        {
            // Send USB I/O request to Tx Data.
            ums_uas_tx_data((void *)&datapath_ram->scsi_response_buffer[0],
                            MIN(ums_uas_cmd.dDataByteCnt, SCSI_RESPONSE_BUFF_SIZE), cmd_index);
        }
        else if (gUAS_state[cmd_index] == UMS_UAS_STATE_READ_READY_PENDING)
        {
            // Add transfer to pending Data-IN queue.
            gPendingData_IN.data_buff[gPendingData_IN.index] =
                (void *)&datapath_ram->scsi_response_buffer[0];
            gPendingData_IN.byte_cnt[gPendingData_IN.index] = MIN(ums_uas_cmd.dDataByteCnt, SCSI_RESPONSE_BUFF_SIZE);//ums_uas_cmd.dDataByteCnt;
            gPendingData_IN.cmd_index[gPendingData_IN.index] = cmd_index;
            gPendingData_IN.xfer_pending[gPendingData_IN.index] = TRUE;

            // Increment array index.
            increment_index(&gPendingData_IN.index);
        }
    }
    else
    {
        if (status != STATUS_OK)
        {
            if (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_IN)
            {
                usb_hal_cancel_io_request((UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN));
            }
            else if (gUAS_state[cmd_index] == UMS_UAS_STATE_DATA_OUT)
            {
                usb_hal_cancel_io_request((UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT));
            }
        }

        // Set SATA xfer complete flag to TRUE so IDLE state will be set in status pipe callback.
        gSATA_xfer_cmpltd[cmd_index] = TRUE;

        if (status == STATUS_SCSI_ATA_ERROR)
        {
            CRIT("@Error: STATUS_SCSI_ATA_ERROR!\n");
            // Save command index for ums_uas_ata_error_callback().
            gResponse_order.cmd_index[gResponse_order.index] = cmd_index;
            increment_index(&gResponse_order.index);            
        }
        else if (gUAS_state[cmd_index] != UMS_UAS_STATE_DATA_OUT)
        {
            // Send status.  Sense data should be populated by SCSI module.  
            // Status for UMS_UAS_STATE_DATA_OUT will be sent in Data OUT pipe callback.
            ums_uas_send_sense_iu(cmd_index, (status == STATUS_OK) ? SCSI_STATUS_GOOD : SCSI_STATUS_CHECK_CONDITION);
        }
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_process_cmd
 *************************************************************************//**
 * This function processes a command IU.
 *
 * @param[in] cmd_buffer_index index of the command buffer.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_process_cmd(UINT32_T cmd_buffer_index)
{
    BOOLEAN_T issue_scsi_cmd = TRUE;
    STATUS_T status;
    UAS_COMMAND_IU_T *cmd_iu;
    UINT32_T byte_cnt;

    cmd_iu = gCommand_IU[cmd_buffer_index];

    DEBUG("-> ums_uas_process_cmd() - cmd_index = %u, CDB[0] = 0x%02x.\n", cmd_buffer_index, cmd_iu->CDB[0]);

    // Determine UAS state based on SCSI command.
    status = ums_uas_determine_state(cmd_iu->CDB, cmd_buffer_index);

    if (status == STATUS_ERROR)
    {
        CRIT("@Error: ums_uas_process_cmd() - scsi_cmd = 0x%02x not supported!\n", cmd_iu->CDB[0]);
        // Send error status.
        scsi_set_sense_data(ILLEGAL_REQUEST, INVALID_COMMAND, NO_ASCQ);
        ums_uas_send_sense_iu(cmd_buffer_index, SCSI_STATUS_CHECK_CONDITION);
        return;
    }

    // Set check condition flag in case of ATA PASS-THROUGH command.
    gCheckCond[cmd_buffer_index] = scsi_is_check_condition_set(&cmd_iu->CDB[0]);

    // For Read/Write commands, if NCQ is not being used, the USB I/O request can be configured now.
    if (!ata_dev[gLUN[cmd_buffer_index]].bNCQ && scsi_is_rw_cmd(cmd_iu->CDB[0]) && ata_dev[gLUN[cmd_buffer_index]].bDeviceInitComplete)
    {
        if (scsi_get_rw_xfer_length(&cmd_iu->CDB[0], gLUN[cmd_buffer_index], &byte_cnt) == STATUS_OK)
        {
            // Start the USB data transfer.
            ums_uas_start_data_xfer(cmd_buffer_index, byte_cnt);
        }
    }
    else /* non-RW command */
    {
        if (gUAS_state[cmd_buffer_index] == UMS_UAS_STATE_DATA_OUT)
        {
            if (usb_dev.dev_speed == USB_HIGH_SPEED)
            {
                // Send Write Ready IU.
                status = ums_uas_send_rw_ready_iu(cmd_buffer_index); 
            }

            if (status == STATUS_OK)
            {
                // BQ - we must determine byte count from SCSI command.

                // Don't use wrap window for xfers that are are not DWORD multiples.
                // We don't know byte count so just use maximum.
                ums_uas_rx_data((void *)datapath_ram->normal_data_buffer,
                                sizeof(datapath_ram->normal_data_buffer),
                                cmd_buffer_index);
            }
            else if (status == STATUS_XFER_ACTIVE)
            {
                // UAS state is Write Ready Pending.
                gPendingData_OUT.data_buff = (void *)datapath_ram->normal_data_buffer;
                gPendingData_OUT.byte_cnt = sizeof(datapath_ram->normal_data_buffer);
                gPendingData_OUT.cmd_index = cmd_buffer_index;
                gPendingData_OUT.xfer_pending = TRUE;
            }

            issue_scsi_cmd = FALSE;                   
        }

        // Non-RW Data-IN commands must fit the allocated 4-KiB normal-data buffer.
    }

    if (issue_scsi_cmd)
    {
        // Issue SCSI command.   
        ums_uas_issue_scsi_cmd(cmd_buffer_index);
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_process_pending_cmds
 *************************************************************************//**
 * This function processes any pending commands.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_process_pending_cmds(void)
{
    INT32_T processing_index;

    DEBUG("-> ums_uas_process_pending_cmds() - pi = %u, uas_state[%u] = %u\n", 
          gCmd_list_processing_index, gCmd_index_list[gCmd_list_processing_index], gUAS_state[gCmd_index_list[gCmd_list_processing_index]]);

    while ((gProcessing_index_decrement_count > 0) &&
           (gUAS_state[gCmd_index_list[gCmd_list_processing_index]] != UMS_UAS_STATE_CMD_WAITING) &&
           (gUAS_state[gCmd_index_list[gCmd_list_processing_index]] != UMS_UAS_STATE_QUEUED_CMD_WAITING))
    {
        /* 
         * Processing index may be behind due to waiting to send status or 
         * retrying a non-queued command. Search for any waiting commands ahead 
         * of the current processing index and update gCmd_list_processing_index accordingly. 
         */
        // Increment command list processing index.
        gCmd_list_processing_index++;
        gCmd_list_processing_index &= CMD_INDEX_LIST_MASK;
        gProcessing_index_decrement_count--;
    }

    if (gUAS_state[gCmd_index_list[gCmd_list_processing_index]] == UMS_UAS_STATE_CMD_WAITING)
    {
        // Check to make sure all R/W commands are finished.

        INFO("Processing pending cmd waiting, gCmdWaitingCount = %u\n", gCmdWaitingCount);

        if (!(READ32(PxSACT(gLUN[gCmd_index_list[gCmd_list_processing_index]])) | 
              READ32(PxCI(gLUN[gCmd_index_list[gCmd_list_processing_index]]))) && 
            !gNonQueuedCmdInProgress)
        {
            gNonQueuedCmdInProgress = TRUE;
            ums_uas_process_cmd(gCmd_index_list[gCmd_list_processing_index]);
            gCmdWaitingCount--;
        }
        else
        {
            // Decrement command list processing index so the command will be retried next time.
            if (--gCmd_list_processing_index < 0)
                gCmd_list_processing_index = CMD_INDEX_LIST_MASK;

            gProcessing_index_decrement_count++;
            INFO("decrement gCmd_list_processing_index = %u.\n", gCmd_list_processing_index);
        }
    }
    else if (gUAS_state[gCmd_index_list[gCmd_list_processing_index]] == UMS_UAS_STATE_QUEUED_CMD_WAITING)
    {
        // Queued command(s) are are waiting.

        // Make sure no non-queued commands are in progress.
        if (!gNonQueuedCmdInProgress)
        {
            // Process contiguous block of queued commands.
            processing_index = gCmd_list_processing_index;

            do
            {
                ums_uas_process_cmd(gCmd_index_list[processing_index]);

                // Increment processing index and roll-over if necessary.
                processing_index++;
                processing_index &= CMD_INDEX_LIST_MASK;

                if (--gCmdWaitingCount == 0)
                    break;

            } while (gUAS_state[gCmd_index_list[processing_index]] == UMS_UAS_STATE_QUEUED_CMD_WAITING);
        }
        else
        {
            // Decrement command list processing index so the command will be retried next time.
            if (--gCmd_list_processing_index < 0)
                gCmd_list_processing_index = CMD_INDEX_LIST_MASK;

            gProcessing_index_decrement_count++;
            INFO(" decrement gCmd_list_processing_index = %u.\n", gCmd_list_processing_index);
        }
    }

    return;
}

#if UAS_REVISION_4_IU 
    #define SENSE_IU_SIZE_WITHOUT_SENSE_DATA  16  /* bytes */
#else
    #define SENSE_IU_SIZE_WITHOUT_SENSE_DATA  8  /* bytes */
#endif 
/*****************************************************************************
 * Function: ums_uas_send_sense_iu
 *************************************************************************//**
 * This function sends a Sense IU with the specified status.
 *
 * @param[in] cmd_index index of the command.
 * @param[in] scsi_status SCSI status.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_send_sense_iu(UINT32_T cmd_index, UINT8_T scsi_status)
{
    UINT32_T sense_data_length = 0;
    UAS_SENSE_IU_T *sense_iu = (UAS_SENSE_IU_T *)&datapath_ram->ums_status_buffer[cmd_index];
    STATUS_T status;

    if (gCheckCond[cmd_index])
    {
        scsi_status = SCSI_STATUS_CHECK_CONDITION;
    }

    if (scsi_status)
    {
        // Set SATA xfer complete flag to TRUE so IDLE state will be set in status pipe callback.
        gSATA_xfer_cmpltd[cmd_index] = TRUE;

        if (!((scsi_status == SCSI_STATUS_TASK_SET_FULL) && (gCmdQueueDepth < 32)))
        {
            CRIT("-> ums_uas_send_sense_iu() - cmd_index = %u, status = 0x%02x, tag = 0x%x, CDB[0] = 0x%02x, retry = %s.\n", 
                 cmd_index, scsi_status, BSWAP_16(gCommand_IU[cmd_index]->wTAG), gCommand_IU[cmd_index]->CDB[0],
                 (gUAS_state[cmd_index] == UMS_UAS_STATE_SENSE_PENDING) ? "Y" : "N");
        }
    }
    else
    {
        DEBUG("-> ums_uas_send_sense_iu() - cmd_index = %u, status = 0x%02x, tag = 0x%x, CDB[0] = 0x%02x, retry = %s.\n", 
              cmd_index, scsi_status, BSWAP_16(gCommand_IU[cmd_index]->wTAG), gCommand_IU[cmd_index]->CDB[0],
              (gUAS_state[cmd_index] == UMS_UAS_STATE_SENSE_PENDING) ? "Y" : "N");
    }

    // Ensure all reserved fields are zero.
    ti_memset(sense_iu, 0, sizeof(UAS_SENSE_IU_T));

    sense_iu->bIU_ID = UAS_SENSE_IU;
    sense_iu->bStatus = scsi_status;
    sense_iu->wTAG = gCommand_IU[cmd_index]->wTAG;
    sense_iu->wLength = 0;

    if (scsi_status == SCSI_STATUS_CHECK_CONDITION)
    {
        // Populate fixed format sense data.
        sense_data_length = scsi_copy_sense_data(sense_iu->bSenseData, FALSE);
        sense_iu->wLength = BSWAP_16(sense_data_length);
    }

    // Check for pending Sense.
    if (gStatus_pending_list.valid_flag[gStatus_pending_list.processing_index])
    {
        status = STATUS_XFER_ACTIVE;
    }
    else
    {
        status = ums_uas_tx_status(sense_iu, (SENSE_IU_SIZE_WITHOUT_SENSE_DATA + sense_data_length), cmd_index);
    }

    if (status == STATUS_OK)
    {
        gUAS_state[cmd_index] = UMS_UAS_STATE_STATUS_SENT;

        if (cmd_index < gCmdQueueDepth)
        {
            // Increment command list processing index.
            gCmd_list_processing_index++;
            gCmd_list_processing_index &= CMD_INDEX_LIST_MASK;

            DEBUG("gCmd_list_processing_index = %u, gUAS_state[%u] = %u.\n", gCmd_list_processing_index,
                  gCmd_index_list[gCmd_list_processing_index],
                  gUAS_state[gCmd_index_list[gCmd_list_processing_index]]);
        }
    }
    else if (status == STATUS_XFER_ACTIVE)
    {
        if (gUAS_state[cmd_index] == UMS_UAS_STATE_SENSE_PENDING)
        {
            // Keep this item on the pending list. 
            decrement_index_plus(&gStatus_pending_list.processing_index);
            gStatus_pending_list.valid_flag[gStatus_pending_list.processing_index] = TRUE;
        }
        else
        {
            // Set UAS state to status pending so Sense IU will be sent out when status pipe is free.
            gUAS_state[cmd_index] = UMS_UAS_STATE_SENSE_PENDING;

            // Add index to pending list.
            gStatus_pending_list.valid_flag[gStatus_pending_list.index] = TRUE;
            gStatus_pending_list.cmd_index[gStatus_pending_list.index] = cmd_index;
            increment_index_plus(&gStatus_pending_list.index);
        }
    }
    else
    {
        CRIT("ums_uas_tx_status = %u.\n", status);
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_retry_send_sense_iu
 *************************************************************************//**
 * This function retries a previously attempt to send a Sense IU.
 *
 * @param[in] cmd_index index of the command.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_retry_send_sense_iu(UINT32_T cmd_index)
{
    UINT32_T sense_data_length = 0;
    UAS_SENSE_IU_T *sense_iu = (UAS_SENSE_IU_T *)&datapath_ram->ums_status_buffer[cmd_index];
    STATUS_T status;

    if (sense_iu->bStatus)
    {
        if (!((sense_iu->bStatus == SCSI_STATUS_TASK_SET_FULL) && (gCmdQueueDepth < 32)))
        {
            CRIT("-> ums_uas_retry_send_sense_iu() - cmd_index = %u, status = 0x%02x, tag = 0x%x, CDB[0] = 0x%02x, retry = %s.\n", 
                 cmd_index, sense_iu->bStatus, BSWAP_16(gCommand_IU[cmd_index]->wTAG), gCommand_IU[cmd_index]->CDB[0],
                 (gUAS_state[cmd_index] == UMS_UAS_STATE_SENSE_PENDING) ? "Y" : "N");
        }
    }
    else
    {
        DEBUG("-> ums_uas_retry_send_sense_iu() - cmd_index = %u, status = 0x%02x, tag = 0x%x, CDB[0] = 0x%02x, retry = %s.\n", 
              cmd_index, scsi_status, BSWAP_16(gCommand_IU[cmd_index]->wTAG), gCommand_IU[cmd_index]->CDB[0],
              (gUAS_state[cmd_index] == UMS_UAS_STATE_SENSE_PENDING) ? "Y" : "N");
    }

    sense_data_length = BSWAP_16(sense_iu->wLength);

    status = ums_uas_tx_status(sense_iu, (SENSE_IU_SIZE_WITHOUT_SENSE_DATA + sense_data_length), cmd_index);

    if (status == STATUS_OK)
    {
        gUAS_state[cmd_index] = UMS_UAS_STATE_STATUS_SENT;

        if (cmd_index < gCmdQueueDepth)
        {
            // Increment command list processing index.
            gCmd_list_processing_index++;
            gCmd_list_processing_index &= CMD_INDEX_LIST_MASK;

            DEBUG("gCmd_list_processing_index = %u, gUAS_state[%u] = %u.\n", gCmd_list_processing_index,
                  gCmd_index_list[gCmd_list_processing_index],
                  gUAS_state[gCmd_index_list[gCmd_list_processing_index]]);
        }
    }
    else
    {
        CRIT("ums_uas_tx_status = %u, gCurrent_status_index = %u.\n", status, gCurrent_status_index);
        DUMP_UAS_STATE();
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_init_queue_depth
 *************************************************************************//**
 * This function populates the free command buffer list.
 *
 * @param lun logical unit number.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_init_queue_depth(UINT32_T lun)
{
    UINT32_T cmd_index;

    if (ata_dev[0].bDeviceInitComplete)
    {
        gCmdQueueDepth = (ata_dev[0].bNCQ) ? (ata_dev[0].bQueueDepth + 1) : AHCI_NCQ_DEPTH;
    }
    else
    {
        gCmdQueueDepth = AHCI_NCQ_DEPTH;
    }

    CRIT("-> ums_uas_init_queue_depth() - %u.\n", gCmdQueueDepth);

    ti_memset(&gFree_cmd_buffer_list, 0, sizeof(gFree_cmd_buffer_list));

    // Populate free command buffer list.
    for (cmd_index = 0; cmd_index < UMS_UAS_CMD_DEPTH; cmd_index++)
    {
        if (gUAS_state[cmd_index] != UMS_UAS_STATE_RX_CMD)
        {
            gUAS_state[cmd_index] = UMS_UAS_STATE_IDLE;

            if (cmd_index < gCmdQueueDepth)
            {
                gFree_cmd_buffer_list.cmd_index[gFree_cmd_buffer_list.index] = cmd_index;
                gFree_cmd_buffer_list.valid_flag[gFree_cmd_buffer_list.index] = TRUE;               
                gFree_cmd_buffer_list.index++;
            }
        }
    }

    return;
}

/*****************************************************************************
 * Function: ums_uas_idle
 *************************************************************************//**
 * This function initializes the queue depth and prepares to receive a
 * command IU if necessary.
 *
 * @param lun logical unit number.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_idle(UINT32_T lun)
{
    // UAS is not support for ATAPI devices.  Reset so we can enumerate as BOT.
    if (ata_dev[0].bPacketDevice)
    {
        system_reset();
    }

    ums_uas_init_queue_depth(lun);

    // Check for no active transfers on the command endpoint.
    if (!usb_dev.ep_info_OUT[UMS_UAS_CMD_ENDPT_NUM].bXferActive)
    {
        // Get index to first free command buffer.
        ums_uas_get_cmd_index(&gFree_cmd_buffer_list, &gCurrent_cmd_buffer_index);

        // Prepare to Rx Command IU.
        ums_uas_rx_command(gCurrent_cmd_buffer_index);
    }

    return;
}

/*****************************************************************************
 * Function: ums_uas_soft_reset
 *************************************************************************//**
 * This function resets the UAS module and cancels any active transfers on 
 * data IN/OUT endpoints.  Command and Status endpoint transfers remain active.
 *
 * @param init_queue_depth flag to initialize the queue depth.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_soft_reset(BOOLEAN_T init_queue_depth)
{
    if (init_queue_depth)
    {
        CRIT("-> ums_uas_soft_reset()\n");
    }

    // Cancel any active transfers on data IN/OUT EPs.
    usb_hal_cancel_io_request((UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT));
    usb_hal_cancel_io_request((UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN));

    // Initialize wrap window memory.
    mww_init();

    // Clear UAS data.
    ti_memset(&gStatus_pending_list, 0, sizeof(gStatus_pending_list));

    ti_memset(&gPendingData_IN, 0, sizeof(gPendingData_IN));
    ti_memset(&gPendingData_OUT, 0, sizeof(gPendingData_OUT));

    ti_memset(gCheckCond, 0, sizeof(gCheckCond));
    ti_memset(gTagUseCnt, 0, sizeof(gTagUseCnt));
    ti_memset(gSATA_xfer_cmpltd, 0, sizeof(gSATA_xfer_cmpltd));

    ti_memset(&gResponse_order, 0, sizeof(gResponse_order));

    // Init flags.
    gRW_Ready_locked = FALSE;
    gNonQueuedCmdInProgress = FALSE;

    gCmdWaitingCount = 0;

    gCmd_list_input_index = 0;
    gCmd_list_processing_index = 0;
    gProcessing_index_decrement_count = 0;

    gCurrentDataOutIndex = 0;
    gCurrentDataInIndex = 0; 

    if (init_queue_depth)
    {
        // Initialize the queue depth.
        ums_uas_init_queue_depth(0);
    }

    return;
}

/*****************************************************************************
 * Function: ums_uas_reset
 *************************************************************************//**
 * This function sets the UAS state to IDLE, initializes variables
 * and sends an I/O request to the USB HAL to recieve a command IU.
 *
 * @param None.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_reset(void)
{
    CRIT("-> ums_uas_reset()\n");

    // Clear UAS state and Response pending list.
    ti_memset(gUAS_state, 0, sizeof(gUAS_state));   
    ti_memset(&gResponse_pending_list, 0, sizeof(gResponse_pending_list));

    // Cancel any active transfers on command and status endpts.
    usb_hal_cancel_io_request((UMS_UAS_CMD_ENDPT_NUM | ENDPT_DIRECTION_OUT));
    usb_hal_cancel_io_request((UMS_UAS_STATUS_ENDPT_NUM | ENDPT_DIRECTION_IN));

    ums_uas_soft_reset(FALSE);

#if ENABLE_UAS_DATA_IN_PACING
    if (ata_dev[0].bNCQ)
    {
        // Make SATA-TO-USB window look full to SATA core by setting read offset equal to the write offset with the
        // overflow bit flipped.
        WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), READ32(MWWxWRTOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM)) ^ (1 << (0x0A + WRAP_WINDOW_BLOCK_SIZE)));
    }
#endif

    // Register callbacks with AHCI module.
    ahci_register_ata_callbacks(ums_uas_ata_cmd_callback, ums_uas_ata_queued_cmd_callback, ums_uas_ata_error_callback);
    ahci_register_port_init_complete_callback(0, ums_uas_idle);

    if (ata_dev[0].bDeviceInitComplete || ata_dev[0].bDeviceInitTimedOut)
    {
        ums_uas_idle(0);
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_find_cmd_index
 *************************************************************************//**
 * This function returns the command index corresponding to the specified TAG.
 *
 * @param[in] tag byte-swapped value of the TAG.
 * @param[in] cmd_index_ptr pointer to memory location to store command index corresponding to the TAG. Cannot be NULL.
 * @param[in] excluded_cmd_index value of command index to exclude from the search (used when searching for overlapped TAG). Use 0xFF to exclude no indexes.
 * 
 * @retval TRUE when the command index is found.
 * @retval FALSE otherwise.
 *
 ****************************************************************************** 
 */

BOOLEAN_T ums_uas_find_cmd_index(UINT32_T tag, UINT32_T *cmd_index_ptr, UINT32_T excluded_cmd_index)
{
    UINT32_T cmd_index;

    DEBUG("-> ums_uas_find_cmd_index() - tag = 0x%x, excluded_index = 0x%x.\n", tag, excluded_cmd_index);

    for (cmd_index = 0; cmd_index < UMS_UAS_CMD_DEPTH; cmd_index++)
    {
        // Check if tag matches any TAGs in use.
        if ((gCommand_IU[cmd_index]->wTAG == tag) && 
            (gUAS_state[cmd_index] > UMS_UAS_STATE_RX_CMD) &&
            (gUAS_state[cmd_index] < UMS_UAS_STATE_STATUS_SENT))
        {
            if (cmd_index != excluded_cmd_index)
            {
                *cmd_index_ptr = cmd_index;

                return TRUE;
            }
        }
    }

    return FALSE;
}


#define ADDR_METHOD_MASK   0xC0
#define ADDR_METHOD_OFFSET 6

#define EXT_FIELD_LENGTH_MASK   0x30
#define EXT_FIELD_LENGTH_OFFSET 4

#define EXT_ADDR_METHOD_MASK                0x0F
#define EXT_ADDR_METHOD_WLUN                0x01
#define EXT_ADDR_METHOD_EXTENDED_FLAT_SPACE 0x02

#define PERIPHERAL_DEV_ADDR_METHOD  0x0
#define FLAT_SPACE_ADDR_METHOD      0x1
#define LOGICAL_UNIT_ADDR_METHOD    0x2
#define EXTENDED_LUN_ADDR_METHOD    0x3

/*****************************************************************************
 * Function: ums_uas_get_lun
 *************************************************************************//**
 * This function extracts the LUN value.
 *
 * @param[in] lun_field pointer to 64-bit LUN field from the command IU. 
 * @param[in] lun_val pointer to memory to store the LUN value.      
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_ERROR when the address method is not supported.
 *
 ****************************************************************************** 
 */

STATUS_T ums_uas_get_lun(UINT8_T *lun_field, UINT32_T *lun_val)
{
    STATUS_T status = STATUS_OK;
    UINT32_T addr_method;
    UINT32_T ext_addr_method;
    UINT32_T length;
    UINT32_T lun = -1UL;

    addr_method = (lun_field[0] & ADDR_METHOD_MASK) >> ADDR_METHOD_OFFSET;

    // When applicable, Bus field is assumed to be 0x00 (current level).

    switch (addr_method)
    {
        case PERIPHERAL_DEV_ADDR_METHOD:
            lun = lun_field[1];
            break;

        case FLAT_SPACE_ADDR_METHOD:  
            lun =  ((lun_field[0] & ~ADDR_METHOD_MASK) << 8) | lun_field[1];
            break;

        case LOGICAL_UNIT_ADDR_METHOD:
            lun = lun_field[1] & 0x1F;
            break;

        case EXTENDED_LUN_ADDR_METHOD:  
            ext_addr_method = lun_field[0] & EXT_ADDR_METHOD_MASK;
            length = (lun_field[0] & EXT_FIELD_LENGTH_MASK) >> EXT_FIELD_LENGTH_OFFSET;
            if ((ext_addr_method == EXT_ADDR_METHOD_WLUN) && (length == 0))
            {
                lun = lun_field[1];
            }
            else if ((ext_addr_method == EXT_ADDR_METHOD_EXTENDED_FLAT_SPACE) && (length == 1))
            {
                lun = (lun_field[1] << 16) | (lun_field[2] << 8) | lun_field[3]; 
            }
            else
            {
                // Invalid LUN field.
                status = STATUS_ERROR;
            }
            break;

        default:
            CRIT("@Error: LUN address method = %u not supported.\n", addr_method);
            status = STATUS_ERROR;
            break;
    }

    INFO("-> ums_uas_get_lun() - addr_method = %u, lun = %u.\n", addr_method, lun);

    if (lun_val)
    {
        *lun_val = lun;
    }

    return status;
}

/*****************************************************************************
 * Function: ums_uas_send_response_iu
 *************************************************************************//**
 * This function sends a Response IU on the status pipe.
 *
 * @param[in] cmd_index index of the command.
 * @param[in] response_code response code for task management function.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_send_response_iu(UINT32_T cmd_index, UINT32_T response_code)
{
    STATUS_T status;
    UAS_RESPONSE_IU_T *response_iu;

    CRIT("-> ums_uas_send_response_iu() - cmd_index = %u, resp_code = 0x%x, tag = 0x%x, retry = %s.\n", 
         cmd_index, response_code,  BSWAP_16(gCommand_IU[cmd_index]->wTAG), (gUAS_state[cmd_index] == UMS_UAS_STATE_RESPONSE_PENDING) ? "Y" : "N");

    response_iu = (UAS_RESPONSE_IU_T *)&datapath_ram->ums_status_buffer[cmd_index];

    ti_memset(response_iu, 0, sizeof(UAS_RESPONSE_IU_T));
    response_iu->bIU_ID = UAS_RESPONSE_IU;
    response_iu->wTAG = gCommand_IU[cmd_index]->wTAG;
    response_iu->bResponseCode = response_code;

    status = ums_uas_tx_status(response_iu, sizeof(UAS_RESPONSE_IU_T), cmd_index);

    if (status == STATUS_OK)
    {
        // Set dummy OPCODE to prevent gRW_Ready_locked from being cleared unintentionally in status pipe callback.
        gCommand_IU[cmd_index]->CDB[0] = DUMMY_SCSI_OPCODE;

        // Set SATA xfer complete flag to TRUE so IDLE state will be set in status pipe callback.
        gSATA_xfer_cmpltd[cmd_index] = TRUE;
        gUAS_state[cmd_index] = UMS_UAS_STATE_STATUS_SENT;
    }
    else if (status == STATUS_XFER_ACTIVE)
    {
        if (gUAS_state[cmd_index] != UMS_UAS_STATE_RESPONSE_PENDING)
        {
            // Set UAS state to status pending so Sense IU will be sent out when status pipe is free.
            gUAS_state[cmd_index] = UMS_UAS_STATE_RESPONSE_PENDING;

            // Add item to pending list.
            gResponse_pending_list.valid_flag[gResponse_pending_list.index] = TRUE;
            gResponse_pending_list.cmd_index[gResponse_pending_list.index] = cmd_index;
            increment_index_plus(&gResponse_pending_list.index);
        }
        else
        {
            // Keep item on pending list.
            decrement_index_plus(&gResponse_pending_list.processing_index);
            gResponse_pending_list.valid_flag[gResponse_pending_list.processing_index] = TRUE;
        }
    }

    return;
}



/*****************************************************************************
 * Function: ums_uas_handle_task_mgmt_iu
 *************************************************************************//**
 * This function handles a Task Management IU. 
 *
 * @param[in] cmd_buffer_index index of the command buffer where the Task Management IU is stored.      
 *
 * @retval None.
 *
 ****************************************************************************** 
 */
inline void ums_uas_handle_task_mgmt_iu(UINT32_T cmd_buffer_index)
{
    UAS_TASK_MGMT_IU_T *task_iu = (UAS_TASK_MGMT_IU_T *)gCommand_IU[cmd_buffer_index];
    UINT32_T cmd_index; 

    CRIT("-> ums_uas_handle_task_mgmt_IU() - task = 0x%x, task_tag = 0x%x, tag = 0x%x.\n", task_iu->bTaskMgmtFunction, BSWAP_16(task_iu->wTaskTAG), BSWAP_16(gCommand_IU[cmd_buffer_index]->wTAG));

    // Validate LUN.
    if (gLUN[cmd_buffer_index] > UMS_MAX_LUN)
    {
        ums_uas_send_response_iu(cmd_buffer_index, RC_INCORRECT_LUN);
        return;
    }

    switch (task_iu->bTaskMgmtFunction)
    {
        case TMF_CLEAR_TASK_SET:    
            /*****************************************************************************************************************
             * All pending status and sense data for the task set shall be cleared. Other previously established conditions,
             * including mode parameters, reservations, and ACA shall not be changed by the CLEAR TASK SET function.
             ****************************************************************************************************************/
            // There is one task set per port so, Clear Task Set is equivalent to Abort Task Set.
        case TMF_ABORT_TASK_SET: 
            /*****************************************************************************************************************
             * All pending status and sense data for the commands that were aborted shall be cleared. Other previously
             * established conditions, including mode parameters, reservations, and ACA shall not be changed by the ABORT
             * TASK SET function
             ****************************************************************************************************************/           
            // Since we only support 1 port, just reset logical unit.
        case TMF_LOGICAL_UNIT_RESET:
            /*****************************************************************************************************************
             * When responding to a logical unit reset condition, the logical unit shall:
             * a) abort all commands as described in 5.6;
             * b) terminate all task management functions;
             * c) clear all ACA conditions (see 5.9.5) in all task sets in the logical unit;
             * d) establish a unit attention condition (see 5.14 and 6.2);
             * e) initiate a logical unit reset for all dependent logical units (see 4.6.19.4); and
             * f) perform any additional functions required by the applicable command standards.
             ****************************************************************************************************************/       
        case TMF_I_T_NEXUS_RESET: 
            /*****************************************************************************************************************
             * When responding to a I_T Nexus Reset condition, the logical unit shall:
             * a) abort all commands received on the I_T nexus as described in 5.6;
             * b) terminate all task management functions received on the I_T nexus;
             * c) clear all ACA conditions (see 5.9.5) associated with the I_T nexus;
             * d) establish a unit attention condition for the SCSI initiator port associated with the I_T nexus (see 5.14 and
             * 6.2); and
             * e) perform any additional functions required by the applicable command standards.
            ****************************************************************************************************************/
            DUMP_UAS_STATE();
            // Reset SATA device.
            ahci_reset_lun(gLUN[cmd_buffer_index], TRUE);

            // Reset UAS module.
            ums_uas_soft_reset(TRUE);

            // Increment tag use counter. (was cleared by ums_uas_soft_reset)
            gTagUseCnt[BSWAP_16(task_iu->wTAG) & TAG_HASH_INDEX_MASK]++; 

            ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_CMPLT);           
            break;

        case TMF_ABORT_TASK:
            /*****************************************************************************************************************
             * The task manager shall abort the specified command, if it exists, as described in 5.6. Previously established
             * conditions, including mode parameters, reservations, and ACA shall not be changed by the ABORT TASK function.
             * A response of FUNCTION COMPLETE shall indicate that the command was aborted or was not in the task set. In
             * either case, the SCSI target device shall guarantee that no further requests or responses are sent from the
             * command 
             ****************************************************************************************************************/
            if (ums_uas_find_cmd_index(task_iu->wTaskTAG, &cmd_index, 0xFF))
            {
                if ((gUAS_state[cmd_index] > UMS_UAS_STATE_RX_CMD) &&
                    (gUAS_state[cmd_index] <= UMS_UAS_STATE_QUEUED_CMD_WAITING))
                {
                    // Command has not been sent to the SATA drive yet so we can abort it.
                    gUAS_state[cmd_index] = UMS_UAS_STATE_IDLE;
                    ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_CMPLT);
                    if (--gCmdWaitingCount < 0)
                        gCmdWaitingCount = 0;
                }
                else
                {
                    // The command cannot be aborted.
                    ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_FAILED);
                }
            }
            else
            {
                // The specified task does not exist.
                ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_FAILED);
            }
            break;

        case TMF_CLEAR_ACA:         
        case TMF_QUERY_TASK:        
        case TMF_QUERY_TASK_SET:    
        case TMF_QUERY_ASYNC_EVENT: 
            ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_FAILED);  /* to pass CV test */
            //ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_NOT_SUPPORTED);
            break;

        default:
            ums_uas_send_response_iu(cmd_buffer_index, RC_TASK_MGMT_FUNCTION_NOT_SUPPORTED);
            //ums_uas_send_response_iu(cmd_buffer_index, RC_INVALID_INFORMATION_UNIT);
            break;
    }

    return;
}



/* Task Attributes */
#define TA_SIMPLE        0x0
#define TA_HEAD_OF_QUEUE 0x1
#define TA_ORDERED       0x2
#define TA_ACA           0x4

#define TASK_ATTRIBUTE_MASK 0x07

/*****************************************************************************
 * Function: ums_uas_handle_command_iu
 *************************************************************************//**
 * This function handles a Command IU.  Task attributes are not supported.
 *
 * @param[in] cmd_buffer_index index of the command buffer where the IU is stored.      
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ums_uas_handle_command_iu(UINT32_T cmd_buffer_index)
{
    BOOLEAN_T process_cmd = FALSE;
    UAS_COMMAND_IU_T *cmd_iu;

    // Set command IU pointer to appropriate command buffer.
    cmd_iu = (UAS_COMMAND_IU_T *)&datapath_ram->ums_cmd_buffer[cmd_buffer_index].command;

    DEBUG("-> ums_uas_handle_command_IU() - cmd_index = %u, LUN = %u, TAG = 0x%x, CDB[0] = 0x%02x.\n",
          cmd_buffer_index, gLUN[cmd_buffer_index], BSWAP_16(cmd_iu->wTAG), cmd_iu->CDB[0]);

    // Validate LUN.
    if (gLUN[cmd_buffer_index] > UMS_MAX_LUN)
    {
        CRIT("@Error: Invalid LUN!\n");
        scsi_set_sense_data(ILLEGAL_REQUEST, LOGICAL_UNIT_NOT_SUPPORTED, NO_ASCQ);
        ums_uas_send_sense_iu(cmd_buffer_index, SCSI_STATUS_CHECK_CONDITION);
        return;
    }

    // Check for task attribute support.
//    if (cmd_iu->bCmdPriority & TASK_ATTRIBUTE_MASK)
//    {
//        // BQ - we treat all priorities as the same currently.
//        INFO("task priority = 0x%x.\n");
//    }

    if (ata_dev[gLUN[cmd_buffer_index]].bNCQ && scsi_is_rw_cmd(cmd_iu->CDB[0]))
    {
        // Verify there are no non-queued commands in progess and no commands waiting.
        if ((gCmdWaitingCount == 0) && !gNonQueuedCmdInProgress && 
            !(READ32(PxSACT(gLUN[cmd_buffer_index])) & (1 << cmd_buffer_index)))
        {
            process_cmd = TRUE;
        }
        else
        {
            DEBUG("non-queued cmds pending...wait to process.\n");
            gUAS_state[cmd_buffer_index] = UMS_UAS_STATE_QUEUED_CMD_WAITING;
            gCmdWaitingCount++;
        }
    }
    else /* Non-Read/Write command (Non-Queued command) */
    {
        // Make sure there are no commands in progress or waiting.
        if (!gNonQueuedCmdInProgress && (gCmdWaitingCount == 0) && 
            !(READ32(PxSACT(gLUN[cmd_buffer_index])) | READ32(PxCI(gLUN[cmd_buffer_index]))))
        {
            process_cmd = TRUE;
            gNonQueuedCmdInProgress = TRUE;
        }
        else
        {
            INFO("cmds pending...wait to process.\n");
            gUAS_state[cmd_buffer_index] = UMS_UAS_STATE_CMD_WAITING;
            gCmdWaitingCount++;

            INFO("gUAS_state[%u] = COMMAND WAITING, gCmdWaitingCount = %u, scsi_cmd = 0x%x\n", cmd_buffer_index, gCmdWaitingCount,
                 gCommand_IU[cmd_buffer_index]->CDB[0]);
        }
    }

    if (process_cmd)
    {
        ums_uas_process_cmd(cmd_buffer_index);
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_process_iu
 *************************************************************************//**
 * This function handles a Command IU.  Task attributes are not supported.
 *
 * @param[in] cmd_buffer_index index of the command buffer where the IU is stored.      
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

inline void ums_uas_process_iu(UINT32_T saved_index)
{
    STATUS_T status = STATUS_OK;
    UAS_CMD_PIPE_IU_T *cmd_pipe_iu;
    UINT32_T lun;
    UINT32_T tag;
    UINT32_T cmd_index;

    // Save command reception order.
    gCmd_index_list[gCmd_list_input_index] = saved_index;
    // Increment command list index.
    gCmd_list_input_index++;
    gCmd_list_input_index &= CMD_INDEX_LIST_MASK;

    DEBUG("gCmd_list_input_index = %u.\n", gCmd_list_input_index);

    // Set pointer to appropriate command buffer.
    cmd_pipe_iu = (UAS_CMD_PIPE_IU_T *)&datapath_ram->ums_cmd_buffer[saved_index].command;

    // Validate IU ID.
    if ((cmd_pipe_iu->bIU_ID != UAS_COMMAND_IU) &&
        (cmd_pipe_iu->bIU_ID != UAS_TASK_MANAGEMENT_IU))
    {
        CRIT("@Error: Invalid IU_ID = 0x%x.\n", cmd_pipe_iu->bIU_ID);
        ums_uas_send_response_iu(saved_index, RC_INVALID_INFORMATION_UNIT);
        status = STATUS_ERROR;                   
    }

    if (status == STATUS_OK)
    {
        // Determine LUN.
        ums_uas_get_lun(cmd_pipe_iu->bLUN, &lun);

        if ((lun == SCSI_REPORT_LUNS_WLUN) &&
            (cmd_pipe_iu->bIU_ID == UAS_COMMAND_IU) && 
            (cmd_pipe_iu->CDB[0] == SCSI_REPORT_LUNS))
        {
            // Treat Well-known LUN for Report LUNs command like LUN 0.
            lun = 0;
        }
    }

    if (status == STATUS_OK)
    {
        // Byte-swap transfer TAG.
        tag = BSWAP_16(cmd_pipe_iu->wTAG);

        // Increment TAG count in hash.
        if (gTagUseCnt[tag & TAG_HASH_INDEX_MASK]++ > 0)
        {
            // Potential overlapped TAG.  Search through command queue.
            if (ums_uas_find_cmd_index(cmd_pipe_iu->wTAG, &cmd_index, saved_index))
            {
                CRIT("@Warning: UAS Tag 0x%x overlap for index = %u! gUAS_state[%u] = %u, IU_ID = %u.\n", tag, saved_index, 
                     cmd_index, gUAS_state[cmd_index], cmd_pipe_iu->bIU_ID);

                if ((cmd_pipe_iu->bIU_ID == UAS_COMMAND_IU) && (gCommand_IU[cmd_index]->bIU_ID == UAS_COMMAND_IU))
                {
                    // Abort all Task Mgmt functions. 
                    // Currently we process all TM functions immediately so no action is required here.

                    // Respond to overlapped command according to SAM-5.
                    scsi_set_sense_data(ABORTED_COMMAND, OVERLAPPED_CMDS_ATTEMPTED, NO_ASCQ);
                    ums_uas_send_sense_iu(saved_index, SCSI_STATUS_CHECK_CONDITION);
                }
                else
                {
                    // Abort all commands and task mgnt functions for the target port.
                    ahci_reset_lun(lun, FALSE);

                    // For now, just reset the entire UAS module since we only support a single port.
                    ums_uas_soft_reset(TRUE);

                    // Increment tag use counter which was cleared during UAS reset.
                    gTagUseCnt[tag & TAG_HASH_INDEX_MASK]++; 

                    // Send Response IU with response code set to OVERLAPPED TAG ATTEMPTED.
                    ums_uas_send_response_iu(saved_index, RC_OVERLAPPED_TAG_ATTEMPTED);
                }

                status = STATUS_ERROR;                   
            }
        }
    }

    if (status == STATUS_OK)
    {
        // Save LUN.
        gLUN[saved_index] = lun;

        if (cmd_pipe_iu->bIU_ID == UAS_COMMAND_IU)
        {
            // Process command IU.
            ums_uas_handle_command_iu(saved_index);       
        }
        else if (cmd_pipe_iu->bIU_ID == UAS_TASK_MANAGEMENT_IU)
        {
            // Process Task management IU.
            ums_uas_handle_task_mgmt_iu(saved_index);
        }
        else
        {
            CRIT("@Error: Unknown IU_ID = 0x%x.\n", cmd_pipe_iu->bIU_ID);
            ums_uas_send_response_iu(saved_index, RC_INVALID_INFORMATION_UNIT);
        }
    }

    return;
}

/*****************************************************************************
 * Function: ums_uas_command_pipe_callback
 *************************************************************************//**
 * This function is called by USB stack when it receives transfer 
 * complete event on the Command pipe.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_command_pipe_callback(EP_INFO_T *ep_info)
{
    UINT32_T saved_index;
    STATUS_T status;

    // Check if overflow command buffer is in use.
    if (gCurrent_cmd_buffer_index >= gCmdQueueDepth)
    {
        // Save current buffer index.
        saved_index = gCurrent_cmd_buffer_index;

        // Check for empty non-overflow command command buffer.
        if (STATUS_OK == ums_uas_get_cmd_index(&gFree_cmd_buffer_list, &gCurrent_cmd_buffer_index))
        {
            // Copy command into the free index.
            ti_memcpy(gCommand_IU[gCurrent_cmd_buffer_index], gCommand_IU[saved_index], sizeof(UMS_COMMAND_T)); 
            gUAS_state[saved_index] = UMS_UAS_STATE_IDLE;
        }
    }

    // Set UAS state.
    gUAS_state[gCurrent_cmd_buffer_index] = UMS_UAS_STATE_CMD_RECEIVED;

    // Save current buffer index.
    saved_index = gCurrent_cmd_buffer_index;

    // Look for empty command buffer to Rx next Command IU.
    status = ums_uas_get_cmd_index(&gFree_cmd_buffer_list, &gCurrent_cmd_buffer_index);

    if (status != STATUS_OK)
    {
        status = find_empty_cmd_overflow_buffer_index(&gCurrent_cmd_buffer_index);

        if (status != STATUS_OK)
        {
            CRIT("@Error: No cmd buff avail!\n");
            ums_uas_dump_state();
        }
    }

    if (status == STATUS_OK)
    {
        // Prepare to Rx next Command IU.
        ums_uas_rx_command(gCurrent_cmd_buffer_index);
    }

    if (saved_index < gCmdQueueDepth)
    {
        ums_uas_process_iu(saved_index);
    }
    else
    {
        // Set dummy OPCODE to prevent gRW_Ready_locked from being cleared unintentionally in status pipe callback.
        gCommand_IU[saved_index]->CDB[0] = DUMMY_SCSI_OPCODE;

        // Command queue overflow.  Send Sense IU.  
        ums_uas_send_sense_iu(saved_index, SCSI_STATUS_TASK_SET_FULL);      
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_status_pipe_callback
 *************************************************************************//**
 * This function is called by USB stack when it receives transfer 
 * complete event on the Status pipe (could be from sending Sense IU, 
 * Response IU, or RW Ready IU.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_status_pipe_callback(EP_INFO_T *ep_info)
{
    STATUS_T status;
    UINT32_T cmd_index;
    UINT32_T hash_index;
    UAS_RESPONSE_IU_T *response_iu;

    INFO("-> ums_uas_status_pipe_callback() - gUAS_state[%u] = %u.\n", gCurrent_status_index, gUAS_state[gCurrent_status_index]);

    if (gUAS_state[gCurrent_status_index] == UMS_UAS_STATE_STATUS_SENT)
    {
        if (gCommand_IU[gCurrent_status_index]->CDB[0] != DUMMY_SCSI_OPCODE)
        {
            // Clear RW Ready locked flag.
            gRW_Ready_locked = FALSE; 

            if (!scsi_is_rw_cmd(gCommand_IU[gCurrent_status_index]->CDB[0]) || !ata_dev[gLUN[gCurrent_status_index]].bNCQ)
            {
                // Clear non-queued command in-progress flag.
                gNonQueuedCmdInProgress = FALSE;
            }
        }

        if (gSATA_xfer_cmpltd[gCurrent_status_index])
        {
            // Set IDLE state here so command index can be reused.
            gUAS_state[gCurrent_status_index] = UMS_UAS_STATE_IDLE;  

            if (gCurrent_status_index < gCmdQueueDepth)
            {
                // Decrement TAG count in hash.
                hash_index = BSWAP_16(gCommand_IU[gCurrent_status_index]->wTAG) & TAG_HASH_INDEX_MASK;
                if (--gTagUseCnt[hash_index] < 0)
                {
                    CRIT("@Error: gTagUseCnt[%u] went negative!\n", hash_index);
                    gTagUseCnt[hash_index] = 0;
                }

                // Add cmd buffer to empty buffer list.
                gFree_cmd_buffer_list.cmd_index[gFree_cmd_buffer_list.index] = gCurrent_status_index;
                gFree_cmd_buffer_list.valid_flag[gFree_cmd_buffer_list.index] = TRUE;
                increment_index(&gFree_cmd_buffer_list.index);
            }
        }

        if (gCmdWaitingCount)
        {
            ums_uas_process_pending_cmds();
        }
    }

    // Check for any pending Status IU to send.
    if (ums_uas_get_cmd_index_plus(&gStatus_pending_list, &cmd_index) == STATUS_OK)
    {
        DEBUG("-> ums_uas_status_pipe_callback() - send pending Sense IU for cmd_index = %u.\n", cmd_index);
        ums_uas_retry_send_sense_iu(cmd_index); 
    }
    // Check for any pending Response IU to send.
    else if (ums_uas_get_cmd_index_plus(&gResponse_pending_list, &cmd_index) == STATUS_OK)
    {
        DEBUG("-> ums_uas_status_pipe_callback() - send pending Response IU.\n");
        response_iu = (UAS_RESPONSE_IU_T *)&datapath_ram->ums_status_buffer[cmd_index];
        ums_uas_send_response_iu(cmd_index, response_iu->bResponseCode);
    }
    else if ((usb_dev.dev_speed == USB_HIGH_SPEED) && !gRW_Ready_locked)
    {
        if (gPendingData_OUT.xfer_pending && (gUAS_state[gPendingData_OUT.cmd_index] == UMS_UAS_STATE_WRITE_READY_PENDING))
        {
            INFO("-> ums_uas_status_pipe_callback() - send pending Write Ready IU.\n");

            status = ums_uas_send_rw_ready_iu(gPendingData_OUT.cmd_index); 

            if (status == STATUS_OK)
            {
                // Rx data over USB.
                status = ums_uas_rx_data(gPendingData_OUT.data_buff, gPendingData_OUT.byte_cnt, gPendingData_OUT.cmd_index);

                if (status == STATUS_OK)
                {
                    gPendingData_OUT.xfer_pending = FALSE;
                }
            }
        }
        else if (gUAS_state[gPendingData_IN.cmd_index[gPendingData_IN.processing_index]] == UMS_UAS_STATE_READ_READY_PENDING)
        {
            INFO("-> ums_uas_status_pipe_callback() - send pending Read Ready IU.\n");

            // Send R/W Ready IU.
            status = ums_uas_send_rw_ready_iu(gPendingData_IN.cmd_index[gPendingData_IN.processing_index]);

            if (status == STATUS_OK)
            {
                INFO("-> ums_uas_status_pipe_callback() - Tx %u-bytes from 0x%x. cmd_index=%u.\n",  gPendingData_IN.byte_cnt[gPendingData_IN.processing_index], 
                     gPendingData_IN.data_buff[gPendingData_IN.processing_index], gPendingData_IN.cmd_index[gPendingData_IN.processing_index]);

                // Set UAS state to DATA IN Pending.
                gUAS_state[gPendingData_IN.cmd_index[gPendingData_IN.processing_index]] = UMS_UAS_STATE_DATA_IN_PENDING;

                // Tx data over USB.
                status = ums_uas_tx_data(gPendingData_IN.data_buff[gPendingData_IN.processing_index],
                                         gPendingData_IN.byte_cnt[gPendingData_IN.processing_index], 
                                         gPendingData_IN.cmd_index[gPendingData_IN.processing_index]);   

                if (status == STATUS_OK)
                {
                    gPendingData_IN.xfer_pending[gPendingData_IN.processing_index] = FALSE;
                    increment_index(&gPendingData_IN.processing_index);
                }
            }
        }
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_data_OUT_pipe_callback
 *************************************************************************//**
 * This function is called by USB stack when it receives transfer 
 * complete event on the DATA-OUT pipe.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_data_OUT_pipe_callback(EP_INFO_T *ep_info)
{
    STATUS_T status;

    CRIT("-> ums_uas_data_OUT_pipe_callback() - sid = 0x%x.\n", ep_info->wStreamID);

    if (gUAS_state[gCurrentDataOutIndex] == UMS_UAS_STATE_DATA_OUT)
    {
        if (!scsi_is_rw_cmd(gCommand_IU[gCurrentDataOutIndex]->CDB[0]))
        {
            CRIT("Issue cmd 0x%x\n", gCommand_IU[gCurrentDataOutIndex]->CDB[0]);
            ums_uas_cmd.dDataXferLength = ep_info->dByteCount;
            ums_uas_issue_scsi_cmd(gCurrentDataOutIndex);
        }

        if (gSATA_xfer_cmpltd[gCurrentDataOutIndex])
        {
            // Send status.
            ums_uas_send_sense_iu(gCurrentDataOutIndex, SCSI_STATUS_GOOD);
        }
        else
        {
            // Set UAS state to Send Sense.
            gUAS_state[gCurrentDataOutIndex] = UMS_UAS_STATE_SEND_SENSE;
        }
    }
    else
    {
        CRIT("@Error: Cmd index %u is not Data-OUT (%u).\n", gCurrentDataOutIndex, gUAS_state[gCurrentDataOutIndex]);
        DUMP_UAS_STATE();
    }

    // Check for pending Data-OUT transfers.
    if ((gPendingData_OUT.xfer_pending) &&
        (gUAS_state[gPendingData_OUT.cmd_index] == UMS_UAS_STATE_DATA_OUT_PENDING))
    {
        // Rx data over USB.
        status = ums_uas_rx_data(gPendingData_OUT.data_buff, gPendingData_OUT.byte_cnt, gPendingData_OUT.cmd_index);   

        if (status == STATUS_OK)
        {
            gPendingData_OUT.xfer_pending = FALSE;
        }
    }

    return;
}


/*****************************************************************************
 * Function: ums_uas_data_IN_pipe_callback
 *************************************************************************//**
 * This function is called by USB stack when it receives transfer 
 * complete event on the DATA-IN pipe.
 *
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_data_IN_pipe_callback(EP_INFO_T *ep_info)
{
    STATUS_T status;

    DEBUG("-> ums_uas_data_IN_pipe_callback() - sid = 0x%x.\n", ep_info->wStreamID);

    if (gUAS_state[gCurrentDataInIndex] == UMS_UAS_STATE_DATA_IN)
    {
        if (gSATA_xfer_cmpltd[gCurrentDataInIndex])
        {
            // Send status.
            ums_uas_send_sense_iu(gCurrentDataInIndex, SCSI_STATUS_GOOD);
        }
        else
        {
            // Set UAS state to Send Sense.
            gUAS_state[gCurrentDataInIndex] = UMS_UAS_STATE_SEND_SENSE;
        }
    }
    else
    {
        CRIT("@Error: Cmd index %u is not Data-IN (%u).\n", gCurrentDataInIndex, gUAS_state[gCurrentDataInIndex]);
        DUMP_UAS_STATE();
    }

    // Check for pending Data-IN transfers.
    if ((gPendingData_IN.xfer_pending[gPendingData_IN.processing_index]) &&
        (gUAS_state[gPendingData_IN.cmd_index[gPendingData_IN.processing_index]] == UMS_UAS_STATE_DATA_IN_PENDING))
    {
        // Tx data over USB.
        status = ums_uas_tx_data(gPendingData_IN.data_buff[gPendingData_IN.processing_index],
                                 gPendingData_IN.byte_cnt[gPendingData_IN.processing_index], 
                                 gPendingData_IN.cmd_index[gPendingData_IN.processing_index]);   

        if (status == STATUS_OK)
        {
            gPendingData_IN.xfer_pending[gPendingData_IN.processing_index] = FALSE;
            increment_index(&gPendingData_IN.processing_index);
        }
    }

    return;
}

#if 0
/*****************************************************************************
 * Function: ums_uas_power_mgmt_callback
 *************************************************************************//**
 * This function is registered with the USB stack to handle SATA power
 * management.
 *
 * @param[in] pm_state current USB power state.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_power_mgmt_callback(eUSB_DEVICE_PM_STATE_T pm_state)
{
    UINT32_T cmd_index;

    DEBUG("-> ums_uas_power_mgmt_callback() - pm_state = %u.\n", pm_state);

    if ((pm_state == USB_PM_RESET) || (pm_state == USB_PM_DISCONNECT))
    {
        for (cmd_index = 0; cmd_index < gCmdQueueDepth; cmd_index++)
        {
            gUAS_state[cmd_index] = UMS_UAS_STATE_IDLE;
            gCmd_index_list[cmd_index] = 0;
        }        
    }

    return;
}
#endif


/*****************************************************************************
 * Function: ums_uas_init
 *************************************************************************//**
 * This function initializes the USB Attached SCSI layer.
 *
 * @param None.      
 *
 * @retval None.
 *
 ****************************************************************************** 
 */

void ums_uas_init(void)
{
    UINT32_T cmd_index;

    DEBUG("-> ums_uas_init().\n");

    // Register data xfer callbacks.
    usb_stack_register_UAS_data_xfer_callback((UMS_UAS_STATUS_ENDPT_NUM | ENDPT_DIRECTION_IN), ums_uas_status_pipe_callback);
    usb_stack_register_UAS_data_xfer_callback((UMS_UAS_CMD_ENDPT_NUM | ENDPT_DIRECTION_OUT), ums_uas_command_pipe_callback);

    usb_stack_register_UAS_data_xfer_callback((UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), ums_uas_data_IN_pipe_callback);
    usb_stack_register_UAS_data_xfer_callback((UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT), ums_uas_data_OUT_pipe_callback);

    // Register UAS reset callback.
    usb_stack_register_UAS_reset_callback(ums_uas_reset);   

//    // Register power management callback.
//    usb_stack_register_PM_callback(ums_uas_power_mgmt_callback);

    // Clear out command and status buffers.
    ti_memset(&datapath_ram->ums_cmd_buffer, 0, sizeof(datapath_ram->ums_cmd_buffer));
    ti_memset(&datapath_ram->ums_status_buffer, 0, sizeof(datapath_ram->ums_status_buffer));

    // Set pointers to command IU buffers.
    for (cmd_index = 0; cmd_index < UMS_UAS_CMD_DEPTH; cmd_index++)
    {
        gCommand_IU[cmd_index] = (UAS_COMMAND_IU_T *)&datapath_ram->ums_cmd_buffer[cmd_index].command;
    }

    return;
}



