#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Reconstruct the native slot migration. Experimental until Java tests pass.

The VM's Java numeric types remain unchanged. Heap/stack references use the
actual C++ pointer width. Primitive arrays and bytecode wire data stay Java-sized.
"""
from pathlib import Path
import argparse,re,shutil
from patch_utils import replace,write_text_if_changed
from apply_vm_pointer_fixes import apply as pointers
from apply_vm_memory_fixes import apply as memory
from prepare_vm_baseline import prepare
from apply_vm_reference_paths import apply as references
from apply_vm_native_handles import apply as handles
from apply_vm_constants import apply as constants
from apply_vm_monitor_layout import apply as monitors
from apply_vm_stackmaps64 import apply as stackmaps
from apply_vm_native_finalizers import apply as finalizers
from apply_vm_stack_values import apply as stack_values
from apply_vm_stack_relocation import apply as stack_relocation
ROOT=Path(__file__).resolve().parents[1]

def apply(source):
    prepare(source);pointers(source);memory(source)
    vm=source/'cldc/src/vm'
    def change(name,before,after,all=False):replace(vm/name,before,after,all)
    shutil.copy2(ROOT/'vm/port/VmSlot.hpp',vm/'share/utilities/VmSlot.hpp')
    change('share/utilities/GlobalDefinitions.hpp','#include "HeapBitmap.hpp"',
           '#include "HeapBitmap.hpp"\n#include "VmSlot.hpp"')
    change('includeDB','GlobalDefinitions.hpp            HeapBitmap.hpp',
           'GlobalDefinitions.hpp            HeapBitmap.hpp\nGlobalDefinitions.hpp            VmSlot.hpp')
    change('share/utilities/GlobalDefinitions.hpp','const int LogBytesPerWord    = 2;',
           'const int LogBytesPerWord = (sizeof(void*) == 8 ? 3 : 2);')
    change('share/utilities/GlobalDefinitions.hpp','const int LogBitsPerWord =LogBitsPerByte+LogBytesPerWord;',
           'const int LogBitsPerWord = 5; // width of a Java int and GC bitmap word')
    change('share/utilities/GlobalDefinitions.cpp','4 == sizeof( jobject)',
           'sizeof(void*) == sizeof( jobject)')
    p=vm/'share/utilities/GlobalDefinitions.hpp';text=p.read_text()
    for name in ('OBJECT','ARRAY','SYMBOLIC'):
        text=re.sub(r'(T_'+name+r'_byte_size\s*=\s*)4,',r'\1sizeof(void*),',text)
    write_text_if_changed(p,text)
    change('share/handles/FieldType.cpp',
           '  return is_static_field \n       ? align_size_up(byte_size, T_INT_byte_size)\n       : byte_size;',
           '  return (is_static_field || byte_size >= T_INT_byte_size)\n'
           '       ? align_size_up(byte_size,wordSize) : byte_size;')
    p=vm/'cpu/c/Frame_c.hpp';text=p.read_text()
    text=re.sub(r'return\s+([+-]?(?:28|24|20|16|12|8|4));',
                lambda m:'return %d * BytesPerWord;'%(int(m[1])//4),text)
    write_text_if_changed(p,text)
    change('share/handles/ConstantPool.hpp','(index * sizeof(jint))','(index * BytesPerWord)',True)
    change('share/runtime/ClassFileParser.cpp','cp_length*sizeof(int)','cp_length * BytesPerWord')
    finalizers(source)
    stack_values(source)
    stack_relocation(source)
    change('share/handles/SymbolTable.cpp','((const juint*)old) + sizeof(OopDesc)/BytesPerWord',
           '(const juint*)((const unsigned char*)old + sizeof(OopDesc))',True)
    change('share/memory/OopDesc.cpp',
           '  case InstanceSize::size_type_array_4:  \n  case InstanceSize::size_obj_array:\n    return ((ArrayDesc*)this)->object_size(4);',
           '  case InstanceSize::size_type_array_4:\n    return ((ArrayDesc*)this)->object_size(4);\n'
           '  case InstanceSize::size_obj_array:\n    return ((ArrayDesc*)this)->object_size(sizeof(OopDesc*));')
    change('share/handles/String.hpp','header_size() + sizeof(jobject) + sizeof(jint)',
           'header_size() + 2 * sizeof(jobject)')
    change('share/runtime/Throwable.hpp','* sizeof(jint)','* sizeof(jobject)',True)
    change('share/handles/Method.cpp','juint n = uint_field(stackmaps_offset());',
           'uintptr_t n = (uintptr_t)native_address_field(stackmaps_offset());')
    change('share/handles/WeakRefArray.cpp','Array::Raw array = Universe::new_int_array(length + 1 JVM_CHECK_0);',
           'Array::Raw array;\n  if(sizeof(OopDesc*)==sizeof(jlong)) {\n'
           '    array=Universe::new_long_array(length+1 JVM_CHECK_0);\n  } else {\n'
           '    array=Universe::new_int_array(length+1 JVM_CHECK_0);\n  }')
    change('share/handles/WeakRefArray.cpp','memset(base + sizeof(jint), -1, length * sizeof(jint));',
           'memset(base + sizeof(OopDesc*), -1, length * sizeof(OopDesc*));')
    change('share/handles/ObjArray.cpp','int_field_put( offset, int(value) );',
           '*obj()->obj_field_addr(offset) = value;')
    for old,new in [('int system_symbols[','intptr_t system_symbols['),
                    ('system_symbols[i] = (int)p;','system_symbols[i] = (intptr_t)p;')]:
        change('share/handles/Symbols.cpp',old,new)
    change('share/utilities/GlobalDefinitions.hpp','int system_symbols[]','intptr_t system_symbols[]',True)
    change('share/runtime/HotRoutines.incl.hpp','jint   *val_base = (jint*) ( (int)(obj()) + base_offset() );',
           'address val_base=(address)obj()+base_offset();')
    change('share/runtime/HotRoutines.incl.hpp','val_base[index]',
           '(*(jint*)(val_base + index * BytesPerWord))',True)
    # Native bytecode payloads carry complete function pointers.
    change('share/interpreter/Bytecodes.cpp','def(fast_invokenative         , 8, "bc__jjjj"',
           'def(fast_invokenative         , 2 * BytesPerWord, (sizeof(void*)==8 ? "bc______________" : "bc__jjjj")')
    patch_interpreter(vm/'cpu/c/Interpreter_c.cpp')
    references(source)
    handles(source)
    constants(source)
    monitors(source)
    stackmaps(source)
    # Reference caches contain pointers, not Java ints. Clearing half a slot
    # creates invalid nonzero references when reusing the verifier frame.
    vf=source/'cldc/src/vm/share/verifier/VerifierFrame.cpp'
    for count in ('_max_stack','_max_locals'):
        replace(vf,count+' * sizeof(int)',count+' * sizeof(OopDesc*)')
    replace(source/'cldc/build/share/jvm.make',
      'OopMaps.cpp: $(ROM_GENERATOR)\n\t$(HOST_VM_RUNNER) $(ROM_GENERATOR) +GenerateOopMaps',
      'ifdef PS3ME_NATIVE_OOPMAPS\nOopMaps.cpp: $(PS3ME_NATIVE_OOPMAPS)\n\tcp $< $@\nelse\n'
      'OopMaps.cpp: $(ROM_GENERATOR)\n\t$(HOST_VM_RUNNER) $(ROM_GENERATOR) +GenerateOopMaps\nendif')

def patch_interpreter(p):
    text=p.read_text()
    if '// PS3ME native slots' in text:
        return
    for name in ('GET_LOCAL','GET_LOCAL_LOW','GET_LOCAL_HIGH'):
        fn='ps3me_slot_read_word' if name=='GET_LOCAL' else 'ps3me_slot_read_int'
        n='(n)+1' if name.endswith('HIGH') else '(n)'
        text=re.sub(r'^#define '+name+r'\(n\).*$',
                    '#define '+name+'(n) '+fn+'(g_jlocals-('+n+')*BytesPerWord)',text,flags=re.M)
    for name in ('SET_LOCAL','SET_LOCAL_LOW','SET_LOCAL_HIGH'):
        fn='ps3me_slot_write_word' if name=='SET_LOCAL' else 'ps3me_slot_write_int'
        n='(n)+1' if name.endswith('HIGH') else '(n)'
        text=re.sub(r'^#define '+name+r'\(n, val\).*$',
                    '#define '+name+'(n, val) '+fn+'(g_jlocals-('+n+')*BytesPerWord,(ps3me_vm_slot_t)(val))',text,flags=re.M)
    start=text.index('  // stack accessors\n');end=text.index('  static inline jlong LONG_POP()',start)
    text=text[:start]+'''  // PS3ME native slots: references are full-width; Java int payloads stay 32-bit.
  static inline ps3me_vm_slot_t WORD_POP() {
    ps3me_vm_slot_t v=ps3me_slot_read_word(g_jsp); g_jsp+=BytesPerWord; return v;
  }
  static inline void WORD_PUSH(ps3me_vm_slot_t v) {
    g_jsp-=BytesPerWord; ps3me_slot_write_word(g_jsp,v);
  }
  static inline jint POP() { return (jint)WORD_POP(); }
  static inline void PUSH(jint v) { WORD_PUSH((ps3me_vm_slot_t)(intptr_t)v); }
  static inline address OBJ_POP() { return (address)WORD_POP(); }
  static inline void OBJ_PUSH(address v) { WORD_PUSH((ps3me_vm_slot_t)v); }
  static inline jfloat FLOAT_POP() { union { jint i; jfloat f; } v; v.i=POP(); return v.f; }
  static inline void FLOAT_PUSH(jfloat f) { union { jint i; jfloat f; } v; v.f=f; PUSH(v.i); }

'''+text[end:]
    text=text.replace('ADVANCE((int)(GET_FRAME(return_advance)))','ADVANCE((intptr_t)(GET_FRAME(return_advance)))')
    text=text.replace('*(jint*)(g_jfp + ENTRY_FRAME_OFFSET(name)) = (value)',
                      '*(intptr_t*)(g_jfp + ENTRY_FRAME_OFFSET(name)) = (intptr_t)(value)')
    # VM C-bridge arguments include both pointers and Java scalar values.
    text=re.sub(r'\bjint (r1, r2|arg1, arg2)',r'intptr_t \1',text)
    text=text.replace('jint& r1, jint& r2','intptr_t& r1, intptr_t& r2')
    text=text.replace('va_arg(ap, jint)','va_arg(ap, intptr_t)')
    text=text.replace('(jint)NATIVE_ARG','(intptr_t)NATIVE_ARG')
    for old,new in [('_class_list_base + id*4','_class_list_base + id*BytesPerWord'),
                    ('get_max_stack(method) * 4','get_max_stack(method) * BytesPerWord'),
                    ('locals += 4','locals += BytesPerStackElement'),
                    ('GET_SHORT_NATIVE(0) * 4','GET_SHORT_NATIVE(0) * BytesPerWord'),
                    ('GET_BYTE(0) * 4','GET_BYTE(0) * BytesPerWord'),
                    ('vlength * 4','vlength * BytesPerWord'),('vlength*4','vlength*BytesPerWord'),
                    ('table + method_index * 4','table + method_index * BytesPerWord'),
                    ('g_jsp += 3 * sizeof(jint)','g_jsp += 3 * BytesPerWord'),
                    ('idx * sizeof(jint)','idx * BytesPerWord')]:text=text.replace(old,new)
    write_text_if_changed(p,text)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('phoneme',type=Path)
    apply(p.parse_args().phoneme.resolve())
