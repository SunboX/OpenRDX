/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef TUSB9261_RDX_SPI_H_H
#define TUSB9261_RDX_SPI_H_H

#include "tusb9261_types.h"

/* Logical outputs 6--9 are active-low. The renderer treats each pair as
 * generic first/second outputs; board wiring identifies F2B4 as
 * cartridge green/amber and F29C as dock amber/green. Keep these constants
 * aligned with the production LED controller instead of assuming a
 * common color order for both pairs. */
#define RDX_CARTRIDGE_GREEN_LOGICAL_OUTPUT 6U
#define RDX_CARTRIDGE_AMBER_LOGICAL_OUTPUT 7U
#define RDX_DOCK_AMBER_LOGICAL_OUTPUT 8U
#define RDX_DOCK_GREEN_LOGICAL_OUTPUT 9U

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Firmware procedure. */
void SPI_Init();
/** @brief Firmware procedure. */
int spi_transfer_word();
/** @brief Firmware procedure. */
uint8_t *spi_get_logical_pin_configuration();
/** @brief Firmware procedure. */
void spi_configure_logical_pin_direction();
/** @brief Firmware procedure. */
void spi_initialize_logical_gpio();
/** @brief Firmware procedure. */
uint32_t spi_wait_for_flag();
/** @brief Firmware procedure. */
void spi_set_logical_output();
#ifdef __cplusplus
}
#endif

#endif /* TUSB9261_RDX_SPI_H_H */
