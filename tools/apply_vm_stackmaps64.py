#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Keep wire switch integers at 32 bits and mixed stackmap entries native-sized."""
from patch_utils import replace

def apply(source):
    vm=source/'cldc/src/vm/share'
    # All wordSize uses in Method.cpp are classfile switch payloads.
    p=vm/'handles/Method.cpp'
    replace(p,'wordSize','BytesPerInt',True)
    replace(vm/'interpreter/Bytecodes.cpp','wordSize','BytesPerInt',True)
    p=vm/'memory/StackmapListDesc.hpp'
    replace(p,'length * sizeof(int)','length * sizeof(uintptr_t)')
    replace(p,'unsigned int* stackmap_entries_base()', 'uintptr_t* stackmap_entries_base()')
    replace(p,'return ((unsigned int*) this) + (header_size()/sizeof(int));',
            'return (uintptr_t*)((address)this + header_size());')
    p=vm/'memory/StackmapListDesc.cpp'
    replace(p,'unsigned int* entries','uintptr_t* entries')
    replace(p,'unsigned int* end','uintptr_t* end')
    replace(p,'unsigned int _entry_stat','uintptr_t _entry_stat')
    p=vm/'handles/StackmapList.hpp'
    replace(p,'return entry_count_offset() + sizeof(int) * (index + 1);',
            'return StackmapListDesc::header_size() + sizeof(uintptr_t) * index;')
    for name in ('StackmapList.hpp','StackmapList.cpp'):
        p=vm/'handles'/name
        replace(p,'uint_field(entry_stat_offset(index))',
                '(juint)(uintptr_t)native_address_field(entry_stat_offset(index))',True)
        replace(p,'uint_field_put(entry_stat_offset(index), entry);',
                'native_address_field_put(entry_stat_offset(index), (address)(uintptr_t)entry);',True)
