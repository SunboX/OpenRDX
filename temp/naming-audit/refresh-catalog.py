"""Refresh catalog references and source-layout checks after the naming audit."""
from pathlib import Path
import json,re,hashlib,subprocess
ROOT=Path.cwd()
AUDIT=ROOT/'temp/naming-audit'


def main():
    """Update source references without changing any address or ABI type."""
    final_map={
        'core_get_active_command_mode_page_buffer':'core_get_active_command_response_buffer',
        'core_get_mww_mode_page_buffer':'core_select_scsi_response_buffer',
    }
    paths=[*Path('src').rglob('*'),*Path('include').rglob('*'),*Path('linker').rglob('*')]
    for p in paths:
        if not p.is_file() or p.suffix not in {'.c','.h','.inc','.cmd'}: continue
        old=p.read_bytes();text=old.decode()
        for source,target in final_map.items(): text=text.replace(source,target)
        text=text.replace('Get active command mode page buffer','Get active command response buffer')
        text=text.replace('Get mww mode page buffer','Select SCSI response buffer')
        new=text.encode()
        if new!=old:p.write_bytes(new)
    mapping=json.loads((AUDIT/'applied-global-map.json').read_text())
    mapping={k:final_map.get(v,v) for k,v in mapping.items()}
    mapping['core_get_mww_mode_page_buffer']='core_select_scsi_response_buffer'
    (AUDIT/'applied-global-map.json').write_text(json.dumps(mapping,indent=2)+'\n')
    p=Path('tests/fixtures/firmware_symbol_manifest.json');manifest=json.loads(p.read_text())
    entries=manifest['symbols'];actual={r['semantic_name']:[] for r in entries}
    functions={}
    for source in sorted([*Path('src').rglob('*.c'),*Path('src').rglob('*.inc')]):
        for n,line in enumerate(source.read_text().splitlines(),1):
            match=re.match(r'^\w[^\n]*\b((?:core|ahci|usb|mww|spi|rti|gio|sci|interrupt)_\w+)\(',line)
            if match:functions[match.group(1)]=f'{source}:{n}'
            for name in set(re.findall(r'\b[A-Za-z_]\w*\b',line)):
                if name in actual: actual[name].append(f'{source}:{n}')
    for entry in entries:
        name=entry['semantic_name'];entry['uses']=sorted(actual[name])
        for i,v in enumerate(entry['verification']):
            v=v.replace('02_management_descriptors.inc','02_multiword_division.inc')
            for old,new in final_map.items():v=v.replace(old,new)
            if re.search(r'src/rdx_core\.c:\d+',v):
                referred=[functions[token] for token in re.findall(r'\bcore_\w+\b',v) if token in functions]
                target=referred[0] if referred else (entry['uses'][0] if entry['uses'] else 'include/firmware_symbols.h')
                v=re.sub(r'src/rdx_core\.c:\d+',target,v)
            entry['verification'][i]=v
    p.write_text(json.dumps(manifest,indent=2)+'\n')
    test=Path('tests/test_rdx_core_fragments.py')
    source=test.read_text();header=re.compile(rb'\A/\*\n \* SPDX-FileCopyrightText:[^\n]+\n \*\n \* SPDX-License-Identifier:[^\n]+\n \*/\n\n')
    combined=b''.join(header.sub(b'',p.read_bytes().replace(b'\r\n',b'\n'),count=1) for p in sorted(Path('src/rdx_core').glob('*.inc')))
    digest=hashlib.sha256(combined).hexdigest()
    source=re.sub(r'(EXPECTED_IMPLEMENTATION_SHA256 = \(\n    ")[a-f0-9]+',lambda m:m.group(1)+digest,source)
    test.write_text(source)
    print('Catalog refreshed:',len(entries),'bindings;',sum(len(r['uses']) for r in entries),'source locations;',len(mapping),'global names.')
    print('Core implementation SHA256:',digest)

if __name__=='__main__':main()
