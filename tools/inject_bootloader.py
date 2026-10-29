#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile and actually preverify the PS3 bootstrap MIDlet before table generation."""
from pathlib import Path
import subprocess,sys,os
ROOT=Path(__file__).resolve().parents[1]
def inject(jdk,runner):
    out=ROOT/'build/midp_ppu';raw=out/'bootstrap_raw';verified=out/'classes'
    raw.mkdir(exist_ok=True)
    src=ROOT/'ps3/java/com/ps3me/loader/GameLoader.java'
    subprocess.run([str(jdk/'bin/javac'),'-source','1.4','-target','1.4','-bootclasspath',str(out/'classes.zip'),'-d',str(raw),str(src)],check=True)
    command=([str(runner)] if runner else [])+[str(ROOT/'deps/phoneME/preverifier/build/linux/preverify'),'-classpath',str(raw)+':'+str(out/'classes.zip'),'-d',str(verified),'com.ps3me.loader.GameLoader']
    subprocess.run(command,check=True)
    subprocess.run([str(jdk/'bin/jar'),'uf',str(out/'classes.zip'),'-C',str(verified),'com/ps3me/loader/GameLoader.class'],check=True)
    print('PS3 bootstrap MIDlet compiled, preverified and injected',flush=True)
