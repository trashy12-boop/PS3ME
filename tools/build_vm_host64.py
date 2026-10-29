#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build real host64 VM sources and report build/runtime results separately."""
from pathlib import Path
import argparse,json,os,shutil,subprocess,sys,resource
from apply_vm_native64 import apply
ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--jdk',type=Path,required=True)
    p.add_argument('--jobs',type=int,default=6)
    p.add_argument('--sanitize',action='store_true')
    p.add_argument('--runner',type=Path)
    a=p.parse_args();source=ROOT/'deps/phoneME';logs=ROOT/'build/validation'
    logs.mkdir(parents=True,exist_ok=True)
    report={'platform':'host-x86-64','status':'starting','ps3_runtime_tested':False,'java_tested':False}
    out=ROOT/'build/vm64';result=logs/'recovery_vm64_result.json'
    result.write_text(json.dumps(report,indent=2)+'\n')
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    env=os.environ.copy()
    env.setdefault('ASAN_OPTIONS','detect_leaks=0:handle_segv=2:allow_user_segv_handler=0:use_sigaltstack=0')
    try:
        apply(source)
        from apply_vm_layout_generator import apply as layouts
        layouts(source)
        for f in out.rglob('*.o'):
            if f.stat().st_size==0:f.unlink()
        profile=source/'cldc/build/linux_c64';profile.mkdir(exist_ok=True)
        for name in ('Makefile','linux_c64.cfg'):shutil.copy2(ROOT/'vm/linux_c64'/name,profile/name)
        shutil.copy2(ROOT/'vm/linux_c64/AsmStubs_amd64.s',source/'cldc/src/vm/cpu/c/AsmStubs_amd64.s')
        preverify=source/'preverifier/build/linux/preverify'
        installed=out/'linux_c64/dist/bin/preverify'
        installed.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(preverify,installed)
        (out/'classes.zip').unlink(missing_ok=True)
        command=['make','-C',str(profile),'romgen','JDK_DIR='+str(a.jdk.resolve()),
                 'JVMWorkSpace='+str(source/'cldc'),'JVMBuildSpace='+str(out),
                 'PREVERIFY_ORIGINAL='+str(preverify),'PARALLEL_ARGS=-k -j%d'%a.jobs]
        if a.runner:command.append('HOST_PREVERIFY_RUNNER='+str(a.runner.resolve()))
        if a.sanitize:command+=['NATIVE_VM_DIAGNOSTIC_FLAGS=-g -fsanitize=address','USE_DEBUG_ROMGEN_SYMBOLS=true']
        with (logs/'recovery_vm64_build.log').open('w') as f:
            proc=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT,timeout=300,env=env)
            empty=[p for p in out.rglob('*.o') if p.stat().st_size==0]
            if proc.returncode and empty:
                # Failed -pipe builds can leave zero-byte cache entries. They
                # are never valid ELF objects, even when their mtime is fresh.
                for obj in empty:
                    obj.unlink()
                    retry=['make','-B','-C',str(obj.parent),obj.name,
                           'JVMWorkSpace='+str(source/'cldc'),'JDK_DIR='+str(a.jdk.resolve())]
                    if a.sanitize:retry+=['NATIVE_VM_DIAGNOSTIC_FLAGS=-g -fsanitize=address','USE_DEBUG_ROMGEN_SYMBOLS=true']
                    one=subprocess.run(retry,stdout=f,stderr=subprocess.STDOUT,env=env,timeout=90)
                    if one.returncode or not obj.is_file() or obj.stat().st_size==0:
                        raise RuntimeError('Invalid object after rebuilding '+obj.name)
                proc=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT,timeout=300,env=env)
        exe=out/'linux_c64/dist/bin/romgen'
        if exe.is_file():exe.chmod(0o755)
        report.update(build_exit_code=proc.returncode,executable_exists=exe.is_file(),status='built' if proc.returncode==0 and exe.is_file() else 'build_failed')
        # Java execution is a separate stage, never inferred from compiler success.
    except Exception as error:report.update(status='failed',error=str(error))
    result.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return 0 if report['status']=='built' else 2

if __name__=='__main__':sys.exit(main())
