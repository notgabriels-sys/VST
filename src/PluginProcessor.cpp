#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter layout helper
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterBool> ("freeze", "Freeze", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("pitch", "Pitch", juce::NormalisableRange<float> (0.5f, 2.0f, 0.01f), 1.0f));
    return layout;
}

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
       apvts (*this, nullptr, "PARAMS", createParameterLayout())
#endif
{
    // Parameters are created via apvts; no legacy addParameter calls here.
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

    // Crossfade: default 30 ms
    const double crossfadeMs = 30.0;
    crossfadeSamples = static_cast<int> (std::max (1.0, std::round (crossfadeMs * 0.001 * currentSampleRate)));
    crossfadePos = 0;
    crossfadeDir = None;
    freezeWriting = true;
    prevFreezeState = false;
}

void GranularFreezeAudioProcessor::releaseResources() {}

bool GranularFreezeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Support only stereo for now
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

// Linear interpolation helper
static float linearInterpolate (const float* data, int size, double index)
{
    if (size <= 0) return 0.0f;
    int i0 = static_cast<int> (std::floor (index));
    int i1 = i0 + 1;
    double frac = index - (double) i0;
    // wrap indices
    i0 = (i0 % size + size) % size;
    i1 = (i1 % size + size) % size;
    return static_cast<float> (data[i0] * (1.0 - frac) + data[i1] * frac);
}

void GranularFreezeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (circularBuffer.getNumChannels(), buffer.getNumChannels());

    // Read parameter values from APVTS
    auto* freezeVal = apvts.getRawParameterValue ("freeze");
    auto* pitchVal = apvts.getRawParameterValue ("pitch");
    const bool frozen = freezeVal ? (*freezeVal > 0.5f) : false;
    const float pitch = pitchVal ? *pitchVal : 1.0f;

    // Detect freeze change and start crossfade
    if (frozen != prevFreezeState)
    {
        if (frozen)
        {
            // Start crossfade TO frozen: set read head to current write position
            readPosition = static_cast<double> (writePosition);
            crossfadeDir = ToFrozen;
            crossfadePos = crossfadeSamples;
            freezeWriting = false; // stop writing during crossfade to freeze the buffer content
        }
        else
        {
            // Start crossfade TO live: allow writing immediately for smooth transition
            crossfadeDir = ToLive;
            crossfadePos = crossfadeSamples;
            freezeWriting = true;
        }
        prevFreezeState = frozen;
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* circularData = circularBuffer.getWritePointer (ch);
        auto* inData = buffer.getReadPointer (ch);
        auto* outData = buffer.getWritePointer (ch);

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
                }
                // keep read head near write position
                readPosition = static_cast<double> (writePosition);
            }
            else
            {
                // Fully frozen playback
                const double increment = pitch;
                for (int i = 0; i < numSamples; ++i)
                {
                    float sample = linearInterpolate (circularData, maxBufferSize, readPosition);
                    outData[i] = sample;
                    readPosition += increment;
                    if (readPosition >= maxBufferSize) readPosition -= maxBufferSize;
                    if (readPosition < 0.0) readPosition += maxBufferSize;
                }
            }
        }
        else
        {
            // Crossfading between live and frozen
            const double increment = pitch;
            for (int i = 0; i < numSamples; ++i)
            {
                float inS = inData[i];
                float frozenS = linearInterpolate (circularData, maxBufferSize, readPosition);

                // compute alpha (amount of frozen audio)
                float alpha = 0.0f;
                if (crossfadePos > 0)
                {
                    double frac = (double) crossfadePos / (double) crossfadeSamples;
                    if (crossfadeDir == ToFrozen)
                        alpha = static_cast<float> (1.0 - frac); // 0 -> 1
                    else // ToLive
                        alpha = static_cast<float> (frac);     // 1 -> 0
                }
                else
                {
                    alpha = (crossfadeDir == ToFrozen) ? 1.0f : 0.0f;
                }

                // mix
                outData[i] = inS * (1.0f - alpha) + frozenS * alpha;

                // read head advances for frozen playback portion
                readPosition += increment;
                if (readPosition >= maxBufferSize) readPosition -= maxBufferSize;
                if (readPosition < 0.0) readPosition += maxBufferSize;

                // Write into circular buffer only if writing is enabled
                if (freezeWriting)
                {
                    circularData[writePosition] = inS;
                    writePosition = (writePosition + 1) % maxBufferSize;
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
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GranularFreezeAudioProcessor(); }
