#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run an actual CLDC Java regression, separately from the compiler result."""
from pathlib import Path
import argparse,json,os,resource,subprocess,shutil,sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--jdk',type=Path,required=True)
    p.add_argument('--runner',type=Path);p.add_argument('--trace',action='store_true');a=p.parse_args()
    out=ROOT/'build/vm64';logs=ROOT/'build/validation';tests=ROOT/'build/vm64_tests'
    report={'status':'starting','java_marker_seen':False,'ps3_runtime_tested':False}
    env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0:handle_segv=2:allow_user_segv_handler=0:use_sigaltstack=0')
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    def run(command,name,timeout=45):
        with (logs/name).open('w') as log:
            r=subprocess.run([str(x) for x in command],stdout=log,stderr=subprocess.STDOUT,env=env,timeout=timeout)
        return r.returncode
    try:
        for name in ('raw','verified'):(tests/name).mkdir(parents=True,exist_ok=True)
        classes=out/'classes.zip';jdk=a.jdk.resolve();runner=[a.runner.resolve()] if a.runner else []
        code=run([jdk/'bin/javac','-source','1.4','-target','1.4','-bootclasspath',classes,'-d',tests/'raw',ROOT/'tests/vm_baseline/VmBaseline.java'],'recovery_vm64_test_compile.log')
        if code:raise RuntimeError('javac exit '+str(code))
        code=run(runner+[ROOT/'deps/phoneME/preverifier/build/linux/preverify','-classpath',str(classes)+':'+str(tests/'raw'),'-d',tests/'verified',tests/'raw'],'recovery_vm64_test_preverify.log')
        if code:raise RuntimeError('preverify exit '+str(code))
        exe=out/'linux_c64/dist/bin/romgen'
        command=[exe,'=HeapCapacity16M','-cp',str(classes)+':'+str(tests/'verified'),'VmBaseline']
        if a.trace:command.insert(1,'+TraceBytecodes')
        code=run(command,'recovery_vm64_java_run.log')
        marker='PS3ME_VM_BASELINE_OK checks=18' in (logs/'recovery_vm64_java_run.log').read_text()
        report.update(status='passed' if code==0 and marker else 'runtime_failed',java_exit_code=code,java_marker_seen=marker)
    except Exception as error:report.update(status='failed',error=str(error))
    (logs/'recovery_vm64_java_result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return 0 if report['status']=='passed' else 2
if __name__=='__main__':sys.exit(main())
