/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_PWM_H_H
#define TUSB9261_RDX_PWM_H_H

#include "tusb9261_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void pwm_configure_activity_led_channels();
/** @brief Firmware procedure. */
void pwm_init_activity_led();
/** @brief Firmware procedure. */
void pwm_init_heartbeat_led();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_PWM_H_H */
