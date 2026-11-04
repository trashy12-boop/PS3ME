#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile and preverify HTTP compatibility against the existing 1.23 runtime."""
from pathlib import Path
import argparse,os,subprocess,zipfile
from apply_network_compatibility import apply
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--jdk',type=Path,required=True);p.add_argument('--runner',type=Path)
    a=p.parse_args(); src=ROOT/'deps/phoneME'; apply(src)
    work=ROOT/'build/network_java'; raw=work/'raw'; verified=work/'verified'
    raw.mkdir(parents=True,exist_ok=True);verified.mkdir(parents=True,exist_ok=True)
    library=ROOT/'build/midp_ppu/classes.zip'
    sources=[src/'midp/src/protocol/http/reference/classes/com/sun/midp/io/j2me/http/Protocol.java']
    sources+=sorted((ROOT/'ps3/java/javax/microedition/io/file').glob('*.java'))
    sources+=[ROOT/'ps3/java/com/nokia/mid/ui/SoftNotification.java']
    subprocess.run([str(a.jdk/'bin/javac'),'-source','1.4','-target','1.4','-bootclasspath',str(library),'-d',str(raw),*[str(s) for s in sources]],check=True)
    cmd=([str(a.runner)] if a.runner else [])+[str(src/'preverifier/build/linux/preverify'),'-classpath',str(raw)+os.pathsep+str(library),'-d',str(verified),'com.sun.midp.io.j2me.http.Protocol']
    for f in sorted(raw.rglob('*.class')):
        cmd[-1]=f.relative_to(raw).with_suffix('').as_posix().replace('/','.')
        subprocess.run(cmd,check=True)
    entries={f.relative_to(verified).as_posix():f.read_bytes() for f in verified.rglob('*.class')}
    staged=work/'classes.zip'
    with zipfile.ZipFile(library) as old,zipfile.ZipFile(staged,'w',zipfile.ZIP_DEFLATED) as new:
        for info in old.infolist():
            if info.filename not in entries:new.writestr(info,old.read(info))
        for name,payload in entries.items():new.writestr(name,payload)
    staged.replace(library)
    print('HTTP PUT/DELETE/PATCH/OPTIONS compiled, preverified and packaged')
if __name__=='__main__':main()
