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

    // Parameters are managed via AudioProcessorValueTreeState for automation and persistence
    juce::AudioProcessorValueTreeState apvts;

private:
    // Note: parameters are exposed through `apvts`; legacy raw pointers removed.

    // Circular buffer for live capture and freeze playback
    juce::AudioBuffer<float> circularBuffer;
    int writePosition = 0;    // where incoming audio is written
    double readPosition = 0.0; // fractional read head for playback when frozen
    double currentSampleRate = 44100.0;
    int maxBufferSize = 0; // samples (depends on configured seconds)

    // Crossfade smoothing when toggling freeze (in samples)
    int crossfadeSamples = 0;
    int crossfadePos = 0; // counts down
    enum CrossfadeDir { None = 0, ToFrozen = 1, ToLive = 2 };
    CrossfadeDir crossfadeDir = None;
    bool freezeWriting = true; // when false, incoming audio is not written to circular buffer
    bool prevFreezeState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessor)
};
