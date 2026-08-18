#include "PluginEditor.h"

GranularFreezeAudioProcessorEditor::GranularFreezeAudioProcessorEditor (GranularFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

GranularFreezeAudioProcessorEditor::~GranularFreezeAudioProcessorEditor() {}

void GranularFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Granular Freeze (prototype UI)", getLocalBounds(), juce::Justification::centredTop, 1);
}

void GranularFreezeAudioProcessorEditor::resized() {}
