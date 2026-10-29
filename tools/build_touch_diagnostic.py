#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
from pathlib import Path
import argparse,subprocess,zipfile
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--ecj',type=Path,required=True);p.add_argument('--runner',type=Path,required=True);a=p.parse_args();out=r/'build/touch_diagnostic';raw=out/'raw';verified=out/'verified';raw.mkdir(parents=True,exist_ok=True);verified.mkdir(exist_ok=True);lib=r/'build/midp_ppu/classes.zip'
subprocess.run(['java','-jar',str(a.ecj.resolve()),'-1.4','-bootclasspath',str(lib),'-d',str(raw),str(r/'tests/midp_smoke/TouchSmoke.java')],check=True)
subprocess.run([str(a.runner.resolve()),str(r/'deps/phoneME/cldc/build/share/bin/linux_i386/preverify'),'-classpath',str(raw)+':'+str(lib),'-d',str(verified),'TouchSmoke','TouchSmoke$Screen'],check=True)
with zipfile.ZipFile(r/'build/pkgroot/USRDIR/games/PS3ME_TouchTest.jar','w',zipfile.ZIP_DEFLATED) as z:
 for name in ['TouchSmoke','TouchSmoke$Screen']:z.writestr(name+'.class',(verified/(name+'.class')).read_bytes())
 z.writestr('META-INF/MANIFEST.MF','Manifest-Version: 1.0\r\nMIDlet-1: Touch Test,,TouchSmoke\r\nMIDlet-Name: Touch Test\r\nMIDlet-Vendor: PS3ME\r\nMIDlet-Version: 1.0.0\r\nMicroEdition-Configuration: CLDC-1.1\r\nMicroEdition-Profile: MIDP-2.0\r\n\r\n')
print('Touch diagnostic compiled and preverified')
