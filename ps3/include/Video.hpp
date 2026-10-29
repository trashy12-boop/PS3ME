// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <stdint.h>
namespace ps3me {
struct Rect {int x,y,w,h;};
Rect fit(int sourceWidth,int sourceHeight,int displayWidth,int displayHeight);
void drawCursor(uint32_t* pixels,int width,int height,int pitch,Rect viewport,int x,int y,int logicalWidth,int logicalHeight,uint32_t dark,uint32_t light);
uint32_t rgb565ToXrgb(uint16_t pixel);
void scaleRgb565(const uint16_t* source,int sw,int sh,uint32_t* destination,
                 int dw,int dh,int destinationPitchPixels);
}
