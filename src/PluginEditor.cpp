#include "PluginEditor.h"
#include "PluginParameters.h"

GranularFreezeAudioProcessorEditor::GranularFreezeAudioProcessorEditor (GranularFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible (freezeButton);
    freezeButton.setClickingTogglesState (true);
    freezeButton.setComponentID ("freezeControl");

    addAndMakeVisible (pitchSlider);
    pitchSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    pitchSlider.setRange (0.5, 2.0, 0.01);
    pitchSlider.setSkewFactorFromMidPoint (1.0);
    pitchSlider.setTextValueSuffix ("x");
    pitchSlider.setComponentID ("pitchControl");
    addAndMakeVisible (pitchLabel);
    pitchLabel.attachToComponent (&pitchSlider, true);

    addAndMakeVisible (positionSlider);
    positionSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    positionSlider.setRange (0.0, 1.0, 0.01);
    positionSlider.setNumDecimalPlacesToDisplay (2);
    positionSlider.setComponentID ("positionControl");
    addAndMakeVisible (positionLabel);
    positionLabel.attachToComponent (&positionSlider, true);

    addAndMakeVisible (grainSizeSlider);
    grainSizeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    grainSizeSlider.setRange (5.0, 200.0, 1.0);
    grainSizeSlider.setTextValueSuffix (" ms");
    grainSizeSlider.setComponentID ("sizeControl");
    addAndMakeVisible (grainSizeLabel);
    grainSizeLabel.attachToComponent (&grainSizeSlider, true);

    addAndMakeVisible (densitySlider);
    densitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    densitySlider.setRange (0.0, 200.0, 1.0);
    densitySlider.setTextValueSuffix (" gr/s");
    densitySlider.setComponentID ("densityControl");
    addAndMakeVisible (densityLabel);
    densityLabel.attachToComponent (&densitySlider, true);

    addAndMakeVisible (holdSlider);
    holdSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    holdSlider.setRange (50.0, 10000.0, 1.0);
    holdSlider.setSkewFactor (0.4);
    holdSlider.setTextValueSuffix (" ms");
    holdSlider.setComponentID ("holdControl");
    addAndMakeVisible (holdLabel);
    holdLabel.attachToComponent (&holdSlider, true);

    addAndMakeVisible (crossfadeSlider);
    crossfadeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    crossfadeSlider.setRange (1.0, 500.0, 1.0);
    crossfadeSlider.setTextValueSuffix (" ms");
    crossfadeSlider.setComponentID ("crossfadeControl");
    addAndMakeVisible (crossfadeLabel);
    crossfadeLabel.attachToComponent (&crossfadeSlider, true);

    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, gf::parameters::freezeId, freezeButton);
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, gf::parameters::pitchId, pitchSlider);
    positionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, gf::parameters::positionId, positionSlider);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, gf::parameters::grainSizeMsId, grainSizeSlider);
    densityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, gf::parameters::densityHzId, densitySlider);
    holdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, gf::parameters::holdMsId, holdSlider);
    crossfadeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, gf::parameters::crossfadeMsId, crossfadeSlider);

    setSize (480, 342);
}

GranularFreezeAudioProcessorEditor::~GranularFreezeAudioProcessorEditor() = default;

void GranularFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (20, 20, 20));
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Granular Freeze", getLocalBounds().reduced (12).withHeight (20),
                      juce::Justification::centred, 1);
}

void GranularFreezeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (20);

    auto freezeRow = area.removeFromTop (38);
    freezeButton.setBounds (freezeRow.removeFromLeft (140).reduced (3));

    constexpr int labelIndent = 92;
    const auto layoutSliderRow = [&area] (juce::Slider& slider)
    {
        auto row = area.removeFromTop (42).reduced (0, 3);
        row.removeFromLeft (labelIndent);
        slider.setBounds (row);
    };

    layoutSliderRow (pitchSlider);
    layoutSliderRow (positionSlider);
    layoutSliderRow (grainSizeSlider);
    layoutSliderRow (densitySlider);
    layoutSliderRow (holdSlider);
    layoutSliderRow (crossfadeSlider);
}
