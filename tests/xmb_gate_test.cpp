// SPDX-License-Identifier: GPL-2.0-only
#include "XmbGate.hpp"
#include <cassert>
#include <cstdio>
extern "C" int64_t sysGetSystemTime(){return 0;}
int main(){
 using namespace ps3me::xmb;
 reset();assert(!displayHeld(0));
 noteEvent(EventMenuOpen,0);noteEvent(EventMenuOpen,0);
 assert(displayHeld(60000000)); // no invented close from a timer or flip
 noteEvent(EventDrawBegin,0);noteEvent(EventDrawEnd,0);assert(displayHeld(0));
 noteEvent(EventMenuClose,0);assert(!displayHeld(0));assert(generation()==1);
 assert(takeResume()&&!takeResume());
 noteEvent(EventMenuClose,0);assert(generation()==1);
 noteEvent(EventDrawBegin,0);assert(displayHeld(60000000));
 noteEvent(EventDrawEnd,0);assert(!displayHeld(0));assert(generation()==2);
 noteEvent(EventExitGame,0);assert(exitRequested());
 reset();assert(!exitRequested()&&generation()==0);
 puts("xmb_gate_test: passed");
}
