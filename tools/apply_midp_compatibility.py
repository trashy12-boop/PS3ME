#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Retain PS2ME's platform-independent drawImage self-blit compatibility rule."""
from patch_utils import replace

def apply(source):
    pixels=source/'midp/src/lowlevelui/graphics/gx_putpixel/native/gxj_putpixel.c'
    replace(pixels,'#include <kni.h>','#include <kni.h>\n#include <stdint.h>')
    # The next stores include jlong. Align all the way to 8 bytes rather than
    # assuming that a single RGB565 pixel fixes a 4/6-byte misalignment.
    replace(pixels,'if (((unsigned int)pPtr & 0x3) && (count > 0)) {',
            'while (((uintptr_t)pPtr & 0x7) && (count > 0)) {',True)
    target=source/'midp/src/lowlevelui/graphics_api/gxapi_native/native/gxapi_graphics_kni.c'
    replace(target,'if (KNI_IsSameObject(gImg, img) || !check_anchor(anchor,0)) {',
            'if (!check_anchor(anchor,0)) {')
    replace(target,'} else if (!normalize_anchor(&x, &y, srcImageDataPtr->width,',
            '} else if (KNI_IsSameObject(gImg, img)) {\n'
            '                /* PS2ME compatibility: drawImage src==dst is undefined\n'
            '                 * by MIDP; accept it as a no-op. drawRegion unchanged. */\n'
            '            } else if (!normalize_anchor(&x, &y, srcImageDataPtr->width,')
