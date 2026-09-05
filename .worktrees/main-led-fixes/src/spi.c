/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : spi.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the spi module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the spi module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Set logical output.
 *
 * @param context Procedure input.
 * @param condition Procedure input.
 * @return No value.
 */
void spi_set_logical_output(context, condition) uint8_t context;
int condition;

{
    /* Set logical output using only caller-provided data. */
    uint32_t *spi_get_spi_transfer_storage_result;
    uint8_t *data_cursor;
    int working_result;
    uint32_t status_bits;
    int pin_mask;
    int16_t operation_status;

    operation_status = 1;
    if (condition == 0) {
        spi_get_spi_transfer_storage_result = (uint32_t *)spi_get_spi_transfer_storage(context);
        if ((char)spi_get_spi_transfer_storage_result[1] == '\0')
            goto spi_set_logical_output_before_updating_status_bits;
    } else {
        spi_get_spi_transfer_storage_result = (uint32_t *)spi_get_spi_transfer_storage(context);
        if ((char)spi_get_spi_transfer_storage_result[1] != '\0')
            goto spi_set_logical_output_before_updating_status_bits;
    }
    operation_status = 0;
/* Shared target for SPI set logical output before updating status bits; incoming paths preserve the same state assumptions. */
spi_set_logical_output_before_updating_status_bits:
    status_bits = *spi_get_spi_transfer_storage_result;
    if (status_bits < 8) {
        working_result = 1 << (status_bits & 0xff);
        pin_mask = working_result;
        if (operation_status == 0) {
            pin_mask = 0;
        }
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        data_cursor = (uint8_t *)&GIOOUT0_REG_OFF;
    } else if ((status_bits == 8) || (status_bits == 9)) {
        working_result = 1 << (status_bits - 7 & 0xff);
        pin_mask = working_result;
        if (operation_status == 0) {
            pin_mask = 0;
        }
        data_cursor = (uint8_t *)&SCI_PIO3;
    } else {
        if ((status_bits != 10) && (status_bits != 0xb)) {
            if (status_bits != 0xc && status_bits != 0xd) {
                return;
            }
            status_bits = *spi_get_spi_transfer_storage_result;
            core_update_rti_clock_mhz_period_units(status_bits - 0xc, operation_status * 100, 250000);
            core_set_channel_mode_bits(status_bits - 0xc, operation_status);
            return;
        }
        working_result = 2 << (status_bits - 10 & 0xff);
        pin_mask = working_result;
        if (operation_status == 0) {
            pin_mask = 0;
        }
        data_cursor = (uint8_t *)&SPI_PC3;
    }
    rti_update_masked_register_bits(data_cursor, working_result, pin_mask);
    return;
}

/**
 * @brief Process pin mask SPI pc3.
 *
 * @param context Procedure input.
 * @return No value.
 */
void spi_process_pin_mask_spi_pc3(context) uint32_t context;

{
    /* Process pin mask SPI pc3 using only caller-provided data. */
    char operation_status;
    uint32_t *spi_get_spi_transfer_storage_result;
    uint32_t sci_pio3_pointer;
    int working_result;
    int pin_mask;
    uint8_t *data_cursor;

    spi_get_spi_transfer_storage_result = (uint32_t *)spi_get_spi_transfer_storage(context & 0xff);
    sci_pio3_pointer = *spi_get_spi_transfer_storage_result;
    if (sci_pio3_pointer < 8) {
        pin_mask = 1 << (sci_pio3_pointer & 0xff);
        working_result = pin_mask;
        if (*(char *)((int)spi_get_spi_transfer_storage_result + 5) == '\0') {
            working_result = 0;
        }
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        data_cursor = (uint8_t *)&GIOOUT0_REG_OFF;
    } else {
        if ((sci_pio3_pointer == 8) || (sci_pio3_pointer == 9)) {
            data_cursor = (uint8_t *)&SCI_PIO3;
            /* Process pin mask SPI pc3; this branch selects whether its associated handler runs. */
            pin_mask = 1 << (*spi_get_spi_transfer_storage_result - 7 & 0xff);
            core_update_masked_register_bits(&SCI_PIO0, pin_mask, 0);
            operation_status = *(char *)((int)spi_get_spi_transfer_storage_result + 5);
        } else {
            if (sci_pio3_pointer != 10 && sci_pio3_pointer != 0xb)
                goto spi_process_pin_mask_spi_pc3_before_set_logical_output;
            data_cursor = (uint8_t *)&SPI_PC3;
            /* Process pin mask SPI pc3; this branch selects whether its associated handler runs. */
            pin_mask = 2 << (*spi_get_spi_transfer_storage_result - 10 & 0xff);
            core_update_masked_register_bits(&SPI_PC0, pin_mask, 0);
            operation_status = *(char *)((int)spi_get_spi_transfer_storage_result + 5);
        }
        working_result = pin_mask;
        if (operation_status == '\0') {
            working_result = 0;
        }
    }
    core_update_masked_register_bits(data_cursor + -8, pin_mask, working_result);
/* Shared target for SPI process pin mask SPI pc3 before set logical output; incoming paths preserve the same state assumptions. */
spi_process_pin_mask_spi_pc3_before_set_logical_output:
    spi_set_logical_output(context, 0);
    return;
}

/**
 * @brief Process SPI gcr0 SPI pc7 peripheral pin configuration.
 *
 * @return No value.
 */
void spi_process_spi_gcr0_spi_pc7_peripheral_pin_configuration()

{
    /* Process SPI gcr0 SPI pc7 peripheral pin configuration; persistent state is carried in GIO pin callback table. */
    uint32_t entry_index;

    core_process_rti_clock_mhz(0);
    core_process_rti_clock_mhz(1);
    ti_memset(&gio_pin_callback_table, 0, 0x20);
    entry_index = 0;
    /* Program GIOGCR0 REG OFF with 1; write ordering is hardware-significant. */
    GIOGCR0_REG_OFF = 1;
    SPI_GCR0 = 1;
    SCI_GCR0 = 1;
    /* Program GIOFLG REG OFF with 0xffff; write ordering is hardware-significant. */
    GIOFLG_REG_OFF = 0xffff;
    /* Repeat while entry_index < 0x11; the body advances or polls the state needed to leave the loop. */
    do {
        spi_process_pin_mask_spi_pc3(entry_index);
        entry_index = entry_index + 1;
    } while (entry_index < 0x11);
    /* Program GIOPULDIS0 REG OFF with 0xff; write ordering is hardware-significant. */
    GIOPULDIS0_REG_OFF = 0xff;
    core_update_masked_register_bits(&SCI_PIO7, 6, 6);
    core_update_masked_register_bits(&SPI_PC7, 6, 6);
    return;
}

/**
 * @brief Initialize the SPI subsystem state and required hardware resources.
 *
 * @return No value.
 */
void SPI_Init()

{
    /* SPI Init using only caller-provided data. */
    uint32_t global_control;

    SPI_GCR1 = 3;
    rti_update_masked_register_bits(&SPI_PC0, 0xe03, 0xe03);
    rti_update_masked_register_bits(&SPI_PC1, 0x601, 0x601);
    SPI_DELAY = 0x2020000;
    SPI_DEF = 3;
    SPI_PC7 = 4;
    SPI_FMT0 = 0x1010308;
    SPI_FMT1 = 0x1002408;
    SPI_LVL = 0;
    global_control = SPI_GCR1;
    SPI_GCR1 = global_control | 0x1000000;
    return;
}

/**
 * @brief Transmit one SPI word and return the simultaneously received word.
 *
 * @param transmit_word Word written to the SPI data register.
 * @param received_word Receives the word read from the SPI buffer register.
 * @return Result produced by the procedure.
 */
int spi_transfer_word(transmit_word, received_word)
uint32_t transmit_word;
uint32_t *received_word;

{
    /* Transfer word using only caller-provided data. */
    uint32_t spi_flags;
    uint32_t receive_buffer;
    int transfer_status;

    transfer_status = spi_wait_for_flag(0x200);
    if (transfer_status == 0) {
        SPI_DAT1 = transmit_word;
        transfer_status = spi_wait_for_flag(0x100);
        if ((transfer_status == 0) &&
            (receive_buffer = SPI_BUF, spi_flags = SPI_FLG, *received_word = receive_buffer, (spi_flags >> 4 & 1) != 0)) {
            transfer_status = -1;
            SPI_FLG = 0x10;
        }
    }
    return transfer_status;
}

/**
 * @brief Poll SPI status until the requested flag appears or the timeout expires.
 *
 * @param completion_mask SPI flag bits that indicate completion.
 * @return Result produced by the procedure.
 */
uint32_t spi_wait_for_flag(completion_mask)
uint32_t completion_mask;

{
    /* Wait for flag using only caller-provided data. */
    uint32_t spi_flags;
    uint32_t timeout_iterations;

    timeout_iterations = 0;
    /* Iterate while spi_flags = SPI_FLG, (completion_mask & spi_flags) == 0, preserving the defined cursor and bound. */
    while (spi_flags = SPI_FLG, (completion_mask & spi_flags) == 0) {
        if (999999 < timeout_iterations) {
            return 0xffffffff;
        }
        rti_delay_microseconds_with_watchdog(10);
        timeout_iterations = timeout_iterations + 1;
    }
    if (999999 < timeout_iterations) {
        return 0xffffffff;
    }
    return 0;
}

/**
 * @brief Get SPI transfer storage.
 *
 * @param entry_index SPI configuration table index.
 * @return Result produced by the procedure.
 */
uint8_t *spi_get_spi_transfer_storage(entry_index)
int entry_index;

{
    /* Get SPI transfer storage; persistent state is carried in SPI transaction table. */
    return (uint8_t *)(&spi_transaction_table + entry_index * 8);
}
