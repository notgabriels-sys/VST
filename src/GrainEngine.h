#pragma once

#include <JuceHeader.h>

namespace gf
{
struct FrozenBufferView
{
    const juce::AudioBuffer<float>* buffer = nullptr;
    int capacity = 0;
    int span = 0;
    int oldestPhysicalIndex = 0;

    bool isReadable() const noexcept;
    float readSample (int channel, double logicalPosition) const noexcept;
};
}
