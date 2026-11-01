// SPDX-License-Identifier: GPL-2.0-only
// A compositor that needs application flips, including while its menu is open.
#include "flip_guard_sim.hpp"
#include "XmbGate.hpp"
#include <SDL.h>
#include <cassert>
namespace sim {
int64_t clk=1000000,pendingUntil=0;
bool pending=false,stall=false,injectMenuOpen=false;
int flips=0,waits=0,resets=0,uploads=0,pumps=0,completed=0;
}
extern "C" int64_t sysGetSystemTime(){return sim::clk;}
extern "C" u32 gcmGetFlipStatus(){
 if(sim::pending&&!sim::stall&&sim::clk>=sim::pendingUntil){sim::pending=false;++sim::completed;}
 return sim::pending?1:0;
}
extern "C" void gcmResetFlipStatus(){assert(!sim::pending);++sim::resets;}
extern "C" s32 gcmSetFlip(gcmContextData*,const u8){
 assert(!sim::pending);++sim::flips;sim::pending=true;sim::pendingUntil=sim::clk+16000;return 0;
}
extern "C" void gcmSetWaitFlip(gcmContextData*){++sim::waits;}
extern "C" void ps3me_pump_sysutil(){
 ++sim::pumps;
 if(sim::injectMenuOpen){sim::injectMenuOpen=false;ps3me::xmb::noteEvent(ps3me::xmb::EventMenuOpen,sim::clk);}
}
