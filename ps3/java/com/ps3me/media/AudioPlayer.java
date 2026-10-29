// SPDX-License-Identifier: GPL-2.0-only
package com.ps3me.media;
import javax.microedition.media.*;
import javax.microedition.media.control.VolumeControl;
import java.io.*;
import java.util.Vector;
/** Single-output SDL music player. Unsupported formats fail explicitly. */
public final class AudioPlayer implements Player,VolumeControl {
 private byte[] data; private String type; private int handle,state=UNREALIZED,loops=1,volume=100; private boolean muted;
 private Vector listeners=new Vector();
 private static native int open(byte[] data);
 private static native int command(int handle,int operation,int value);
 public AudioPlayer(InputStream in,String mime)throws IOException,MediaException {
  ByteArrayOutputStream out=new ByteArrayOutputStream(); byte[] b=new byte[4096];int n,empty=0;
  while((n=in.read(b))!=-1){if(n==0){if(++empty>16)throw new IOException("Stalled audio stream");continue;}empty=0;if(out.size()+n>8388608)throw new MediaException("Audio exceeds 8 MiB");out.write(b,0,n);}
  data=out.toByteArray();type=mime==null?"application/octet-stream":mime;
 }
 private void check(){if(state==CLOSED)throw new IllegalStateException("Player closed");}
 private void event(String e,Object value){Object[] copy; synchronized(listeners){copy=new Object[listeners.size()];listeners.copyInto(copy);}for(int i=0;i<copy.length;i++)try{((PlayerListener)copy[i]).playerUpdate(this,e,value);}catch(Throwable ignored){}}
 public synchronized void realize()throws MediaException {check();if(state>=REALIZED&&handle>0)return;handle=open(data);if(handle<=0)throw new MediaException("Unsupported audio or output unavailable");state=REALIZED;}
 public synchronized void prefetch()throws MediaException {realize();if(state<PREFETCHED)state=PREFETCHED;}
 public synchronized void start()throws MediaException {check();if(state==STARTED){if(command(handle,5,0)!=0)return;state=PREFETCHED;}prefetch();command(handle,4,muted?0:volume);if(command(handle,1,loops)<0)throw new MediaException("Audio start failed");state=STARTED;event(PlayerListener.STARTED,new Long(0));}
 public synchronized void stop()throws MediaException {check();if(state!=STARTED)return;command(handle,2,0);state=PREFETCHED;event(PlayerListener.STOPPED,new Long(0));}
 public synchronized void deallocate(){check();if(state==UNREALIZED)return;if(handle>0){command(handle,3,0);handle=0;}state=REALIZED;}
 public synchronized void close(){if(state==CLOSED)return;if(handle>0)command(handle,3,0);handle=0;data=null;state=CLOSED;event(PlayerListener.CLOSED,null);listeners.removeAllElements();}
 public synchronized int getState(){if(state==STARTED&&command(handle,5,0)==0){state=PREFETCHED;event(PlayerListener.END_OF_MEDIA,new Long(-1));}return state;}
 public synchronized void setLoopCount(int n){check();if(n==0||n< -1)throw new IllegalArgumentException();if(state==STARTED)throw new IllegalStateException();loops=n;}
 public synchronized long setMediaTime(long now)throws MediaException {check();if(now!=0)throw new MediaException("Seeking is not supported");if(handle>0&&command(handle,6,0)<0)throw new MediaException("Rewind failed");return 0;}
 public long getMediaTime(){check();return TIME_UNKNOWN;} public long getDuration(){check();return TIME_UNKNOWN;}
 public String getContentType(){check();return type;}
 public void addPlayerListener(PlayerListener p){check();if(p!=null&&!listeners.contains(p))listeners.addElement(p);} public void removePlayerListener(PlayerListener p){check();listeners.removeElement(p);}
 public Control getControl(String name){check();return "VolumeControl".equals(name)||"javax.microedition.media.control.VolumeControl".equals(name)?this:null;} public Control[] getControls(){check();return new Control[]{this};}
 public synchronized int setLevel(int n){check();volume=n<0?0:n>100?100:n;if(handle>0)command(handle,4,muted?0:volume);event(PlayerListener.VOLUME_CHANGED,this);return volume;}
 public int getLevel(){check();return volume;} public synchronized void setMute(boolean value){check();muted=value;if(handle>0)command(handle,4,muted?0:volume);event(PlayerListener.VOLUME_CHANGED,this);}public boolean isMuted(){check();return muted;}
}
