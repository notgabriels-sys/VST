#pragma once

#include <cstdint>

namespace gf
{
struct PlanarBufferView
{
    float* const* channels = nullptr;
    std::uint32_t channelCount = 0;
    std::uint32_t sampleCount = 0;

    bool isWritable() const noexcept
    {
        if (channels == nullptr || channelCount == 0 || sampleCount == 0)
            return false;
        for (std::uint32_t channel = 0; channel < channelCount; ++channel)
            if (channels[channel] == nullptr)
                return false;
        return true;
    }
};

struct FrozenBufferView
{
    const float* const* channels = nullptr;
    int channelCount = 0;
    int capacity = 0;
    int span = 0;
    int oldestPhysicalIndex = 0;

    bool isReadable() const noexcept;
    float readSample(int channel, double logicalPosition) const noexcept;
};
}
