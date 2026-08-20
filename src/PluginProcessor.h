#pragma once

#include <JuceHeader.h>
#include "GrainEngine.h"
#include "PluginParameters.h"

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

    // Circular buffer for chronological live capture.
    juce::AudioBuffer<float> circularBuffer;
    juce::AudioBuffer<float> wetScratch;
    gf::GrainEngine grainEngine;
    gf::FrozenBufferView frozenView;

    int writePosition = 0;
    double currentSampleRate = 44100.0;
    int maxBufferSize = 1;
    int validSamples = 0;
    int preparedBlockSize = 1;

    bool freezeTarget = false;
    float wetMix = 0.0f;
    float transitionStartMix = 0.0f;
    float transitionTargetMix = 0.0f;
    int transitionLength = 0;
    int transitionPosition = 0;
    bool transitionActive = false;

    void snapshotFrozenView() noexcept;
    void beginTransition (float targetMix, float crossfadeMs) noexcept;
    bool advanceTransition() noexcept;
    bool isFullyLive() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessor)
};
