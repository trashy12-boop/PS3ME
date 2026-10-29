#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Inspect classfile dependencies/members and compare upstream source contents.

This is a reproducible static inventory, not a claim of runtime equivalence.
"""
import argparse, hashlib, json, re, struct, zipfile
from pathlib import Path

def parse(data):
    pos=0
    def u(n):
        nonlocal pos
        value=int.from_bytes(data[pos:pos+n],'big');pos+=n;return value
    def skip(n):
        nonlocal pos
        pos+=n
    if u(4)!=0xcafebabe:raise ValueError('Not a classfile')
    minor,major=u(2),u(2);pool=[None]*u(2);i=1
    while i<len(pool):
        tag=u(1)
        if tag==1:
            n=u(2);pool[i]=(tag,data[pos:pos+n].decode('utf-8','replace'));skip(n)
        elif tag in (7,8,16):pool[i]=(tag,u(2))
        elif tag in (9,10,11,12,18):pool[i]=(tag,u(2),u(2))
        elif tag in (3,4):skip(4)
        elif tag in (5,6):skip(8);i+=1
        elif tag==15:skip(3)
        else:raise ValueError('Unsupported constant-pool tag '+str(tag))
        i+=1
    def utf(i):return pool[i][1]
    def cls(i):return utf(pool[i][1]) if i else None
    flags,this,parent=u(2),cls(u(2)),cls(u(2))
    interfaces=[cls(u(2)) for _ in range(u(2))]
    def attrs():
        names=[]
        for _ in range(u(2)):
            names.append(utf(u(2)));skip(u(4))
        return names
    members=[]
    for kind in ('field','method'):
        for _ in range(u(2)):
            access,name,desc=u(2),utf(u(2)),utf(u(2));attributes=attrs()
            members.append(dict(kind=kind,name=name,descriptor=desc,access=access,attributes=attributes))
    attrs();refs=set();calls=[]
    for item in pool:
        if item and item[0]==7:
            value=utf(item[1])
            if value.startswith('['):refs.update(re.findall(r'L([^;]+);',value))
            else:refs.add(value)
        if item and item[0] in (9,10,11):
            signature=pool[item[2]]
            calls.append((cls(item[1]),utf(signature[1]),utf(signature[2]),'field' if item[0]==9 else 'method'))
            refs.update(re.findall(r'L([^;]+);',utf(signature[2])))
    return dict(name=this,parent=parent,interfaces=interfaces,major=major,minor=minor,
                members=members,references=sorted(refs),calls=calls)

def archive(path):
    with zipfile.ZipFile(path) as z:
        names=z.namelist()
        if len(names)!=len(set(names)):raise ValueError('Duplicate archive entries')
        assert z.testzip() is None
        return {n[:-6]:parse(z.read(n)) for n in names if n.endswith('.class')}

def missing_members(classes,game):
    def exists(owner,name,desc,kind,seen=None):
        seen=set() if seen is None else seen
        if owner in seen:return False
        seen.add(owner)
        # JVM-defined array clone and Object methods do not have classfiles.
        if owner.startswith('['):return name=='clone' or exists('java/lang/Object',name,desc,kind,seen)
        c=classes.get(owner)
        if c is None:return False
        if any(m['name']==name and m['descriptor']==desc and m['kind']==kind for m in c['members']):return True
        if name=='<init>':return False
        return any(exists(p,name,desc,kind,seen) for p in [c['parent']]+c['interfaces'] if p)
    return sorted({tuple(call) for c in game.values() for call in c['calls'] if not exists(*call)})

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--library',type=Path,required=True);p.add_argument('--game',type=Path)
    p.add_argument('--upstream',type=Path);p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    lib=archive(a.library);result={'runtime_class_count':len(lib),'runtime_sha256':hashlib.sha256(a.library.read_bytes()).hexdigest(),
        'packages':{},'natives':{},'runtime_verification_claimed':False}
    for name,c in lib.items():
        package=name.rsplit('/',1)[0];result['packages'][package]=result['packages'].get(package,0)+1
        native=[m for m in c['members'] if m['access']&0x100]
        if native:result['natives'][name]=native
    if a.game:
        game=archive(a.game);allclasses=dict(lib,**game)
        refs={ref for c in game.values() for ref in c['references']}
        result['game']={'sha256':hashlib.sha256(a.game.read_bytes()).hexdigest(),'classes':len(game),
            'missing_classes':sorted(refs-set(allclasses)), 'missing_members':missing_members(allclasses,game)}
    if a.upstream:
        def compare(source,target):
            report={'identical':[],'changed':[],'absent':[]}
            for f in sorted(source.rglob('*')):
                if not f.is_file() or any(x.startswith('.') for x in f.relative_to(source).parts):continue
                rel=f.relative_to(source);t=target/rel
                key='absent' if not t.is_file() else 'identical' if f.read_bytes()==t.read_bytes() else 'changed'
                report[key].append(rel.as_posix())
            return report
        result['imported_ps2_source']=compare(a.upstream/'ps2',a.root/'upstream/ps2')
        result['phoneme_overrides']=compare(a.upstream/'ps2/phoneme',a.root/'deps/phoneME')
        result['upstream_java_api_classes']=[]
        for f in sorted((a.upstream/'ps2/vm/api').rglob('*.java')):
            code=f.read_text();pkg=re.search(r'package\s+([\w.]+)',code)
            if pkg:
                name=pkg[1].replace('.','/')+'/'+f.stem
                result['upstream_java_api_classes'].append({'class':name,'packaged':name in lib})
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k in ('runtime_class_count','game')},indent=2))
if __name__=='__main__':main()
