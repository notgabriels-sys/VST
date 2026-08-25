#include "GranularFreezeCore.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr double defaultRate = 44100.0;
constexpr double maximumRate = 384000.0;
constexpr double captureSeconds = 10.0;
constexpr std::uint32_t maximumChunk = 16384;
constexpr float pi = 3.14159265358979323846f;

float finiteClamp(float value, float fallback, float low, float high) noexcept
{
    return std::clamp(std::isfinite(value) ? value : fallback, low, high);
}

int positiveRounded(double value) noexcept
{
    if (! std::isfinite(value) || value <= 0.0)
        return 1;
    return static_cast<int>(std::clamp(std::round(value), 1.0,
        static_cast<double>(std::numeric_limits<int>::max())));
}
}

namespace gf
{
void GranularFreezeCore::prepare(double sampleRate, std::uint32_t maximumBlockSize)
{
    rate = std::isfinite(sampleRate) && sampleRate > 0.0
        ? std::min(sampleRate, maximumRate) : defaultRate;
    chunkCapacity = std::clamp(maximumBlockSize, 1U, maximumChunk);
    const auto captureSize = static_cast<std::size_t>(std::ceil(rate * captureSeconds));
    for (auto& channel : capture) channel.assign(captureSize, 0.0f);
    for (auto& channel : wet) channel.assign(chunkCapacity, 0.0f);
    for (std::size_t i = 0; i < 2; ++i)
    {
        captureReadPointers[i] = capture[i].data();
        wetWritePointers[i] = wet[i].data();
    }
    engine.prepare(rate);
    spectrum.prepare(rate);
    reset();
}

void GranularFreezeCore::reset() noexcept
{
    for (auto& channel : capture) std::fill(channel.begin(), channel.end(), 0.0f);
    for (auto& channel : wet) std::fill(channel.begin(), channel.end(), 0.0f);
    engine.reset();
    spectrum.reset();
    frozen = {};
    writePosition = validSamples = 0;
    freezeTarget = false;
    wetMix = transitionStart = transitionTarget = 0.0f;
    transitionLength = transitionPosition = 0;
    transitionActive = false;
}

void GranularFreezeCore::snapshot(float holdMs) noexcept
{
    if (validSamples <= 0 || capture[0].empty()) { frozen = {}; return; }
    const int requested = positiveRounded(holdMs * 0.001 * rate);
    const int held = std::clamp(requested, 1, validSamples);
    const int capacity = static_cast<int>(capture[0].size());
    frozen = { captureReadPointers.data(), 2, capacity, held,
               (writePosition - held + capacity) % capacity };
}

void GranularFreezeCore::beginTransition(float target, float milliseconds) noexcept
{
    transitionStart = wetMix;
    transitionTarget = std::clamp(target, 0.0f, 1.0f);
    transitionPosition = 0;
    const int full = positiveRounded(milliseconds * 0.001 * rate);
    transitionLength = positiveRounded(full * std::abs(transitionTarget - transitionStart));
    transitionActive = true;
}

bool GranularFreezeCore::advanceTransition() noexcept
{
    if (! transitionActive) return false;
    ++transitionPosition;
    if (transitionLength <= 1)
    {
        wetMix = transitionTarget;
        transitionActive = false;
        return transitionTarget == 0.0f;
    }
    const float progress = std::min(1.0f,
        static_cast<float>(transitionPosition) / (transitionLength - 1));
    const float curve = 0.5f - 0.5f * std::cos(pi * progress);
    wetMix = transitionStart + (transitionTarget - transitionStart) * curve;
    if (transitionPosition >= transitionLength - 1)
    {
        wetMix = transitionTarget;
        transitionActive = false;
        return transitionTarget == 0.0f;
    }
    return false;
}

bool GranularFreezeCore::fullyLive() const noexcept
{
    return ! freezeTarget && ! transitionActive && wetMix == 0.0f;
}

void GranularFreezeCore::process(const float* const inputs[2], float* const outputs[2],
                                 std::uint32_t frames, const ParameterValues& raw) noexcept
{
    if (inputs == nullptr || outputs == nullptr || inputs[0] == nullptr || inputs[1] == nullptr
        || outputs[0] == nullptr || outputs[1] == nullptr || capture[0].empty() || wet[0].empty())
        return;

    const bool requestedFreeze = std::isfinite(raw.freeze) && raw.freeze > 0.5f;
    const float crossfade = finiteClamp(raw.crossfadeMs, 30.0f, 1.0f, 500.0f);
    const float hold = finiteClamp(raw.holdMs, 1000.0f, 50.0f, 10000.0f);
    const GrainParameters grain {
        finiteClamp(raw.grainSizeMs, 80.0f, 5.0f, 200.0f),
        finiteClamp(raw.densityHz, 20.0f, 0.0f, 200.0f),
        finiteClamp(raw.position, 1.0f, 0.0f, 1.0f),
        finiteClamp(raw.pitch, 1.0f, 0.5f, 2.0f)
    };
    if (requestedFreeze != freezeTarget)
    {
        if (requestedFreeze && fullyLive()) { snapshot(hold); engine.reset(); }
        beginTransition(requestedFreeze ? 1.0f : 0.0f, crossfade);
        freezeTarget = requestedFreeze;
    }

    const int capacity = static_cast<int>(capture[0].size());
    for (std::uint32_t base = 0; base < frames;)
    {
        const std::uint32_t count = std::min(chunkCapacity, frames - base);
        PlanarBufferView wetView { wetWritePointers.data(), 2, chunkCapacity };
        std::fill_n(wet[0].data(), count, 0.0f);
        std::fill_n(wet[1].data(), count, 0.0f);
        if (freezeTarget || transitionActive || wetMix > 0.0f)
            engine.render(frozen, wetView, 0, count, grain);

        for (std::uint32_t i = 0; i < count; ++i)
        {
            const std::uint32_t host = base + i;
            // A malformed upstream node must not inject NaN/Inf into the
            // capture ring, wet path, or host output. Preserve all finite
            // samples exactly; only non-finite samples become silence.
            const float inLeft = std::isfinite(inputs[0][host])
                ? inputs[0][host] : 0.0f;
            const float inRight = std::isfinite(inputs[1][host])
                ? inputs[1][host] : 0.0f;
            outputs[0][host] = inLeft * (1.0f - wetMix) + wet[0][i] * wetMix;
            outputs[1][host] = inRight * (1.0f - wetMix) + wet[1][i] * wetMix;
            spectrum.push(0.5f * (outputs[0][host] + outputs[1][host]));
            if (transitionActive && advanceTransition()) { engine.reset(); frozen = {}; }
            if (fullyLive())
            {
                capture[0][writePosition] = inLeft;
                capture[1][writePosition] = inRight;
                writePosition = (writePosition + 1) % capacity;
                validSamples = std::min(validSamples + 1, capacity);
            }
        }
        base += count;
    }
}
}
