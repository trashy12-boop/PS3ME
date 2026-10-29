#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
from pathlib import Path
import subprocess,zipfile,argparse,shutil
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--ecj',type=Path,required=True);p.add_argument('--runner',type=Path);p.add_argument('--proguard',type=Path,help='ProGuard 6.2.2 alternate CLDC preverifier');a=p.parse_args();r=ROOT;src=r/'deps/phoneME';sdk=r/'deps/ps3dev';out=r/'build/audio';out.mkdir(exist_ok=True)
 manager=src/'midp/src/media/reference/classes/javax/microedition/media/Manager.java';s=manager.read_text()
 start=s.index('    public static Player createPlayer(InputStream stream, String type)');end=s.index('\n\n\n    /**',start)
 s=s[:start]+'''    public static Player createPlayer(InputStream stream, String type) throws IOException, MediaException {
        if(stream==null)throw new IllegalArgumentException();
        return new com.ps3me.media.AudioPlayer(stream,type);
    }'''+s[end:];manager.write_text(s)
 raw=out/'raw';verified=out/'verified';raw.mkdir(exist_ok=True);verified.mkdir(exist_ok=True)
 lib=r/'build/midp_ppu/classes.zip'
 subprocess.run(['java','-jar',str(a.ecj.resolve()),'-1.4','-bootclasspath',str(lib),'-d',str(raw),str(manager),str(r/'ps3/java/com/ps3me/media/AudioPlayer.java')],check=True)
 names=['javax.microedition.media.Manager','com.ps3me.media.AudioPlayer']
 if a.proguard:
  nameset=set(n.replace('.','/')+'.class' for n in names)
  resolver=out/'resolver.zip';destination=out/'verified.zip'
  destination.unlink(missing_ok=True)
  with zipfile.ZipFile(lib) as source,zipfile.ZipFile(resolver,'w') as target:
   for item in source.infolist():
    if item.filename not in nameset:target.writestr(item,source.read(item))
  subprocess.run(['java','-jar',str(a.proguard.resolve()),'-injars',str(raw),'-outjars',str(destination),'-libraryjars',str(resolver),'-dontshrink','-dontoptimize','-dontobfuscate','-microedition','-target','1.1'],check=True)
  with zipfile.ZipFile(destination) as source:source.extractall(verified)
 else:
  if not a.runner:p.error('--runner or --proguard is required')
  pre=src/'cldc/build/share/bin/linux_i386/preverify';pre.chmod(0o755);a.runner.chmod(0o755)
  subprocess.run([str(a.runner.resolve()),str(pre),'-classpath',str(raw)+':'+str(lib),'-d',str(verified)]+names,check=True)
 # Zip replacement, with no duplicate entries.
 files={n.replace('.','/')+'.class':(verified/(n.replace('.','/')+'.class')).read_bytes() for n in names};assert len(files)==2
 with zipfile.ZipFile(lib) as z:entries={x.filename:z.read(x) for x in z.infolist()}
 entries.update(files)
 with zipfile.ZipFile(lib,'w',zipfile.ZIP_DEFLATED) as z:
  for n,b in entries.items():z.writestr(n,b)
 # Rename the original table only once, keeping every existing native binding.
 archive=r/'build/midp_ppu/bin/ppu/libmidp.a';member=out/'nativeFunctionTable.o'
 with member.open('wb') as f:subprocess.run([str(sdk/'ppu/bin/ppu-ar'),'p',str(archive),'nativeFunctionTable.o'],stdout=f,check=True)
 symbols=subprocess.check_output([str(sdk/'ppu/bin/ppu-nm'),str(member)],text=True)
 if ' ps3me_original_natives_table' not in symbols:
  subprocess.run([str(sdk/'ppu/bin/ppu-objcopy'),'--redefine-sym','jvm_natives_table=ps3me_original_natives_table',str(member)],check=True)
  subprocess.run([str(sdk/'ppu/bin/ppu-ar'),'r',str(archive),str(member)],check=True)
 print('Audio Java classes compiled/preverified and native table preserved')
if __name__=='__main__':main()
