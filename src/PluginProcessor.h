#pragma once

#include <JuceHeader.h>

// A compact prototype: circular buffer with a freeze toggle and coarse pitch control.
class GranularFreezeAudioProcessor  : public juce::AudioProcessor
{
public:
    GranularFreezeAudioProcessor();
    ~GranularFreezeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GranularFreeze"; }

    double getTailLengthSeconds() const override { return 0.0; }

    // Persistence
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    // Parameters (simple, using legacy addParameter API for a quick prototype)
    juce::AudioParameterBool* freezeParam = nullptr;    // when true, stop writing to buffer and play back
    juce::AudioParameterFloat* pitchParam = nullptr;    // coarse pitch: 0.5x - 2.0x

    // Circular buffer for live capture and freeze playback
    juce::AudioBuffer<float> circularBuffer;
    int writePosition = 0;    // where incoming audio is written
    double readPosition = 0.0; // fractional read head for playback when frozen
    double currentSampleRate = 44100.0;
    int maxBufferSize = 0; // samples (depends on configured seconds)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessor)
};
