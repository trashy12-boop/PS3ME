#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile and preverify our diagnostic with the real CLDC verifier."""
from pathlib import Path
import argparse,subprocess,zipfile
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--jdk',type=Path,required=True)
    p.add_argument('--preverify',type=Path,required=True)
    p.add_argument('--runner',type=Path)
    a=p.parse_args();out=ROOT/'build/diagnostic';raw=out/'raw';verified=out/'verified'
    raw.mkdir(parents=True,exist_ok=True);verified.mkdir(exist_ok=True)
    library=ROOT/'build/midp_ppu/classes.zip'
    subprocess.run([str(a.jdk.resolve()/'bin/javac'),'-source','1.4','-target','1.4',
        '-bootclasspath',str(library),'-d',str(raw),str(ROOT/'tests/midp_smoke/PS3MESmoke.java')],check=True)
    names=['PS3MESmoke','PS3MESmoke$Screen']
    command=([str(a.runner.resolve())] if a.runner else [])+[str(a.preverify.resolve()),
        '-classpath',str(raw)+':'+str(library),'-d',str(verified)]+names
    subprocess.run(command,check=True)
    manifest=('Manifest-Version: 1.0\r\nMIDlet-1: PS3ME Diagnostic,,PS3MESmoke\r\n'
        'MIDlet-Name: PS3ME Diagnostic\r\nMIDlet-Vendor: PS3ME Port\r\nMIDlet-Version: 0.2.0\r\n'
        'MicroEdition-Configuration: CLDC-1.1\r\nMicroEdition-Profile: MIDP-2.0\r\n\r\n')
    jar=ROOT/'build/pkgroot/USRDIR/games/PS3ME_Diagnostic.jar';jar.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(jar,'w',zipfile.ZIP_DEFLATED) as z:
        z.writestr('META-INF/MANIFEST.MF',manifest)
        for name in names:
            data=(verified/(name+'.class')).read_bytes();assert data[:4]==b'\xca\xfe\xba\xbe'
            z.writestr(name+'.class',data)
    with zipfile.ZipFile(jar) as z:assert z.testzip() is None
    print('Compiled and preverified:',jar)
if __name__=='__main__':main()
