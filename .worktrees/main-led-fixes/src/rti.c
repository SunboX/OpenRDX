/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : rti.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the rti module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the rti module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Initialize the RTI subsystem state and required hardware resources.
 *
 * @return No value.
 */
void rti_init()

{
    /* Init; persistent state is carried in RTI counter zero callback, RTI counter one callback. */
    rti_counter_zero_callback = 0;
    rti_counter_one_callback = 0;
    /* Program RTIGCTRL REG OFF with 0; write ordering is hardware-significant. */
    RTIGCTRL_REG_OFF = 0;
    RTISETINT_REG_OFF = 0;
    /* Program RTIUC0 REG OFF with 0; write ordering is hardware-significant. */
    RTIUC0_REG_OFF = 0;
    RTIFRC0_REG_OFF = 0;
    /* Program RTICPUC0 REG OFF with rti_clock_mhz - 1U; write ordering is hardware-significant. */
    RTICPUC0_REG_OFF = rti_clock_mhz - 1U;
    rti_update_masked_register_bits(0xfffffc80, 0x20000, 0x20000);
    rti_update_masked_register_bits(0xfffffc00, 1, 1);
    rti_start_periodic_interrupts();
    return;
}

/**
 * @brief Configure counter one and start the periodic RTI interrupt schedule.
 *
 * @return No value.
 */
void rti_start_periodic_interrupts()

{
    /* Start periodic interrupts using only caller-provided data. */
    /* Program RTIUC1 REG OFF with 0; write ordering is hardware-significant. */
    RTIUC1_REG_OFF = 0;
    RTIFRC1_REG_OFF = 0;
    /* Program RTICPUC1 REG OFF with rti_clock_mhz * 1000U - 1U; write ordering is hardware-significant. */
    RTICPUC1_REG_OFF = rti_clock_mhz * 1000U - 1U;
    rti_config_compare_interrupt(0, 10);
    rti_update_masked_register_bits(0xfffffc80, 0x40000, 0x40000);
    rti_update_masked_register_bits(0xfffffc00, 2, 2);
    return;
}

/**
 * @brief Program one RTI compare channel and enable its interrupt.
 *
 * @param compare_index RTI compare channel index.
 * @param compare_ticks New compare interval in RTI ticks.
 * @return No value.
 */
void rti_config_compare_interrupt(compare_index, compare_ticks) uint32_t compare_index;
int compare_ticks;

{
    /* Config compare interrupt using only caller-provided data. */
    int compare_control_mask;
    int *compare_register;
    int compare_interrupt_mask;

    compare_interrupt_mask = 1 << (compare_index & 0xff);
    compare_register = (int *)((int)compare_index * 8 - 0x3b0);
    /* Program RTICLRINT REG OFF with compare_interrupt_mask; write ordering is hardware-significant. */
    RTICLRINT_REG_OFF = compare_interrupt_mask;
    compare_control_mask = 1 << ((compare_index & 0x3f) << 2);
    *compare_register = compare_ticks + *compare_register;
    *(int *)((int)compare_index * 8 - 0x3ac) = compare_ticks;
    rti_update_masked_register_bits(0xfffffc0c, compare_control_mask, compare_control_mask);
    rti_update_masked_register_bits(0xfffffc80, compare_interrupt_mask, compare_interrupt_mask);
    return;
}

/**
 * @brief Busy-wait for a microsecond interval while servicing the watchdog.
 *
 * @param delay_ticks Busy-wait interval in RTI counter ticks.
 * @return No value.
 */
void rti_delay_microseconds(delay_ticks) uint32_t delay_ticks;

{
    /* Delay microseconds using only caller-provided data. */
    int start_counter;
    uint32_t current_counter;
    uint32_t deadline;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    start_counter = RTIFRC0_REG_OFF;
    deadline = delay_ticks + start_counter;
    if (deadline < delay_ticks) {
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        current_counter = RTIFRC0_REG_OFF;
        /* Iterate while deadline < current_counter, preserving the defined cursor and bound. */
        while (deadline < current_counter) {
            /* Snapshot the fixed-address register before decoding its status or capability fields. */
            current_counter = RTIFRC0_REG_OFF;
        }
    }
    /* Iterate while current_counter = RTIFRC0_REG_OFF, current_counter < deadline, preserving the defined cursor and bound. */
    while (current_counter = RTIFRC0_REG_OFF, current_counter < deadline) {
        wdt_reset();
    }
    return;
}

/**
 * @brief Busy-wait for a millisecond interval on RTI counter one.
 *
 * @param delay_ticks Busy-wait interval in RTI counter ticks.
 * @return No value.
 */
void rti_delay_milliseconds(delay_ticks) uint32_t delay_ticks;

{
    /* Delay milliseconds using only caller-provided data. */
    int start_counter;
    uint32_t current_counter;
    uint32_t deadline;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    start_counter = RTIFRC1_REG_OFF;
    deadline = delay_ticks + start_counter;
    if (deadline < delay_ticks) {
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        current_counter = RTIFRC1_REG_OFF;
        /* Iterate while deadline < current_counter, preserving the defined cursor and bound. */
        while (deadline < current_counter) {
            /* Snapshot the fixed-address register before decoding its status or capability fields. */
            current_counter = RTIFRC1_REG_OFF;
        }
    }
    /* Repeat while current_counter < deadline; the body advances or polls the state needed to leave the loop. */
    do {
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        current_counter = RTIFRC1_REG_OFF;
    } while (current_counter < deadline);
    return;
}

/**
 * @brief Update interrupt enabled rtisetint reg off.
 *
 * @param interrupt_enabled Receives whether compare interrupt zero is enabled.
 * @return No value.
 */
void rti_update_interrupt_enabled_rtisetint_reg_off(interrupt_enabled) uint8_t *interrupt_enabled;

{
    /* Update interrupt enabled rtisetint reg off using only caller-provided data. */
    uint32_t enabled_interrupts;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    enabled_interrupts = RTISETINT_REG_OFF;
    if ((enabled_interrupts & 1) != 0) {
        rti_update_masked_register_bits(0xfffffc84, 1, 1);
        *interrupt_enabled = 1;
    }
    return;
}

/**
 * @brief Process thermal timer state.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @param flags Procedure input.
 * @return No value.
 */
void rti_process_thermal_timer_state(buffer, value, length, flags) uint8_t *buffer;
uint32_t value;
uint32_t length;
uint32_t flags;

{
    /* Process thermal timer state; persistent state is carried in core thermal control context. */
    uint32_t flags_snapshot;

    flags_snapshot = flags;
    rti_forward_rtisetint_reg_off_rtisetint(&flags_snapshot);
    *(uint32_t *)(&core_thermal_control_context + (uint32_t)*buffer * 4) = value;
    rti_enable_counter_when_requested(flags_snapshot & 0xff);
    return;
}

/**
 * @brief Enable counter if requested.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void rti_enable_counter_if_requested(condition) int condition;

{
    /* Enable counter if requested using only caller-provided data. */
    if (condition != 0) {
        rti_update_masked_register_bits(0xfffffc80, 1, 1);
        return;
    }
    return;
}

/**
 * @brief Update masked register bits.
 *
 * @param buffer Procedure input.
 * @param value Procedure input.
 * @param length Procedure input.
 * @return No value.
 */
void rti_update_masked_register_bits(buffer, value, length) uint32_t *buffer;
uint32_t value;
uint32_t length;

{
    /* Update masked register bits using only caller-provided data. */
    *buffer = length & value | *buffer & ~value;
    return;
}

/**
 * @brief Forward execution to the corresponding RTI implementation routine.
 *
 * @param buffer Procedure input.
 * @return No value.
 */
void rti_forward_rtisetint_reg_off_rtisetint(buffer) uint8_t *buffer;

{
    /* Forward rtisetint reg off rtisetint using only caller-provided data. */
    uint32_t rtisetint_snapshot;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    rtisetint_snapshot = RTISETINT_REG_OFF;
    if ((rtisetint_snapshot & 1) != 0) {
        /* Call RTI update masked register bits through the defined unprototyped function-pointer signature. */
        ((void (*)())rti_update_masked_register_bits)(0xfffffc84, 1, 1);
        *buffer = 1;
    }
    return;
}

/**
 * @brief Forward execution to the corresponding RTI implementation routine.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void rti_enable_counter_when_requested(condition) int condition;

{
    /* Enable counter when requested using only caller-provided data. */
    if (condition != 0) {
        /* Call RTI update masked register bits through the defined unprototyped function-pointer signature. */
        ((void (*)())rti_update_masked_register_bits)(0xfffffc80, 1, 1);
        return;
    }
    return;
}

/**
 * @brief Busy-wait on RTI counter zero while servicing the watchdog.
 *
 * @param condition Procedure input.
 * @return No value.
 */
void rti_delay_microseconds_with_watchdog(condition) uint32_t condition;

{
    /* Delay microseconds with watchdog using only caller-provided data. */
    int rtifrc0_snapshot;
    uint32_t operation_status;
    uint32_t condition_field;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    rtifrc0_snapshot = RTIFRC0_REG_OFF;
    condition_field = condition + rtifrc0_snapshot;
    if (condition_field < condition) {
        /* Snapshot the fixed-address register before decoding its status or capability fields. */
        operation_status = RTIFRC0_REG_OFF;
        /* Iterate while condition_field < operation_status, preserving the defined cursor and bound. */
        while (condition_field < operation_status) {
            /* Snapshot the fixed-address register before decoding its status or capability fields. */
            operation_status = RTIFRC0_REG_OFF;
        }
    }
    /* Iterate while operation_status = RTIFRC0_REG_OFF, operation_status < condition_field, preserving the defined cursor and bound. */
    while (operation_status = RTIFRC0_REG_OFF, operation_status < condition_field) {
        wdt_reset();
    }
    return;
}
