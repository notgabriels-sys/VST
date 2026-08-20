#include <JuceHeader.h>
#include "../src/GrainEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <vector>

namespace gf
{
struct GrainEngineTestAccess
{
    struct State
    {
        double sampleRate = 0.0;
        double samplesUntilNextLaunch = 0.0;
        double currentLaunchInterval = 0.0;
        bool schedulingEnabled = false;
        std::uint64_t nextLaunchOrder = 0;
        std::uint64_t totalLaunchCount = 0;
        std::array<bool, GrainEngine::maxVoices> active {};
        std::array<double, GrainEngine::maxVoices> readPositions {};
        std::array<int, GrainEngine::maxVoices> envelopeIndices {};
        std::array<std::uint64_t, GrainEngine::maxVoices> launchOrders {};
    };

    static GrainParameters sanitise (GrainParameters parameters) noexcept
    {
        return GrainEngine::sanitise (parameters);
    }

    static State state (const GrainEngine& engine) noexcept
    {
        State result;
        result.sampleRate = engine.sampleRate;
        result.samplesUntilNextLaunch = engine.samplesUntilNextLaunch;
        result.currentLaunchInterval = engine.currentLaunchInterval;
        result.schedulingEnabled = engine.schedulingEnabled;
        result.nextLaunchOrder = engine.nextLaunchOrder;
        result.totalLaunchCount = engine.totalLaunchCount;

        for (std::size_t index = 0; index < GrainEngine::maxVoices; ++index)
        {
            result.active[index] = engine.voices[index].active;
            result.readPositions[index] = engine.voices[index].logicalReadPosition;
            result.envelopeIndices[index] = engine.voices[index].envelopeIndex;
            result.launchOrders[index] = engine.voices[index].launchOrder;
        }

        return result;
    }

    static void launch (GrainEngine& engine, const FrozenBufferView& source,
                        const GrainParameters& parameters) noexcept
    {
        engine.launchVoice (source, parameters);
    }

    static void setActive (GrainEngine& engine, std::size_t index, bool active) noexcept
    {
        engine.voices[index].active = active;
    }

    static void setLaunchOrder (GrainEngine& engine, std::size_t index,
                                std::uint64_t launchOrder) noexcept
    {
        engine.voices[index].launchOrder = launchOrder;
    }

    static bool isActive (const GrainEngine& engine, std::size_t index) noexcept
    {
        return engine.voices[index].active;
    }

    static double readPosition (const GrainEngine& engine, std::size_t index) noexcept
    {
        return engine.voices[index].logicalReadPosition;
    }

    static double sourceIncrement (const GrainEngine& engine, std::size_t index) noexcept
    {
        return engine.voices[index].sourceIncrement;
    }

    static int envelopeLength (const GrainEngine& engine, std::size_t index) noexcept
    {
        return engine.voices[index].envelopeLength;
    }

    static std::uint64_t launchOrder (const GrainEngine& engine, std::size_t index) noexcept
    {
        return engine.voices[index].launchOrder;
    }
};
}

namespace
{
int failures = 0;

void check (bool ok, const char* name)
{
    std::printf ("%-74s %s\n", name, ok ? "PASS" : "FAIL");
    if (! ok)
        ++failures;
}

bool near (float actual, float expected, float tolerance = 1.0e-5f)
{
    return std::abs (actual - expected) <= tolerance;
}

bool nearDouble (double actual, double expected, double tolerance = 1.0e-10)
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

void fillConstant (juce::AudioBuffer<float>& buffer, float left, float right)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        if (buffer.getNumChannels() > 0)
            buffer.setSample (0, sample, left);
        if (buffer.getNumChannels() > 1)
            buffer.setSample (1, sample, right);
        for (int channel = 2; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, sample, left + (float) channel);
    }
}

void fillSine (juce::AudioBuffer<float>& buffer, double sampleRate, double frequency,
               bool identicalStereo)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float value = std::sin (juce::MathConstants<double>::twoPi
                                      * frequency * (double) sample / sampleRate);
        buffer.setSample (0, sample, value);
        if (buffer.getNumChannels() > 1)
            buffer.setSample (1, sample, identicalStereo ? value : -0.5f * value);
    }
}

void fillSegmentedLogical (juce::AudioBuffer<float>& buffer, int oldestPhysicalIndex,
                           int span)
{
    for (int logical = 0; logical < span; ++logical)
    {
        const int physical = (oldestPhysicalIndex + logical) % buffer.getNumSamples();
        const float value = logical < span / 2 ? -0.75f : 0.75f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, physical, value);
    }
}

bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
                return false;

    return true;
}

bool allEqual (const juce::AudioBuffer<float>& buffer, float expected)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (buffer.getSample (channel, sample) != expected)
                return false;

    return true;
}

bool regionEqual (const juce::AudioBuffer<float>& buffer, int channel,
                  int startSample, int numSamples, float expected)
{
    for (int sample = startSample; sample < startSample + numSamples; ++sample)
        if (buffer.getSample (channel, sample) != expected)
            return false;

    return true;
}

float maxStereoDifference (const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
        return 0.0f;

    float difference = 0.0f;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        difference = std::max (difference, std::abs (buffer.getSample (0, sample)
                                                    - buffer.getSample (1, sample)));
    return difference;
}

float peakMagnitude (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            peak = std::max (peak, std::abs (buffer.getSample (channel, sample)));
    return peak;
}

std::vector<float> channelSamples (const juce::AudioBuffer<float>& buffer, int channel)
{
    std::vector<float> result ((std::size_t) buffer.getNumSamples());
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        result[(std::size_t) sample] = buffer.getSample (channel, sample);
    return result;
}

std::vector<float> flatten (const juce::AudioBuffer<float>& buffer)
{
    std::vector<float> result;
    result.reserve ((std::size_t) buffer.getNumChannels()
                    * (std::size_t) buffer.getNumSamples());
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            result.push_back (buffer.getSample (channel, sample));
    return result;
}

double signedInteriorSum (const std::vector<float>& samples)
{
    double sum = 0.0;
    for (std::size_t index = 1; index + 1 < samples.size(); ++index)
        sum += samples[index];
    return sum;
}

int countZeroCrossings (const std::vector<float>& samples)
{
    int crossings = 0;
    int previousSign = 0;

    for (std::size_t index = 64; index + 64 < samples.size(); ++index)
    {
        const float value = samples[index];
        if (std::abs (value) < 1.0e-5f)
            continue;

        const int sign = value > 0.0f ? 1 : -1;
        if (previousSign != 0 && sign != previousSign)
            ++crossings;
        previousSign = sign;
    }

    return crossings;
}

bool sameState (const gf::GrainEngineTestAccess::State& left,
                const gf::GrainEngineTestAccess::State& right)
{
    return left.sampleRate == right.sampleRate
        && left.samplesUntilNextLaunch == right.samplesUntilNextLaunch
        && left.currentLaunchInterval == right.currentLaunchInterval
        && left.schedulingEnabled == right.schedulingEnabled
        && left.nextLaunchOrder == right.nextLaunchOrder
        && left.totalLaunchCount == right.totalLaunchCount
        && left.active == right.active
        && left.readPositions == right.readPositions
        && left.envelopeIndices == right.envelopeIndices
        && left.launchOrders == right.launchOrders;
}

juce::AudioBuffer<float> renderOneGrain (const gf::FrozenBufferView& source,
                                         float position)
{
    gf::GrainEngine engine;
    engine.prepare (1000.0);
    juce::AudioBuffer<float> output (2, 45);
    gf::GrainParameters parameters { 40.0f, 1.0f, position, 1.0f };
    engine.render (source, output, 0, output.getNumSamples(), parameters);
    return output;
}

juce::AudioBuffer<float> renderPitch (const gf::FrozenBufferView& source, float pitch)
{
    gf::GrainEngine engine;
    engine.prepare (48000.0);
    juce::AudioBuffer<float> output (2, 9600);
    gf::GrainParameters parameters { 200.0f, 1.0f, 0.0f, pitch };
    engine.render (source, output, 0, output.getNumSamples(), parameters);
    return output;
}

juce::AudioBuffer<float> renderDeterministic (const gf::FrozenBufferView& source)
{
    gf::GrainEngine engine;
    engine.prepare (48000.0);
    juce::AudioBuffer<float> output (2, 12000);
    gf::GrainParameters parameters { 80.0f, 37.0f, 0.37f, 1.23f };
    engine.render (source, output, 0, output.getNumSamples(), parameters);
    return output;
}

void renderCountedSamples (gf::GrainEngine& engine, const gf::FrozenBufferView& source,
                           int numSamples, gf::GrainParameters parameters)
{
    juce::AudioBuffer<float> output (2, std::max (1, numSamples));
    engine.render (source, output, 0, numSamples, parameters);
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

    const gf::FrozenBufferView channelBoundsView { &wrapped, 8, 8, 0 };
    juce::AudioBuffer<float> noChannels (0, 8);
    const gf::FrozenBufferView noChannelStorage { &noChannels, 8, 8, 0 };
    check (! noChannelStorage.isReadable(), "view: zero-channel storage is unreadable");
    check (noChannelStorage.readSample (0, 0.0) == 0.0f,
           "view: zero-channel storage reads exact zero");
    check (channelBoundsView.readSample (-1, 0.0) == 0.0f,
           "view: negative channel reads exact zero");
    check (channelBoundsView.readSample (2, 0.0) == 0.0f,
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
    check (view.readSample (0, nan) == 0.0f, "view: NaN position reads exact zero");
    check (view.readSample (0, positiveInfinity) == 0.0f, "view: +Inf position reads exact zero");
    check (view.readSample (0, negativeInfinity) == 0.0f, "view: -Inf position reads exact zero");

    constexpr double sampleRate = 48000.0;
    gf::GrainParameters defaults;
    check (near (defaults.grainSizeMs, 80.0f), "engine: default size is 80 ms");
    check (near (defaults.densityHz, 20.0f), "engine: default density is 20 grains/s");
    check (near (defaults.position, 1.0f), "engine: default position selects newest window");
    check (near (defaults.pitch, 1.0f), "engine: default pitch is unity");

    const float floatLowest = std::numeric_limits<float>::lowest();
    const float floatMaximum = std::numeric_limits<float>::max();
    const float nanFloat = std::numeric_limits<float>::quiet_NaN();
    const float positiveInfinityFloat = std::numeric_limits<float>::infinity();
    const float negativeInfinityFloat = -positiveInfinityFloat;

    check (near (gf::GrainEngineTestAccess::sanitise ({ floatLowest, 20.0f, 1.0f, 1.0f }).grainSizeMs, 5.0f),
           "engine: finite-low size sanitises exactly to 5 ms");
    check (near (gf::GrainEngineTestAccess::sanitise ({ floatMaximum, 20.0f, 1.0f, 1.0f }).grainSizeMs, 200.0f),
           "engine: finite-high size sanitises exactly to 200 ms");
    check (near (gf::GrainEngineTestAccess::sanitise ({ nanFloat, 20.0f, 1.0f, 1.0f }).grainSizeMs, 80.0f),
           "engine: NaN size sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ positiveInfinityFloat, 20.0f, 1.0f, 1.0f }).grainSizeMs, 80.0f),
           "engine: +Inf size sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ negativeInfinityFloat, 20.0f, 1.0f, 1.0f }).grainSizeMs, 80.0f),
           "engine: -Inf size sanitises exactly to its default");

    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, floatLowest, 1.0f, 1.0f }).densityHz, 0.0f),
           "engine: finite-low density sanitises exactly to zero");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, floatMaximum, 1.0f, 1.0f }).densityHz, 200.0f),
           "engine: finite-high density sanitises exactly to 200 Hz");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, nanFloat, 1.0f, 1.0f }).densityHz, 20.0f),
           "engine: NaN density sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, positiveInfinityFloat, 1.0f, 1.0f }).densityHz, 20.0f),
           "engine: +Inf density sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, negativeInfinityFloat, 1.0f, 1.0f }).densityHz, 20.0f),
           "engine: -Inf density sanitises exactly to its default");

    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, floatLowest, 1.0f }).position, 0.0f),
           "engine: finite-low position sanitises exactly to zero");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, floatMaximum, 1.0f }).position, 1.0f),
           "engine: finite-high position sanitises exactly to one");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, nanFloat, 1.0f }).position, 1.0f),
           "engine: NaN position sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, positiveInfinityFloat, 1.0f }).position, 1.0f),
           "engine: +Inf position sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, negativeInfinityFloat, 1.0f }).position, 1.0f),
           "engine: -Inf position sanitises exactly to its default");

    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, 1.0f, floatLowest }).pitch, 0.5f),
           "engine: finite-low pitch sanitises exactly to 0.5x");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, 1.0f, floatMaximum }).pitch, 2.0f),
           "engine: finite-high pitch sanitises exactly to 2x");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, 1.0f, nanFloat }).pitch, 1.0f),
           "engine: NaN pitch sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, 1.0f, positiveInfinityFloat }).pitch, 1.0f),
           "engine: +Inf pitch sanitises exactly to its default");
    check (near (gf::GrainEngineTestAccess::sanitise ({ 80.0f, 20.0f, 1.0f, negativeInfinityFloat }).pitch, 1.0f),
           "engine: -Inf pitch sanitises exactly to its default");

    juce::AudioBuffer<float> constantSource (2, 48000);
    fillConstant (constantSource, 1.0f, 1.0f);
    const gf::FrozenBufferView constantView { &constantSource, 48000, 48000, 0 };

    gf::GrainEngine lifecycleEngine;
    lifecycleEngine.prepare (96000.0);
    check (nearDouble (gf::GrainEngineTestAccess::state (lifecycleEngine).sampleRate, 96000.0),
           "engine: prepare accepts a finite positive sample rate");
    gf::GrainEngineTestAccess::launch (lifecycleEngine, constantView, defaults);
    lifecycleEngine.prepare (48000.0);
    auto lifecycleState = gf::GrainEngineTestAccess::state (lifecycleEngine);
    check (nearDouble (lifecycleState.sampleRate, 48000.0)
           && lifecycleState.totalLaunchCount == 0
           && lifecycleState.nextLaunchOrder == 0
           && lifecycleState.samplesUntilNextLaunch == 0.0
           && lifecycleState.currentLaunchInterval == 0.0
           && ! lifecycleState.schedulingEnabled
           && lifecycleEngine.getActiveVoiceCount() == 0,
           "engine: prepare resets voices, counters, and scheduler state");

    const std::array<double, 5> invalidSampleRates {
        0.0, -1.0, nan, positiveInfinity, negativeInfinity
    };
    const std::array<const char*, 5> invalidSampleRateChecks {
        "engine: zero sample rate falls back exactly to 44.1 kHz",
        "engine: negative sample rate falls back exactly to 44.1 kHz",
        "engine: NaN sample rate falls back exactly to 44.1 kHz",
        "engine: +Inf sample rate falls back exactly to 44.1 kHz",
        "engine: -Inf sample rate falls back exactly to 44.1 kHz"
    };
    for (std::size_t index = 0; index < invalidSampleRates.size(); ++index)
    {
        gf::GrainEngineTestAccess::launch (lifecycleEngine, constantView, defaults);
        lifecycleEngine.prepare (invalidSampleRates[index]);
        const auto prepared = gf::GrainEngineTestAccess::state (lifecycleEngine);
        check (prepared.sampleRate == 44100.0
               && prepared.totalLaunchCount == 0
               && prepared.nextLaunchOrder == 0
               && prepared.samplesUntilNextLaunch == 0.0
               && prepared.currentLaunchInterval == 0.0
               && ! prepared.schedulingEnabled
               && lifecycleEngine.getActiveVoiceCount() == 0,
               invalidSampleRateChecks[index]);
    }

    lifecycleEngine.prepare (1000.0);
    renderCountedSamples (lifecycleEngine, constantView, 1,
                          { 5.0f, 100.0f, 0.0f, 1.0f });
    lifecycleEngine.reset();
    lifecycleState = gf::GrainEngineTestAccess::state (lifecycleEngine);
    check (lifecycleState.sampleRate == 1000.0
           && lifecycleState.totalLaunchCount == 0
           && lifecycleState.nextLaunchOrder == 0
           && lifecycleState.samplesUntilNextLaunch == 0.0
           && lifecycleState.currentLaunchInterval == 0.0
           && ! lifecycleState.schedulingEnabled
           && lifecycleEngine.getActiveVoiceCount() == 0,
           "engine: reset preserves rate and zeros complete lifecycle state");
    renderCountedSamples (lifecycleEngine, constantView, 1,
                          { 5.0f, 100.0f, 0.0f, 1.0f });
    check (lifecycleEngine.getTotalLaunchCount() == 1,
           "engine: reset restores immediate first-positive launch");

    gf::GrainEngine silentEngine;
    silentEngine.prepare (sampleRate);
    juce::AudioBuffer<float> silentOut (2, 4800);
    fillConstant (silentOut, 0.25f, -0.25f);
    gf::GrainParameters zeroDensity = defaults;
    zeroDensity.densityHz = 0.0f;
    silentEngine.render (constantView, silentOut, 0, silentOut.getNumSamples(), zeroDensity);
    check (silentOut.getMagnitude (0, silentOut.getNumSamples()) == 0.0f,
           "engine: density zero renders silence");
    check (silentEngine.getTotalLaunchCount() == 0,
           "engine: density zero launches no voices");

    gf::GrainEngine hannEngine;
    hannEngine.prepare (sampleRate);
    juce::AudioBuffer<float> hannOut (2, 600);
    gf::GrainParameters shortGrain { 10.0f, 1.0f, 0.0f, 1.0f };
    hannEngine.render (constantView, hannOut, 0, hannOut.getNumSamples(), shortGrain);
    check (near (hannOut.getSample (0, 0), 0.0f), "engine: Hann starts at zero");
    check (hannOut.getSample (0, 240) > 0.99f, "engine: Hann reaches unity near centre");
    check (near (hannOut.getSample (0, 479), 0.0f), "engine: 10 ms grain ends at sample 479");
    check (near (hannOut.getSample (0, 480), 0.0f), "engine: completed voice is inactive");

    gf::GrainEngine oddHannEngine;
    oddHannEngine.prepare (1000.0);
    juce::AudioBuffer<float> oddHannOut (2, 6);
    oddHannEngine.render (constantView, oddHannOut, 0, oddHannOut.getNumSamples(),
                          { 5.0f, 1.0f, 0.0f, 1.0f });
    check (near (oddHannOut.getSample (0, 0), 0.0f)
           && near (oddHannOut.getSample (0, 4), 0.0f),
           "engine: odd Hann has exact zero endpoints");
    check (near (oddHannOut.getSample (0, 1), 0.5f)
           && near (oddHannOut.getSample (0, 2), 1.0f)
           && near (oddHannOut.getSample (0, 3), 0.5f),
           "engine: odd Hann has exact half-unity-half centre");
    check (near (oddHannOut.getSample (0, 5), 0.0f)
           && oddHannEngine.getActiveVoiceCount() == 0,
           "engine: odd Hann voice completes immediately after its endpoint");

    gf::GrainEngine densityEngine;
    densityEngine.prepare (sampleRate);
    juce::AudioBuffer<float> oneSecond (2, 48000);
    densityEngine.render (constantView, oneSecond, 0, oneSecond.getNumSamples(), defaults);
    check (densityEngine.getTotalLaunchCount() == 20,
           "engine: 20 Hz launches exactly 20 grains in one second");
    check (densityEngine.getActiveVoiceCount() <= gf::GrainEngine::maxVoices,
           "engine: active voices stay inside fixed pool");

    gf::GrainEngine integralScheduler;
    integralScheduler.prepare (100.0);
    const gf::GrainParameters integralDensity { 200.0f, 20.0f, 0.0f, 1.0f };
    renderCountedSamples (integralScheduler, constantView, 1, integralDensity);
    check (integralScheduler.getTotalLaunchCount() == 1,
           "engine: integral interval launches exactly at sample zero");
    renderCountedSamples (integralScheduler, constantView, 4, integralDensity);
    check (integralScheduler.getTotalLaunchCount() == 1,
           "engine: integral interval does not launch before sample five");
    renderCountedSamples (integralScheduler, constantView, 1, integralDensity);
    check (integralScheduler.getTotalLaunchCount() == 2,
           "engine: integral interval launches exactly at sample five");

    gf::GrainEngine fractionalScheduler;
    fractionalScheduler.prepare (10.0);
    const gf::GrainParameters fractionalDensity { 200.0f, 4.0f, 0.0f, 1.0f };
    renderCountedSamples (fractionalScheduler, constantView, 1, fractionalDensity);
    check (fractionalScheduler.getTotalLaunchCount() == 1,
           "engine: non-integral interval launches exactly at sample zero");
    renderCountedSamples (fractionalScheduler, constantView, 2, fractionalDensity);
    check (fractionalScheduler.getTotalLaunchCount() == 1,
           "engine: 2.5-sample interval waits through sample two");
    renderCountedSamples (fractionalScheduler, constantView, 1, fractionalDensity);
    check (fractionalScheduler.getTotalLaunchCount() == 2,
           "engine: 2.5-sample interval next launches at sample three");
    renderCountedSamples (fractionalScheduler, constantView, 1, fractionalDensity);
    check (fractionalScheduler.getTotalLaunchCount() == 2,
           "engine: 2.5-sample interval waits at sample four");
    renderCountedSamples (fractionalScheduler, constantView, 1, fractionalDensity);
    check (fractionalScheduler.getTotalLaunchCount() == 3,
           "engine: 2.5-sample interval next launches at sample five");

    gf::GrainEngine shorteningScheduler;
    shorteningScheduler.prepare (100.0);
    renderCountedSamples (shorteningScheduler, constantView, 1,
                          { 200.0f, 10.0f, 0.0f, 1.0f });
    renderCountedSamples (shorteningScheduler, constantView, 4,
                          { 200.0f, 25.0f, 0.0f, 1.0f });
    check (shorteningScheduler.getTotalLaunchCount() == 1,
           "engine: shortened interval clamps wait without early launch");
    renderCountedSamples (shorteningScheduler, constantView, 1,
                          { 200.0f, 25.0f, 0.0f, 1.0f });
    check (shorteningScheduler.getTotalLaunchCount() == 2,
           "engine: shortened interval launches at the clamped boundary");

    gf::GrainEngine lengtheningScheduler;
    lengtheningScheduler.prepare (100.0);
    renderCountedSamples (lengtheningScheduler, constantView, 1,
                          { 200.0f, 25.0f, 0.0f, 1.0f });
    renderCountedSamples (lengtheningScheduler, constantView, 3,
                          { 200.0f, 10.0f, 0.0f, 1.0f });
    check (lengtheningScheduler.getTotalLaunchCount() == 1,
           "engine: lengthened interval preserves the shorter pending wait");
    renderCountedSamples (lengtheningScheduler, constantView, 1,
                          { 200.0f, 10.0f, 0.0f, 1.0f });
    check (lengtheningScheduler.getTotalLaunchCount() == 2,
           "engine: lengthened interval launches at the preserved boundary");

    gf::GrainEngine immediateEngine;
    immediateEngine.prepare (sampleRate);
    renderCountedSamples (immediateEngine, constantView, 1, zeroDensity);
    renderCountedSamples (immediateEngine, constantView, 1, defaults);
    check (immediateEngine.getTotalLaunchCount() == 1,
           "engine: zero-to-positive density launches immediately");

    gf::GrainEngine onePerSampleEngine;
    onePerSampleEngine.prepare (100.0);
    renderCountedSamples (onePerSampleEngine, constantView, 10,
                          { 200.0f, 200.0f, 0.0f, 1.0f });
    check (onePerSampleEngine.getTotalLaunchCount() == 10,
           "engine: sub-sample interval launches at most once per sample");

    gf::GrainEngine completionEngine;
    completionEngine.prepare (1000.0);
    juce::AudioBuffer<float> completionStart (2, 1);
    completionEngine.render (constantView, completionStart, 0, 1,
                             { 5.0f, 100.0f, 0.0f, 1.0f });
    juce::AudioBuffer<float> completionTail (2, 4);
    completionEngine.render (constantView, completionTail, 0, 4,
                             { 5.0f, 0.0f, 0.0f, 1.0f });
    check (completionTail.getSample (0, 1) > 0.99f,
           "engine: density zero lets an active voice render to its centre");
    check (completionEngine.getTotalLaunchCount() == 1
           && completionEngine.getActiveVoiceCount() == 0,
           "engine: density zero lets voices complete without relaunching");

    juce::AudioBuffer<float> segmentedSource (2, 100);
    fillSegmentedLogical (segmentedSource, 73, 100);
    const gf::FrozenBufferView wrappedSegmentedView { &segmentedSource, 100, 100, 73 };
    check (signedInteriorSum (channelSamples (renderOneGrain (wrappedSegmentedView, 0.0f), 0)) < -10.0,
           "engine: position zero selects old segment");
    check (signedInteriorSum (channelSamples (renderOneGrain (wrappedSegmentedView, 1.0f), 0)) > 10.0,
           "engine: position one selects newest complete segment");

    juce::AudioBuffer<float> rampSource (1, 12);
    for (int sample = 0; sample < rampSource.getNumSamples(); ++sample)
        rampSource.setSample (0, sample, (float) sample);
    const gf::FrozenBufferView rampView { &rampSource, 12, 12, 0 };
    gf::GrainEngine fractionalPositionEngine;
    fractionalPositionEngine.prepare (1000.0);
    gf::GrainEngineTestAccess::launch (fractionalPositionEngine, rampView,
                                       { 5.0f, 1.0f, 0.5f, 1.0f });
    check (nearDouble (gf::GrainEngineTestAccess::readPosition (fractionalPositionEngine, 0), 4.0),
           "engine: fractional position rounds the complete-window start");
    check (gf::GrainEngineTestAccess::envelopeLength (fractionalPositionEngine, 0) == 5
           && nearDouble (gf::GrainEngineTestAccess::sourceIncrement (fractionalPositionEngine, 0), 1.0),
           "engine: launch captures grain length and source increment");

    juce::AudioBuffer<float> sineSource (2, 48000);
    fillSine (sineSource, sampleRate, 220.0, true);
    const gf::FrozenBufferView sineView { &sineSource, 48000, 48000, 0 };
    const auto pitchHalfRender = renderPitch (sineView, 0.5f);
    const auto pitchUnityRender = renderPitch (sineView, 1.0f);
    const auto pitchDoubleRender = renderPitch (sineView, 2.0f);
    const int pitchHalfCrossings = countZeroCrossings (channelSamples (pitchHalfRender, 0));
    const int pitchUnityCrossings = countZeroCrossings (channelSamples (pitchUnityRender, 0));
    const int pitchDoubleCrossings = countZeroCrossings (channelSamples (pitchDoubleRender, 0));
    const double halfRatio = (double) pitchHalfCrossings / (double) pitchUnityCrossings;
    const double doubleRatio = (double) pitchDoubleCrossings / (double) pitchUnityCrossings;
    check (pitchUnityCrossings > 0 && halfRatio >= 0.45 && halfRatio <= 0.55,
           "engine: pitch 0.5 has the expected zero-crossing ratio");
    check (pitchUnityCrossings > 0 && doubleRatio >= 1.9 && doubleRatio <= 2.1,
           "engine: pitch 2.0 has the expected zero-crossing ratio");

    check (maxStereoDifference (pitchUnityRender) < 1.0e-6f,
           "engine: identical stereo stays sample-aligned");

    juce::AudioBuffer<float> shortGrainSource (1, 3);
    shortGrainSource.setSample (0, 0, 0.2f);
    shortGrainSource.setSample (0, 1, -0.4f);
    shortGrainSource.setSample (0, 2, 0.8f);
    const gf::FrozenBufferView shortGrainView { &shortGrainSource, 3, 3, 0 };
    gf::GrainEngine shortPositionZeroEngine;
    shortPositionZeroEngine.prepare (1000.0);
    juce::AudioBuffer<float> shortPositionZero (2, 201);
    shortPositionZeroEngine.render (shortGrainView, shortPositionZero, 0, 201,
                                    { 200.0f, 1.0f, 0.0f, 2.0f });
    gf::GrainEngine shortPositionOneEngine;
    shortPositionOneEngine.prepare (1000.0);
    juce::AudioBuffer<float> shortPositionOne (2, 201);
    shortPositionOneEngine.render (shortGrainView, shortPositionOne, 0, 201,
                                   { 200.0f, 1.0f, 1.0f, 2.0f });
    check (allFinite (shortPositionZero),
           "engine: source shorter than grain at position zero wraps and stays finite");
    check (allFinite (shortPositionOne),
           "engine: source shorter than grain wraps and stays finite");
    check (flatten (shortPositionZero) != flatten (shortPositionOne),
           "engine: short source positions zero and one retain distinct starts");

    juce::AudioBuffer<float> asymmetricSource (2, 16);
    fillConstant (asymmetricSource, 0.25f, -0.5f);
    const gf::FrozenBufferView asymmetricView { &asymmetricSource, 16, 16, 0 };
    gf::GrainEngine asymmetricEngine;
    asymmetricEngine.prepare (1000.0);
    juce::AudioBuffer<float> asymmetricRender (2, 6);
    asymmetricEngine.render (asymmetricView, asymmetricRender, 0, 6,
                             { 5.0f, 1.0f, 0.0f, 1.0f });
    check (near (asymmetricRender.getSample (0, 2), 0.25f)
           && near (asymmetricRender.getSample (1, 2), -0.5f),
           "engine: asymmetric stereo preserves each channel at shared phase");

    juce::AudioBuffer<float> monoSource (1, 16);
    fillConstant (monoSource, 0.4f, 0.0f);
    const gf::FrozenBufferView monoView { &monoSource, 16, 16, 0 };
    gf::GrainEngine monoToStereoEngine;
    monoToStereoEngine.prepare (1000.0);
    juce::AudioBuffer<float> monoToStereo (2, 6);
    monoToStereoEngine.render (monoView, monoToStereo, 0, 6,
                               { 5.0f, 1.0f, 0.0f, 1.0f });
    check (near (monoToStereo.getSample (0, 2), 0.4f)
           && near (monoToStereo.getSample (1, 2), 0.4f),
           "engine: mono source is mirrored to stereo destination");

    gf::GrainEngine monoDestinationEngine;
    monoDestinationEngine.prepare (1000.0);
    juce::AudioBuffer<float> monoDestination (1, 6);
    monoDestinationEngine.render (asymmetricView, monoDestination, 0, 6,
                                  { 5.0f, 1.0f, 0.0f, 1.0f });
    check (allFinite (monoDestination)
           && near (monoDestination.getSample (0, 2), 0.25f),
           "engine: mono destination renders the source left channel safely");

    gf::GrainEngine multichannelDestinationEngine;
    multichannelDestinationEngine.prepare (1000.0);
    juce::AudioBuffer<float> multichannelDestination (4, 10);
    fillConstant (multichannelDestination, 0.33f, 0.33f);
    for (int channel = 2; channel < multichannelDestination.getNumChannels(); ++channel)
        for (int sample = 0; sample < multichannelDestination.getNumSamples(); ++sample)
            multichannelDestination.setSample (channel, sample, 0.33f);
    multichannelDestinationEngine.render (monoView, multichannelDestination, 2, 6,
                                          { 5.0f, 1.0f, 0.0f, 1.0f });
    check (regionEqual (multichannelDestination, 2, 2, 6, 0.0f)
           && regionEqual (multichannelDestination, 3, 2, 6, 0.0f),
           "engine: destination channels above stereo are cleared");
    check (regionEqual (multichannelDestination, 0, 0, 2, 0.33f)
           && regionEqual (multichannelDestination, 0, 8, 2, 0.33f)
           && near (multichannelDestination.getSample (0, 4), 0.4f),
           "engine: render clears and writes only the requested destination region");

    juce::AudioBuffer<float> nonFiniteSource (1, 16);
    fillConstant (nonFiniteSource, nanFloat, 0.0f);
    const gf::FrozenBufferView nonFiniteView { &nonFiniteSource, 16, 16, 0 };
    gf::GrainEngine nonFiniteEngine;
    nonFiniteEngine.prepare (1000.0);
    juce::AudioBuffer<float> nonFiniteRender (2, 5);
    nonFiniteEngine.render (nonFiniteView, nonFiniteRender, 0, 5,
                            { 5.0f, 1.0f, 0.0f, 1.0f });
    check (allFinite (nonFiniteRender) && allEqual (nonFiniteRender, 0.0f),
           "engine: non-finite source results are replaced with exact zero");

    gf::GrainEngine invalidInputEngine;
    invalidInputEngine.prepare (1000.0);
    const gf::GrainParameters tenSampleInterval { 200.0f, 100.0f, 0.0f, 1.0f };
    renderCountedSamples (invalidInputEngine, constantView, 1, tenSampleInterval);
    const auto validStateBeforeInvalid = gf::GrainEngineTestAccess::state (invalidInputEngine);
    juce::AudioBuffer<float> invalidSourceDestination (2, 8);
    fillConstant (invalidSourceDestination, 0.33f, 0.33f);
    invalidInputEngine.render (nullBuffer, invalidSourceDestination, 0, 8,
                               tenSampleInterval);
    check (allEqual (invalidSourceDestination, 0.33f),
           "engine: unreadable source leaves destination untouched");
    check (sameState (validStateBeforeInvalid,
                      gf::GrainEngineTestAccess::state (invalidInputEngine)),
           "engine: unreadable source leaves scheduler and voices untouched");

    auto checkInvalidDestination = [&] (int startSample, int numSamples,
                                        const char* outputCheck,
                                        const char* stateCheck)
    {
        juce::AudioBuffer<float> destination (2, 8);
        fillConstant (destination, 0.33f, 0.33f);
        invalidInputEngine.render (constantView, destination, startSample, numSamples,
                                   tenSampleInterval);
        check (allEqual (destination, 0.33f), outputCheck);
        check (sameState (validStateBeforeInvalid,
                          gf::GrainEngineTestAccess::state (invalidInputEngine)),
               stateCheck);
    };

    checkInvalidDestination (-1, 4,
                             "engine: negative destination start leaves output untouched",
                             "engine: negative destination start leaves scheduler untouched");
    checkInvalidDestination (0, -1,
                             "engine: negative destination length leaves output untouched",
                             "engine: negative destination length leaves scheduler untouched");
    checkInvalidDestination (9, 0,
                             "engine: destination start past end leaves output untouched",
                             "engine: destination start past end leaves scheduler untouched");
    checkInvalidDestination (6, 3,
                             "engine: overflowing destination range leaves output untouched",
                             "engine: overflowing destination range leaves scheduler untouched");

    juce::AudioBuffer<float> noChannelDestination (0, 8);
    invalidInputEngine.render (constantView, noChannelDestination, 0, 8,
                               tenSampleInterval);
    check (sameState (validStateBeforeInvalid,
                      gf::GrainEngineTestAccess::state (invalidInputEngine)),
           "engine: zero-channel destination leaves scheduler untouched");

    juce::AudioBuffer<float> zeroLengthDestination (2, 8);
    fillConstant (zeroLengthDestination, 0.33f, 0.33f);
    invalidInputEngine.render (constantView, zeroLengthDestination, 4, 0,
                               tenSampleInterval);
    check (allEqual (zeroLengthDestination, 0.33f)
           && sameState (validStateBeforeInvalid,
                         gf::GrainEngineTestAccess::state (invalidInputEngine)),
           "engine: zero-length destination is an exact no-op");

    renderCountedSamples (invalidInputEngine, constantView, 9, tenSampleInterval);
    check (invalidInputEngine.getTotalLaunchCount() == 1,
           "engine: invalid calls do not move the pending launch boundary");
    renderCountedSamples (invalidInputEngine, constantView, 1, tenSampleInterval);
    check (invalidInputEngine.getTotalLaunchCount() == 2,
           "engine: launch still occurs at exact boundary after invalid calls");

    check (gf::GrainEngine::maxVoices == 64,
           "engine: fixed voice pool contains exactly 64 voices");
    gf::GrainEngine voicePoolEngine;
    voicePoolEngine.prepare (1000.0);
    gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    gf::GrainEngineTestAccess::setActive (voicePoolEngine, 1, false);
    gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    check (gf::GrainEngineTestAccess::isActive (voicePoolEngine, 1)
           && gf::GrainEngineTestAccess::launchOrder (voicePoolEngine, 1) == 3
           && ! gf::GrainEngineTestAccess::isActive (voicePoolEngine, 3),
           "engine: launch selects the first inactive voice");

    voicePoolEngine.reset();
    for (std::size_t index = 0; index < gf::GrainEngine::maxVoices; ++index)
        gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    check (voicePoolEngine.getActiveVoiceCount() == 64
           && voicePoolEngine.getTotalLaunchCount() == 64,
           "engine: all 64 fixed voices can be active without allocation");
    gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    check (gf::GrainEngineTestAccess::launchOrder (voicePoolEngine, 0) == 64
           && gf::GrainEngineTestAccess::launchOrder (voicePoolEngine, 1) == 1,
           "engine: full pool deterministically replaces oldest launch");

    for (std::size_t index = 0; index < gf::GrainEngine::maxVoices; ++index)
        gf::GrainEngineTestAccess::setLaunchOrder (voicePoolEngine, index,
                                                  100 + (std::uint64_t) index);
    gf::GrainEngineTestAccess::setLaunchOrder (voicePoolEngine, 5, 1);
    gf::GrainEngineTestAccess::setLaunchOrder (voicePoolEngine, 7, 1);
    gf::GrainEngineTestAccess::launch (voicePoolEngine, constantView, defaults);
    check (gf::GrainEngineTestAccess::launchOrder (voicePoolEngine, 5) == 65
           && gf::GrainEngineTestAccess::launchOrder (voicePoolEngine, 7) == 1,
           "engine: oldest-order tie deterministically replaces lowest index");

    const auto renderA = flatten (renderDeterministic (sineView));
    const auto renderB = flatten (renderDeterministic (sineView));
    check (renderA == renderB,
           "engine: identical state and input are sample-identical");

    gf::GrainEngine maximumOverlapEngine;
    maximumOverlapEngine.prepare (sampleRate);
    juce::AudioBuffer<float> maximumOverlapRender (2, 48000);
    const gf::GrainParameters maximumOverlap { 200.0f, 200.0f, 0.0f, 1.0f };
    std::size_t maximumObservedVoices = 0;
    for (int sample = 0; sample < maximumOverlapRender.getNumSamples(); ++sample)
    {
        maximumOverlapEngine.render (constantView, maximumOverlapRender, sample, 1,
                                     maximumOverlap);
        maximumObservedVoices = std::max (maximumObservedVoices,
                                          maximumOverlapEngine.getActiveVoiceCount());
    }
    const float maximumOverlapPeak = peakMagnitude (maximumOverlapRender);
    check (allFinite (maximumOverlapRender),
           "engine: maximum size and density remain finite");
    check (maximumOverlapPeak <= 1.25f,
           "engine: overlap normalization controls gain");
    check (maximumOverlapPeak > 0.99f
           && maximumOverlapEngine.getTotalLaunchCount() == 200,
           "engine: maximum overlap produces meaningful scheduled signal");
    check (maximumObservedVoices == 40
           && maximumObservedVoices <= gf::GrainEngine::maxVoices,
           "engine: maximum overlap stays at the theoretical 40-voice bound");

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
