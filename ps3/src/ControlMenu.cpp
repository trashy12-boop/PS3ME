// SPDX-License-Identifier: GPL-2.0-only
#include "ControlMenu.hpp"
#include "Controls.hpp"
#include "Ps3Backend.hpp"
#include "MenuLanguage.hpp"
#include "MenuPrompt.hpp"
#include "LocalizedControls.hpp"
#include <SDL_ttf.h>
#include <SDL_gfxPrimitives.h>
#include <sys/systime.h>
#include <sysutil/sysutil.h>
#include <cstdio>
#include <string>

// Defined by XmbGate.cpp: true while the system (XMB) owns the display,
// when no frame may be presented.
extern "C" int ps3me_xmb_open(void);
extern "C" void ps3me_display_tick(void);
extern "C" void ps3me_pump_sysutil(void);


namespace ps3me {
namespace {
void uiText(SDL_Surface* s,TTF_Font* font,int x,int y,const std::string& value){
    if(!font){stringRGBA(s,x,y,value.c_str(),238,242,255,255);return;}
    SDL_Color c={238,242,255,0};
    SDL_Surface* rendered=TTF_RenderUTF8_Blended(font,value.c_str(),c);
    if(rendered){SDL_Rect dest={(Sint16)x,(Sint16)y,0,0};SDL_BlitSurface(rendered,0,s,&dest);SDL_FreeSurface(rendered);}
}
}
void showControlMenu(Controller& pad,bool automated){
    ControlConfig original=controlConfig();
    ControlConfig& c=controlConfig();
    const MenuLanguage originalLanguage=menuLanguage();
    SDL_Surface* screen=SDL_GetVideoSurface();
    SDL_Surface* surface=SDL_CreateRGBSurface(SDL_SWSURFACE,640,360,32,0xff0000,0xff00,0xff,0);
    if(!surface||!screen){if(surface)SDL_FreeSurface(surface);return;}
    TTF_Init();
    TTF_Font* font=TTF_OpenFont("/dev_hdd0/game/PS3ME0001/USRDIR/ui/DejaVuSans.ttf",14);
    TTF_Font* title=TTF_OpenFont("/dev_hdd0/game/PS3ME0001/USRDIR/ui/DejaVuSans.ttf",19);
    ps2::hal::PadButtons previous;pad.read(&previous);
    unsigned testFrame=0;
    int row=0;
    const int rows=BUTTON_COUNT+4; // 15 buttons, pointer, speed, deadzone, language
    bool done=false,saved=false;
    std::string message=tr("As configurações afetam todos os jogos.","Settings apply to all games.");
    while(!done){
        ps3me_pump_sysutil();
        SDL_Event e;
        while(SDL_PollEvent(&e))if(e.type==SDL_QUIT)done=true;
        ps2::hal::PadButtons b;pad.read(&b);
        // The XMB owns the display while it is open: keep the pad state frozen
        // and the queue drained, but present nothing. Reseed once it is back.
        if(ps3me_xmb_open()){
            ps3me_display_tick();
            previous=b;sysUsleep(16000);continue;
        }
        if(automated){
            b=ps2::hal::PadButtons();++testFrame;
            if(testFrame==2||testFrame==4||testFrame==6||testFrame==8)b.down=true;
            if(testFrame>=12&&testFrame<=26&&testFrame%2==0)b.right=true;
            if(testFrame==32)b.circle=true;
            if(testFrame>80)done=true;
        }
        if(b.up&&!previous.up&&row>0)--row;
        if(b.down&&!previous.down&&row<rows-1)++row;
        int delta=((b.right&&!previous.right)||(b.cross&&!previous.cross))?1:0;
        if(b.left&&!previous.left)delta=-1;
        if(delta){
            if(row<BUTTON_COUNT)c.map[row]=(c.map[row]+ACTION_COUNT+delta)%ACTION_COUNT;
            else if(row==BUTTON_COUNT)c.pointer=!c.pointer;
            else if(row==BUTTON_COUNT+1){
                c.speed+=delta*30;
                if(c.speed<60)c.speed=600;
                if(c.speed>600)c.speed=60;
            }else if(row==BUTTON_COUNT+2){
                c.deadzone+=delta*4;
                if(c.deadzone<12)c.deadzone=64;
                if(c.deadzone>64)c.deadzone=12;
            }else menuLanguage()=menuLanguage()==PT_BR?ENGLISH:PT_BR;
        }
        if(b.triangle&&!previous.triangle){
            c=ControlConfig();
            message=tr("Padrões restaurados. O salva; START cancela.","Defaults restored. Circle saves; Start cancels.");
        }
        if(b.start&&!previous.start)done=true;
        if(b.circle&&!previous.circle){
            if(c.save("/dev_hdd0/game/PS3ME0001/USRDIR/config/controls.cfg")&&saveMenuLanguage()){
                saved=true;done=true;
            }else message=tr("Falha ao salvar. O tenta novamente; START cancela.",
                            "Could not save. Circle retries; Start cancels.");
        }
        SDL_FillRect(surface,0,SDL_MapRGB(surface->format,18,23,34));
        uiText(surface,title,18,12,tr("CONFIGURAÇÕES / CONTROLES","SETTINGS / CONTROLS"));
        int first=row>13?row-13:0;
        for(int i=first;i<rows&&i<first+15;i++){
            int y=44+(i-first)*15;
            if(i==row)boxRGBA(surface,12,y-2,625,y+13,39,72,106,255);
            char line[180];
            if(i<BUTTON_COUNT)
                ::snprintf(line,sizeof(line),"%-15s   %s",localizedButton(i),localizedAction(c.map[i]));
            else if(i==BUTTON_COUNT)
                ::snprintf(line,sizeof(line),"%s: %s",tr("Cursor do analógico direito","Right-stick cursor"),
                    c.pointer?tr("Ativado","Enabled"):tr("Desativado","Disabled"));
            else if(i==BUTTON_COUNT+1)
                ::snprintf(line,sizeof(line),"%s: %d",tr("Velocidade do cursor","Cursor speed"),c.speed);
            else if(i==BUTTON_COUNT+2)
                ::snprintf(line,sizeof(line),"%s: %d",tr("Zona morta","Stick deadzone"),c.deadzone);
            else
                ::snprintf(line,sizeof(line),"%s: %s",tr("Idioma","Language"),languageName());
            uiText(surface,font,22,y,line);
        }
        // Native button glyphs, matching the library's prompt treatment.
        drawPrompt(surface,18,287,"X",18);
        uiText(surface,font,44,288,tr("Alterar valor","Change value"));
        drawPrompt(surface,245,287,"O",18);
        uiText(surface,font,272,288,tr("Salvar","Save"));
        drawPrompt(surface,450,287,"START",18);
        uiText(surface,font,489,288,tr("Cancelar","Cancel"));
        uiText(surface,font,18,315,tr("Cima/Baixo: selecionar    Triângulo: restaurar controles",
                                   "Up/Down: select    Triangle: reset controls"));
        uiText(surface,font,18,336,message);
        if(automated&&testFrame==30)
            SDL_SaveBMP(surface,"/dev_hdd0/game/PS3ME0001/USRDIR/logs/controls_menu_test.bmp");
        SDL_SoftStretch(surface,0,screen,0);
        SDL_Flip(screen);
        previous=b;
        sysUsleep(16000);
    }
    if(!saved){c=original;menuLanguage()=originalLanguage;}
    if(automated)std::fprintf(stderr,"[PS3ME controls test] menu returned Cross=%s\n",actionName(c.map[CROSS]));
    if(title)TTF_CloseFont(title);
    if(font)TTF_CloseFont(font);
    SDL_FreeSurface(surface);
}
}
