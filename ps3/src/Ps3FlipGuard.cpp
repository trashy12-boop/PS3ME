// SPDX-License-Identifier: GPL-2.0-only
// PS3ME 01.23: maintain the application's flip stream while XMB composites.
// Do not invent flip completions or enqueue GPU waits: XMB uses this FIFO too.
// Admission happens BEFORE SDL copies into an RSX framebuffer; a pending flip
// defers that upload, leaving the CPU surface for the next heartbeat.
#include <rsx/gcm_sys.h>
#include <sys/systime.h>
#include <SDL.h>
extern "C" void ps3me_pump_sysutil(void);
extern "C" {
u32 __real_gcmGetFlipStatus(void);
void __real_gcmResetFlipStatus(void);
s32 __real_gcmSetFlip(gcmContextData*,const u8);
int __real_SDL_Flip(SDL_Surface*);
void __real_SDL_UpdateRect(SDL_Surface*,Sint32,Sint32,Uint32,Uint32);
void __real_SDL_UpdateRects(SDL_Surface*,int,SDL_Rect*);
}
namespace {
bool pending=false;
bool started=false;
int presentDepth=0;
u64 lastSubmit=0;
u64 lastPump=0;
const u64 heartbeatUs=33333;
bool ready(){
    if(!pending)return true;
    if(__real_gcmGetFlipStatus()!=0)return false;
    pending=false;
    return true;
}
bool admit(){return presentDepth==0&&ready();}
}
extern "C" int ps3me_present_blocked(void){return ready()?0:1;}
extern "C" void ps3me_display_tick(void){
    if(!started||presentDepth||sysGetSystemTime()-lastSubmit<heartbeatUs||!ready())return;
    SDL_Surface* screen=SDL_GetVideoSurface();
    if(screen){
        // Refresh BOTH retained CPU framebuffer content and the flip chain.
        // SDL updates one alternating RSX target at a time; use the full image.
        ++presentDepth;
        __real_SDL_UpdateRect(screen,0,0,0,0);
        --presentDepth;
    }
}
extern "C" int __wrap_SDL_Flip(SDL_Surface* screen){
    if(!admit())return 0; // deferred; the CPU surface remains authoritative
    ++presentDepth;const int rc=__real_SDL_Flip(screen);--presentDepth;return rc;
}
extern "C" void __wrap_SDL_UpdateRect(SDL_Surface* screen,Sint32 x,Sint32 y,Uint32 w,Uint32 h){
    if(!admit())return;
    ++presentDepth;__real_SDL_UpdateRect(screen,x,y,w,h);--presentDepth;
}
extern "C" void __wrap_SDL_UpdateRects(SDL_Surface* screen,int count,SDL_Rect* rects){
    if(!admit())return;
    ++presentDepth;__real_SDL_UpdateRects(screen,count,rects);--presentDepth;
}
extern "C" u32 __wrap_gcmGetFlipStatus(void){
    // Defensive fallback for a library wait: keep firmware callbacks alive,
    // but never report completion until the actual flip has finished.
    const u64 now=sysGetSystemTime();
    if(now-lastPump>=10000){lastPump=now;ps3me_pump_sysutil();}
    const u32 status=__real_gcmGetFlipStatus();
    if(status==0)pending=false;
    return status;
}
extern "C" void __wrap_gcmResetFlipStatus(void){__real_gcmResetFlipStatus();}
extern "C" s32 __wrap_gcmSetFlip(gcmContextData* context,const u8 bufferId){
    const s32 rc=__real_gcmSetFlip(context,bufferId);
    if(rc==0){pending=true;started=true;lastSubmit=sysGetSystemTime();}
    return rc;
}
extern "C" void __wrap_gcmSetWaitFlip(gcmContextData*){
    // Intentionally no GPU semaphore wait. The CPU admission check above waits
    // for the real completion before any next upload. This is Salsa64's model:
    // at most one in-flight flip, with the RSX free for the system overlay.
}
