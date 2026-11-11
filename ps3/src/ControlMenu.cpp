// SPDX-License-Identifier: GPL-2.0-only
#include "ControlMenu.hpp"
#include "Controls.hpp"
#include "Ps3Backend.hpp"
#include "MenuLanguage.hpp"
#include "MenuPrompt.hpp"
#include "LocalizedControls.hpp"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_gfxPrimitives.h>
#include <sys/systime.h>
#include <sysutil/sysutil.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <utility>

// Defined by XmbGate.cpp: true while the system (XMB) owns the display.
extern "C" int ps3me_xmb_open(void);
extern "C" void ps3me_display_tick(void);
extern "C" void ps3me_pump_sysutil(void);

namespace ps3me {
namespace {
const char uiRoot[]="/dev_hdd0/game/PS3ME0001/USRDIR/ui";
const int keypadSourceWidth=284,keypadSourceHeight=214;

struct KeyRect { int x,y,w,h; };
struct CachedText { SDL_Surface* surface; CachedText():surface(0){} };
std::map<std::string,CachedText> textCache;
SDL_Surface* background=0;
SDL_Surface* keypad=0;
SDL_Surface* scaledKeypad=0;
int scaledKeypadWidth=0,scaledKeypadHeight=0;
bool attemptedKeypadLoad=false;

void uiText(SDL_Surface* target,TTF_Font* font,int x,int y,const std::string& value){
    if(!target)return;
    if(!font){stringRGBA(target,x,y,value.c_str(),238,242,255,255);return;}
    char prefix[40];::snprintf(prefix,sizeof(prefix),"%p|",static_cast<void*>(font));
    const std::string key=std::string(prefix)+value;
    std::map<std::string,CachedText>::iterator found=textCache.find(key);
    if(found==textCache.end()){
        CachedText cached;SDL_Color color={238,242,255,0};
        cached.surface=TTF_RenderUTF8_Blended(font,value.c_str(),color);
        if(textCache.size()>=256){
            for(std::map<std::string,CachedText>::iterator it=textCache.begin();it!=textCache.end();++it)
                if(it->second.surface)SDL_FreeSurface(it->second.surface);
            textCache.clear();
        }
        found=textCache.insert(std::make_pair(key,cached)).first;
    }
    if(found->second.surface){SDL_Rect destination={(Sint16)x,(Sint16)y,0,0};SDL_BlitSurface(found->second.surface,0,target,&destination);}
}

void clearTextCache(){
    for(std::map<std::string,CachedText>::iterator it=textCache.begin();it!=textCache.end();++it)
        if(it->second.surface)SDL_FreeSurface(it->second.surface);
    textCache.clear();
}

SDL_Surface* controlBackground(){
    if(background)return background;
    background=SDL_CreateRGBSurface(SDL_SWSURFACE,1280,720,32,0xff0000,0xff00,0xff,0);
    if(!background)return 0;
    for(int y=0;y<720;++y)
        hlineRGBA(background,0,1279,y,12+y*10/720,24+y*8/720,42+y*13/720,255);
    for(int layer=0;layer<2;++layer)for(int x=0;x<1280;x+=3){
        int y=410+layer*52+(int)(30.0f*std::sin(x/450.0f+layer));
        vlineRGBA(background,x,y,std::min(719,y+130),70,135,213,15);
    }
    return background;
}

SDL_Surface* controlKeypad(){
    if(attemptedKeypadLoad)return keypad;
    attemptedKeypadLoad=true;
    const std::string path=std::string(uiRoot)+"/lg_c397_keypad.png";
    SDL_Surface* raw=IMG_Load(path.c_str());
    if(!raw)return 0;
    keypad=SDL_DisplayFormatAlpha(raw);
    if(!keypad)keypad=SDL_ConvertSurface(raw,raw->format,SDL_SWSURFACE);
    SDL_FreeSurface(raw);
    if(keypad)SDL_SetAlpha(keypad,SDL_SRCALPHA,255);
    return keypad;
}

bool actionRectangle(int action,KeyRect& result){
    // Bounds are in the cropped 284x214 LG C397 keypad asset.
    switch(action){
        case KEY_UP:    result={121,4,42,18};return true;
        case KEY_DOWN:  result={121,57,42,18};return true;
        case KEY_LEFT:  result={101,22,22,36};return true;
        case KEY_RIGHT: result={161,22,22,36};return true;
        case FIRE:      result={119,22,46,36};return true;
        case SOFT1:     result={61,9,38,23};return true;
        case SOFT2:     result={184,9,38,23};return true;
        case NUM1:      result={36,78,29,30};return true;
        case NUM2:      result={64,78,28,30};return true;
        case NUM3:      result={91,78,28,30};return true;
        case NUM4:      result={65,111,29,30};return true;
        case NUM5:      result={92,111,29,30};return true;
        case NUM6:      result={119,111,29,30};return true;
        case NUM7:      result={65,145,29,30};return true;
        case NUM8:      result={92,145,29,30};return true;
        case NUM9:      result={119,145,29,30};return true;
        case NUM0:      result={76,181,33,29};return true;
        case STAR:      result={198,181,34,29};return true;
        case POUND:     result={11,77,27,32};return true;
        default:return false;
    }
}

void drawActionOverlay(SDL_Surface* target,const SDL_Rect& destination,int action){
    KeyRect keyRect;
    if(action==TOUCH){
        int left=destination.x+12,top=destination.y+destination.h-31;
        roundedBoxRGBA(target,left,top,destination.x+destination.w-12,top+22,7,18,72,133,238);
        roundedRectangleRGBA(target,left,top,destination.x+destination.w-12,top+22,7,113,190,255,255);
        stringRGBA(target,left+9,top+7,"TOUCH / RIGHT STICK + R2",236,246,255,255);
        return;
    }
    if(!actionRectangle(action,keyRect))return;
    const int x1=destination.x+keyRect.x*destination.w/keypadSourceWidth;
    const int y1=destination.y+keyRect.y*destination.h/keypadSourceHeight;
    const int x2=destination.x+(keyRect.x+keyRect.w)*destination.w/keypadSourceWidth;
    const int y2=destination.y+(keyRect.y+keyRect.h)*destination.h/keypadSourceHeight;
    boxRGBA(target,x1,y1,x2,y2,40,139,255,96);
    roundedRectangleRGBA(target,x1,y1,x2,y2,4,115,190,255,255);
    roundedRectangleRGBA(target,x1+1,y1+1,x2-1,y2-1,4,31,103,204,230);
}

const char* facePrompt(int button){
    switch(button){case CROSS:return "X";case CIRCLE:return "O";case SQUARE:return "SQUARE";case TRIANGLE:return "TRIANGLE";default:return 0;}
}
}

void drawControlDiagram(SDL_Surface* target,int x,int y,int width,int height,int selectedAction){
    if(!target||width<80||height<80)return;
    roundedBoxRGBA(target,x,y,x+width,y+height,13,10,17,28,255);
    roundedRectangleRGBA(target,x,y,x+width,y+height,13,83,117,166,255);
    const int inset=14;
    const int maxWidth=width-inset*2,maxHeight=height-inset*2;
    SDL_Surface* image=controlKeypad();
    if(!image){
        roundedBoxRGBA(target,x+inset,y+inset,x+width-inset,y+height-inset,10,28,37,53,255);
        stringRGBA(target,x+inset+20,y+inset+30,"LG C397 keypad image unavailable",180,205,233,255);
        return;
    }
    const double scale=std::min(maxWidth/(double)image->w,maxHeight/(double)image->h);
    const int drawWidth=std::max(1,(int)(image->w*scale));
    const int drawHeight=std::max(1,(int)(image->h*scale));
    const int drawX=x+(width-drawWidth)/2;
    const int drawY=y+(height-drawHeight)/2;
    if(!scaledKeypad||scaledKeypadWidth!=drawWidth||scaledKeypadHeight!=drawHeight){
        if(scaledKeypad)SDL_FreeSurface(scaledKeypad);
        scaledKeypad=SDL_CreateRGBSurface(SDL_SWSURFACE,drawWidth,drawHeight,32,0xff0000,0xff00,0xff,0xff000000);
        scaledKeypadWidth=drawWidth;scaledKeypadHeight=drawHeight;
        if(scaledKeypad){SDL_Rect targetRect={0,0,(Uint16)drawWidth,(Uint16)drawHeight};SDL_SoftStretch(image,0,scaledKeypad,&targetRect);SDL_SetAlpha(scaledKeypad,SDL_SRCALPHA,255);}
    }
    SDL_Rect destination={(Sint16)drawX,(Sint16)drawY,(Uint16)drawWidth,(Uint16)drawHeight};
    if(scaledKeypad){SDL_BlitSurface(scaledKeypad,0,target,&destination);drawActionOverlay(target,destination,selectedAction);}
}

void showControlMenu(Controller& pad,bool automated){
    ControlConfig original=controlConfig();
    ControlConfig& config=controlConfig();
    const MenuLanguage originalLanguage=menuLanguage();
    SDL_Surface* screen=SDL_GetVideoSurface();
    SDL_Surface* surface=SDL_CreateRGBSurface(SDL_SWSURFACE,1280,720,32,0xff0000,0xff00,0xff,0);
    if(!surface||!screen){if(surface)SDL_FreeSurface(surface);return;}
    TTF_Init();
    TTF_Font* font=TTF_OpenFont((std::string(uiRoot)+"/DejaVuSans.ttf").c_str(),18);
    TTF_Font* title=TTF_OpenFont((std::string(uiRoot)+"/DejaVuSans.ttf").c_str(),29);
    ps2::hal::PadButtons previous;pad.read(&previous);
    unsigned testFrame=0;
    int row=0;
    const int rows=BUTTON_COUNT+4;
    bool done=false,saved=false;
    std::string message=tr("As configurações afetam todos os jogos.","Settings apply to all games.");
    while(!done){
        ps3me_pump_sysutil();
        SDL_Event event;
        while(SDL_PollEvent(&event))if(event.type==SDL_QUIT)done=true;
        ps2::hal::PadButtons buttons;pad.read(&buttons);
        if(ps3me_xmb_open()){
            ps3me_display_tick();previous=buttons;sysUsleep(16000);continue;
        }
        if(automated){
            buttons=ps2::hal::PadButtons();++testFrame;
            if(testFrame==2||testFrame==4||testFrame==6||testFrame==8)buttons.down=true;
            if(testFrame>=12&&testFrame<=26&&testFrame%2==0)buttons.right=true;
            if(testFrame==32)buttons.circle=true;
            if(testFrame>80)done=true;
        }
        if(buttons.up&&!previous.up&&row>0)--row;
        if(buttons.down&&!previous.down&&row<rows-1)++row;
        int delta=((buttons.right&&!previous.right)||(buttons.cross&&!previous.cross))?1:0;
        if(buttons.left&&!previous.left)delta=-1;
        if(delta){
            if(row<BUTTON_COUNT)config.map[row]=(config.map[row]+ACTION_COUNT+delta)%ACTION_COUNT;
            else if(row==BUTTON_COUNT)config.pointer=!config.pointer;
            else if(row==BUTTON_COUNT+1){
                config.speed+=delta*30;if(config.speed<60)config.speed=600;if(config.speed>600)config.speed=60;
            }else if(row==BUTTON_COUNT+2){
                config.deadzone+=delta*4;if(config.deadzone<12)config.deadzone=64;if(config.deadzone>64)config.deadzone=12;
            }else menuLanguage()=menuLanguage()==PT_BR?ENGLISH:PT_BR;
        }
        if(buttons.triangle&&!previous.triangle){
            config=ControlConfig();
            message=tr("Padrões restaurados. O salva; START cancela.","Defaults restored. Circle saves; Start cancels.");
        }
        if(buttons.start&&!previous.start)done=true;
        if(buttons.circle&&!previous.circle){
            if(config.save("/dev_hdd0/game/PS3ME0001/USRDIR/config/controls.cfg")&&saveMenuLanguage()){
                saved=true;done=true;
            }else message=tr("Falha ao salvar. O tenta novamente; START cancela.",
                            "Could not save. Circle retries; Start cancels.");
        }

        SDL_Surface* bg=controlBackground();if(bg)SDL_BlitSurface(bg,0,surface,0);
        else SDL_FillRect(surface,0,SDL_MapRGB(surface->format,18,23,34));
        uiText(surface,title,42,27,tr("PS3ME / CONFIGURAÇÃO DOS CONTROLES","PS3ME / CONTROLLER MAPPING"));
        uiText(surface,font,570,78,tr("MAPEAMENTO DO CONTROLE","CONTROLLER MAPPING"));
        const int action=config.map[row<BUTTON_COUNT?row:CROSS];
        drawControlDiagram(surface,38,132,500,430,row<BUTTON_COUNT?action:NONE);
        uiText(surface,font,58,573,tr("A tecla destacada em azul é a ação J2ME selecionada.","The key highlighted in blue is the selected J2ME action."));
        for(int i=0;i<rows;++i){
            const int y=112+i*25;
            if(i==row)roundedBoxRGBA(surface,565,y-2,1234,y+22,6,35,78,123,245);
            if(i<BUTTON_COUNT){
                const char* face=facePrompt(i);
                if(face){drawPrompt(surface,579,y,face,19);uiText(surface,font,615,y+1,localizedButton(i));}
                else uiText(surface,font,584,y+1,localizedButton(i));
                uiText(surface,font,838,y+1,localizedAction(config.map[i]));
            }else{
                char line[150];
                if(i==BUTTON_COUNT)::snprintf(line,sizeof(line),"%s: %s",tr("Cursor do analógico direito","Right-stick cursor"),config.pointer?tr("Ativado","Enabled"):tr("Desativado","Disabled"));
                else if(i==BUTTON_COUNT+1)::snprintf(line,sizeof(line),"%s: %d",tr("Velocidade do cursor","Cursor speed"),config.speed);
                else if(i==BUTTON_COUNT+2)::snprintf(line,sizeof(line),"%s: %d",tr("Zona morta","Stick deadzone"),config.deadzone);
                else::snprintf(line,sizeof(line),"%s: %s",tr("Idioma","Language"),languageName());
                uiText(surface,font,584,y+1,line);
            }
        }
        uiText(surface,font,584,590,message);
        hlineRGBA(surface,42,1238,620,130,167,207,100);
        drawPrompt(surface,54,635,"X",22);uiText(surface,font,86,636,tr("Alterar","Change"));
        drawPrompt(surface,236,635,"O",22);uiText(surface,font,268,636,tr("Salvar","Save"));
        drawPrompt(surface,407,635,"TRIANGLE",22);uiText(surface,font,480,636,tr("Restaurar","Reset"));
        drawPrompt(surface,710,635,"START",22);uiText(surface,font,775,636,tr("Cancelar","Cancel"));
        uiText(surface,font,54,682,tr("Cima/Baixo: escolher controle   Esquerda/Direita: escolher tecla",
                                          "Up/Down: choose control   Left/Right: choose key"));
        if(automated&&testFrame==30)SDL_SaveBMP(surface,"/dev_hdd0/game/PS3ME0001/USRDIR/logs/controls_menu_test.bmp");
        SDL_SoftStretch(surface,0,screen,0);SDL_Flip(screen);
        previous=buttons;sysUsleep(16000);
    }
    if(!saved){config=original;menuLanguage()=originalLanguage;}
    if(automated)std::fprintf(stderr,"[PS3ME controls test] menu returned Cross=%s\n",actionName(config.map[CROSS]));
    clearTextCache();
    if(title)TTF_CloseFont(title);
    if(font)TTF_CloseFont(font);
    SDL_FreeSurface(surface);
}
}
