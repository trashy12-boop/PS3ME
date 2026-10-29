#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Link the actual native front end, CLDC, MIDP, PCSL and PS3 services.

Prerequisites: build_vm_host64, probe_vm_ppu, build_pcsl_ppu, build_midp_ppu.
This stage produces a real PPU application; it does not assert console success.
"""
from pathlib import Path
import argparse,json,os,shutil,subprocess
from validate_elf import validate
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--reuse-properties',action='store_true')
    parser.add_argument('--reuse-prebuilt-archives',action='store_true',help='link against the existing PS3 VM/MIDP/platform archives without rebuilding JavaCall')
    args=parser.parse_args()
    os.chdir(ROOT);sdk=ROOT/'deps/ps3dev';build=ROOT/'build/ps3'
    env=os.environ.copy();env.update(PSL1GHT=str(sdk),PS3DEV=str(sdk))
    log=ROOT/'build/validation/application_build.log'
    cc=str(sdk/'ppu/bin/ppu-g++')
    inc=[sdk/'ppu/include',ROOT/'ps3/include',ROOT/'upstream/ps2/javacall',ROOT/'upstream/ps2/javacall/include',
         ROOT/'deps/phoneME/javacall/interface',ROOT/'deps/phoneME/javacall/interface/common',
         ROOT/'deps/phoneME/javacall/interface/midp',sdk/'portlibs/ppu/include/SDL',sdk/'portlibs/ppu/include']
    midp_inc=[ROOT/'build/midp_ppu/generated',ROOT/'build/midp_ppu',ROOT/'build/vm_ppu/ps3_ppu/dist/include',
              ROOT/'build/pcsl_ppu/linux_ppu/inc',ROOT/'deps/phoneME/javacall/implementation/share/properties/inc']
    midp_inc+=sorted({p.parent for p in (ROOT/'deps/phoneME/midp/src').rglob('*.h') if p.parent.name=='include'})
    with log.open('w') as out:
        def run(cmd):subprocess.run([str(x) for x in cmd],stdout=out,stderr=subprocess.STDOUT,env=env,check=True)
        if args.reuse_prebuilt_archives:
            required=[ROOT/'build/midp_ppu/bin/ppu/libmidp.a',ROOT/'build/vm_ppu/ps3_ppu/dist/lib/libcldc_vm.a',ROOT/'build/vm_ppu/ps3_ppu/dist/lib/libcldc_vmx.a',build/'libps3me_javacall.a',build/'libps3me_properties.a',build/'libps3me_platform.a']
            required+=list((ROOT/'build/pcsl_ppu/linux_ppu/lib').glob('*.a'))
            missing=[str(path.relative_to(ROOT)) for path in required if not path.is_file()]
            if missing:raise RuntimeError('Missing prebuilt archives: '+', '.join(missing))
        elif args.reuse_properties:
            assert (build/'libps3me_properties.a').is_file()
            run(['make','-C','ps3','platform',str(build/'libps3me_javacall.a')])
        else:run(['make','-C','ps3','javacall'])
        for source,includes in [('Main',inc),('MidpRuntime',inc+midp_inc),('MediaAudio',inc+midp_inc),('MidiEngine',inc),('Mp3Engine',inc),('BrowserUI',inc),('VmFiles',inc)]:
            run([cc,'-std=gnu++11','-O2','-DPRODUCT','-DENABLE_MIDP_MALLOC=1']+['-I'+str(x) for x in includes]+['-c','ps3/src/'+source+'.cpp','-o',build/(source+'.o')])
        archives=[ROOT/'build/midp_ppu/bin/ppu/libmidp.a',ROOT/'build/vm_ppu/ps3_ppu/dist/lib/libcldc_vm.a',
                  ROOT/'build/vm_ppu/ps3_ppu/dist/lib/libcldc_vmx.a']
        archives+=[build/('libps3me_'+x+'.a') for x in ('javacall','properties','platform')]
        archives+=sorted((ROOT/'build/pcsl_ppu/linux_ppu/lib').glob('*.a'))
        run([cc,build/'Main.o',build/'VmFiles.o',build/'MidpRuntime.o',build/'MediaAudio.o',build/'MidiEngine.o',build/'Mp3Engine.o',build/'BrowserUI.o',ROOT/'build/vm_ppu/ps3_ppu/target/product/ROMImage.o',
             '-Wl,--start-group']+archives+['-L'+str(sdk/'portlibs/ppu/lib'),'-L'+str(sdk/'ppu/lib'),
             '-lSDL_gfx','-lSDL_ttf','-lfreetype','-lSDL_image','-lpng','-ljpeg','-ltiff','-lzip','-lz','-lSDL_mixer','-lmikmod','-lvorbisfile','-lvorbis','-logg','-lSDL','-lm','-lgcm_sys','-lrsx','-lsysutil','-lio','-laudio','-lrt','-llv2',
             '-Wl,--end-group','-Wl,-Map='+str(build/'application.map'),'-o',build/'PS2ME_PS3_01.15.elf'])
    # Follow ppu_rules: strip the linked ELF, then fix imports/PRX descriptors.
    # The signed SELF writer does not preserve non-loadable debug sections.
    shutil.copy2(build/'PS2ME_PS3_01.15.elf',build/'PS2ME_PS3_01.15.debug.elf')
    subprocess.run([str(sdk/'ppu/bin/ppu-strip'),str(build/'PS2ME_PS3_01.15.debug.elf'),'-o',str(build/'PS2ME_PS3_01.15.elf')],env=env,check=True)
    subprocess.run([str(sdk/'bin/sprxlinker'),str(build/'PS2ME_PS3_01.15.elf')],env=env,check=True)
    for name in ('PS2ME_PS3_01.15.elf','PS2ME_PS3_01.15.debug.elf'):
        target=build/name;payload=target.read_bytes();temp=build/(name+'.complete');temp.write_bytes(payload);temp.replace(target)
    (build/'release_native.elf').write_bytes((build/'PS2ME_PS3_01.15.elf').read_bytes())
    report=validate(build/'release_native.elf');report['elf']='build/ps3/release_native.elf'
    (ROOT/'build/validation/application_result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
