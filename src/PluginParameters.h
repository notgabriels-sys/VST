#pragma once

#include <JuceHeader.h>

namespace gf::parameters
{
inline constexpr auto rootId = "PARAMS";
inline constexpr auto freezeId = "freeze";
inline constexpr auto pitchId = "pitch";
inline constexpr auto crossfadeMsId = "crossfadeMs";
inline constexpr auto grainSizeMsId = "grainSizeMs";
inline constexpr auto densityHzId = "densityHz";
inline constexpr auto positionId = "position";

inline constexpr bool freezeDefault = false;
inline constexpr float pitchDefault = 1.0f;
inline constexpr float crossfadeMsDefault = 30.0f;
inline constexpr float grainSizeMsDefault = 80.0f;
inline constexpr float densityHzDefault = 20.0f;
inline constexpr float positionDefault = 1.0f;

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
void addMissingV02Defaults (juce::ValueTree& state);
}
