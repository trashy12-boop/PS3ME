#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Regenerate CLDC StackMaps without modifying non-class game resources.
Requires ProGuard 6.2.2; does not disable the runtime verifier.
"""
import argparse, hashlib, subprocess, tempfile, zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('input',type=Path);p.add_argument('output',type=Path)
p.add_argument('--proguard',type=Path,required=True)
p.add_argument('--library',type=Path,default=Path(__file__).resolve().parents[1]/'build/midp_ppu/classes.zip')
a=p.parse_args()
if a.input.resolve()==a.output.resolve():p.error('keep the original; output must be a different path')
if a.output.exists():p.error('output already exists; choose a new path')
a.output.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='ps3me-cldc-') as td:
 out=Path(td)/'verified.jar'
 subprocess.run(['java','-jar',str(a.proguard.resolve()),'-injars',str(a.input.resolve()),'-outjars',str(out),'-libraryjars',str(a.library.resolve()),'-dontshrink','-dontoptimize','-dontobfuscate','-microedition','-target','1.1','-keepattributes','Exceptions,InnerClasses,EnclosingMethod'],check=True)
 with zipfile.ZipFile(a.input) as original,zipfile.ZipFile(out) as repaired:
  assert repaired.testzip() is None
  count=0
  for name in original.namelist():
   if name.endswith(('.class','/')):continue
   if original.read(name)!=repaired.read(name):raise RuntimeError('Resource changed: '+name)
   count+=1
 a.output.write_bytes(out.read_bytes())
print('Regenerated CLDC verification data; preserved',count,'non-class resources')
print('SHA256',hashlib.sha256(a.output.read_bytes()).hexdigest())
