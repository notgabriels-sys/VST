#include <JuceHeader.h>
#include "../src/GrainEngine.h"

#include <cmath>
#include <cstdio>
#include <limits>

namespace
{
int failures = 0;

void check (bool ok, const char* name)
{
    std::printf ("%-58s %s\n", name, ok ? "PASS" : "FAIL");
    if (! ok)
        ++failures;
}

bool near (float actual, float expected, float tolerance = 1.0e-5f)
{
    return std::abs (actual - expected) <= tolerance;
}

void fillLogical (juce::AudioBuffer<float>& buffer, int oldestPhysicalIndex, int span,
                  float firstValue)
{
    for (int logical = 0; logical < span; ++logical)
    {
        const int physical = (oldestPhysicalIndex + logical) % buffer.getNumSamples();
        buffer.setSample (0, physical, firstValue + (float) logical);
        if (buffer.getNumChannels() > 1)
            buffer.setSample (1, physical, firstValue + 100.0f + (float) logical);
    }
}
}

int main()
{
    juce::AudioBuffer<float> wrapped (2, 8);
    fillLogical (wrapped, 5, 8, 10.0f);

    const gf::FrozenBufferView view { &wrapped, 8, 8, 5 };
    check (view.isReadable(), "view: wrapped capture is readable");
    check (near (view.readSample (0, 0.0), 10.0f), "view: logical zero is oldest sample");
    check (near (view.readSample (0, 7.0), 17.0f), "view: logical end is newest sample");
    check (near (view.readSample (1, 2.0), 112.0f), "view: channel chronology is preserved");
    check (near (view.readSample (0, 2.5), 12.5f), "view: cubic interpolation handles linear data");
    check (near (view.readSample (0, 7.5), 13.5f),
           "view: fractional chronological seam wraps before interpolation");
    check (near (view.readSample (0, -1.0), 17.0f), "view: negative positions wrap in valid span");
    check (near (view.readSample (0, 8.0), 10.0f), "view: end positions wrap in valid span");

    juce::AudioBuffer<float> shortCapture (1, 8);
    shortCapture.clear();

    fillLogical (shortCapture, 6, 1, 30.0f);
    const gf::FrozenBufferView spanOne { &shortCapture, 8, 1, 6 };
    check (spanOne.isReadable(), "view: span 1 is readable");
    check (near (spanOne.readSample (0, 0.25), 30.0f),
           "view: span 1 fractional interpolation repeats its only sample");
    check (near (spanOne.readSample (0, -1.0), 30.0f),
           "view: span 1 negative position wraps");

    fillLogical (shortCapture, 6, 2, 40.0f);
    const gf::FrozenBufferView spanTwo { &shortCapture, 8, 2, 6 };
    check (spanTwo.isReadable(), "view: span 2 is readable");
    check (near (spanTwo.readSample (0, 0.5), 40.5f),
           "view: span 2 interpolates between chronological samples");
    check (near (spanTwo.readSample (0, 2.0), 40.0f),
           "view: span 2 end position wraps to oldest sample");

    fillLogical (shortCapture, 6, 3, 50.0f);
    const gf::FrozenBufferView spanThree { &shortCapture, 8, 3, 6 };
    check (spanThree.isReadable(), "view: span 3 is readable");
    check (near (spanThree.readSample (0, 1.5), 51.6875f),
           "view: span 3 performs cubic interpolation");
    check (near (spanThree.readSample (0, -0.5), 51.0f),
           "view: span 3 fractional negative position wraps");

    const gf::FrozenBufferView empty { &wrapped, 8, 0, 0 };
    check (! empty.isReadable(), "view: empty capture is unreadable");
    check (empty.readSample (0, 0.0) == 0.0f, "view: empty capture reads exact zero");

    const gf::FrozenBufferView nullBuffer { nullptr, 8, 8, 0 };
    check (! nullBuffer.isReadable(), "view: null buffer is unreadable");
    check (nullBuffer.readSample (0, 0.0) == 0.0f, "view: null buffer reads exact zero");

    const gf::FrozenBufferView zeroChannels { &wrapped, 8, 8, 0 };
    juce::AudioBuffer<float> noChannels (0, 8);
    const gf::FrozenBufferView noChannelStorage { &noChannels, 8, 8, 0 };
    check (! noChannelStorage.isReadable(), "view: zero-channel storage is unreadable");
    check (noChannelStorage.readSample (0, 0.0) == 0.0f,
           "view: zero-channel storage reads exact zero");
    check (zeroChannels.readSample (-1, 0.0) == 0.0f,
           "view: negative channel reads exact zero");
    check (zeroChannels.readSample (2, 0.0) == 0.0f,
           "view: out-of-range channel reads exact zero");

    const gf::FrozenBufferView negativeCapacity { &wrapped, -1, 1, 0 };
    check (! negativeCapacity.isReadable(), "view: negative capacity is rejected");
    check (negativeCapacity.readSample (0, 0.0) == 0.0f,
           "view: negative capacity reads exact zero");

    const gf::FrozenBufferView zeroCapacity { &wrapped, 0, 0, 0 };
    check (! zeroCapacity.isReadable(), "view: zero capacity is rejected");
    check (zeroCapacity.readSample (0, 0.0) == 0.0f,
           "view: zero capacity reads exact zero");

    const gf::FrozenBufferView negativeSpan { &wrapped, 8, -1, 0 };
    check (! negativeSpan.isReadable(), "view: negative span is rejected");
    check (negativeSpan.readSample (0, 0.0) == 0.0f,
           "view: negative span reads exact zero");

    const gf::FrozenBufferView spanBeyondCapacity { &wrapped, 8, 9, 0 };
    check (! spanBeyondCapacity.isReadable(), "view: span beyond capacity is rejected");
    check (spanBeyondCapacity.readSample (0, 0.0) == 0.0f,
           "view: span beyond capacity reads exact zero");

    const gf::FrozenBufferView capacityBeyondStorage { &wrapped, 9, 8, 0 };
    check (! capacityBeyondStorage.isReadable(), "view: capacity beyond storage is rejected");
    check (capacityBeyondStorage.readSample (0, 0.0) == 0.0f,
           "view: capacity beyond storage reads exact zero");

    const gf::FrozenBufferView negativeOldest { &wrapped, 8, 8, -1 };
    check (! negativeOldest.isReadable(), "view: negative oldest index is rejected");
    check (negativeOldest.readSample (0, 0.0) == 0.0f,
           "view: negative oldest index reads exact zero");

    const gf::FrozenBufferView oldestAtCapacity { &wrapped, 8, 8, 8 };
    check (! oldestAtCapacity.isReadable(), "view: oldest index at capacity is rejected");
    check (oldestAtCapacity.readSample (0, 0.0) == 0.0f,
           "view: oldest index at capacity reads exact zero");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double positiveInfinity = std::numeric_limits<double>::infinity();
    const double negativeInfinity = -positiveInfinity;
    check(view.readSample (0, nan) == 0.0f, "view: NaN position reads exact zero");
    check(view.readSample (0, positiveInfinity) == 0.0f, "view: +Inf position reads exact zero");
    check(view.readSample (0, negativeInfinity) == 0.0f, "view: -Inf position reads exact zero");

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
