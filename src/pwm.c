/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : pwm.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the pwm module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the pwm module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Initialize the PWM subsystem state and required hardware resources.
 *
 * @return No value.
 */
void pwm_init_heartbeat_led()

{
    /* Pwm init heartbeat led using only caller-provided data. */
    core_configure_board_logical_pins();
    core_hold_logical_output_fourteen();
    core_process_notification_context();
    return;
}

/**
 * @brief Pwm configure activity led channels.
 *
 * @return No value.
 */
void pwm_configure_activity_led_channels()

{
    /* Pwm configure activity led channels using only caller-provided data. */
    core_clear_firmware_image_validation();
    core_initialize_rdx_identity_records();
    return;
}

/**
 * @brief Initialize the PWM subsystem state and required hardware resources.
 *
 * @return No value.
 */
void pwm_init_activity_led()

{
    /* Pwm init activity led using only caller-provided data. */
    core_clear_firmware_image_validation();
    core_initialize_rdx_identity_records();
    return;
}
