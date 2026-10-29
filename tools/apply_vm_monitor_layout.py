#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Native-width deferred calls and monitor records for the C interpreter."""
from patch_utils import replace,write_text_if_changed
import re
def apply(source):
    p=source/'cldc/src/vm/cpu/c/Interpreter_c.cpp';s=p.read_text()
    if '#define GET_ENTRY_ADDRESS' not in s:
        s=s.replace('  // stacklock access\n',
          '#define GET_ENTRY_ADDRESS(entry,name) (*(address*)((address)entry+ENTRY_OFFSET(name)))\n'
          '  // stacklock access\n')
    s=re.sub(r'GET_ENTRY\((\w+), (method|next)\)',r'GET_ENTRY_ADDRESS(\1, \2)',s)
    s=s.replace('jint entry = GET_ENTRY_ADDRESS', 'address entry = GET_ENTRY_ADDRESS')
    s=s.replace('max_len * 8 + StackLock::size() + 4', 'max_len * 2 * BytesPerWord + StackLock::size() + BytesPerWord')
    s=s.replace('jint len = StackLock::size() + 4;', 'jint len = StackLock::size() + BytesPerWord;')
    s=s.replace('    address tmp1 = g_jsp;\n    while (tmp1 != lock) {\n'
                '      *(jint*)tmp1 = *(jint*)(tmp1 + len);\n      tmp1 += 4;\n    }',
                '    jvm_memmove(g_jsp,g_jsp+len,(size_t)(lock-g_jsp));')
    s=re.sub(r'PUSH\(\*\(\(jint\*\)header \+ 2 \* i\)\);',
      'address value=header+2*i*BytesPerWord;\n'
      '      if(*(Tag*)(value+BytesPerWord)==obj_tag) OBJ_PUSH(*(address*)value);\n'
      '      else PUSH(*(jint*)value);',s)
    s=s.replace('JavaNear::raw_value_offset() == 8','JavaNear::raw_value_offset() == 2*BytesPerWord')
    s=s.replace('    address tmp0 = GET_STACKLOCK_OFFSET(lock, copied_near,\n                                        JavaNear::raw_value_offset());',
                '    jint tmp0=*(jint*)(lock+StackLock::copied_near_offset()+JavaNear::raw_value_offset());')
    write_text_if_changed(p,s)
    replace(p,'    SET_STACKLOCK_OFFSET(lock, copied_near, 0,\n'
              '                         *(address*)(near_obj + 0));\n'
              '    SET_STACKLOCK_OFFSET(lock, copied_near, 4,\n'
              '                         *(address*)(near_obj+4));\n'
              '    // set lock bit\n'
              '    jint raw_value =  *(jint*)(near_obj+8) | 0x1;\n'
              '    SET_STACKLOCK_OFFSET(lock, copied_near, 8,\n'
              '                         (address)raw_value);',
              '    address copied=lock+StackLock::copied_near_offset();\n'
              '    memcpy(copied,near_obj,sizeof(JavaNearDesc));\n'
              '    *(jint*)(copied+JavaNear::raw_value_offset()) |= 1;')
    replace(source/'cldc/src/vm/share/runtime/Frame.cpp',
            '#define STACK_LOCK_SIZE (StackLock::size() + 4)',
            '#define STACK_LOCK_SIZE (StackLock::size() + BytesPerWord)')
    replace(source/'cldc/src/vm/share/runtime/Frame.hpp',
            '(sizeof(int) + StackLock::size())', '(BytesPerWord + StackLock::size())',True)
