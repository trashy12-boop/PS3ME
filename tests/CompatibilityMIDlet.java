/* SPDX-License-Identifier: GPL-2.0-only */
package com.ps3me.test;
import java.io.ByteArrayInputStream;
import javax.microedition.midlet.MIDlet;
import javax.microedition.lcdui.*;
import javax.microedition.media.*;
import javax.microedition.media.control.*;
import com.nokia.mid.ui.*;
import com.nokia.mid.sound.Sound;

public final class CompatibilityMIDlet extends MIDlet implements Runnable {
    protected void startApp(){new Thread(this).start();}
    protected void pauseApp(){}
    protected void destroyApp(boolean unconditional){}
    private static void check(boolean ok){if(!ok)throw new RuntimeException("Compatibility assertion");}
    public void run(){try{
        com.nokia.mid.sound.NokiaToneRegression.main(null);
        Image image=Image.createImage(17,13);Graphics graphics=image.getGraphics();
        graphics.setColor(0xff00ff);graphics.fillRect(1,1,15,11);
        graphics.drawImage(image,0,0,Graphics.TOP|Graphics.LEFT);
        int[] pixels=new int[1];image.getRGB(pixels,0,1,2,2,1,1);check((pixels[0]&0xffffff)==0xff00ff);
        boolean badAnchor=false;try{graphics.drawImage(image,0,0,-1);}catch(IllegalArgumentException e){badAnchor=true;}check(badAnchor);
        DirectGraphics direct=DirectUtils.getDirectGraphics(graphics);
        direct.drawPixels(new short[]{(short)0xf800},false,0,1,3,3,1,1,0,DirectGraphics.TYPE_USHORT_565_RGB);
        image.getRGB(pixels,0,1,3,3,1,1);check((pixels[0]&0xffffff)==0xff0000);
        direct.fillPolygon(new int[]{0,10,0},0,new int[]{0,0,10},0,3,0xff00ff00);
        System.out.println("PS3ME_GRAPHICS_COMPAT_OK");
        Player tone=Manager.createPlayer(Manager.TONE_DEVICE_LOCATOR);tone.realize();
        ToneControl control=(ToneControl)tone.getControl("ToneControl");check(control!=null);
        control.setSequence(new byte[]{-2,1,-3,30,-4,64,69,16});
        tone.prefetch();check(tone.getState()==Player.PREFETCHED);tone.setLoopCount(-1);tone.start();
        check(tone.getState()==Player.STARTED);Thread.sleep(100);tone.stop();tone.deallocate();tone.close();check(tone.getState()==Player.CLOSED);
        Player stream=Manager.createPlayer(new ByteArrayInputStream(new byte[]{-2,1,60,8}),"audio/x-tone-seq");
        stream.start();Thread.sleep(100);stream.close();
        Sound sound=new Sound(440,200);sound.setGain(128);check(sound.getGain()==128);sound.play(1);
        check(sound.getState()==Sound.SOUND_PLAYING);Thread.sleep(100);sound.stop();sound.release();check(sound.getState()==Sound.SOUND_UNINITIALIZED);
        Manager.playTone(69,100,75);Thread.sleep(150);
        System.out.println("PS3ME_MEDIA_COMPAT_OK");
        Display.getDisplay(this).setCurrent(new Canvas(){protected void paint(Graphics g){g.setColor(0x008000);g.fillRect(0,0,getWidth(),getHeight());g.setColor(0xffffff);g.drawString("Compatibility tests passed",2,30,Graphics.TOP|Graphics.LEFT);}});
        System.out.println("PS3ME_COMPAT_MIDLET_OK");
    }catch(Throwable error){System.out.println("PS3ME_COMPAT_MIDLET_FAILED "+error);error.printStackTrace();}}
}
