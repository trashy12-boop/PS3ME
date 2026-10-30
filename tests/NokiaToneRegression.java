/* SPDX-License-Identifier: GPL-2.0-only */
package com.nokia.mid.sound;
import com.ps3me.media.ToneSequence;
import com.nokia.mid.ui.DeviceControl;
import java.io.ByteArrayOutputStream;
public final class NokiaToneRegression {
    private static int checks;
    private static void check(boolean value){checks++;if(!value)throw new RuntimeException("check "+checks);}
    private static int[] stats(byte[] midi){
        check(midi[0]==77&&midi[1]==84&&midi[2]==104&&midi[3]==100);
        int pos=22,time=0,count=0,last=-1,lastAt=0;
        while(pos<midi.length){int delta=0,b;do{b=midi[pos++]&255;delta=(delta<<7)|(b&127);}while((b&128)!=0);time+=delta;
            int status=midi[pos++]&255;
            if(status==255){int type=midi[pos++]&255,len=midi[pos++]&255;pos+=len;if(type==47)break;}
            else if((status&240)==192)pos++;
            else {int note=midi[pos++]&255;int value=midi[pos++]&255;if((status&240)==144&&value!=0){count++;last=note;lastAt=time;}}
        }
        return new int[]{count,last,time,lastAt};
    }
    private static byte[] ota(){
        // programming marker + temporary song, one scale-2 A quarter note.
        int[] values={2,0x25,0x1d,2,1,0,0,0,1,1,10,2,0};
        int[] widths={8,7,7,3,8,3,2,4,8,3,4,3,2};
        ByteArrayOutputStream out=new ByteArrayOutputStream();int byteValue=0,bits=0;
        for(int k=0;k<values.length;k++){
            if(k==2&&bits!=0){out.write(byteValue<<(8-bits));bits=0;byteValue=0;}
            for(int n=widths[k]-1;n>=0;n--){byteValue=(byteValue<<1)|((values[k]>>>n)&1);if(++bits==8){out.write(byteValue);bits=0;byteValue=0;}}
        }
        if(bits!=0)out.write(byteValue<<(8-bits));return out.toByteArray();
    }
    public static void main(String[] args){
        DeviceControl.setLights(0,100);DeviceControl.flashLights(10);DeviceControl.startVibra(50,10);DeviceControl.stopVibra();check(true);
        int[] tone=stats(NokiaTone.tone(440,250));check(tone[0]==1&&tone[1]==69&&tone[2]==250);
        int[] melody=stats(NokiaTone.ota(ota()));check(melody[0]==1&&melody[1]==81&&melody[2]==60000/63);
        byte[] sequence={-2,1,-3,30,-4,64,60,16,-1,8,-9,3,64,8};
        int[] seq=stats(ToneSequence.convert(sequence));check(seq[0]==4&&seq[1]==64&&seq[2]==1500);
        // A block is defined once and played twice, not played at definition.
        byte[] block={-2,1,-5,0,67,16,-6,0,-7,0,-7,0};
        int[] blocks=stats(ToneSequence.convert(block));check(blocks[0]==2&&blocks[2]==1000);
        byte[][] invalid={{},{-2,2},{-2,1,-9,1,60,1},{-2,1,-5,0,-7,0,-6,0,-7,0},{-2,1,-7,0},{-2,1,60,0}};
        for(int i=0;i<invalid.length;i++){boolean rejected=false;try{ToneSequence.convert(invalid[i]);}catch(IllegalArgumentException e){rejected=true;}check(rejected);}
        byte[] ota=ota();for(int i=0;i<ota.length-1;i++){byte[] truncated=new byte[i];System.arraycopy(ota,0,truncated,0,i);boolean rejected=false;try{NokiaTone.ota(truncated);}catch(IllegalArgumentException e){rejected=true;}check(rejected);}
        int[] single=stats(ToneSequence.single(127,1000,100));check(single[1]==127&&single[2]==1000);
        System.out.println("PS3ME_NOKIA_TONE_OK checks="+checks);
    }
}
