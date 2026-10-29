#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Heap-relative bitmaps and full-width native fields, not a complete VM port."""
from pathlib import Path
import re,shutil,argparse
from patch_utils import replace,write_text_if_changed
ROOT=Path(__file__).resolve().parents[1]

def apply(source):
    vm=source/'cldc/src/vm'
    util=vm/'share/utilities'
    shutil.copy2(ROOT/'vm/port/HeapBitmap.hpp',util/'HeapBitmap.hpp')
    replace(util/'GlobalDefinitions.hpp','#include "PointerMath.hpp"',
            '#include "PointerMath.hpp"\n#include "HeapBitmap.hpp"')
    replace(vm/'includeDB','GlobalDefinitions.hpp            PointerMath.hpp',
            'GlobalDefinitions.hpp            PointerMath.hpp\nGlobalDefinitions.hpp            HeapBitmap.hpp')
    p=vm/'share/handles/Oop.hpp'
    replace(p,'  /// Short field access operations.\n  jshort short_field',
            '  // Opaque native addresses are not traced Java object references.\n'
            '  address native_address_field(int offset) const {\n'
            '    address value; memcpy(&value,(address)obj()+offset,sizeof(value)); return value;\n  }\n'
            '  void native_address_field_put(int offset, address value) {\n'
            '    memcpy((address)obj()+offset,&value,sizeof(value));\n  }\n\n'
            '  /// Short field access operations.\n  jshort short_field')
    p=vm/'share/memory/ThreadDesc.hpp'
    for field in ('last_java_sp_or_frame','last_java_fp','stack_limit','stack_pointer'):
        text=p.read_text()
        text=re.sub(r'\bjint(\s+_'+field+r'\s*;)',r'address\1',text)
        write_text_if_changed(p,text)
    p=vm/'share/runtime/Thread.hpp'
    for name in ('last_java_sp','last_java_fp','stack_limit','stack_pointer'):
        text=p.read_text()
        text=re.sub(r'(?:\(address\)\s*)?int_field\('+name+r'_offset\(\)\)',
                    'native_address_field('+name+'_offset())',text)
        text=re.sub(r'int_field_put\('+name+r'_offset\(\),\s*(?:\(jint\))?value\)',
                    'native_address_field_put('+name+'_offset(),value)',text)
        if name=='stack_pointer':
            text=text.replace('jint stack_pointer()', 'address stack_pointer()')
            text=text.replace('set_stack_pointer(jint value)', 'set_stack_pointer(address value)')
        write_text_if_changed(p,text)
    # The C interpreter has no assembler contract for absolute bitmap indexing.
    p=vm/'share/memory/ObjectHeap.hpp'
    replace(p,'return unsigned(p) >> LogBytesPerWord;',
            '#if ENABLE_C_INTERPRETER\n'
            '    return (unsigned)ps3me_heap_slot_index((uintptr_t)_heap_start,(uintptr_t)p,LogBytesPerWord);\n'
            '#else\n    return (uintptr_t)p >> LogBytesPerWord;\n#endif')
    replace(p,'((uintptr_t)p) >> (LogBytesPerWord+LogBitsPerByte)',
            '(ENABLE_C_INTERPRETER ? ps3me_bitmap_byte_offset((uintptr_t)_heap_start,(uintptr_t)p,LogBytesPerWord) : ((uintptr_t)p) >> (LogBytesPerWord+LogBitsPerByte))')
    # Complete initialization/inverse mapping adjustments follow in the slots stage.
    replace(p,'return (OopDesc**)\n        (DISTANCE(_bitvector_base, ptr) << (LogBytesPerWord + LogBitsPerByte));',
            'return (OopDesc**)(ENABLE_C_INTERPRETER\n'
            '      ? ps3me_heap_address_for_bitmap_byte((uintptr_t)_heap_start,DISTANCE(_bitvector_base,ptr),LogBytesPerWord)\n'
            '      : (DISTANCE(_bitvector_base,ptr) << (LogBytesPerWord+LogBitsPerByte)));')
    replace(p,'intptr_t* w = (intptr_t*)word_for(offset, map);',
            'juint* w = (juint*)word_for(offset, map);',True)
    p=vm/'share/memory/ObjectHeap.cpp'
    replace(p,'_bitvector_start - (((uintptr_t)_heap_start >> 2) / BitsPerByte);',
            'ENABLE_C_INTERPRETER ? _bitvector_start :\n'
            '      _bitvector_start - (((uintptr_t)_heap_start >> LogBytesPerWord) / BitsPerByte);')
    replace(p,'target_heap_size/BitsPerWord','target_heap_size/(BytesPerWord*BitsPerByte)',True)
    replace(p,'_heap_capacity/BitsPerWord','_heap_capacity/(BytesPerWord*BitsPerByte)',True)
    replace(p,'(object_heap_size - 1) >> 2','(object_heap_size - 1) >> LogBytesPerWord')
    p=vm/'share/runtime/Thread.cpp'
    replace(p,'set_stack_pointer((jint)sp);','set_stack_pointer(sp);')
    replace(p,'int_field_put(stack_limit_offset(), (jint) value);',
            'native_address_field_put(stack_limit_offset(),value);')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('phoneme',type=Path)
    apply(p.parse_args().phoneme.resolve())
