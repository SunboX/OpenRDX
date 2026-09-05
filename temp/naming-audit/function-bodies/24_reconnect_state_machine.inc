
void core_forward_reconnect_state()
{
    int operation_status;
    uint32_t working_result;
    uint32_t next_state;
    if (core_reconnect_state == 1) {
        operation_status = ((bool (*)())core_get_thermal_control_context)(&core_reconnect_delay_timer);
        if (operation_status == 0) {
            return;
        }
        operation_status = core_forward_cartridge_input_value();
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
                operation_status = ((bool (*)())core_get_thermal_control_context)(&core_reconnect_delay_timer);
                if (operation_status != 0) {
                    core_reconnect_state = 0;
                    core_reconnect_device_and_start_timer();
                    return;
                }
                return;
            }
            operation_status = ((bool (*)())core_get_thermal_control_context)(&core_reconnect_delay_timer);
            if (operation_status == 0) {
                return;
            }
            operation_status = core_forward_cartridge_input_value();
            if (operation_status != 0) {
                core_reconnect_state = 2;
                return;
            }
            usb_hal_disconnect();
            next_state = 4;
            working_result = 1000;
            goto core_forward_reconnect_state_before_rti_process_thermal_timer_state;
        }
        operation_status = ((bool (*)())core_get_thermal_control_context)(&core_reconnect_delay_timer);
        if (operation_status == 0) {
            return;
        }
        operation_status = core_forward_cartridge_input_value();
        if (operation_status != 0) {
            core_reconnect_state = 2;
            return;
        }
        next_state = 1;
    }
    working_result = 1;
core_forward_reconnect_state_before_rti_process_thermal_timer_state:
    rti_process_thermal_timer_state(&core_reconnect_delay_timer, working_result);
    core_reconnect_state = next_state;
    return;
}
void core_store_record_field_14_value(offset, value) int offset;
uint32_t value;
{
    *(uint32_t *)(offset + 0x14) = value;
    return;
}
