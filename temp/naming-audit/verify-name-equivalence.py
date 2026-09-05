"""Compare firmware token streams against the checked pre-audit commit."""
from pathlib import Path
import collections,difflib,importlib.util,json,re,subprocess
spec=importlib.util.spec_from_file_location('names','temp/naming-audit/apply-names.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
rename_map=json.loads(Path('temp/naming-audit/applied-global-map.json').read_text())
production_map={
 'RDX_MECHANISM_STATE2_TIME_MS':'RDX_MECHANISM_NO_MEDIA_GRACE_MS',
 'RDX_MECHANISM_MAX_REVERSE_EXPIRIES':'RDX_MECHANISM_MAX_RETURN_TIMEOUTS',
 'reverse_expiries':'return_drive_timeouts',
 'RDX_MECHANISM_STATE_1':'RDX_MECHANISM_LOW_DRIVE_CHECK_MEDIA',
 'RDX_MECHANISM_STATE_2':'RDX_MECHANISM_NO_MEDIA_GRACE',
 'RDX_MECHANISM_STATE_3':'RDX_MECHANISM_LOW_DRIVE_WAIT_ENDPOINT',
 'RDX_MECHANISM_STATE_4':'RDX_MECHANISM_SUCCESS_SETTLE',
 'RDX_MECHANISM_STATE_5':'RDX_MECHANISM_RETURN_PAUSE',
 'RDX_MECHANISM_STATE_6':'RDX_MECHANISM_HIGH_DRIVE_WAIT_ENDPOINT',
 'RDX_MECHANISM_STATE_7':'RDX_MECHANISM_RETURN_SETTLE',
 'RDX_MECHANISM_STATE_8':'RDX_MECHANISM_RETRIES_EXHAUSTED',
}
locals_by_file=collections.defaultdict(dict)
for r in json.loads(Path('temp/naming-audit/local-renames.json').read_text()):
 locals_by_file[r['file']].setdefault(r['function'],{})[r['old']]=r['new']
pattern=re.compile('|'.join(re.escape(k) for k in sorted(rename_map,key=len,reverse=True)))
files=subprocess.check_output(['git','ls-files'],text=True).splitlines()
failures=[];count=0
for filename in files:
 p=Path(filename)
 if p.parts[0] not in {'src','include','linker'} or p.suffix not in {'.c','.h','.inc','.asm','.cmd'}:continue
 baseline=subprocess.check_output(['git','show','HEAD:'+filename]).decode().replace('\r\n','\n')
 for function,local_map in locals_by_file.get(filename,{}).items():
  start,end=mod.function_bounds(baseline,function)
  baseline=baseline[:start]+mod.TOKEN.sub(lambda m:local_map.get(m.group(),m.group()),baseline[start:end])+baseline[end:]
 if 'rdx_mount' in p.parts:
  baseline=mod.TOKEN.sub(lambda m:production_map.get(m.group(),m.group()),baseline)
 else:baseline=mod.TOKEN.sub(lambda m:pattern.sub(lambda n:rename_map[n.group()],m.group()),baseline)
 baseline=baseline.replace('02_management_descriptors.inc','02_multiword_division.inc')
 baseline=baseline.replace('ti_vendor_request_callback() - No matching request.', 'vendor_request_callback() - No matching request.').replace('-> vendor_init()', '-> usb_vendor_init()').replace('ums_uas_handle_task_mgmt_IU()', 'ums_uas_handle_task_mgmt_iu()').replace('ums_uas_handle_command_IU()', 'ums_uas_handle_command_iu()')
 current_path=Path(filename.replace('02_management_descriptors.inc','02_multiword_division.inc'))
 current=current_path.read_text()
 if p.suffix=='.asm':
  baseline=re.sub(r';[^\n]*','',baseline);current=re.sub(r';[^\n]*','',current)
 old_tokens=mod.source_tokens(baseline);new_tokens=mod.source_tokens(current)
 if old_tokens!=new_tokens:
  diffs=[]
  for tag,a,b,c,d in difflib.SequenceMatcher(None,old_tokens,new_tokens,autojunk=False).get_opcodes():
   if tag!='equal':diffs.append({'expected':old_tokens[a:b],'actual':new_tokens[c:d]})
  failures.append({'file':filename,'differences':diffs})
 count+=1
result={'files_checked':count,'differences':failures}
Path('temp/naming-audit/final-token-proof.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
raise SystemExit(bool(failures))
