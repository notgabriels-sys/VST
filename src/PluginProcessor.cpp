#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace
{
// Hosts can report corrupt or merely unrealistic preparation values. 384 kHz
// covers a practical upper edge for audio production while bounding the
// ten-second stereo capture allocation. Non-finite/non-positive rates fall
// back to 44.1 kHz; larger finite rates clamp to this ceiling.
constexpr double defaultSampleRate = 44100.0;
constexpr double maximumSupportedSampleRate = 384000.0;
constexpr double captureSeconds = 10.0;
constexpr int defaultCaptureSamples = 441000;

// Host buffers larger than this are already processed in bounded chunks, so a
// larger preparation hint must not enlarge audio-thread scratch storage.
constexpr int maximumPreparedChunkSamples = 16384;

double sanitisePreparationSampleRate (double requestedSampleRate) noexcept
{
    if (! std::isfinite (requestedSampleRate) || requestedSampleRate <= 0.0)
        return defaultSampleRate;

    return juce::jmin (requestedSampleRate, maximumSupportedSampleRate);
}

int checkedCeilToPositiveInt (double value, int fallback) noexcept
{
    const double intMaximum = (double) std::numeric_limits<int>::max();
    if (! std::isfinite (value) || value <= 0.0 || value > intMaximum)
        return fallback;

    const double ceiled = std::ceil (value);
    if (! std::isfinite (ceiled) || ceiled < 1.0 || ceiled > intMaximum)
        return fallback;

    const auto wide = (std::int64_t) ceiled;
    if (wide < 1 || wide > (std::int64_t) std::numeric_limits<int>::max())
        return fallback;

    return (int) wide;
}

int checkedRoundToPositiveInt (double value) noexcept
{
    const double intMaximum = (double) std::numeric_limits<int>::max();
    if (! std::isfinite (value) || value <= 0.0)
        return 1;
    if (value >= intMaximum)
        return std::numeric_limits<int>::max();

    const double rounded = std::round (value);
    if (! std::isfinite (rounded) || rounded < 1.0)
        return 1;
    if (rounded >= intMaximum)
        return std::numeric_limits<int>::max();

    const auto wide = (std::int64_t) rounded;
    if (wide < 1)
        return 1;
    if (wide > (std::int64_t) std::numeric_limits<int>::max())
        return std::numeric_limits<int>::max();

    return (int) wide;
}

bool isStableParameterId (const juce::String& id)
{
    return id == gf::parameters::freezeId
        || id == gf::parameters::pitchId
        || id == gf::parameters::crossfadeMsId
        || id == gf::parameters::holdMsId
        || id == gf::parameters::grainSizeMsId
        || id == gf::parameters::densityHzId
        || id == gf::parameters::positionId;
}

struct MaskedNonParameterId
{
    juce::ValueTree child;
    juce::var originalId;
};

std::vector<MaskedNonParameterId> maskStableIdCollisions (juce::ValueTree& state)
{
    std::vector<MaskedNonParameterId> masked;

    for (auto child : state)
    {
        const auto id = child.getProperty ("id").toString();
        if (! child.hasType ("PARAM") && isStableParameterId (id))
        {
            masked.push_back ({ child, child.getProperty ("id") });
            child.setProperty ("id", "__gf_non_parameter_" + juce::String (masked.size()), nullptr);
        }
    }

    return masked;
}

void restoreMaskedIds (std::vector<MaskedNonParameterId>& masked)
{
    for (auto& entry : masked)
        entry.child.setProperty ("id", entry.originalId, nullptr);
}
} // namespace

GranularFreezeAudioProcessor::GranularFreezeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                       ),
       apvts (*this, nullptr, gf::parameters::rootId, gf::parameters::createLayout())
#endif
{
    freezeParameter = apvts.getRawParameterValue (gf::parameters::freezeId);
    pitchParameter = apvts.getRawParameterValue (gf::parameters::pitchId);
    crossfadeMsParameter = apvts.getRawParameterValue (gf::parameters::crossfadeMsId);
    holdMsParameter = apvts.getRawParameterValue (gf::parameters::holdMsId);
    grainSizeMsParameter = apvts.getRawParameterValue (gf::parameters::grainSizeMsId);
    densityHzParameter = apvts.getRawParameterValue (gf::parameters::densityHzId);
    positionParameter = apvts.getRawParameterValue (gf::parameters::positionId);
}

GranularFreezeAudioProcessor::~GranularFreezeAudioProcessor() {}

void GranularFreezeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sanitisePreparationSampleRate (sampleRate);
    const int positiveBlockSize = samplesPerBlock > 0 ? samplesPerBlock : 1;
    preparedBlockSize = juce::jmin (
        positiveBlockSize, maximumPreparedChunkSamples);

    const double requestedCaptureSamples = captureSeconds * currentSampleRate;
    maxBufferSize = checkedCeilToPositiveInt (
        requestedCaptureSamples, defaultCaptureSamples);
    circularBuffer.setSize (2, maxBufferSize, false, false, true);
    wetScratch.setSize (2, preparedBlockSize, false, false, true);
    circularBuffer.clear();
    wetScratch.clear();

    grainEngine.prepare (currentSampleRate);
    grainEngine.reset();
    frozenView = {};

    writePosition = 0;
    validSamples = 0;
    freezeTarget = false;
    wetMix = 0.0f;
    transitionStartMix = 0.0f;
    transitionTargetMix = 0.0f;
    transitionLength = 0;
    transitionPosition = 0;
    transitionActive = false;
}

void GranularFreezeAudioProcessor::releaseResources() {}

bool GranularFreezeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Support only stereo for now.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // The input set was never checked, so a host could negotiate a mismatched
    // layout (e.g. mono in / stereo out) that the processing loop does not
    // handle. This is an in-place effect: input and output must match.
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return true;
}

void GranularFreezeAudioProcessor::snapshotFrozenView (float holdMs) noexcept
{
    if (validSamples <= 0)
    {
        frozenView = {};
        return;
    }

    const int requestedHoldSamples = checkedRoundToPositiveInt (
        (double) holdMs * 0.001 * currentSampleRate);
    const int heldSamples = juce::jlimit (
        1, validSamples, requestedHoldSamples);
    const int heldStart = (writePosition - heldSamples + maxBufferSize)
                        % maxBufferSize;
    frozenView = {
        &circularBuffer,
        maxBufferSize,
        heldSamples,
        heldStart
    };
}

void GranularFreezeAudioProcessor::beginTransition (float targetMix,
                                                     float crossfadeMs) noexcept
{
    transitionStartMix = wetMix;
    transitionTargetMix = juce::jlimit (0.0f, 1.0f, targetMix);
    transitionPosition = 0;

    const double requestedFullScaleSamples = (double) crossfadeMs
                                           * 0.001 * currentSampleRate;
    const int fullScaleSamples = checkedRoundToPositiveInt (
        requestedFullScaleSamples);
    const float distance = std::abs (transitionTargetMix - transitionStartMix);
    const double requestedTransitionSamples = (double) fullScaleSamples
                                            * (double) distance;
    transitionLength = checkedRoundToPositiveInt (
        requestedTransitionSamples);
    transitionActive = true;
}

bool GranularFreezeAudioProcessor::advanceTransition() noexcept
{
    if (! transitionActive)
        return false;

    ++transitionPosition;
    if (transitionLength <= 1)
    {
        wetMix = transitionTargetMix;
        transitionActive = false;
        return transitionTargetMix == 0.0f;
    }

    const float progress = juce::jmin (
        1.0f,
        (float) transitionPosition / (float) (transitionLength - 1));
    const float curve = 0.5f - 0.5f * std::cos (
        juce::MathConstants<float>::pi * progress);
    wetMix = transitionStartMix
           + (transitionTargetMix - transitionStartMix) * curve;

    if (transitionPosition >= transitionLength - 1)
    {
        wetMix = transitionTargetMix;
        transitionActive = false;
        return transitionTargetMix == 0.0f;
    }

    return false;
}

bool GranularFreezeAudioProcessor::isFullyLive() const noexcept
{
    return ! freezeTarget && ! transitionActive && wetMix == 0.0f;
}

void GranularFreezeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rawFreeze = freezeParameter != nullptr
        ? freezeParameter->load()
        : 0.0f;
    const float rawPitch = pitchParameter != nullptr
        ? pitchParameter->load()
        : gf::parameters::pitchDefault;
    const float rawCrossfadeMs = crossfadeMsParameter != nullptr
        ? crossfadeMsParameter->load()
        : gf::parameters::crossfadeMsDefault;
    const float rawHoldMs = holdMsParameter != nullptr
        ? holdMsParameter->load()
        : gf::parameters::holdMsDefault;
    const float rawGrainSizeMs = grainSizeMsParameter != nullptr
        ? grainSizeMsParameter->load()
        : gf::parameters::grainSizeMsDefault;
    const float rawDensityHz = densityHzParameter != nullptr
        ? densityHzParameter->load()
        : gf::parameters::densityHzDefault;
    const float rawPosition = positionParameter != nullptr
        ? positionParameter->load()
        : gf::parameters::positionDefault;

    const auto finiteOr = [] (float value, float fallback) noexcept
    {
        return std::isfinite (value) ? value : fallback;
    };

    const bool requestedFreeze = std::isfinite (rawFreeze) && rawFreeze > 0.5f;
    const float pitch = juce::jlimit (
        0.5f, 2.0f, finiteOr (rawPitch, gf::parameters::pitchDefault));
    const float crossfadeMs = juce::jlimit (
        1.0f, 500.0f,
        finiteOr (rawCrossfadeMs, gf::parameters::crossfadeMsDefault));
    const float holdMs = juce::jlimit (
        50.0f, 10000.0f,
        finiteOr (rawHoldMs, gf::parameters::holdMsDefault));
    const float grainSizeMs = juce::jlimit (
        5.0f, 200.0f,
        finiteOr (rawGrainSizeMs, gf::parameters::grainSizeMsDefault));
    const float densityHz = juce::jlimit (
        0.0f, 200.0f,
        finiteOr (rawDensityHz, gf::parameters::densityHzDefault));
    const float position = juce::jlimit (
        0.0f, 1.0f,
        finiteOr (rawPosition, gf::parameters::positionDefault));
    const gf::GrainParameters grainParameters {
        grainSizeMs, densityHz, position, pitch
    };

    if (requestedFreeze != freezeTarget)
    {
        if (requestedFreeze && isFullyLive())
        {
            snapshotFrozenView (holdMs);
            grainEngine.reset();
        }

        beginTransition (requestedFreeze ? 1.0f : 0.0f, crossfadeMs);
        freezeTarget = requestedFreeze;
    }

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int offset = 0; offset < numSamples;)
    {
        const int chunkSamples = juce::jmin (
            preparedBlockSize, numSamples - offset);
        wetScratch.clear (0, chunkSamples);
        if (freezeTarget || transitionActive || wetMix > 0.0f)
            grainEngine.render (frozenView, wetScratch, 0, chunkSamples,
                                grainParameters);

        for (int sample = 0; sample < chunkSamples; ++sample)
        {
            const int hostSample = offset + sample;
            const float inputLeft = numChannels > 0
                ? buffer.getSample (0, hostSample)
                : 0.0f;
            const float inputRight = numChannels > 1
                ? buffer.getSample (1, hostSample)
                : inputLeft;
            const float currentMix = wetMix;
            const float dryMix = 1.0f - currentMix;

            if (numChannels > 0)
                buffer.setSample (
                    0, hostSample,
                    inputLeft * dryMix + wetScratch.getSample (0, sample) * currentMix);
            if (numChannels > 1)
                buffer.setSample (
                    1, hostSample,
                    inputRight * dryMix + wetScratch.getSample (1, sample) * currentMix);

            const bool reachedLive = transitionActive && advanceTransition();
            if (reachedLive)
            {
                grainEngine.reset();
                frozenView = {};
            }

            if (isFullyLive())
            {
                circularBuffer.setSample (0, writePosition, inputLeft);
                circularBuffer.setSample (1, writePosition, inputRight);
                writePosition = (writePosition + 1) % maxBufferSize;
                if (validSamples < maxBufferSize)
                    ++validSamples;
            }
        }

        // chunkSamples is positive and no larger than numSamples - offset, so
        // this addition cannot exceed numSamples or overflow int.
        offset += chunkSamples;
    }

    for (int channel = 2; channel < numChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

juce::AudioProcessorEditor* GranularFreezeAudioProcessor::createEditor() { return new GranularFreezeAudioProcessorEditor (*this); }

void GranularFreezeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Let APVTS handle state serialization
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GranularFreezeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = std::unique_ptr<juce::XmlElement> (getXmlFromBinary (data, sizeInBytes)))
    {
        auto restored = juce::ValueTree::fromXml (*xmlState);
        if (restored.isValid()
            && restored.getType().toString() == gf::parameters::rootId)
        {
            gf::parameters::addMissingV02Defaults (restored);
            auto masked = maskStableIdCollisions (restored);
            apvts.replaceState (restored);
            restoreMaskedIds (masked);
        }
    }
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GranularFreezeAudioProcessor(); }
