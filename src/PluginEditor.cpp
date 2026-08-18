#include "PluginEditor.h"

GranularFreezeAudioProcessorEditor::GranularFreezeAudioProcessorEditor (GranularFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Freeze button
    addAndMakeVisible (freezeButton);
    freezeButton.setClickingTogglesState (true);
    if (audioProcessor.getFreezeParameter())
    {
        freezeButton.onClick = [this]
        {
            auto* param = audioProcessor.getFreezeParameter();
            if (param) param->setValueNotifyingHost (freezeButton.getToggleState() ? 1.0f : 0.0f);
        };
        // Initialize UI state from parameter
        freezeButton.setToggleState (audioProcessor.getFreezeParameter()->get() > 0.5f, juce::dontSendNotification);
    }

    // Pitch slider
    addAndMakeVisible (pitchSlider);
    pitchSlider.setRange (0.5, 2.0, 0.01);
    pitchSlider.setSkewFactorFromMidPoint (1.0); // better resolution around 1.0
    pitchSlider.setTextValueSuffix ("x");
    addAndMakeVisible (pitchLabel);
    pitchLabel.attachToComponent (&pitchSlider, true);

    if (audioProcessor.getPitchParameter())
    {
        pitchSlider.onValueChange = [this]
        {
            auto* p = audioProcessor.getPitchParameter();
            if (p) p->setValueNotifyingHost ((float) pitchSlider.getValue());
        };
        pitchSlider.setValue (audioProcessor.getPitchParameter()->get(), juce::dontSendNotification);
    }

    setSize (420, 160);
}

GranularFreezeAudioProcessorEditor::~GranularFreezeAudioProcessorEditor() {}

void GranularFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (20, 20, 20));
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Granular Freeze (prototype)", getLocalBounds().withTrimmedTop (4), juce::Justification::centredTop, 1);
}

void GranularFreezeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    auto top = area.removeFromTop (36);
    freezeButton.setBounds (top.removeFromLeft (120).reduced (6));
    pitchSlider.setBounds (area.removeFromTop (40).removeFromLeft (260).reduced (6));
}
