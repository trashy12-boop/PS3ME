#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Keep class-file long/double constants in two Java slots on either endian."""
from patch_utils import replace
def apply(source):
    p=source/'cldc/src/vm/share/handles/ConstantPool.hpp'
    old=p.read_text()
    updated=old.replace('return msw_first ? jlong_from_low_high(b,a) : jlong_from_low_high(a,b);',
                        'return jlong_from_msw_lsw(msw_first ? a : b, msw_first ? b : a);')
    if old!=updated:p.write_text(updated)
    replace(p,'  jlong long_at(int index) {',
      '  jlong wide_bits_at(int index,bool msw_first) const {\n'
      '    jint a=int_field(offset_from_index(index));\n'
      '    jint b=int_field(offset_from_index(index+1));\n'
      '    return jlong_from_msw_lsw(msw_first ? a : b, msw_first ? b : a);\n  }\n'
      '  jlong long_at(int index) {')
    replace(p,'return long_field(offset_from_index(index));',
      'return wide_bits_at(index,tag_at(index).is_double() ? MSW_FIRST_FOR_DOUBLE : MSW_FIRST_FOR_LONG);',True)
    replace(p,'return double_field(offset_from_index(index));',
      'return jdouble_cast(wide_bits_at(index,MSW_FIRST_FOR_DOUBLE));',True)
    replace(p,'long_field_put(offset_from_index(index), l);',
      'long_parts_at_put(index,(jint)((julong)l>>32),(jint)l);')
    replace(p,'double_field_put(offset_from_index(index), d);',
      'jlong bits=jlong_cast(d);\n    double_parts_at_put(index,(jint)((julong)bits>>32),(jint)bits);')
    p=source/'cldc/src/vm/cpu/c/Interpreter_c.cpp'
    replace(p,'jlong val = long_from_addr(cpool + idx * 4);',
      'ConstantPool::Raw cp=(OopDesc*)(cpool-ConstantPool::base_offset());\n'
      '        jlong val=cp().wide_bits_at(idx,cp().tag_at(idx).is_double() ? MSW_FIRST_FOR_DOUBLE : MSW_FIRST_FOR_LONG);')
