// SPDX-License-Identifier: GPL-2.0-only
#include "ControlMenu.hpp"
#include "Controls.hpp"
#include "Ps3Backend.hpp"
#include <SDL_gfxPrimitives.h>
#include <sys/systime.h>
#include <sysutil/sysutil.h>
#include <cstdio>
#include <string>
namespace ps3me {
void showControlMenu(Controller& pad,bool automated){
 ControlConfig original=controlConfig();

ControlConfig& c=controlConfig();

SDL_Surface* screen=SDL_GetVideoSurface();

SDL_Surface* surface=SDL_CreateRGBSurface(SDL_SWSURFACE,640,360,32,0xff0000,0xff00,0xff,0);

if(!surface)return;


 ps2::hal::PadButtons previous;

pad.read(&previous);

unsigned testFrame=0;
int row=0;

bool done=false;

std::string message="Settings apply to all games. Start always returns to menu.";


 while(!done){sysUtilCheckCallback();

SDL_Event e;

while(SDL_PollEvent(&e))if(e.type==SDL_QUIT){c=original;

done=true;

}
  ps2::hal::PadButtons b;

pad.read(&b);

if(automated){b=ps2::hal::PadButtons();
++testFrame;
if(testFrame==2||testFrame==4||testFrame==6||testFrame==8)b.down=true;
if(testFrame>=12&&testFrame<=26&&testFrame%2==0)b.right=true;
if(testFrame==32)b.circle=true;
if(testFrame>80){c=original;
done=true;
}}
if(b.up&&!previous.up&&row>0)--row;

if(b.down&&!previous.down&&row<BUTTON_COUNT+2)++row;


  int delta=((b.right&&!previous.right)||(b.cross&&!previous.cross))?1:0;

if(b.left&&!previous.left)delta=-1;


  if(delta){if(row<BUTTON_COUNT)c.map[row]=(c.map[row]+ACTION_COUNT+delta)%ACTION_COUNT;

else if(row==BUTTON_COUNT)c.pointer=!c.pointer;

else if(row==BUTTON_COUNT+1){c.speed+=delta*30;

if(c.speed<60)c.speed=600;

if(c.speed>600)c.speed=60;

}else {c.deadzone+=delta*4;

if(c.deadzone<12)c.deadzone=64;

if(c.deadzone>64)c.deadzone=12;

}}
  if(b.triangle&&!previous.triangle){c=ControlConfig();

message="Defaults restored. Circle saves; Start cancels.";

}
  if(b.start&&!previous.start){c=original;

done=true;

}
  if(b.circle&&!previous.circle){if(c.save("/dev_hdd0/game/PS3ME0001/USRDIR/config/controls.cfg"))done=true;

else message="Could not save controls. Circle retries; Start cancels.";

}
  SDL_FillRect(surface,0,SDL_MapRGB(surface->format,18,23,34));

stringRGBA(surface,18,16,"CONTROLS / TOUCHSCREEN",100,190,255,255);


  int first=row>15?row-15:0;


  for(int i=first;

i<BUTTON_COUNT+3&&i<first+16;

i++){int y=43+(i-first)*15;

if(i==row)boxRGBA(surface,12,y-3,625,y+10,39,72,106,255);

char text[100];


   if(i<BUTTON_COUNT)::snprintf(text,sizeof(text),"%-18s %s",buttonName(i),actionName(c.map[i]));

else if(i==BUTTON_COUNT)::snprintf(text,sizeof(text),"Right-stick cursor %s",c.pointer?"Enabled":"Disabled");

else if(i==BUTTON_COUNT+1)::snprintf(text,sizeof(text),"Cursor speed       %d",c.speed);

else ::snprintf(text,sizeof(text),"Stick deadzone     %d",c.deadzone);


   stringRGBA(surface,22,y,text,235,235,235,255);


  }
  stringRGBA(surface,18,292,"Up/Down: select   Left/Right or X: change",235,235,235,255);


  stringRGBA(surface,18,309,"Circle: save   Start: cancel   Triangle: defaults",235,235,235,255);


  stringRGBA(surface,18,331,message.c_str(),220,190,130,255);


  if(automated&&testFrame==30)SDL_SaveBMP(surface,"/dev_hdd0/game/PS3ME0001/USRDIR/logs/controls_menu_test.bmp");

SDL_SoftStretch(surface,0,screen,0);

SDL_Flip(screen);

previous=b;

sysUsleep(16000);


 }
 if(automated)std::fprintf(stderr,"[PS3ME controls test] menu returned Cross=%s\n",actionName(c.map[CROSS]));

 SDL_FreeSurface(surface);


}
}
