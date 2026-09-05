/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : gio.h
//             
// Project     : TUSB926x Firmware.
//             
// Description : Header file for GIO module driver.
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
 * Header file for the General Input/Output module driver.
 * 
 */

#ifndef _GIO_H_
#define _GIO_H_

#include "tusb9260_types.h"

/*----------------------------------------------------------------------------+
| Macros                                                                      |
+----------------------------------------------------------------------------*/

#define LED_PULL_UP_RESISTORS    1   /* Set this to 1 if LEDs are connected to pull-up resistors 
                                        and GIO must be driven low to turn on the LED */

/* The PDK board power switch has active-high enable and the DEMO board has active low enable */
#ifndef SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH
#define SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH   0  /* Set to 1 if the SATA device power switch enable is active high */
#endif

#if LED_PULL_UP_RESISTORS
#define LED_ON(gio_num)   gio_low(gio_num)
#define LED_OFF(gio_num)  gio_high(gio_num)
#else /* LED is connected to ground and GIO */
#define LED_ON(gio_num)   gio_high(gio_num)
#define LED_OFF(gio_num)  gio_low(gio_num)
#endif

#define GIOGCR0_REG_OFF      0xFFF7BC00
#define GIOINTDET_REG_OFF    0xFFF7BC00
#define GIOPOL_REG_OFF       0xFFF7BC0C
#define GIOENASET_REG_OFF    0xFFF7BC10
#define GIOENACLR_REG_OFF    0xFFF7BC14
#define GIOLVLSET_REG_OFF    0xFFF7BC18
#define GIOLVLCLR_REG_OFF    0xFFF7BC1C
#define GIOFLG_REG_OFF       0xFFF7BC20
#define GIOOFFA_REG_OFF      0xFFF7BC24
#define GIOOFFB_REG_OFF      0xFFF7BC28
#define GIOEMUA_REG_OFF      0xFFF7BC2C
#define GIOEMUB_REG_OFF      0xFFF7BC30
#define GIODIR0_REG_OFF      0xFFF7BC34
#define GIOIN0_REG_OFF       0xFFF7BC38
#define GIOOUT0_REG_OFF      0xFFF7BC3C
#define GIODSET0_REG_OFF     0xFFF7BC40
#define GIODCLR0_REG_OFF     0xFFF7BC44
#define GIOPDR0_REG_OFF      0xFFF7BC48
#define GIOPULDIS0_REG_OFF   0xFFF7BC4C
#define GIOPSL0_REG_OFF      0xFFF7BC50
#define GIOSRS0_REG_OFF      0xFFF7BD34

#define NUM_STD_GIOS         8  /* 1-based */
#define TOTAL_NUM_GIOS       12 /* 1-based */

typedef enum GIO_NUM
{
    GIO0_NUM                              = 0,
    RDX_EJECT_BUTTON_GIO_NUM              = 1,
    RDX_MECHANISM_INPUT_GIO_NUM           = 2,
    RDX_MOTOR_CONTROL_GIO_NUM             = 3,
    RDX_MOTOR_GATE_GIO_NUM                = RDX_MOTOR_CONTROL_GIO_NUM,
    SELF_OR_BUS_POWER_INDICATOR_GIO_NUM   = 4,
    RDX_CARTRIDGE_PRESENT_GIO_NUM         = 5,
    RDX_CARTRIDGE_AMBER_GIO_NUM           = 6,
    RDX_CARTRIDGE_GREEN_GIO_NUM           = 7,

    /* The dock/eject-button LEDs are GPIOs supplied by SCI. */
    RDX_DOCK_AMBER_GIO_NUM                = 8,
    RDX_DOCK_GREEN_GIO_NUM                = 9,

    /* The following are GIOs from SPI module */
    SATA_PWR_ENABLE_GIO_NUM               = 10,
    RDX_MECHANISM_AUXILIARY_GIO_NUM       = 11,

    /* This alias resolves to logical output 5. Product code must use the
     * mechanism-auxiliary API and must not treat the pin as a fault input. */
    POWER_FAULT_GIO_NUM                   = RDX_MECHANISM_AUXILIARY_GIO_NUM,

    /*
     * Demo-board indicators do not exist on the RDX hardware. Their optional
     * status-helper selectors are deliberately out-of-range no-ops so they
     * cannot drive an RDX input or actuator pin.
     */
    RDX_UNMAPPED_TI_INDICATOR_GIO_NUM     = TOTAL_NUM_GIOS,
    SS_LINK_STATE_LED0_GIO_NUM            = RDX_UNMAPPED_TI_INDICATOR_GIO_NUM,
    HS_SUSPEND_LED_GIO_NUM                = RDX_UNMAPPED_TI_INDICATOR_GIO_NUM,
    INPUT_BUTTON_GIO_NUM                  = RDX_EJECT_BUTTON_GIO_NUM,
    SS_LINK_STATE_LED1_GIO_NUM            = RDX_UNMAPPED_TI_INDICATOR_GIO_NUM,
    GIO6_NUM                              = RDX_CARTRIDGE_AMBER_GIO_NUM,
    SS_USB_LED_GIO_NUM                    = RDX_UNMAPPED_TI_INDICATOR_GIO_NUM,
    UART_RX_GIO8_NUM                      = RDX_DOCK_AMBER_GIO_NUM,
    UART_TX_GIO9_NUM                      = RDX_DOCK_GREEN_GIO_NUM
} GIO_NUM_T;


typedef enum GIO_DIR
{
    GIO_DIR_INPUT  = 0,
    GIO_DIR_OUTPUT = 1
} GIO_DIR_T;

/*----------------------------------------------------------------------------+
| Function Prototypes                                                         |
+----------------------------------------------------------------------------*/

void gio_high(UINT32_T gio_num);
void gio_low(UINT32_T gio_num);
void gio_toggle(UINT32_T gio_num);

UINT32_T gio_get_state(UINT32_T gio_num);
UINT32_T gio_get_direction(void);
UINT32_T gio_get_function(void);

void gio_set_direction(UINT32_T gio_num, GIO_DIR_T dir);

UINT32_T gio_get_pull_ctrl(void);
UINT32_T gio_get_pull_disable(void);

void gio_set_as_input(UINT32_T gio_num);

BOOLEAN_T gio_rdx_eject_button_pressed(void);
BOOLEAN_T gio_rdx_mechanism_input_asserted(void);
BOOLEAN_T gio_rdx_cartridge_present(void);

/**
 * @brief Drive logical output 10's GPIO3 level without inferred use.
 *
 * Mechanism states 1/3 drive this output low and state 6 drives it high.  The
 * required levels and their ordering relative to PWM1 do not establish a
 * narrower board-level electrical function.
 *
 * @param[in] high TRUE to drive GPIO3 high, FALSE to drive it low.
 */
void gio_rdx_motor_control_set_high(BOOLEAN_T high);

/**
 * @brief Drive GPIO3 through a low-asserted enable-style convenience API.
 *
 * @param[in] enable TRUE to drive GPIO3 low, FALSE to drive GPIO3 high.
 */
void gio_rdx_motor_gate_enable(BOOLEAN_T enable);

/**
 * @brief Set logical output 5, the generic RDX mechanism auxiliary.
 *
 * Logical output 5 maps to selector 11 (SPI SCS2/GPIO11) with active-high
 * polarity.  Its board-level electrical purpose is intentionally unspecified,
 * so callers must not infer SATA-power, fault-input, or other semantics from
 * this interface.
 *
 * @param[in] asserted TRUE for logical one/physical high, FALSE for logical
 *                     zero/physical low.
 */
void gio_rdx_mechanism_auxiliary_set(BOOLEAN_T asserted);

void gio_init(void);
void gio_isr(void);


void gio_sata_device_power_enable(BOOLEAN_T power_on);
BOOLEAN_T gio_is_sata_device_powered(void);
BOOLEAN_T gio_is_usb_device_self_powered(void);

#endif
