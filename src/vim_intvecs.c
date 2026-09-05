/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : vim_intvecs.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the vim intvecs module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the vim intvecs module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void ahci_rx_error_isr()

{
    /* Rx error isr; persistent state is carried in AHCI port phy status register, AHCI port SATA status register and related record fields. */
    uint32_t ahci_port_status;
    int handler_status;
    uint32_t interrupt_cause;

    wdt_reset();
    /* Snapshot interrupt MMIO register 003 before decoding its status or capability fields. */
    interrupt_cause = ahci_port_phy_status_register;
    interrupt_cause = (interrupt_cause & 0xfff) >> 8;
    if (interrupt_cause != 0) {
        handler_status = core_process_sense_record_context(0);
        if (handler_status == 0) {
            ahci_port_status = ahci_port_sata_status_register;
            if ((ahci_port_status & 0xf) != 4) {
                if (interrupt_cause == 0xf) {
                    if (ahci_rx_error_burst_count == 0) {
                        /* Snapshot ahci rx error window start timestamp before decoding its status or capability fields. */
                        ahci_rx_error_window_start_timestamp = RTIFRC1_REG_OFF;
                    }
                    ahci_rx_error_burst_count = ahci_rx_error_burst_count + 1;
                    if (4 < ahci_rx_error_burst_count) {
                        ahci_rx_error_burst_count = 0;
                        /* Snapshot the fixed-address register before decoding its status or capability fields. */
                        handler_status = RTIFRC1_REG_OFF;
                        if ((uint32_t)(handler_status - ahci_rx_error_window_start_timestamp) < 0x32) {
                            interrupt_cause = ahci_port_command_register;
                            if ((interrupt_cause & 1) == 0) {
                                ahci_port_reset(interrupt_cause >> 1);
                            } else {
                                ahci_stop();
                                handler_status = ahci_port_reset();
                                if (handler_status == 0) {
                                    ahci_start();
                                }
                            }
                            ahci_rx_error_port_reset_count = ahci_rx_error_port_reset_count + 1;
                            ahci_get_TFD_info();
                            core_enqueue_ata_completion_callback(ahci_error_callback);
                        }
                    }
                } else {
                    ahci_rx_error_burst_count = 0;
                    ahci_rx_error_port_reset_count = 0;
                }
            }
        } else {
            /* Program VIM request mask clear zero register with 0x20; write ordering is hardware-significant. */
            vim_request_mask_clear_zero_register = 0x20;
            core_reset_sata_link();
        }
        core_update_masked_register_bits(&ahci_port_phy_control_register, 0x100, 0x100);
        core_update_masked_register_bits(&ahci_port_phy_control_register, 0x100, 0);
    }
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void rti_compare0_isr()

{
    /* Compare0 isr; persistent state is carried in USB core revision, USB debug link state register. */
    int controller_signature;

    if ((usb_core_revision == 0x101a) &&
        ((controller_signature = usb_debug_link_state_register, controller_signature == 0x1081440 || (controller_signature == 0xc81442)))) {
        core_request_system_reset();
    }
    core_advance_software_timers(10);
    usb_hal_recover_stalled_event_interrupt();
    /* Program RTIINTFLAG REG OFF with 1; write ordering is hardware-significant. */
    RTIINTFLAG_REG_OFF = 1;
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void ahci_isr()

{
    /* Isr; persistent state is carried in AHCI global interrupt status register, AHCI port interrupt status register and related record fields. */
    uint32_t host_interrupt_status;
    uint32_t port_interrupt_mask;

    /* Snapshot AHCI global interrupt status register before decoding its status or capability fields. */
    host_interrupt_status = ahci_global_interrupt_status_register;
    if ((host_interrupt_status & 1) != 0) {
        /* Snapshot AHCI port interrupt status register before decoding its status or capability fields. */
        host_interrupt_status = ahci_port_interrupt_status_register;
        port_interrupt_mask = ahci_port_interrupt_enable_register;
        if ((port_interrupt_mask & host_interrupt_status) != 0) {
            /* Program AHCI port interrupt status register with port_interrupt_mask & host_interrupt_status; write ordering is hardware-significant. */
            ahci_port_interrupt_status_register = port_interrupt_mask & host_interrupt_status;
            ahci_port_intr_handler(0);
        }
        /* Program AHCI global interrupt status register with 1; write ordering is hardware-significant. */
        ahci_global_interrupt_status_register = 1;
    }
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void gio_isr()

{
    /* Isr; persistent state is carried in GIO pin callback table. */
    uint32_t enabled_interrupts;
    uint32_t pending_interrupts;
    uint32_t pin_index;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    enabled_interrupts = GIOENASET_REG_OFF;
    pending_interrupts = GIOFLG_REG_OFF;
    pin_index = 0;
    /* Repeat while (int)pin_index < 8; the body advances or polls the state needed to leave the loop. */
    do {
        if (((enabled_interrupts & pending_interrupts & 1 << (pin_index & 0xff)) != 0) &&
            ((firmware_callback_t *)(&gio_pin_callback_table)[pin_index] != (firmware_callback_t *)0x0)) {
            (*(firmware_callback_t *)(&gio_pin_callback_table)[pin_index])();
        }
        pin_index = pin_index + 1;
    } while ((int)pin_index < 8);
    /* Program GIOFLG REG OFF with enabled_interrupts & pending_interrupts; write ordering is hardware-significant. */
    GIOFLG_REG_OFF = enabled_interrupts & pending_interrupts;
    return;
}

/**
 * @brief Usage fault.
 *
 * @return No value.
 */
void _Usage_fault()

{
    /* Usage fault using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief IO PHANTOM INT.
 *
 * @return No value.
 */
void _IO_PHANTOM_INT()

{
    /* IO PHANTOM INT using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief NMI.
 *
 * @return No value.
 */
void _NMI()

{
    /* NMI using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief SYSTick.
 *
 * @return No value.
 */
void _SYSTick()

{
    /* SYSTick using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief MPU.
 *
 * @return No value.
 */
void _MPU()

{
    /* MPU using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief PSR.
 *
 * @return No value.
 */
void _PSR()

{
    /* PSR using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief Hard fault.
 *
 * @return No value.
 */
void _Hard_fault()

{
    /* Hard fault using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief Debug monitor.
 *
 * @return No value.
 */
void _Debug_monitor()

{
    /* Debug monitor using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief SWI.
 *
 * @return No value.
 */
void _SWI()

{
    /* SWI using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief Bus fault.
 *
 * @return No value.
 */
void _Bus_fault()

{
    /* Bus fault using only caller-provided data. */
    /* Intentional terminal loop: execution remains here until a reset or higher-priority exception intervenes. */
    do {
    } while (true);
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void rti_overflow0_isr()

{
    /* Overflow0 isr; persistent state is carried in rti counter zero overflow count. */
    rti_counter_zero_overflow_count = rti_counter_zero_overflow_count + 1;
    /* Program RTIINTFLAG REG OFF with 0x20000; write ordering is hardware-significant. */
    RTIINTFLAG_REG_OFF = 0x20000;
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void rti_overflow1_isr()

{
    /* Overflow1 isr; persistent state is carried in rti counter one overflow count. */
    rti_counter_one_overflow_count = rti_counter_one_overflow_count + 1;
    /* Program RTIINTFLAG REG OFF with 0x40000; write ordering is hardware-significant. */
    RTIINTFLAG_REG_OFF = 0x40000;
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void rti_compare1_isr()

{
    /* Compare1 isr using only caller-provided data. */
    /* Program RTIINTFLAG REG OFF with 2; write ordering is hardware-significant. */
    RTIINTFLAG_REG_OFF = 2;
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void rti_compare2_isr()

{
    /* Compare2 isr using only caller-provided data. */
    /* Program RTIINTFLAG REG OFF with 4; write ordering is hardware-significant. */
    RTIINTFLAG_REG_OFF = 4;
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void mww_error_isr()

{
    /* Error isr using only caller-provided data. */
    uint32_t mww_error_status;

    /* Snapshot the fixed-address register before decoding its status or capability fields. */
    mww_error_status = MWW_STATUS_REG_OFF;
    /* Program MWW STATUS REG OFF with mww_error_status; write ordering is hardware-significant. */
    MWW_STATUS_REG_OFF = mww_error_status;
    return;
}

/**
 * @brief Handle the INTERRUPT interrupt and acknowledge or route pending work.
 *
 * @return No value.
 */
void default_isr()

{
    /* Default isr using only caller-provided data. */
    /* Program SYSECR REG OFF with 0x8000; write ordering is hardware-significant. */
    SYSECR_REG_OFF = 0x8000;
    return;
}
