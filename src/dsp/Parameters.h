#pragma once

#include <array>
#include <cstddef>

namespace gf
{
enum ParameterIndex : std::size_t
{
    freezeIndex, pitchIndex, crossfadeMsIndex, holdMsIndex,
    grainSizeMsIndex, densityHzIndex, positionIndex, parameterCount
};

struct ParameterDescriptor
{
    const char* symbol;
    const char* name;
    const char* unit;
    float minimum;
    float maximum;
    float defaultValue;
    bool integer;
    bool boolean;
};

inline constexpr std::array<ParameterDescriptor, parameterCount> parameterDescriptors {{
    { "freeze", "Freeze", "", 0.0f, 1.0f, 0.0f, true, true },
    { "pitch", "Pitch", "x", 0.5f, 2.0f, 1.0f, false, false },
    { "crossfadeMs", "Crossfade", "ms", 1.0f, 500.0f, 30.0f, true, false },
    { "holdMs", "Hold", "ms", 50.0f, 10000.0f, 1000.0f, true, false },
    { "grainSizeMs", "Size", "ms", 5.0f, 200.0f, 80.0f, true, false },
    { "densityHz", "Density", "Hz", 0.0f, 200.0f, 20.0f, true, false },
    { "position", "Position", "", 0.0f, 1.0f, 1.0f, false, false }
}};

struct ParameterValues
{
    float freeze = 0.0f;
    float pitch = 1.0f;
    float crossfadeMs = 30.0f;
    float holdMs = 1000.0f;
    float grainSizeMs = 80.0f;
    float densityHz = 20.0f;
    float position = 1.0f;
};
}
