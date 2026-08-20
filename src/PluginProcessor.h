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

    // Required AudioProcessor overrides. This is an audio effect with no MIDI
    // and no program/preset support, so these are minimal implementations.
    // JUCE requires getNumPrograms() to return at least 1.
    bool acceptsMidi() const override                              { return false; }
    bool producesMidi() const override                             { return false; }
    int getNumPrograms() override                                  { return 1; }
    int getCurrentProgram() override                               { return 0; }
    void setCurrentProgram (int) override                          {}
    const juce::String getProgramName (int) override               { return {}; }
    void changeProgramName (int, const juce::String&) override     {}

    // Persistence
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameters are managed via AudioProcessorValueTreeState for automation and persistence
    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float>* freezeParameter = nullptr;
    std::atomic<float>* pitchParameter = nullptr;
    std::atomic<float>* crossfadeMsParameter = nullptr;
    std::atomic<float>* grainSizeMsParameter = nullptr;
    std::atomic<float>* densityHzParameter = nullptr;
    std::atomic<float>* positionParameter = nullptr;

    // Circular buffer for live capture and freeze playback
    juce::AudioBuffer<float> circularBuffer;
    int writePosition = 0;    // where incoming audio is written
    double readPosition = 0.0; // fractional read head for playback when frozen
    double currentSampleRate = 44100.0;
    int maxBufferSize = 0; // samples (depends on configured seconds)
    // How much of circularBuffer actually holds captured audio. Until the
    // buffer has filled once this is less than maxBufferSize, and frozen
    // playback must wrap within it -- otherwise the read head runs through the
    // still-zeroed remainder and freeze outputs silence.
    int validSamples = 0;

    // Crossfade smoothing when toggling freeze (in samples)
    int crossfadeSamples = 0;
    int crossfadePos = 0; // counts down
    enum CrossfadeDir { None = 0, ToFrozen = 1, ToLive = 2 };
    CrossfadeDir crossfadeDir = None;
    bool freezeWriting = true; // when false, incoming audio is not written to circular buffer
    bool prevFreezeState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessor)
};
