#include "../src/dsp/Parameters.h"

#include <cmath>
#include <cstdio>
#include <cstring>

int main()
{
    const char* symbols[] { "freeze", "pitch", "crossfadeMs", "holdMs",
                            "grainSizeMs", "densityHz", "position" };
    const float defaults[] { 0.0f, 1.0f, 30.0f, 1000.0f, 80.0f, 20.0f, 1.0f };
    int failures = 0;
    if (gf::parameterCount != 7) ++failures;
    for (std::size_t i = 0; i < gf::parameterCount; ++i)
    {
        if (std::strcmp(gf::parameterDescriptors[i].symbol, symbols[i]) != 0)
            ++failures;
        if (std::abs(gf::parameterDescriptors[i].defaultValue - defaults[i]) > 1.0e-6f)
            ++failures;
    }
    std::printf("contract: seven stable parameter symbols/defaults %s\n",
                failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}
