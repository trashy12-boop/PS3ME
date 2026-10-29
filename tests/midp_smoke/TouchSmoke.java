// SPDX-License-Identifier: GPL-2.0-only
import javax.microedition.midlet.MIDlet;
import javax.microedition.lcdui.*;
public class TouchSmoke extends MIDlet {
 protected void startApp(){Display.getDisplay(this).setCurrent(new Screen());}protected void pauseApp(){}protected void destroyApp(boolean b){}
 static class Screen extends Canvas {
  private String status="Right stick moves; R2 touches";private int x=120,y=160;
  Screen(){setFullScreenMode(true);System.out.println("[PS3ME TOUCH TEST] supported="+hasPointerEvents()+" motion="+hasPointerMotionEvents());}
  protected void sizeChanged(int w,int h){System.out.println("[PS3ME RESIZE TEST] "+w+"x"+h);}
  protected void paint(Graphics g){g.setColor(0x172537);g.fillRect(0,0,getWidth(),getHeight());g.setColor(0xffffff);g.drawString("PS3ME controls test",8,15,Graphics.TOP|Graphics.LEFT);g.drawString(status,8,40,Graphics.TOP|Graphics.LEFT);g.setColor(0x00ff88);g.fillRect(x-3,y-3,7,7);}
  protected void pointerPressed(int a,int b){x=a;y=b;status="Touch pressed";System.out.println("[PS3ME TOUCH TEST] pressed "+a+","+b);repaint();}
  protected void pointerDragged(int a,int b){x=a;y=b;status="Touch dragged";System.out.println("[PS3ME TOUCH TEST] dragged "+a+","+b);repaint();}
  protected void pointerReleased(int a,int b){x=a;y=b;status="Touch released";System.out.println("[PS3ME TOUCH TEST] released "+a+","+b);repaint();}
  protected void keyPressed(int k){status="Key "+k;System.out.println("[PS3ME TOUCH TEST] key pressed "+k);repaint();}
  protected void keyReleased(int k){System.out.println("[PS3ME TOUCH TEST] key released "+k);}
 }
}
