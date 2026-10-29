// SPDX-License-Identifier: GPL-2.0-only
#include "AudioLimiter.hpp"
#include <cassert>
#include <cmath>

int main() {
    const float quiet = 0.4f;
    assert(ps3me::softLimitAudioSample(quiet) == quiet);
    assert(ps3me::softLimitAudioSample(-quiet) == -quiet);

    float previous = 0.85f;
    for (int i = 86; i <= 500; ++i) {
        const float input = i / 100.0f;
        const float output = ps3me::softLimitAudioSample(input);
        assert(std::isfinite(output));
        assert(output >= previous && output < 1.0f);
        assert(std::fabs(ps3me::softLimitAudioSample(-input) + output) < 1.0e-6f);
        previous = output;
    }
}
