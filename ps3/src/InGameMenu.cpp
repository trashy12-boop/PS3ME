// SPDX-License-Identifier: GPL-2.0-only
#include "GameSettings.hpp"
#include "ControlMenu.hpp"
#include "MenuLanguage.hpp"
#include "LocalizedControls.hpp"
#include "MenuPrompt.hpp"
#include "hal/LcdDevice.hpp"
#include "hal/Keypad.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_gfxPrimitives.h>
#include <sysutil/sysutil.h>
#include <sys/systime.h>
#include <string>
#include <cstdio>
#include <stdio.h>
extern "C" {
#include <javacall_lcd.h>
}
// Defined by XmbGate.cpp: true while the system (XMB) owns the display,
// when no frame may be presented.
extern "C" int ps3me_xmb_open(void);
extern "C" void ps3me_display_tick(void);
extern "C" void ps3me_pump_sysutil(void);

namespace ps3me {
static bool active=false;bool overlayActive(){return active;}
static void text(SDL_Surface* s,TTF_Font* font,int x,int y,const std::string& value){if(!font){stringRGBA(s,x,y,value.c_str(),240,245,255,255);return;}SDL_Color c={235,244,255,0};SDL_Surface* t=TTF_RenderUTF8_Blended(font,value.c_str(),c);
 if(t){SDL_Rect r={(Sint16)x,(Sint16)y,0,0};SDL_BlitSurface(t,0,s,&r);SDL_FreeSurface(t);}}
int showInGameMenu(bool automated){
 ps2::hal::IPad* pad=ps2::hal::Keypad::instance().pad();
 if(!pad)return 0;
 SDL_Surface* screen=SDL_GetVideoSurface();SDL_Surface* surface=SDL_CreateRGBSurface(SDL_SWSURFACE,1280,720,32,0xff0000,0xff00,0xff,0);
 if(!surface)return 0;
 active=true;SDL_PauseAudio(1);bool ownTtf=TTF_WasInit()==0;if(ownTtf)TTF_Init();TTF_Font* title=TTF_OpenFont("/dev_hdd0/game/PS3ME0001/USRDIR/ui/DejaVuSans.ttf",32);TTF_Font* font=TTF_OpenFont("/dev_hdd0/game/PS3ME0001/USRDIR/ui/DejaVuSans.ttf",23);
 auto& lcd=ps2::hal::LcdDevice::instance();int w=lcd.width(),h=lcd.height(),originalW=w,originalH=h,row=0,result=0;bool done=false;unsigned frames=0;ps2::hal::PadButtons previous;pad->read(&previous);std::string message=tr("Alterações são aplicadas ao voltar. Salve para este jogo para mantê-las.","Changes apply on Resume. Save for this game to keep them.");
 while(!done){ps3me_pump_sysutil();SDL_Event event;while(SDL_PollEvent(&event))if(event.type==SDL_QUIT){result=1;done=true;}ps2::hal::PadButtons b;pad->read(&b);
  // The XMB owns the display while it is open: no presentation, no edges.
  if(ps3me_xmb_open()){
   ps3me_display_tick();
   previous=b;sysUsleep(16000);continue;
  }
  if(automated){b=ps2::hal::PadButtons();++frames;
 if(frames==2)b.right=true;
 if(frames>=4&&frames<=12&&frames%2==0)b.down=true;
 if(frames==14)b.right=true;
 if(frames>=16&&frames<=22&&frames%2==0)b.up=true;
 if(frames==24)b.cross=true;
 if(frames==26)b.circle=true;}
  if(b.up&&!previous.up&&row>0)--row;
 if(b.down&&!previous.down&&row<BUTTON_COUNT+7)++row;
  int delta=((b.right&&!previous.right)||(b.cross&&!previous.cross))?1:0;
 if(b.left&&!previous.left)delta=-1;
  if(delta){if(row==0){const int dims[][2]={{176,220},{240,320},{320,240},{320,480},{352,416},{480,800}};int i=0;for(int n=0;n<6;n++)if(w==dims[n][0]&&h==dims[n][1])i=n;i=(i+6+delta)%6;w=dims[i][0];h=dims[i][1];}
   else if(row==1){message=gameSettings().save(controlConfig(),w,h)?tr("Salvo apenas para este jogo.","Saved for this game only."):tr("Não foi possível salvar. As alterações continuam nesta sessão.","Save failed. Settings remain active for this session.");}
   else if(row==2){message=gameSettings().useGlobal(controlConfig(),w,h)?tr("Padrões globais restaurados.","Global defaults restored; game overrides removed."):tr("Não foi possível remover as configurações do jogo.","Could not remove game override.");}
   else if(row==3)done=true;else if(row==4){result=1;done=true;}
   else if(row<5+BUTTON_COUNT){int& action=controlConfig().map[row-5];action=(action+ACTION_COUNT+delta)%ACTION_COUNT;}
   else if(row==5+BUTTON_COUNT)controlConfig().pointer=!controlConfig().pointer;
   else if(row==6+BUTTON_COUNT){int& speed=controlConfig().speed;speed+=delta*30;
 if(speed<60)speed=600;
 if(speed>600)speed=60;}
   else {int& zone=controlConfig().deadzone;zone+=delta*4;
 if(zone<12)zone=64;
 if(zone>64)zone=12;}
  }
  if((b.circle&&!previous.circle)||(b.start&&!previous.start))done=true;
  for(int y=0;y<720;y++)hlineRGBA(surface,0,1279,y,23+y*17/720,64-y*45/720,91-y*26/720,255);
  text(surface,title,58,34,tr("PS3ME / Configurações do jogo","PS3ME / In-game settings"));text(surface,font,58,89,tr("L3 + R3   /   Configurações deste jogo","L3 + R3   /   Settings for this game"));
  const int selectedAction=(row>=5&&row<5+BUTTON_COUNT)?controlConfig().map[row-5]:NONE;
  drawControlDiagram(surface,48,146,470,396,selectedAction);
  text(surface,font,72,553,tr("Ação J2ME em azul","J2ME action highlighted in blue"));
  int first=row>3?row-3:0;
  for(int i=first;i<BUTTON_COUNT+8&&i<first+8;i++){int y=176+(i-first)*46;std::string label;char value[180];
 if(i==0){::snprintf(value,sizeof(value),tr("Resolução do jogo: %d x %d","Game resolution: %d x %d"),w,h);label=value;}else if(i==1)label=tr("Salvar configurações deste jogo","Save settings for this game");else if(i==2)label=tr("Usar configurações globais","Use global defaults");else if(i==3)label=tr("Voltar ao jogo","Resume game");else if(i==4)label=tr("Voltar à biblioteca","Return to game library");else if(i<5+BUTTON_COUNT){::snprintf(value,sizeof(value),"%-12s   %s",localizedButton(i-5),localizedAction(controlConfig().map[i-5]));label=value;}else if(i==5+BUTTON_COUNT)label=std::string(tr("Cursor de toque: ","Touchscreen cursor: "))+(controlConfig().pointer?tr("Ativado","Enabled"):tr("Desativado","Disabled"));else{::snprintf(value,sizeof(value),"%s: %d",i==6+BUTTON_COUNT?tr("Velocidade do cursor","Cursor speed"):tr("Zona morta","Stick deadzone"),i==6+BUTTON_COUNT?controlConfig().speed:controlConfig().deadzone);label=value;}if(i==row)roundedBoxRGBA(surface,548,y-5,1225,y+35,6,36,79,125,245);text(surface,font,570,y,label);}
  text(surface,font,80,586,message);text(surface,font,80,640,tr("Cima/Baixo: selecionar   Direita/Esquerda ou X: alterar   O: voltar","Up/Down: select   Left/Right or X: change   Circle: resume"));text(surface,font,80,681,tr("Alguns jogos precisam reiniciar após mudar a resolução.","Some games need a restart to adapt their own graphics to a new size."));SDL_SoftStretch(surface,0,screen,0);SDL_Flip(screen);previous=b;sysUsleep(16000);
  if(automated&&frames==25)SDL_SaveBMP(surface,"/dev_hdd0/game/PS3ME0001/USRDIR/logs/ingame_menu.bmp");
 }
 if(w!=originalW||h!=originalH){lcd.setResolution(w,h);javanotify_rotation(lcd.PRIMARY_ID);std::fprintf(stderr,"[PS3ME overlay] resize %dx%d -> %dx%d\n",originalW,originalH,w,h);}
 if(title){TTF_CloseFont(title);}

 if(font){TTF_CloseFont(font);}
SDL_FreeSurface(surface);if(ownTtf)TTF_Quit();lcd.flush();active=false;SDL_PauseAudio(0);std::fprintf(stderr,"[PS3ME overlay] resumed exit=%d\n",result);return result;
}
}
