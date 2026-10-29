#!/usr/bin/env python3
"""Compile the real float adapter against deterministic integer PCM input."""
from pathlib import Path
import tempfile,subprocess,os
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
 d=Path(d);(d/'SDL.h').write_text('#include <stdint.h>\ntypedef int16_t Sint16;\n#define AUDIO_F32SYS 0x9120\n#define AUDIO_S16SYS 0x9010\n')
 (d/'test.cpp').write_text('''#include <cassert>
#include <cmath>
extern "C" int Timidity_Init(int,int,int,int);
extern "C" int Timidity_PlaySome(void*,int);
extern "C" void ps3me_midi_pcm_shutdown();
extern "C" int ps3me_timidity_init_s16(int,int fmt,int,int){assert(fmt==0x9010);return 0;}
extern "C" int ps3me_timidity_play_s16(void* p,int n){short* x=(short*)p;for(int i=0;i<n;i++){*x++=-32768;*x++=16384;}return 17;}
int main(){assert(Timidity_Init(44100,0x9120,2,4)==0);float out[22];out[0]=33;out[21]=44;assert(Timidity_PlaySome(out+1,10)==17);assert(out[0]==33&&out[21]==44);for(int i=1;i<21;i+=2){assert(out[i]==-1.0f);assert(out[i+1]==0.5f);}ps3me_midi_pcm_shutdown();assert(Timidity_Init(44100,0x9120,2,0)==-1);}
''')
 subprocess.run(['g++','-fsanitize=address,undefined','-g','-I'+str(d),str(r/'ps3/src/MidiPcm.cpp'),str(d/'test.cpp'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True,env=dict(os.environ,ASAN_OPTIONS="detect_leaks=0"))
print('MIDI float conversion, stereo order, buffer bounds, chunking and invalid size: PASS')
