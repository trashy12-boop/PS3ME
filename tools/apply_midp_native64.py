#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Preserve native image pointers across the Java/C boundary on PPU64."""
from patch_utils import replace

def apply(source):
    for variant, native in (('img_putpixel','imgj'),('img_gci','imggci')):
        root=source/'midp/src/lowlevelui/image'/variant
        java=root/'classes/javax/microedition/lcdui/ImageData.java'
        for name in ('nativePixelData','nativeAlphaData'):
            replace(java,'private int '+name+';', 'private long '+name+';')
        c=root/('native/'+native+'_imagedatafactory_kni.c')
        replace(c,'(jint)rawBuffer->data','(jlong)(intptr_t)rawBuffer->data')
        replace(c,'(jint)(rawBuffer->data + pixelSize)',
                '(jlong)(intptr_t)(rawBuffer->data + pixelSize)')
        replace(c,'#include <stddef.h>', '#include <stddef.h>\n#include <stdint.h>')
    # ROMized skin pointers travel through internal Java interfaces as long.
    for p in (source/'midp/src').rglob('*'):
        if p.suffix not in ('.java','.jpp'):continue
        s=p.read_text(encoding='latin1')
        n=s.replace('int imageDataArrayPtr','long imageDataArrayPtr')
        n=n.replace('int imageDataPtr = getRomizedImageDataArrayPtr','long imageDataPtr = getRomizedImageDataArrayPtr')
        n=n.replace('native static int getRomizedImageDataArrayPtr','native static long getRomizedImageDataArrayPtr')
        if n!=s:p.write_bytes(n.encode('latin1'))
    p=source/'midp/src/highlevelui/lcdlf/lfjava/native/lfj_cskin.c'
    replace(p,'KNIEXPORT KNI_RETURNTYPE_INT\nKNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_getRomizedImageDataArrayPtr)',
              'KNIEXPORT KNI_RETURNTYPE_LONG\nKNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_getRomizedImageDataArrayPtr)')
    replace(p,'KNI_ReturnInt((int)(void*)imageData);','KNI_ReturnLong((jlong)(intptr_t)imageData);')
    replace(p,'#include <kni.h>','#include <kni.h>\n#include <stdint.h>')
    for variant,native in (('img_putpixel','imgj'),('img_gci','imggci')):
        p=source/('midp/src/lowlevelui/image/'+variant+'/native/'+native+'_imagedatafactory_kni.c')
        replace(p,'int imageDataPtr = KNI_GetParameterAsInt(2);','jlong imageDataPtr = KNI_GetParameterAsLong(2);')
        replace(p,'int imageDataLength = KNI_GetParameterAsInt(3);','int imageDataLength = KNI_GetParameterAsInt(4);')
    replace(source/'midp/src/configuration/configuration_xml/javacall/rawimage.xml','Endian="Little"','Endian="Big"')
