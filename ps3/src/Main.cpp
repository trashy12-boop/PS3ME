// SPDX-License-Identifier: GPL-2.0-only
#include "Ps3Backend.hpp"
#include "XmbGate.hpp"
#include "Controls.hpp"
#include "GameSettings.hpp"
#include "ControlMenu.hpp"
#include "BrowserUI.hpp"
#include "MenuLanguage.hpp"
#include "hal/LcdDevice.hpp"
#include <SDL_gfxPrimitives.h>
#include <sys/process.h>
#include <sys/systime.h>
#include <sysutil/sysutil.h>
#include <malloc.h>
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
// The system menu pauses application input, while a retained-image heartbeat
// continues feeding the flip stream used by the XMB compositor.
// Published so the diagnostic line can show what the system actually sent.
static volatile int eventsOpen=0,eventsClose=0,eventsDraw=0,eventsExit=0;
static volatile u64 lastEventStatus=0;
static volatile u64 lastEventAt=0;
static int slot1Result=-1,slot2Result=-1;
// Persistent last-stage record, written from the menu loop and on every system
// event. After a freeze the console can be power-cycled and the next launch
// shows what the previous run was doing, which is the only way to see what a
// hard lock-up was in the middle of.
static void writeState(const char* stage);
static volatile int stateDirty=0;
// Events are queued from the firmware callback and written to the log by the
// main loop: no file or stdio work happens inside the callback.
enum { EVENT_RING=32 };
static volatile u64 eventRingStatus[EVENT_RING];
static volatile u64 eventRingAt[EVENT_RING];
static volatile unsigned eventRingHead=0,eventRingTail=0,eventRingDropped=0;
static void flushEventLog(){
    while(eventRingTail!=eventRingHead){
        const unsigned at=eventRingTail;
        std::fprintf(stderr,"[PS3ME sysutil] event status=0x%llx uptime_ms=%llu\n",
            static_cast<unsigned long long>(eventRingStatus[at]),
            static_cast<unsigned long long>(eventRingAt[at]/1000));
        eventRingTail=(at+1)%EVENT_RING;
    }
    std::fflush(stderr);
}
static void onSysutilEvent(u64 status,u64,void*){
    const u64 now=sysGetSystemTime();
    lastEventStatus=status;lastEventAt=now;
    const unsigned head=eventRingHead,next=(head+1)%EVENT_RING;
    if(next!=eventRingTail){eventRingStatus[head]=status;eventRingAt[head]=now;eventRingHead=next;}
    else ++eventRingDropped;
    ps3me::xmb::noteEvent(status,now);
    switch(status){
    case SYSUTIL_MENU_OPEN:++eventsOpen;break;
    case SYSUTIL_DRAW_BEGIN:++eventsDraw;break;
    case SYSUTIL_MENU_CLOSE:++eventsClose;break;
    case SYSUTIL_EXIT_GAME:++eventsExit;break;
    default:break;
    }
    // No file or stdio work here: this runs inside the firmware's callback and
    // the loop writes the record on its next iteration. A write to the HDD
    // from here was blocking the system's dispatcher mid-event.
    stateDirty=1;
}
// Leave SDL on slot 0; register our event recorder once, on slot 1.
static void installSysutilEvents(){
    slot1Result=sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT1,onSysutilEvent,0);
    slot2Result=-1; // one application callback; slot 0 remains SDL-owned
    std::fprintf(stderr,"[PS3ME sysutil] callbacks slot1=%d slot2=%d\n",slot1Result,slot2Result);
}
static void removeSysutilEvents(){
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT1);

}
// Called from the flip wrappers through unpatched code paths as well, so the
// event queue keeps being drained even while SDL's renderer waits on the RSX.
extern "C" void ps3me_pump_sysutil(){sysUtilCheckCallback();}
// Defined by Ps3FlipGuard.cpp.
extern "C" int ps3me_present_blocked(void);

static std::string sysutilReport(){
    char text[224];
    ::snprintf(text,sizeof(text),"sysutil slot1=%d slot2=%d open=%d draw=%d close=%d exit=%d drop=%u gen=%u last=0x%llx",
        slot1Result,slot2Result,eventsOpen,eventsDraw,eventsClose,eventsExit,
        static_cast<unsigned int>(eventRingDropped),
        static_cast<unsigned int>(ps3me_xmb_generation()),
        static_cast<unsigned long long>(lastEventStatus));
    return text;
}
static u64 diagnosticStopAt=0;
static bool menuTraceEnabled=false;
static bool menuTraceForceRedraw=false;
static bool menuTraceForceStretch=false;
static char menuTracePhase='A';
static u64 menuTracePhaseStart=0;
static const u64 menuTracePhaseDuration=10000000;
static u64 menuNextFrameAt=0;
static const u64 menuFrameIntervalUs=1000000/30;
static void paceMenuTo30Fps(){
    u64 now=sysGetSystemTime();
    if(!menuNextFrameAt)menuNextFrameAt=now;
    if(now<menuNextFrameAt)sysUsleep(static_cast<unsigned int>(menuNextFrameAt-now));
    else if(now-menuNextFrameAt>menuFrameIntervalUs)menuNextFrameAt=now;
    menuNextFrameAt+=menuFrameIntervalUs;
}
static void traceMenuStage(unsigned int frame,const char* stage){
    if(!menuTraceEnabled)return;
    u64 now=sysGetSystemTime();
    std::fprintf(stderr,"[PS3ME menu trace] phase=%c frame=%u uptime_ms=%llu stage=%s\n",
        menuTracePhase,frame,
        static_cast<unsigned long long>(now/1000),stage);
    if(frame%180==0&&std::strcmp(stage,"loop.begin")==0){
        struct mallinfo memory=mallinfo();
        std::fprintf(stderr,"[PS3ME menu memory] phase=%c frame=%u arena=%d used=%d free=%d\n",
            menuTracePhase,frame,memory.arena,memory.uordblks,memory.fordblks);
    }
    std::fflush(stderr);
}
static void updateMenuTracePhase(){
    if(!menuTraceEnabled)return;
    u64 now=sysGetSystemTime();
    if(now-menuTracePhaseStart<menuTracePhaseDuration)return;
    if(menuTracePhase=='A'){
        menuTracePhase='B';menuTraceForceRedraw=true;
        menuTracePhaseStart=now;
        std::fprintf(stderr,"[PS3ME menu trace] phase B begins: continuous redraw with direct blit for 10 seconds\n");
    }else if(menuTracePhase=='B'){
        menuTracePhase='C';menuTraceForceStretch=true;
        menuTracePhaseStart=now;
        std::fprintf(stderr,"[PS3ME menu trace] phase C begins: continuous redraw with SDL_SoftStretch for 10 seconds\n");
    }else{
        menuTraceEnabled=false;
        menuTraceForceRedraw=false;
        menuTraceForceStretch=false;
        std::fprintf(stderr,"[PS3ME menu trace] A/B/C trace complete; returning to normal 01.19 loop\n");
    }
    std::fflush(stderr);
}
#ifdef PS3ME_RUNTIME_REPLAY
#include "../tests/RuntimeReplay.hpp"
#endif
extern "C" void ps3me_on_start(){if(inGame&&!ps3me::overlayActive())JVM_Stop(0);}
extern "C" void ps3me_service_app(){
    static bool exitStopIssued=false;
    ps3me_pump_sysutil();
    if(inGame)ps3me_display_tick();
    flushEventLog();
#ifdef PS3ME_RUNTIME_REPLAY
    if(inGame)ps3me_runtime_replay();
#endif
    if(ps3me_exit_requested()&&inGame&&!exitStopIssued){
        // XMB "Quit"/power off while a game runs: leave the VM through the same
        // handler Start uses instead of letting the system time out.
        exitStopIssued=true;
        std::fprintf(stderr,"[PS3ME sysutil] exit requested during game; stopping MIDP\n");
        ps3me_on_start();
    }
    if(inGame&&diagnosticStopAt&&sysGetSystemTime()>=diagnosticStopAt){
        diagnosticStopAt=0;
        std::fprintf(stderr,"[PS3ME test] requesting return through Start handler\n");
        ps3me_on_start();
    }
}
namespace {
const char home[]="/dev_hdd0/game/PS3ME0001/USRDIR";

} // namespace (temporarily closed for the state recorder)

// Updated by the menu loop; "" means "not in a known stage".
static const char* currentStage="boot";
static unsigned int framesDone=0;
static u64 lastStateWrite=0;
static void writeState(const char* stage){
    u64 now=sysGetSystemTime();
    bool changed=false;
    if(stage&&std::strcmp(stage,currentStage)!=0){currentStage=stage;changed=true;}
    // A stage change alone is not worth a write: the menu cycles
    // draw/present/idle/loop every frame, which used to open and close the log
    // four times per frame. Sample at most 4x/second, and always record a
    // system event.
    if(!stateDirty&&(!changed||now-lastStateWrite<250000))return;
    stateDirty=0;lastStateWrite=now;
    std::string path=std::string("/dev_hdd0/game/PS3ME0001/USRDIR")+"/logs/last_run.txt";
    FILE* f=::fopen(path.c_str(),"wb");
    if(!f)return;
    std::fprintf(f,"stage=%s uptime_ms=%llu frames=%u xmb=%d open=%d draw=%d close=%d exit=%d last=0x%llx\n",
        currentStage?currentStage:"?",static_cast<unsigned long long>(now/1000),framesDone,
        ps3me_xmb_open(),eventsOpen,eventsDraw,eventsClose,eventsExit,
        static_cast<unsigned long long>(lastEventStatus));
    ::fclose(f);
}
static std::string previousRun(){
    std::string path=std::string("/dev_hdd0/game/PS3ME0001/USRDIR")+"/logs/last_run.txt";
    FILE* f=::fopen(path.c_str(),"rb");
    if(!f)return "prev: none";
    char line[192]={0};
    if(!::fgets(line,sizeof(line),f))line[0]=0;
    ::fclose(f);
    std::string text=line;
    while(!text.empty()&&(text.back()=='\n'||text.back()=='\r'))text.pop_back();
    return "prev: "+(text.empty()?std::string("empty"):text);
}
namespace {
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
    // Show the loading message BEFORE the blocking native VM startup call.
    // Skipped if the XMB happens to be up: the system owns the display then.
    SDL_Surface* screen=SDL_GetVideoSurface();
    if(screen&&!ps3me_xmb_open()){
        SDL_Surface* canvas=SDL_CreateRGBSurface(SDL_SWSURFACE,1280,720,32,0xff0000,0xff00,0xff,0);
        if(canvas){
            ps3me_browser_draw_loading(canvas,chosen);
            SDL_SoftStretch(canvas,0,screen,0);
            SDL_Flip(screen);
            SDL_FreeSurface(canvas);
        }
    }
    ps3me::beginInput(width,height);
    writeState("game");
    inGame=true;int result=ps3me_run_game(url.c_str(),db.c_str(),config.c_str(),classes.c_str());inGame=false;ps3me::endInput();ps3me::gameSettings().end(ps3me::controlConfig());
    writeState("game.return");
    std::fprintf(stderr,"[PS3ME] MIDP returned %d\n",result);
    return result;
}
}
int main(int argc,char** argv) {
    ::printf("[PS3ME boot] entered main\n");::fflush(stdout);
    directories(std::string(home)+"/games");directories(std::string(home)+"/appdb");directories(std::string(home)+"/logs");
    directories(std::string(home)+"/config");
    ps3me::controlConfig().load((std::string(home)+"/config/controls.cfg").c_str());
    ps3me::loadMenuLanguage();
    ps3me::Storage storage(std::vector<std::string>{std::string(home)+"/games","/dev_usb000","/dev_usb001"});
    ps3me::Framebuffer video;ps3me::Controller controller;ps3me::Clock clock;ps3me::Heap heap;ps3me::Log log;
    ::freopen((std::string(home)+"/logs/runtime.log").c_str(),"ab",stdout);
    ::freopen((std::string(home)+"/logs/runtime.log").c_str(),"ab",stderr);
    ::setvbuf(stdout,0,_IOLBF,0);
    ::setvbuf(stderr,0,_IOLBF,0);
    ::fprintf(stderr,"[PS3ME boot] build 01.24: initializing SDL video (audio deferred)\n");
    if(SDL_Init(SDL_INIT_VIDEO)!=0){::fprintf(stderr,"[PS3ME boot] SDL_Init: %s\n",SDL_GetError());return 1;}
    ::fprintf(stderr,"[PS3ME boot] SDL video subsystem ready\n");
    ps3me::installPlatform(storage,video,controller,clock,heap,log);
    installSysutilEvents();
    ::fprintf(stderr,"[PS3ME boot] sysutil slot 1 registered for XMB/exit events\n");
    if(!video.initializeVideo()){::fprintf(stderr,"[PS3ME boot] video mode failed: %s\n",SDL_GetError());SDL_Quit();return 2;}
    ::fprintf(stderr,"[PS3ME boot] video mode ready\n");
    SDL_Surface* screen=SDL_GetVideoSurface();
    SDL_Surface* menu=SDL_CreateRGBSurface(SDL_SWSURFACE,1280,720,32,0xff0000,0xff00,0xff,0);
    if(!menu){::fprintf(stderr,"[PS3ME boot] menu surface failed: %s\n",SDL_GetError());SDL_Quit();return 3;}
    ::fprintf(stderr,"[PS3ME boot] entering menu\n");
    std::string path=std::string(home)+"/games",message=ps3me::tr("Direcional direito: cursor. R2: toque. Quadrado: configurações.","Right stick: cursor. R2: touch/drag. Square: settings.");
    // Boot line carries the callback registration result; it is the first thing
    // to check if the console is still unhappy about the PS button.
    message+="  |  "+sysutilReport()+"  |  "+previousRun();
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
    std::string menuTraceTrigger=std::string(home)+"/logs/menu_trace.once";
    FILE* menuTraceRequest=::fopen(menuTraceTrigger.c_str(),"rb");
    if(menuTraceRequest){
        ::fclose(menuTraceRequest);::remove(menuTraceTrigger.c_str());
        menuTraceEnabled=true;menuTracePhaseStart=sysGetSystemTime();
        std::fprintf(stderr,"[PS3ME menu trace] starting phase A: 01.19 redraw gate with direct blit for 10 seconds\n");
        std::fflush(stderr);
    }
    FILE* menuTest=::fopen((std::string(home)+"/logs/controls_test.once").c_str(),"rb");
    if(menuTest){::fclose(menuTest);::remove((std::string(home)+"/logs/controls_test.once").c_str());ps3me::showControlMenu(controller,true);}
    std::string sequence=std::string(home)+"/logs/sequence_test.once";FILE* seq=::fopen(sequence.c_str(),"rb");
    if(seq){::fclose(seq);::remove(sequence.c_str());const char* games[]={"PS3ME_TouchTest.jar","PS3ME_AudioTest.jar","PS3ME_TouchTest.jar"};for(int i=0;i<3;i++){diagnosticStopAt=sysGetSystemTime()+11000000;launch(std::string(home)+"/games/"+games[i],240,320);}std::fprintf(stderr,"[PS3ME sequence] completed\n");}
    if(diagnostic)
        launch(std::string(home)+"/games/PS3ME_Diagnostic.jar",240,320);
    ps2::hal::PadButtons previous;
    bool done=false,ignoreStartUntilReleased=true,dirty=true,wasSysutilPaused=false;unsigned int frames=0;
    u32 menuGeneration=ps3me_xmb_generation();
    u64 menuFpsWindowStart=sysGetSystemTime();
    while(!done) {
        paceMenuTo30Fps();
        updateMenuTracePhase();
        traceMenuStage(frames,"loop.begin");
        traceMenuStage(frames,"sysutil.before");
        ps3me_service_app();
        traceMenuStage(frames,"sysutil.after");
        if(ps3me_exit_requested()){
            // The XMB "Quit" item (or a power-off request) ends the application.
            // Ignoring it makes the console look hung until it is forced off.
            std::fprintf(stderr,"[PS3ME sysutil] exit requested by the system; leaving menu\n");
            done=true;break;
        }
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT)done=true;
        }
        if(done)break;
        // Keep the system compositor moving, but consume no application input.
        if(ps3me_xmb_open()){
            wasSysutilPaused=true;
            ps3me_display_tick();
            continue;
        }
        const u32 generation=ps3me_xmb_generation();
        if(wasSysutilPaused||generation!=menuGeneration){
            // Returning from the XMB (or from a system overlay): the retained
            // surfaces and the pad state are both stale, so rebuild from
            // scratch and require Start to be released before it can exit.
            const bool closed=generation!=menuGeneration;
            wasSysutilPaused=false;menuGeneration=generation;ps3me_xmb_resume();
            dirty=true;menuNextFrameAt=0;
            ps3me_browser_invalidate();
            ignoreStartUntilReleased=true;
            controller.read(&previous);
            menuFpsWindowStart=sysGetSystemTime();
            writeState("resume");
            // Only a real system close/end event resumes application input.
            std::fprintf(stderr,"[PS3ME sysutil] menu resumed (%s); forcing redraw (generation %u)\n",
                closed?"XMB closed":"system overlay ended",static_cast<unsigned int>(generation));
            // Show what the system actually delivered, so a console test says
            // whether the events arrive even when the log cannot be copied.
            message=sysutilReport();
        }
        if(frames&&frames%30==0){
            u64 now=sysGetSystemTime();
            u64 elapsed=now-menuFpsWindowStart;
            unsigned int fpsTenths=elapsed?static_cast<unsigned int>(300000000ULL/elapsed):0;
            std::fprintf(stderr,"[PS3ME boot] menu frame %u; measured %u.%u FPS\n",frames,fpsTenths/10,fpsTenths%10);
            menuFpsWindowStart=now;
        }
        traceMenuStage(frames,"events.after");
        ps2::hal::PadButtons b;controller.read(&b);
        traceMenuStage(frames,"controller.after");
        if(b.square&&!previous.square){ps3me::showControlMenu(controller);controller.read(&b);previous=b;message=ps3me::tr("Configurações atualizadas.","Settings updated.");dirty=true;}
        if(!b.start)ignoreStartUntilReleased=false;
        if(b.start&&!previous.start&&!ignoreStartUntilReleased)done=true;
        if(b.up&&!previous.up&&selected>0){--selected;dirty=true;}
        if(b.down&&!previous.down&&selected+1<static_cast<int>(entries.size())){++selected;dirty=true;}
        if(b.triangle&&!previous.triangle){resolution=(resolution+1)%5;dirty=true;}
        bool reload=false;
        if(b.select&&!previous.select){reload=true;ps3me_browser_clear();message=ps3me::tr("Biblioteca atualizada.","Library refreshed.");dirty=true;}
        if(b.l1&&!previous.l1){path="/dev_usb000";reload=true;dirty=true;}
        if(b.r1&&!previous.r1){path="/dev_hdd0";reload=true;dirty=true;}
        if(b.circle&&!previous.circle){size_t at=path.find_last_of('/');path=at==0?"/":path.substr(0,at);reload=true;dirty=true;}
        if(b.cross&&!previous.cross&&selected>=0&&selected<static_cast<int>(entries.size())) {
            const Entry e=entries[selected];std::string chosen=path+"/"+e.name;
            if(e.directory){path=chosen;reload=true;dirty=true;}
            else {
                ps3me_browser_clear();
                int result=launch(chosen,dimensions[resolution][0],dimensions[resolution][1]);
                ignoreStartUntilReleased=true;controller.read(&b);
                char info[80];::snprintf(info,sizeof(info),"MIDP returned %d; see logs/runtime.log",result);message=info;ps3me_browser_reset_animation();ps3me_browser_invalidate();dirty=true;
            }
        }
        if(reload){ps3me_browser_clear();ps3me_browser_reset_animation();ps3me_browser_invalidate();entries=list(path);selected=0;dirty=true;}
        if(dirty||ps3me_browser_animation_active(selected)||menuTraceForceRedraw){
            std::vector<std::string> names;names.reserve(entries.size());for(const Entry& e:entries)names.push_back(e.name);
            writeState("draw");
            traceMenuStage(frames,"browser.draw.before");
            ps3me_browser_draw(menu,path,names,selected,dimensions[resolution][0],dimensions[resolution][1],message);
            traceMenuStage(frames,"browser.draw.after");
            // Most PS3s run this menu at 1280x720 already. Avoid a full-screen
            // software scaler when no scaling is required.
            writeState("present");
            if(!menuTraceForceStretch&&screen->w==menu->w&&screen->h==menu->h){
                // Only the row band changes while the selection animates, so
                // copy and present just that strip instead of the whole frame.
                BrowserBand band=ps3me_browser_last_band();
                SDL_Rect from={(Sint16)0,(Sint16)band.top,(Uint16)menu->w,(Uint16)(band.bottom-band.top)};
                SDL_Rect to={0,(Sint16)band.top,0,0};
                SDL_BlitSurface(menu,&from,screen,&to);
                traceMenuStage(frames,"blit.after");
                SDL_UpdateRect(screen,0,band.top,menu->w,band.bottom-band.top);
            }else{
                SDL_SoftStretch(menu,0,screen,0);
                traceMenuStage(frames,"blit.after");
                SDL_Flip(screen);
            }
            dirty=false;
            writeState("idle");
            traceMenuStage(frames,"flip.after");
        }else traceMenuStage(frames,"redraw.skipped");
        // Even a completely idle menu must continue flipping for the XMB.
        ps3me_display_tick();
        previous=b;
        ++frames;framesDone=frames;
        writeState("loop");
        if(frames==120){std::string p=std::string(home)+"/logs/preview.once";FILE* f=::fopen(p.c_str(),"rb");if(f){::fclose(f);::remove(p.c_str());SDL_SaveBMP(menu,(std::string(home)+"/logs/preview.bmp").c_str());}}

        if(frames==1)std::fprintf(stderr,"[PS3ME boot] menu loop started; capped at 30 FPS\n");
    }
    removeSysutilEvents();
    SDL_FreeSurface(menu);SDL_Quit();return 0;
}
