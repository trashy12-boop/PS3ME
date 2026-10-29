/* SPDX-License-Identifier: GPL-2.0-only */
import javax.microedition.midlet.MIDlet;
import javax.microedition.lcdui.*;
import javax.microedition.rms.*;
/** Console diagnostic: actual Canvas/input/RMS paths, not host mock APIs. */
public final class PS3MESmoke extends MIDlet {
    private Screen screen;
    protected void startApp(){
        System.out.println("[PS3ME diagnostic] startApp");
        if(screen==null){
            long literal=1000L;
            if(literal!=(long)1000 || 3.25d+2.5d!=5.75d)throw new RuntimeException("wide constants failed");
            long before=System.currentTimeMillis();
            try{Thread.sleep(25L);}catch(InterruptedException e){}
            long elapsed=System.currentTimeMillis()-before;
            if(elapsed<1 || elapsed>10000)throw new RuntimeException("sleep/time failed");
            System.out.println("[PS3ME diagnostic] wide constants and sleep passed elapsed="+elapsed);
            screen=new Screen();
            int checksum=0;
            for(int i=0;i<256;++i){byte[] garbage=new byte[65536];garbage[0]=(byte)(i&127);checksum+=garbage[0];}
            System.gc();
            if(checksum!=16256)throw new RuntimeException("allocation checksum failed");
            System.out.println("[PS3ME diagnostic] allocation and GC passed");
        }
        Display.getDisplay(this).setCurrent(screen);
    }
    protected void pauseApp(){}
    protected void destroyApp(boolean unconditional){}
    static final class Screen extends Canvas {
        private int boots,key;
        private boolean paintReported;
        private String state="RMS not checked";
        Screen(){
            try {
                RecordStore rs=RecordStore.openRecordStore("ps3me-smoke",true);
                if(rs.getNumRecords()>0){byte[] b=rs.getRecord(1);boots=((b[0]&255)<<24)|((b[1]&255)<<16)|((b[2]&255)<<8)|(b[3]&255);}
                ++boots;
                byte[] b={(byte)(boots>>24),(byte)(boots>>16),(byte)(boots>>8),(byte)boots};
                if(rs.getNumRecords()>0)rs.setRecord(1,b,0,4);else rs.addRecord(b,0,4);
                rs.closeRecordStore();state="RMS opened, wrote, closed";
                System.out.println("[PS3ME diagnostic] RMS saved launches="+boots);
            } catch(Throwable t){state="RMS ERROR: "+t;System.out.println("[PS3ME diagnostic] "+state);t.printStackTrace();}
        }
        protected void paint(Graphics g){
            int w=getWidth(),h=getHeight();g.setColor(0x101820);g.fillRect(0,0,w,h);
            g.setColor(0xff0000);g.fillRect(8,16,w/3-8,40);
            g.setColor(0x00ff00);g.fillRect(w/3,16,w/3-1,40);
            g.setColor(0x0000ff);g.fillRect(2*w/3,16,w/3-8,40);
            g.setColor(0xffffff);
            g.drawString("PS3ME MIDP diagnostic",8,72,Graphics.TOP|Graphics.LEFT);
            g.drawString(w+" x "+h,8,96,Graphics.TOP|Graphics.LEFT);
            g.drawString("Saved launches: "+boots,8,120,Graphics.TOP|Graphics.LEFT);
            g.drawString("Key: "+key,8,144,Graphics.TOP|Graphics.LEFT);
            g.drawString(state,8,168,Graphics.TOP|Graphics.LEFT);
            g.drawString("Start: return to native menu",8,h-28,Graphics.TOP|Graphics.LEFT);
            if(!paintReported){paintReported=true;System.out.println("[PS3ME diagnostic] Canvas paint completed "+w+"x"+h);}
        }
        protected void keyPressed(int code){key=code;repaint();}
    }
}
