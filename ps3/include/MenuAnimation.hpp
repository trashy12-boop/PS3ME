// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cmath>
namespace ps3me {
// Salsa64's exponential approach (14 per second), independent of display FPS.
inline float easeMenuSelection(float current, int selected, float dtSeconds) {
    if (dtSeconds < 0.0f) dtSeconds = 0.0f;
    if (dtSeconds > 0.1f) dtSeconds = 0.1f;
    const float target = (float)selected;
    current += (target-current)*(1.0f-std::exp(-14.0f*dtSeconds));
    if (std::fabs(target-current) < 0.001f) return target;
    return current;
}
} // namespace ps3me
