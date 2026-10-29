#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Keep the CRC32 accumulator at 32 bits when unsigned long is 64 bits."""
from patch_utils import replace

def apply(source):
    p=source/'midp/src/core/jarutil/reference/native/jar.c'
    replace(p, '#include <stdio.h>', '#include <stdio.h>\n#include <stdint.h>')
    replace(p, 'unsigned long crc = 0xFFFFFFFF;', 'uint32_t crc = UINT32_C(0xFFFFFFFF);')
