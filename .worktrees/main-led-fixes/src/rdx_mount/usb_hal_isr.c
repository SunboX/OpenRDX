/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hal_isr.c
//
// Project     : TUSB926x Firmware.
//
// Description : Interrupt service routine for the USB HAL.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   01/15/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 *
 * This file contains the implementation of the interrupt service routine for the USB hardware abstraction layer.
 *
 */

#include "usb_hal.h"
#include "ahci.h" /* ata_dev */
#include "gio.h"
#include "mww.h"
#include "reg_io.h"
#include "rti.h"
#include "sci.h"
#include "string.h"
#include "system.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
#include "ums_uas.h"  // ENABLE_UAS_DATA_IN_PACING
#include "vim_nvic.h"
#include "wdt.h"


/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#if USB_HAL_DEBUG_DISABLE
    #define kprintf(str, args...)  {}
#endif

/*----------------------------------------------------------------------------+
| Global Variables                                                            |
+----------------------------------------------------------------------------*/

#if USB_CORE_RESET_TEST
BOOLEAN_T gUSBCoreReset = FALSE;
#endif


/*****************************************************************************
 * Function: config_connection_speed_gio
 *************************************************************************//**
 * This function configures the general-purpose I/O's depending on the
 * USB connection speed.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void config_connection_speed_gio(void)
{
    switch (usb_dev.dev_speed)
    {
        case USB_SUPER_SPEED:
            if (emulation_platform)
            {
                // The USB speed indicator LEDs are connected opposite of the other LEDs on the FPGA board.
//                gio_low(HS_USB_LED_GIO_NUM);
                gio_high(SS_USB_LED_GIO_NUM);
            }
            else
            {
//                LED_OFF(HS_USB_LED_GIO_NUM);
                LED_ON(SS_USB_LED_GIO_NUM);
            }
            break;

        case USB_HIGH_SPEED:
        case USB_FULL_SPEED:
            if (emulation_platform)
            {
                // The USB speed indicator LEDs are connected opposite of the other LEDs on the FPGA board.
//                gio_high(HS_USB_LED_GIO_NUM);
                gio_low(SS_USB_LED_GIO_NUM);
            }
            else
            {
//                LED_ON(HS_USB_LED_GIO_NUM);
                LED_OFF(SS_USB_LED_GIO_NUM);
            }
            break;

        default: /* No connection */
            if (emulation_platform)
            {
                // The USB speed indicator LEDs are connected opposite of the other LEDs on the FPGA board.
//                gio_low(HS_USB_LED_GIO_NUM);
                gio_low(SS_USB_LED_GIO_NUM);
            }
            else
            {
//                LED_OFF(HS_USB_LED_GIO_NUM);
                LED_OFF(SS_USB_LED_GIO_NUM);
            }
            break;
    }

#if ENABLE_U1_U2_TRANSITIONS
    // Drive USB3 link state GIOs low. (in case we didn't get disconnect notification).
    LED_OFF(SS_LINK_STATE_LED0_GIO_NUM);
    LED_OFF(SS_LINK_STATE_LED1_GIO_NUM);
#endif

    return;
}


/*****************************************************************************
 * Function: usb_hal_handle_ep0_xfer_cmplt
 *************************************************************************//**
 * This function handles the transfer complete event for EP0.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] ep_info pointer to endpoint information structure.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_ep0_xfer_cmplt(UINT32_T ep_num, EP_INFO_T *ep_info)
{
    INFO("-> usb_hal_handle_ep0_xfer_cmplt() - ep0_state = %u.\n", usb_dev.ep0_state);

    if (usb_dev.ep0_state == EP0_STATE_IDLE)  /* Setup phase */
    {
        // Copy setup pkt.
        ti_memcpy(&usb_dev.setup_packet, (void*)&datapath_ram->ep0_buffer[0], SETUP_PACKET_DATA_LENGTH);

        // Save xfer stage info for setup of status TRB.
        usb_dev.ep0_three_stage_xfer = (usb_dev.setup_packet.wLength == 0) ? FALSE : TRUE;

        // Call USB stack setup packet callback.
        pUsbStackSetupPktCallback();
    }
    else /* Data or Status phase */
    {
        // Call USB stack data xfer callback.
        pUsbStackDataXferCallback(ep_num, ep_info);

        if (usb_dev.ep0_state == EP0_STATE_IDLE)
        {
            // Status stage complete. Prepare to receive setup packet.
            usb_hal_ep0_setup_stage();
        }
    }

    return;
}


/* Xfer Not Ready event status */
#define EVNT_STATUS_CONTROL_STAGE_MASK   0x00003000
#define EVNT_STATUS_CONTROL_STAGE_OFFSET 12

#define CONTROL_SETUP_STAGE  0x0
#define CONTROL_DATA_STAGE   0x1
#define CONTROL_STATUS_STAGE 0x2

/*****************************************************************************
 * Function: usb_hal_handle_ep0_xfer_not_ready
 *************************************************************************//**
 * This function handles the transfer not ready event for EP0.
 *
 * @param[in] event 32-bit event value from the event buffer.
 * @param[in] direction endpoint direction. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_ep0_xfer_not_ready(UINT32_T event, ENDPT_DIR_T direction)
{
    UINT32_T control_stage;

    // Determine control stage from event bits.
    control_stage = (event & EVNT_STATUS_CONTROL_STAGE_MASK) >> EVNT_STATUS_CONTROL_STAGE_OFFSET;

    INFO("-> usb_hal_handle_ep0_xfer_not_ready() - %s, ctrl_stage = %u, ep0_state = %u.\n",
         (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", control_stage, usb_dev.ep0_state);

    // Check for XferNotReady (Data/Status) when expecting a setup packet.
    if ((usb_dev.ep0_state == EP0_STATE_IDLE) && (control_stage != CONTROL_SETUP_STAGE))
    {
        usb_dev.ep0_state = EP0_STATE_ERROR;
    }

    if (control_stage == CONTROL_STATUS_STAGE)
    {
        if (usb_dev.ep0_three_stage_xfer)
        {
            // Check if data stage was successful.
            if (((direction == ENDPT_DIRECTION_IN) && usb_dev.ep_info_OUT[EP0].dBytesRemaining) ||
                ((direction == ENDPT_DIRECTION_OUT) && usb_dev.ep_info_IN[EP0].dBytesRemaining))
            {
                CRIT("@Error: XferNotReady(Status) data stage failure!\n");
                usb_dev.ep0_state = EP0_STATE_ERROR;
            }
        }

        if (usb_dev.ep0_state != EP0_STATE_ERROR)
        {
            usb_hal_ep0_status_stage(direction);
        }
    }
    else if (control_stage == CONTROL_DATA_STAGE)
    {
        // Check for XferNotReady(Data) with unexpected data direction.
        if (((usb_dev.ep0_state == EP0_STATE_DATA_IN) && (direction == ENDPT_DIRECTION_OUT)) ||
            ((usb_dev.ep0_state == EP0_STATE_DATA_OUT) && (direction == ENDPT_DIRECTION_IN)))
        {
            CRIT("@Error: XferNotReady(Data-%s) direction mismatch!\n",
                  (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

            usb_dev.ep0_state = EP0_STATE_ERROR;
        }
        else if (usb_dev.ep0_zlp_pending)
        {
            if (((direction == ENDPT_DIRECTION_OUT) && !usb_dev.ep_info_OUT[EP0].bXferActive) ||
                ((direction == ENDPT_DIRECTION_IN) && !usb_dev.ep_info_IN[EP0].bXferActive))
            {
                CRIT("@Error: XferNotReady(Data-%s) when data stage should be complete.\n",
                     (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
                usb_dev.ep0_state = EP0_STATE_ERROR;
            }
        }
        else if (!usb_dev.ep0_three_stage_xfer)
        {
            CRIT("@Error: XferNotReady(Data) during 2-stage transfer!\n",
                  (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
            usb_dev.ep0_state = EP0_STATE_ERROR;
        }
        else if ((usb_dev.ep0_state == EP0_STATE_STATUS_IN) ||
                 (usb_dev.ep0_state == EP0_STATE_STATUS_OUT))
        {
            /* We need to send or receive a zero-length packet to end Data phase */
            DEBUG("-> usb_hal_handle_ep0_xfer_not_ready() - Preparing normal TRB for ZLP %s.\n",
                  (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

            usb_dev.ep0_zlp_pending = TRUE;

            if (direction == ENDPT_DIRECTION_OUT)
            {
                usb_dev.ep0_state = EP0_STATE_DATA_OUT;
                // Setup Normal TRB to Rx additional data (we expect a ZLP, but host could send us more which would result in an error).
                usb_hal_ep0_io_continue(ENDPT_DIRECTION_OUT, (void*)datapath_ram->ep0_buffer,
                                        usb_dev.ep_info_OUT[EP0].wMaxPktSize);
            }
            else /* IN */
            {
                usb_dev.ep0_state = EP0_STATE_DATA_IN;
                // Setup Normal TRB to send ZLP.
                usb_hal_ep0_io_continue(ENDPT_DIRECTION_IN, NULL, 0);
            }
        }
    }

    if (usb_dev.ep0_state == EP0_STATE_ERROR)
    {
        // Cancel any active transfers.
        usb_hal_cancel_io_request(EP0 | ENDPT_DIRECTION_OUT);
        usb_hal_cancel_io_request(EP0 | ENDPT_DIRECTION_IN);

        // Stall EP0 OUT.  EP0 state will transistion to EP0_IDLE.
        usb_hal_set_endpt_stall(EP0 | ENDPT_DIRECTION_OUT);
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_xfer_in_progress
 *************************************************************************//**
 * This function handles the transfer in-progress event for an endpoint when using
 * circular TRB structures.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_xfer_in_progress(UINT32_T ep_num)
{
    EP_INFO_T *pEP;
    TRB_RING_INFO_T *ring_info;
    STATUS_T status;
    TRANSFER_REQUEST_BLOCK_T *trb_ring;
    UINT32_T max_ring_index;
    UINT32_T trb_byte_cnt;

    INFO("-> usb_hal_handle_xfer_in_progress() - EP%u %s.\n", (ep_num & ~ENDPT_DIRECTION_MASK),
         ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

    usb_hal_get_ep_info_ptr(ep_num, &pEP);

    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        // Get pointers to TRB ring info and TRB.
        ring_info = &usb_dev.trb_ring_info_OUT;
        trb_ring = datapath_ram->trb_ring_OUT;

        // Set max ring index.
        max_ring_index = (TRB_RING_OUT_SIZE - 1);
    }
    else /* IN */
    {
        // Get pointers to TRB ring info and TRB.
        ring_info = &usb_dev.trb_ring_info_IN;
        trb_ring = datapath_ram->trb_ring_IN;

        // Set max ring index.
        max_ring_index = (TRB_RING_IN_SIZE - 1);
    }

    // Determine number of bytes transferred for this TRB.
    trb_byte_cnt = (ring_info->xfer_length[ring_info->index_pending] -
                    (trb_ring[ring_info->index_pending].dStatus & TRB_STATUS_BUFFER_SIZE_MASK));

    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_IN)
    {
#if ENABLE_UAS_DATA_IN_PACING
        // Update wrap window read offset.
        if ((usb_dev.active_mass_storage_class == USB_MSC_UAS) && (ata_dev[0].bNCQ))
        {
            if (pEP->dBytesRemaining > WRAP_WINDOW_BLOCK_SIZE_BYTES)
            {
                // Update wrap window read offset but leave window "FULL" when xfer is .
                WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), (trb_ring[ring_info->index_pending].dBufferPtrLow +
                                                                    MIN(trb_byte_cnt, (pEP->dBytesRemaining - WRAP_WINDOW_BLOCK_SIZE_BYTES))));
            }
        }
        else
#endif
        {
            // Update wrap window read offset.
            WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), (trb_ring[ring_info->index_pending].dBufferPtrLow + trb_byte_cnt));
        }
    }

    // Update EP info.
    pEP->dBytesRemaining -= trb_byte_cnt;
    pEP->dByteCount += trb_byte_cnt;
    pEP->pBuffer = ((UINT8_T*)pEP->pBuffer + trb_byte_cnt);

    // Increment index.
    ring_info->index_pending++;

    if (ring_info->index_pending >= max_ring_index)
    {
        ring_info->index_pending = 0;
    }

    if (ring_info->bytes_remaining > 0)
    {
        status = usb_hal_config_next_trb(ep_num);

        if (status != STATUS_OK)
        {
            CRIT("-> usb_hal_config_next_trb() timed out - TRB_index = %u, buff 0x%08x, bytes_remaining = %u + last xfer.\n",
                 ring_info->index, ring_info->buffer_ptr, ring_info->bytes_remaining);
        }
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_xfer_complete
 *************************************************************************//**
 * This function handles the transfer complete event for an endpoint.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_xfer_complete(UINT32_T ep_num)
{
    EP_INFO_T *pEP;
    TRB_RING_INFO_T *ring_info;
    TRANSFER_REQUEST_BLOCK_T *trb_ring;
    UINT32_T trb_byte_cnt;

    usb_hal_get_ep_info_ptr(ep_num, &pEP);

    if (!pEP->bXferActive)
    {
        // Could happen if end xfer command was issued after the xfer was already complete but
        // before xfer complete event was processed.
        CRIT("@Warning: Xfer complete event on EP%u %s when no xfer active!\n",
             (ep_num & ~ENDPT_DIRECTION_MASK),
             ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
#if DEBUG_LEVEL >= 2
        dump_usb_core_debug();
#endif
        return;
    }

    // Update endpt xfer info.
    if (pEP->wrap_window_xfer)
    {
        if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
        {
            // Get pointers to TRB ring info and TRB.
            ring_info = &usb_dev.trb_ring_info_OUT;
            trb_ring = datapath_ram->trb_ring_OUT;
        }
        else /* IN */
        {
            // Get pointers to TRB ring info and TRB.
            ring_info = &usb_dev.trb_ring_info_IN;
            trb_ring = datapath_ram->trb_ring_IN;
        }

        trb_byte_cnt = (ring_info->xfer_length[ring_info->index_pending] -
                        (trb_ring[ring_info->index_pending].dStatus & TRB_STATUS_BUFFER_SIZE_MASK));

        if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_IN)
        {
#if ENABLE_UAS_DATA_IN_PACING
             if ((usb_dev.active_mass_storage_class == USB_MSC_UAS) && (ata_dev[0].bNCQ))
             {
                 // Don't update wrap window read offset. Leave window "full".
             }
             else
#endif
             {
                 // Update wrap window read offset.
                 WRITE32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), (trb_ring[ring_info->index_pending].dBufferPtrLow + trb_byte_cnt));
             }
        }
    }
    else
    {
        trb_byte_cnt = pEP->dXferLength - (pEP->pTRB->dStatus & TRB_STATUS_BUFFER_SIZE_MASK);
    }

    // Update EP info.
    pEP->dBytesRemaining -= trb_byte_cnt;
    pEP->dByteCount += trb_byte_cnt;
    pEP->pBuffer = ((UINT8_T*)pEP->pBuffer + trb_byte_cnt);

    DEBUG("  Xfer complete event: EP%u %s, byte_cnt = %u, bytes_remaining = %u.\n", (ep_num & ~ENDPT_DIRECTION_MASK),
          ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", pEP->dByteCount, pEP->dBytesRemaining);

    // Clear transfer active flag.
    pEP->bXferActive = FALSE;

    if ((ep_num & ~ENDPT_DIRECTION_MASK) == EP0)
    {
        if (((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) &&
            (usb_dev.ep0_zlp_pending == TRUE) && (pEP->dByteCount > 0))
        {
            // Host sent us more data than specified in setup packet.
            CRIT("@Error: ZLP was expected but %u bytes was transfered.  Stalling EP0 OUT.\n", pEP->dByteCount);
            usb_dev.ep0_state = EP0_STATE_ERROR;
            // Stall EP0 OUT.
            usb_hal_set_endpt_stall(EP0 | ENDPT_DIRECTION_OUT);
        }
        else
        {
            if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
            {
                // EP0 OUT stall is cleared automatically by HW so clear flag to match.
                usb_dev.ep_info_OUT[EP0].bStalled = FALSE;
            }

            usb_hal_handle_ep0_xfer_cmplt(ep_num, pEP);
        }
    }
    else /* Non-EP0 */
    {
        // Call USB stack data xfer callback.
        pUsbStackDataXferCallback(ep_num, pEP);
    }

    return;
}

// For Xfer complete or In-progress commands.
#define DEPEVT_STATUS_BUS_ERROR_BIT    0x00001000

// For Stream events.
#define DEPEVT_STATUS_STREAM_FOUND     0x00001000
#define DEPEVT_STATUS_STREAM_NOT_FOUND 0x00002000

// For Start Transfer commands.
#define DEPEVT_STATUS_NO_MORE_XFER_RSC 0x00001000
#define DEPEVT_STATUS_BUS_TIME_EXPIRY  0x00002000

#define DEPEVT_PARAMS_COMMAND_TYPE_MASK     0x0F000000
#define DEPEVT_PARAMS_COMMAND_TYPE_OFFSET   24
#define DEPEVT_PARAMS_XFER_RSC_MASK         0x007F0000
#define DEPEVT_PARAMS_XFER_RSC_OFFSET       16
#define DEPEVT_PARAMS_STREAM_ID_MASK        0xFFFF0000
#define DEPEVT_PARAMS_STREAM_ID_OFFSET      16

/*****************************************************************************
 * Function: usb_hal_handle_endpt_event
 *************************************************************************//**
 * This function handles endpoint events.
 *
 * @param[in] event 4-byte event value from the event buffer.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_endpt_event(UINT32_T event)
{
    EP_INFO_T *pEP;
    ENDPT_DIR_T direction;
    UINT32_T phys_ep_num;
    UINT32_T endpt_number;  /* logical endpt num w/o direction */
    UINT32_T event_type;
    UINT32_T cmd_type;

    phys_ep_num = (event & DEPEVT_PHYSICAL_EP_NUM_MASK) >> DEPEVT_PHYSICAL_EP_NUM_OFFSET;
    direction = (phys_ep_num & 0x1) ? ENDPT_DIRECTION_IN : ENDPT_DIRECTION_OUT;
    endpt_number = (phys_ep_num >> 1);  /* Extract logical endpt num */

    event_type = (event & DEPEVT_EVENT_TYPE_MASK) >> DEPEVT_EVENT_TYPE_OFFSET;

    switch (event_type)
    {
        case DEPEVT_TYPE_STREAM_EVT:
            if (event & DEPEVT_STATUS_STREAM_FOUND)
            {
                // Workaround for WEBS PG3_0_Silicon.8 (CRM #9000416825) No ERDY transmitted for stream-capable OUT endpoint.
                if (usb_dev.usb_core_version == 0x120a)
                {
                    if (direction == ENDPT_DIRECTION_OUT)
                    {
                        usb_hal_get_ep_info_ptr((endpt_number | direction), &pEP);

                        // Set HWO bit for 1st TRB.
                        pEP->pTRB->dControl |= TRB_CTRL_HWO_BIT;

                        // Issue Update Transfer.
                        usb_hal_update_transfer(endpt_number | direction);
                    }
                }

                DEBUG(" Stream found. SID = 0x%x.\n", (event & DEPEVT_PARAMS_STREAM_ID_MASK) >> DEPEVT_PARAMS_STREAM_ID_OFFSET);
            }
            else if (event & DEPEVT_STATUS_STREAM_NOT_FOUND)
            {
                INFO(" EP%u %s Stream NOT found.\n", endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
            }
            break;

        case DEPEVT_TYPE_EP_CMD_CMPLT:
            // End transfer command interrupts cannot be masked off so they will be handled here.
            cmd_type = (event & DEPEVT_PARAMS_COMMAND_TYPE_MASK) >> DEPEVT_PARAMS_COMMAND_TYPE_OFFSET;

            INFO("  EP%u %s command %u complete event.\n", endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", cmd_type);

            usb_hal_get_ep_info_ptr((endpt_number | direction), &pEP);

            if (cmd_type == DEPCMD_TYPE_START_XFER)
            {
                // Save transfer resource index.
                pEP->dXferRscIndex = (event & DEPEVT_PARAMS_XFER_RSC_MASK) >> DEPEVT_PARAMS_XFER_RSC_OFFSET;

                DEBUG(" EP%u %s xfer_resource = %u.\n", endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", pEP->dXferRscIndex);

                if (event & DEPEVT_STATUS_NO_MORE_XFER_RSC)
                {
                    CRIT("@Error: No more xfer resources on EP%d %s.\n",  endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
                }

                if (event & DEPEVT_STATUS_BUS_TIME_EXPIRY)
                {
                    CRIT("@Error: Expiry of bus time on EP%d %s.\n",  endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
                }
            }

            break;

        case DEPEVT_TYPE_XFER_IN_PROGRESS:
            if (event & DEPEVT_STATUS_BUS_ERROR_BIT)
            {
                CRIT("@Error: Bus error occurred on Xfer in progress event!\n");
            }

            usb_hal_handle_xfer_in_progress(endpt_number | direction);
            break;


        case DEPEVT_TYPE_XFER_COMPLETE:
            if (event & DEPEVT_STATUS_BUS_ERROR_BIT)
            {
                CRIT("@Error: Bus error occurred on Xfer complete event!\n");
            }

            usb_hal_handle_xfer_complete(endpt_number | direction);
            break;

        case DEPEVT_TYPE_XFER_NOT_READY:
            DEBUG("  EP%u %s xfer not ready event.\n", endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

            usb_hal_get_ep_info_ptr((endpt_number | direction), &pEP);

            if (pEP->bStalled)
            {
                CRIT("@Warning: Xfer not ready event on stalled EP%u %s.  Ignoring event.\n",
                     endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");
            }
            else
            {
                if (endpt_number == EP0)
                {
                    usb_hal_handle_ep0_xfer_not_ready(event, direction);
                }
            }
            break;

        default:
            CRIT("@Error: EP%u %s - Unhandled endpt event type = %u. (event = 0x%x).\n",
                 endpt_number, (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",  event_type, event);
            break;
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_usb_reset_stage2
 *************************************************************************//**
 * This function handles the 2nd stage of USB reset initialization.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_handle_usb_reset_stage2(void)
{
    // Set device state to DEFAULT.
    usb_dev.dev_state = USB_DEVICE_STATE_DEFAULT;

    // Cancel all non-EP0 I/O requests.
    usb_hal_cancel_all_io_requests();

    // Reset device address to zero.
    usb_hal_set_address(0);

    // Set USB Reset initialization complete flag.
    usb_dev.bUSBResetInitComplete = TRUE;

    // Set PM state.
    usb_dev.dev_pm_state = USB_PM_RESET;

    // Call power management callback.
    if (pUsbStackPowerMngmtCallback)
    {
        pUsbStackPowerMngmtCallback(usb_dev.dev_pm_state);
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_usb_reset
 *************************************************************************//**
 * This function handles the USB reset event.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_usb_reset(void)
{
    INFO("-> usb_hal_handle_usb_reset()\n");

    // Enable physical endpts 0 & 1 for EP0 IN/OUT and disable all others.
    WRITE32(USB_REG_OFF(DALEPENA_REG_OFF), (DALEPENA_EP0_OUT | DALEPENA_EP0_IN));

#if ENABLE_U1_U2_TRANSITIONS
    // Clear U1/U2 initiate enable bits. (Host must send SET FEATURE request to re-enable these features)
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), (DCTL_U1_INITIATE_ENABLE_BIT | DCTL_U2_INITIATE_ENABLE_BIT), 0);

    // Set U1/U2 Accept Enable bits.  They are cleared by HW upon USB Reset.
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), (DCTL_U1_ACCEPT_ENABLE | DCTL_U2_ACCEPT_ENABLE), (DCTL_U1_ACCEPT_ENABLE | DCTL_U2_ACCEPT_ENABLE));
#endif

    // Clear U1/U2 enable flags.
    usb_dev.bIsU1Enabled = FALSE;
    usb_dev.bIsU2Enabled = FALSE;

    if (usb_dev.ep0_state == EP0_STATE_IDLE)
    {
        usb_hal_handle_usb_reset_stage2();

        usb_dev.bUSBResetInitComplete = TRUE;
    }
    else
    {
        // Allow previous control transfer to complete before completing stage 2 of USB Reset initialization.
        DEBUG("\nUSB Reset Stage 1.\n");
        usb_dev.bUSBResetInitComplete = FALSE;
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_connection_done
 *************************************************************************//**
 * This function handles the connection done event by determining the
 * connection speed, configuring EP0, and preparing to receive a setup packet.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_connection_done(void)
{
    INFO("-> usb_hal_handle_connection_done()\n");

    system_disable_clock_gating();

    // Get the connection speed.
    usb_dev.dev_speed = usb_hal_get_connect_speed();

    // Count down rate uses RTI COMP1 (4Hz). DisconnectCountDown of 20 gives 5 sec delay to spin down HDD.
    usb_dev.dDisconnectCountDown = 20;

    // Configure interrupt rate based on connection speed for HDD activity blink.
    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        // Blink rate is twice as fast when connected at SuperSpeed.
        rti_config_compare_interrupt(2, (RTI_COMP2_INTERRUPT_PERIOD >> 1));
    }
    else
    {
        rti_config_compare_interrupt(2, RTI_COMP2_INTERRUPT_PERIOD);
    }

    // Setup connection speed GIOs.
    config_connection_speed_gio();

    // Configure EP0.
    usb_hal_config_ep0(EP0_TX_FIFO, FALSE);

    if (usb_dev.bConfigEP0SetupXfer)
    {
        // Clear flag since we only want to do this once after power-on.
        usb_dev.bConfigEP0SetupXfer = FALSE;

        // Program the SETUP TRB and start xfer.
        usb_hal_ep0_setup_stage();
    }

    return;
}


/*****************************************************************************
 * Function: usb_hal_handle_resume
 *************************************************************************//**
 * This function handles the wake up event.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_resume(void)
{
    CRIT("-> usb_hal_handle_resume()\n");

    system_disable_clock_gating();

    // Set PM state and call USB stack callback.
    usb_dev.dev_pm_state = USB_PM_RESUME;

    // Call power management callback.
    if (pUsbStackPowerMngmtCallback)
    {
        pUsbStackPowerMngmtCallback(usb_dev.dev_pm_state);
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_disconnect
 *************************************************************************//**
 * This function handles the disconnect event.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_handle_disconnect(void)
{
    INFO("-> usb_hal_handle_disconnect()\n");

    /* Clear the link state change request field because a remote wakeup may have been issued
     * to exit L1 sleep and the core will not be able to return to ON state. */
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, 0);

    // Set device state and speed.
    usb_dev.dev_state = USB_DEVICE_STATE_NOT_CONNECTED;
    usb_dev.dev_speed = USB_SPEED_UNKNOWN;

    // Setup connection speed GIOs.
    config_connection_speed_gio();

    // Set PM state.
    usb_dev.dev_pm_state = USB_PM_DISCONNECT;

    // Call power management callback.
    if (pUsbStackPowerMngmtCallback)
    {
        pUsbStackPowerMngmtCallback(usb_dev.dev_pm_state);
    }

    DEBUG("Max USB Event count = %u.\n", usb_dev.dMaxEventCnt);

#if USB_CORE_RESET_TEST  // For testing only.
    // Reinitialize USB Core.
    usb_hal_init(NULL, NULL, NULL);

    // Set core reset flag so event count will not be updated in ISR.
    gUSBCoreReset = TRUE;

    // Enable USB controller and connect to upstream port.
    usb_hal_connect();
#endif

    return;
}

#define DEVT_LINK_STATE_MASK        0x000F0000
#define DEVT_LINK_STATE_OFFSET      16
#define DEVT_LINK_STATE_SS_EVNT_BIT 0x00100000

/*****************************************************************************
 * Function: usb_hal_handle_link_state_change
 *************************************************************************//**
 * This function handles the USB/Link state change event.
 *
 * @param[in] event value of the link state change event.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_link_state_change(UINT32_T event)
{
    UINT32_T state;
    UINT32_T fifo_max_sz;
    UINT32_T fifo_num;
    UINT32_T fifo_space_avail;
    BOOLEAN_T SS_event = FALSE;

    if (usb_dev.usb_core_version >= 0x110a)
    {
        INFO("link_event = 0x%x\n", event);
        state = (event & DEVT_LINK_STATE_MASK) >> DEVT_LINK_STATE_OFFSET;
        SS_event = (event & DEVT_LINK_STATE_SS_EVNT_BIT) ? TRUE : FALSE;
    }
    else
    {
        // Determine state from DSTS.
        state = usb_hal_get_usb_link_state();
    }

    if (SS_event || ((usb_dev.usb_core_version < 0x110a) && (usb_dev.dev_speed == USB_SUPER_SPEED)))
    {
        CRIT("LTSSM state = (0x%x) %s.\n", state,
             (state == U0_STATE) ? "U0" :
             (state == U1_STATE) ? "U1" :
             (state == U2_STATE) ? "U2" :
             (state == U3_STATE) ? "U3" :
             (state == SS_DISABLED_STATE) ? "SS DISABLED" :
             (state == RX_DETECT_STATE) ? "RX DETECT" :
             (state == SS_INACTIVE_STATE) ? "SS INACTIVE" :
             (state == POLLING_STATE) ? "POLLING" :
             (state == RECOVERY_STATE) ? "RECOVERY" :
             (state == HOT_RESET_STATE) ? "HOT RESET" :
             (state == COMPLIANCE_STATE) ? "COMPLIANCE" :
             (state == LOOPBACK_STATE) ? "LOOPBACK" : "?");

        switch (state)
        {
            case U0_STATE:
                system_disable_clock_gating();
#if ENABLE_U1_U2_TRANSITIONS
                LED_OFF(SS_LINK_STATE_LED0_GIO_NUM);
                LED_OFF(SS_LINK_STATE_LED1_GIO_NUM);
#endif
                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
                break;

            case U1_STATE:
                system_disable_clock_gating();
#if ENABLE_U1_U2_TRANSITIONS
                LED_ON(SS_LINK_STATE_LED0_GIO_NUM);
                LED_OFF(SS_LINK_STATE_LED1_GIO_NUM);
#endif
                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
                break;

            case U2_STATE:
#if ENABLE_U1_U2_TRANSITIONS
                LED_OFF(SS_LINK_STATE_LED0_GIO_NUM);
                LED_ON(SS_LINK_STATE_LED1_GIO_NUM);
#endif
                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
                system_enable_clock_gating();
                break;

            case U3_STATE:
#if ENABLE_U1_U2_TRANSITIONS
                LED_ON(SS_LINK_STATE_LED0_GIO_NUM);
                LED_ON(SS_LINK_STATE_LED1_GIO_NUM);
#endif
                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.

                // Count down rate uses RTI COMP1 (4Hz). SuspendCountDown of 1 gives 250 millisec delay to spin down HDD.
                usb_dev.dSuspendCountDown = 1;

                // Set PM state and call USB stack callback.
                usb_dev.dev_pm_state = USB_PM_SUSPEND;
                if (pUsbStackPowerMngmtCallback)
                {
                    pUsbStackPowerMngmtCallback(usb_dev.dev_pm_state);
                }

                system_enable_clock_gating();
                break;

            case SS_DISABLED_STATE:
#if ENABLE_USB_PHY_SUSPEND
                // Enable USB 2.0 suspend mode.  This is required because sometimes the SUSPEND event does not occur,
                // when unplugging the USB cable.
                MODIFY32(USB_REG_OFF(GUSB2PHYCFG_REG_OFF), PHYCFG_SUSPEND_ENABLE_BIT, PHYCFG_SUSPEND_ENABLE_BIT);
#endif

#if ENABLE_U1_U2_TRANSITIONS
                LED_OFF(SS_LINK_STATE_LED0_GIO_NUM);
                LED_OFF(SS_LINK_STATE_LED1_GIO_NUM);
#endif

                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
                if (usb_dev.usb_core_version <= 0x109B)
                {
                    usb_hal_handle_disconnect();
                }
                system_enable_clock_gating();
                break;

            case RECOVERY_STATE:
                // When exiting U1 or U2, the core indicates a Recovery link state change, but no U0 state change
                // afterward so all U0 related actions need to be handled here.
                system_disable_clock_gating();
#if ENABLE_U1_U2_TRANSITIONS
                LED_OFF(SS_LINK_STATE_LED0_GIO_NUM);
                LED_OFF(SS_LINK_STATE_LED1_GIO_NUM);
#endif
                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.

#if DISABLE_U1_U2_INITIATE_WHILE_XFER_ACTIVE & ENABLE_U1_U2_TRANSITIONS
                // Workaround for WEBS PG3_0_Silicon.21 (CRM #9000446952) If U1/U2 ->U0 takes >128us, the core sends LGO_Ux
                // after entering U0.  Results in reduced throughput with Fresco host.
                // Disable device-initiated U1/U2. Re-enable after transfers are complete.
                if (usb_dev.usb_core_version < 0x183a)
                {
                    if (usb_dev.bIsU1Enabled)
                    {
                        usb_hal_set_U1_initiate_enable(FALSE);
                    }

                    if (usb_dev.bIsU2Enabled)
                    {
                        usb_hal_set_U2_initiate_enable(FALSE);
                    }
                }
#endif
                if (usb_dev.bFunctionWakeDeviceNotificationPending)
                {
                    CRIT("Function Wake Device.\n");

                    // Wait for any previous command to complete.
                    while(READ32(USB_REG_OFF(DGCMD_REG_OFF)) & DGCMD_CMD_ACT_BIT);

                    // Issue the function wake device notification.
                    WRITE32(USB_REG_OFF(DGCMDPAR_REG_OFF), usb_dev.bFunctionWakeInterfaceNum);
                    WRITE32(USB_REG_OFF(DGCMD_REG_OFF), (DGCMD_CMD_ACT_BIT | DGCMD_TYPE_WAKE_DEVICE));

                    // Clear the pending flag.
                    usb_dev.bFunctionWakeDeviceNotificationPending = FALSE;
                }
                break;

//            case SS_INACTIVE_STATE:
//                CRIT("attempt recovery\n");
//                // Clear the link state change request field.
//                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, 0);
//
//                // Issue remote wakeup request.
//                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, (DCTL_REMOTE_WAKEUP_REQ << DCTL_UL_STATE_CHNG_REQ_OFFSET));
//                break;

            default:
                system_disable_clock_gating();
#if ENABLE_U1_U2_TRANSITIONS
                LED_OFF(SS_LINK_STATE_LED0_GIO_NUM);
                LED_OFF(SS_LINK_STATE_LED1_GIO_NUM);
#endif
                LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
                break;
        }
    }
    else /* HS/FS/LS */
    {
        CRIT("HS/FS/LS state = (0x%x) %s.\n", state,
             (state == ON_STATE) ? "ON" :
             (state == SLEEP_STATE) ? "SLEEP" :
             (state == SUSPEND_STATE) ? "SUSPEND" :
             (state == VBUS_OFF_STATE) ? "VBUS OFF" :
             (state == EARLY_SUSPEND_STATE) ? "EARLY SUSPEND" : "?");

        if (state == VBUS_OFF_STATE)
        {
            if (usb_dev.usb_core_version <= 0x109B)
            {
                usb_hal_handle_disconnect();
            }

            LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
            system_enable_clock_gating();
        }
        else if (state == SUSPEND_STATE)
        {
#if ENABLE_USB_PHY_SUSPEND
            // Enable USB 2.0 suspend mode.
            MODIFY32(USB_REG_OFF(GUSB2PHYCFG_REG_OFF), PHYCFG_SUSPEND_ENABLE_BIT, PHYCFG_SUSPEND_ENABLE_BIT);
#endif
            // Count down rate uses RTI COMP1 (4Hz). SuspendCountDown of 8 gives 2 sec delay to spin down HDD.
            // This is required because SUSPEND STATE occurs transiently during USB hot plug and we do not
            // want to spin down the drive unless we are actually in suspended state.
            usb_dev.dSuspendCountDown = 8;

            LED_ON(HS_SUSPEND_LED_GIO_NUM);   // Turn on FS/HS suspend LED.

            // Set PM state and call USB stack callback.
            // SUSPEND state occurs naturally during USB 2.0 hot-plug so do not call power
            // management callback unless the device is in addressed or configured state.
            usb_dev.dev_pm_state = USB_PM_SUSPEND;
            if ((usb_dev.dev_state >= USB_DEVICE_STATE_ADDRESSED) && pUsbStackPowerMngmtCallback)
            {
                pUsbStackPowerMngmtCallback(usb_dev.dev_pm_state);
            }

            system_enable_clock_gating();
        }
        else if (state == SLEEP_STATE)
        {
            // Check Tx FIFOs to see if any are not empty.
            for (fifo_num = 0; fifo_num <= MAX_TX_FIFO_NUM; fifo_num++)
            {
                // Set FIFO number.
                WRITE32(USB_REG_OFF(GDBGFIFOSPACE_REG_OFF), fifo_num);

                // Read FIFO info.
                fifo_space_avail = READ32(USB_REG_OFF(GDBGFIFOSPACE_REG_OFF)) >> GDBGFIFOSPACE_AVAIL_OFFSET;
                fifo_max_sz = READ32(GTXFIFOSIZ(fifo_num)) & GTXFIFOSIZ_FIFO_DEPTH_MASK;
               
                // If the FIFO space available is less than the FIFO's max size, there is data in the FIFO and we need to wake the link.
                if (fifo_space_avail != fifo_max_sz)
                {
                    INFO("Waking link, FIFO%u avail=0x%x, sz=0x%x\n", fifo_num, fifo_space_avail, fifo_max_sz);
                    // Clear the link state change request field.
                    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, 0);

                    // Issue remote wakeup request.
                    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, (DCTL_REMOTE_WAKEUP_REQ << DCTL_UL_STATE_CHNG_REQ_OFFSET));

                    // Exit for loop.
                    break;
                }
            }
        }
        else
        {
            // Disable USB 2.0 suspend mode so SUSPEND event is not delayed.  Enable PHY suspend mode after processing SUSPEND event.
            MODIFY32(USB_REG_OFF(GUSB2PHYCFG_REG_OFF), PHYCFG_SUSPEND_ENABLE_BIT, 0);
            system_disable_clock_gating();
            LED_OFF(HS_SUSPEND_LED_GIO_NUM);  // Turn off FS/HS suspend LED.
        }
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_handle_device_event
 *************************************************************************//**
 * This function handles device events.
 *
 * @param[in] event 4-byte event value from the event buffer.
 *
 * @retval None.
 *
 ******************************************************************************
 */

inline void usb_hal_handle_device_event(UINT32_T event)
{
    UINT32_T event_type = (event & DEVT_EVENT_TYPE_MASK) >> DEVT_EVENT_TYPE_OFFSET;

    switch (event_type)
    {
        case DEVT_TYPE_USB_RESET:
            CRIT("USB Reset event occurred.\n");
            usb_hal_handle_usb_reset();
            break;

        case DEVT_TYPE_CONNECTION_DONE:
            usb_hal_handle_connection_done();
            break;

        case DEVT_TYPE_RESUME:
            usb_hal_handle_resume();
            break;

        case DEVT_TYPE_DISCONNECT:
            if (usb_dev.usb_core_version > 0x109B)
            {
                CRIT("Disconnect event occurred.\n");
                usb_hal_handle_disconnect();
            }
            break;

        case DEVT_TYPE_USB_LINK_STATE_CHNG:
            usb_hal_handle_link_state_change(event);
            break;

        case DEVT_EVNT_OVERFLOW:
            CRIT("@Error:  Event overflow occurred!\n");
            break;

        case DEVT_ERRATIC_ERROR:
            CRIT("@Error:  Erratic error on UTMI!\n");
            break;

        default:
            CRIT("@Error:  Unhandled device event type = %u!\n", event_type);
            break;
    }

    return;
}


#define NON_EP_EVNT_TYPE_BIT  0x00000001
#define DEVICE_OTG_EVNT_MASK  0x000000FE
#define DEVICE_EVNT_TYPE      0x00000000
#define OTG_EVNT_TYPE         0x00000002

/*****************************************************************************
 * Function: usb_hal_isr
 *************************************************************************//**
 * This function is called by the interrupt controller to handle
 * USB events.  Interrupt line is de-asserted when event count is zero.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_isr(void)
{
    UINT32_T evnt_cnt;
    UINT32_T num_events;
    UINT32_T event;

    // Get event count (in bytes).
    evnt_cnt = READ32(GEVNTCOUNT(EVNT_BUFF_0));

#if DEBUG_LEVEL > 1
    if (evnt_cnt > usb_dev.dMaxEventCnt) usb_dev.dMaxEventCnt = evnt_cnt;
#endif

    // Clock crossing delays may result in continual assertion of the interrupt line even after
    // all events have been processed so check for a non-zero event count.
    if (evnt_cnt == 0) return;

    do
    {
        INFO("-> usb_hal_isr() - Event count = %u bytes.\n", evnt_cnt);

        // Calculate number of events (4 bytes per event).
        num_events = evnt_cnt >> 2;

        while(num_events--)
        {
            // Retrieve event.
            event = READ32((UINT32_T)usb_dev.event_ptr[EVNT_BUFF_0]);

            // Increment event pointer.
            usb_dev.event_ptr[EVNT_BUFF_0]++;

            // Check for rollover.
            if (usb_dev.event_ptr[EVNT_BUFF_0] >= usb_dev.event_buff_end[EVNT_BUFF_0])
            {
                usb_dev.event_ptr[EVNT_BUFF_0] = usb_dev.event_buff[EVNT_BUFF_0];
            }

            // Check event type.
            if (event & NON_EP_EVNT_TYPE_BIT)
            {
                if ((event & DEVICE_OTG_EVNT_MASK) == DEVICE_EVNT_TYPE)
                {
                    DEBUG("-> usb_hal_isr() - Device event = 0x%08x.\n", event);
                    usb_hal_handle_device_event(event);
                }
                else
                {
                    // OTG and Other core events are not handled.
                    CRIT("@Error: usb_hal_isr() - Unhandled non-EP event = 0x%08x.\n", event);
                }
            }
            else /* Endpt specific event */
            {
                INFO("-> usb_hal_isr() - Endpt event = 0x%08x.\n", event);
                usb_hal_handle_endpt_event(event);
            }
#if USB_CORE_RESET_TEST
            // Core reset flag must be set if usb_hal_init() is called within the ISR.
            if (gUSBCoreReset)
            {
                gUSBCoreReset = FALSE;
                goto isr_exit;
            }
#endif
        }

        // Update event count.
        WRITE32(GEVNTCOUNT(EVNT_BUFF_0), evnt_cnt);

        // Recheck event count.
        evnt_cnt = READ32(GEVNTCOUNT(EVNT_BUFF_0));

    } while (evnt_cnt > 0);

#if USB_CORE_RESET_TEST
isr_exit:
#endif

    return;
}

