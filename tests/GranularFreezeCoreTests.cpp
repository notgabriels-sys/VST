#include "../src/dsp/GranularFreezeCore.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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

    return failures == 0 ? 0 : 1;
}
