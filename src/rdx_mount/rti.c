/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rti.c
//
// Project     : TUSB9260 Firmware.
//
// Description : Real Time Interrupt module driver.
//
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   05/12/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 *
 * This file contains the implementation of the Real-Time Interrupt module driver.
 *
 * There are two independent counters. Counter 0 is configured for microsecond resolution
 * to be used for a usleep() function. Counter 1 is configured for millisecond resolution
 * and is used to generate periodic interrupts and a msleep() function.
 *
 */

#include "rti.h"
#include "rdx_hardware.h"
#include "rdx_manager_protocol.h"
#include "ahci.h"
#include "gio.h"
#include "reg_io.h"
#include "sci.h"
#include "system.h"  // system_reset()
#include "tusb9260_types.h"
#include "usb_hal.h"
#include "vim_nvic.h"
#include "wdt.h"

volatile UINT32_T gCounter0_overflow_cnt = 0;
volatile UINT32_T gCounter1_overflow_cnt = 0;

extern BOOLEAN_T wfi_enable;

/*****************************************************************************
 * Function: usleep
 *************************************************************************//**
 * This function uses counter 0 to wait for the specified number
 * of microseconds before returning.  The watchdog timer may expire if
 * the sleep time is excessive.  See WDT_TIMEOUT_MS in wdt.h.
 *
 * @param[in] usec number of microseconds to sleep.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void usleep(UINT32_T usec)
{
    UINT32_T overflow_cnt = gCounter0_overflow_cnt;
    UINT32_T end_time = READ_REG32(RTIFRC0_REG_OFF) + usec;

    // Check for end time rollover.
    if (end_time < usec)
    {
        // Wait for counter rollover.
        while (READ_REG32(RTIFRC0_REG_OFF) > end_time)
        {
            // Busy wait.  Break out of while loop if counter overflows.
            if (gCounter0_overflow_cnt > overflow_cnt)
                break;
        }
    }

    while (READ_REG32(RTIFRC0_REG_OFF) < end_time)
    {
        // Busy wait.
    }

    return;
}


/*****************************************************************************
 * Function: msleep
 *************************************************************************//**
 * This function uses counter 1 to wait for the specified number
 * of milliseconds before returning.  The watchdog timer may expire if
 * the sleep time is excessive. See WDT_TIMEOUT_MS in wdt.h.
 *
 * @param[in] msec number of milliseconds to sleep.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void msleep(UINT32_T msec)
{
    UINT32_T overflow_cnt = gCounter1_overflow_cnt;
    UINT32_T end_time = READ_REG32(RTIFRC1_REG_OFF) + msec;

    // Check for end time rollover.
    if (end_time < msec)
    {
        // Wait for counter rollover.
        while (READ_REG32(RTIFRC1_REG_OFF) > end_time)
        {
            // Busy wait.  Break out of while loop if counter overflows.
            if (gCounter1_overflow_cnt > overflow_cnt)
                break;
        }
    }

    while (READ_REG32(RTIFRC1_REG_OFF) < end_time)
    {
        // Busy wait.
    }

    return;
}

#define COMPSEL_FRC1(compare_num)   (0x0001 << (4 * (compare_num)))
#define COMPINT(compare_num)        (1 << (compare_num))

/*****************************************************************************
 * Function: rti_config_compare_interrupt
 *************************************************************************//**
 * This function configures a compare interrupt with the specified period.
 *
 * @param [in] compare_num compare interrupt number.
 * @param [in] period interrupt period in milliseconds.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_config_compare_interrupt(UINT32_T compare_num, UINT32_T period)
{
    UINT32_T comp_int = COMPINT(compare_num);

    // Disable compare interrupt.
    WRITE_REG32(RTICLRINT_REG_OFF, comp_int);

    /* Set Compare */
    WRITE_REG32(RTI_COMP_REG_OFF(compare_num), READ_REG32(RTI_COMP_REG_OFF(compare_num)) + period);

    /* Set Update Compare */
    WRITE_REG32(RTI_UDCP_REG_OFF(compare_num), period);

    // Set Compare select to FRC 1.
    MODIFY_REG32(RTICOMPCTRL_REG_OFF, COMPSEL_FRC1(compare_num), COMPSEL_FRC1(compare_num));

    // Enable compare 0 interrupt.
    MODIFY_REG32(RTISETINT_REG_OFF, comp_int, comp_int);
}


/*****************************************************************************
 * Function: rti_start_periodic_interrupts
 *************************************************************************//**
 * This function configures counter 1 for millisecond resolution and
 * programs counter compare registers to provide periodic interrupts.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_start_periodic_interrupts(void)
{
    /* Reset up counter 1 */
    WRITE_REG32(RTIUC1_REG_OFF , 0x00000000);

    /* Reset free-running counter 1 */
    WRITE_REG32(RTIFRC1_REG_OFF, 0x00000000);

    /* Set Compare Up Counter 1 to 1 millisecond resolution */
    WRITE_REG32(RTICPUC1_REG_OFF, RTI1_PRESCALE);

#if RTI_COMP0_INTERRUPT_ENABLE

    // Configure compare 0 interrupt.
    rti_config_compare_interrupt(0, RTI_COMP0_INTERRUPT_PERIOD);

#endif

#if RTI_COMP1_INTERRUPT_ENABLE

    // Configure compare 1 interrupt.
    rti_config_compare_interrupt(1, RTI_COMP1_INTERRUPT_PERIOD);

#endif

#if RTI_COMP2_INTERRUPT_ENABLE

    // Configure compare 2 interrupt.
    rti_config_compare_interrupt(2, RTI_COMP2_INTERRUPT_PERIOD);

#endif

    // Enable counter 1 overflow interrupt.
    MODIFY_REG32(RTISETINT_REG_OFF, RTI_FRC1_OVERFLOW_INT_BIT, RTI_FRC1_OVERFLOW_INT_BIT);

    /* Start counter 1 */
    MODIFY_REG32(RTIGCTRL_REG_OFF, RTIGCTRL_CNT1_ENABLE_BIT, RTIGCTRL_CNT1_ENABLE_BIT);

    return;
}


/*****************************************************************************
 * Function: rti_init
 *************************************************************************//**
 * This function initializes the RTI module counters and starts periodic
 * interrupt generation.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_init(void)
{
    /* Disable both counters */
    WRITE_REG32(RTIGCTRL_REG_OFF, 0);

    // Disable all compare interrupts.
    WRITE_REG32(RTISETINT_REG_OFF, 0);

    /* Reset Up Counter 0 */
    WRITE_REG32(RTIUC0_REG_OFF , 0x00000000);

    /* Reset Free-running counter 0 */
    WRITE_REG32(RTIFRC0_REG_OFF, 0x00000000);

    /* Set Compare Up Counter 0 to 1 usec */
    WRITE_REG32(RTICPUC0_REG_OFF , RTI0_PRESCALE);

    // Enable counter 0 overflow interrupt.
    MODIFY_REG32(RTISETINT_REG_OFF, RTI_FRC0_OVERFLOW_INT_BIT, RTI_FRC0_OVERFLOW_INT_BIT);

    /* Start Counter 0 */
    MODIFY_REG32(RTIGCTRL_REG_OFF, RTIGCTRL_CNT0_ENABLE_BIT, RTIGCTRL_CNT0_ENABLE_BIT);

    // Start periodic interrupts.
    rti_start_periodic_interrupts();

    return;
}

/*****************************************************************************
 * Function: rti_overflow0_isr
 *************************************************************************//**
 * Interrupt service routine for handling counter 0 overflow.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_overflow0_isr(void)
{
    // Increment overflow 0 count.
    gCounter0_overflow_cnt++;

    DEBUG("-> rti_overflow0_isr() - cnt = %u\n", gCounter0_overflow_cnt);

    // Clear overlow 0 interrupt.
    WRITE_REG32(RTIINTFLAG_REG_OFF, RTI_FRC0_OVERFLOW_INT_BIT);

    return;
}

/*****************************************************************************
 * Function: rti_overflow1_isr
 *************************************************************************//**
 * Interrupt service routine for handling counter 1 overflow.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_overflow1_isr(void)
{
    // Increment overflow 1 count.
    gCounter1_overflow_cnt++;

    DEBUG("-> rti_overflow1_isr() - cnt = %u\n", gCounter1_overflow_cnt);

    // Clear overlow 1 interrupt.
    WRITE_REG32(RTIINTFLAG_REG_OFF, RTI_FRC1_OVERFLOW_INT_BIT);

    return;
}

/*****************************************************************************
 * Function: rti_compare2_isr
 *************************************************************************//**
 * Interrupt service routine for handling the RTI compare 2 interrupt.
 * It performs periodic USB housekeeping and advances RDX LED timing.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_compare2_isr(void)
{
    UINT32_T link_state = usb_hal_get_usb_link_state();

    /* Advance Manager control/protocol timers from the periodic 100-ms
     * service. The LED controller derives its normal 500-ms phase cadence
     * internally and temporarily uses every tick for fast confirmation. */
    rdx_manager_control_tick();
    rdx_manager_protocol_tick();

    if (gDiskActivity)
    {
        /* PWM0 drives the fan, not an activity indicator. Forward storage
         * activity to the foreground cartridge-LED coordinator instead. */
        rdx_hardware_note_activity();
        gDiskActivity = FALSE;
    }
    else
    {
        // Set flag so MCU will be put into WFI mode in main().
        wfi_enable = TRUE;
    }

    if ((link_state == SS_DISABLED_STATE) || (link_state == 0x0E))  // Link state sometimes reports this odd 0x0E value when calling usb_hal_disconnect().
    {
#if ENABLE_SATA_STANDBY_POWER_MODE
        /* In case USB cable is disconnected while a USB transfer using datapath RAM wrap window is
         * busy waiting for the HDD to spin-up or become ready, we need to cancel USB wrap window
         * transfers to free USB core so interrupts can occur */
        if (!mww_check_usb_interface_ready())
        {
            if (usb_dev.ep0_state == EP0_STATE_IDLE)
            {
                usb_hal_cancel_io_request((EP3 | ENDPT_DIRECTION_OUT));
                usb_hal_cancel_io_request((EP3 | ENDPT_DIRECTION_IN));
            }
        }
#endif
    }
    else if (link_state != SUSPEND_STATE)
    {

#if DISABLE_U1_U2_INITIATE_WHILE_XFER_ACTIVE & ENABLE_U1_U2_TRANSITIONS

        if (usb_dev.usb_core_version < 0x183a)
        {
            // Workaround for WEBS PG3_0_Silicon.21 (CRM #9000446952)
            // Re-enable device-initiated U1 if no SATA commands are queued or in-progress.
            if (!(READ32(PxCI(0)) | READ32(PxSACT(0))))
            {
                if (usb_dev.bIsU1Enabled)
                {
                    usb_hal_set_U1_initiate_enable(TRUE);
                }

                if (usb_dev.bIsU2Enabled)
                {
                    usb_hal_set_U2_initiate_enable(TRUE);
                }
            }
        }
#endif
    }
    // Clear compare 2 interrupt.
    WRITE_REG32(RTIINTFLAG_REG_OFF, RTI_COMPARE2_INT_FLAG);

    return;
}


/*****************************************************************************
 * Function: rti_compare1_isr
 *************************************************************************//**
 * Interrupt service routine for handling the RTI compare 1 interrupt.
 * It performs vendor specific periodic tasks. RTI_COMP1_INTERRUPT_ENABLE
 * macro must be set 1 to enable the compare 1 interrupt and the period
 * is controlled using the RTI_COMP1_INTERRUPT_PERIOD macro.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_compare1_isr(void)
{
#ifdef SATA_COMPLIANCE_MODE_DEBUG  // BQ - This is for SATA compliance mode debugging only.
    static UINT32_T bistafr;

    if (READ32(AHCI_REG_OFF(BISTAFR_REG_OFF)) != bistafr)
    {
        bistafr = READ32(AHCI_REG_OFF(BISTAFR_REG_OFF));
        CRIT("BISTAFR = 0x%08x\n", READ32(AHCI_REG_OFF(BISTAFR_REG_OFF)));
    }
#endif

#if ENABLE_SATA_STANDBY_POWER_MODE

    UINT32_T link_state = usb_hal_get_usb_link_state();
    UINT32_T port_num;

    if (link_state == SS_DISABLED_STATE)
    {
        if (usb_dev.dDisconnectCountDown-- == 0)
        {
            usb_dev.dDisconnectCountDown = -1UL;  /* Set max value so standby command is not sent again for a long time */

            // Disable USB interrupt so commands cannot be issued to the drive.
            WRITE_REG32(VIM_REQMASKCLR0, 0x00200000);

            for (port_num = 0; port_num < gSATADeviceCount; port_num++)
            {
                // Put the drive in Standby mode.
                ahci_standby_immediate(port_num);
            }

            // Re-enable USB interrupt.
            WRITE_REG32(VIM_REQMASKSET0, 0x00200000);
        }
    }
    else if (link_state == SUSPEND_STATE)
    {
        if (usb_dev.dSuspendCountDown-- == 0)
        {
            usb_dev.dSuspendCountDown = -1UL;  /* Set max value so standby command is not sent again for a long time */

            // Disable USB interrupt so commands cannot be issued to the drive.
            WRITE_REG32(VIM_REQMASKCLR0, 0x00200000);

            for (port_num = 0; port_num < gSATADeviceCount; port_num++)
            {
                // Put the drive in Standby mode.
                ahci_standby_immediate(port_num);
            }

            // Re-enable USB interrupt.
            WRITE_REG32(VIM_REQMASKSET0, 0x00200000);
        }
    }
#endif
    /***********************************************************/
    /********** Insert vendor specific tasks here **************/
    /***********************************************************/


    // Clear compare 1 interrupt.
    WRITE_REG32(RTIINTFLAG_REG_OFF, RTI_COMPARE1_INT_FLAG);

    return;
}


#define TX_BITS 0x50

/*****************************************************************************
 * Function: rti_compare0_isr
 *************************************************************************//**
 * Interrupt service routine for handling the RTI compare 0 interrupt.
 * It performs periodic tasks such as blinking the heartbeat LED.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void rti_compare0_isr(void)
{
#if TI_PRELIM_ASIC_COMPATIBILITY

    UINT32_T ltssm;

    // Check for pre-PG3.0 ASIC.
    if (usb_dev.usb_core_version == 0x101a)
    {
        // Check LTSSM register for bad states which can occur when entering U3 link state.
        ltssm = READ_REG32(USB_REG_OFF(GDBGLTSSM_REG_OFF));

        if ((ltssm == 0x01081440) || (ltssm == 0x00c81442))
        {
            // USB is hosed.  Reset system so USB will resume properly.
            system_reset();
        }

        // Workaround for ACK-RETRY issue in USB 3.0 CV MSC Error Recovery Test.
        if (usb_dev.ep_info_IN[BOT_BULK_IN_ENDPT_NUM].bStalled)
        {
            // Set EP number in debug register.
            WRITE32(USB_REG_OFF(0x170), LEP2PEP(BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN));

            // Read upper DWORD of endpt debug register and see if ERDY mask bit is set.
            if (READ32(USB_REG_OFF(0x17c)) & 0x01000000)
            {
                // Wait for any previous command to complete.
                while (READ32(USB_REG_OFF(DGCMD_REG_OFF)) & DGCMD_CMD_ACT_BIT);

                // Clear the ERDY mask to force the core to send ERDY.
                WRITE32(USB_REG_OFF(DGCMDPAR_REG_OFF), LEP2PEP(BOT_BULK_IN_ENDPT_NUM | ENDPT_DIRECTION_IN));
                WRITE32(USB_REG_OFF(DGCMD_REG_OFF), (DGCMD_CMD_ACT_BIT | DGCMD_TYPE_SET_EP_NRDY));
                CRIT("Clearing ERDY mask on Stalled EP.\n");
            }
        }
    } /* END: usb_dev.usb_core_version == 0x101a */
#endif

    // Clear compare 0 interrupt.
    WRITE_REG32(RTIINTFLAG_REG_OFF, RTI_COMPARE0_INT_FLAG);

    return;
}

