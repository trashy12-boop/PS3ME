// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <stdint.h>
typedef int32_t Sint32;
typedef uint32_t Uint32;
struct SDL_Surface { int retained; };
struct SDL_Rect { int x,y,w,h; };
extern "C" {
SDL_Surface* SDL_GetVideoSurface(void);
int SDL_Flip(SDL_Surface*);
void SDL_UpdateRect(SDL_Surface*,Sint32,Sint32,Uint32,Uint32);
void SDL_UpdateRects(SDL_Surface*,int,SDL_Rect*);
}
