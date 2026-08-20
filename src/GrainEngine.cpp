#include "GrainEngine.h"

#include <cmath>

namespace
{
int wrapIndex (int index, int span) noexcept
{
    index %= span;
    return index < 0 ? index + span : index;
}
}

namespace gf
{
bool FrozenBufferView::isReadable() const noexcept
{
    return buffer != nullptr
        && buffer->getNumChannels() > 0
        && capacity > 0
        && span > 0
        && span <= capacity
        && capacity <= buffer->getNumSamples()
        && oldestPhysicalIndex >= 0
        && oldestPhysicalIndex < capacity;
}

float FrozenBufferView::readSample (int channel, double logicalPosition) const noexcept
{
    if (! isReadable()
        || channel < 0
        || channel >= buffer->getNumChannels()
        || ! std::isfinite (logicalPosition))
        return 0.0f;

    double wrappedPosition = std::fmod (logicalPosition, (double) span);
    if (wrappedPosition < 0.0)
        wrappedPosition += (double) span;

    const int base = (int) std::floor (wrappedPosition);
    const float fraction = (float) (wrappedPosition - (double) base);

    auto sampleAt = [this, channel] (int logicalIndex) noexcept
    {
        const int wrappedLogical = wrapIndex (logicalIndex, span);
        const int physical = (oldestPhysicalIndex + wrappedLogical) % capacity;
        return buffer->getSample (channel, physical);
    };

    const float p0 = sampleAt (base - 1);
    const float p1 = sampleAt (base);
    const float p2 = sampleAt (base + 1);
    const float p3 = sampleAt (base + 2);
    const float t2 = fraction * fraction;
    const float t3 = t2 * fraction;

    return 0.5f * ((2.0f * p1)
        + (-p0 + p2) * fraction
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

GrainParameters GrainEngine::sanitise (GrainParameters parameters) noexcept
{
    const GrainParameters defaults;

    auto finiteOrDefault = [] (float value, float fallback) noexcept
    {
        return std::isfinite (value) ? value : fallback;
    };

    parameters.grainSizeMs = juce::jlimit (
        5.0f, 200.0f,
        finiteOrDefault (parameters.grainSizeMs, defaults.grainSizeMs));
    parameters.densityHz = juce::jlimit (
        0.0f, 200.0f,
        finiteOrDefault (parameters.densityHz, defaults.densityHz));
    parameters.position = juce::jlimit (
        0.0f, 1.0f,
        finiteOrDefault (parameters.position, defaults.position));
    parameters.pitch = juce::jlimit (
        0.5f, 2.0f,
        finiteOrDefault (parameters.pitch, defaults.pitch));

    return parameters;
}

void GrainEngine::prepare (double newSampleRate) noexcept
{
    sampleRate = std::isfinite (newSampleRate) && newSampleRate > 0.0
        ? newSampleRate
        : 44100.0;
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
}

std::size_t GrainEngine::getActiveVoiceCount() const noexcept
{
    std::size_t activeVoiceCount = 0;
    for (const auto& voice : voices)
        if (voice.active)
            ++activeVoiceCount;

    return activeVoiceCount;
}

void GrainEngine::launchVoice (const FrozenBufferView& source,
                               const GrainParameters& parameters) noexcept
{
    if (! source.isReadable())
        return;

    const int grainSamples = juce::jmax (2, juce::roundToInt (
        parameters.grainSizeMs * 0.001f * (float) sampleRate));
    const int sourceSpan = 1 + (int) std::ceil (
        (double) (grainSamples - 1) * (double) parameters.pitch);
    const int maxCompleteStart = source.span - sourceSpan;
    const double logicalStart = maxCompleteStart >= 0
        ? std::round ((double) parameters.position * (double) maxCompleteStart)
        : std::round ((double) parameters.position * (double) (source.span - 1));

    Voice* selectedVoice = nullptr;
    for (auto& voice : voices)
    {
        if (! voice.active)
        {
            selectedVoice = &voice;
            break;
        }
    }

    if (selectedVoice == nullptr)
    {
        selectedVoice = &voices[0];
        for (std::size_t index = 1; index < voices.size(); ++index)
            if (voices[index].launchOrder < selectedVoice->launchOrder)
                selectedVoice = &voices[index];
    }

    selectedVoice->active = true;
    selectedVoice->logicalReadPosition = logicalStart;
    selectedVoice->sourceIncrement = parameters.pitch;
    selectedVoice->envelopeIndex = 0;
    selectedVoice->envelopeLength = grainSamples;
    selectedVoice->launchOrder = nextLaunchOrder++;
    ++totalLaunchCount;
}

void GrainEngine::render (const FrozenBufferView& source,
                          juce::AudioBuffer<float>& destination,
                          int destinationStartSample, int numSamples,
                          GrainParameters parameters) noexcept
{
    if (! source.isReadable()
        || destination.getNumChannels() <= 0
        || destinationStartSample < 0
        || numSamples <= 0
        || destinationStartSample > destination.getNumSamples()
        || numSamples > destination.getNumSamples() - destinationStartSample)
        return;

    for (int channel = 0; channel < destination.getNumChannels(); ++channel)
        destination.clear (channel, destinationStartSample, numSamples);

    parameters = sanitise (parameters);

    if (parameters.densityHz <= 0.0f)
    {
        schedulingEnabled = false;
        samplesUntilNextLaunch = 0.0;
        currentLaunchInterval = 0.0;
    }
    else
    {
        const double nextInterval = juce::jmax (
            1.0, sampleRate / (double) parameters.densityHz);

        if (! schedulingEnabled)
        {
            schedulingEnabled = true;
            samplesUntilNextLaunch = 0.0;
        }
        else if (nextInterval != currentLaunchInterval)
        {
            samplesUntilNextLaunch = juce::jmin (
                samplesUntilNextLaunch, nextInterval);
        }

        currentLaunchInterval = nextInterval;
    }

    const int sourceRightChannel = source.buffer->getNumChannels() > 1 ? 1 : 0;

    for (int offset = 0; offset < numSamples; ++offset)
    {
        if (schedulingEnabled && samplesUntilNextLaunch <= 0.0)
        {
            launchVoice (source, parameters);
            samplesUntilNextLaunch += currentLaunchInterval;
        }

        float leftSum = 0.0f;
        float rightSum = 0.0f;
        float weightSum = 0.0f;

        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;

            const double phase = (double) voice.envelopeIndex
                               / (double) (voice.envelopeLength - 1);
            const float weight = 0.5f - 0.5f * std::cos (
                juce::MathConstants<float>::twoPi * (float) phase);

            leftSum += source.readSample (0, voice.logicalReadPosition) * weight;
            rightSum += source.readSample (sourceRightChannel,
                                           voice.logicalReadPosition) * weight;
            weightSum += weight;

            voice.logicalReadPosition += voice.sourceIncrement;
            ++voice.envelopeIndex;
            if (voice.envelopeIndex >= voice.envelopeLength)
                voice.active = false;
        }

        const float normalisation = juce::jmax (1.0f, weightSum);
        float leftOutput = leftSum / normalisation;
        float rightOutput = rightSum / normalisation;

        if (! std::isfinite (leftOutput))
            leftOutput = 0.0f;
        if (! std::isfinite (rightOutput))
            rightOutput = 0.0f;

        const int destinationSample = destinationStartSample + offset;
        destination.setSample (0, destinationSample, leftOutput);
        if (destination.getNumChannels() > 1)
            destination.setSample (1, destinationSample, rightOutput);

        if (schedulingEnabled)
            samplesUntilNextLaunch -= 1.0;
    }
}
}
