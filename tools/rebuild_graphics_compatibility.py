#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Rebuild the graphics KNI object against the recovered ABI headers."""
from pathlib import Path
import subprocess
from apply_midp_compatibility import apply
ROOT=Path(__file__).resolve().parents[1]

def main():
    source=ROOT/'deps/phoneME';sdk=ROOT/'deps/ps3dev';apply(source)
    includes=[ROOT/'build/midp_ppu/generated',ROOT/'build/midp_ppu',
        ROOT/'build/vm_ppu/ps3_ppu/dist/include',ROOT/'build/pcsl_ppu/linux_ppu/inc',
        source/'javacall/interface',source/'javacall/interface/common',source/'javacall/interface/midp',
        source/'javacall/implementation/share/properties/inc',sdk/'ppu/include']
    includes+=sorted({f.parent for f in (source/'midp/src').rglob('*.h')})
    for name,relative in [('gxapi_graphics_kni','graphics_api/gxapi_native/native/gxapi_graphics_kni.c'),
                          ('gxj_putpixel','graphics/gx_putpixel/native/gxj_putpixel.c')]:
        output=ROOT/('build/midp_ppu/'+name+'.o')
        command=[str(sdk/'ppu/bin/ppu-gcc'),'-std=gnu99','-O2','-DPRODUCT',
            '-DENABLE_MIDP_MALLOC=1','-DPS3ME_PPU','-fno-strict-aliasing','-fwrapv',
            '-fno-tree-vectorize','-fno-store-merging']
        command+=['-I'+str(p) for p in includes]
        command+=['-c',str(source/('midp/src/lowlevelui/'+relative)),'-o',str(output)]
        subprocess.run(command,check=True)
        subprocess.run([str(sdk/'ppu/bin/ppu-ar'),'r',str(ROOT/'build/midp_ppu/bin/ppu/libmidp.a'),str(output)],check=True)
    print('Graphics compatibility object rebuilt and archived')
if __name__=='__main__':main()
