/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded Nokia Smart Messaging melody -> standard MIDI adapter.
 * OTA instruction decoding follows PS2ME Ps2Audio::decodeOta; output is consumed
 * by PS3ME's existing MIDI synth, never by a new decoder/audio thread.
 */
package com.nokia.mid.sound;
import java.io.ByteArrayOutputStream;

final class NokiaTone {
    private static final int[] BPM={25,28,31,35,40,45,50,56,63,70,80,90,100,112,125,140,
        160,180,200,225,250,285,320,355,400,450,500,565,635,715,800,900};
    private final ByteArrayOutputStream track=new ByteArrayOutputStream();
    private byte[] input;private int bit,notes,pending;
    private NokiaTone() {
        // 1000 ticks/quarter, 1 second/quarter: one tick is one millisecond.
        write(0,255,81,3,15,66,64);
        write(0,192,80); // square lead, matching the upstream simple tone source
    }
    private void write(int a,int b,int c){track.write(a);track.write(b);track.write(c);}
    private void write(int a,int b,int c,int d,int e,int f,int g){write(a,b,c);track.write(d);write(e,f,g);}
    private void delta(int n) {
        int buffer=n&127;
        while((n>>>=7)!=0)buffer=(buffer<<8)|((n&127)|128);
        for(;;){track.write(buffer&255);if((buffer&128)==0)break;buffer>>>=8;}
    }
    private void note(int number,int duration,int gate,int volume) {
        if(++notes>2048 || duration<1 || duration>0x0fffffff-pending)throw new IllegalArgumentException("Melody too large");
        if(number<0){pending+=duration;return;}
        delta(pending);pending=0;track.write(144);track.write(number);track.write(volume);
        int on=(int)((long)duration*gate/100);delta(on);track.write(128);track.write(number);track.write(0);
        pending=duration-on;
    }
    private byte[] finish() {
        delta(pending);write(255,47,0);
        byte[] body=track.toByteArray();ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] header={77,84,104,100,0,0,0,6,0,0,0,1,3,(byte)232,77,84,114,107};
        out.write(header,0,header.length);
        out.write(body.length>>>24);out.write(body.length>>>16);out.write(body.length>>>8);out.write(body.length);
        out.write(body,0,body.length);return out.toByteArray();
    }
    static byte[] tone(int frequency,int duration) {
        NokiaTone midi=new NokiaTone();
        double base=8.1757989156;int note=0;
        while(note<127 && base*1.0293022366<frequency){base*=1.05946309436;note++;}
        // ln(f/base) series near 1 avoids unavailable CLDC Math.log/pow.
        double x=(frequency-base)/(frequency+base),x2=x*x;
        double ln=2*x*(1+x2/3+x2*x2/5+x2*x2*x2/7);
        int bend=8192+(int)(ln*12/0.69314718056*4096);
        bend=bend<0?0:bend>16383?16383:bend;
        midi.write(0,224,bend&127);midi.track.write(bend>>>7);
        midi.note(note,duration,100,127);return midi.finish();
    }
    private int read(int count) {
        if(count<0 || count>input.length*8-bit)throw new IllegalArgumentException("Truncated OTA melody");
        int value=0;
        for(int i=0;i<count;i++,bit++)value=(value<<1)|((input[bit>>>3]>>>(7-(bit&7)))&1);
        return value;
    }
    private void skip(int count) {if(count<0 || count>input.length*8-bit)throw new IllegalArgumentException();bit+=count;}
    static byte[] ota(byte[] data) {
        if(data.length>131072)throw new IllegalArgumentException("OTA melody too large");
        NokiaTone midi=new NokiaTone();midi.input=data;
        int parts=midi.read(8),scale=2,tempo=8,gate=90,volume=127;
        for(int part=0;part<parts;part++) {
            midi.bit=(midi.bit+7)&~7;
            int command=midi.read(7);
            if(command==0x25)continue;
            if(command!=0x1d)throw new IllegalArgumentException("Unsupported OTA command");
            int type=midi.read(3);
            if(type==1)midi.skip(midi.read(4)*8);
            else if(type!=2)throw new IllegalArgumentException("Unsupported OTA song");
            int patterns=midi.read(8);
            for(int p=0;p<patterns;p++) {
                midi.read(3);midi.read(2);midi.read(4);
                int instructions=midi.read(8);
                // Pattern reuse/embedded loop counts are also absent upstream.
                for(int i=0;i<instructions;i++) {
                    int op=midi.read(3);
                    if(op==1) {
                        int value=midi.read(4),length=midi.read(3),modifier=midi.read(2);
                        int duration=60000/BPM[tempo];
                        if(length<2)duration<<=2-length;else if(length<6)duration>>=length-2;
                        if(modifier==1)duration=duration*3/2;
                        else if(modifier==2)duration=duration*7/4;
                        else if(modifier==3)duration=duration*2/3;
                        midi.note(value>=1 && value<=12?48+12*scale+value-1:-1,duration,gate,volume);
                    } else if(op==2)scale=midi.read(2)+1;
                    else if(op==3){int style=midi.read(2);gate=style==1?100:style==2?55:90;}
                    else if(op==4)tempo=midi.read(5);
                    else if(op==5)volume=midi.read(4)*127/15;
                    else throw new IllegalArgumentException("Unsupported OTA instruction");
                }
            }
        }
        if(midi.notes==0)throw new IllegalArgumentException("Empty OTA melody");
        return midi.finish();
    }
}
