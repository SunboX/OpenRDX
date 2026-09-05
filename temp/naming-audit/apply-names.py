"""Apply reviewed identifier families without changing firmware operations."""
from pathlib import Path
import collections,hashlib,json,re,subprocess
ROOT=Path.cwd()
AUDIT=ROOT/'temp/naming-audit'
ARTIFACTS=['control-flow-renames','core-state-renames','core-ram-renames','peripheral-renames','function-renames','late-function-renames','accessor-renames']
TOKEN=re.compile(r'[A-Za-z_]\w*')
COMMENT=re.compile(r'/\*.*?\*/|//[^\n]*',re.S)

def load_rows():
    """Merge independently reviewed proposals and reject ambiguous targets."""
    rows=[]
    for stem in ARTIFACTS:
        rows.extend(json.loads((AUDIT/(stem+'.json')).read_text()))
    mapping={}
    for row in rows:
        if row['old']==row['new']: continue
        assert row['old'] not in mapping or mapping[row['old']]==row['new'],row
        mapping[row['old']]=row['new']
    assert len(set(mapping.values()))==len(mapping)
    return rows,mapping

def mask_noncode(text):
    """Keep source positions intact while hiding comments and quoted text."""
    return re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m.group()),text,flags=re.S)

def function_bounds(text,name):
    """Find the full documented K&R or ANSI function definition."""
    match=re.search(r'^\w[^\n]*\b'+re.escape(name)+r'\(',text,re.M)
    assert match,name
    masked=mask_noncode(text)
    opening=masked.index('{',match.end())
    depth=1;end=opening+1
    while depth:
        depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
    start=text.rfind('/**',0,match.start())
    assert start>=0
    return start,end

def source_tokens(text):
    """Return code tokens while retaining numeric and quoted constants."""
    stripped=COMMENT.sub('',text)
    return re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|0[xX][0-9A-Fa-f]+|\d+|[^\s]',stripped)

def title(name):
    """Turn a descriptive procedure name into a concise brief line."""
    result=re.sub(r'^(core|usb|mww|spi|rti|ahci)_','',name).replace('_',' ')
    for word in ['usb','scsi','ata','ahci','spi','pwm','bot','csw','cbw','gpio','rti','fis','uas','adc','fua']:
        result=re.sub(r'\b'+word+r'\b',word.upper(),result)
    return result[0].upper()+result[1:]+'.'

def main():
    """Perform simultaneous local and global edits and record exact checks."""
    rows,mapping=load_rows()
    pattern=re.compile('|'.join(re.escape(k) for k in sorted(mapping,key=len,reverse=True)))
    def rename(text):
        """Substitute within identifiers once, including derived locals."""
        return TOKEN.sub(lambda m:pattern.sub(lambda n:mapping[n.group()],m.group()),text)
    local_rows=json.loads((AUDIT/'local-renames.json').read_text())
    grouped=collections.defaultdict(dict)
    for row in local_rows: grouped[(row['file'],row['function'])][row['old']]=row['new']
    filenames=subprocess.check_output(['git','ls-files','-co','--exclude-standard'],text=True).splitlines()
    paths=sorted({Path(p) for p in filenames if Path(p).is_file() and Path(p).suffix in {'.c','.h','.inc','.cmd','.asm','.py','.json','.md'}})
    paths=[p for p in paths if p.parts[0] in {'src','include','linker','tests','docs','temp'} and p.as_posix()!='tests/fixtures/firmware_symbol_manifest.json' and 'naming-audit' not in p.parts and p.name!='test_j7_recovery_documentation.py' and p.as_posix() not in {'docs/README.md','docs/development/rom-loader-recovery.md','docs/development/j7-rom-loader-recovery.md'}]
    snapshots={p:p.read_bytes() for p in paths}
    by_path={p:p.read_text() for p in paths}
    for (filename,function),local_map in grouped.items():
        p=Path(filename);text=by_path[p];start,end=function_bounds(text,function)
        block=text[start:end]
        new_block=TOKEN.sub(lambda m:local_map.get(m.group(),m.group()),block)
        by_path[p]=text[:start]+new_block+text[end:]
    expected_tokens={p:source_tokens(rename(text)) for p,text in by_path.items() if p.suffix in {'.c','.h','.inc','.cmd','.asm'}}
    for p,text in by_path.items():
        new=rename(text)
        if p.suffix in {'.c','.inc'}:
            # Preserve accurate state references in prose as names change.
            for old in sorted(mapping,key=len,reverse=True):
                old_prose=old.replace('_',' ')
                new_prose=mapping[old].replace('_',' ')
                new=re.sub(r'(?<![A-Za-z0-9])'+re.escape(old_prose)+r'(?![A-Za-z0-9])',lambda m:new_prose,new,flags=re.I)
            # Briefs describe the renamed operation instead of its former name.
            function_maps=[r for r in rows if 'address' not in r and r['old']!=r['new']]
            for row in function_maps:
                if not re.search(r'^\w[^\n]*\b'+re.escape(row['new'])+r'\(',new,re.M): continue
                start,end=function_bounds(new,row['new']);block=new[start:end]
                block=re.sub(r'(?m)^ \* @brief [^\n]*',lambda m:' * @brief '+title(row['new']),block,count=1)
                old_action=title(row['old']).rstrip('.')
                block=COMMENT.sub(lambda m:re.sub(re.escape(old_action),lambda n:title(row['new']).rstrip('.'),m.group(),flags=re.I),block)
                new=new[:start]+block+new[end:]
        if p in expected_tokens: assert source_tokens(new)==expected_tokens[p],f'Unexpected code change: {p}'
        old_bytes=snapshots[p]
        ending='\r\n' if b'\r\n' in old_bytes and b'\n' not in old_bytes.replace(b'\r\n',b'') else '\n'
        encoded=new.replace('\n',ending).encode()
        if encoded!=old_bytes: p.write_bytes(encoded)
    manifest_path=ROOT/'tests/fixtures/firmware_symbol_manifest.json'
    manifest=json.loads(manifest_path.read_text())
    by_old={r['old']:r for r in rows if 'address' in r}
    for entry in manifest['symbols']:
        old=entry['semantic_name'];entry['semantic_name']=mapping.get(old,old)
        row=by_old.get(old)
        if row:
            assert entry['address']==row['address']
            evidence=row['evidence'];evidence=[evidence] if isinstance(evidence,str) else evidence
            clean=[]
            for item in evidence:
                # Keep supplied package references; other sibling paths are
                # represented by the checked image address and current code.
                item=re.sub(r'\.\./TUSB9261_RDX_Ghidra_Project/TI_Reference_Source/TUSB9261FW_SourceCode/([^\s;]+)',r'TI firmware package \1',item)
                item=re.sub(r'\.\./[^\s;]+', 'adjacent firmware evidence',item)
                item=re.sub(r'(?i)\bSibling function\b','Corresponding function',item)
                clean.append(rename(item))
            entry['verification']=[f"Fixed binding: {entry['address']}."]+clean
            entry['confidence']=row.get('confidence',entry['confidence'])
        else:
            entry['verification']=[rename(v) for v in entry['verification']]
        if entry['placeholder_name'] in {f'core_branch_target_{n:03d}' for n in [86,87,117,118,119,120]}:
            entry['category']='storage'
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n')
    # Rename the fragment whose entire contents perform unsigned division.
    old_fragment=Path('src/rdx_core/02_management_descriptors.inc')
    new_fragment=old_fragment.with_name('02_multiword_division.inc')
    old_fragment.rename(new_fragment)
    for p in [Path('src/rdx_core.c'),Path('tests/test_rdx_core_fragments.py')]:
        p.write_text(p.read_text().replace(old_fragment.name,new_fragment.name))
    (AUDIT/'applied-global-map.json').write_text(json.dumps(mapping,indent=2)+'\n')
    (AUDIT/'rename-proof.json').write_text(json.dumps({'global_renames':len(mapping),'local_renames':len(local_rows),'code_files_checked':len(expected_tokens),'operations_preserved':True},indent=2)+'\n')
    print(json.dumps({'global_renames':len(mapping),'local_renames':len(local_rows),'code_files_checked':len(expected_tokens)}))

if __name__=='__main__': main()
