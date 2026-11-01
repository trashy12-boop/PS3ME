// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <cstring>

namespace ps3me {
inline bool promptIsFace(const char* id) {
    return id && (!std::strcmp(id,"X") || !std::strcmp(id,"O") ||
                  !std::strcmp(id,"SQUARE") || !std::strcmp(id,"TRIANGLE"));
}
inline int promptWidth(const char* id, int size=25) {
    if (!id || size < 12) return 0;
    if (promptIsFace(id)) return size;
    const int textW=(int)std::strlen(id)*8;
    const int minW=size+8;
    return textW+12>minW?textW+12:minW;
}
// Lightweight DualShock-style vector prompts. Word buttons use their real text width
// instead of being squeezed into a square, so SELECT/START remain centered.
inline int drawPrompt(SDL_Surface* dst, int x, int y, const char* id, int size=25) {
    if (!dst || !id || size < 12) return 0;
    const int w=promptWidth(id,size);
    const int cx=x+size/2, cy=y+size/2, r=size/2-2;
    if (promptIsFace(id)) {
        filledCircleRGBA(dst,cx,cy,r+2,14,31,54,248);
        aacircleRGBA(dst,cx,cy,r+1,148,182,216,255);
        if (!std::strcmp(id,"X")) {
            const int e=(r*3)/5;
            // Plot both arms explicitly so the X stays symmetric on PS3 SDL.
            for (int d=-e; d<=e; ++d) {
                pixelRGBA(dst,cx+d,cy+d,116,167,250,255);
                pixelRGBA(dst,cx+d+1,cy+d,116,167,250,255);
                pixelRGBA(dst,cx+d,cy-d,116,167,250,255);
                pixelRGBA(dst,cx+d+1,cy-d,116,167,250,255);
            }
        } else if (!std::strcmp(id,"O")) {
            aacircleRGBA(dst,cx,cy,r-4,246,133,147,255);
            aacircleRGBA(dst,cx,cy,r-3,246,133,147,255);
        } else if (!std::strcmp(id,"SQUARE")) {
            rectangleRGBA(dst,cx-r/2,cy-r/2,cx+r/2,cy+r/2,245,158,224,255);
            rectangleRGBA(dst,cx-r/2+1,cy-r/2+1,cx+r/2-1,cy+r/2-1,245,158,224,255);
        } else {
            aatrigonRGBA(dst,cx,cy-r/2-2,cx-r/2-2,cy+r/2,cx+r/2+2,cy+r/2,124,221,175,255);
            trigonRGBA(dst,cx,cy-r/2-1,cx-r/2-1,cy+r/2-1,cx+r/2+1,cy+r/2-1,124,221,175,255);
        }
    } else {
        roundedBoxRGBA(dst,x,y+1,x+w-1,y+size-1,5,21,40,65,240);
        roundedRectangleRGBA(dst,x,y+1,x+w-1,y+size-1,5,147,176,212,255);
        const int textW=(int)std::strlen(id)*8;
        stringRGBA(dst,x+(w-textW)/2,y+(size-8)/2,id,240,246,255,255);
    }
    return w;
}
} // namespace ps3me
