#pragma once

#include "GrainEngine.h"
#include "Parameters.h"

#include <array>
#include <cstdint>
#include <vector>

namespace gf
{
class GranularFreezeCore
{
public:
    void prepare(double sampleRate, std::uint32_t maximumBlockSize);
    void reset() noexcept;
    void process(const float* const inputs[2], float* const outputs[2],
                 std::uint32_t frames, const ParameterValues&) noexcept;

private:
    void snapshot(float holdMs) noexcept;
    void beginTransition(float target, float milliseconds) noexcept;
    bool advanceTransition() noexcept;
    bool fullyLive() const noexcept;

    std::array<std::vector<float>, 2> capture;
    std::array<std::vector<float>, 2> wet;
    std::array<const float*, 2> captureReadPointers {};
    std::array<float*, 2> wetWritePointers {};
    FrozenBufferView frozen {};
    GrainEngine engine;
    double rate = 44100.0;
    std::uint32_t chunkCapacity = 1;
    int writePosition = 0;
    int validSamples = 0;
    bool freezeTarget = false;
    float wetMix = 0.0f;
    float transitionStart = 0.0f;
    float transitionTarget = 0.0f;
    int transitionLength = 0;
    int transitionPosition = 0;
    bool transitionActive = false;
};
}
