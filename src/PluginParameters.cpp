#include "PluginParameters.h"

namespace gf::parameters
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { freezeId, 1 }, "Freeze", freezeDefault));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { pitchId, 1 }, "Pitch",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.01f), pitchDefault));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { crossfadeMsId, 1 }, "Crossfade (ms)",
        juce::NormalisableRange<float> (1.0f, 500.0f, 1.0f), crossfadeMsDefault));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { holdMsId, 2 }, "Hold (ms)",
        juce::NormalisableRange<float> (50.0f, 10000.0f, 1.0f, 0.4f), holdMsDefault));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { grainSizeMsId, 3 }, "Size",
        juce::NormalisableRange<float> (5.0f, 200.0f, 1.0f), grainSizeMsDefault));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { densityHzId, 3 }, "Density",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), densityHzDefault));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { positionId, 3 }, "Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), positionDefault));

    return layout;
}

namespace
{
bool hasParameterChild (const juce::ValueTree& state, const char* parameterId)
{
    for (int index = 0; index < state.getNumChildren(); ++index)
    {
        const auto child = state.getChild (index);
        if (child.hasType ("PARAM") && child.getProperty ("id").toString() == parameterId)
            return true;
    }

    return false;
}

void appendParameterDefault (juce::ValueTree& state, const char* parameterId, float defaultValue)
{
    if (hasParameterChild (state, parameterId))
        return;

    juce::ValueTree parameter ("PARAM");
    parameter.setProperty ("id", parameterId, nullptr);
    parameter.setProperty ("value", defaultValue, nullptr);
    state.addChild (parameter, -1, nullptr);
}
} // namespace

void addMissingV02Defaults (juce::ValueTree& state)
{
    appendParameterDefault (state, holdMsId, holdMsDefault);
    appendParameterDefault (state, grainSizeMsId, grainSizeMsDefault);
    appendParameterDefault (state, densityHzId, densityHzDefault);
    appendParameterDefault (state, positionId, positionDefault);
}
} // namespace gf::parameters
