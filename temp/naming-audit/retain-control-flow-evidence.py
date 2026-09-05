"""Add the reviewed behavior for the retained control-flow symbols."""
import json
from pathlib import Path
p=Path('temp/naming-audit/early-control-flow-renames.json')
rows=json.loads(p.read_text())
evidence={
'core_branch_target_003':'Tests response_code and resets the image-validation context only on a nonzero failure status.',
'core_branch_target_004':'Invokes the validation-context reset helper after selecting the rejection code; the helper preserves the context option byte at offset 0x20.',
'core_branch_target_007':'Resets the validation context and returns the selected nonzero transfer or validation failure status.',
'core_branch_target_011':'Builds mode page 0x33 by copying its two-byte header, publishing notification suppression at byte 3 for current/default values, and exposing mask 1 for changeable values.',
'core_branch_target_012':'Stores the already selected working byte at offset 6 of the current mode page and returns; used by page 0x31 and the page 0x34 changeable mask.',
'core_branch_target_013':'Appends the attribute identifier and advances by two bytes only when the attribute probe returned zero, the success convention shared by all three supported attribute types.',
'core_branch_target_015':'Writes attribute identifier, format byte, and two-byte value length into the five-byte entry header, then advances the response cursor by that header and its value.',
'core_branch_target_017':'Stores the selected attribute command error into the command-status word before the common payload attachment path.',
'core_branch_target_018':'Allocates the four-byte response for the empty attribute list forms and writes the compact response header.',
'core_branch_target_028':'Preserves the record-initialization error when command phase 3 is available; when the phase is absent, it falls through to success instead of publishing that initialization error.',
'core_branch_target_029':'Releases the temporary device-descriptor record only when opening it succeeded, then returns the selected operation status.',
'ahci_branch_target_001':'Sets the expected register-mask value to zero before entering the shared AHCI polling call.',
'ahci_branch_target_002':'Calls ahci_wait_complete with the register, mask, expected value, and attempt limit selected by the incoming initialization case.',
'core_branch_target_039':'Retains cartridge states 2 and 6 while the debounced presence helper is true, and falls through to the absent-state transition when presence clears.',
'core_branch_target_040':'Selects cartridge state 2; the state setter calls core_handle_clear_memory_lookup_ata_command_buffer, which performs ATA device discovery and identity processing.',
'core_branch_target_041':'Publishes the selected cartridge state through its common state-transition handler exactly once and returns.',
'core_branch_target_042':'Selects cartridge state zero and joins the common publication path; state zero invokes the absent-cartridge reset handler.',
'core_branch_target_044':'Selects mechanism state 5, the retry-delay output state, from expired state 3 or the retry branches of states 1 and 6.',
'core_branch_target_045':'Selects terminal mechanism state 8 after the response condition or exhaustion of the bounded retry count.',
'core_branch_target_046':'Selects mechanism state 4 when the input-release helper signals during active output states 1, 2, 3, or 5.',
'core_branch_target_049':'Stores the selected internal command status, then publishes the selected sense key and ASC/ASCQ through core_write_selected_sense_fields.',
'core_branch_target_050':'Publishes sense key 4 with the selected hardware-error ASC/ASCQ and response class, then stores the selected internal command status and returns.',
'core_branch_target_051':'Clears the READ CAPACITY(16) logical-block alignment offset when ATA alignment is absent or lies outside the reported logical-per-physical bounds.',
'core_branch_target_052':'Writes the final selected response field through the big-endian field writer: block size for READ CAPACITY(10), or alignment offset for READ CAPACITY(16).',
}
for row in rows:
 if row['changed']:continue
 old='Existing role name agrees with the target block and its incoming branches; retained after review.'
 row['evidence']=row['evidence'].replace(old,evidence[row['placeholder_name']])
assert sum(not row['changed'] for row in rows)==len(evidence)
p.write_text(json.dumps(rows,indent=2)+'\n')
print(len(evidence))
