#include "../src/dsp/GranularFreezeCore.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace
{
int failures = 0;
void check(bool ok, const char* name)
{
    std::printf("%-58s %s\n", name, ok ? "PASS" : "FAIL");
    if (! ok) ++failures;
}
bool near(float a, float b, float tolerance = 1.0e-6f)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    gf::GranularFreezeCore core;
    core.prepare(1000.0, 8);

    std::vector<float> left { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };
    std::vector<float> right { -0.1f, -0.2f, -0.3f, -0.4f, -0.5f, -0.6f, -0.7f, -0.8f };
    std::vector<float> outLeft(8), outRight(8);
    const float* inputs[] { left.data(), right.data() };
    float* outputs[] { outLeft.data(), outRight.data() };

    gf::ParameterValues values;
    core.process(inputs, outputs, 8, values);
    check(std::equal(left.begin(), left.end(), outLeft.begin()),
          "core: live left channel is transparent");
    check(std::equal(right.begin(), right.end(), outRight.begin()),
          "core: live right channel is transparent");

    values.freeze = 1.0f;
    values.crossfadeMs = 1.0f;
    values.holdMs = 50.0f;
    values.grainSizeMs = 5.0f;
    values.densityHz = 200.0f;
    std::fill(left.begin(), left.end(), 0.0f);
    std::fill(right.begin(), right.end(), 0.0f);
    core.process(inputs, outputs, 8, values);

    bool finite = true;
    bool audible = false;
    for (std::size_t i = 0; i < outLeft.size(); ++i)
    {
        finite = finite && std::isfinite(outLeft[i]) && std::isfinite(outRight[i]);
        audible = audible || std::abs(outLeft[i]) > 1.0e-5f;
    }
    check(finite, "core: frozen output remains finite");
    check(audible, "core: freeze renders captured history");

    values.freeze = 0.0f;
    core.process(inputs, outputs, 8, values);
    check(near(outLeft.back(), 0.0f), "core: unfreeze returns to current live input");

    gf::GranularFreezeCore oversized;
    oversized.prepare(48000.0, 64);
    std::vector<float> largeLeft(4097), largeRight(4097), largeOutLeft(4097), largeOutRight(4097);
    for (std::size_t i = 0; i < largeLeft.size(); ++i)
    {
        largeLeft[i] = static_cast<float>(std::sin(i * 0.013));
        largeRight[i] = -0.25f * largeLeft[i];
    }
    const float* largeInputs[] { largeLeft.data(), largeRight.data() };
    float* largeOutputs[] { largeOutLeft.data(), largeOutRight.data() };
    oversized.process(largeInputs, largeOutputs, static_cast<std::uint32_t>(largeLeft.size()), {});
    check(std::equal(largeLeft.begin(), largeLeft.end(), largeOutLeft.begin()),
          "core: oversized live block is chunked transparently");
    check(std::equal(largeRight.begin(), largeRight.end(), largeOutRight.begin()),
          "core: oversized stereo timeline is not advanced per channel");

    gf::ParameterValues invalid;
    invalid.freeze = 1.0f;
    invalid.pitch = std::numeric_limits<float>::quiet_NaN();
    invalid.crossfadeMs = std::numeric_limits<float>::infinity();
    invalid.holdMs = -std::numeric_limits<float>::infinity();
    invalid.grainSizeMs = std::numeric_limits<float>::quiet_NaN();
    invalid.densityHz = std::numeric_limits<float>::infinity();
    invalid.position = -std::numeric_limits<float>::infinity();
    std::fill(largeLeft.begin(), largeLeft.end(), 0.0f);
    std::fill(largeRight.begin(), largeRight.end(), 0.0f);
    oversized.process(largeInputs, largeOutputs, static_cast<std::uint32_t>(largeLeft.size()), invalid);
    bool invalidFinite = true;
    for (std::size_t i = 0; i < largeOutLeft.size(); ++i)
        invalidFinite = invalidFinite && std::isfinite(largeOutLeft[i]) && std::isfinite(largeOutRight[i]);
    check(invalidFinite, "core: non-finite parameter block produces finite output");

    gf::GranularFreezeCore malformedAudio;
    malformedAudio.prepare(48000.0, 8);
    const float malformedLeft[] {
        std::numeric_limits<float>::quiet_NaN(), 0.25f,
        std::numeric_limits<float>::infinity(), -0.25f
    };
    const float malformedRight[] {
        -std::numeric_limits<float>::infinity(), 0.5f,
        std::numeric_limits<float>::quiet_NaN(), -0.5f
    };
    float malformedOutLeft[4] {};
    float malformedOutRight[4] {};
    const float* malformedInputs[] { malformedLeft, malformedRight };
    float* malformedOutputs[] { malformedOutLeft, malformedOutRight };
    malformedAudio.process(malformedInputs, malformedOutputs, 4, {});
    bool malformedFinite = true;
    for (int i = 0; i < 4; ++i)
        malformedFinite = malformedFinite
            && std::isfinite(malformedOutLeft[i])
            && std::isfinite(malformedOutRight[i]);
    check(malformedFinite, "core: non-finite audio samples become finite output");

    return failures == 0 ? 0 : 1;
}
