/* SPDX-License-Identifier: GPL-2.0-only */
package com.ps3me.media;
import java.io.ByteArrayOutputStream;
/** MMAPI ToneControl bytecode to SMF, with bounded block execution. */
public final class ToneSequence {
    private ByteArrayOutputStream track=new ByteArrayOutputStream();
    private int pending,notes;
    private void delta(int n){int b=n&127;while((n>>>=7)!=0)b=(b<<8)|((n&127)|128);for(;;){track.write(b&255);if((b&128)==0)break;b>>>=8;}}
    private void note(int n,int duration,int volume){
        if(++notes>8192 || duration<=0 || pending>0x0fffffff-duration)throw new IllegalArgumentException("Tone sequence too large");
        if(n==-1){pending+=duration;return;}
        if(n<0 || n>127)throw new IllegalArgumentException("Invalid tone");
        delta(pending);pending=0;track.write(144);track.write(n);track.write(volume);
        delta(duration);track.write(128);track.write(n);track.write(0);
    }
    private byte[] finish(){
        delta(pending);track.write(255);track.write(47);track.write(0);
        byte[] body=track.toByteArray();ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] header={77,84,104,100,0,0,0,6,0,0,0,1,3,(byte)232,77,84,114,107};
        out.write(header,0,header.length);
        out.write(body.length>>>24);out.write(body.length>>>16);out.write(body.length>>>8);out.write(body.length);
        out.write(body,0,body.length);return out.toByteArray();
    }
    private ToneSequence(){int[] tempo={0,255,81,3,15,66,64,0,192,80};for(int i=0;i<tempo.length;i++)track.write(tempo[i]);}
    public static byte[] single(int note,int duration,int volume){
        if(note<0 || note>127 || duration<=0 || duration>0x0fffffff)throw new IllegalArgumentException();
        ToneSequence midi=new ToneSequence();midi.note(note,duration,volume<0?0:volume>100?127:volume*127/100);return midi.finish();
    }
    public static byte[] convert(byte[] data){
        if(data==null)throw new IllegalArgumentException();
        if(data.length<2 || data.length>65536 || (data.length&1)!=0 || data[0]!=-2 || data[1]!=1)throw new IllegalArgumentException("Invalid ToneControl version/length");
        int[] starts=new int[128],ends=new int[128];for(int i=0;i<128;i++)starts[i]=-1;
        int block=-1;
        for(int p=2;p<data.length;p+=2){
            int tag=data[p],value=data[p+1];
            if(value<0)throw new IllegalArgumentException("Invalid ToneControl operand");
            if(tag==-5){if(block!=-1 || starts[value]!=-1)throw new IllegalArgumentException("Invalid tone block");block=value;starts[value]=p+2;}
            else if(tag==-6){if(block!=value)throw new IllegalArgumentException("Mismatched tone block");ends[value]=p+2;block=-1;}
            else if(tag==-9){if(p+3>=data.length || value<2 || data[p+2]<-1 || data[p+3]<=0)throw new IllegalArgumentException("Invalid repeat");p+=2;}
            else if(tag<-9 || tag==-2 || (tag==-8&&value>100) || ((tag>=-4&&tag<=-3)&&value==0) || (tag>=-1&&value==0))throw new IllegalArgumentException("Invalid ToneControl instruction");
        }
        if(block!=-1)throw new IllegalArgumentException("Unclosed tone block");
        ToneSequence midi=new ToneSequence();int[] returns=new int[32];int depth=0,tempo=30,resolution=64,volume=127,steps=0,pos=2;
        while(pos<data.length){
            if(++steps>32768)throw new IllegalArgumentException("Cyclic/oversized tone sequence");
            int tag=data[pos],value=data[pos+1];pos+=2;
            if(tag==-5)pos=ends[value];
            else if(tag==-6){if(depth==0)throw new IllegalArgumentException();pos=returns[--depth];}
            else if(tag==-7){if(starts[value]<0 || depth==32)throw new IllegalArgumentException("Invalid tone block call");returns[depth++]=pos;pos=starts[value];}
            else if(tag==-3)tempo=value;
            else if(tag==-4)resolution=value;
            else if(tag==-8)volume=value*127/100;
            else if(tag==-9){int n=data[pos],d=data[pos+1];pos+=2;int ms=Math.max(1,d*60000/(resolution*tempo));for(int i=0;i<value;i++)midi.note(n,ms,volume);}
            else midi.note(tag,Math.max(1,value*60000/(resolution*tempo)),volume);
        }
        if(depth!=0 || midi.notes==0)throw new IllegalArgumentException("Empty/incomplete tone sequence");
        return midi.finish();
    }
}
