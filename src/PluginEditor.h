#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GranularFreezeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    GranularFreezeAudioProcessorEditor (GranularFreezeAudioProcessor&);
    ~GranularFreezeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GranularFreezeAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessorEditor)
};
