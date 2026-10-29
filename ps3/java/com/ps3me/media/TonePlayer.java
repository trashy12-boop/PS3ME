/* SPDX-License-Identifier: GPL-2.0-only */
package com.ps3me.media;
import java.io.*;
import java.util.Vector;
import javax.microedition.media.*;
import javax.microedition.media.control.*;

/** ToneControl reuses AudioPlayer and its resource budget, mixer and session teardown. */
public final class TonePlayer implements Player,ToneControl,VolumeControl,PlayerListener {
    public static TonePlayer fromStream(InputStream stream)throws IOException {
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();byte[] block=new byte[1024];int n,empty=0;
        while((n=stream.read(block))!=-1){if(n==0){if(++empty>16)throw new IOException("Stalled tone stream");continue;}empty=0;
            if(bytes.size()+n>65536)throw new IOException("Tone sequence too large");bytes.write(block,0,n);}
        TonePlayer player=new TonePlayer();player.setSequence(bytes.toByteArray());return player;
    }
    private AudioPlayer player;
    private int state=UNREALIZED,loops=1,volume=100;
    private boolean muted;
    private Vector listeners=new Vector();
    private void check(){if(state==Player.CLOSED)throw new IllegalStateException("Player closed");}
    public void setSequence(byte[] sequence){
        check();if(state>=PREFETCHED)throw new IllegalStateException("Tone player prefetched");
        byte[] midi=ToneSequence.convert(sequence);
        try{AudioPlayer replacement=new AudioPlayer(new ByteArrayInputStream(midi),"audio/midi");
            if(player!=null){player.removePlayerListener(this);player.close();}
            player=replacement;player.addPlayerListener(this);
        }catch(IOException e){throw new IllegalArgumentException(e.toString());}
        catch(MediaException e){throw new IllegalArgumentException(e.toString());}
    }
    public void realize(){check();if(state<REALIZED)state=REALIZED;}
    public void prefetch()throws MediaException{
        realize();if(state>=PREFETCHED)return;if(player==null)throw new MediaException("No tone sequence");
        player.prefetch();player.setLoopCount(loops);player.setLevel(volume);player.setMute(muted);state=PREFETCHED;
    }
    public void start()throws MediaException{check();if(getState()==Player.STARTED)return;prefetch();player.start();state=Player.STARTED;}
    public void stop()throws MediaException{check();if(player!=null)player.stop();if(state==Player.STARTED)state=PREFETCHED;}
    public void deallocate(){check();if(player!=null)player.deallocate();if(state>REALIZED)state=REALIZED;}
    public void close(){if(state==Player.CLOSED)return;if(player!=null){player.removePlayerListener(this);player.close();player=null;}state=Player.CLOSED;event(PlayerListener.CLOSED,null);listeners.removeAllElements();}
    public int getState(){if(state==Player.STARTED&&player!=null&&player.getState()!=Player.STARTED)state=PREFETCHED;return state;}
    public void setLoopCount(int n){check();if(n==0||n<-1)throw new IllegalArgumentException();if(getState()==Player.STARTED)throw new IllegalStateException();loops=n;}
    public long setMediaTime(long now)throws MediaException{check();if(player==null){if(now!=0)throw new MediaException("Seeking unsupported");return 0;}return player.setMediaTime(now);}
    public long getMediaTime(){check();return player==null?TIME_UNKNOWN:player.getMediaTime();}
    public long getDuration(){check();return player==null?TIME_UNKNOWN:player.getDuration();}
    public String getContentType(){check();return "audio/x-tone-seq";}
    public Control getControl(String name){check();return "ToneControl".equals(name)||"javax.microedition.media.control.ToneControl".equals(name)||"VolumeControl".equals(name)||"javax.microedition.media.control.VolumeControl".equals(name)?this:null;}
    public Control[] getControls(){check();return new Control[]{this};}
    public int setLevel(int n){check();volume=n<0?0:n>100?100:n;if(player!=null)player.setLevel(volume);return volume;}
    public int getLevel(){check();return volume;}
    public void setMute(boolean value){check();muted=value;if(player!=null)player.setMute(value);}
    public boolean isMuted(){check();return muted;}
    public void addPlayerListener(PlayerListener p){check();if(p!=null&&!listeners.contains(p))listeners.addElement(p);}
    public void removePlayerListener(PlayerListener p){check();listeners.removeElement(p);}
    private void event(String event,Object data){Object[] copy=new Object[listeners.size()];listeners.copyInto(copy);for(int i=0;i<copy.length;i++)try{((PlayerListener)copy[i]).playerUpdate(this,event,data);}catch(Throwable ignored){}}
    public void playerUpdate(Player source,String event,Object data){if(source==player)event(event,data);}
    private static AudioPlayer simpleTone;
    public static synchronized void playTone(int note,int duration,int volume)throws MediaException{
        byte[] midi=ToneSequence.single(note,duration,volume);
        if(simpleTone!=null){simpleTone.close();simpleTone=null;}
        try{simpleTone=new AudioPlayer(new ByteArrayInputStream(midi),"audio/midi");simpleTone.start();}
        catch(IOException e){throw new MediaException(e.toString());}
    }
}
