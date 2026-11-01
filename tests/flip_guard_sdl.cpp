// SPDX-License-Identifier: GPL-2.0-only
#include "flip_guard_sim.hpp"
#include <SDL.h>
#include <cassert>
static SDL_Surface screen={0};
extern "C" SDL_Surface* SDL_GetVideoSurface(){return &screen;}
static void render(){
 // Like the SDK renderer: waitFlip, reset, flip, flush, SetWaitFlip.
 // Admission must make the original unbounded wait unnecessary.
 assert(gcmGetFlipStatus()==0);
 ++sim::uploads;
 gcmResetFlipStatus();gcmSetFlip(nullptr,0);gcmSetWaitFlip(nullptr);
}
extern "C" int SDL_Flip(SDL_Surface*){render();return 0;}
extern "C" void SDL_UpdateRect(SDL_Surface*,Sint32,Sint32,Uint32,Uint32){render();}
extern "C" void SDL_UpdateRects(SDL_Surface*,int,SDL_Rect*){render();}
