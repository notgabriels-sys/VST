#pragma once

#include <JuceHeader.h>

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessor)
};
