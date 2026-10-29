#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Full-width native interpreter references and width-aware shared opcodes."""
from patch_utils import replace,write_text_if_changed
import re

def apply(source):
    # Fast int/float field bytecodes use a word-scaled operand even when the
    # Java primitive is narrower than a native pointer.
    replace(source/'cldc/src/vm/share/interpreter/InterpreterRuntime.cpp',
            '::byte_size_for(type) >= BytesPerWord',
            '::byte_size_for(type) >= BytesPerInt')
    replace(source/'cldc/src/vm/share/handles/Method.cpp',
            'byte_size_for(type) >= BytesPerWord', 'byte_size_for(type) >= BytesPerInt',True)
    replace(source/'cldc/src/vm/share/memory/ClassInfoDesc.cpp',
            '  for (i = 0; i < _itable_length; i++) {\n'
            '    itable++;          // skip interface klass_index\n'
            '    itable++;          // skip integer method index\n  }',
            '  // The itable header contains two Java ints, not two pointers.\n'
            '  itable=(OopDesc**)((address)itable+_itable_length*2*sizeof(jint));')
    p=source/'cldc/src/vm/cpu/c/Interpreter_c.cpp';s=p.read_text()
    if '#define GET_THREAD_ADDRESS' not in s:
        s=s.replace('  // entry access\n',
          '#define GET_THREAD_ADDRESS(name) (*(address*)((address)_current_thread + THREAD_OFFSET(name)))\n'
          '#define SET_THREAD_ADDRESS(name,value) (*(address*)((address)_current_thread + THREAD_OFFSET(name))=(address)(value))\n'
          '  // entry access\n')
    for field in ('obj_value','stack_pointer','last_java_fp','last_java_sp','pending_entries','thread_obj','stack_limit'):
        s=s.replace('GET_THREAD_INT('+field+')','GET_THREAD_ADDRESS('+field+')')
        s=s.replace('SET_THREAD_INT('+field+',','SET_THREAD_ADDRESS('+field+',')
    s=s.replace('jint &r1, jint &r2','intptr_t &r1, intptr_t &r2')
    s=s.replace('jint r1 = 0, r2 = 0','intptr_t r1 = 0, r2 = 0')
    s=s.replace('jint r1, r2','intptr_t r1, r2')
    s=s.replace('jint arg1','intptr_t arg1').replace('jint arg2','intptr_t arg2')
    # Narrowing is valid for bounded integer frame offsets, never addresses.
    s=s.replace('(int)method','(intptr_t)method')
    for value in ('g_jfp','g_jsp','exception','_current_pending_exception','return_point','new_stack_ptr','clazz','klass'):
        s=s.replace('(jint)'+value,'(intptr_t)'+value)
    s=s.replace('(jint)f(NATIVE_ARG','(intptr_t)f(NATIVE_ARG')
    s=s.replace('(jint)find_exception_frame','(intptr_t)find_exception_frame')
    for value in ('old_jsp','old_jfp','obj','_jvm_quick_native_exception'):
        s=re.sub(r'\(jint\)'+re.escape(value)+r'\b','(intptr_t)'+value,s)
    s=s.replace('(juint)method','(uintptr_t)method')
    s=s.replace('(int)(GET_FRAME(return_advance))','(intptr_t)(GET_FRAME(return_advance))')
    s=s.replace('(jint)(g_jpc + 1 + 3)','(uintptr_t)(g_jpc + 1 + 3)')
    s=s.replace('((int)g_jfp)','((uintptr_t)g_jfp)')
    s=s.replace('(jint)GET_NEAR(near_obj, raw_value)',
                '(*(jint*)(near_obj+JavaNear::raw_value_offset()))')
    s=s.replace('(jint)tmp0','(intptr_t)tmp0')
    s=re.sub(r'\bPUSH\(GET_THREAD_ADDRESS\(', 'OBJ_PUSH(GET_THREAD_ADDRESS(',s)
    s=s.replace('OBJ_OBJ_PUSH(', 'OBJ_PUSH(')
    s=s.replace('(jint)GET_ENTRY_FRAME(', '(jint)(intptr_t)GET_ENTRY_FRAME(')
    s=s.replace('(jint)(intptr_t)GET_ENTRY_FRAME(stored_obj_value)',
                '(intptr_t)GET_ENTRY_FRAME(stored_obj_value)')
    s=s.replace('(address)get_from_cpool(cpool, index)',
                '*(address*)(cpool+index*BytesPerWord)')
    s=s.replace('return *(jint*)arg_address_from_sp(index);','return ps3me_slot_read_int(arg_address_from_sp(index));')
    s=s.replace('return *((jint*)g_jsp + n);','return ps3me_slot_read_word(g_jsp+n*BytesPerWord);')
    s=s.replace('static inline jint PEEK(jint n)','static inline ps3me_vm_slot_t PEEK(jint n)')
    # Stack shuffles must not truncate object values in transient temporaries.
    begin=s.index('  BYTECODE_IMPL(pop)');end=s.index('  BYTECODE_IMPL(iadd)',begin)
    block=s[begin:end]
    if '// PS3ME reference stack shuffles' not in block:
        block=block.replace('  BYTECODE_IMPL(pop)', '  BYTECODE_IMPL(pop) // PS3ME reference stack shuffles',1)
        block=block.replace('jint val','ps3me_vm_slot_t val')
        block=re.sub(r'\bPOP\(\)', 'WORD_POP()',block)
        block=re.sub(r'\bPUSH\(', 'WORD_PUSH(',block)
    block=block.replace('jint v = PEEK(0);','ps3me_vm_slot_t v = PEEK(0);')
    s=s[:begin]+block+s[end:]
    s=s.replace('cpool += index * 4', 'cpool += index * BytesPerWord')
    s=s.replace('cpool + index * 4', 'cpool + index * BytesPerWord')
    s=s.replace('ci + idx * 4', 'ci + idx * BytesPerWord')
    s=s.replace('*((jint*)cpool + index)','*(jint*)(cpool+index*BytesPerWord)')
    s=s.replace('    PUSH(GET_LOCAL(n));\n  }\n\n  static inline void lload',
                '    PUSH(GET_LOCAL_LOW(n));\n  }\n\n  static inline void lload')
    # Object locals are explicitly distinguished from scalar load/store paths.
    s=re.sub(r'(static inline void aload\(int n\) \{\s*)PUSH\(GET_LOCAL\(n\)\);',
             r'\1WORD_PUSH(GET_LOCAL(n));',s)
    s=re.sub(r'(static inline void astore\(int n\) \{\s*)SET_LOCAL\(n, POP\(\)\);',
             r'\1SET_LOCAL(n, WORD_POP());',s)
    s=s.replace('jint v = GET_ARRAY_ELEMENT(ref, idx, jint);\n        PUSH(v);\n      }\n      break;\n    default',
                'address v = GET_ARRAY_ELEMENT(ref, idx, address);\n        OBJ_PUSH(v);\n      }\n      break;\n    default')
    s=s.replace('    case T_OBJECT :\n    case T_INT    :\n    case T_FLOAT  :\n      rv.int_val =  int_from_sp(0);',
                '    case T_OBJECT:\n      rv.object_val=address_from_sp(0);\n      break;\n'
                '    case T_INT:\n    case T_FLOAT:\n      rv.int_val=int_from_sp(0);')
    s=s.replace('    case T_INT:\n    case T_OBJECT:\n      PUSH(r1);',
                '    case T_OBJECT:\n      OBJ_PUSH((address)r1);\n      break;\n    case T_INT:\n      PUSH((jint)r1);')
    # A resolved String shares fast_1_ldc with ints/floats. CP scalar fields use
    # Java element layout, while an oop entry contains a complete native address.
    old='''        jint val = get_from_cpool(cpool, idx);
        PUSH(val);'''
    new='''        ConstantPool::Raw cp=(OopDesc*)(cpool-ConstantPool::base_offset());
        if(cp().tag_at(idx).is_oop()) OBJ_PUSH(*(address*)(cpool+idx*BytesPerWord));
        else PUSH(get_from_cpool(cpool,idx));'''
    s=s.replace(old,new)
    # Both primitive and object getter optimizations used the int entry upstream.
    s=s.replace('    PUSH(*(jint*)(OBJ_POP() + get_fast_accessor_offset()));',
                '    address field=OBJ_POP()+get_fast_accessor_offset();\n'
                '    BasicType type=(BasicType)*(jushort*)(callee_method()+Method::fast_accessor_type_offset());\n'
                '    if(type==T_OBJECT || type==T_ARRAY) OBJ_PUSH(*(address*)field);\n'
                '    else PUSH(*(jint*)field);')
    s=s.replace('    PUSH(*(jint*)(obj + get_fast_accessor_offset()));',
                '    address field=obj+get_fast_accessor_offset();\n'
                '    BasicType type=(BasicType)*(jushort*)(callee_method()+Method::fast_accessor_type_offset());\n'
                '    if(type==T_OBJECT || type==T_ARRAY) OBJ_PUSH(*(address*)field);\n'
                '    else PUSH(*(jint*)field);')
    s=s.replace('      PUSH(*(jint*)addr);\n      ADVANCE(3);',
                '      ConstantPool::Raw cp=(OopDesc*)(GET_FRAME(cpool)-ConstantPool::base_offset());\n'
                '      BasicType type=ConstantTag::resolved_field_type(cp().tag_value_at(GET_SHORT(0)));\n'
                '      if(type==T_OBJECT || type==T_ARRAY) OBJ_PUSH(*(address*)addr);\n'
                '      else PUSH(*(jint*)addr);\n      ADVANCE(3);')
    s=s.replace('_protected_page[INTERPRETER_TIMER_TICK_SLOT] = (int)g_jpc;',
                '_protected_page[INTERPRETER_TIMER_TICK_SLOT] = 1;')
    write_text_if_changed(p,s)
    p=source/'cldc/src/vm/share/natives/kni.cpp';s=p.read_text()
    s=re.sub(r'\*\(jint\*\)(parameter_address\(index(?:\+1)?\))',r'ps3me_slot_read_int(\1)',s)
    s=s.replace('(int)fieldID','(int)(intptr_t)fieldID');write_text_if_changed(p,s)
    p=source/'cldc/src/vm/share/utilities/GlobalDefinitions_gcc.hpp'
    replace(p,'tw 12, r0, r0','tw 31, 0, 0')
