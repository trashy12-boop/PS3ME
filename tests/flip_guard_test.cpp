// SPDX-License-Identifier: GPL-2.0-only
#include "flip_guard_sim.hpp"
#include "XmbGate.hpp"
#include <SDL.h>
#include <cassert>
#include <cstdio>
int main(){
 using namespace ps3me::xmb;
 SDL_Surface* screen=SDL_GetVideoSurface();
 SDL_Flip(screen);assert(sim::flips==1&&sim::waits==0);
 // A slow system flip must defer BEFORE uploading/resetting the framebuffer.
 sim::stall=true;
 const int uploads=sim::uploads,resets=sim::resets;
 for(int n=0;n<2000;++n){
  sim::clk+=10000;ps3me_pump_sysutil();ps3me_display_tick();
  SDL_Flip(screen);SDL_UpdateRect(screen,0,0,0,0);SDL_UpdateRects(screen,0,nullptr);
 }
 assert(sim::flips==1&&sim::uploads==uploads&&sim::resets==resets);
 assert(gcmGetFlipStatus()!=0); // no fabricated completion after 20 seconds
 assert(sim::pumps>=2000);
 // Completion allows the next heartbeat immediately, without resetting truth.
 sim::stall=false;ps3me_display_tick();assert(sim::flips==2);
 // MENU_OPEN must NOT prevent the application flips that drive the XMB.
 noteEvent(EventDrawBegin,sim::clk);noteEvent(EventMenuOpen,sim::clk);
 const int before=sim::flips;
 for(int n=0;n<1000;++n){sim::clk+=10000;ps3me_pump_sysutil();ps3me_display_tick();}
 assert(sim::flips>before+200&&sim::flips<=before+300);
 // A delayed flip while XMB is already open neither fabricates a close nor
 // allows writes to the in-flight display target.
 sim::stall=true;const int heldUploads=sim::uploads;
 for(int n=0;n<300;++n){sim::clk+=10000;ps3me_pump_sysutil();ps3me_display_tick();}
 assert(sim::uploads==heldUploads);sim::stall=false;
 assert(ps3me_xmb_open()); // successful flips do not fabricate MENU_CLOSE
 assert(sim::waits==0);   // GPU command stream remains available to compositor
 // Actual close releases input and increments the resume generation once.
 noteEvent(EventDrawEnd,sim::clk);assert(ps3me_xmb_open());
 noteEvent(EventMenuClose,sim::clk);assert(!ps3me_xmb_open());
 assert(takeResume()&&!takeResume());const auto gen=generation();
 noteEvent(EventMenuClose,sim::clk);assert(generation()==gen);
 // Race: a menu callback while SDL polls does not drop the flip or its truth.
 sim::clk+=100000;sim::injectMenuOpen=true;SDL_Flip(screen);
 assert(ps3me_xmb_open());assert(sim::waits==0);
 puts("flip_guard_test: passed (20s GPU stall, XMB heartbeat, all SDL entry points, callback race)");
}
