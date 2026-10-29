// SPDX-License-Identifier: GPL-2.0-only
// Input resources are supplied locally for testing; no game audio is distributed.
import javax.microedition.midlet.MIDlet;
import javax.microedition.media.*;
public class GameAudioSmoke extends MIDlet implements Runnable {
 protected void startApp(){new Thread(this).start();}protected void pauseApp(){}protected void destroyApp(boolean b){}
 public void run(){try{Player p=Manager.createPlayer(getClass().getResourceAsStream("/sample.mid"),"audio/midi");p.realize();p.start();Thread.sleep(1600);System.out.println("[PS3ME GAME AUDIO] midi state="+p.getState());p.close();p=Manager.createPlayer(getClass().getResourceAsStream("/effect.wav"),"audio/x-wav");p.realize();p.start();Thread.sleep(1500);p.close();System.out.println("[PS3ME GAME AUDIO] PASS");}catch(Throwable t){System.out.println("[PS3ME GAME AUDIO] FAIL "+t);t.printStackTrace();}}
}
