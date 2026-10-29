#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Preserve opaque native handles and relocate addresses without 32-bit casts."""
from patch_utils import replace,write_text_if_changed
import re

def apply(source):
    from patch_utils import replace
    heap=source/'cldc/src/vm/share/memory/ObjectHeap.hpp'
    for name in ('align_down','align_up'):
        text=heap.read_text().replace('static unsigned '+name+' ( const unsigned n )',
                                     'static unsigned '+name+' ( const uintptr_t n )')
        write_text_if_changed(heap,text)
        replace(heap,'static unsigned '+name+' ( const uintptr_t n )','static uintptr_t '+name+' ( const uintptr_t n )')

    vm=source/'cldc/src/vm'
    def c(name,before,after,all=False):replace(vm/name,before,after,all)
    c('share/memory/FinalizerConsDesc.cpp',
      '  unsigned char *remember_kni_parameter_base;\n  \n  {\n    JavaOop::Raw r = referent();',
      '  unsigned char *remember_kni_parameter_base;\n'
      '  JavaOop::Raw r = referent(); // remains rooted until native finalization returns\n  {')
    c('share/runtime/BufferedFile.hpp','(OsFile_Handle)int_field(file_pointer_offset())',
      '(OsFile_Handle)native_address_field(file_pointer_offset())')
    c('share/runtime/BufferedFile.cpp','int_field_put(file_pointer_offset(), (int)val);',
      'native_address_field_put(file_pointer_offset(),(address)val);')
    c('share/runtime/BufferedFile.hpp','int_field_put(file_pointer_offset(), -1);',
      'native_address_field_put(file_pointer_offset(),(address)(intptr_t)-1);')
    c('share/runtime/FileDecoder.hpp','(OsFile_Handle) int_field(file_handle_offset())',
      '(OsFile_Handle) native_address_field(file_handle_offset())')
    c('share/runtime/FileDecoder.hpp','int_field_put(file_handle_offset(), (int) value);',
      'native_address_field_put(file_handle_offset(),(address)value);')
    replace(source/'cldc/src/javaapi/share/com/sun/cldchi/jvm/FileDescriptor.java','    int handle;','    long handle;')
    c('share/runtime/JarFileParser.hpp','return header_size() + sizeof(jobject);','return header_size() + sizeof(jlong);')
    c('share/runtime/JarFileParser.hpp','(OsFile_Handle)int_field(handle_offset())',
      '(OsFile_Handle)(intptr_t)long_field(handle_offset())')
    c('share/runtime/JarFileParser.cpp','int_field_put(handle_offset(), (int)value);',
      'long_field_put(handle_offset(),(jlong)(intptr_t)value);')
    c('share/runtime/JarFileParser.cpp','int_field_put(handle_offset(), -1);','long_field_put(handle_offset(), -1);')
    c('share/runtime/JarFileParser.cpp','Universe::new_byte_array_raw(4 JVM_NO_CHECK_AT_BOTTOM)',
      'Universe::new_byte_array_raw(sizeof(jlong)+sizeof(jint) JVM_NO_CHECK_AT_BOTTOM)')
    for name in ('KniUncommon.cpp','kni.cpp'):
        c('share/natives/'+name,'(int)fieldID','(int)(intptr_t)fieldID',True)
    c('share/runtime/ClassFileParser.cpp','(int)(cfp->_bufptr)','(intptr_t)(cfp->_bufptr)',True)
    c('share/runtime/ClassFileParser.cpp','(int)(cfp->_bufend)','(intptr_t)(cfp->_bufend)',True)
    c('share/memory/MethodDesc.cpp','(MethodVariablePart*)(int(_variable_part) + delta)',
      'DERIVED(MethodVariablePart*,_variable_part,delta)')
    c('share/utilities/Stream.hpp','print_hex8((int)a);','print_hex8((int)(uintptr_t)a);')
    # Bitmap storage remains uint32_t independently of the native slot size.
    p=vm/'share/memory/ObjectHeap.hpp';s=p.read_text()
    s=s.replace('clear_nth_bit(*w, pos);','*w &= ~(juint(1) << pos);')
    s=s.replace('set_nth_bit(*w, pos);','*w |= juint(1) << pos;')
    s=s.replace('const unsigned n )','const uintptr_t n )');write_text_if_changed(p,s)
    # Offset arithmetic is bounded by the VM heap/stack, but pointer operands
    # must remain native-width until subtraction has been performed.
    c('share/ROM/ROM.cpp','juint p = ((juint)target) - ((juint)&_rom_heap_block[0]);',
      'uintptr_t p = (uintptr_t)target - (uintptr_t)&_rom_heap_block[0];')
    c('share/ROM/ROM.cpp','juint byte_offset = ((juint)obj) - ((juint)&_rom_text_block[0]);',
      'uintptr_t byte_offset = (uintptr_t)obj - (uintptr_t)&_rom_text_block[0];')
    p=vm/'share/memory/ObjectHeap.cpp';s=p.read_text()
    for before,after in [('(int)_inline_allocation_top','(uintptr_t)_inline_allocation_top'),
                         ('unsigned(_heap_chunk)','uintptr_t(_heap_chunk)'),
                         ('unsigned(_bitv_chunk)','uintptr_t(_bitv_chunk)'),
                         ('int(qv.rom_text_start)','intptr_t(qv.rom_text_start)'),
                         ('int(qv.rom_data_start)','intptr_t(qv.rom_data_start)'),
                         ('int(_rom_text_block)','intptr_t(_rom_text_block)'),
                         ('int(_rom_data_block)','intptr_t(_rom_data_block)'),
                         ('(int)destination - (int)this_stack','DISTANCE(this_stack,destination)')]:s=s.replace(before,after)
    write_text_if_changed(p,s)
    c('os/linux/OsMemory_linux.cpp','(juint)chunk','(uintptr_t)chunk',True)
    p=vm/'share/runtime/Frame.cpp';s=p.read_text()
    for before,after in [('((jint)raw_bcp())','((jint)(intptr_t)raw_bcp())'),
                         ('(int)frame_bcp - (int)actual_bcp','DISTANCE(actual_bcp,frame_bcp)'),
                         ('(int)raw_bcp()','(int)(intptr_t)raw_bcp()'),
                         ('(address)((int)actual_bcp + flags)','DERIVED(address,actual_bcp,flags)'),
                         ('((jint)guessed_fp)','((uintptr_t)guessed_fp)'),
                         ('((jint)limit)','((uintptr_t)limit)'),
                         ('print_hex8((int)addr)','print_hex8((int)(uintptr_t)addr)')]:s=s.replace(before,after)
    write_text_if_changed(p,s)
    # Diagnostic hex8 intentionally prints the low 32 bits, not a stored address.
    for name in ('share/runtime/Scheduler.cpp','share/runtime/ClassFileParser.cpp'):
        p=vm/name;s=p.read_text(encoding='latin1')
        for expr in ('this_thread().obj()','thread->obj()','this'):
            s=s.replace('(int)'+expr,'(int)(uintptr_t)'+expr)
        p.write_bytes(s.encode('latin1'))
    # The legacy source-ROM serializer is not a 64-bit ROM format. Its
    # address-by-reference mode is disabled for the native-width build.
    c('share/ROM/ROMWriter.cpp','return (int)(object->obj());','return (int)(intptr_t)(object->obj());')
    c('share/ROM/ROMWriter.cpp','int offset = (int)p - (int)_streaming_oop;',
      'int offset = DISTANCE(_streaming_oop,p);')
    c('share/ROM/ROMWriter.hpp','((int)(object->obj()))','((uintptr_t)(object->obj()))',True)
    c('share/ROM/ROMWriter.hpp','static bool write_by_reference(OopDesc *obj) {',
      'static bool write_by_reference(OopDesc *obj) {\n    if(sizeof(void*)>sizeof(jint)) return false;')
    p=vm/'share/ROM/ROMTools.cpp';s=p.read_text()
    s=s.replace('int o1 = (int)(s1->obj());','intptr_t o1=(intptr_t)s1->obj();')
    s=s.replace('int o2 = (int)(s2->obj());','intptr_t o2=(intptr_t)s2->obj();')
    s=s.replace('(((int)obj->obj()) + old_size - reduction)',
                '((address)obj->obj() + old_size - reduction)');write_text_if_changed(p,s)
    p=vm/'share/ROM/SourceROMWriter.cpp';s=p.read_text()
    s=s.replace('(int)(oop.obj())','(int)(intptr_t)(oop.obj())').replace('(int)(klass.obj())','(int)(intptr_t)(klass.obj())')
    s=s.replace('by_ref ? (int)object->obj()', 'by_ref ? (int)(intptr_t)object->obj()');write_text_if_changed(p,s)
    c('share/handles/Symbols.cpp','system_symbols[i] == (int)symbol',
      'system_symbols[i] == (intptr_t)symbol')
    p=vm/'share/handles/TypeSymbol.cpp';s=p.read_text()
    s=s.replace('int base = (int)(p->_heap_input->base_address());','intptr_t base=(intptr_t)p->_heap_input->base_address();')
    s=s.replace('int cur  = (int)(p->_current_ptr);','intptr_t cur=(intptr_t)p->_current_ptr;')
    s=s.replace('int pos = (int)p->_current_ptr;','intptr_t pos=(intptr_t)p->_current_ptr;');write_text_if_changed(p,s)
    p=vm/'share/memory/ObjectHeap.cpp';s=p.read_text()
    s=s.replace('(int)destination - (int)p','DISTANCE(p,destination)')
    # Compaction tag bit 31 is a 32-bit encoded flag, not an address sign bit.
    s=s.replace('((int) klass) >= 0','((int32_t)(uintptr_t)klass) >= 0')
    s=s.replace('unsigned(new_heap_limit)','uintptr_t(new_heap_limit)')
    s=s.replace('int(_heap_top) - int(_heap_start)','DISTANCE(_heap_start,_heap_top)')
    s=s.replace('int(_slices_start) - int(_bitvector_start)','DISTANCE(_bitvector_start,_slices_start)')
    write_text_if_changed(p,s)
    p=vm/'share/runtime/Synchronizer.cpp';s=p.read_text()
    s=s.replace('(int)stack_lock','(int)(uintptr_t)stack_lock')
    s=s.replace('(OopDesc *)((int)obj->_klass + delta)','DERIVED(OopDesc*,obj->_klass,delta)');write_text_if_changed(p,s)

