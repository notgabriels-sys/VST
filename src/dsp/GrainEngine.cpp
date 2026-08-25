#include "GrainEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace
{
constexpr float twoPi = 6.28318530717958647692f;

int wrapIndex(int index, int span) noexcept
{
    index %= span;
    return index < 0 ? index + span : index;
}
}

namespace gf
{
bool FrozenBufferView::isReadable() const noexcept
{
    if (channels == nullptr || channelCount <= 0 || capacity <= 0 || span <= 0
        || span > capacity || oldestPhysicalIndex < 0
        || oldestPhysicalIndex >= capacity)
        return false;

    for (int channel = 0; channel < channelCount; ++channel)
        if (channels[channel] == nullptr)
            return false;
    return true;
}

float FrozenBufferView::readSample(int channel, double logicalPosition) const noexcept
{
    if (! isReadable() || channel < 0 || channel >= channelCount
        || ! std::isfinite(logicalPosition))
        return 0.0f;

    double wrappedPosition = std::fmod(logicalPosition, static_cast<double>(span));
    if (wrappedPosition < 0.0)
        wrappedPosition += static_cast<double>(span);

    const int base = static_cast<int>(std::floor(wrappedPosition));
    const float fraction = static_cast<float>(wrappedPosition - base);
    const auto sampleAt = [this, channel](int logicalIndex) noexcept {
        const int physical = (oldestPhysicalIndex + wrapIndex(logicalIndex, span)) % capacity;
        return channels[channel][physical];
    };

    const float p0 = sampleAt(base - 1);
    const float p1 = sampleAt(base);
    const float p2 = sampleAt(base + 1);
    const float p3 = sampleAt(base + 2);
    const float t2 = fraction * fraction;
    const float t3 = t2 * fraction;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * fraction
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

GrainParameters GrainEngine::sanitise(GrainParameters parameters) noexcept
{
    const GrainParameters defaults;
    const auto finiteOr = [](float value, float fallback) noexcept {
        return std::isfinite(value) ? value : fallback;
    };
    parameters.grainSizeMs = std::clamp(finiteOr(parameters.grainSizeMs, defaults.grainSizeMs), 5.0f, 200.0f);
    parameters.densityHz = std::clamp(finiteOr(parameters.densityHz, defaults.densityHz), 0.0f, 200.0f);
    parameters.position = std::clamp(finiteOr(parameters.position, defaults.position), 0.0f, 1.0f);
    parameters.pitch = std::clamp(finiteOr(parameters.pitch, defaults.pitch), 0.5f, 2.0f);
    return parameters;
}

void GrainEngine::prepare(double newSampleRate) noexcept
{
    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 0.0 ? newSampleRate : 44100.0;
    reset();
}

void GrainEngine::reset() noexcept
{
    for (auto& voice : voices)
        voice = {};
    samplesUntilNextLaunch = 0.0;
    currentLaunchInterval = 0.0;
    schedulingEnabled = false;
    nextLaunchOrder = 0;
    totalLaunchCount = 0;
    activity = 0.0f;
    visualVoices = {};
}

std::size_t GrainEngine::getActiveVoiceCount() const noexcept
{
    std::size_t count = 0;
    for (const auto& voice : voices)
        count += voice.active ? 1U : 0U;
    return count;
}

float GrainEngine::getSequencePhase() const noexcept
{
    if (! schedulingEnabled || ! std::isfinite(currentLaunchInterval)
        || currentLaunchInterval <= 0.0)
        return 0.0f;

    const double remaining = std::clamp(samplesUntilNextLaunch,
                                        0.0, currentLaunchInterval);
    return static_cast<float>(1.0 - remaining / currentLaunchInterval);
}

void GrainEngine::launchVoice(const FrozenBufferView& source,
                              const GrainParameters& parameters) noexcept
{
    if (! source.isReadable())
        return;
    const double duration = parameters.grainSizeMs * 0.001 * sampleRate;
    const double intMax = static_cast<double>(std::numeric_limits<int>::max());
    int grainSamples = 2;
    if (! std::isfinite(duration) || duration >= intMax)
        grainSamples = std::numeric_limits<int>::max();
    else if (duration > 2.0)
        grainSamples = static_cast<int>(std::round(duration));

    const double distance = static_cast<double>(grainSamples - 1) * parameters.pitch;
    std::int64_t sourceSpan = std::numeric_limits<std::int64_t>::max();
    if (std::isfinite(distance)
        && distance < static_cast<double>(std::numeric_limits<std::int64_t>::max() - 1))
        sourceSpan = 1 + static_cast<std::int64_t>(std::ceil(distance));

    const std::int64_t latestStart = static_cast<std::int64_t>(source.span) - sourceSpan;
    const double logicalStart = latestStart >= 0
        ? std::round(parameters.position * static_cast<double>(latestStart))
        : std::round(parameters.position * static_cast<double>(source.span - 1));

    Voice* selected = nullptr;
    for (auto& voice : voices)
        if (! voice.active) { selected = &voice; break; }
    if (selected == nullptr)
    {
        selected = &voices[0];
        for (std::size_t i = 1; i < voices.size(); ++i)
            if (voices[i].launchOrder < selected->launchOrder)
                selected = &voices[i];
    }
    *selected = { true, logicalStart, parameters.pitch, 0, grainSamples, nextLaunchOrder++ };
    ++totalLaunchCount;
}

void GrainEngine::render(const FrozenBufferView& source,
                         const PlanarBufferView& destination,
                         std::uint32_t start, std::uint32_t count,
                         GrainParameters parameters) noexcept
{
    if (! source.isReadable() || ! destination.isWritable() || count == 0
        || start > destination.sampleCount || count > destination.sampleCount - start)
        return;
    for (std::uint32_t channel = 0; channel < destination.channelCount; ++channel)
        std::fill_n(destination.channels[channel] + start, count, 0.0f);

    parameters = sanitise(parameters);
    if (parameters.densityHz <= 0.0f)
    {
        schedulingEnabled = false;
        samplesUntilNextLaunch = currentLaunchInterval = 0.0;
    }
    else
    {
        const double interval = std::max(1.0, sampleRate / parameters.densityHz);
        if (! schedulingEnabled)
        {
            schedulingEnabled = true;
            samplesUntilNextLaunch = 0.0;
        }
        else if (interval != currentLaunchInterval)
            samplesUntilNextLaunch = std::min(samplesUntilNextLaunch, interval);
        currentLaunchInterval = interval;
    }

    const int rightSource = source.channelCount > 1 ? 1 : 0;
    float peakWeight = 0.0f;
    for (std::uint32_t offset = 0; offset < count; ++offset)
    {
        if (schedulingEnabled && samplesUntilNextLaunch <= 0.0)
        {
            launchVoice(source, parameters);
            samplesUntilNextLaunch += currentLaunchInterval;
        }
        float left = 0.0f, right = 0.0f, weights = 0.0f;
        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;
            const double phase = static_cast<double>(voice.envelopeIndex) / (voice.envelopeLength - 1);
            const float weight = 0.5f - 0.5f * std::cos(twoPi * static_cast<float>(phase));
            left += source.readSample(0, voice.logicalReadPosition) * weight;
            right += source.readSample(rightSource, voice.logicalReadPosition) * weight;
            weights += weight;
            voice.logicalReadPosition += voice.sourceIncrement;
            if (++voice.envelopeIndex >= voice.envelopeLength)
                voice.active = false;
        }
        const float normaliser = std::max(1.0f, weights);
        left = std::isfinite(left) ? left / normaliser : 0.0f;
        right = std::isfinite(right) ? right / normaliser : 0.0f;
        peakWeight = std::max(peakWeight, weights);
        destination.channels[0][start + offset] = left;
        if (destination.channelCount > 1)
            destination.channels[1][start + offset] = right;
        if (schedulingEnabled)
            samplesUntilNextLaunch -= 1.0;
    }

    activity = std::clamp(peakWeight / 4.0f, 0.0f, 1.0f);
    visualVoices = {};
    for (const auto& voice : voices)
    {
        if (! voice.active)
            continue;

        const std::size_t slot = static_cast<std::size_t>(
            voice.launchOrder % visualVoiceCount);
        const float phase = voice.envelopeLength > 1
            ? static_cast<float>(voice.envelopeIndex)
                / static_cast<float>(voice.envelopeLength - 1)
            : 1.0f;
        visualVoices[slot] = {
            true,
            std::clamp(phase, 0.0f, 1.0f),
            std::clamp(0.5f - 0.5f * std::cos(twoPi * phase), 0.0f, 1.0f)
        };
    }
}
}
