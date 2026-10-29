// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <SDL.h>
#include <vector>
#include <string>
void ps3me_browser_clear();
void ps3me_browser_draw(SDL_Surface*,const std::string&,const std::vector<std::string>&,int,int,int,const std::string&);
