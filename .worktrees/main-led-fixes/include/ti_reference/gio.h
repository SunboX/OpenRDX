/*
 * SPDX-FileCopyrightText: 2009 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x
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

#define LED_PULL_UP_RESISTORS                                                                                                              \
    1 /* Set this to 1 if LEDs are connected to pull-up resistors 
                                        and GIO must be driven low to turn on the LED */

/* The PDK board power switch has active-high enable and the DEMO board has active low enable */
#ifndef SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH
#define SATA_DEVICE_PWR_SWITCH_ENABLE_IS_ACTIVE_HIGH 0 /* Set to 1 if the SATA device power switch enable is active high */
#endif

#if LED_PULL_UP_RESISTORS
#define LED_ON(gio_num) gio_low(gio_num)
#define LED_OFF(gio_num) gio_high(gio_num)
#else /* LED is connected to ground and GIO */
#define LED_ON(gio_num) gio_high(gio_num)
#define LED_OFF(gio_num) gio_low(gio_num)
#endif

#define GIOGCR0_REG_OFF 0xFFF7BC00
#define GIOINTDET_REG_OFF 0xFFF7BC00
#define GIOPOL_REG_OFF 0xFFF7BC0C
#define GIOENASET_REG_OFF 0xFFF7BC10
#define GIOENACLR_REG_OFF 0xFFF7BC14
#define GIOLVLSET_REG_OFF 0xFFF7BC18
#define GIOLVLCLR_REG_OFF 0xFFF7BC1C
#define GIOFLG_REG_OFF 0xFFF7BC20
#define GIOOFFA_REG_OFF 0xFFF7BC24
#define GIOOFFB_REG_OFF 0xFFF7BC28
#define GIOEMUA_REG_OFF 0xFFF7BC2C
#define GIOEMUB_REG_OFF 0xFFF7BC30
#define GIODIR0_REG_OFF 0xFFF7BC34
#define GIOIN0_REG_OFF 0xFFF7BC38
#define GIOOUT0_REG_OFF 0xFFF7BC3C
#define GIODSET0_REG_OFF 0xFFF7BC40
#define GIODCLR0_REG_OFF 0xFFF7BC44
#define GIOPDR0_REG_OFF 0xFFF7BC48
#define GIOPULDIS0_REG_OFF 0xFFF7BC4C
#define GIOPSL0_REG_OFF 0xFFF7BC50
#define GIOSRS0_REG_OFF 0xFFF7BD34

#define NUM_STD_GIOS 8    /* 1-based */
#define TOTAL_NUM_GIOS 12 /* 1-based */

typedef enum GIO_NUM {
    GIO0_NUM = 0,
    SS_LINK_STATE_LED0_GIO_NUM = 1,
    HS_SUSPEND_LED_GIO_NUM = 2,
    INPUT_BUTTON_GIO_NUM = 3,
    SELF_OR_BUS_POWER_INDICATOR_GIO_NUM = 4,
    SS_LINK_STATE_LED1_GIO_NUM = 5,
    GIO6_NUM = 6,
    //    HS_USB_LED_GIO_NUM                    = 6,
    SS_USB_LED_GIO_NUM = 7,

    /* The following are GIOs from SCI module */
    UART_RX_GIO8_NUM = 8,
    UART_TX_GIO9_NUM = 9,

    /* The following are GIOs from SPI module */
    SATA_PWR_ENABLE_GIO_NUM = 10,
    POWER_FAULT_GIO_NUM = 11
} GIO_NUM_T;

typedef enum GIO_DIR { GIO_DIR_INPUT = 0, GIO_DIR_OUTPUT = 1 } GIO_DIR_T;

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

void gio_init(void);
void gio_isr(void);

void gio_sata_device_power_enable(BOOLEAN_T power_on);
BOOLEAN_T gio_is_sata_device_powered(void);
BOOLEAN_T gio_is_usb_device_self_powered(void);

#endif
