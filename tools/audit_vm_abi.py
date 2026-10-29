#!/usr/bin/env python3
"""Report actual compiler ABI against the CLDC layout in the supplied tree."""
import argparse,json,pathlib,re,subprocess
p=argparse.ArgumentParser();p.add_argument('--compiler',required=True);p.add_argument('--phoneme',required=True);p.add_argument('--output',required=True);p.add_argument('--require-compatible',action='store_true');a=p.parse_args()
tree=pathlib.Path(a.phoneme)
defs=tree/'cldc/src/vm/share/utilities/GlobalDefinitions.hpp'
text=defs.read_text();match=re.search(r'const int LogBytesPerWord\s*=\s*(\d+);',text)
native_slots='const int LogBytesPerWord = (sizeof(void*) == 8 ? 3 : 2);' in text
if not match and not native_slots:raise SystemExit('Unknown upstream layout: audit needs review')
macros=subprocess.check_output([a.compiler,'-dM','-E','-x','c++','-'],input=b'').decode()
def macro(name):
 m=re.search(r'^#define '+re.escape(name)+r'\s+(.+)$',macros,re.M)
 return m.group(1) if m else None
pointer=int(macro('__SIZEOF_POINTER__'));word=pointer if native_slots else 1<<int(match.group(1))
oop=(tree/'cldc/src/vm/share/memory/OopDesc.hpp').read_text()
interpreter=(tree/'cldc/src/vm/cpu/c/Interpreter_c.cpp').read_text()
issues=[]
if pointer!=word:issues.append('CLDC BytesPerWord does not match the native pointer width.')
if 'OopDesc* _klass;' in oop and pointer!=word:issues.append('Object class headers store a native pointer; changing compiler flags cannot retain their 32-bit layout.')
if pointer!=word and '(address*)' in interpreter:issues.append('C interpreter reads native address pointers while its Java stack/frame arithmetic uses BytesPerWord.')
result={'compiler':subprocess.check_output([a.compiler,'--version']).decode().splitlines()[0],
 'pointer_bytes':pointer,'long_bytes':int(macro('__SIZEOF_LONG__')),
 'byte_order':macro('__BYTE_ORDER__'),'cldc_word_bytes':word,
 'native_slot_migration_present':native_slots,'pointer_width_preflight_matches':not issues,'runtime_compatibility_proven':False,'blockers':issues,
 'files_inspected':[str(x.relative_to(tree)) for x in [defs,tree/'cldc/src/vm/share/memory/OopDesc.hpp',tree/'cldc/src/vm/cpu/c/Interpreter_c.cpp']]}
out=pathlib.Path(a.output);out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if a.require_compatible and issues:raise SystemExit(2)
