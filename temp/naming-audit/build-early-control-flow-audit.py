"""Record manually reviewed fixed control-flow names with precise evidence."""
import json
import re
from pathlib import Path

manifest = json.loads(Path('tests/fixtures/firmware_symbol_manifest.json').read_text())['symbols']
renames = {
'core_branch_target_001': ('core_image_validation_select_compatibility_checks', 'Transforms the record compatibility field with (field ^ 0x30) & 0x30; the resulting two bits gate signature and authentication checks.'),
'core_branch_target_002': ('core_image_validation_reject_metadata', 'Sets response_code to 0x2d and resets validation state when the metadata is short, its payload span is below 0xa8, or metadata initialization fails.'),
'core_branch_target_005': ('core_image_validation_dispatch_transfer_state', 'Tests the validation context state: state 1 continues payload processing, and state 2 with finalization enabled and zero source length invokes completion.'),
'core_branch_target_006': ('core_image_validation_transfer_payload', 'Initializes transfer status, checks the continuation offset, bounds the payload against remaining bytes, writes flash data, and advances CRC/hash state.'),
'core_branch_target_008': ('core_mode_sense_include_caching_page_length', 'Adds 0x14 bytes for caching mode page 8, then selects page 0x33 as well only for the all-pages request 0x3f.'),
'core_branch_target_009': ('core_mode_sense_select_page_33_length', 'Selects core_mode_page_33_length for direct page 0x33 requests and after the caching-page length for an all-pages request.'),
'core_branch_target_010': ('core_mode_sense_check_response_length', 'Returns when the accumulated response length is zero; otherwise allocates and builds the MODE SENSE response.'),
'core_branch_target_014': ('core_attribute_values_open_source_record', 'Opens the runtime source record for the attribute-values response after validating the requested attribute identifier.'),
'core_branch_target_016': ('core_attribute_response_store_lengths_and_release_record', 'Stores the final response length twice, writes the four-byte attribute data length, and releases the temporary source record.'),
'core_branch_target_019': ('core_attribute_response_attach_payload', 'Calls the response-payload attachment helper using the selected response record and allocation-length CDB bytes 10 through 13.'),
'core_branch_target_020': ('core_multiword_division_copy_remainder', 'Copies the dividend to the remainder buffer when it has fewer significant limbs than the divisor, or equal limb count but a smaller value. The quotient has already been cleared.'),
'core_branch_target_021': ('core_multiword_division_correct_quotient_estimate', 'Checks the estimated quotient limb against the next divisor and dividend limbs; decrements the estimate and adds the high divisor limb to the trial remainder when the estimate is too large.'),
'core_branch_target_022': ('core_attribute_update_append_entry_metadata', 'Enforces the three-entry bound, copies six bytes of validated attribute metadata into the selected slot, and increments the entry count.'),
'core_branch_target_023': ('core_attribute_update_reject_entry', 'Stores command status 10 for a short entry header, unsupported or nonascending identifier, invalid length, or more than three selected entries.'),
'core_branch_target_024': ('core_attribute_update_apply_entries', 'Opens the runtime source record and applies the collected attribute entries before releasing it and finalizing the command.'),
'core_branch_target_025': ('core_attribute_update_finalize_response', 'Invokes the common command-response finalization hook and returns after attribute processing or a format error.'),
'core_branch_target_026': ('core_attribute_update_store_validation_error', 'Stores the selected command status and returns before attribute parsing for an excessive parameter length or failure to acquire the command payload.'),
'core_branch_target_027': ('core_descriptor_response_check_source_status', 'Checks whether the source-record initialization succeeded; failed initialization is routed to the existing error-preservation branch, while a usable source yields success.'),
'core_branch_target_030': ('core_management_record_return_dispatch_result', 'Returns the original value argument in the high word and the selected dispatch status in the low word with CONCAT44.'),
'core_branch_target_031': ('core_ata_transfer_submit_command_descriptor', 'Clears reserved FIS and command-descriptor fields, stores the requested transfer byte count, submits the ATA command descriptor, and maps its result.'),
'core_branch_target_032': ('core_mode_select_read_parameter_length', 'Reads parameter-list length from CDB byte 4 for MODE SELECT(6), or bytes 7 through 8 for MODE SELECT(10).'),
'core_branch_target_033': ('core_mode_select_choose_page_31_flags', 'Examines incoming page 0x31 bit 3 and chooses either the existing flags with bit 3 cleared or the shared branch that sets bit 3; reached again after consuming the page bit-4 request.'),
'core_branch_target_034': ('core_mode_select_set_page_31_bit_three', 'Sets bit 3 in the existing page 0x31 state before merging bit 4 and publishing the byte.'),
'core_branch_target_035': ('core_mode_select_reject_page_parameters', 'Selects command status 10 for unsupported mode pages or invalid page 0x31/0x33 fields and branches to the common page-status store.'),
'core_branch_target_036': ('core_mode_select_store_page_status', 'Stores the selected status in the command buffer, shared by accepted caching-page parameters and rejected mode-page parameters.'),
'core_branch_target_037': ('core_mode_select_finalize_response', 'Calls the common response-finalization hook after mode-page handling, including the path where the page-format CDB bit is clear.'),
'core_branch_target_038': ('core_mode_select_store_validation_error', 'Stores the selected early validation error and immediately returns when parameter length exceeds capacity or payload acquisition fails.'),
'core_branch_target_043': ('core_ata_discovery_mark_failure', 'If no successful discovery path set condition_met, marks ATA discovery failed and schedules command phase 1.'),
'core_branch_target_047': ('core_mechanism_update_input_debounce', 'Samples the mechanism input edge, starts or polls its 100 ms debounce timer, and records the latest raw level after mechanism-state handling.'),
'core_branch_target_048': ('core_sense_status_select_failure_code_five', 'Selects internal command status 5 for two runtime sense paths; preserves the already selected sense key and ASC/ASCQ for the shared publication branch.'),
'core_branch_target_053': ('core_identity_import_commit_fields', 'Calls the identity-update commit helper after importing identity fields and returns its result.'),
'ahci_branch_target_003': ('ahci_port_init_commit_next_state', 'Stores the selected next initialization state through the caller pointer, returns only for terminal state 0x10, and otherwise continues the state-machine loop.'),
'usb_branch_target_001': ('usb_init_apply_revision_configuration', 'Applies USB3 PIPE, USB2 PHY, and global control changes gated by core revision, then initializes endpoint transfer state and marks the stack ready.'),
'usb_branch_target_002': ('usb_link_update_usb2_phy_state', 'Dispatches the USB2 link-state path and changes USB2 PHY configuration bit 0x40 according to the selected link state; the older controller device state 3 uses the alternate state branch.'),
'usb_branch_target_003': ('usb_link_disable_dynamic_clock_gating', 'Invokes the helper that writes CDDIS=0x38. TI system.h defines 0x08, 0x10, and 0x20 as dynamic clock-gating disable bits for M3 RAM/ROM, BMM2, and AHB-to-VBUS.'),
'usb_branch_target_004': ('usb_link_dispatch_event_state', 'Dispatches selected link-state values 2, 3, 4, and 8; state 3 emits the link callback and state 8 can complete the pending generic command.'),
'usb_branch_target_005': ('usb_link_enable_sleep_clock_gating', 'Calls the revision-gated helper writing CDDIS=3. TI system.h defines these bits as GCLK/HCLK sleep gating and its system_enable_clock_gating helper uses the same revision threshold.'),
'mww_branch_target_001': ('mww_transfer_check_buffered_read_limit', 'Rejects an input request larger than the 0x1014-byte local response buffer by stalling endpoint 0x83 and building command status; otherwise dispatches only when the buffered operation is ready.'),
'mww_branch_target_002': ('mww_transfer_dispatch_saved_command', 'Calls the saved-command dispatcher after the transfer mode and any buffered payload have been prepared, then returns.'),
'mww_branch_target_003': ('mww_transfer_initialize_write_window', 'Initializes the write-only wrap window, dispatches the command, and submits endpoint 3 from MWW1_ADDR only when dispatch permits a nonzero transfer.'),
'mww_branch_target_004': ('mww_transfer_submit_selected_endpoint', 'Submits the already selected endpoint/address pair and transfer length; incoming read and write paths choose 0x83/MWW0_ADDR or 3/MWW1_ADDR.'),
'mww_branch_target_005': ('mww_phase_select_bulk_in_stall', 'Selects endpoint 0x83 for phase-error paths 2, 3, 7, and 8, then calls the endpoint-stall helper. TI usb_hal.h defines endpoint command 4 used by that helper as SET_STALL.'),
'mww_branch_target_006': ('mww_phase_send_command_status', 'Invokes the command-status wrapper builder with the selected status: zero for phase paths 4/5 or two after other stalled phase-error paths.'),
'mww_branch_target_007': ('mww_command_status_store_residue', 'Stores the remaining transfer byte count in CSW word 2; the wrapper has signature 0x53425355 and status byte 12. Phase 9 uses the whole requested length, while phases 5/11 subtract transferred bytes.'),
'mww_branch_target_008': ('mww_command_status_clear_transfer_context', 'Clears the two transfer-context flags after command-status submission or when the saved request already denotes a completed status wrapper.'),
'spi_branch_target_001': ('spi_output_apply_polarity_adjusted_level', 'Uses the physical pin mapping to write the polarity-adjusted logical output to GIO, SCI, SPI, or PWM; the incoming branches have already selected the physical level.'),
'spi_branch_target_002': ('spi_output_initialize_inactive_level', 'Initializes the configured logical output to zero after any required GPIO direction and function setup.'),
'sci_branch_target_001': ('sci_input_apply_configured_polarity', 'Compares the sampled physical input level with the configured active polarity, returning logical one when they match and zero otherwise.'),
}
code_paths = [p for p in Path('src').rglob('*') if p.suffix in {'.c', '.inc'}]
code = {p:p.read_text().splitlines() for p in code_paths}
sibling_paths = list(Path('../TUSB9261_RDX_Ghidra_Project/Reconstructed_Source/functions').glob('*.c'))
sibling = {p:p.read_text() for p in sibling_paths}
results = []
for sym in manifest:
    ident = sym['placeholder_name']
    if sym['category'] != 'branch_target' or (ident.startswith('core_') and int(ident[-3:]) > 53):
        continue
    old = sym['semantic_name']
    target = renames.get(ident)
    new, explanation = target if target else (old, 'Existing role name agrees with the target block and its incoming branches; retained after review.')
    locations = []
    for p, lines in code.items():
        for idx, line in enumerate(lines):
            if re.match(r'\s*'+re.escape(old)+r':', line):
                function = ''
                for previous in reversed(lines[:idx]):
                    match = re.match(r'^(?:[A-Za-z_][A-Za-z_0-9]*\s+|\*\s*)+([A-Za-z_][A-Za-z_0-9]*)\(', previous)
                    if match:
                        function = match.group(1)
                        break
                locations.append(f'{p}:{idx+1}; {function}')
    address_label = 'LAB_'+sym['address'][2:].lower()
    sibling_evidence = []
    for p, data in sibling.items():
        if address_label in data:
            line_num = next(n for n,line in enumerate(data.splitlines(),1) if address_label+':' in line)
            sibling_evidence.append(f'{p}:{line_num}')
    evidence = '; '.join(locations)+'. '+explanation
    if sibling_evidence:
        evidence += ' Matching address block: '+ '; '.join(sibling_evidence)+'.'
    results.append(dict(old=old,new=new,address=sym['address'],evidence=evidence,confidence='structural',placeholder_name=ident,changed=old!=new))
assert len(results)==72, len(results)
assert all(x['evidence'].startswith('src/') for x in results)
assert len({x['new'] for x in results}) == len(results)
Path('temp/naming-audit/early-control-flow-renames.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps({'audited':len(results),'renamed':sum(x['changed'] for x in results),'retained':sum(not x['changed'] for x in results),'path':'temp/naming-audit/early-control-flow-renames.json'},indent=2))
