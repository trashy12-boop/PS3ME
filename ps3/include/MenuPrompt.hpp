// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <cstring>

// Native SDL_gfx DualShock-style prompts: no platform-dependent RSX texture atlas.
// Inspired by Salsa64's Kenney/CC0 prompt layout; the glyphs here are original vectors.
namespace ps3me {
inline void drawPrompt(SDL_Surface* s, int x, int y, const char* id, int size = 27) {
    if (!s || !id) return;
    int cx = x + size / 2, cy = y + size / 2, r = size / 2 - 2;
    if (!std::strcmp(id, "X") || !std::strcmp(id, "O") ||
        !std::strcmp(id, "SQUARE") || !std::strcmp(id, "TRIANGLE")) {
        filledCircleRGBA(s, cx, cy, r + 1, 18, 31, 52, 235);
        circleRGBA(s, cx, cy, r + 1, 158, 188, 222, 240);
        if (!std::strcmp(id, "X")) {
            thickLineRGBA(s, cx-r/2, cy-r/2, cx+r/2, cy+r/2, 3, 123, 177, 249, 255);
            thickLineRGBA(s, cx+r/2, cy-r/2, cx-r/2, cy+r/2, 3, 123, 177, 249, 255);
        } else if (!std::strcmp(id, "O")) {
            circleRGBA(s, cx, cy, r-4, 245, 126, 140, 255);
            circleRGBA(s, cx, cy, r-3, 245, 126, 140, 255);
        } else if (!std::strcmp(id, "SQUARE")) {
            rectangleRGBA(s, cx-r/2, cy-r/2, cx+r/2, cy+r/2, 250, 160, 222, 255);
            rectangleRGBA(s, cx-r/2+1, cy-r/2+1, cx+r/2-1, cy+r/2-1, 250, 160, 222, 255);
        } else {
            trigonRGBA(s, cx, cy-r/2-2, cx-r/2-2, cy+r/2, cx+r/2+2, cy+r/2, 120, 220, 172, 255);
            trigonRGBA(s, cx, cy-r/2-1, cx-r/2-1, cy+r/2-1, cx+r/2+1, cy+r/2-1, 120, 220, 172, 255);
        }
    } else {
        roundedBoxRGBA(s, x, y+2, x+size+7, y+size-2, 6, 23, 39, 65, 236);
        roundedRectangleRGBA(s, x, y+2, x+size+7, y+size-2, 6, 173, 195, 230, 225);
        const int len = (int)std::strlen(id);
        stringRGBA(s, x + (size+7-len*8)/2, y + (size-8)/2, id, 240, 246, 255, 255);
    }
}
} // namespace ps3me
