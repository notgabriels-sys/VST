#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace gf
{
struct FrozenBufferView
{
    const juce::AudioBuffer<float>* buffer = nullptr;
    int capacity = 0;
    int span = 0;
    int oldestPhysicalIndex = 0;

    bool isReadable() const noexcept;
    float readSample (int channel, double logicalPosition) const noexcept;
};

struct GrainParameters
{
    float grainSizeMs = 80.0f;
    float densityHz = 20.0f;
    float position = 1.0f;
    float pitch = 1.0f;
};

struct GrainEngineTestAccess;

class GrainEngine
{
public:
    static constexpr std::size_t maxVoices = 64;

    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;
    void render (const FrozenBufferView&, juce::AudioBuffer<float>&,
                 int destinationStartSample, int numSamples,
                 GrainParameters) noexcept;

    std::uint64_t getTotalLaunchCount() const noexcept { return totalLaunchCount; }
    std::size_t getActiveVoiceCount() const noexcept;

private:
    friend struct GrainEngineTestAccess;

    struct Voice
    {
        bool active = false;
        double logicalReadPosition = 0.0;
        double sourceIncrement = 1.0;
        int envelopeIndex = 0;
        int envelopeLength = 2;
        std::uint64_t launchOrder = 0;
    };

    void launchVoice (const FrozenBufferView&, const GrainParameters&) noexcept;
    static GrainParameters sanitise (GrainParameters) noexcept;

    std::array<Voice, maxVoices> voices {};
    double sampleRate = 44100.0;
    double samplesUntilNextLaunch = 0.0;
    double currentLaunchInterval = 0.0;
    bool schedulingEnabled = false;
    std::uint64_t nextLaunchOrder = 0;
    std::uint64_t totalLaunchCount = 0;
};
}
