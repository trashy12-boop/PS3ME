// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include <cmath>

namespace ps3me {

// Preserve ordinary samples and ease overloaded mixes into the output range.
// The previous hard clamp flattened every over-range peak, which can sound
// rough when several MIDI tracks or WAV effects overlap.
inline float softLimitAudioSample(float sample) {
    const float magnitude = std::fabs(sample);
    const float knee = 0.85f;
    const float range = 0.15f;
    if (magnitude <= knee) return sample;
    const float shaped = knee + range * std::tanh((magnitude - knee) / range);
    const float limited = shaped < 0.999f ? shaped : 0.999f;
    return sample < 0.0f ? -limited : limited;
}

} // namespace ps3me
