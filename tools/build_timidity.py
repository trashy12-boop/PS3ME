#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
from pathlib import Path
import subprocess,shutil
r=Path(__file__).resolve().parents[1];sdk=r/'deps/ps3dev';out=r/'build/audio';src=r/'third_party/timidity';out.mkdir(exist_ok=True)
a=out/'libSDL_mixer.a';shutil.copy2(sdk/'portlibs/ppu/lib/libSDL_mixer.a',a)
for p in sorted(src.glob('*.c')):
 o=out/p.with_suffix('.o').name
 cmd=[str(sdk/'ppu/bin/ppu-gcc'),'-O2','-fno-strict-aliasing','-I'+str(sdk/'portlibs/ppu/include/SDL'),'-I'+str(sdk/'ppu/include'),'-I'+str(src)]
 if p.name=='timidity.c':cmd+=['-DTimidity_Init=ps3me_timidity_init_s16']
 if p.name=='playmidi.c':cmd+=['-DTimidity_PlaySome=ps3me_timidity_play_s16']
 subprocess.run(cmd+['-c',str(p),'-o',str(o)],check=True)
 subprocess.run([str(sdk/'ppu/bin/ppu-ar'),'r',str(a),str(o)],check=True)
