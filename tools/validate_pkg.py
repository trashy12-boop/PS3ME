#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Independently decrypt and check our own PSL1GHT retail-format homebrew package."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,re,struct,subprocess,zipfile,io
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('pkg',type=Path);p.add_argument('--stage',type=Path,required=True)
    p.add_argument('--key-reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();blob=a.pkg.read_bytes()
    assert blob[:4]==b'\x7fPKG' and int.from_bytes(blob[4:8],'big')==0x80000001
    count=struct.unpack_from('>I',blob,0x14)[0]
    size,offset,length=struct.unpack_from('>QQQ',blob,0x18)
    assert size==len(blob) and 20<count<5000 and offset+length<=size
    assert hashlib.sha1(blob[:-32]).digest()==blob[-32:-12]
    keytext=a.key_reference.read_text();key=re.search(r'retail_pkg_aes_key\[\].*?\{([^}]+)',keytext).group(1)
    key=bytes(int(x.strip(),16) for x in key.split(',') if x.strip())
    result=subprocess.run(['openssl','enc','-d','-aes-128-ctr','-K',key.hex(),'-iv',blob[0x70:0x80].hex(),'-nopad'],input=blob[offset:offset+length],stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    payload=result.stdout;entries={};different=[]
    for i in range(count):
        npos,nlen,dpos,dlen,flags,padding=struct.unpack_from('>IIQQII',payload,i*32)
        assert npos+nlen<=length and dpos+dlen<=length
        name=payload[npos:npos+nlen].decode('utf-8');path=PurePosixPath(name)
        assert not path.is_absolute() and '..' not in path.parts
        if (flags&255)==4:continue
        data=payload[dpos:dpos+dlen];staged=a.stage/name
        assert staged.is_file(),name
        expected=staged.read_bytes()
        assert data==expected,name+' payload differs'
        entries[name]=data
    for name in ('PARAM.SFO','ICON0.PNG','USRDIR/EBOOT.BIN','USRDIR/lib/classes.zip'):assert name in entries,name
    forbidden_test_jars=('USRDIR/games/PS3ME_Diagnostic.jar','USRDIR/games/PS3ME_AudioTest.jar','USRDIR/games/PS3ME_TouchTest.jar')
    assert not set(forbidden_test_jars).intersection(entries), 'release package includes internal test JARs'
    assert entries['USRDIR/EBOOT.BIN'][:4]==b'SCE\0'
    assert entries['PARAM.SFO'][:4]==b'\0PSF'
    sfo=entries['PARAM.SFO'];keybase,datbase,records=struct.unpack_from('<III',sfo,8);values={}
    for i in range(records):
        keyoff,fmt,length,maximum,dataoff=struct.unpack_from('<HHIII',sfo,20+i*16)
        name=sfo[keybase+keyoff:].split(b'\0',1)[0].decode();value=sfo[datbase+dataoff:datbase+dataoff+length]
        assert len(value)==length and length<=maximum
        values[name]=int.from_bytes(value,'little') if fmt==0x404 else value.rstrip(b'\0').decode()
    assert values['TITLE_ID']=='PS3ME0001' and values['CATEGORY']=='HG' and values['BOOTABLE']==1
    assert re.fullmatch(r'\d{2}\.\d{2}',values['APP_VER'])
    with zipfile.ZipFile(io.BytesIO(entries['USRDIR/lib/classes.zip'])) as z:
        assert z.testzip() is None
        for name in ('com/ps3me/loader/GameLoader.class','javax/microedition/lcdui/Canvas.class','javax/microedition/rms/RecordStore.class'):assert name in z.namelist()
    report={'status':'passed','format':'PSL1GHT finalized retail-format homebrew PKG','entries':count,'files':len(entries),'bytes':size,'sha256':hashlib.sha256(blob).hexdigest(),'title_id':values['TITLE_ID'],'app_version':values['APP_VER'],'all_file_payloads_match_stage':True,'class_library_crc':'passed','console_runtime_tested':False}
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
