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

    // UI controls
    juce::ToggleButton freezeButton {"Freeze"};
    juce::Slider pitchSlider;
    juce::Label pitchLabel {"pitchLabel", "Pitch"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessorEditor)
};
