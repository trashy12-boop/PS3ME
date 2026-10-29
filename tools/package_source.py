#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Preserve source, dependency pins and genuine validation logs in a checked ZIP."""
from pathlib import Path
import argparse,hashlib,json,subprocess,zipfile
ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
    source=ROOT/'deps/phoneME'
    if (source/'.git').is_dir():
        patch=ROOT/'patches/phoneme-current.patch';patch.parent.mkdir(exist_ok=True)
        patch.write_bytes(subprocess.check_output(['git','-C',str(source),'diff','--binary','--','cldc']))
    files=[]
    kept_build={'build/ps3/release_native.elf','build/pkgroot/USRDIR/games/PS3ME_TouchTest.jar','build/ps3/libps3me_platform.a','build/ps3/libps3me_javacall.a','build/ps3/libps3me_properties.a',
        'build/midp_ppu/bin/ppu/libmidp.a','build/midp_ppu/ROMStructs.h','build/midp_ppu/classes.zip',
        'build/vm_ppu/ps3_ppu/dist/lib/libcldc_vm.a','build/vm_ppu/ps3_ppu/dist/lib/libcldc_vmx.a',
        'build/vm_ppu/ps3_ppu/target/product/ROMImage.o',
        'build/pkgroot/ICON0.PNG','build/pkgroot/USRDIR/games/PS3ME_Diagnostic.jar','build/ps3/PS2ME_PS3_01.15.debug.elf'}
    kept_build.update(str(x.relative_to(ROOT)) for x in (ROOT/'build/pcsl_ppu').rglob('*.a'))
    for folder in ('build/midp_ppu/generated','build/vm_ppu/ps3_ppu/dist/include','build/pcsl_ppu/linux_ppu/inc'):
        kept_build.update(str(x.relative_to(ROOT)) for x in (ROOT/folder).glob('*.h*'))
    kept_build.update(str(x.relative_to(ROOT)) for x in (ROOT/'build/midp_ppu/lib').rglob('*') if x.is_file())
    for f in sorted(ROOT.rglob('*')):
        if not f.is_file():continue
        r=f.relative_to(ROOT)
        if any(x in r.parts for x in ('deps','vendor','.git','__pycache__')):continue
        # RPCS3 configs and shader/PPU caches are disposable test state, not
        # source or validation evidence. They may also contain unreadable files.
        if r.parts[0]=='build' and r.parts[1]=='validation' and any(x in r.parts for x in ('config','cache')):continue
        if r.name in ('FILE_MANIFEST.json','source_package_result.json'):continue
        if r.parts[0]=='build' and not (r.parts[1]=='validation' or str(r) in kept_build or str(r)=='build/vm_abi_audit.json'):continue
        if f.suffix in ('.zip','.pkg','.elf','.class','.jar','.pyc','.deb') and str(r) not in kept_build:continue
        files.append((r.as_posix(),f.read_bytes()))
    # Include concrete adapted CLDC sources, not just descriptions/patchers.
    # SDKs and tool binaries are excluded. The known build exports and our own
    # diagnostic are retained above so that a recovered bundle can be relinked.
    if source.is_dir():
        for f in sorted(x for component in ('cldc','pcsl','midp','javacall') for x in (source/component).rglob('*')):
            if not f.is_file() or f.suffix in ('.o','.a','.so','.jar','.zip','.class','.exe'):continue
            data=f.read_bytes()
            if data[:4] in (b'\x7fELF',b'PK\x03\x04') or data[:2]==b'MZ':continue
            files.append(('vendor/phoneME/'+f.relative_to(source).as_posix(),data))
    manifest={name:hashlib.sha256(data).hexdigest() for name,data in files}
    files.append(('FILE_MANIFEST.json',(json.dumps(manifest,indent=2)+'\n').encode()))
    prefix='PS2ME_PS3_Source/'
    with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in files:z.writestr(prefix+name,data)
    with zipfile.ZipFile(a.output) as z:
        assert z.testzip() is None
        for name,digest in manifest.items():
            assert hashlib.sha256(z.read(prefix+name)).hexdigest()==digest
    print(json.dumps({'file':str(a.output.resolve()),'entries':len(files),'bytes':a.output.stat().st_size,'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest()}))

if __name__=='__main__':main()
