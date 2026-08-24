#include "../src/dsp/GrainEngine.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace
{
int failures = 0;

void check(bool condition, const char* name)
{
    std::printf("%-58s %s\n", name, condition ? "PASS" : "FAIL");
    if (! condition)
        ++failures;
}

bool near(float actual, float expected, float tolerance = 1.0e-6f)
{
    return std::abs(actual - expected) <= tolerance;
}
}

int main()
{
    const float left[] = { 20.0f, 30.0f, 40.0f, 10.0f };
    const float right[] = { 2.0f, 3.0f, 4.0f, 1.0f };
    const float* channels[] = { left, right };

    const gf::FrozenBufferView view { channels, 2, 4, 4, 3 };

    check(view.isReadable(), "view: wrapped stereo source is readable");
    check(near(view.readSample(0, 0.0), 10.0f),
          "view: logical zero maps to oldest physical sample");
    check(near(view.readSample(0, 1.0), 20.0f),
          "view: chronological read wraps physical storage");
    check(near(view.readSample(1, 3.0), 4.0f),
          "view: right channel follows the same timeline");
    check(near(view.readSample(0, 0.5), 12.5f),
          "view: cubic interpolation uses wrapped neighbours");
    check(near(view.readSample(2, 0.0), 0.0f),
          "view: invalid channel reads silence");
    check(near(view.readSample(0, INFINITY), 0.0f),
          "view: non-finite positions read silence");

    std::vector<float> sourceLeft(256, 1.0f);
    std::vector<float> sourceRight(256, -0.5f);
    const float* sourceChannels[] { sourceLeft.data(), sourceRight.data() };
    const gf::FrozenBufferView source { sourceChannels, 2, 256, 256, 0 };
    std::vector<float> renderedLeft(101, 99.0f);
    std::vector<float> renderedRight(101, 99.0f);
    float* renderedChannels[] { renderedLeft.data(), renderedRight.data() };
    const gf::PlanarBufferView destination { renderedChannels, 2, 101 };
    gf::GrainEngine engine;
    engine.prepare(1000.0);
    engine.render(source, destination, 0, 101, { 20.0f, 100.0f, 0.0f, 1.0f });
    check(engine.getTotalLaunchCount() == 11,
          "engine: fractional scheduler launches without backlog");
    check(engine.getActiveVoiceCount() <= gf::GrainEngine::maxVoices,
          "engine: active voices stay inside fixed pool");
    bool stereoFinite = true;
    for (std::size_t i = 0; i < renderedLeft.size(); ++i)
        stereoFinite = stereoFinite && std::isfinite(renderedLeft[i])
            && std::isfinite(renderedRight[i])
            && near(renderedRight[i], -0.5f * renderedLeft[i], 2.0e-5f);
    check(stereoFinite, "engine: stereo timeline and normalization stay aligned");

    const auto launchesBeforeZero = engine.getTotalLaunchCount();
    engine.render(source, destination, 0, 101, { 20.0f, 0.0f, 0.0f, 1.0f });
    check(engine.getTotalLaunchCount() == launchesBeforeZero,
          "engine: zero density launches no additional grains");

    engine.reset();
    engine.render(source, destination, 0, 101,
                  { std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::quiet_NaN() });
    bool sanitisedFinite = true;
    for (const float sample : renderedLeft)
        sanitisedFinite = sanitisedFinite && std::isfinite(sample);
    check(sanitisedFinite, "engine: non-finite automation is sanitised");

    return failures == 0 ? 0 : 1;
}
