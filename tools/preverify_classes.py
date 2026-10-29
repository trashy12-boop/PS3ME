#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run the legacy build-time preverifier in isolated processes per class.

Directory mode retains mutable resolver state across classes and fails on a
complete modern-filesystem MIDP tree. Each output still comes from the actual
CLDC preverifier; this does not skip verification or modify classfile bytes.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import subprocess,sys,tempfile,shutil

def main():
    runner,native,*args=sys.argv[1:]
    base=[runner,native]
    if not args or not Path(args[-1]).is_dir():
        return subprocess.call(base+args)
    root=Path(args[-1]); common=args[:-1]
    output=Path(common[common.index('-d')+1])
    stage=tempfile.TemporaryDirectory(prefix='ps3me-preverify-',dir=output.parent)
    common[common.index('-d')+1]=stage.name
    names=[f.relative_to(root).with_suffix('').as_posix().replace('/','.')
           for f in sorted(root.rglob('*.class'))]
    def verify(name):
        r=subprocess.run(base+common+[name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        return name,r.returncode,r.stdout
    failures=[]
    with ThreadPoolExecutor(max_workers=4) as pool:
        for name,code,diagnostic in pool.map(verify,names):
            if code:
                failures.append(name);sys.stderr.buffer.write(diagnostic)
    if not failures:
        for f in Path(stage.name).rglob('*.class'):
            target=output/f.relative_to(stage.name);target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(f,target)
    stage.cleanup()
    print('PS3ME preverify: %d classes, %d failures'%(len(names),len(failures)))
    return 1 if failures else 0

if __name__=='__main__':sys.exit(main())
