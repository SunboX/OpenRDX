/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : usb_hal.c
//
// Project     : TUSB926x Firmware.
//
// Description : Hardware abstraction layer for USB 3.0 controller.
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
 * This file contains the implementation of the USB hardware abstraction layer.
 *
 */

#include "usb_hal.h"
#include "gio.h"  // gio_get_state()
#include "mww.h"
#include "reg_io.h"
#include "rti.h"  // usleep()
#include "sci.h"
#include "system.h"
#include "tusb9260.h"
#include "tusb9260_types.h"
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

USB_DEVICE_T usb_dev;

/* USB Stack callbacks */
void (*pUsbStackSetupPktCallback)(void) = NULL;
void (*pUsbStackDataXferCallback)(UINT32_T ep_num, EP_INFO_T *ep_info) = NULL;
void (*pUsbStackPowerMngmtCallback)(eUSB_DEVICE_PM_STATE_T pm_state) = NULL;


/* indexed by EP# and speed */
const UINT16_T usb_endpt_max_pkt_size[4][4] = {
    /* EP0 - LS, FS, HS, SS */            {8, 64, 64, 512},
    /* EP1 (HID) - LS, FS, HS, SS */      {8, 64, 64, 64},
    /* EP2 (UAS) - LS, FS, HS, SS */      {0, 64, 512, 1024},   /* Bulk endpoints not supported at low speed */
    /* EP3 (UAS/BOT) - LS, FS, HS, SS */  {0, 64, 512, 1024}
};

#if DEBUG_LEVEL >= 1

#define NUM_EPS 8

void dump_usb_core_debug(void)
{
    UINT32_T i;

    CRIT("GBUSERRADDR = 0x%08x.\n", READ32(USB_REG_OFF(GBUSERRADDR_REG_OFF)));
    CRIT("GSTS = 0x%08x.\n", READ32(USB_REG_OFF(GSTS_REG_OFF)));
    CRIT("LTSSM = 0x%08x.\n", READ_REG32(USB_REG_OFF(GDBGLTSSM_REG_OFF)));
    CRIT("Link State = %d.\n", usb_hal_get_usb_link_state());

    for (i = 0; i < 11; i++)
    {
        WRITE32(USB_REG_OFF(0x170), (i << 4));
        CRIT("%u: 0xc174 = 0x%08x.\n", i, READ32(USB_REG_OFF(0x174)));
    }

    for (i = 0; i < NUM_EPS; i++)
    {
        WRITE32(USB_REG_OFF(0x170), i);
        CRIT("EP%u:\n", i);
        CRIT("  0xc178 = 0x%08x.\n", READ32(USB_REG_OFF(0x178)));
        CRIT("  0xc17C = 0x%08x.\n", READ32(USB_REG_OFF(0x17C)));
    }

    CRIT("\n\n");
    CRIT("Event Buffer ptr = 0x%08x\n", (UINT32_T)usb_dev.event_ptr[EVNT_BUFF_0]);

    CRIT("EP0 OUT TRB ptr = 0x%08x\n", (UINT32_T)usb_dev.ep_info_OUT[0].pTRB);

    CRIT("Setup Pkt TRB @ 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x.\n", (UINT32_T)&datapath_ram->trb_ep0_setup_packet,
         datapath_ram->trb_ep0_setup_packet.dBufferPtrLow, datapath_ram->trb_ep0_setup_packet.dBufferPtrHigh,
         datapath_ram->trb_ep0_setup_packet.dStatus, datapath_ram->trb_ep0_setup_packet.dControl);

    for (i = 0; i <= MAX_EP_NUM; i++)
    {
        CRIT("EP%u IN TRB @ 0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x.\n", i, (UINT32_T)&datapath_ram->trb_data_IN[i],
             datapath_ram->trb_data_IN[i].dBufferPtrLow, datapath_ram->trb_data_IN[i].dBufferPtrHigh,
             datapath_ram->trb_data_IN[i].dStatus, datapath_ram->trb_data_IN[i].dControl);

        CRIT("EP%u OUT TRB @ 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x.\n", i, (UINT32_T)&datapath_ram->trb_data_OUT[i],
             datapath_ram->trb_data_OUT[i].dBufferPtrLow, datapath_ram->trb_data_OUT[i].dBufferPtrHigh,
             datapath_ram->trb_data_OUT[i].dStatus, datapath_ram->trb_data_OUT[i].dControl);
    }

    return;

}
#endif


#define SLEEP_TIME   1   /* us */

/*****************************************************************************
 * Function: usb_hal_wait_depcmd_complete
 *************************************************************************//**
 * This function waits for the completion of an endpoint command.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] timeout_us timeout in microseconds.
 *
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

inline STATUS_T usb_hal_wait_depcmd_complete(UINT32_T ep_num, INT32_T timeout_us)
{
    while ((READ_REG32(DEPCMD(ep_num)) & DEPCMD_CMD_ACT_BIT) && (timeout_us > 0))
    {
        usleep(SLEEP_TIME);
        timeout_us -= SLEEP_TIME;
    }

    if (timeout_us <= 0)
    {
        CRIT("@Error: DEPCMD (0x%08x) timed out on EP%u %s!\n", READ32(DEPCMD(ep_num)),
             (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

#if DEBUG_LEVEL >= 1
        dump_usb_core_debug();
#endif
        return STATUS_TIMEOUT;
    }
    else
    {
        return STATUS_OK;
    }
}


#define CMD_TIMEOUT_US   1000   /* us */

/*****************************************************************************
 * Function: usb_hal_dep_cmd
 *************************************************************************//**
 * This function issues an endpoint command to the USB controller.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] cmd endpoint command.
 * @param[in] par0 command parameter 0.
 * @param[in] par1 command parameter 1.
 *
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_TIMEOUT when previously issued command for the specified endpoint times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_dep_cmd(UINT32_T ep_num, UINT32_T cmd, UINT32_T par0, UINT32_T par1)
{
    EP_INFO_T *ep_info;
    STATUS_T status;
    UINT32_T cmd_type;
    UINT32_T wrap_window_block_size;

    INFO("-> usb_hal_dep_cmd() EP%u %s, cmd = 0x%08x, par0 = 0x%08x, par1 = 0x%08x.\n",
         (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
         cmd, par0, par1);

    // Wait for completion of previous command.
    status = usb_hal_wait_depcmd_complete(ep_num, CMD_TIMEOUT_US);

    if (status != STATUS_OK)
    {
        return status;
    }

    // Issue endpoint command.
    WRITE32(DEPCMDPAR1(ep_num), par1);
    WRITE32(DEPCMDPAR0(ep_num), par0);

    cmd_type = (cmd & DEPCMD_CMD_TYPE_MASK);

    // We don't need to set the CmdAct bit for update transfer commands on USB core versions > 1.08a.
    if ((cmd_type == DEPCMD_TYPE_UPDATE_XFER) && (usb_dev.usb_core_version > 0x108A))
    {
        WRITE32(DEPCMD(ep_num), cmd);
    }
    else
    {
        WRITE32(DEPCMD(ep_num), (cmd | DEPCMD_CMD_ACT_BIT));
    }

    // We cannot force Wrap Windows ready for the STALL command because BOT uses STALLs during operation to handle error cases.
    if ((cmd_type == DEPCMD_TYPE_END_XFER)/* || (cmd_type == DEPCMD_TYPE_SET_STALL)*/)
    {
        usb_hal_get_ep_info_ptr(ep_num, &ep_info);

        // Force wrap window ready if it is in use.
        if (ep_info->wrap_window_xfer)
        {
            if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
            {
                INFO("force wrap window ready - OUT.\n");
                // Set read_offset equal to write_offset for Window 1 so it appears empty so
                // USB can finish writing any pending data.
                WRITE32(MWWxRDOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM), READ32(MWWxWRTOFFSET(USB_TO_SATA_WRAP_WINDOW_NUM)));
            }
            else /* IN */
            {
                INFO("force wrap window ready - IN.\n");

                wrap_window_block_size = (usb_dev.active_mass_storage_class == USB_MSC_BOT) ? MWW_BLK_SIZE_64KB : MWW_BLK_SIZE_32KB;

                // Read Window 0 read_offset, flip overflow bit and write that value to the write_offset to
                // make the window appear full so USB can finish any pending data reads.
                WRITE32(MWWxWRTOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM), (READ32(MWWxRDOFFSET(SATA_TO_USB_WRAP_WINDOW_NUM)) ^ (1 << (0x0A + wrap_window_block_size))));
            }
        }
    }

    return STATUS_OK;
}


/*****************************************************************************
 * Function: usb_hal_dep_xfer_resource_config
 *************************************************************************//** *
 * This function configures an endpoint's transfer resources.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] xfer_rsc_num number of transfer resources (must be at least 1).
 *
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_dep_xfer_resource_config(UINT32_T ep_num, UINT32_T xfer_rsc_num)
{
    DEBUG("-> usb_hal_dep_xfer_resource_config() EP%u %s, rsc_num = %u.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          xfer_rsc_num);

    // Issue tranfer resource config command.
    return usb_hal_dep_cmd(ep_num, DEPCMD_TYPE_SET_EP_XFER_RESOURCE_CONFIG, (xfer_rsc_num & PAR0_NUM_XFER_RES_MASK), 0);
}


/*****************************************************************************
 * Function: usb_hal_dep_xfer_start
 *************************************************************************//**
 * This function starts an endpoint transfer and stores information
 * related to the transfer.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] trb pointer to Transfer Request Block.
 * @param[in] byte_cnt total length of the data transfer in bytes.
 * @param[in] stream_id stream ID. Valid for USB 3.0 bulk endpoints which support streams.
 *
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_dep_xfer_start(UINT32_T ep_num, TRANSFER_REQUEST_BLOCK_T *trb, UINT32_T byte_cnt, UINT32_T stream_id)
{
    STATUS_T status;
    EP_INFO_T *ep_info;

    DEBUG("-> usb_hal_dep_xfer_start() EP%u %s, sid = 0x%x, byte_cnt = %u, TRB_ptr = 0x%08x.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          stream_id, byte_cnt, (UINT32_T)trb);

    usb_hal_get_ep_info_ptr(ep_num, &ep_info);

    // Save EP info.
    ep_info->pTRB = trb;
    ep_info->dXferLength = trb->dStatus;
    ep_info->dBytesRemaining = byte_cnt;
    ep_info->dByteCount = 0;
    ep_info->pBuffer = (void*)trb->dBufferPtrLow;
    ep_info->wStreamID = stream_id;

    // Issue EP transfer command.  (Set IOC bit so we can save transfer resource index)
    status = usb_hal_dep_cmd(ep_num, ((stream_id << DEPCMD_STREAM_ID_OFFSET) | DEPCMD_CMD_IOC_BIT | DEPCMD_TYPE_START_XFER), 0, (UINT32_T)trb);

    if (status == STATUS_OK)
    {
        // Set transfer active flag.
        ep_info->bXferActive = TRUE;

        // Check if link is in sleep state.
        if ((usb_dev.dev_speed != USB_SUPER_SPEED) && (usb_hal_get_usb_link_state() == SLEEP_STATE))
        {
            if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_IN)
            {
                // Clear the link state change request field.
                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, 0);
    
                // Issue remote wakeup request.
                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, (DCTL_REMOTE_WAKEUP_REQ << DCTL_UL_STATE_CHNG_REQ_OFFSET));
            }
        }
    }

    return status;
}


#define PAR0_DATA_SEQ_MASK   0x7C000000
#define PAR0_DATA_SEQ_OFFSET 26

/*****************************************************************************
 * Function: usb_hal_dep_get_data_seq
 *************************************************************************//**
 * This function returns the data sequence number.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[out] data_seq_num pointer to memory to store data sequence number.
 *
 * @retval STATUS_OK when command completes successfully.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_dep_get_data_seq(UINT32_T ep_num, UINT32_T *data_seq_num)
{
    STATUS_T status;

    status = usb_hal_dep_cmd(ep_num, DEPCMD_TYPE_GET_DATA_SEQ, 0, 0);

    if (status == STATUS_OK)
    {
        *data_seq_num = ((READ32(DEPCMDPAR0(ep_num)) & PAR0_DATA_SEQ_MASK) >> PAR0_DATA_SEQ_OFFSET);
    }

    return status;
}


/*****************************************************************************
 * Function: usb_hal_set_endpt_stall
 *************************************************************************//**
 * This function sets the stall condition for the endpoint specified.  Any
 * wrap window transfers for the endpoint must be stopped using
 * usb_hal_cancel_io_request() prior to STALLing.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_set_endpt_stall(UINT32_T ep_num)
{
    STATUS_T status;
    EP_INFO_T *ep_info;

    DEBUG("-> usb_hal_set_endpt_stall() - EP%u %s.\n", (ep_num & ~ENDPT_DIRECTION_MASK),
          ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

    usb_hal_get_ep_info_ptr(ep_num, &ep_info);

    if (ep_info->wrap_window_xfer)
    {
        // Cancel any active wrap window transfers before attempting to STALL to
        // prevent USB core from hanging waiting for DMA access.
        usb_hal_cancel_io_request(ep_num);
    }

    status = usb_hal_dep_cmd(ep_num, DEPCMD_TYPE_SET_STALL, 0, 0);

    if (status == STATUS_OK)
    {
        ep_info->bStalled = TRUE;
    }

    if (ep_num == (EP0 | ENDPT_DIRECTION_OUT))
    {
        // Prepare to receive setup packet.
        usb_hal_ep0_setup_stage();
    }

    return;
}


/*****************************************************************************
 * Function: usb_hal_clear_endpt_stall
 *************************************************************************//**
 * This function clears the stall condition for the endpoint specified.
 * Consequently, the data toggle (USB 2.0) will be reset to DATA0 and the
 * sequence number (USB 3.0) will be reset to zero.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_clear_endpt_stall(UINT32_T ep_num)
{
    STATUS_T status;
    EP_INFO_T *ep_info;

    DEBUG("-> usb_hal_clear_endpt_stall() - EP%u %s.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

    status = usb_hal_dep_cmd(ep_num, DEPCMD_TYPE_CLEAR_STALL, 0, 0);

    if (status == STATUS_OK)
    {
        usb_hal_get_ep_info_ptr(ep_num, &ep_info);
        ep_info->bStalled = FALSE;
    }

    return;
}


/*****************************************************************************
 * Function: usb_hal_is_endpt_stalled
 *************************************************************************//**
 * This function returns the STALL status of an endpoint.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval TRUE when endpoint is STALLED.
 * @retval FALSE otherwise.
 *
 ******************************************************************************
 */

BOOLEAN_T usb_hal_is_endpt_stalled(UINT32_T ep_num)
{
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        return usb_dev.ep_info_OUT[ep_num & ~ENDPT_DIRECTION_MASK].bStalled;
    }
    else /* IN */
    {
        return usb_dev.ep_info_IN[ep_num & ~ENDPT_DIRECTION_MASK].bStalled;
    }
}


/*****************************************************************************
 * Function: usb_hal_set_address
 *************************************************************************//**
 * This function is called by USB stack when host sends a set address
 * command.  Call this function before entering status stage.  HW will set new
 * address after host ACKs the zero length status packet.
 *
 * @param[in] address USB device address.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_set_address(UINT32_T address)
{
    if (address != 0)
    {
        CRIT("-> usb_hal_set_address() - addr: 0x%x.\n", address);
    }

    MODIFY32(USB_REG_OFF(DCFG_REG_OFF), DCFG_DEV_ADDR_MASK, (address << DCFG_DEV_ADDR_OFFSET) & DCFG_DEV_ADDR_MASK);

    return;
}



/*****************************************************************************
 * Function: usb_hal_config_ep0
 *************************************************************************//**
 * This function is used to configure the control endpoint.  It
 * does not enable the endpoint.
 *
 * @param[in] tx_fifo transmit FIFO number.
 * @param[in] config_xfer_resources flag indicating whether transfer resources
 * should be allocated. It should only be set to TRUE during HW initialization,
 * and FALSE otherwise.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_config_ep0(UINT32_T tx_fifo, BOOLEAN_T config_xfer_resources)
{
    UINT32_T max_pkt_sz;
    UINT32_T epcfg_par0 = 0;
    UINT32_T epcfg_par1 = 0;

    if (usb_dev.dev_speed != USB_SPEED_UNKNOWN)
    {
        max_pkt_sz = usb_endpt_max_pkt_size[EP0][usb_dev.dev_speed];
    }
    else
    {
        // Set default size of 512 bytes in case this function is called during init.
        max_pkt_sz = 512;
    }

    usb_dev.ep_info_OUT[EP0].wMaxPktSize = max_pkt_sz;
    usb_dev.ep_info_IN[EP0].wMaxPktSize = max_pkt_sz;

    DEBUG("-> usb_hal_config_ep0() - max_pkt_size = %u.\n", max_pkt_sz);

    // Configure EP0 OUT.
    epcfg_par0 = (PAR0_EPTYPE_CONTROL << PAR0_EPTYPE_OFFSET) | (max_pkt_sz << PAR0_MPS_OFFSET);
    epcfg_par1 = (PAR1_XFER_NRDY_EN | PAR1_XFER_CMPLT_EN | EP_INTR_NUM);
    usb_hal_dep_cmd((EP0 | ENDPT_DIRECTION_OUT), DEPCMD_TYPE_SET_EP_CONFIG, epcfg_par0, epcfg_par1);

    // Allocate transfer resources.
    if (config_xfer_resources)
    {
        usb_hal_dep_xfer_resource_config((EP0 | ENDPT_DIRECTION_OUT) , 1);
    }

    // Configure EP0 IN.
    epcfg_par1 |= PAR1_EP_DIR_BIT;  /* set IN direction bit */
    epcfg_par0 |= (tx_fifo << PAR0_FIFO_NUM_OFFSET) & PAR0_FIFO_NUM_MASK;  /* Set FIFO number. */
    usb_hal_dep_cmd((EP0 | ENDPT_DIRECTION_IN), DEPCMD_TYPE_SET_EP_CONFIG, epcfg_par0, epcfg_par1);

    // Allocate transfer resources.
    if (config_xfer_resources)
    {
        usb_hal_dep_xfer_resource_config((EP0 | ENDPT_DIRECTION_IN) , 1);
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_config_intr_endpt
 *************************************************************************//**
 * This function is used to configure the interrupt endpoint.  It
 * does not enable the endpoint.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] max_pkt_size maximum packet size in bytes.
 * @param[in] tx_fifo transmit FIFO number (only valid for IN endpts)..
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_config_intr_endpt(UINT32_T ep_num, UINT32_T max_pkt_size, UINT32_T tx_fifo)
{
    UINT32_T epcfg_par0 = 0;
    UINT32_T epcfg_par1 = 0;

    DEBUG("-> usb_hal_config_intr_endpt() - EP%u %s, MPS: %u bytes, FIFO #%u.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK),
          ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          max_pkt_size, tx_fifo);

    // Configure params.
    epcfg_par0 = (PAR0_EPTYPE_INTERRUPT << PAR0_EPTYPE_OFFSET) | (max_pkt_size << PAR0_MPS_OFFSET);

    epcfg_par1 = (((UINT32_T)(ep_num & ~ENDPT_DIRECTION_MASK) << PAR1_EP_NUM_OFFSET) |
                  PAR1_XFER_NRDY_EN | PAR1_XFER_IN_PROG_EN | PAR1_XFER_CMPLT_EN | EP_INTR_NUM);

    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        usb_dev.ep_info_OUT[ep_num & ~ENDPT_DIRECTION_MASK].wMaxPktSize = max_pkt_size;
    }
    else /* IN */
    {
        epcfg_par1 |= PAR1_EP_DIR_BIT;  /* set IN direction bit */
        epcfg_par0 |= (tx_fifo << PAR0_FIFO_NUM_OFFSET) & PAR0_FIFO_NUM_MASK;  /* Set FIFO number. */
        usb_dev.ep_info_IN[ep_num & ~ENDPT_DIRECTION_MASK].wMaxPktSize = max_pkt_size;
    }

    usb_hal_dep_cmd(ep_num, DEPCMD_TYPE_SET_EP_CONFIG, epcfg_par0, epcfg_par1);
    usb_hal_dep_xfer_resource_config(ep_num , 1);

    return;
}


/*****************************************************************************
 * Function: usb_hal_config_bulk_endpt
 *************************************************************************//**
 * This function is used to configure a bulk endpoint.  It
 * does not enable the endpoint.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] max_pkt_size maximum packet size in bytes.
 * @param[in] xfer_rsc_num number of transfer resources (must be at least 1).
 * @param[in] tx_fifo transmit FIFO number (only valid for IN endpts).
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_config_bulk_endpt(UINT32_T ep_num, UINT32_T max_pkt_size, UINT32_T xfer_rsc_num, UINT32_T tx_fifo)
{
    UINT32_T epcfg_par0 = 0;
    UINT32_T epcfg_par1 = 0;

    DEBUG("-> usb_hal_config_bulk_endpt() - EP%u %s, MPS: %u bytes, xfer_resources: %u, FIFO #%u.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          max_pkt_size, xfer_rsc_num, tx_fifo);

    // Configure params.
    epcfg_par0 = (PAR0_EPTYPE_BULK << PAR0_EPTYPE_OFFSET) | (max_pkt_size << PAR0_MPS_OFFSET);

    if (usb_dev.dev_speed == USB_SUPER_SPEED)
    {
        epcfg_par0 |= (MAX_BURST_SIZE << PAR0_BURST_SIZE_OFFSET);
    }

    epcfg_par1 = (((UINT32_T)(ep_num & ~ENDPT_DIRECTION_MASK) << PAR1_EP_NUM_OFFSET) |
                  /*PAR1_XFER_NRDY_EN |*/ PAR1_XFER_IN_PROG_EN | PAR1_XFER_CMPLT_EN | EP_INTR_NUM);

    if (xfer_rsc_num > 1)
    {
        // Set Stream capable bit if more than 1 transfer resource will be allocated.
        epcfg_par1 |= PAR1_STRM_CAP;

        // Workaround for WEBS PG3_0_Silicon.8 (CRM #9000416825) No ERDY transmitted for stream-capable OUT endpoint.
        if (usb_dev.usb_core_version == 0x120A)
        {
            // Enable stream events for OUT endpoints.
            if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
            {
                epcfg_par1 |= PAR1_STREAM_EVENT_EN;
            }
        }
    }

    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        // Save max packet size.
        usb_dev.ep_info_OUT[ep_num & ~ENDPT_DIRECTION_MASK].wMaxPktSize = max_pkt_size;
    }
    else /* IN */
    {
        epcfg_par1 |= PAR1_EP_DIR_BIT;  /* set IN direction bit. */
        epcfg_par0 |= (tx_fifo << PAR0_FIFO_NUM_OFFSET) & PAR0_FIFO_NUM_MASK;  /* Set Tx FIFO number. */

        // Save max packet size.
        usb_dev.ep_info_IN[ep_num & ~ENDPT_DIRECTION_MASK].wMaxPktSize = max_pkt_size;
    }

    usb_hal_dep_cmd(ep_num, DEPCMD_TYPE_SET_EP_CONFIG, epcfg_par0, epcfg_par1);

    usb_hal_dep_xfer_resource_config(ep_num , xfer_rsc_num);

    return;
}


#define NON_STREAM_ENDPT_XFER_RESOURCE_NUM  1
#define STREAM_ENDPT_XFER_RESOURCE_NUM      2

/*****************************************************************************
 * Function: usb_hal_configure_endpts
 *************************************************************************//**
 * This function is used to configure all endpoints except EP0 and is
 * called by the USB stack when a SET CONFIGURATION or SET INTERFACE
 * request is received. Data toggles and sequence numbers get reset.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_configure_endpts(void)
{
    UINT32_T num_xfer_rsc = NON_STREAM_ENDPT_XFER_RESOURCE_NUM;
    UINT32_T physical_endpts = (DALEPENA_EP0_OUT | DALEPENA_EP0_IN);

    if (usb_dev.dev_speed == USB_SPEED_UNKNOWN)
    {
        return;
    }

    // Disable all physical endpoints except EP0 IN/OUT.
    WRITE32(USB_REG_OFF(DALEPENA_REG_OFF), physical_endpts);

    while (!(READ32(USB_REG_OFF(DSTS_REG_OFF)) & DSTS_RX_FIFO_EMPTY_BIT))
    {
        // Wait for RX FIFO empty.
    }

    // Cancel any active USB transfers.
    usb_hal_cancel_all_io_requests();

    if (usb_dev.usb_core_version > 0x108A)
    {
        // Initialize transfer resource allocation with index 2.
        usb_hal_dep_cmd((EP0 | ENDPT_DIRECTION_OUT), (DEPCMD_START_NEW_CONFIG | (0x2 << DEPCMD_XFER_RSC_INDEX_OFFSET)), 0, 0);
    }

    // Config HID endpt.
    usb_hal_config_intr_endpt((USB_HID_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT), usb_endpt_max_pkt_size[USB_HID_OUT_ENDPT_NUM][usb_dev.dev_speed], 0);
    usb_hal_config_intr_endpt((USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN), usb_endpt_max_pkt_size[USB_HID_IN_ENDPT_NUM][usb_dev.dev_speed], USB_HID_IN_ENDPT_FIFO);

    usb_hal_clear_endpt_stall(USB_HID_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT);
    usb_hal_clear_endpt_stall(USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN);

    physical_endpts |= ((1 << LEP2PEP(USB_HID_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT)) |
                        (1 << LEP2PEP(USB_HID_IN_ENDPT_NUM | ENDPT_DIRECTION_IN)));

    // Config Bulk endpts if not operation at Low-Speed.
    if (usb_dev.dev_speed != USB_LOW_SPEED)
    {
        if (usb_dev.active_mass_storage_class == USB_MSC_UAS)
        {
            // Configure streams on appropriate endpts if operating at SuperSpeed and UAS is active.
            if (usb_dev.dev_speed == USB_SUPER_SPEED)
            {
                num_xfer_rsc = STREAM_ENDPT_XFER_RESOURCE_NUM;
            }

            // Configure UAS command endpt.
            usb_hal_config_bulk_endpt((UMS_UAS_CMD_ENDPT_NUM | ENDPT_DIRECTION_OUT),
                                      usb_endpt_max_pkt_size[UMS_UAS_CMD_ENDPT_NUM][usb_dev.dev_speed],
                                      NON_STREAM_ENDPT_XFER_RESOURCE_NUM, 0);  /* only 1 xfer resource on UMS command endpt */

            usb_hal_clear_endpt_stall(UMS_UAS_CMD_ENDPT_NUM | ENDPT_DIRECTION_OUT);

            // Configure UAS status endpt.
            usb_hal_config_bulk_endpt((UMS_UAS_STATUS_ENDPT_NUM | ENDPT_DIRECTION_IN),
                                      usb_endpt_max_pkt_size[UMS_UAS_STATUS_ENDPT_NUM][usb_dev.dev_speed],
                                      num_xfer_rsc, UMS_UAS_STATUS_ENDPT_FIFO);

            usb_hal_clear_endpt_stall(UMS_UAS_STATUS_ENDPT_NUM | ENDPT_DIRECTION_IN);

            physical_endpts |= ((1 << LEP2PEP(UMS_UAS_CMD_ENDPT_NUM | ENDPT_DIRECTION_OUT)) |
                                (1 << LEP2PEP(UMS_UAS_STATUS_ENDPT_NUM | ENDPT_DIRECTION_IN)));
        }

        // Configure UAS data OUT endpt. (alternately used as BOT bulk OUT endpt).
        usb_hal_config_bulk_endpt((UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT),
                                  usb_endpt_max_pkt_size[UMS_UAS_DATA_OUT_ENDPT_NUM][usb_dev.dev_speed],
                                  num_xfer_rsc, 0);

        usb_hal_clear_endpt_stall(UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT);


        // Configure UAS data IN endpt. (alternately used as BOT bulk IN endpt).
        usb_hal_config_bulk_endpt((UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN),
                                  usb_endpt_max_pkt_size[UMS_UAS_DATA_IN_ENDPT_NUM][usb_dev.dev_speed],
                                  num_xfer_rsc, UMS_UAS_DATA_IN_ENDPT_FIFO);

        usb_hal_clear_endpt_stall(UMS_UAS_DATA_IN_ENDPT_NUM| ENDPT_DIRECTION_IN);

        physical_endpts |= ((1 << LEP2PEP(UMS_UAS_DATA_OUT_ENDPT_NUM | ENDPT_DIRECTION_OUT)) |
                            (1 << LEP2PEP(UMS_UAS_DATA_IN_ENDPT_NUM | ENDPT_DIRECTION_IN)));

    }

    // Enable required physical endpoints.
    WRITE32(USB_REG_OFF(DALEPENA_REG_OFF), physical_endpts);

    return;
}


/*****************************************************************************
 * Function: usb_hal_ep0_setup_stage
 *************************************************************************//**
 * This function sets up a transfer to receive a setup packet on EP0.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_ep0_setup_stage(void)
{
    TRANSFER_REQUEST_BLOCK_T *trb;

    DEBUG("-> usb_hal_ep0_setup_stage()\n");

    usb_dev.ep0_state = EP0_STATE_IDLE;
    usb_dev.ep0_zlp_pending = FALSE;

    // Get pointer to TRB.
    trb = &datapath_ram->trb_ep0_setup_packet;

    // Populate TRB.
    WRITE32((UINT32_T)&trb->dBufferPtrLow, (UINT32_T)&datapath_ram->ep0_buffer[0]);
    WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
    WRITE32((UINT32_T)&trb->dStatus, SETUP_PACKET_DATA_LENGTH);
    WRITE32((UINT32_T)&trb->dControl, ((TRBCTRL_CONTROL_SETUP << TRB_CTRL_TRBCTL_OFFSET) |
                                       TRB_CTRL_HWO_BIT | TRB_CTRL_LST_BIT));

    INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
    INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
    INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
    INFO("  TRB[3] = 0x%08x.\n", trb->dControl);

    // Start xfer.
    usb_hal_dep_xfer_start((EP0 | ENDPT_DIRECTION_OUT), trb, SETUP_PACKET_DATA_LENGTH, 0);

    if (!usb_dev.bUSBResetInitComplete)
    {
        DEBUG("\nUSB Reset Stage 2.\n");

        usb_hal_handle_usb_reset_stage2();
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_ep0_status_stage
 *************************************************************************//**
 * This function sets up a TRB to receive or send a zero length packet on EP0.
 *
 * @param[in] direction endpoint direction. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_ep0_status_stage(ENDPT_DIR_T direction)
{
    TRANSFER_REQUEST_BLOCK_T *trb;
    UINT32_T trbctl;

    DEBUG("-> usb_hal_ep0_status_stage() - %s.\n", (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

    // Get TRB pointer.
    trb = (direction == ENDPT_DIRECTION_OUT) ? &datapath_ram->trb_data_OUT[EP0] : &datapath_ram->trb_data_IN[EP0];

    // Determine the TRB type.
    trbctl = (usb_dev.ep0_three_stage_xfer) ? TRBCTRL_CONTROL_STATUS_3 : TRBCTRL_CONTROL_STATUS_2;

    // Populate TRB.
    WRITE32((UINT32_T)&trb->dBufferPtrLow, 0);  /* no data buffer req'd */
    WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
    WRITE32((UINT32_T)&trb->dStatus, 0);
    WRITE32((UINT32_T)&trb->dControl, ((trbctl << TRB_CTRL_TRBCTL_OFFSET) |
                                       TRB_CTRL_HWO_BIT | TRB_CTRL_LST_BIT));

    INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
    INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
    INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
    INFO("  TRB[3] = 0x%08x.\n", trb->dControl);

    // Start xfer.
    usb_hal_dep_xfer_start((EP0 | direction), trb, 0, 0);

    return;
}


/*****************************************************************************
 * Function: usb_hal_get_connect_speed
 *************************************************************************//**
 * This function returns the device connection speed w/ host and
 * can be called after the connect done event.
 *
 * @param None.
 *
 * @return The connection speed:
 * - USB_LOW_SPEED
 * - USB_FULL_SPEED
 * - USB_HIGH_SPEED
 * - USB_SUPER_SPEED
 * - USB_SPEED_UNKNOWN
 *
 ******************************************************************************
 */

eUSB_DEVICE_SPEED_T usb_hal_get_connect_speed()
{
    UINT32_T dsts;
    eUSB_DEVICE_SPEED_T speed;

    dsts = READ32(USB_REG_OFF(DSTS_REG_OFF)) & DSTS_SPEED_MASK;

    switch (dsts)
    {
        case DSTS_SPEED_SS_PHY_125MHZ_OR_250MHZ:
            speed = USB_SUPER_SPEED;
            break;

        case DSTS_SPEED_HS_PHY_30MHZ_OR_60MHZ:
            speed = USB_HIGH_SPEED;
            break;

        case DSTS_SPEED_FS_PHY_30MHZ_OR_60MHZ:
        case DSTS_SPEED_FS_PHY_48MHZ:
            speed = USB_FULL_SPEED;
            break;

        case DSTS_SPEED_LS_PHY_6MHZ:
            speed = USB_LOW_SPEED;  /* We don't support low speed */
            break;

        default:
            CRIT("@Error Invalid connection speed in DSTS register = %u\n.", speed);
            speed = USB_SPEED_UNKNOWN;
            break;
    }

    CRIT("Connected at %s speed.\n", (speed == USB_SUPER_SPEED) ? "SUPER" :
         (speed == USB_HIGH_SPEED) ? "HIGH" : (speed == USB_FULL_SPEED) ? "FULL" :
         (speed == USB_LOW_SPEED) ? "LOW" : "UNKNOWN");

    return speed;
}

/*****************************************************************************
 * Function: usb_hal_update_transfer
 *************************************************************************//**
 * This function issues an update transfer commmand to the USB core to re-cache
 * a TRB.  It is used whenever a TRB whose HWO bit was zero is updated to 1
 * as part of a circular TRB buffer ring.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_update_transfer(UINT32_T ep_num)
{
    EP_INFO_T *ep_info;

    // Get pointer to endpoint info.
    usb_hal_get_ep_info_ptr(ep_num, &ep_info);

    INFO("-> usb_hal_update_transfer() - xfer_rsc_index = %u.\n", ep_info->dXferRscIndex);

    return usb_hal_dep_cmd(ep_num, ((ep_info->dXferRscIndex << DEPCMD_XFER_RSC_INDEX_OFFSET) | DEPCMD_TYPE_UPDATE_XFER), 0, 0);
}



/*
 * TRB xfer lengths for IN transfers must be limited so that datapath RAM can
 * be freed up for use by the SATA DMA as TRBs are completed.  The default
 * size is half of the datapath RAM transfer window size.  TRB xfer lengths
 * for OUT transfers are limited by the virtual size of the wrap window (248KB).
 */
#define RING_TRB_IN_XFER_LENGTH_BOT  (32 * 1024)  /* bytes */
#define RING_TRB_IN_XFER_LENGTH_UAS  (16 * 1024)  /* bytes */
#define RING_TRB_OUT_XFER_LENGTH    (128 * 1024)  /* bytes */


/*****************************************************************************
 * Function: usb_hal_config_next_trb
 *************************************************************************//**
 * This function configures the next available TRB in the ring.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when command times out.
 * @retval STATUS_ERROR when there are zero bytes remaining.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_config_next_trb(UINT32_T ep_num)
{
    TRB_RING_INFO_T *ring_info;
    TRANSFER_REQUEST_BLOCK_T *trb;
    UINT32_T cntrl;
    UINT32_T xfer_length;
    UINT32_T max_ring_index;
    UINT32_T wrap_window_overflow_mask;
    STATUS_T status;

    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        // Get pointers to TRB ring info and TRB.
        ring_info = &usb_dev.trb_ring_info_OUT;
        trb = &datapath_ram->trb_ring_OUT[ring_info->index];

        // Set max ring index.
        max_ring_index = (TRB_RING_OUT_SIZE - 1);

        // Determine xfer length.
        xfer_length = MIN(ring_info->bytes_remaining, RING_TRB_OUT_XFER_LENGTH);
    }
    else /* IN */
    {
        // Get pointers to TRB ring info and TRB.
        ring_info = &usb_dev.trb_ring_info_IN;
        trb = &datapath_ram->trb_ring_IN[ring_info->index];

        // Set max ring index.
        max_ring_index = (TRB_RING_IN_SIZE - 1);

        if (usb_dev.active_mass_storage_class == USB_MSC_BOT)
        {
            // Determine xfer length.
            xfer_length = MIN(ring_info->bytes_remaining, RING_TRB_IN_XFER_LENGTH_BOT);
        }
        else
        {
            // Determine xfer length.
            xfer_length = MIN(ring_info->bytes_remaining, RING_TRB_IN_XFER_LENGTH_UAS);
        }
    }

    if (usb_dev.active_mass_storage_class == USB_MSC_BOT)
    {
        // Set overflow mask for 64KB window size.
        wrap_window_overflow_mask = MWW_64KB_BLK_ADDR_OVERFLOW_MASK;
    }
    else
    {
        // Set overflow mask for 32KB window size.
        wrap_window_overflow_mask = MWW_32KB_BLK_ADDR_OVERFLOW_MASK;
    }

    INFO("-> usb_hal_config_next_trb() - TRB_index = %u, buff 0x%08x, bytes_remaining = %u, sid = 0x%x.\n",
         ring_info->index, ring_info->buffer_ptr, ring_info->bytes_remaining, ring_info->stream_id);

    if (ring_info->bytes_remaining == 0)
    {
        CRIT("@Error - attempt to configure next ring TRB with zero bytes remaining!\n");
        return STATUS_ERROR;
    }

    // Make sure buffer pointer is inside wrap window bounds.
    ring_info->buffer_ptr = (UINT8_T*)((UINT32_T)ring_info->buffer_ptr & ~wrap_window_overflow_mask);

    // Update bytes remaining.
    ring_info->bytes_remaining -= xfer_length;

    // Setup TRB control bits.
    cntrl = ((TRBCTRL_NORMAL << TRB_CTRL_TRBCTL_OFFSET) |
             ((ring_info->stream_id << TRB_CTRL_STRM_ID_OFFSET) & TRB_CTRL_STRM_ID_MASK) |
             TRB_CTRL_IOC_BIT | TRB_CTRL_HWO_BIT);

    // Set chain bit for all TRB except the last one.
    cntrl |= (ring_info->bytes_remaining == 0) ? TRB_CTRL_LST_BIT : TRB_CTRL_CHN_BIT;

    WRITE32((UINT32_T)&trb->dBufferPtrLow, (UINT32_T)ring_info->buffer_ptr);
    WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
    WRITE32((UINT32_T)&trb->dStatus, xfer_length);
    WRITE32((UINT32_T)&trb->dControl, cntrl);

    INFO("Configure next ring TRB[%u]:\n", ring_info->index);
    INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
    INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
    INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
    INFO("  TRB[3] = 0x%08x.\n", trb->dControl);

    // Store xfer length for this TRB.
    ring_info->xfer_length[ring_info->index] = xfer_length;

    // Update buffer pointer.
    ring_info->buffer_ptr += xfer_length;

    // Increment TRB index.
    ring_info->index++;

    // Check for TRB index wrap-around. (we don't want to overwrite LINK TRB)
    if (ring_info->index >= max_ring_index)
    {
        ring_info->index = 0;
    }

    if ((usb_dev.usb_core_version < 0x109A) &&
        ((READ_REG32(DEPCMD(ep_num)) & 0x000000FF) == DEPCMD_TYPE_UPDATE_XFER))
    {
        // Update transfer is already in progress so just return.
        status = STATUS_OK;
    }
    else
    {
        // Issue update transfer command.
        status = usb_hal_update_transfer(ep_num);
    }

    return status;
}

/*****************************************************************************
 * Function: usb_hal_config_trb_ring
 *************************************************************************//**
 * This function configures the circular TRB structure.
 *
 * @param[in] direction endpoint direction. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] byte_cnt total size of transfer in bytes.
 * @param[in] stream_id stream ID.
 *
 * @return Pointer to the first Transfer Request Block in the ring
 *
 ******************************************************************************
 */

inline TRANSFER_REQUEST_BLOCK_T* usb_hal_config_trb_ring(UINT32_T direction, UINT32_T byte_cnt, UINT32_T stream_id)
{
    TRB_RING_INFO_T *ring_info;
    TRANSFER_REQUEST_BLOCK_T *first_trb;
    TRANSFER_REQUEST_BLOCK_T *trb;
    UINT32_T index = 0;
    UINT32_T cntrl;
    UINT32_T max_xfer_length;
    UINT32_T max_ring_index;
    UINT32_T xfer_length;
    UINT32_T wrap_window_overflow_mask;

    if (usb_dev.active_mass_storage_class == USB_MSC_BOT)
    {
        // Set overflow mask for 64KB window size.
        wrap_window_overflow_mask = MWW_64KB_BLK_ADDR_OVERFLOW_MASK;
    }
    else
    {
        // Set overflow mask for 32KB window size.
        wrap_window_overflow_mask = MWW_32KB_BLK_ADDR_OVERFLOW_MASK;
    }

    if (direction == ENDPT_DIRECTION_OUT)
    {
        // Get pointers to TRB ring info and TRB.
        ring_info = &usb_dev.trb_ring_info_OUT;
        trb = &datapath_ram->trb_ring_OUT[0];

        // Set max ring parameters.
        max_xfer_length = RING_TRB_OUT_XFER_LENGTH;
        max_ring_index = (TRB_RING_OUT_SIZE - 1);
    }
    else /* IN */
    {
        // Get pointers to TRB ring info and TRB.
        ring_info = &usb_dev.trb_ring_info_IN;
        trb = &datapath_ram->trb_ring_IN[0];
        // Set max ring parameters.
        max_xfer_length =  (usb_dev.active_mass_storage_class == USB_MSC_BOT) ? RING_TRB_IN_XFER_LENGTH_BOT : RING_TRB_IN_XFER_LENGTH_UAS;
        max_ring_index = (TRB_RING_IN_SIZE - 1);
    }

    INFO("-> usb_hal_config_trb_ring() - (%s) TRB_ptr = 0x%08x, buff = 0x%08x, byte_cnt = %u, sid = 0x%x.\n",
         (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", (UINT32_T)trb, (UINT32_T)ring_info->buffer_ptr, byte_cnt, stream_id);

    // Save pointer to first TRB in the ring.
    first_trb = trb;

    do
    {
        // Determine xfer length.
        xfer_length = MIN(byte_cnt, max_xfer_length);

        // Make sure buffer pointer is inside wrap window bounds.
        ring_info->buffer_ptr = (UINT8_T*)((UINT32_T)ring_info->buffer_ptr & ~wrap_window_overflow_mask);

        // Update byte count.
        byte_cnt -= xfer_length;

        // Setup TRB control bits.
        cntrl = ((TRBCTRL_NORMAL << TRB_CTRL_TRBCTL_OFFSET) |
                 ((stream_id << TRB_CTRL_STRM_ID_OFFSET) & TRB_CTRL_STRM_ID_MASK) | TRB_CTRL_IOC_BIT | TRB_CTRL_HWO_BIT);

        // Workaround for WEBS PG3_0_Silicon.8 (CRM #9000416825) No ERDY transmitted for stream-capable OUT endpoint.
        if (usb_dev.usb_core_version == 0x120A)
        {
            if ((stream_id != 0) && (index == 0) && (direction == ENDPT_DIRECTION_OUT))
            {
                // Clear HWO bit for 1st TRB.
                cntrl &= ~TRB_CTRL_HWO_BIT;
            }
        }

        // Set chain bit for all TRB except the last one.
        cntrl |= (byte_cnt == 0) ? TRB_CTRL_LST_BIT : TRB_CTRL_CHN_BIT;

        // Populate TRB.
        WRITE32((UINT32_T)&trb->dBufferPtrLow, (UINT32_T)ring_info->buffer_ptr);
        WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
        WRITE32((UINT32_T)&trb->dStatus, xfer_length);
        WRITE32((UINT32_T)&trb->dControl, cntrl);

        // Store xfer length for this TRB.
        ring_info->xfer_length[index] = xfer_length;

        INFO("Configure %s TRB[%u]:\n", (byte_cnt == 0) ? "last" : "chain", index);
        INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
        INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
        INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
        INFO("  TRB[3] = 0x%08x.\n", trb->dControl);

        // Increment buffer and TRB pointers.
        ring_info->buffer_ptr += xfer_length;
        trb++;
        index++;

    } while ((byte_cnt > 0) && (index < max_ring_index));

    if (index == max_ring_index)
    {
        // Configure link TRB.
        WRITE32((UINT32_T)&trb->dBufferPtrLow, (UINT32_T)first_trb);
        WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
        WRITE32((UINT32_T)&trb->dStatus, 0);
        WRITE32((UINT32_T)&trb->dControl, (TRBCTRL_LINK_TRB << TRB_CTRL_TRBCTL_OFFSET) | TRB_CTRL_HWO_BIT);

        INFO("Configure link TRB[%u]:.\n", index);
        INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
        INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
        INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
        INFO("  TRB[3] = 0x%08x.\n", trb->dControl);

        // Wrap index around to zero.
        index = 0;
    }

    // Save buffer pointer and TRB ring index.
    ring_info->index = index;
    ring_info->bytes_remaining = byte_cnt;
    ring_info->stream_id = stream_id;

    // Initialize pending TRB index to zero.
    ring_info->index_pending = 0;

    INFO(" buff_ptr = 0x%08x, TRB_index = %u, bytes_remaining = %u.\n", (UINT32_T)ring_info->buffer_ptr, index, byte_cnt);

    return first_trb;
}

/*****************************************************************************
 * Function: usb_hal_io_request
 *************************************************************************//**
 * This function is used to Tx/Rx data for any non-control endpoint.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] buffer pointer to data buffer (must be located in datapath RAM).
 * @param[in] byte_cnt total size of transfer in bytes.
 * @param[in] stream_id stream ID.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_XFER_ACTIVE when there is already a transfer in progress.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_io_request(UINT32_T ep_num, void *buffer, UINT32_T byte_cnt, UINT32_T stream_id)
{
    TRANSFER_REQUEST_BLOCK_T *trb;
    UINT32_T rounded_byte_cnt;
    EP_INFO_T *ep_info;

    usb_hal_get_ep_info_ptr(ep_num, &ep_info);

    if (ep_info->bXferActive)
    {
        DEBUG("@Warning: usb_hal_io_request() - xfer active on EP%u %s.\n",
              (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN");

        return STATUS_XFER_ACTIVE;
    }

    if (((UINT32_T)buffer == SATA_TO_USB_WRAP_WINDOW_ADDR) ||
        ((UINT32_T)buffer == USB_TO_SATA_WRAP_WINDOW_ADDR))
    {
        ep_info->wrap_window_xfer = TRUE;
    }
    else
    {
        ep_info->wrap_window_xfer = FALSE;
    }

    // Get pointer to TRB and set wrap window transfer direction.
    if ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT)
    {
        // Get TRB pointer.
        trb = &datapath_ram->trb_data_OUT[ep_num & ~ENDPT_DIRECTION_MASK];

        // Round byte count to next highest multiple of the max packet size.
        rounded_byte_cnt = (byte_cnt + ep_info->wMaxPktSize - 1) / ep_info->wMaxPktSize;
        rounded_byte_cnt *= ep_info->wMaxPktSize;
    }
    else /* IN */
    {
        // Get TRB pointer.
        trb = &datapath_ram->trb_data_IN[ep_num & ~ENDPT_DIRECTION_MASK];

        // No rounding of byte count required for IN endpts.
        rounded_byte_cnt = byte_cnt;
    }

    if (ep_info->wrap_window_xfer)
    {
        // Configure TRB ring (and get pointer to first TRB in the ring).
        trb = usb_hal_config_trb_ring((ep_num & ENDPT_DIRECTION_MASK), rounded_byte_cnt, stream_id);
    }
    else
    {
        // Populate TRB.
        WRITE32((UINT32_T)&trb->dBufferPtrLow, (UINT32_T)buffer);
        WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
        WRITE32((UINT32_T)&trb->dStatus, rounded_byte_cnt);
        WRITE32((UINT32_T)&trb->dControl, ((TRBCTRL_NORMAL << TRB_CTRL_TRBCTL_OFFSET) |
                                           ((stream_id << TRB_CTRL_STRM_ID_OFFSET) & TRB_CTRL_STRM_ID_MASK) |
                                           TRB_CTRL_IOC_BIT | TRB_CTRL_HWO_BIT | TRB_CTRL_LST_BIT));

        INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
        INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
        INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
        INFO("  TRB[3] = 0x%08x.\n", trb->dControl);
    }

    DEBUG("-> usb_hal_io_request() - EP%u %s, %u (%u) bytes, buff = 0x%08x, sid = 0x%x.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          byte_cnt, rounded_byte_cnt, (UINT32_T)buffer, stream_id);

    // Start xfer.
    return usb_hal_dep_xfer_start(ep_num, trb, byte_cnt, stream_id);
}


/*****************************************************************************
 * Function: usb_hal_ep0_io_request
 *************************************************************************//**
 * This function is used to start Tx/Rx data on EP0.
 *
 * @param[in] direction endpoint direction. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] buffer pointer to data buffer (must be located in datapath RAM).
 * @param[in] byte_cnt total size of transfer in bytes.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_XFER_ACTIVE when there is already a transfer in progress.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_ep0_io_request(ENDPT_DIR_T direction, void *buffer, UINT32_T byte_cnt)
{
    EP_INFO_T *ep_info;
    TRANSFER_REQUEST_BLOCK_T *trb;
    UINT32_T rounded_byte_cnt = byte_cnt;

    usb_hal_get_ep_info_ptr((EP0 | direction), &ep_info);

    if (ep_info->bXferActive)
    {
        DEBUG("@Warning: usb_hal_ep0_io_request() - xfer active!\n");
        return STATUS_XFER_ACTIVE;
    }

    DEBUG("-> usb_hal_ep0_io_request() %s, %u bytes, buff = 0x%08x.\n", (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", byte_cnt, (UINT32_T)buffer);

    if (direction == ENDPT_DIRECTION_OUT)
    {
        // Round byte count to next highest multiple of the max packet size.
        rounded_byte_cnt = (byte_cnt + ep_info->wMaxPktSize - 1) / ep_info->wMaxPktSize;
        rounded_byte_cnt *= ep_info->wMaxPktSize;
    }

    trb = (direction == ENDPT_DIRECTION_OUT) ? &datapath_ram->trb_data_OUT[EP0] : &datapath_ram->trb_data_IN[EP0];

    // Populate TRB.
    WRITE32((UINT32_T)&trb->dBufferPtrLow, (UINT32_T)buffer);
    WRITE32((UINT32_T)&trb->dBufferPtrHigh, 0);
    WRITE32((UINT32_T)&trb->dStatus, MIN(rounded_byte_cnt, usb_dev.ep_info_OUT[EP0].wMaxPktSize));
    WRITE32((UINT32_T)&trb->dControl, ((TRBCTRL_CONTROL_DATA << TRB_CTRL_TRBCTL_OFFSET) |
                                       TRB_CTRL_IOC_BIT | TRB_CTRL_HWO_BIT | TRB_CTRL_LST_BIT));

    INFO("  TRB[0] = 0x%08x.\n", trb->dBufferPtrLow);
    INFO("  TRB[1] = 0x%08x.\n", trb->dBufferPtrHigh);
    INFO("  TRB[2] = 0x%08x.\n", trb->dStatus);
    INFO("  TRB[3] = 0x%08x.\n", trb->dControl);

    // Start xfer.
    return usb_hal_dep_xfer_start((EP0 | direction), trb, byte_cnt, 0);
}


/*****************************************************************************
 * Function: usb_hal_ep0_io_continue
 *************************************************************************//**
 * This function is used to continue Tx/Rx data on EP0 when data
 * length is greater than the max packet size or when it is necessary to Tx/Rx
 * a zero length packet to end a data transfer w/ the host.
 *
 * @param[in] direction endpoint direction. Bit 7 indicates the direction (0=OUT, 1=IN).
 * @param[in] buffer pointer to data buffer (must be located in datapath RAM).
 * @param[in] byte_cnt total size of transfer in bytes.
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_ep0_io_continue(ENDPT_DIR_T direction, void *buffer, UINT32_T byte_cnt)
{
    DEBUG("-> usb_hal_ep0_io_continue() %s, %u bytes, buff = 0x%08x.\n", (direction == ENDPT_DIRECTION_OUT) ? "OUT" : "IN", byte_cnt, (UINT32_T)buffer);

    return usb_hal_io_request((EP0 | direction), buffer, byte_cnt, 0);
}

#define END_XFER_CMD_TIMEOUT_US   10000  /* us */

/*****************************************************************************
 * Function: usb_hal_cancel_io_request
 *************************************************************************//**
 * This function is used to end DMA transfers for an endpoint.
 *
 * @param[in] ep_num endpoint number. Bit 7 indicates the direction (0=OUT, 1=IN).
 *
 * @retval STATUS_OK when successful.
 * @retval STATUS_TIMEOUT when command times out.
 *
 ******************************************************************************
 */

STATUS_T usb_hal_cancel_io_request(UINT32_T ep_num)
{
    STATUS_T status;
    EP_INFO_T *ep_info;

    usb_hal_get_ep_info_ptr(ep_num, &ep_info);

    if (!ep_info->bXferActive)
    {
        return STATUS_OK;
    }

    DEBUG("-> usb_hal_cancel_io_request() - EP%u %s, xfer_resource = %u.\n",
          (ep_num & ~ENDPT_DIRECTION_MASK), ((ep_num & ENDPT_DIRECTION_MASK) == ENDPT_DIRECTION_OUT) ? "OUT" : "IN",
          ep_info->dXferRscIndex);

    // Issue command.  (IOC bit must be set for End Transfer commands)
    status = usb_hal_dep_cmd(ep_num, ((ep_info->dXferRscIndex << DEPCMD_XFER_RSC_INDEX_OFFSET) | DEPCMD_TYPE_END_XFER | DEPCMD_CMD_IOC_BIT | DEPCMD_HIPRI_FORCE_RM_BIT), 0, 0);

    if (status == STATUS_OK)
    {
        ep_info->bXferActive = FALSE;
    }

    // Reset WDT.
    wdt_reset();

    // Wait for completion of command.
    status = usb_hal_wait_depcmd_complete(ep_num, END_XFER_CMD_TIMEOUT_US);

    return status;
}


/*****************************************************************************
 * Function: usb_hal_cancel_all_io_requests
 *************************************************************************//**
 * This function is used to end DMA transfers for all endpoints except EP0.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_cancel_all_io_requests(void)
{
    UINT32_T ep_num;

    // Cancel all I/O requests except EP0.
    for (ep_num = EP1; ep_num <= MAX_EP_NUM; ep_num++)
    {
        usb_hal_cancel_io_request((ep_num | ENDPT_DIRECTION_OUT));
        usb_hal_cancel_io_request((ep_num | ENDPT_DIRECTION_IN));
    }

    return;
}


/*****************************************************************************
 * Function: usb_hal_set_test_mode
 *************************************************************************//**
 * This function is used to configure USB test modes.
 *
 * @param[in] test_mode test mode:
 * - NORMAL_OPERATION
 * - TEST_J
 * - TEST_K
 * - TEST_SE_NAK
 * - TEST_PACKET
 * - TEST_FORCE_ENABLE
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_set_test_mode(UINT32_T test_mode)
{
    CRIT("-> usb_hal_set_test_mode() - %u.\n", test_mode);

    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_TEST_MODE_MASK, (test_mode << DCTL_TEST_MODE_OFFSET));

    return;
}


/*****************************************************************************
 * Function: usb_hal_remote_wakeup
 *************************************************************************//**
 * This function is used to issue a remote wakeup request when
 * the device is in suspend or early suspend state (USB 2.0 only).
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_remote_wakeup(void)
{
    UINT32_T state;

    if ((usb_dev.dev_speed == USB_SUPER_SPEED) || (!usb_dev.bRemoteWakeupEnabled[0]))
    {
        return;
    }

    CRIT("-> usb_hal_remote_wakeup()\n");

    state = usb_hal_get_usb_link_state();

    if ((state == SUSPEND_STATE) || (state == EARLY_SUSPEND_STATE))
    {
        // Clear the link state change request field.
        MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, 0);

        // Issue remote wakeup request.
        MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, (DCTL_REMOTE_WAKEUP_REQ << DCTL_UL_STATE_CHNG_REQ_OFFSET));
    }
    else
    {
        CRIT("@Error: Link state = 0x%x. Can't send wakeup request.\n", state);
    }

    return;
}

/*****************************************************************************
 * Function: usb_hal_function_wake_device
 *************************************************************************//**
 * This function is used to issue a function wake device notification
 * (USB 3.0 only).
 *
 * @param[in] interface_num interface number that caused remote wakeup.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_function_wake_device(UINT32_T interface_num)
{
    CRIT("-> usb_hal_function_wake_device() - interface_num = %u.\n", interface_num);

    if ((usb_dev.dev_speed != USB_SUPER_SPEED) || (!usb_dev.bRemoteWakeupEnabled[interface_num]))
    {
        return;
    }

    // Clear the link state change request field.
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, 0);

    // Set link state change request to RECOVERY.
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_UL_STATE_CHNG_REQ_MASK, (RECOVERY_STATE << DCTL_UL_STATE_CHNG_REQ_OFFSET));

    // Set pending flag and save interface number so function wake device notification can be
    // sent when Recovery link state event occurs.
    usb_dev.bFunctionWakeDeviceNotificationPending = TRUE;
    usb_dev.bFunctionWakeInterfaceNum = interface_num;

    return;
}


/*****************************************************************************
 * Function: usb_hal_set_U1_initiate_enable
 *************************************************************************//**
 * This function is enables or disables the ability of the device
 * to initiate transitions to U1.
 *
 * @param[in] enable flag to enable or disable the feature.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_set_U1_initiate_enable(BOOLEAN_T enable)
{
#if ENABLE_U1_U2_TRANSITIONS
    INFO("-> usb_hal_set_U1_initiate_enable() - %u.\n", enable);

    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_U1_INITIATE_ENABLE_BIT, (enable) ? DCTL_U1_INITIATE_ENABLE_BIT : 0);

    // BQ - This workaround is not required because U1/U2 initiate enable is only cleared within RECOVERY link state change ISR.
#if DISABLE_U1_U2_INITIATE_WHILE_XFER_ACTIVE
//    if (usb_dev.usb_core_version < 0x180a)
//    {
//        if (!enable)
//        {
//            if (U1_STATE == usb_hal_get_usb_link_state())
//            {
//                CRIT("@Warning: Stuck in U1.\n");
//                // Workaround for CRM #9000444281 If link enters U1/U2 and the corresponding bit from DCTL[12:9] is cleared, link stays in U1/U2.
//                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_U1_INITIATE_ENABLE_BIT, DCTL_U1_INITIATE_ENABLE_BIT);
//                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_U1_INITIATE_ENABLE_BIT, 0);
//                CRIT("New state = U%u.\n", usb_hal_get_usb_link_state());
//            }
//        }
//    }
#endif

#endif

    return;
}


/*****************************************************************************
 * Function: usb_hal_set_U2_initiate_enable
 *************************************************************************//**
 * This function is enables or disables the ability of the device
 * to initiate transitions to U2.
 *
 * @param[in] enable flag to enable or disable the feature.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_set_U2_initiate_enable(BOOLEAN_T enable)
{
#if ENABLE_U1_U2_TRANSITIONS
    INFO("-> usb_hal_set_U2_initiate_enable() - %u.\n", enable);

    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_U2_INITIATE_ENABLE_BIT, (enable) ? DCTL_U2_INITIATE_ENABLE_BIT : 0);

    // BQ - This workaround is not required because U1/U2 initiate enable is only cleared within RECOVERY link state change ISR.
#if DISABLE_U1_U2_INITIATE_WHILE_XFER_ACTIVE
//    if (usb_dev.usb_core_version < 0x180a)
//    {
//        if (!enable)
//        {
//            if (U2_STATE == usb_hal_get_usb_link_state())
//            {
//                CRIT("@Warning: Stuck in U2.\n");
//                // Workaround for CRM #9000444281 If link enters U1/U2 and the corresponding bit from DCTL[12:9] is cleared, link stays in U1/U2.
//                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_U2_INITIATE_ENABLE_BIT, DCTL_U2_INITIATE_ENABLE_BIT);
//                MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_U2_INITIATE_ENABLE_BIT, 0);
//                CRIT("New state = U%u.\n", usb_hal_get_usb_link_state());
//            }
//        }
//    }
#endif

#endif

    return;
}


/*****************************************************************************
 * Function: usb_hal_set_sel
 *************************************************************************//**
 * This function sets the parameters for system exit latency.
 *
 * @param[in] u2pel time in us for U2 Device to Host Exit Latency
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_set_sel(UINT32_T u2pel)
{
    DEBUG("-> usb_hal_set_sel() - U2PEL = %u us.\n", u2pel);

#if ENABLE_U1_U2_TRANSITIONS

#if 0   // Currently system exit latency configuration is not supported in HW.
    // Program U2PEL.
    WRITE32(USB_REG_OFF(DGCMDPAR_REG_OFF), (u2pel > 125) ? 0 : u2pel);
    WRITE32(USB_REG_OFF(DGCMD_REG_OFF), (DGCMD_CMD_ACT_BIT | DGCMD_TYPE_SET_SEL_PARAM));
#endif

#endif

    return;
}


/*****************************************************************************
 * Function: usb_hal_connect
 *************************************************************************//**
 * This function enables the USB controller and connects to the
 * upstream port.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_connect(void)
{
    CRIT("-> usb_hal_connect()\n");

    // Set DCTL.Run_Stop to 1 to enable the USB controller and connect to upstream port.
    // (EP0 will be configured & enabled upon USB Reset)
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_RUN_BIT, DCTL_RUN_BIT);

    return;
}


/*****************************************************************************
 * Function: usb_hal_disconnect
 *************************************************************************//**
 * This function disables the USB controller and disconnects from
 * the upstream port.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_disconnect(void)
{
    if (READ32(USB_REG_OFF(DCTL_REG_OFF)) & DCTL_RUN_BIT)
    {
        CRIT("-> usb_hal_disconnect()\n");

        usb_hal_cancel_all_io_requests();

        // Set DCTL.Run_Stop to 0 to disable the USB controller.
        MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_RUN_BIT, 0);

        // Set USB link state to RX_DETECT.
        MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_TARGET_UL_STATE_MASK, (RX_DETECT_STATE << DCTL_TARGET_UL_STATE_OFFSET));

        if (usb_dev.usb_core_version < 0x108A)
        {
            // Reset USB controller.
            MODIFY32(USB_REG_OFF(DCTL_REG_OFF), DCTL_SOFT_RESET_BIT, DCTL_SOFT_RESET_BIT);

            while (READ32(USB_REG_OFF(DCTL_REG_OFF)) & DCTL_SOFT_RESET_BIT)
            {
                // Wait till reset is complete.
            }
        }

        // Wait at least 33 ms so USB hub can detect that the device has been removed.
        // Extending time to 500 ms to ensure host sees disconnect.
        msleep(500);
    }

    return;
}


#define USB_CORE_VERSION_MASK  0xFFFF

/*****************************************************************************
 * Function: usb_hal_init
 *************************************************************************//**
 * This function is used to register callbacks and initialize the USB controller.
 * The function pointer arguments cannot be NULL the first time this function is called.
 * If NULL function pointer arguments are specified, the previously registered
 * callbacks will persist and the USB core will be re-initialized.
 * usb_hal_connect() must be called after this function to enable the USB controller
 * and connect to upstream port.
 *
 * @param[in] pSetupPktCallback function pointer to setup packet callback.
 * @param[in] pDataXferCallback function pointer to data transfer callback.  Bit 7 of the ep_num indicates the direction (0=OUT, 1=IN).
 * @param[in] pPowerMngmtCallback function pointer to power management callback.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usb_hal_init(void (*pSetupPktCallback)(void), void (*pDataXferCallback)(UINT32_T ep_num, EP_INFO_T *ep_info), void (*pPowerMngmtCallback)(eUSB_DEVICE_PM_STATE_T pm_state))
{
    UINT32_T ep_num;

    CRIT("-> usb_hal_init()\n");

    usb_hal_cancel_all_io_requests();

    // Clear Run bit and Reset USB controller.
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), (DCTL_RUN_BIT | DCTL_SOFT_RESET_BIT), DCTL_SOFT_RESET_BIT);

    while (READ32(USB_REG_OFF(DCTL_REG_OFF)) & DCTL_SOFT_RESET_BIT)
    {
        // Wait till reset is complete.
    }

    // Determine USB core version.
    usb_dev.usb_core_version = READ32(USB_REG_OFF(GSNPSID_REG_OFF)) & USB_CORE_VERSION_MASK;
    CRIT("USB Core Ver: 0x%04x.\n", usb_dev.usb_core_version);

    // Init device info (speed, status, EP0 state).
    usb_dev.dev_speed = USB_SPEED_UNKNOWN;
    usb_dev.dev_state = USB_DEVICE_STATE_NOT_CONNECTED;
    usb_dev.ep0_state = EP0_STATE_IDLE;

    // Register USB stack callbacks.
    if (pSetupPktCallback) pUsbStackSetupPktCallback = pSetupPktCallback;
    if (pDataXferCallback) pUsbStackDataXferCallback = pDataXferCallback;
    if (pPowerMngmtCallback) pUsbStackPowerMngmtCallback = pPowerMngmtCallback;

    for (ep_num = 0; ep_num <= MAX_EP_NUM; ep_num++)
    {
        // Init EP info struct.
        usb_dev.ep_info_OUT[ep_num].bStalled = FALSE;
        usb_dev.ep_info_OUT[ep_num].bXferActive = FALSE;
        usb_dev.ep_info_OUT[ep_num].wrap_window_xfer = FALSE;
        usb_dev.ep_info_IN[ep_num].bStalled = FALSE;
        usb_dev.ep_info_IN[ep_num].bXferActive = FALSE;
        usb_dev.ep_info_IN[ep_num].wrap_window_xfer = FALSE;

        // Initialize EP command registers.
        WRITE32(DEPCMD(ep_num | ENDPT_DIRECTION_OUT), 0);
        WRITE32(DEPCMD(ep_num | ENDPT_DIRECTION_IN), 0);
    }

    // Init event buffer pointers.
    usb_dev.event_ptr[EVNT_BUFF_0] = (UINT32_T*)datapath_ram->event_buffer_0;
    usb_dev.event_buff[EVNT_BUFF_0] = (UINT32_T*)datapath_ram->event_buffer_0;
    usb_dev.event_buff_end[EVNT_BUFF_0] = usb_dev.event_buff[EVNT_BUFF_0] + (EVNT_BUFFER_SIZE / 4);

    // Program the Event Buffer address and size registers and interrupt mask.
    WRITE32(GEVNTADR_HI(EVNT_BUFF_0), 0);
    WRITE32(GEVNTADR_LO(EVNT_BUFF_0), (UINT32_T)datapath_ram->event_buffer_0);
    DEBUG("USB event buffer addr = 0x%08x.\n", (UINT32_T)datapath_ram->event_buffer_0);
    WRITE32(GEVNTSIZ(EVNT_BUFF_0), EVNT_BUFFER_SIZE & GEVNTSIZ0_BUFFER_SIZE_MASK);

    // Write 0 into GEVNTCOUNTn register to enable event buffer.
    WRITE32(GEVNTCOUNT(EVNT_BUFF_0), 0x0);

    // Program device speed, NumP, and IntrNum in DCFG register.
    WRITE32(USB_REG_OFF(DCFG_REG_OFF), ((NUM_P << DCFG_NUM_P_OFFSET) |
                                        (NON_EP_INTR_NUM << DCFG_INTR_NUM_OFFSET) |
                                        DSTS_SPEED_SS_PHY_125MHZ_OR_250MHZ |
                                        DCFG_LPM_CAPABLE_BIT));

#if FORCE_USB20_ONLY
    // Program device speed, NumP, and IntrNum in DCFG register.
    WRITE32(USB_REG_OFF(DCFG_REG_OFF), ((NUM_P << DCFG_NUM_P_OFFSET) |
                                        (NON_EP_INTR_NUM << DCFG_INTR_NUM_OFFSET) |
                                        DSTS_SPEED_HS_PHY_30MHZ_OR_60MHZ));
#endif

    // Set bits for USB reset, link state change, connection done, and USB resume/remote wakeup in the DEVTEN register.
    WRITE32(USB_REG_OFF(DEVTEN_REG_OFF), (DEVTEN_RESUME_WKUP_EVNT | DEVTEN_LINK_STATE_CHANGE |
                                          DEVTEN_CONNECT_DONE | DEVTEN_USB_RESET | DEVTEN_DISCONNECT));

#if DISABLE_SCRAMBLING
    // Disable scrambling.
    MODIFY32(USB_REG_OFF(GCTL_REG_OFF), GCTL_DISABLE_SCRAMBLING, GCTL_DISABLE_SCRAMBLING);
#endif

    // SSC control.
    if (usb_dev.usb_core_version != 0x101a)
    {
        MODIFY32(GPREG1_REG_OFF, GPREG1_USB3_SSC_DISABLE_BIT, (ENABLE_USB_SSC) ? 0 : GPREG1_USB3_SSC_DISABLE_BIT);
        CRIT("USB SSC is %s.\n", (ENABLE_USB_SSC) ? "ON" : "OFF");
    }

#if ENABLE_U1_U2_TRANSITIONS
    // Set U1/U2 Accept enable bits.
    MODIFY32(USB_REG_OFF(DCTL_REG_OFF), (DCTL_U1_ACCEPT_ENABLE | DCTL_U2_ACCEPT_ENABLE), (DCTL_U1_ACCEPT_ENABLE | DCTL_U2_ACCEPT_ENABLE));
#endif

    if (emulation_platform)  /* FPGA */
    {
        // Set FPGA PwrDnScale. ((120 MHz) / 2 / 16kHz = 3,750 = 0xEA6)
        MODIFY32(USB_REG_OFF(GCTL_REG_OFF), 0xFFF80000, (0xEA6 << 19));
    }
    else if (usb_dev.usb_core_version < 0x131a)
    {
        // Workaround for WEBS PG3_0_Silicon.9 (CRM #9000395615)
        // Change PHY power state to P2 before attempting U3 exit handshake.  DwrDnScale is ignored.
        MODIFY32(USB_REG_OFF(GUSB3PIPECTL_REG_OFF), PIPECTL_P3_EXIT_SIGNAL_TO_P2_BIT,  PIPECTL_P3_EXIT_SIGNAL_TO_P2_BIT);
    }

    if (usb_dev.usb_core_version >= 0x110a)
    {
        // Set P0/P1 Rx Valid Low.
        MODIFY32(USB_REG_OFF(GUSB3PIPECTL_REG_OFF), PIPECTL_P0P1_RX_VALID_LOW_BIT,  PIPECTL_P0P1_RX_VALID_LOW_BIT);

#if ENABLE_USB_PHY_SUSPEND
        // Enable USB core clock gating.
        MODIFY32(USB_REG_OFF(GCTL_REG_OFF), GCTL_DISABLE_CLOCK_GATING_BIT, 0);

        // Enable SS PHY suspend mode.
        MODIFY32(USB_REG_OFF(GUSB3PIPECTL_REG_OFF), PIPECTL_SUSPEND_SS_PHY_ENABLE_BIT, PIPECTL_SUSPEND_SS_PHY_ENABLE_BIT);

        // Note: HS PHY suspend mode is controlled in link state change ISR.
#endif

        // Disable ECN so SS enumeration can be retried up to 3x once in SS.Disabled state.
        MODIFY32(USB_REG_OFF(GCTL_REG_OFF), GCTL_U2_RESET_ECN_BIT, GCTL_U2_RESET_ECN_BIT);
    }

    if (usb_dev.usb_core_version > 0x108A)
    {
        // Initialize transfer resource allocation with index 0.
        usb_hal_dep_cmd((EP0 | ENDPT_DIRECTION_OUT), DEPCMD_START_NEW_CONFIG, 0, 0);
    }

    // Configure EP0. (Required here according to databook).
    usb_hal_config_ep0(EP0_TX_FIFO, TRUE);

    // Set flag so EP0 setup transfer will be configured upon USB connect event.
    usb_dev.bConfigEP0SetupXfer = TRUE;

    // Set USB Reset intialization complete flag.
    usb_dev.bUSBResetInitComplete = TRUE;

    // This delay is required in case we are behind a hub and the system is reset.
    // The hub needs ~32ms to detect a device has been disconnected.
    msleep(50);

    return;
}



