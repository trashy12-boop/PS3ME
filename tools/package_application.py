#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Package an already built native application with PSL1GHT and verify its bytes."""
from pathlib import Path
import argparse,os,shutil,subprocess,sys,zipfile
from validate_elf import validate
ROOT=Path(__file__).resolve().parents[1]
CID='UP0001-PS3ME0001_00-0000000000000001'
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--crypto-libs',type=Path)
    p.add_argument('--include-test-jars',action='store_true',help='include the internal diagnostic MIDlets (development builds only)')
    a=p.parse_args();os.chdir(ROOT);sdk=ROOT/'deps/ps3dev';pkgtools=ROOT/'tools/packaging'
    # Sources in a repository are not evidence of a usable packaged API.
    with zipfile.ZipFile(ROOT/'build/midp_ppu/classes.zip') as runtime:
        required={'com/ps3me/loader/GameLoader.class','com/ps3me/media/AudioPlayer.class',
            'com/ps3me/media/TonePlayer.class','com/ps3me/media/ToneSequence.class',
            'com/nokia/mid/ui/DeviceControl.class','com/nokia/mid/ui/FullCanvas.class',
            'com/nokia/mid/ui/DirectUtils.class','com/nokia/mid/ui/DirectGraphics.class',
            'com/nokia/mid/ui/DirectGraphicsImp.class','com/nokia/mid/sound/Sound.class',
            'com/nokia/mid/sound/SoundListener.class','com/nokia/mid/sound/NokiaTone.class'}
        missing=required-set(runtime.namelist())
        if missing:raise RuntimeError('Runtime API packaging incomplete: '+', '.join(sorted(missing)))
    source_stage=ROOT/'build/pkgroot';stage=ROOT/'build/pkgroot_release'
    shutil.rmtree(stage,ignore_errors=True);shutil.copytree(source_stage,stage)
    env=os.environ.copy();env.setdefault('CC','gcc');env.setdefault('LDSHARED','gcc -shared')
    if a.crypto_libs:env['LD_LIBRARY_PATH']=str(a.crypto_libs.resolve())
    log=ROOT/'build/validation/package_application.log'
    with log.open('w') as out:
        def run(command,**kw):subprocess.run([str(x) for x in command],stdout=out,stderr=subprocess.STDOUT,env=env,check=True,**kw)
        native=ROOT/'build/ps3/release_native.elf';validate(native)
        for folder in ('lib','games','logs','appdb'):(stage/'USRDIR'/folder).mkdir(parents=True,exist_ok=True)
        run([sdk/'bin/make_self_npdrm',native,stage/'USRDIR/EBOOT.BIN',CID])
        # PSL1GHT's PKG writer aligns NPDRM SELF data after its 0x30-byte
        # prefix. Materialize that zero padding so installed bytes and the
        # independently validated staging file are exactly identical.
        eboot=stage/'USRDIR/EBOOT.BIN';data=eboot.read_bytes()
        padded=((len(data)-0x30+63)&~63)+0x30
        eboot.write_bytes(data+b'\0'*(padded-len(data)))
        shutil.copy2(ROOT/'build/midp_ppu/classes.zip',stage/'USRDIR/lib/classes.zip')
        for f in (ROOT/'build/midp_ppu/lib').iterdir():
            if f.is_file():shutil.copy2(f,stage/'USRDIR/lib'/f.name)
        config=ROOT/'deps/phoneME/midp/src/security/midp_permissions/reference/config'
        for kind in ('_policy.txt','_function_groups.txt'):
            for f in config.glob(kind+'.*'):shutil.copy2(f,stage/'USRDIR/lib'/f.name)
            shutil.copy2(config/(kind+'.JTWI'),stage/'USRDIR/lib'/kind)
        for name in ('README_PS3.md','COPYING_PHONE_ME.txt'):shutil.copy2(ROOT/name,stage/'USRDIR'/name)
        shutil.rmtree(stage/'USRDIR/timidity',ignore_errors=True)
        (stage/'USRDIR/timidity.cfg').unlink(missing_ok=True)
        shutil.copytree(ROOT/'assets/audio/soundfont',stage/'USRDIR/soundfont',dirs_exist_ok=True)
        shutil.copytree(ROOT/'assets/ui',stage/'USRDIR/ui',dirs_exist_ok=True)
        # Keep the XMB icon source-controlled and package the same 320x176 PNG.
        shutil.copy2(ROOT/'assets/ui/ICON0.PNG',stage/'ICON0.PNG')
        shutil.copy2(ROOT/'third_party/minimp3/LICENSE',stage/'USRDIR/COPYING_MINIMP3.txt')
        shutil.copy2(ROOT/'third_party/tinysoundfont/LICENSE',stage/'USRDIR/COPYING_TINYSOUNDFONT.txt')
        shutil.copy2(ROOT/'COPYING_SDL_MIXER.txt',stage/'USRDIR/COPYING_SDL_MIXER.txt')
        # Keep diagnostic MIDlets out of public/release packages by default.
        test_jars=('PS3ME_Diagnostic.jar','PS3ME_AudioTest.jar','PS3ME_TouchTest.jar')
        if not a.include_test_jars:
            for name in test_jars:(stage/'USRDIR/games'/name).unlink(missing_ok=True)
        assert (stage/'ICON0.PNG').is_file()
        run([sys.executable,pkgtools/'sfo.py','--fromxml','--title','PS3ME - Experimental','--appid','PS3ME0001',ROOT/'ps3/sfo.xml',stage/'PARAM.SFO'])
        run([sys.executable,pkgtools/'setup.py','build_ext','--inplace'],cwd=pkgtools)
        for f in stage.rglob('*'):
            if f.is_file():f.write_bytes(f.read_bytes())
        output=a.output.resolve()
        run([sys.executable,pkgtools/'pkg.py','--contentid',CID,str(stage)+'/',output])
        run([sdk/'bin/package_finalize',output])
        output.write_bytes(output.read_bytes())
        run([sys.executable,ROOT/'tools/validate_pkg.py',output,'--stage',stage,'--key-reference',pkgtools/'pkg_transport_key.h','--output',ROOT/'build/validation/pkg_result.json'])
    print('Packaged and structurally validated:',output)
if __name__=='__main__':main()
