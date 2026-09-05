/*
 * SPDX-FileCopyrightText: 2010 Texas Instruments Incorporated
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: LicenseRef-TI-TUSB926x AND AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : one_touch.c
//
// Project     : TUSB926x Firmware.
//
// Description : One-touch backup button state logic.
//
//   (C) Copyright 2010 by Texas Instruments Incorporated.
//   All rights reserved.
//
// Revision History:
//   MM/DD/YY
//   09/22/10 - Dwight Schauer - Creation.
//
//=======================================================================================

/*! @file
 *
 * This file contains functions to query and update the one touch button state.
 *
 */

#include "one_touch.h"
#include "sci.h"
#include "tusb9260.h"

static one_touch_state_t one_touch_state = OTS_DEFAULT_STATE;

/*****************************************************************************
 * Function: one_touch_button_pressed
 *************************************************************************//**
 * Updates one touch state to pressed if in the the default state.
 *
 * @param None.
 *
 * @retval None.
 *
 ******************************************************************************
 */
void one_touch_button_pressed(void)
{
    switch (one_touch_state)
    {
        case OTS_DEFAULT_STATE:
            one_touch_state = OTS_BUTTON_PRESSED;
            DEBUG("  one_touch_state: OTS_DEFAULT_STATE to OTS_BUTTON_PRESSED.\n");
            break;

        case OTS_BUTTON_PRESSED: break;
    }
}

/*****************************************************************************
 * Function: one_touch_button_state_query
 *************************************************************************//**
 * Queries and updates the one touch state.
 *
 * @param None.
 *
 * @retval pointer to one touch button state prior to being updated.
 *
 ******************************************************************************
 */
void * one_touch_button_state_query(void)
{
    static one_touch_state_t query_state;

    query_state = one_touch_state;

    switch (one_touch_state)
    {
        case OTS_DEFAULT_STATE: break;

        case OTS_BUTTON_PRESSED:
            one_touch_state = OTS_DEFAULT_STATE;
            break;
    }

    return (void*)&query_state;
}


