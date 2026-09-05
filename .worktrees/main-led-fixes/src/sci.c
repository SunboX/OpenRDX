/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : sci.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the sci module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the sci module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Read logical input.
 *
 * @param input_index Logical input configuration index.
 * @return Result produced by the procedure.
 */
uint32_t sci_read_logical_input(input_index)
uint8_t input_index;

{
    /* Read logical input using only caller-provided data. */
    uint32_t *input_configuration;
    uint32_t logical_input;
    uint32_t *input_register;
    uint32_t input_state;

    input_configuration = (uint32_t *)spi_get_spi_transfer_storage(input_index);
    logical_input = *input_configuration;
    if (logical_input < 8) {
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        input_register = (uint32_t *)&GIOIN0_REG_OFF;
    } else {
        if ((logical_input != 8) && (logical_input != 9)) {
            if ((logical_input == 10) || (logical_input == 0xb)) {
                input_state = SPI_PC2;
                input_state = (uint32_t)((2 << (logical_input - 10 & 0xff) & input_state) != 0);
            } else {
                input_state = 0;
                if ((0xd < logical_input) && (logical_input < 0x17)) {
                    input_state = core_update_device_update_context(logical_input - 0xe & 0xff);
                }
            }
            goto sci_read_logical_input_return_path;
        }
        input_register = (uint32_t *)&SCI_PIO2;
        logical_input = logical_input - 7;
    }
    input_state = *input_register >> (logical_input & 0xff) & 1;
/* Shared target for SCI read logical input return path; incoming paths preserve the same state assumptions. */
sci_read_logical_input_return_path:
    if (input_state == 0) {
        if ((char)input_configuration[1] == '\0') {
            return 1;
        }
    } else if ((char)input_configuration[1] != '\0') {
        return 1;
    }
    return 0;
}

/**
 * @brief Initialize the SCI subsystem state and required hardware resources.
 *
 * @return No value.
 */
void sci_init()

{
    /* Init using only caller-provided data. */
    core_update_clear_memory_thermal_control_context();
    spi_process_spi_gcr0_spi_pc7_peripheral_pin_configuration();
    core_process_clear_memory_response_end_context();
    core_process_clear_memory_command_phase();
    return;
}
