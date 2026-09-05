/*
 * SPDX-FileCopyrightText: 2026 André Fiedler
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/**
 * @brief Service USB reconnect.
 *
 * @return No value.
 */
void core_service_usb_reconnect()

{
    /* Service USB reconnect; persistent state is carried in core reconnect state, core reconnect delay timer. */
    int operation_status;
    uint32_t working_result;
    uint32_t next_state;

    if (core_reconnect_state == 1) {
        operation_status = ((bool (*)())core_is_software_timer_expired)(&core_reconnect_delay_timer);
        if (operation_status == 0) {
            return;
        }
        operation_status = core_is_mww_command_received();
        if (operation_status != 0) {
            core_reconnect_state = 2;
            return;
        }
        next_state = 3;
    } else {
        if (core_reconnect_state != 2) {
            if (core_reconnect_state != 3) {
                if (core_reconnect_state != 4) {
                    return;
                }
                operation_status = ((bool (*)())core_is_software_timer_expired)(&core_reconnect_delay_timer);
                if (operation_status != 0) {
                    core_reconnect_state = 0;
                    core_reconnect_device_and_start_timer();
                    return;
                }
                return;
            }
            operation_status = ((bool (*)())core_is_software_timer_expired)(&core_reconnect_delay_timer);
            if (operation_status == 0) {
                return;
            }
            operation_status = core_is_mww_command_received();
            if (operation_status != 0) {
                core_reconnect_state = 2;
                return;
            }
            usb_hal_disconnect();
            next_state = 4;
            working_result = 1000;
            goto core_reconnect_arm_timer_and_commit_state;
        }
        operation_status = ((bool (*)())core_is_software_timer_expired)(&core_reconnect_delay_timer);
        if (operation_status == 0) {
            return;
        }
        operation_status = core_is_mww_command_received();
        if (operation_status != 0) {
            core_reconnect_state = 2;
            return;
        }
        next_state = 1;
    }
    working_result = 1;
/* Shared target for core reconnect arm timer and commit state; incoming paths preserve the same state assumptions. */
core_reconnect_arm_timer_and_commit_state:
    rti_set_software_timer(&core_reconnect_delay_timer, working_result);
    core_reconnect_state = next_state;
    return;
}

/**
 * @brief Store record field 14 value.
 *
 * @param offset Procedure input.
 * @param value Procedure input.
 * @return No value.
 */
void core_store_record_field_14_value(offset, value) int offset;
uint32_t value;

{
    /* Store record field 14 value using only caller-provided data. */
    *(uint32_t *)(offset + 0x14) = value;
    return;
}
