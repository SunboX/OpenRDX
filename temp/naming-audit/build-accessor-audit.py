"""Produce body-checked accessor/helper rename proposals."""
import json
import re
from pathlib import Path
base=Path('temp/naming-audit')
candidates={r['name']:r for r in json.loads((base/'accessor-candidates.json').read_text())}
choices={
'core_update_persistent_state_status':('core_update_descriptor_response_metadata','Copies descriptor response data and stores the two returned status bytes; no persistent-storage write occurs.'),
'core_update_flash_completion_context':('core_commit_firmware_image_header','Programs the four-byte firmware image commit word at flash offset zero; clears ready/validation-active flags only after the write succeeds.'),
'core_update_sense_record_context':('core_is_mechanism_transition_pending','Returns zero only when completion is uninhibited, the mechanism transition timer expired, and the output state is terminal state 4 or 8.'),
'core_register_usb_event_callback':('core_register_usb_power_management_callback','Registers up to four callbacks in the USB_STACK_FXN power-management callback table and increments its count.'),
'core_process_command_processing_context':('core_is_cartridge_startup_delay_active','Returns one only for identity variant 0x38 while the five-second cartridge startup timer has not expired.'),
'core_process_input_pin_state':('core_hold_logical_output_fourteen','Sets logical output 14, allocates its timer and arms a 30000 ms hold interval.'),
'core_update_default_mww_transfer_value':('core_block_mww_dispatch_and_claim_device_command','Blocks MWW dispatch and, when the device-command busy flag is clear, sets that flag before clearing the request byte.'),
'core_update_command_validation_context':('core_get_active_command_mode_page_buffer','Returns the mode-page buffer pointer and the active command descriptor length to the caller.'),
'core_process_firmware_transfer_resource':('core_abort_failed_usb_control_transfer','Only in EP0 error state 5, ends both endpoint-zero directions and stalls the OUT endpoint.'),
'core_update_flash_validation_status':('core_set_thermal_pwm_duty','Unless thermal PWM is in disabled state 8, applies the requested percentage to PWM channel 15 and records the duty target.'),
'core_process_clear_memory_spi_request_context':('core_initialize_led_controller_registry','Clears the two-entry LED-controller table and its count, then allocates the controller-service timer.'),
'core_process_clear_memory_transfer_buffer_context':('core_initialize_usb_reconnect_state','Resets device-command arbitration, clears the reconnect timer and disconnect-request byte, and allocates the reconnect timer.'),
'core_dispatch_usb_event_callbacks':('core_dispatch_usb_power_management_callbacks','Walks the four USB_STACK_FXN power-management callback entries after the cursor origin and calls each non-null entry with the supplied event.'),
'core_process_mode_processing_context':('core_request_ata_standby_when_idle','Requests an MWW dispatch pause; records a pending ATA standby request while busy, or immediately invokes the standby handler when idle.'),
'core_process_device_status_context':('core_apply_thermal_pwm_override','Without an override selects thermal PWM control phase 1; with an override stops PWM and selects state 7 unless already disabled.'),
'core_transfer_bounded_transfer_context':('core_permute_word_cipher_key_bytes','Copies sixteen source bytes using the fixed key-byte permutation table.'),
'core_get_flash_validation_context_for_validate_structured_flash':('core_get_identity_variant_code','Returns the identity-record variant halfword, normalizing 0,1 and 0xFFFF to default variant 0x37.'),
'core_update_pin_update_context':('core_check_usb_reconnect_timer','Returns true when no USB disconnect is requested and the delegated software-timer check reports expiry.'),
'core_update_mww_transfer_context_for_mww_process_resource':('core_release_mww_device_command','Releases device-command ownership when the MWW command-active flag is set, then clears MWW active/received request bytes.'),
'core_update_firmware_record_update':('core_register_usb_bot_transfer_callback','Selects the BOT OUT or IN callback table by endpoint direction and stores the callback for endpoint number minus two.'),
'core_update_sense_update_context':('core_set_mechanism_output_selection','Stores the requested mechanism output selection when the response-type condition permits it.'),
'core_process_command_runtime_state':('core_restart_ahci_port_initialization','Sets AHCI port initialization state to zero, initializes the port and resets its SATA link.'),
'core_update_mechanism_update_context_for_primary_sense_context':('core_extend_pending_cartridge_eject_wait','Rearms the five-second cartridge-eject wait timer only while an eject request is pending.'),
'core_update_persistent_update_context':('core_reset_smart_temperature_monitor','Sets temperature to the unavailable sentinel 0xFF and clears monitoring-active and hot-sample retry state.'),
'core_update_clear_memory_mcp3008_sample':('core_clear_firmware_image_validation','Clears the firmware-image validation-active flag and its 0x198-byte validation context.'),
'core_update_thermal_monitor_enabled_for_clear_memory_resource':('core_reset_device_command_arbitration','Clears device-command busy, MWW blocked/deferred/active/received flags and the accompanying update flag.'),
'core_update_spi_transfer_context':('core_resume_deferred_mww_dispatch','When dispatch is deferred, clears deferred and blocked flags before invoking the MWW command dispatcher.'),
'core_update_response_update_context':('core_arm_standby_activity_check','Arms the standby activity check timer for 300000 milliseconds.'),
'core_process_media_processing_context':('core_initialize_smart_temperature_timer','Allocates the SMART-temperature sample timer and resets the temperature monitor.'),
'core_get_mww_transfer_context':('core_request_mww_dispatch_pause','Sets the MWW dispatch-blocked flag and reports whether the device-command owner is idle.'),
'core_update_notification_context':('core_load_persistent_notification_mode','Loads the notification mode from the persistent-record getter.'),
'core_update_clear_memory_response_end_context':('core_clear_ata_discovery_flags','Clears the six-byte cartridge discovery/state prefix beginning at the ATA-device-discovered flag.'),
'core_update_mechanism_update_context_for_advance_mechanism_state':('core_mark_cartridge_eject_pending','Sets the cartridge-eject pending flag when the caller supplies a nonzero condition.'),
'core_set_mechanism_phase_to_zero':('core_set_thermal_pwm_control_phase_zero','Writes zero to the thermal PWM control phase.'),
'core_set_mechanism_phase_to_two':('core_set_thermal_pwm_control_phase_two','Writes two to the thermal PWM control phase.'),
'core_set_mechanism_phase_to_one':('core_set_thermal_pwm_control_phase_one','Writes one to the thermal PWM control phase.'),
'core_update_identity_persistence_context':('core_increment_persistent_mechanism_completion_count','Increments the persistent mechanism-completion count and saves the checksummed state record.'),
'core_update_spi_transfer_context_for_mww_handle_transfer':('core_defer_mww_dispatch_and_mask_usb_interrupt','Marks MWW dispatch deferred and invokes the USB-interrupt mask helper.'),
'core_set_cartridge_input_value_to_one':('core_mark_mww_command_received','Sets the MWW command-received flag.'),
'core_update_mww_transfer_context_for_mww_handle_transfer':('core_claim_device_command_for_mww','Sets the MWW device-command active flag and marks the device-command owner busy.'),
'core_get_ata_command_buffer':('core_get_cartridge_ata_device_info','Returns the address of the cached cartridge ATA device information, which contains capacity, sector size, LBA48 and alignment fields.'),
'core_get_gpio_pin_context_value':('core_is_ata_standby_active','Returns the ATA-standby active flag.'),
'core_set_command_runtime_state_to_zero':('core_reset_ahci_port_initialization_state','Writes zero to AHCI port initialization state.'),
'core_get_mcp3008_sample_value':('core_get_firmware_image_validation_active','Returns the firmware-image validation-active flag; no ADC sample is accessed.'),
'core_get_mww_transfer_value':('core_is_mww_dispatch_blocked','Returns the MWW dispatch-blocked flag.'),
'core_set_thermal_monitor_enabled_to_zero':('core_release_device_command','Clears the device-command busy flag.'),
'core_get_cartridge_input_value':('core_get_mww_command_received','Returns the MWW command-received flag; no cartridge input is sampled.'),
'core_set_thermal_monitor_enabled_to_one':('core_claim_device_command','Sets the device-command busy flag.'),
'core_get_notification_context_value':('core_get_notification_mode','Returns the notification mode.'),
'core_get_command_context_value':('core_is_command_write_blocked','Returns the command write-blocked flag.'),
'core_update_secondary_sense_field_context':('core_set_selected_sense_asc_ascq','Stores the supplied ASC/ASCQ halfword in the selected sense fields.'),
'core_update_response_buffer_context':('core_set_selected_sense_status','Stores the supplied selected sense status byte.'),
'core_update_primary_sense_field_context':('core_set_selected_sense_key','Stores the supplied selected sense key byte.'),
'core_update_command_phase_auxiliary':('core_set_command_sense_table_index','Stores the selected command sense-table index.'),
'core_update_firmware_context_primary_firmware_callback':('core_register_usb_bot_reset_callback','Stores the BOT reset callback function pointer.'),
'core_update_firmware_context_secondary_firmware_callback':('core_register_usb_ep0_out_transfer_callback','Stores the EP0 OUT data-transfer callback function pointer.'),
'core_get_response_end_context_pointer':('core_get_ata_discovery_flags','Returns the base pointer to the cartridge ATA discovery/state prefix.'),
'core_update_thermal_monitor_enabled_for_read_smart_temperature':('core_clear_request_and_release_device_command','Clears the caller request word and the device-command busy flag.'),
'core_forward_mww_transfer_value':('core_pause_mww_dispatch_for_standby','Sets MWW dispatch blocked and reports device-command idleness; callers use the result to schedule ATA standby.'),
'core_forward_mcp3008_sample_value':('core_is_firmware_image_validation_active','Returns the firmware-image validation-active flag, queried by cartridge monitoring and response handling.'),
'core_forward_cartridge_input_value':('core_is_mww_command_received','Returns the MWW command-received flag, queried by reconnect and cartridge management state machines.'),
}
existing=set()
for p in list(Path('src').rglob('*.c'))+list(Path('src').rglob('*.inc'))+list(Path('include').rglob('*.h')):
 existing.update(re.findall(r'\b[A-Za-z_]\w*(?=\s*\()',p.read_text()))
done=set();proposed=set()
for n in ['function-renames.json','late-function-renames.json']:
 for r in json.loads((base/n).read_text()):done.add(r['old']);proposed.add(r['new'])
rows=[]
for old,(new,reason) in choices.items():
 if old in done:continue
 assert old in candidates,old
 assert new not in existing and new not in proposed,new
 proposed.add(new);c=candidates[old]
 mappings='; '.join(a+' -> '+b for a,b in c['globals'].items())
 rows.append({'old':old,'new':new,'evidence':[f"{c['path']}:{c['line']}:{old}: {reason}",mappings]})
(base/'accessor-renames.json').write_text(json.dumps(rows,indent=2)+'\n')
print('accessor proposals',len(rows));print('duplicates',len(rows)-len({r['new'] for r in rows}))
