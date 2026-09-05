/*
 * SPDX-FileCopyrightText: 2010 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : one_touch.h
//
// Project     : TUSB926x Firmware.
//
// Description : Header file for one-touch backup function.
//
//   (C) Copyright 2010 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   09/23/10 - Dwight Schauer - Creation.
//
//=======================================================================================

/*! @file
 * 
 * Header file for the one-touch backup function.
 * 
 */

#ifndef _ONE_TOUCH_H_
#define _ONE_TOUCH_H_

#define ONE_TOUCH_STATE_SIZE   2  /* bytes */

typedef enum
{
    OTS_DEFAULT_STATE = 0xABBA,
    OTS_BUTTON_PRESSED = 0xACDC,
} one_touch_state_t;


void one_touch_button_pressed(void);

void * one_touch_button_state_query(void);

#endif /*_ONE_TOUCH_H_*/
