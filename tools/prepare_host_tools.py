#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Extract multilib/QEMU into a private directory without upgrading the host.

Input is apt-get --print-uris output from authenticated package metadata.
This helper verifies each advertised size/digest before extracting it.
"""
from pathlib import Path
import argparse,concurrent.futures,hashlib,re,subprocess,urllib.request,shlex

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--uris',type=Path,required=True)
    p.add_argument('--root',type=Path,required=True)
    p.add_argument('--cache',type=Path,required=True)
    a=p.parse_args();a.root.mkdir(parents=True,exist_ok=True);a.cache.mkdir(parents=True,exist_ok=True)
    packages=[]
    for line in a.uris.read_text().splitlines():
        if not line.startswith("'"):continue
        url,name,size,digest=shlex.split(line)
        packages.append((url,name,int(size),digest))
    def fetch(spec):
        url,name,size,digest=spec;path=a.cache/name
        if not path.is_file():
            try:
                with urllib.request.urlopen(url,timeout=30) as src,path.open('wb') as out:
                    while True:
                        data=src.read(1024*1024)
                        if not data:break
                        out.write(data)
            except Exception as error:
                raise RuntimeError(name+': '+str(error)) from error
        assert path.stat().st_size==size, name+' size'
        kind,want=digest.split(':',1)
        actual=hashlib.new({'MD5Sum':'md5','SHA256':'sha256'}[kind],path.read_bytes()).hexdigest()
        assert actual==want,name+' digest'
        return path
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
        files=list(pool.map(fetch,packages))
    for f in files:subprocess.run(['dpkg-deb','-x',str(f),str(a.root)],check=True)
    for name,target in [('lib32','usr/lib32'),('lib/ld-linux.so.2','../usr/lib32/ld-linux.so.2')]:
        link=a.root/name;link.parent.mkdir(parents=True,exist_ok=True)
        if not link.exists() and not link.is_symlink():link.symlink_to(target)
    for name,target in [('linux','/usr/include/linux'),('asm-generic','/usr/include/asm-generic'),('asm','/usr/include/x86_64-linux-gnu/asm')]:
        link=a.root/'usr/include'/name
        if not link.exists() and not link.is_symlink():link.symlink_to(target)
    link=a.root/'usr/include/x86_64-linux-gnu/asm'
    if not link.exists() and not link.is_symlink():link.symlink_to('/usr/include/x86_64-linux-gnu/asm')
    wrappers=a.root.parent/'host32/bin';wrappers.mkdir(parents=True,exist_ok=True)
    root=a.root.resolve()
    for name in ('gcc','g++'):
        flags=['--sysroot='+str(root),'-B'+str(root/'usr/lib/gcc/x86_64-linux-gnu/13/32'),
               '-B'+str(root/'usr/lib32'),'-isystem',str(root/'usr/include'),
               '-isystem',str(root/'usr/include/x86_64-linux-gnu'),
               '-Wl,--dynamic-linker='+str(root/'usr/lib32/ld-linux.so.2'),
               '-Wl,-rpath,'+str(root/'usr/lib32')]
        f=wrappers/name
        f.write_text('#!/bin/sh\nexec /usr/bin/'+name+' '+shlex.join(flags)+' "$@"\n');f.chmod(0o755)
    print('Prepared',len(files),'verified packages in',a.root)

if __name__=='__main__':main()
