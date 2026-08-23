#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
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

    juce::Slider positionSlider;
    juce::Label positionLabel {"positionLabel", "Position"};

    juce::Slider grainSizeSlider;
    juce::Label grainSizeLabel {"grainSizeLabel", "Size"};

    juce::Slider densitySlider;
    juce::Label densityLabel {"densityLabel", "Density"};

    juce::Slider crossfadeSlider;
    juce::Label crossfadeLabel {"crossfadeLabel", "Crossfade"};

    // Attachments are declared after their controls so they are destroyed first.
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> positionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> densityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossfadeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularFreezeAudioProcessorEditor)
};
