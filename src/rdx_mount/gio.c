/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : gio.c
//             
// Project     : TUSB926x Firmware.
//             
// Description : General Input/Output module driver.
// 
//   (C) Copyright 2009 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   07/29/09 - Brian Quach - Creation.
//
//=======================================================================================

/*! @file
 * 
 * This file contains the implementation of the General Input/Output (GIO) module driver. 
 * GIO[7:0] are controlled by the GIO module directly, GPIO[9:8] are controlled through
 * SCI registers, and GPIO[11:10] are controlled through SPI module registers. OpenRDX
 * permanently assigns SCI GPIO8/9 to the dock LED after preloading both active-low
 * channels off; GPIO helpers therefore preserve support for all three register banks.
 *  
 */

#include "gio.h"
#include "reg_io.h"
#include "sci.h"
#include "spi.h"
#include "tusb9260.h"
#include "tusb9260_types.h"

#define GIOGCR_RESET_BIT        0x1U
#define RDX_STD_HIGH_PRELOAD_MASK \
                                ((1UL << RDX_MOTOR_CONTROL_GIO_NUM) | \
                                 (1UL << RDX_CARTRIDGE_AMBER_GIO_NUM) | \
                                 (1UL << RDX_CARTRIDGE_GREEN_GIO_NUM))
#define RDX_STD_OUTPUT_MASK     RDX_STD_HIGH_PRELOAD_MASK
#define RDX_STD_INPUT_MASK      ((1UL << RDX_EJECT_BUTTON_GIO_NUM) | \
                                 (1UL << RDX_MECHANISM_INPUT_GIO_NUM) | \
                                 (1UL << RDX_CARTRIDGE_PRESENT_GIO_NUM))
#define RDX_SCI_LED_MASK        (SCI_PIO_RX_GPIO8 | SCI_PIO_TX_GPIO9)
#define RDX_MECHANISM_AUXILIARY_MASK SPI_PC_SCS2_GPIO11

/* Logical output 11 must remain unavailable until profile outputs are ready. */
static BOOLEAN_T rdx_profile_mechanism_auxiliary_available = FALSE;

/*****************************************************************************
 * Function: gio_high
 *************************************************************************//**
 * This function drives an output GIO logic high.
 *
 * @param[in] gio_num number of the GIO.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_high(UINT32_T gio_num)
{
    if (gio_num < NUM_STD_GIOS)
    {
        MODIFY_REG32(GIOOUT0_REG_OFF, (1 << gio_num), (1 << gio_num)); 
    }
    else if ((gio_num == 8) || (gio_num == 9))  // SCI module GIO.
    {
        // Set Data-Out bit in SCIPIO3.
        MODIFY_REG32(SCI_PIO3, (1 << (gio_num - 7)), (1 << (gio_num - 7)));        
    }
    else if ((gio_num == 10) || (gio_num == 11))  // SPI module GIO.
    {
        // Set bit in SPIDSET register.
        MODIFY_REG32(SPI_PC4, (1 << (gio_num - 9)), (1 << (gio_num - 9)));
    }

    return;
}


/*****************************************************************************
 * Function: gio_low
 *************************************************************************//**
 * This function drives an output GIO logic low.
 *
 * @param[in] gio_num number of the GIO.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_low(UINT32_T gio_num)
{
    if (gio_num < NUM_STD_GIOS)
    {
        MODIFY_REG32(GIOOUT0_REG_OFF, (1 << gio_num), 0); 
    }
    else if ((gio_num == 8) || (gio_num == 9))  // SCI module GIO.
    {
        // Clear Data-Out bit in SCIPIO3.
        MODIFY_REG32(SCI_PIO3, (1 << (gio_num - 7)), 0);        
    }
    else if ((gio_num == 10) || (gio_num == 11))  // SPI module GIO.
    {
        // Set bit in SPIDCLR register.
        MODIFY_REG32(SPI_PC5, (1 << (gio_num - 9)), (1 << (gio_num - 9)));
    }

    return;
}


/*****************************************************************************
 * Function: gio_toggle
 *************************************************************************//**
 * This function flips the logic state of an output GIO high if currently low 
 * or low if currently high. 
 *
 * @param[in] gio_num number of the GIO.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_toggle(UINT32_T gio_num)
{
    if (gio_num < NUM_STD_GIOS)
    {
        WRITE_REG32(GIOOUT0_REG_OFF, (READ_REG32(GIOOUT0_REG_OFF) ^ (1 << gio_num))); 
    }
    else if ((gio_num == 8) || (gio_num == 9))  // SCI module GIO.
    {
        // Toggle Data-Out bit in SCIPIO3.
        WRITE_REG32(SCI_PIO3, (READ_REG32(SCI_PIO3) ^ (1 << (gio_num - 7))));        
    }
    else if ((gio_num == 10) || (gio_num == 11))  // SPI module GIO.
    {
        if (READ32(SPI_PC3) & (1 << (gio_num - 9)))
        {
            // Set bit in SPIDCLR register.
            MODIFY_REG32(SPI_PC5, (1 << (gio_num - 9)), (1 << (gio_num - 9)));            
        }
        else
        {
            // Set bit in SPIDSET register.
            MODIFY_REG32(SPI_PC4, (1 << (gio_num - 9)), (1 << (gio_num - 9)));           
        }
    }

    return;
}

/*****************************************************************************
 * Function: gio_set_direction
 *************************************************************************//**
 * This function configures the direction of a GIO
 *
 * @param[in] gio_num number of the GIO.
 * @param[in] dir direction.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_set_direction(UINT32_T gio_num, GIO_DIR_T dir)
{
    if (gio_num < NUM_STD_GIOS)
    {   
        // Set GIO direction.
        MODIFY32(GIODIR0_REG_OFF, (1 << gio_num), (dir << gio_num));
    }
    else if ((gio_num == 8) || (gio_num == 9))  // SCI module GIO.
    {
        // Set Data-Dir in SCIPIO1.
        MODIFY_REG32(SCI_PIO1, (1 << (gio_num - 7)), (dir << (gio_num - 7)));               
    }
    else if ((gio_num == 10) || (gio_num == 11))  // SPI module GIO.
    {
        // Set SPIDIR.
        MODIFY_REG32(SPI_PC1, (1 << (gio_num - 9)), (dir << (gio_num - 9)));
    }

    return;
}

/*****************************************************************************
 * Function: gio_input_setup
 *************************************************************************//**
 * This function configures a GIO for use as an input and enables the
 * interrupt for that GIO if enable_interrupt flag is TRUE.  Interrupts
 * are currently supported for GIO[0-7] only.  SCI and SPI module pins 
 * must be preconfigured for GIO functionality.
 *
 * @param[in] gio_num number of the GIO.
 * @param[in] enable_interrupt flag indicating whether enable a rising edge interrupt for the GIO.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_input_setup(UINT32_T gio_num, BOOLEAN_T enable_interrupt)
{
    if (gio_num < NUM_STD_GIOS)
    {   
        // Enable internal pull-down.
        MODIFY32(GIOPULDIS0_REG_OFF, (1 << gio_num), 0);

        // Set GIO direction to input.
        MODIFY32(GIODIR0_REG_OFF, (1 << gio_num), 0);
    
        // Configure interrupt flag to be set on rising edge for input GIO.
        MODIFY32(GIOPOL_REG_OFF, (1 << gio_num), (1 << gio_num));
    
        // Configure the input GIO as high priority so it is reported on interrupt line 11 (GIOA).
        WRITE32(GIOLVLSET_REG_OFF, (1 << gio_num));
    
        // Clear interrupt flags for GIO.
        WRITE32(GIOFLG_REG_OFF, (1 << gio_num));
    
        if (enable_interrupt)
        {    
            // Enable interrupt for input GIO.
            WRITE32(GIOENASET_REG_OFF, (1 << gio_num));
        }
    }
    else if ((gio_num == 8) || (gio_num == 9))  // SCI module GIO.
    {
        // Clear Pull Control Disable bit in SCIPIO7. (Enables internal pull-up).
        MODIFY_REG32(SCI_PIO7, (1 << (gio_num - 7)), 0);               

        // Clear Data-Dir bit in SCIPIO1.
        MODIFY_REG32(SCI_PIO1, (1 << (gio_num - 7)), 0);               
    }
    else if ((gio_num == 10) || (gio_num == 11))  // SPI module GIO.
    {
        // Enable internal pull-up.
        MODIFY_REG32(SPI_PC7, (1 << (gio_num - 9)), 0);

        // Clear bit in SPIDIR register.
        MODIFY_REG32(SPI_PC1, (1 << (gio_num - 9)), 0);
    }

    return;
}



/*****************************************************************************
 * Function: gio_set_as_input
 *************************************************************************//**
 * Configure a GIO as a polled input without enabling its interrupt.
 *
 * @param[in] gio_num number of the GIO.
 *
 * @retval None.
 *
 *****************************************************************************
 */

void gio_set_as_input(UINT32_T gio_num)
{
    gio_input_setup(gio_num, FALSE);
}

/*****************************************************************************
 * Function: gio_get_direction
 *************************************************************************//**
 * This function returns a bitmask for the data direction for all GIOs.  
 * 1 if port is configured as an input.  0 if port is configured as an output.
 *
 * @param None.
 *
 * @return The direction bitmask.
 *
 ****************************************************************************** 
 */

UINT32_T gio_get_direction(void)
{
    UINT32_T sci_gio = (~READ_REG32(SCI_PIO1) & (SCI_PIO_TX_GPIO9 | SCI_PIO_RX_GPIO8)) << 7;
    UINT32_T spi_gio = (~READ_REG32(SPI_PC1) & (SPI_PC_SCS1_GPIO10 | SPI_PC_SCS2_GPIO11)) << 9;

    return ((~READ32(GIODIR0_REG_OFF) & 0xFF) | sci_gio | spi_gio);
}

/*****************************************************************************
 * Function: gio_get_function
 *************************************************************************//**
 * This function returns a bitmask for the current pin function.  
 * 1 if port is configured as an GPIO.  0 if port is configured for alternate 
 * functionality. Ports 0-7 are always GPIOs.
 *
 * @param None.
 *                    
 * @return The function bitmask.
 *
 ****************************************************************************** 
 */

UINT32_T gio_get_function(void)
{
    UINT32_T sci_gio = (~READ_REG32(SCI_PIO0) & (SCI_PIO_TX_GPIO9 | SCI_PIO_RX_GPIO8)) << 7;
    UINT32_T spi_gio = (~READ_REG32(SPI_PC0) & (SPI_PC_SCS1_GPIO10 | SPI_PC_SCS2_GPIO11)) << 9;

    return (sci_gio | spi_gio | 0xFF);
}

/*****************************************************************************
 * Function: gio_get_pull_ctrl
 *************************************************************************//**
 * This function returns a bitmask for the current pin pull control.  
 * 1 if port is configured with Pull-Up.  0 if port is configured for Pull-Down.
 *
 * @param None.
 *                    
 * @return The function bitmask.
 *
 ****************************************************************************** 
 */

UINT32_T gio_get_pull_ctrl(void)
{
    UINT32_T sci_pull_ctrl = (READ_REG32(SCI_PIO8) & (SCI_PIO_TX_GPIO9 | SCI_PIO_RX_GPIO8)) << 7;
    UINT32_T spi_pull_ctrl = (READ_REG32(SPI_PC8) & (SPI_PC_SCS1_GPIO10 | SPI_PC_SCS2_GPIO11)) << 9;

    return (sci_pull_ctrl | spi_pull_ctrl | READ_REG32(GIOPSL0_REG_OFF));
}

/*****************************************************************************
 * Function: gio_get_pull_disable
 *************************************************************************//**
 * This function returns a bitmask for the current pin pull disable.  
 * 1 if port pull control is disabled.  1 if port pull control is enabled.
 *
 * @param None.
 *                    
 * @return The function bitmask.
 *
 ****************************************************************************** 
 */

UINT32_T gio_get_pull_disable(void)
{
    UINT32_T sci_pull_dis = (READ_REG32(SCI_PIO7) & (SCI_PIO_TX_GPIO9 | SCI_PIO_RX_GPIO8)) << 7;
    UINT32_T spi_pull_dis = (READ_REG32(SPI_PC7) & (SPI_PC_SCS1_GPIO10 | SPI_PC_SCS2_GPIO11)) << 9;

    return (sci_pull_dis | spi_pull_dis | READ_REG32(GIOPULDIS0_REG_OFF));
}

/*****************************************************************************
 * Function: gio_get_state
 *************************************************************************//**
 * This function returns the current state of a GIO.
 *
 * @param[in] gio_num number of the GIO.
 *                    
 * @retval 0 if current state is low.
 * @retval 1 if current state is high.
 *
 ****************************************************************************** 
 */

UINT32_T gio_get_state(UINT32_T gio_num)
{
    UINT32_T gio_val = 0;

    if (gio_num < 8)
    {
        gio_val = READ32(GIOIN0_REG_OFF) & (1 << gio_num);
    }
    else if ((gio_num == 8) || (gio_num == 9))  // SCI module GIO.
    {
        gio_val = READ32(SCI_PIO2) & (1 << (gio_num - 7));
    }
    else if ((gio_num == 10) || (gio_num == 11))  // SPI module GIO.
    {
        // Clear bit in SPIDIR register.
        if (READ32(SPI_PC1) & (1 << (gio_num - 9)))
        {
            // Pin is configured as output.
            gio_val = READ32(SPI_PC3) & (1 << (gio_num - 9));
        }
        else
        {
            // Pin is configured as input.
            gio_val = READ32(SPI_PC2) & (1 << (gio_num - 9));
        }
    }

    return (gio_val) ? 1 : 0;
}

/*****************************************************************************
 * Function: gio_rdx_eject_button_pressed
 *************************************************************************//**
 * Return the state of the active-low RDX eject button on GPIO1.
 *
 * @param None.
 *
 * @retval TRUE when the button is pressed.
 * @retval FALSE when the button is released.
 *
 *****************************************************************************
 */

BOOLEAN_T gio_rdx_eject_button_pressed(void)
{
    return (gio_get_state(RDX_EJECT_BUTTON_GIO_NUM) == 0U) ? TRUE : FALSE;
}

/*****************************************************************************
 * Function: gio_rdx_mechanism_input_asserted
 *************************************************************************//**
 * Return the state of the active-low RDX mechanism input on GPIO2.
 *
 * @param None.
 *
 * @retval TRUE when the mechanism input is asserted.
 * @retval FALSE when the mechanism input is deasserted.
 *
 *****************************************************************************
 */

BOOLEAN_T gio_rdx_mechanism_input_asserted(void)
{
    return (gio_get_state(RDX_MECHANISM_INPUT_GIO_NUM) == 0U) ? TRUE : FALSE;
}

/*****************************************************************************
 * Function: gio_rdx_cartridge_present
 *************************************************************************//**
 * Return whether the active-low cartridge-present input on GPIO5 is set.
 *
 * @param None.
 *
 * @retval TRUE when a cartridge is present.
 * @retval FALSE when no cartridge is present.
 *
 *****************************************************************************
 */

BOOLEAN_T gio_rdx_cartridge_present(void)
{
    return (gio_get_state(RDX_CARTRIDGE_PRESENT_GIO_NUM) == 0U) ? TRUE : FALSE;
}

/*****************************************************************************
 * Function: gio_rdx_motor_control_set_high
 *************************************************************************//**
 * Drive the generic GPIO3 mechanism-control output to an explicit level.
 *
 * Logical output 10 maps to selector 3 with active-high polarity. States 1,
 * 3, and 6 write logical zero before starting their selected PWM output.
 * Calling it a direction, enable, or gate signal would assert a board-level
 * electrical function that this interface does not require.
 *
 * @param[in] high TRUE to drive GPIO3 high, FALSE to drive it low.
 *
 * @retval None.
 *
 ****************************************************************************
 */
void gio_rdx_motor_control_set_high(BOOLEAN_T high)
{
    if (high)
    {
        gio_high(RDX_MOTOR_CONTROL_GIO_NUM);
    }
    else
    {
        gio_low(RDX_MOTOR_CONTROL_GIO_NUM);
    }
}

/*****************************************************************************
 * Function: gio_rdx_motor_gate_enable
 *************************************************************************//**
 * Drive GPIO3 through an active-low enable-style convenience wrapper.
 *
 * State-driven code uses gio_rdx_motor_control_set_high() directly: states
 * 1/3/6 select GPIO3 low, and stopped states restore it high.
 * This wrapper is suitable only when a caller explicitly needs low assertion.
 *
 * @param[in] enable TRUE to drive GPIO3 low, FALSE to drive GPIO3 high.
 *
 * @retval None.
 *
 ****************************************************************************
 */
void gio_rdx_motor_gate_enable(BOOLEAN_T enable)
{
    gio_rdx_motor_control_set_high(enable ? FALSE : TRUE);
}

/*****************************************************************************
 * Function: gio_rdx_mechanism_auxiliary_set
 *************************************************************************//**
 * Set the active-high SPI SCS2/GPIO11 mechanism auxiliary output.
 *
 * Logical output 5 maps to selector 11 as an active-high output.  Its fixed
 * mapping survives all supported hardware-profile selections, and mechanism
 * state 1 drives it low.  This establishes pin, direction, polarity, and
 * mechanism ownership, but not a narrower board-level purpose.
 *
 * @param[in] asserted TRUE for logical one/physical high, FALSE for logical
 *                     zero/physical low.
 *
 * @retval None.
 *
 ****************************************************************************
 */
void gio_rdx_mechanism_auxiliary_set(BOOLEAN_T asserted)
{
    if (asserted)
    {
        gio_high(RDX_MECHANISM_AUXILIARY_GIO_NUM);
    }
    else
    {
        gio_low(RDX_MECHANISM_AUXILIARY_GIO_NUM);
    }
}

/*****************************************************************************
 * Function: gio_rdx_profile_outputs_init
 *************************************************************************//**
 * Apply the effective profile's active-high output mapping for GPIO0.
 *
 * The vendor v2.83 mapping routine at 08008F40 applies tables at 0800E398
 * and 0800E3D0: selector zero is GPIO0, while 17h is unassigned. Logical
 * output 11 selects GPIO0 except on profile 38h, where output 14 owns it.
 * The latter starts high (0800BB3C) and stays high while USB is connected
 * or the mechanism is active (0800921C). Its separate idle deassertion
 * policy also depends on USB, ATA, ADC, and startup state; that policy is
 * not implemented here, so this implementation retains the high level.
 *
 * Identity is initialized after gio_init(). Defer this output's direction
 * until its correct level can be preloaded without a transient low pulse.
 *
 * @param[in] hardware_profile Effective hardware profile from identity data.
 *
 * @retval None.
 *
 ****************************************************************************
 */
void gio_rdx_profile_outputs_init(UINT16_T hardware_profile)
{
    rdx_profile_mechanism_auxiliary_available = FALSE;
    if (hardware_profile == 0x38U)
    {
        gio_high(RDX_PROFILE_AUXILIARY_GIO_NUM);
    }
    else
    {
        gio_low(RDX_PROFILE_AUXILIARY_GIO_NUM);
    }
    gio_set_direction(RDX_PROFILE_AUXILIARY_GIO_NUM, GIO_DIR_OUTPUT);
    rdx_profile_mechanism_auxiliary_available =
        (hardware_profile != 0x38U) ? TRUE : FALSE;
}

/*****************************************************************************
 * Function: gio_rdx_profile_mechanism_auxiliary_set
 *************************************************************************//**
 * Set logical output 11 without driving another profile's GPIO0 mapping.
 *
 * The vendor stop routine writes logical zero for every mapped profile.
 * Recovery state 6 asserts logical one for profiles 35h and 37h only. The
 * mechanism selects when to assert it; this helper enforces pin ownership.
 *
 * @param[in] asserted TRUE for logical one/physical high, FALSE for logical
 *                     zero/physical low.
 *
 * @retval None.
 *
 ****************************************************************************
 */
void gio_rdx_profile_mechanism_auxiliary_set(BOOLEAN_T asserted)
{
    if (!rdx_profile_mechanism_auxiliary_available)
    {
        return;
    }
    if (asserted)
    {
        gio_high(RDX_PROFILE_AUXILIARY_GIO_NUM);
    }
    else
    {
        gio_low(RDX_PROFILE_AUXILIARY_GIO_NUM);
    }
}

/*****************************************************************************
 * Function: gio_init
 *************************************************************************//**
 * This function initializes the general-purpose I/O module.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */

void gio_init(void)
{
    /* Profile identity becomes available later in rdx_hardware_init(). */
    rdx_profile_mechanism_auxiliary_available = FALSE;

    // Remove GIO from reset.
    WRITE32(GIOGCR0_REG_OFF, GIOGCR_RESET_BIT);

    /* The RDX eject path is polled; no standard GPIO interrupt is enabled. */
    WRITE32(GIOENACLR_REG_OFF, 0xFFU);

    // Clear all interrupt flags for GIOA and GIOB.
    WRITE32(GIOFLG_REG_OFF, 0xFFFF);

    /*
     * Preload GPIO3 high before enabling output direction.  The stopped
     * pattern uses that level, while powered states drive it low; the pin is
     * therefore described only as a mechanism control, not an enable/gate.
     * The same preload keeps both active-low cartridge LEDs dark.
     */
    MODIFY32(GIOOUT0_REG_OFF, RDX_STD_OUTPUT_MASK,
             RDX_STD_HIGH_PRELOAD_MASK);

    /* GPIO1, GPIO2, and GPIO5 are external, active-low polled inputs. */
    WRITE32(GIOPULDIS0_REG_OFF, 0xFFU);
    MODIFY32(GIOPOL_REG_OFF, RDX_STD_INPUT_MASK, 0U);
    WRITE32(GIODIR0_REG_OFF, RDX_STD_OUTPUT_MASK);

    /*
     * GPIO8/9 share the SCI peripheral. Preload both active-low dock/eject
     * LEDs high before selecting GPIO output mode to avoid a visible flash.
     */
    MODIFY_REG32(SCI_PIO3, RDX_SCI_LED_MASK, RDX_SCI_LED_MASK);
    MODIFY_REG32(SCI_PIO7, RDX_SCI_LED_MASK, RDX_SCI_LED_MASK);
    MODIFY_REG32(SCI_PIO1, RDX_SCI_LED_MASK, RDX_SCI_LED_MASK);
    MODIFY_REG32(SCI_PIO0, RDX_SCI_LED_MASK, 0U);

    /*
     * Select GPIO mode and output direction for logical output 5 after its
     * fixed mapping is available.  The
     * controller SPI setup intentionally leaves SCS2 as an input; preload its
     * inactive logical-zero level before changing direction so startup cannot
     * produce an unintended high pulse on this mechanism-owned line.
     */
    MODIFY_REG32(SPI_PC0, RDX_MECHANISM_AUXILIARY_MASK, 0U);
    MODIFY_REG32(SPI_PC3, RDX_MECHANISM_AUXILIARY_MASK, 0U);
    MODIFY_REG32(SPI_PC1, RDX_MECHANISM_AUXILIARY_MASK,
                 RDX_MECHANISM_AUXILIARY_MASK);

#if ENABLE_SATA_POWER_SWITCH_CONTROL

    if (gio_is_usb_device_self_powered())
    {
        // If device is self-powered, enable SATA power.
        // This must be done prior to setting the GPIO direction to avoid
        // transient power disruptions when resetting the system.
        gio_sata_device_power_enable(TRUE);
    }
    else
    {
        gio_sata_device_power_enable(FALSE);
    }

    MODIFY32(SPI_PC1, SPI_PC_SCS1_GPIO10, SPI_PC_SCS1_GPIO10);  /* Make SPI_CS[1]/GPIO[10] output */    
#endif

    return;
}


/*****************************************************************************
 * Function: gio_isr
 *************************************************************************//**
 * Interrupt service routine for the general-purpose I/O module.
 *
 * @param None.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_isr(void)
{
    UINT32_T gio_flags;

    // Read GIO flags to determine if there was a level transition and
    // mask off any interrupts that are not enabled.
    gio_flags = READ32(GIOFLG_REG_OFF) & READ32(GIOENASET_REG_OFF);

    CRIT("-> gio_isr() - unexpected flags = 0x%08x.\n", gio_flags);

    // Clear the interrupt flags.
    WRITE32(GIOFLG_REG_OFF, gio_flags);

    return;
}


/*****************************************************************************
 * Function: gio_sata_device_power_enable
 *************************************************************************//**
 * This function controls the SATA device power.
 *
 * @param[in] power_on flag to turn SATA device power ON.
 *                    
 * @retval None.
 *
 ****************************************************************************** 
 */

void gio_sata_device_power_enable(BOOLEAN_T power_on)
{
#if ENABLE_SATA_POWER_SWITCH_CONTROL

#if SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH
    if (power_on)
    {
        gio_high(SATA_PWR_ENABLE_GIO_NUM);
    }
    else
    {
        gio_low(SATA_PWR_ENABLE_GIO_NUM);
    }
#else /* Power enable is active low */
    if (power_on)
    {
        gio_low(SATA_PWR_ENABLE_GIO_NUM);
    }
    else
    {
        gio_high(SATA_PWR_ENABLE_GIO_NUM);
    }
#endif

#endif
    return;
}

/*****************************************************************************
 * Function: gio_is_sata_device_powered
 *************************************************************************//**
 * This function returns the state of the SATA device power enable.
 *
 * @param None.
 *                    
 * @retval TRUE when SATA device power is ON.
 * @retval FALSE when SATA device power is OFF.
 *
 ****************************************************************************** 
 */

BOOLEAN_T gio_is_sata_device_powered(void)
{
#if ENABLE_SATA_POWER_SWITCH_CONTROL

#if SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH
    return gio_get_state(SATA_PWR_ENABLE_GIO_NUM) ? TRUE : FALSE;
#else /* Power enable is active low */
    return gio_get_state(SATA_PWR_ENABLE_GIO_NUM) ? FALSE : TRUE;
#endif

#else

    return TRUE;
#endif
}


/*****************************************************************************
 * Function: gio_is_usb_device_self_powered
 *************************************************************************//**
 * This function returns TRUE is the USB device is currently self-powered and 
 * FALSE if bus-powered.
 *
 * @param None.
 *                    
 * @retval TRUE when USB device is self-powered.
 * @retval FALSE when USB device is bus-powered.
 *
 ****************************************************************************** 
 */

BOOLEAN_T gio_is_usb_device_self_powered(void)
{
    return gio_get_state(SELF_OR_BUS_POWER_INDICATOR_GIO_NUM) ? TRUE : FALSE;
}
