#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace gf
{
// This is intentionally a tiny, read-only bridge from the audio thread to the
// editor. The audio thread publishes fixed-size values; the editor only reads
// them during its idle callback. It is not part of the host parameter contract.
constexpr std::size_t visualVoiceCount = 8;
constexpr std::size_t spectrumBandCount = 24;

struct GranularFreezeTelemetry
{
    std::atomic<float> activity { 0.0f };
    std::atomic<float> sequencePhase { 0.0f };
    std::atomic<std::uint32_t> activeVoices { 0 };
    std::atomic<std::uint64_t> launchCount { 0 };
    std::array<std::atomic<float>, visualVoiceCount> voicePhases {};
    std::array<std::atomic<float>, visualVoiceCount> voiceEnvelopes {};
    std::array<std::atomic<float>, spectrumBandCount> spectrumLevels {};

    GranularFreezeTelemetry() noexcept
    {
        for (std::size_t i = 0; i < visualVoiceCount; ++i)
        {
            voicePhases[i].store(0.0f, std::memory_order_relaxed);
            voiceEnvelopes[i].store(0.0f, std::memory_order_relaxed);
        }
        for (std::size_t i = 0; i < spectrumBandCount; ++i)
            spectrumLevels[i].store(0.0f, std::memory_order_relaxed);
    }

    void clear() noexcept
    {
        activity.store(0.0f, std::memory_order_relaxed);
        sequencePhase.store(0.0f, std::memory_order_relaxed);
        activeVoices.store(0, std::memory_order_relaxed);
        launchCount.store(0, std::memory_order_relaxed);
        for (std::size_t i = 0; i < visualVoiceCount; ++i)
        {
            voicePhases[i].store(0.0f, std::memory_order_relaxed);
            voiceEnvelopes[i].store(0.0f, std::memory_order_relaxed);
        }
        for (std::size_t i = 0; i < spectrumBandCount; ++i)
            spectrumLevels[i].store(0.0f, std::memory_order_relaxed);
    }
};

class GranularFreezeTelemetrySource
{
public:
    virtual const GranularFreezeTelemetry& granularFreezeTelemetry() const noexcept = 0;

protected:
    virtual ~GranularFreezeTelemetrySource() = default;
};
}
