#include "PluginProcessor.h"
#include "PluginEditor.h"

GranularFreezeAudioProcessor::GranularFreezeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                       )
#endif
{
    // Create parameters
    addParameter (freezeParam = new juce::AudioParameterBool ("freeze", "Freeze", false));
    addParameter (pitchParam = new juce::AudioParameterFloat ("pitch", "Pitch", 0.5f, 2.0f, 1.0f));
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

    const bool frozen = freezeParam ? freezeParam->get() : false;
    const float pitch = pitchParam ? pitchParam->get() : 1.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* writeData = circularBuffer.getWritePointer (ch);
        auto* channelData = buffer.getReadPointer (ch);
        auto* outData = buffer.getWritePointer (ch);

        if (! frozen)
        {
            // Write incoming audio to circular buffer and pass-through
            for (int i = 0; i < numSamples; ++i)
            {
                writeData[writePosition] = channelData[i];
                outData[i] = channelData[i];
                writePosition = (writePosition + 1) % maxBufferSize;
            }
            // Keep readPosition following writePosition so playback starts near live
            readPosition = static_cast<double> (writePosition);
        }
        else
        {
            // When frozen, read from circular buffer using readPosition and pitch
            const double increment = pitch; // 1.0 = normal speed, 2.0 = double, 0.5 = half
            for (int i = 0; i < numSamples; ++i)
            {
                // readPosition is fractional; use interpolation
                float sample = linearInterpolate (writeData, maxBufferSize, readPosition);
                outData[i] = sample;
                readPosition += increment;
                // wrap
                if (readPosition >= maxBufferSize) readPosition -= maxBufferSize;
                if (readPosition < 0.0) readPosition += maxBufferSize;
            }
            // Do not advance writePosition when frozen
        }
    }

    // Clear any remaining channels
    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* GranularFreezeAudioProcessor::createEditor() { return new juce::GenericAudioProcessorEditor (*this); }

void GranularFreezeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Simple state serialization: two floats (freeze as 0/1, pitch)
    juce::MemoryOutputStream mos (destData, true);
    mos.writeBool (freezeParam ? freezeParam->get() : false);
    mos.writeFloat (pitchParam ? pitchParam->get() : 1.0f);
}

void GranularFreezeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream mis (data, static_cast<size_t> (sizeInBytes), false);
    if (freezeParam) freezeParam->setValueNotifyingHost (mis.readBool() ? 1.0f : 0.0f);
    if (pitchParam) pitchParam->setValueNotifyingHost (mis.readFloat());
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GranularFreezeAudioProcessor(); }
