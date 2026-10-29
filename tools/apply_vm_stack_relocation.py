#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Relocate the complete saved frame pointer, including on big-endian LP64."""
from patch_utils import replace

def apply(source):
    vm = source / 'cldc/src/vm'
    replace(vm/'share/memory/ExecutionStackDesc.cpp',
            'jint* stored_fp_addr = (jint*)thread->stack_pointer();',
            'address* stored_fp_addr = (address*)thread->stack_pointer();')
    replace(vm/'share/runtime/Thread.cpp',
            'stack_used_size + 4);', 'stack_used_size + BytesPerWord);')
