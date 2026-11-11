// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <SDL.h>
namespace ps3me {
class Controller;
void showControlMenu(Controller&,bool automated=false);
void drawControlDiagram(SDL_Surface*,int x,int y,int width,int height,int selectedAction);
}
