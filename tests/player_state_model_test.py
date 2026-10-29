from pathlib import Path
import subprocess
root=Path('diagnostics_0113/java_model');raw=root/'raw';raw.mkdir(parents=True,exist_ok=True)
s=Path('ps3/java/com/ps3me/media/AudioPlayer.java').read_text().replace('private static native int open(byte[] data);','public static boolean active; public static int starts; private static int open(byte[] data){return 1;}').replace('private static native int command(int handle,int operation,int value);','private static int command(int handle,int operation,int value){if(operation==1){active=true;starts++;}if(operation==2||operation==3)active=false;if(operation==5)return active?1:0;return 0;}')
p=root/'com/ps3me/media/AudioPlayer.java';p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s)
t=root/'PlayerStateTest.java';t.write_text('''import com.ps3me.media.AudioPlayer;import java.io.ByteArrayInputStream;import javax.microedition.media.Manager;
public class PlayerStateTest{static void check(boolean v){if(!v)throw new RuntimeException("failed");}public static void main(String[] a)throws Exception{
AudioPlayer p=new AudioPlayer(new ByteArrayInputStream(new byte[]{1}),"audio/mpeg");p.start();check(AudioPlayer.starts==1);p.start();check(AudioPlayer.starts==1);AudioPlayer.active=false;p.start();check(AudioPlayer.starts==2);p.stop();p.start();check(AudioPlayer.starts==3);p.deallocate();p.start();check(AudioPlayer.starts==4);p.close();boolean threw=false;try{p.start();}catch(IllegalStateException e){threw=true;}check(threw);String[] types=Manager.getSupportedContentTypes(null);boolean mp3=false;for(int i=0;i<types.length;i++)if(types[i].equals("audio/mpeg"))mp3=true;check(mp3);System.out.println("PASS: completed Player restarts; active start no-op; stop/deallocate/restart; closed state; MP3 capability");}}
''')
subprocess.run(['java','-jar','deps/ecj.jar','-1.4','-bootclasspath','build/midp_ppu/classes.zip','-d',str(raw),str(p),str(t)],check=True)
subprocess.run(['java','-cp',str(raw)+':build/midp_ppu/classes.zip','PlayerStateTest'],check=True)
