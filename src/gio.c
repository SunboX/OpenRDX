/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : gio.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the gio module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the gio module.
 */

#include "../include/rdx_firmware.h"

#define RDX_SATA_POWER_ENABLE_MASK 0x00000004U

/**
 * @brief Configure GPIO11 and assert the Tandberg RDX SATA power enable.
 *
 * @return No value.
 */
void gio_enable_rdx_sata_power()

{
    /*
     * The fixed Tandberg pin table maps cartridge-power logical output 5
     * to physical GPIO11 with active-high polarity. GPIO11 shares SPI_CS2.
     * Set the output latch high before enabling the output driver.
     */
    core_update_masked_register_bits(
        &SPI_PC3,
        RDX_SATA_POWER_ENABLE_MASK,
        RDX_SATA_POWER_ENABLE_MASK);
    core_update_masked_register_bits(&SPI_PC0, RDX_SATA_POWER_ENABLE_MASK, 0);
    core_update_masked_register_bits(
        &SPI_PC1,
        RDX_SATA_POWER_ENABLE_MASK,
        RDX_SATA_POWER_ENABLE_MASK);
    return;
}

/**
 * @brief Gio configure logical pin interrupt.
 *
 * @param pin_config_index Logical pin configuration index.
 * @param configuration_value Pin configuration selector passed to the lookup.
 * @return No value.
 */
void gio_configure_logical_pin_interrupt(pin_config_index, configuration_value) uint8_t pin_config_index;
uint32_t configuration_value;

{
    /* Configure pin configuration pin number peripheral pin configuration; persistent state is carried in GIO pin callback table. */
    uint32_t active_high;
    uint32_t *pin_configuration;
    uint32_t pin_number;
    uint32_t callback_address = 0;
    int polarity_bits;
    int pin_mask;

    pin_configuration = (uint32_t *)spi_get_logical_pin_configuration(pin_config_index, configuration_value, configuration_value);
    pin_number = *pin_configuration;
    if (pin_number < 8) {
        active_high = pin_configuration[1];
        (&gio_pin_callback_table)[pin_number] = callback_address;
        pin_mask = 1 << (pin_number & 0xff);
        polarity_bits = pin_mask;
        if ((char)active_high == '\0') {
            polarity_bits = 0;
        }
        core_update_masked_register_bits(&GIOPOL_REG_OFF, pin_mask, polarity_bits);
        /* Program GIOLVLSET REG OFF with pin_mask; write ordering is hardware-significant. */
        GIOLVLSET_REG_OFF = pin_mask;
        GIOFLG_REG_OFF = pin_mask;
        /* Program GIOENASET REG OFF with pin_mask; write ordering is hardware-significant. */
        GIOENASET_REG_OFF = pin_mask;
    }
    return;
}
