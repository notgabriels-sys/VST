#include "PluginEditor.h"

GranularFreezeAudioProcessorEditor::GranularFreezeAudioProcessorEditor (GranularFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Freeze button
    addAndMakeVisible (freezeButton);
    freezeButton.setClickingTogglesState (true);

    // Pitch slider
    addAndMakeVisible (pitchSlider);
    pitchSlider.setRange (0.5, 2.0, 0.01);
    pitchSlider.setSkewFactorFromMidPoint (1.0); // better resolution around 1.0
    pitchSlider.setTextValueSuffix ("x");
    addAndMakeVisible (pitchLabel);
    pitchLabel.attachToComponent (&pitchSlider, true);

    // Crossfade slider
    addAndMakeVisible (crossfadeSlider);
    crossfadeSlider.setRange (1.0, 500.0, 1.0);
    crossfadeSlider.setTextValueSuffix (" ms");
    addAndMakeVisible (crossfadeLabel);
    crossfadeLabel.attachToComponent (&crossfadeSlider, true);

    // Attach UI to parameters via APVTS
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "freeze", freezeButton);
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "pitch", pitchSlider);
    crossfadeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "crossfadeMs", crossfadeSlider);

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
    area.removeFromTop (20); // leave room for the title drawn in paint()

    auto top = area.removeFromTop (36);
    freezeButton.setBounds (top.removeFromLeft (120).reduced (6));

    // Both labels are attachToComponent(..., onLeft = true), so each slider has
    // to be indented or the label is drawn off the left edge.
    const int labelWidth = 80;

    auto pitchRow = area.removeFromTop (40).reduced (6);
    pitchRow.removeFromLeft (labelWidth);
    pitchSlider.setBounds (pitchRow);

    // crossfadeSlider was created, made visible and attached to the parameter,
    // but never given bounds -- so it rendered at zero size and the crossfade
    // time could not be adjusted from the UI at all.
    auto crossfadeRow = area.removeFromTop (40).reduced (6);
    crossfadeRow.removeFromLeft (labelWidth);
    crossfadeSlider.setBounds (crossfadeRow);
}
