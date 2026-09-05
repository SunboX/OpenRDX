/*
 * SPDX-FileCopyrightText: 2026 Andre Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//=======================================================================================
// Filename    : ahci.c
//
// Project     : TUSB9261 RDX Firmware.
//
// Description : Firmware procedures assigned to the ahci module.
//
//=======================================================================================

/*! @file
 * @brief Firmware procedures assigned to the ahci module.
 */

#include "../include/rdx_firmware.h"

/**
 * @brief Advance the AHCI port initialization state machine to completion or error.
 *
 * @param buffer Procedure input.
 * @return Result produced by the procedure.
 */
int ahci_run_port_initialization_state_machine(buffer)
uint32_t *buffer;

{
    /* Run port initialization state machine; persistent state is carried in AHCI port command issue register, AHCI port task file data register and related record fields. */
    uint32_t status_bits;
    uint8_t *data_cursor;
    uint32_t working_result;
    uint32_t expected_status_bits;
    uint32_t poll_attempt_limit;
    int operation_result;
    uint32_t next_dispatch_state;

    status_bits = *buffer;
    operation_result = 0;
    /* Repeat until the following state-machine block reaches its terminal state or reports an error. */
    do {
        if ((status_bits & 0xff) < 0xf) {
            next_dispatch_state = (status_bits & 0xff) + 1;
        } else {
            next_dispatch_state = 0x10;
        }
        data_cursor = (uint8_t *)&ahci_port_command_issue_register;
        /* Dispatch on status_bits; each case preserves the defined state-machine ordering. */
        switch (status_bits) {
        case 0:
            operation_result = ahci_init_port();
            break;
        case 1:
            data_cursor = (uint8_t *)&ahci_port_task_file_data_register;
            poll_attempt_limit = 1;
            working_result = 0x89;
            goto ahci_branch_target_001;
        case 2:
            /* Program VIM request mask set zero register with 0x20; write ordering is hardware-significant. */
            vim_request_mask_set_zero_register = 0x20;
            break;
        case 3:
            working_result = 0xffff;
            expected_status_bits = 0x101;
            poll_attempt_limit = 1;
            data_cursor = (uint8_t *)&ahci_port_signature_register;
            goto ahci_branch_target_002;
        case 4:
            ahci_start();
            core_atapi_device_flag = 0;
            operation_result = core_process_command_opcode_transfer_size_atapi_device_flag(0);
            break;
        case 5:
            operation_result = ahci_wait_complete(&ahci_port_command_issue_register, 1, 0, 10000);
            if (operation_result != 0) {
                return operation_result;
            }
            core_transfer_capacity_word_default_block_size_capacity_block_context(0);
            operation_result = 0;
            break;
        case 6:
            operation_result = 0;
            if ((1 < core_state_179) && (status_bits = core_get_ahci_status_register(0), status_bits < 2)) {
                core_update_masked_register_bits(&ahci_port_command_register, 1, 0);
                ahci_wait_complete(&ahci_port_command_register, 0x8000, 0, 2000);
                ahci_port_reset();
                status_bits = core_get_ahci_status_register(0);
                if (status_bits < 2) {
                    ahci_hba_reset();
                    ahci_global_host_control_register = 2;
                    rti_delay_microseconds(0xfa);
                    ahci_set_port_speed(2);
                    core_update_masked_register_bits(&ahci_port_command_register, 0x10, 0x10);
                    operation_result = 0x11;
                } else {
                    operation_result = 0x10;
                }
            }
            if (operation_result == 0x10) {
                next_dispatch_state = 8;
            } else if (operation_result != 0x11) {
                next_dispatch_state = 9;
                break;
            }
            operation_result = 0;
            status_bits = next_dispatch_state;
            goto ahci_run_port_initialization_state_machine_return_path;
        case 7:
            working_result = 0xf;
            expected_status_bits = 3;
            poll_attempt_limit = 0x32;
            data_cursor = (uint8_t *)&ahci_port_sata_status_register;
            goto ahci_branch_target_002;
        case 8:
            operation_result = ahci_wait_complete(&ahci_port_task_file_data_register, 0x89, 0, 1);
            if (operation_result != 0) {
                return operation_result;
            }
            ahci_start();
            operation_result = 0;
            break;
        case 9:
        case 0xb:
            operation_result = core_process_command_opcode_command(0, 0);
            break;
        case 10:
        case 0xc:
            working_result = 1;
            poll_attempt_limit = 3000;
        /* Shared target for AHCI branch target 001; incoming paths preserve the same state assumptions. */
        ahci_branch_target_001:
            expected_status_bits = 0;
        /* Shared target for AHCI branch target 002; incoming paths preserve the same state assumptions. */
        ahci_branch_target_002:
            operation_result = ahci_wait_complete(data_cursor, working_result, expected_status_bits, poll_attempt_limit);
            break;
        case 0xd:
            if (core_state_180 == '\0') {
                next_dispatch_state = 0xf;
            } else {
                core_configure_command_opcode_response(0, 1);
            }
            break;
        case 0xe:
            operation_result = ahci_wait_complete(&ahci_port_command_issue_register, 1, 0, 3000);
            status_bits = next_dispatch_state;
            if (operation_result != 0) {
                /* Program AHCI port interrupt enable register with 0x40; write ordering is hardware-significant. */
                ahci_port_interrupt_enable_register = 0x40;
                goto switchD_08001bc4_default;
            }
            goto ahci_run_port_initialization_state_machine_return_path;
        case 0xf:
            /* Program AHCI port interrupt enable register with 0x7900005f; write ordering is hardware-significant. */
            ahci_port_interrupt_enable_register = 0x7900005f;
            break;
        default:
        /* Shared target for switchD 08001bc4 default; incoming paths preserve the same state assumptions. */
        switchD_08001bc4_default:
            next_dispatch_state = 0x10;
        }
        status_bits = next_dispatch_state;
        if (operation_result != 0) {
            return operation_result;
        }
    /* Shared target for AHCI run port initialization state machine return path; incoming paths preserve the same state assumptions. */
    ahci_run_port_initialization_state_machine_return_path:
        *buffer = status_bits;
        if (status_bits == 0x10) {
            return operation_result;
        }
    } while (true);
}

/**
 * @brief Build and issue an ATA task-file command, then return its received FIS fields.
 *
 * @param command_buffer Procedure input.
 * @param offset Procedure input.
 * @param length Procedure input.
 * @param response_buffer Procedure input.
 * @return Result produced by the procedure.
 */
bool ahci_execute_taskfile_command(command_buffer, offset, length, response_buffer) uint8_t *command_buffer;
int offset;
int length;
uint8_t *response_buffer;

{
    /* Execute taskfile command; persistent state is carried in AHCI port task file data register, AHCI port command issue register and related record fields. */
    uint32_t ahci_port_task_file_data_register_snapshot;
    uint8_t working_result;
    int operation_status;
    uint16_t poll_timeout;
    uint8_t status_bits;
    volatile uint8_t command_control_byte = 0;
    volatile uint8_t command_byte_ten = 0;
    volatile uint8_t buffer_1_cursor = 0;
    volatile uint8_t command_feature_byte = 0;
    volatile uint8_t command_byte_five = 0;
    volatile uint8_t command_byte_seven = 0;
    volatile uint8_t command_byte_nine = 0;
    volatile uint8_t command_byte_four = 0;
    volatile uint8_t command_byte_six = 0;
    volatile uint8_t command_byte_eight = 0;
    volatile uint8_t command_byte_one = 0;
    volatile uint8_t command_byte_two = 0;
    volatile uint8_t length_component = 0;
    volatile uint8_t reserved_command_byte = 0;
    volatile uint32_t transfer_address = 0;
    volatile uint8_t command_valid_flag = 0;

    ti_memset(&status_bits, 0, 0x30);
    status_bits = 0x27;
    command_byte_seven = command_buffer[7];
    command_feature_byte = command_buffer[3];
    command_control_byte = 0x80;
    command_byte_five = command_buffer[5];
    command_byte_four = command_buffer[4];
    command_byte_six = command_buffer[6];
    command_byte_two = command_buffer[2];
    command_byte_one = command_buffer[1];
    command_byte_eight = command_buffer[8];
    command_byte_nine = command_buffer[9];
    command_byte_ten = command_buffer[10];
    buffer_1_cursor = *command_buffer;
    length_component = length == 0;
    reserved_command_byte = 0;
    transfer_address = (uint32_t)*(uint16_t *)(offset + 4);
    command_valid_flag = 1;
    operation_status = core_submit_command_descriptor_to_device(0, &status_bits, 0);
    if ((operation_status == 0) && (operation_status = core_update_ahci_control_register(0, 0, 0), operation_status == 0)) {
        poll_timeout = 9000;
        if (9000 < *(uint16_t *)(offset + 6)) {
            poll_timeout = *(uint16_t *)(offset + 6);
        }
        operation_status = ahci_wait_complete(&ahci_port_command_issue_register, 1, 0, poll_timeout);
        if ((operation_status == 0) && (ahci_port_task_file_data_register_snapshot = ahci_port_task_file_data_register, (ahci_port_task_file_data_register_snapshot & 1) != 0)) {
            operation_status = 1;
        }
    }
    if (response_buffer != (uint8_t *)0x0) {
        working_result = ata_received_d2h_lba_low;
        response_buffer[3] = working_result;
        working_result = ata_received_d2h_lba_mid;
        response_buffer[5] = working_result;
        working_result = ata_received_d2h_lba_high;
        response_buffer[7] = working_result;
        working_result = ata_received_d2h_lba_low_exp;
        response_buffer[4] = working_result;
        working_result = ata_received_d2h_lba_mid_exp;
        response_buffer[6] = working_result;
        working_result = ata_received_d2h_lba_high_exp;
        response_buffer[8] = working_result;
        working_result = ata_received_d2h_sector_count;
        response_buffer[1] = working_result;
        working_result = ata_received_d2h_sector_count_exp;
        response_buffer[2] = working_result;
        working_result = ata_received_d2h_device;
        response_buffer[9] = working_result;
        working_result = ata_received_d2h_status;
        response_buffer[10] = working_result;
        working_result = ata_received_d2h_error;
        *response_buffer = working_result;
    }
    return operation_status == 0;
}

/**
 * @brief Initialize AHCI port zero and its command-list and received-FIS memory.
 *
 * @return Result produced by the procedure.
 */
int ahci_init_port()

{
    /* Init port; persistent state is carried in AHCI port command register, AHCI port interrupt enable register and related record fields. */
    uint32_t ahci_port_command_register_snapshot;
    int operation_result;

    ahci_port_command_register = 0x40000;
    /* Program AHCI port interrupt enable register with 0; write ordering is hardware-significant. */
    ahci_port_interrupt_enable_register = 0;
    ti_memset(ATA_DEVICES_ADDRESS, 0, 0x1d0);
    ahci_set_port_speed(2);
    ahci_port_command_register_snapshot = ahci_port_command_register;
    if ((ahci_port_command_register_snapshot & 0xc011) == 0) {
        operation_result = 0;
    } else {
        operation_result = ahci_stop();
        if (operation_result == 0) {
            core_update_masked_register_bits(&ahci_port_command_register, 0x10, 0);
            operation_result = ahci_wait_complete(&ahci_port_command_register, 0x4000, 0, 1);
        }
    }
    ti_memset(&ahci_command_list_base, 0, 0x100);
    ti_memset(&ahci_received_fis_buffer, 0, 0x100);
    /* Program AHCI port command list base register with 0xc0010000; write ordering is hardware-significant. */
    ahci_port_command_list_base_register = 0xc0010000;
    ahci_port_command_list_base_upper_register = 0;
    /* Program AHCI port fis base register with 0xc0010100; write ordering is hardware-significant. */
    ahci_port_fis_base_register = 0xc0010100;
    core_get_ahci_control_register(0);
    ahci_port_sata_error_register = 0xffffffff;
    /* Program AHCI port interrupt status register with 0xffffffff; write ordering is hardware-significant. */
    ahci_port_interrupt_status_register = 0xffffffff;
    ahci_global_interrupt_status_register = 1;
    return operation_result;
}

/**
 * @brief Initialize the AHCI subsystem state and required hardware resources.
 *
 * @return Result produced by the procedure.
 */
bool ahci_init()

{
    /* Init; persistent state is carried in AHCI ports implemented register, AHCI initialization retry count and related record fields. */
    uint32_t ahci_ports_implemented_register_snapshot;
    int operation_status;

    ti_memset(&ahci_initialization_retry_count, 0, 0x14);
    core_update_device_label_context(&ahci_device_label_template);
    /* Program AHCI ports implemented register with 1; write ordering is hardware-significant. */
    ahci_ports_implemented_register = 1;
    AHCI_BAR = 0;
    core_update_masked_register_bits(&ahci_diagnostic_register_one, 0x10000, 0x10000);
    core_update_masked_register_bits(&ahci_port_phy_control_register, 0x10, 0x10);
    core_update_masked_register_bits(&ahci_port_phy_control_register, 8, 8);
    core_update_masked_register_bits(&ahci_port_phy_control_register, 7, 2);
    operation_status = ahci_hba_reset();
    if (operation_status == 0) {
        ti_memset(&ahci_command_completion_context, 0, 0x10);
        ahci_global_host_control_register = 2;
        /* Snapshot AHCI ports implemented register before decoding its status or capability fields. */
        ahci_ports_implemented_register_snapshot = ahci_ports_implemented_register;
        if (((ahci_ports_implemented_register_snapshot & 1) != 0) && (operation_status = ahci_init_port(), operation_status == 0)) {
            ahci_initialization_retry_count = ahci_initialization_retry_count + 1;
        }
    }
    return ahci_initialization_retry_count == 0;
}

/**
 * @brief Dispatch the status bits reported by one AHCI port interrupt.
 *
 * @param device_index Procedure input.
 * @param interrupt_status_bits Procedure input.
 * @return Result produced by the procedure.
 */
uint32_t ahci_port_intr_handler(device_index, interrupt_status_bits)
int device_index;
uint32_t interrupt_status_bits;

{
    /* Port intr handler; persistent state is carried in AHCI port SATA error register, core state 185. */
    uint32_t operation_result;

    if ((interrupt_status_bits & 0x78000010) != 0) {
        ahci_fatal_error_recovery(*(uint32_t *)(&ahci_port_sata_error_register + device_index * 0x80));
    }
    if ((interrupt_status_bits >> 1 & 1) != 0) {
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        if (*(char *)(device_index * 0x1d0 + (uint32_t)(uint8_t)(&core_state_185)[device_index * 0x1d0] + 0x800f5f8) == '\0') {
            core_configure_ata_address_ata_queue_context();
        }
    }
    if ((interrupt_status_bits >> 2 & 1) != 0) {
        core_update_ata_address_ata_device_address(interrupt_status_bits >> 3);
    }
    if ((interrupt_status_bits >> 3 & 1) != 0) {
        core_update_ata_address_ata_queue_context(interrupt_status_bits >> 4);
    }
    if ((interrupt_status_bits & 1) != 0) {
        /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
        if (*(char *)(device_index * 0x1d0 + (uint32_t)(uint8_t)(&core_state_185)[device_index * 0x1d0] + 0x800f5f8) == '\0') {
            core_configure_ata_address_ata_configuration_address();
        }
    }
    if ((interrupt_status_bits >> 0x18 & 1) != 0) {
        ahci_stop(interrupt_status_bits >> 0x19);
        ahci_start();
    }
    operation_result = interrupt_status_bits >> 7;
    if ((interrupt_status_bits >> 6 & 1) != 0) {
        *(uint32_t *)(&ahci_port_sata_error_register + device_index * 0x80) = 0x4000000;
        ahci_stop();
        operation_result = ahci_init_port();
    }
    return operation_result;
}

/**
 * @brief Stop, reset, and restart the SATA port after a fatal transfer error.
 *
 * @return No value.
 */
void ahci_fatal_error_recovery()

{
    /* Fatal error recovery; persistent state is carried in AHCI port task file data register, core state 213 and related record fields. */
    uint32_t ahci_port_task_file_data_register_snapshot;
    int core_state_213_snapshot;

    if (core_state_182 == '\0') {
        core_state_213_snapshot = 0;
    } else {
        core_state_213_snapshot = core_state_213;
    }
    ahci_stop();
    ahci_port_task_file_data_register_snapshot = ahci_port_task_file_data_register;
    if ((ahci_port_task_file_data_register_snapshot & 0x88) != 0 || core_state_213_snapshot != 0) {
        ahci_port_reset();
    }
    ahci_start();
    ahci_get_TFD_info();
    if (core_endpoint_context == 3) {
        core_update_ata_queue_address_ata_pending_ata_queue_context(interrupt_state_004);
    }
    return;
}

/**
 * @brief Issue COMRESET and wait for the SATA device-detection state.
 *
 * @return No value.
 */
uint32_t ahci_port_reset()

{
    /* Port reset; persistent state is carried in AHCI port SATA control register, AHCI port SATA status register. */
    int ahci_wait_complete_result;
    uint32_t remaining_count;
    bool condition_met;

    remaining_count = 0;
    /* Repeat while condition_met; the body advances or polls the state needed to leave the loop. */
    do {
        core_update_masked_register_bits(&ahci_port_sata_control_register, 0xf, 1);
        rti_delay_milliseconds(5);
        core_update_masked_register_bits(&ahci_port_sata_control_register, 0xf, 0);
        ahci_wait_complete_result = ahci_wait_complete(&ahci_port_sata_status_register, 0xf, 3, 0x32);
        if (ahci_wait_complete_result != 2) {
            return 0;
        }
        condition_met = remaining_count < 0xc;
        remaining_count = remaining_count + 1;
    } while (condition_met);
    return 0;
}

/**
 * @brief Poll a masked AHCI register value until it matches or times out.
 *
 * @param buffer Procedure input.
 * @param status_mask Procedure input.
 * @param expected_status Procedure input.
 * @param poll_attempts_remaining Maximum number of hardware polls.
 * @return Result produced by the procedure.
 */
uint32_t ahci_wait_complete(buffer, status_mask, expected_status, poll_attempts_remaining)
uint32_t *buffer;
uint32_t status_mask;
uint32_t expected_status;
int poll_attempts_remaining;

{
    /* Wait complete using only caller-provided data. */
    if (0 < poll_attempts_remaining) {
        /* Repeat while poll_attempts_remaining != 0; the body advances or polls the state needed to leave the loop. */
        do {
            /* Gate the following state transition on expected_status == (*buffer & status_mask); the handler is skipped when this condition is false. */
            if (expected_status == (*buffer & status_mask)) {
                return 0;
            }
            rti_delay_microseconds(1000);
            wdt_reset();
            poll_attempts_remaining = poll_attempts_remaining + -1;
        } while (poll_attempts_remaining != 0);
    }
    return 2;
}

/**
 * @brief Program the SATA generation limit and retrain the port.
 *
 * @param context Procedure input.
 * @return No value.
 */
void ahci_set_port_speed(context) int context;

{
    /* Set port speed; persistent state is carried in AHCI port SATA control register. */
    core_update_masked_register_bits(&ahci_port_sata_control_register, 0xff, context << 4 | 1);
    rti_delay_milliseconds(5);
    core_update_masked_register_bits(&ahci_port_sata_control_register, 0xf, 0);
    return;
}

/**
 * @brief Continue port initialization once the task-file register is idle.
 *
 * @return Result produced by the procedure.
 */
bool ahci_continue_port_initialization()

{
    /* Continue port initialization; persistent state is carried in AHCI port task file data register, core command runtime state. */
    uint32_t ahci_port_task_file_data_register_snapshot;
    int ahci_run_port_initialization_state_machine_result;

    ahci_port_task_file_data_register_snapshot = ahci_port_task_file_data_register;
    if ((ahci_port_task_file_data_register_snapshot & 0x89) == 0) {
        ahci_run_port_initialization_state_machine_result = ahci_run_port_initialization_state_machine(&core_command_runtime_state);
    } else {
        ahci_run_port_initialization_state_machine_result = 1;
    }
    return ahci_run_port_initialization_state_machine_result == 0;
}

/**
 * @brief Stop AHCI command processing and wait for the command engine to halt.
 *
 * @return No value.
 */
uint32_t ahci_stop()

{
    /* Stop; persistent state is carried in AHCI port command register. */
    core_update_masked_register_bits(&ahci_port_command_register, 1, 0);
    ahci_wait_complete(&ahci_port_command_register, 0x8000, 0, 2000);
    return 0;
}

/**
 * @brief Copy task-file status and error bytes into the active callback record.
 *
 * @return No value.
 */
void ahci_get_TFD_info()

{
    /* Get TFD info; persistent state is carried in AHCI port task file data register, core ATA queue context. */
    uint32_t ahci_port_task_file_data_register_snapshot;
    uint8_t *core_ata_queue_context_snapshot;
    uint8_t *ahci_status_high_byte;

    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    core_ata_queue_context_snapshot = (uint8_t *)(core_ata_queue_context * 0xc + 0x800f521);
    /* This fixed address selects the fixed packed firmware record; its field offsets are ABI-significant. */
    ahci_status_high_byte = (uint8_t *)(core_ata_queue_context * 0xc + 0x800f522);
    ahci_port_task_file_data_register_snapshot = ahci_port_task_file_data_register;
    if (core_ata_queue_context_snapshot != (uint8_t *)0x0) {
        *core_ata_queue_context_snapshot = (char)ahci_port_task_file_data_register_snapshot;
    }
    if (ahci_status_high_byte != (uint8_t *)0x0) {
        *ahci_status_high_byte = (char)((uint32_t)ahci_port_task_file_data_register_snapshot >> 8);
    }
    return;
}

/**
 * @brief Clear SATA errors, configure DMA, and start AHCI command processing.
 *
 * @return No value.
 */
void ahci_start()

{
    /* Start; persistent state is carried in AHCI port SATA error register, AHCI port dma control register and related record fields. */
    ahci_port_sata_error_register = 0xffffffff;
    /* Program AHCI port dma control register with 0x66; write ordering is hardware-significant. */
    ahci_port_dma_control_register = 0x66;
    core_update_masked_register_bits(&ahci_port_command_register, 1, 1);
    return;
}

/**
 * @brief Return whether the SATA status register reports an active link.
 *
 * @return Result produced by the procedure.
 */
bool ahci_is_link_active()

{
    /* Is link active; persistent state is carried in AHCI port SATA status register. */
    uint32_t ahci_port_sata_status_register_snapshot;

    ahci_port_sata_status_register_snapshot = ahci_port_sata_status_register;
    return (ahci_port_sata_status_register_snapshot & 0xf) == 3;
}

/**
 * @brief Reset the AHCI host bus adapter and wait for reset completion.
 *
 * @return No value.
 */
uint32_t ahci_hba_reset()

{
    /* Hba reset; persistent state is carried in AHCI global host control register. */
    ahci_global_host_control_register = 1;
    ahci_wait_complete(&ahci_global_host_control_register, 1, 0, 1000);
    return 0;
}
