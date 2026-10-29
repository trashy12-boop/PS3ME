/* SPDX-License-Identifier: GPL-2.0-only
 * Nokia Sound API adapted from PS2ME's Sound contract to PS3ME MMAPI.
 * No PS2 native methods or SPU2 objects are used.
 */
package com.nokia.mid.sound;
import java.io.ByteArrayInputStream;
import javax.microedition.media.*;
import javax.microedition.media.control.VolumeControl;

public class Sound implements PlayerListener {
    public static final int FORMAT_TONE=1, FORMAT_WAV=5;
    public static final int SOUND_PLAYING=0, SOUND_STOPPED=1, SOUND_UNINITIALIZED=3;
    private Player player;
    private int state=SOUND_UNINITIALIZED, gain=255;
    private SoundListener listener;
    public Sound(int frequency,long duration) { init(frequency,duration); }
    public Sound(byte[] data,int type) { init(data,type); }
    public static int[] getSupportedFormats() { return new int[]{FORMAT_TONE,FORMAT_WAV}; }
    public static int getConcurrentSoundCount(int type) {
        if(type!=FORMAT_TONE && type!=FORMAT_WAV)throw new IllegalArgumentException();
        return 32;
    }
    public void init(int frequency,long duration) {
        if(frequency<=0 || duration<=0 || duration>Integer.MAX_VALUE/2)throw new IllegalArgumentException();
        install(NokiaTone.tone(frequency,(int)duration),"audio/midi");
    }
    public void init(byte[] data,int type) {
        if(data==null)throw new NullPointerException();
        if(type!=FORMAT_TONE && type!=FORMAT_WAV)throw new IllegalArgumentException();
        install(type==FORMAT_TONE?NokiaTone.ota(data):data,type==FORMAT_WAV?"audio/x-wav":"audio/midi");
    }
    private void install(byte[] data,String type) {
        release();
        try {
            player=Manager.createPlayer(new ByteArrayInputStream(data),type);
            player.addPlayerListener(this);
            setState(SOUND_STOPPED);
        } catch(Exception e) { throw new IllegalArgumentException(e.toString()); }
    }
    public void play(int loop) {
        if(loop<0)throw new IllegalArgumentException();
        if(player==null)return;
        try {
            player.stop();player.setMediaTime(0);player.setLoopCount(loop==0?-1:loop);
            player.realize();applyGain();player.start();
            setState(SOUND_PLAYING);
        } catch(MediaException e) { setState(SOUND_STOPPED); }
    }
    public void stop() {
        if(player==null)return;
        try {player.stop();}catch(MediaException ignored){}
        setState(SOUND_STOPPED);
    }
    public void resume() { if(state==SOUND_STOPPED)play(1); }
    public void release() {
        if(player!=null){player.removePlayerListener(this);player.close();player=null;}
        setState(SOUND_UNINITIALIZED);
    }
    public int getState() {
        if(player!=null && state==SOUND_PLAYING && player.getState()!=Player.STARTED)setState(SOUND_STOPPED);
        return state;
    }
    public void setGain(int value) {gain=value<0?0:value>255?255:value;applyGain();}
    public int getGain() { return gain; }
    private void applyGain() {
        if(player==null)return;
        VolumeControl control=(VolumeControl)player.getControl("VolumeControl");
        if(control!=null)control.setLevel((gain*100+127)/255);
    }
    public void setSoundListener(SoundListener value) {listener=value;}
    private void setState(int value) {
        if(state==value)return;
        state=value;
        if(listener!=null)listener.soundStateChanged(this,value);
    }
    public void playerUpdate(Player source,String event,Object data) {
        if(source==player && PlayerListener.END_OF_MEDIA.equals(event))setState(SOUND_STOPPED);
    }
}
