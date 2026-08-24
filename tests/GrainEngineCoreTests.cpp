#include "../src/dsp/GrainEngine.h"

#include <cmath>
#include <cstdio>

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

    return failures == 0 ? 0 : 1;
}
