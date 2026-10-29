#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build PCSL with the actual PPU compiler and POSIX-backed persistent files."""
from pathlib import Path
import argparse,json,os,shlex,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--jobs',type=int,default=2);a=p.parse_args()
    src=ROOT/'deps/phoneME/pcsl';sdk=ROOT/'deps/ps3dev';out=ROOT/'build/pcsl_ppu'
    from apply_pcsl_ps3 import apply
    apply(ROOT/'deps/phoneME')
    shutil.copy2(ROOT/'pcsl/ps3_ppu_gcc.gmk',src/'makefiles/platforms/ps3_ppu_gcc.gmk')
    types=src/'types/ps3_ppu_gcc';types.mkdir(exist_ok=True)
    shutil.copy2(ROOT/'pcsl/java_types_md.h',types/'java_types_md.h')
    wrappers=ROOT/'build/ppu_tools/bin';wrappers.mkdir(parents=True,exist_ok=True)
    for name in ('gcc','g++','ar','ranlib','strip','nm'):
        path=wrappers/name
        path.write_text('#!/bin/sh\nexec '+shlex.quote(str(sdk/'ppu/bin'/('ppu-'+name)))+' "$@"\n');path.chmod(0o755)
    env=os.environ.copy();env.update(PS3DEV=str(sdk),PSL1GHT=str(sdk))
    command=['make','-C',str(src),'-j%d'%a.jobs,'PCSL_PLATFORM=ps3_ppu_gcc',
             'PCSL_OUTPUT_DIR='+str(out),'GNU_TOOLS_DIR='+str(wrappers.parent),'all']
    logs=ROOT/'build/validation';logs.mkdir(parents=True,exist_ok=True)
    report={'platform':'PPU','status':'starting','ps3_runtime_tested':False}
    try:
        with (logs/'pcsl_ppu_build.log').open('w') as log:
            r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=180)
        libs=list(out.rglob('*.a'))
        report.update(status='built' if r.returncode==0 else 'failed',build_exit_code=r.returncode,
                      archives=[str(x.relative_to(ROOT)) for x in libs])
    except Exception as error:report.update(status='failed',error=str(error))
    (logs/'pcsl_ppu_result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return 0 if report['status']=='built' else 2
if __name__=='__main__':sys.exit(main())
