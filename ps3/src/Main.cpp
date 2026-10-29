// SPDX-License-Identifier: GPL-2.0-only
#include "Ps3Backend.hpp"
#include "Controls.hpp"
#include "GameSettings.hpp"
#include "ControlMenu.hpp"
#include "BrowserUI.hpp"
#include "hal/LcdDevice.hpp"
#include <SDL_gfxPrimitives.h>
#include <sys/process.h>
#include <sys/systime.h>
#include <sysutil/sysutil.h>
SYS_PROCESS_PARAM(1001, 0x200000);
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <stdio.h>
#include <cstring>
#include <string>
#include <vector>
extern "C" int ps3me_run_game(const char*,const char*,const char*,const char*);
extern "C" void JVM_Stop(int);
static bool inGame=false;
static u64 diagnosticStopAt=0;
#ifdef PS3ME_RUNTIME_REPLAY
#include "../tests/RuntimeReplay.hpp"
#endif
extern "C" void ps3me_on_start(){if(inGame&&!ps3me::overlayActive())JVM_Stop(0);}
extern "C" void ps3me_service_app(){
    sysUtilCheckCallback();
#ifdef PS3ME_RUNTIME_REPLAY
    if(inGame)ps3me_runtime_replay();
#endif
    if(inGame&&diagnosticStopAt&&sysGetSystemTime()>=diagnosticStopAt){
        diagnosticStopAt=0;
        std::fprintf(stderr,"[PS3ME test] requesting return through Start handler\n");
        ps3me_on_start();
    }
}
namespace {
const char home[]="/dev_hdd0/game/PS3ME0001/USRDIR";
struct Entry{std::string name;bool directory;};
bool game(const std::string& name) {
    if(name.size()<5)return false;
    std::string end=name.substr(name.size()-4);
    for(size_t i=0;i<end.size();++i)end[i]=std::tolower(static_cast<unsigned char>(end[i]));
    return end==".jar"||end==".jad";
}
void directories(const std::string& path){for(size_t i=1;i<=path.size();++i)if(i==path.size()||path[i]=='/')::mkdir(path.substr(0,i).c_str(),0700);}
std::vector<Entry> list(const std::string& path) {
    std::vector<Entry> entries;DIR* dir=::opendir(path.c_str());if(!dir)return entries;
    struct dirent* d;
    while((d=::readdir(dir))&&entries.size()<1024) {
        if(!std::strcmp(d->d_name,".")||!std::strcmp(d->d_name,".."))continue;
        struct stat s;std::string p=path+"/"+d->d_name;
        if(::stat(p.c_str(),&s)!=0)continue;
        if(S_ISDIR(s.st_mode)||game(d->d_name)){Entry e;e.name=d->d_name;e.directory=S_ISDIR(s.st_mode);entries.push_back(e);}
    }
    ::closedir(dir);
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.directory!=b.directory?a.directory>b.directory:a.name<b.name;});
    return entries;
}
std::string key(const std::string& path) {
    uint64_t hash=14695981039346656037ULL;
    for(size_t i=0;i<path.size();++i){hash^=static_cast<unsigned char>(path[i]);hash*=1099511628211ULL;}
    char text[24];::snprintf(text,sizeof(text),"%016llx",static_cast<unsigned long long>(hash));return text;
}
void label(SDL_Surface* surface,int x,int y,std::string text,Uint8 r=235,Uint8 g=235,Uint8 b=235) {
    if(text.size()>73)text=text.substr(0,70)+"...";
    stringRGBA(surface,x,y,text.c_str(),r,g,b,255);
}
int launch(const std::string& chosen,int width,int height) {
    std::string db=std::string(home)+"/appdb/"+key(chosen)+"/";directories(db);
    ps3me::gameSettings().begin(chosen,std::string(home)+"/config/games",ps3me::controlConfig(),width,height);
    ps2::hal::LcdDevice::instance().setResolution(width,height);
    std::string config=std::string(home)+"/lib/",classes=config+"classes.zip",url="file://"+chosen;
    std::fprintf(stderr,"[PS3ME] loading %s\n",chosen.c_str());
    ps3me::beginInput(width,height);
    inGame=true;int result=ps3me_run_game(url.c_str(),db.c_str(),config.c_str(),classes.c_str());inGame=false;ps3me::endInput();ps3me::gameSettings().end(ps3me::controlConfig());
    std::fprintf(stderr,"[PS3ME] MIDP returned %d\n",result);
    return result;
}
}
int main(int argc,char** argv) {
    ::printf("[PS3ME boot] entered main\n");::fflush(stdout);
    directories(std::string(home)+"/games");directories(std::string(home)+"/appdb");directories(std::string(home)+"/logs");
    directories(std::string(home)+"/config");
    ps3me::controlConfig().load((std::string(home)+"/config/controls.cfg").c_str());
    ps3me::Storage storage(std::vector<std::string>{std::string(home)+"/games","/dev_usb000","/dev_usb001"});
    ps3me::Framebuffer video;ps3me::Controller controller;ps3me::Clock clock;ps3me::Heap heap;ps3me::Log log;
    ::freopen((std::string(home)+"/logs/runtime.log").c_str(),"ab",stdout);
    ::freopen((std::string(home)+"/logs/runtime.log").c_str(),"ab",stderr);
    ::setvbuf(stdout,0,_IOLBF,0);
    ::setvbuf(stderr,0,_IOLBF,0);
    ::fprintf(stderr,"[PS3ME boot] build 01.16: initializing SDL video (audio deferred)\n");
    if(SDL_Init(SDL_INIT_VIDEO)!=0){::fprintf(stderr,"[PS3ME boot] SDL_Init: %s\n",SDL_GetError());return 1;}
    ::fprintf(stderr,"[PS3ME boot] SDL video subsystem ready\n");
    ps3me::installPlatform(storage,video,controller,clock,heap,log);
    if(!video.initializeVideo()){::fprintf(stderr,"[PS3ME boot] video mode failed: %s\n",SDL_GetError());SDL_Quit();return 2;}
    ::fprintf(stderr,"[PS3ME boot] video mode ready\n");
    SDL_Surface* screen=SDL_GetVideoSurface();
    SDL_Surface* menu=SDL_CreateRGBSurface(SDL_SWSURFACE,1280,720,32,0xff0000,0xff00,0xff,0);
    if(!menu){::fprintf(stderr,"[PS3ME boot] menu surface failed: %s\n",SDL_GetError());SDL_Quit();return 3;}
    ::fprintf(stderr,"[PS3ME boot] entering menu\n");
    std::string path=std::string(home)+"/games",message="Right stick: cursor. R2: touch/drag. Square: remap controls.";
    std::vector<Entry> entries=list(path);int selected=0,resolution=1;
    const int dimensions[][2]={{176,220},{240,320},{320,240},{352,416},{480,800}};
    // Explicit opt-in tests; normal XMB boot always opens the browser.
    bool diagnostic=false;
    for(int i=0;i<argc;++i)if(argv[i]&&!std::strcmp(argv[i],"--diagnostic"))diagnostic=true;
    std::string trigger=std::string(home)+"/logs/diagnostic.once";
    FILE* request=::fopen(trigger.c_str(),"rb");
    if(request){
        unsigned int ms=0;
        if(::fscanf(request,"%u",&ms)==1&&ms>=1000&&ms<=60000)
            diagnosticStopAt=sysGetSystemTime()+(u64)ms*1000;
        ::fclose(request);::remove(trigger.c_str());diagnostic=true;
    }
    FILE* menuTest=::fopen((std::string(home)+"/logs/controls_test.once").c_str(),"rb");
    if(menuTest){::fclose(menuTest);::remove((std::string(home)+"/logs/controls_test.once").c_str());ps3me::showControlMenu(controller,true);}
    std::string sequence=std::string(home)+"/logs/sequence_test.once";FILE* seq=::fopen(sequence.c_str(),"rb");
    if(seq){::fclose(seq);::remove(sequence.c_str());const char* games[]={"PS3ME_TouchTest.jar","PS3ME_AudioTest.jar","PS3ME_TouchTest.jar"};for(int i=0;i<3;i++){diagnosticStopAt=sysGetSystemTime()+11000000;launch(std::string(home)+"/games/"+games[i],240,320);}std::fprintf(stderr,"[PS3ME sequence] completed\n");}
    if(diagnostic)
        launch(std::string(home)+"/games/PS3ME_Diagnostic.jar",240,320);
    ps2::hal::PadButtons previous;
    bool done=false,ignoreStartUntilReleased=true;unsigned int frames=0;
    while(!done) {
        ps3me_service_app();
        SDL_Event event;while(SDL_PollEvent(&event))if(event.type==SDL_QUIT)done=true;
        ps2::hal::PadButtons b;controller.read(&b);
        if(b.square&&!previous.square){ps3me::showControlMenu(controller);controller.read(&b);previous=b;}
        if(!b.start)ignoreStartUntilReleased=false;
        if(b.start&&!previous.start&&!ignoreStartUntilReleased)done=true;
        if(b.up&&!previous.up&&selected>0)--selected;
        if(b.down&&!previous.down&&selected+1<static_cast<int>(entries.size()))++selected;
        if(b.triangle&&!previous.triangle)resolution=(resolution+1)%5;
        bool reload=false;
        if(b.select&&!previous.select){reload=true;ps3me_browser_clear();message="Library refreshed.";}
        if(b.l1&&!previous.l1){path="/dev_usb000";reload=true;}
        if(b.r1&&!previous.r1){path="/dev_hdd0";reload=true;}
        if(b.circle&&!previous.circle){size_t at=path.find_last_of('/');path=at==0?"/":path.substr(0,at);reload=true;}
        if(b.cross&&!previous.cross&&selected>=0&&selected<static_cast<int>(entries.size())) {
            const Entry e=entries[selected];std::string chosen=path+"/"+e.name;
            if(e.directory){path=chosen;reload=true;}
            else {
                ps3me_browser_clear();
                int result=launch(chosen,dimensions[resolution][0],dimensions[resolution][1]);
                ignoreStartUntilReleased=true;controller.read(&b);
                char info[80];::snprintf(info,sizeof(info),"MIDP returned %d; see logs/runtime.log",result);message=info;
            }
        }
        if(reload){ps3me_browser_clear();entries=list(path);selected=0;}
        std::vector<std::string> names;for(const Entry& e:entries)names.push_back(e.name);
        ps3me_browser_draw(menu,path,names,selected,dimensions[resolution][0],dimensions[resolution][1],message);
        SDL_SoftStretch(menu,0,screen,0);SDL_Flip(screen);previous=b;clock.sleep(16);
        ++frames;
        if(frames==120){std::string p=std::string(home)+"/logs/preview.once";FILE* f=::fopen(p.c_str(),"rb");if(f){::fclose(f);::remove(p.c_str());SDL_SaveBMP(menu,(std::string(home)+"/logs/preview.bmp").c_str());}}

        if(frames==1||frames==120)std::fprintf(stderr,"[PS3ME boot] menu frames: %u\n",frames);
    }
    SDL_FreeSurface(menu);SDL_Quit();return 0;
}
