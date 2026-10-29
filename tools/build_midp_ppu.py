#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Attempt an actual MIDP cross build, using dynamic system class loading."""
from pathlib import Path
import argparse,os,json,shutil,subprocess,sys,shlex
from patch_utils import replace
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--jdk',type=Path,required=True)
    p.add_argument('--classes-only',action='store_true');p.add_argument('--jobs',type=int,default=2)
    p.add_argument('--runner',type=Path)
    a=p.parse_args();source=ROOT/'deps/phoneME';sdk=ROOT/'deps/ps3dev';out=ROOT/'build/midp_ppu'
    from apply_midp_native64 import apply
    apply(source)
    from apply_installer_paths import apply as installer_paths
    from apply_jar_crc32 import apply as jar_crc
    installer_paths(source)
    jar_crc(source)
    cldc=ROOT/'build/vm_ppu/ps3_ppu/dist';jc=ROOT/'build/javacall_ppu'
    (jc/'inc').mkdir(parents=True,exist_ok=True);(jc/'lib').mkdir(exist_ok=True)
    for folder in ('javacall/interface','javacall/interface/common','javacall/interface/midp',
                   'javacall/implementation/share/properties/inc','javacall/implementation/share/utils/inc'):
        for f in (source/folder).glob('*.h'):shutil.copy2(f,jc/'inc'/f.name)
    shutil.copy2(ROOT/'upstream/ps2/javacall/include/javacall_platform_defs.h',jc/'inc')
    shutil.copy2(ROOT/'upstream/ps2/javacall/config/properties.xml',jc/'properties.xml')
    shutil.copy2(source/'javacall/build/configuration.dtd',jc/'configuration.dtd')
    for f in (ROOT/'midp').glob('*.gmk'):shutil.copy2(f,source/'midp/build/javacall'/f.name)
    config=source/'midp/build/common/makefiles/cldc_vm.gmk'
    # These backends use the PS3 OS adapters directly. PCSL remains linked into
    # MIDP for persistent storage, independently of the VM's own I/O facade.
    s=config.read_text()
    if '# PS3ME CLDC uses native PSL1GHT OS adapters' not in s:
        s=s.replace('cldc_target_os = $(TARGET_OS)',
                    'cldc_target_os = $(TARGET_OS)\n'
                    '# PS3ME CLDC uses native PSL1GHT OS adapters\n'
                    'ifeq ($(JAVACALL_PLATFORM),ps3_ppu_gcc)\ncldc_target_os = generic\nendif')
        config.write_text(s)
    if 'PS3ME native OS facade' not in s:
        start=s.index('    if grep "^\\#define ENABLE_PCSL')
        end=s.index('    fi; \\',start)+len('    fi; \\')
        block=s[start:end]
        s=s[:start]+'    :; \\'+s[end:]
        s=s.replace('# Specify romizer options','# PS3ME native OS facade supplies CLDC I/O without ENABLE_PCSL.\n# Specify romizer options')
        s=s.replace('JTWI_NATIVE_FILES += ROMImage.cpp nativeFunctionTable.cpp',
                    'JTWI_NATIVE_FILES += nativeFunctionTable.cpp')
        s=s.replace('EXTRA_CFLAGS += -DROMIZING','# Dynamic classes: no 32-bit ROM image is linked.')
        config.write_text(s)
    replace(source/'midp/src/security/midp_permissions/lib.gmk',
            'javacall_policy_load.c : policy_files','javacall_policy_load.c : | policy_files')
    (out/'obj/ppu').mkdir(parents=True,exist_ok=True)
    out.mkdir(parents=True,exist_ok=True);logs=ROOT/'build/validation'
    env=os.environ.copy();env.update(PS3DEV=str(sdk),PSL1GHT=str(sdk),
      JAVA_TOOL_OPTIONS='-Djava.awt.headless=true')
    target=str(out/'classes.zip') if a.classes_only else str(out/'bin/ppu/libmidp.a')
    for obj in (out/'obj/ppu').glob('*.o'):
        if obj.stat().st_size<64:obj.unlink()
    command=['make','-C',str(source/'midp/build/javacall'),'-j%d'%a.jobs,
      'JAVACALL_PLATFORM=ps3_ppu_gcc','USE_VERBOSE_MAKE=false',
      'PS3ME_LAYOUT_GENERATOR='+str(ROOT/'build/vm64/linux_c64/dist/bin/romgen'),'JAVACALL_OUTPUT_DIR='+str(jc),
      'CLDC_DIST_DIR='+str(cldc),'PCSL_OUTPUT_DIR='+str(ROOT/'build/pcsl_ppu'),
      'MIDP_OUTPUT_DIR='+str(out),'TOOLS_DIR='+str(source/'tools'),
      'GNU_TOOLS_DIR='+str(ROOT/'build/ppu_tools'),'JDK_DIR='+str(a.jdk.resolve()),target]
    command.insert(-1,'VERIFYCLASSPATH='+str(out/'tmpclasses')+':'+str(out/'classes'))
    if a.runner:
        verifier=cldc/'bin/preverify'
        native=cldc/'bin/preverify.native'
        if verifier.read_bytes()[:4]==b'\x7fELF':shutil.copy2(verifier,native)
        verifier.write_text('#!/bin/sh\nexec python3 '+shlex.quote(str(ROOT/'tools/preverify_classes.py'))+' '+shlex.quote(str(a.runner.resolve()))+' '+shlex.quote(str(native))+' "$@"\n')
        verifier.chmod(0o755)
        installed=out/'bin/ppu/preverify'
        if installed.exists():shutil.copy2(verifier,installed)
    report={'platform':'PPU','status':'starting','classes_only':a.classes_only,'ps3_runtime_tested':False}
    try:
        with (logs/'midp_ppu_build.log').open('w') as log:
            result=subprocess.run(command[:-1]+['jpp'],stdout=log,stderr=subprocess.STDOUT,env=env,timeout=90)
            if result.returncode==0 and not a.classes_only:
                result=subprocess.run(command[:-1]+[str(out/'classes.zip')],stdout=log,stderr=subprocess.STDOUT,env=env,timeout=240)
            if result.returncode==0 and not a.classes_only:
                from inject_bootloader import inject
                inject(a.jdk.resolve(),a.runner.resolve() if a.runner else None)
            if result.returncode==0:
                result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=240)
        report.update(status='built' if result.returncode==0 else 'failed',build_exit_code=result.returncode,
                      classes_zip_exists=(out/'classes.zip').is_file())
    except Exception as error:report.update(status='failed',error=str(error))
    (logs/'midp_ppu_result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    return 0 if report['status']=='built' else 2
if __name__=='__main__':sys.exit(main())
