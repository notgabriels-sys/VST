#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

namespace
{
bool isStableParameterId (const juce::String& id)
{
    return id == gf::parameters::freezeId
        || id == gf::parameters::pitchId
        || id == gf::parameters::crossfadeMsId
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
    grainSizeMsParameter = apvts.getRawParameterValue (gf::parameters::grainSizeMsId);
    densityHzParameter = apvts.getRawParameterValue (gf::parameters::densityHzId);
    positionParameter = apvts.getRawParameterValue (gf::parameters::positionId);
}

GranularFreezeAudioProcessor::~GranularFreezeAudioProcessor() {}

void GranularFreezeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    // Allocate a max buffer of 8 seconds for prototype
    const double bufferSeconds = 8.0;
    maxBufferSize = static_cast<int> (std::ceil (bufferSeconds * currentSampleRate));

    const int numOutputChannels = getTotalNumOutputChannels();
    circularBuffer.setSize (numOutputChannels, maxBufferSize);
    circularBuffer.clear();

    writePosition = 0;
    readPosition = 0.0;
    validSamples = 0;

    // Seed the crossfade length from the actual parameter rather than a
    // hardcoded 30 ms, so a session restored with a different crossfade time is
    // correct on the very first block instead of only after processBlock
    // recomputes it.
    const double crossfadeMs = crossfadeMsParameter != nullptr
        ? (double) crossfadeMsParameter->load()
        : (double) gf::parameters::crossfadeMsDefault;
    crossfadeSamples = static_cast<int> (std::max (1.0, std::round (crossfadeMs * 0.001 * currentSampleRate)));
    crossfadePos = 0;
    crossfadeDir = None;
    freezeWriting = true;
    prevFreezeState = false;
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

// Cubic interpolation (Catmull-Rom) for smoother playback
static float cubicInterpolate (const float* data, int size, double index)
{
    if (size <= 0) return 0.0f;
    int i1 = static_cast<int> (std::floor (index));
    double t = index - (double) i1;

    // indices for -1, 0, +1, +2 relative to i1
    int im1 = i1 - 1;
    int i0 = i1;
    int i2 = i1 + 1;
    int i3 = i1 + 2;

    // wrap
    auto wrap = [size](int i){ return (i % size + size) % size; };
    float y0 = data[wrap(im1)];
    float y1 = data[wrap(i0)];
    float y2 = data[wrap(i2)];
    float y3 = data[wrap(i3)];

    // Catmull-Rom cubic interpolation
    float a = (-0.5f * y0) + (1.5f * y1) - (1.5f * y2) + (0.5f * y3);
    float b = y0 - (2.5f * y1) + (2.0f * y2) - (0.5f * y3);
    float c = (-0.5f * y0) + (0.5f * y2);
    float d = y1;

    return ((a * (float)(t * t * t)) + (b * (float)(t * t)) + (c * (float)t) + d);
}


// Reads the frozen buffer with a crossfaded loop point.
//
// Without this the read head jumps from the end of the captured region straight
// back to the start. The waveform either side of that seam is unrelated, so the
// discontinuity is an audible click on every single wrap -- once per loop, which
// for a short capture is a continuous buzz.
//
// The loop is shortened by loopFade samples, and the first loopFade samples are
// mixed with the tail that was cut off. At pos == 0 the output is entirely the
// tail, at pos == loopFade entirely the head, so the seam is continuous.
static float readFrozen (const float* data, int span, int loopFade, int loopLen, double pos)
{
    const float head = cubicInterpolate (data, span, pos);

    if (loopFade <= 0 || pos >= (double) loopFade)
        return head;

    const double t = pos / (double) loopFade;              // 0 -> 1
    const float tail = cubicInterpolate (data, span, pos + loopLen);
    return (float) ((1.0 - t) * tail + t * head);
}

void GranularFreezeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (circularBuffer.getNumChannels(), buffer.getNumChannels());

    // Read parameter values from APVTS
    const bool frozen = freezeParameter != nullptr ? (freezeParameter->load() > 0.5f) : false;
    const float pitch = pitchParameter != nullptr ? pitchParameter->load() : gf::parameters::pitchDefault;
    const float crossfadeMs = crossfadeMsParameter != nullptr
        ? crossfadeMsParameter->load()
        : gf::parameters::crossfadeMsDefault;

    // Compute crossfade length in samples based on parameter and sample rate
    const int requestedCrossfadeSamples = static_cast<int> (std::max (1.0, std::round (crossfadeMs * 0.001 * currentSampleRate)));
    // update crossfadeSamples if changed
    if (requestedCrossfadeSamples != crossfadeSamples)
        crossfadeSamples = requestedCrossfadeSamples;

    // Detect freeze change and start crossfade
    if (frozen != prevFreezeState)
    {
        if (frozen)
        {
            // Start crossfade TO frozen: set read head to current write position
            readPosition = static_cast<double> (writePosition);
            crossfadeDir = ToFrozen;
            crossfadePos = crossfadeSamples;
            // During the crossfade we will stop writing to freeze the buffer content; keep freezeWriting false
            freezeWriting = false;
        }
        else
        {
            // Start crossfade TO live: allow writing during crossfade so live audio replaces buffer gradually
            crossfadeDir = ToLive;
            crossfadePos = crossfadeSamples;
            freezeWriting = true;
        }
        prevFreezeState = frozen;
    }

    // These are per-instance members, but the loop below advances them once per
    // CHANNEL. Without resetting, channel 1 would resume from wherever channel 0
    // finished: writePosition would drift a block further each time (destroying
    // the stereo image), crossfadePos would count down twice as fast, and once
    // crossfadeDir was cleared the later channels would skip the fade entirely
    // and hard-switch. Snapshot here, restore per channel, and let the final
    // channel leave the committed values -- every channel advances identically,
    // so the post-loop state is correct.
    const int       entryWritePosition  = writePosition;
    const double    entryReadPosition   = readPosition;
    const int       entryCrossfadePos   = crossfadePos;
    const CrossfadeDir entryCrossfadeDir = crossfadeDir;
    const bool      entryFreezeWriting  = freezeWriting;
    const int       entryValidSamples   = validSamples;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* circularData = circularBuffer.getWritePointer (ch);
        auto* inData = buffer.getReadPointer (ch);
        auto* outData = buffer.getWritePointer (ch);

        writePosition = entryWritePosition;
        readPosition  = entryReadPosition;
        crossfadePos  = entryCrossfadePos;
        crossfadeDir  = entryCrossfadeDir;
        freezeWriting = entryFreezeWriting;
        validSamples  = entryValidSamples;

        if (crossfadeDir == None)
        {
            if (! frozen)
            {
                // Normal live pass-through and writing
                for (int i = 0; i < numSamples; ++i)
                {
                    float inS = inData[i];
                    outData[i] = inS;
                    circularData[writePosition] = inS;
                    writePosition = (writePosition + 1) % maxBufferSize;
                    if (validSamples < maxBufferSize) ++validSamples;
                }
                // keep read head near write position
                readPosition = static_cast<double> (writePosition);
            }
            else
            {
                // Fully frozen playback
                const double increment = pitch;
                const int span     = juce::jmax (1, validSamples);
                const int loopFade = juce::jlimit (0, span / 4, crossfadeSamples);
                const int loopLen  = juce::jmax (1, span - loopFade);

                readPosition = std::fmod (readPosition, (double) loopLen);
                if (readPosition < 0.0) readPosition += loopLen;

                for (int i = 0; i < numSamples; ++i)
                {
                    outData[i] = readFrozen (circularData, span, loopFade, loopLen, readPosition);
                    readPosition += increment;
                    if (readPosition >= loopLen) readPosition -= loopLen;
                    if (readPosition < 0.0) readPosition += loopLen;
                }
            }
        }
        else
        {
            // Crossfading between live and frozen
            const double increment = pitch;

            // Fixed for the whole block. When fading back to live, freezeWriting
            // is true, so validSamples grows every sample -- recomputing these
            // per sample would move the loop boundary underneath the read head
            // and tear the frozen source.
            const int span     = juce::jmax (1, validSamples);
            const int loopFade = juce::jlimit (0, span / 4, crossfadeSamples);
            const int loopLen  = juce::jmax (1, span - loopFade);

            readPosition = std::fmod (readPosition, (double) loopLen);
            if (readPosition < 0.0) readPosition += loopLen;

            for (int i = 0; i < numSamples; ++i)
            {
                float inS = inData[i];
                float frozenS = readFrozen (circularData, span, loopFade, loopLen, readPosition);

                // compute alpha (amount of frozen audio) using cosine smoothstep for less audible artifacts
                float alpha = 0.0f;
                if (crossfadePos > 0 && crossfadeSamples > 0)
                {
                    // progress from 0..1 (0 start, 1 end)
                    double prog = 1.0 - (double) crossfadePos / (double) crossfadeSamples; // 0->1 as crossfadePos counts down
                    // cosine curve: smooth in/out, map prog to (0..1)
                    double cosv = 0.5 * (1.0 - std::cos (juce::MathConstants<double>::pi * prog));
                    if (crossfadeDir == ToFrozen)
                        alpha = static_cast<float> (cosv); // 0 -> 1
                    else // ToLive
                        alpha = static_cast<float> (1.0 - cosv); // 1 -> 0
                }
                else
                {
                    alpha = (crossfadeDir == ToFrozen) ? 1.0f : 0.0f;
                }

                // mix
                outData[i] = inS * (1.0f - alpha) + frozenS * alpha;

                // read head advances for frozen playback portion, wrapping
                // within the crossfaded loop rather than the whole buffer
                readPosition += increment;
                if (readPosition >= loopLen) readPosition -= loopLen;
                if (readPosition < 0.0) readPosition += loopLen;

                // Write into circular buffer only if writing is enabled
                if (freezeWriting)
                {
                    circularData[writePosition] = inS;
                    writePosition = (writePosition + 1) % maxBufferSize;
                    if (validSamples < maxBufferSize) ++validSamples;
                }

                // advance crossfade position per sample
                if (crossfadePos > 0)
                    --crossfadePos;
            }

            // If crossfade finished, update state
            if (crossfadePos <= 0)
            {
                if (crossfadeDir == ToFrozen)
                {
                    // now fully frozen; ensure writing is disabled
                    freezeWriting = false;
                }
                else
                {
                    // fully live
                    freezeWriting = true;
                }
                crossfadeDir = None;
            }
        }
    }

    // Clear any remaining channels
    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
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
