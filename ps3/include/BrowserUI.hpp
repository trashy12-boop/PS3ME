// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <SDL.h>
#include <vector>
#include <string>
// Vertical strip of the 1280x720 menu surface that the last draw touched.
// Animation frames only repaint the row band, so the caller can copy and
// present that strip instead of the whole screen.
struct BrowserBand { int top; int bottom; };
void ps3me_browser_clear();
// Force the next draw to repaint the whole surface, e.g. after the game or the
// XMB replaced what is currently on screen.
void ps3me_browser_invalidate();
void ps3me_browser_reset_animation();
bool ps3me_browser_animation_active(int selected);
void ps3me_browser_draw_loading(SDL_Surface*,const std::string&);
void ps3me_browser_draw(SDL_Surface*,const std::string&,const std::vector<std::string>&,
                        const std::vector<std::string>&,const std::vector<std::string>&,
                        int,int,int,const std::string&,bool favoritesView);
BrowserBand ps3me_browser_last_band();
