#include "GrainEngine.h"

#include <cmath>

namespace
{
int wrapIndex (int index, int span) noexcept
{
    index %= span;
    return index < 0 ? index + span : index;
}
}

namespace gf
{
bool FrozenBufferView::isReadable() const noexcept
{
    return buffer != nullptr
        && buffer->getNumChannels() > 0
        && capacity > 0
        && span > 0
        && span <= capacity
        && capacity <= buffer->getNumSamples()
        && oldestPhysicalIndex >= 0
        && oldestPhysicalIndex < capacity;
}

float FrozenBufferView::readSample (int channel, double logicalPosition) const noexcept
{
    if (! isReadable()
        || channel < 0
        || channel >= buffer->getNumChannels()
        || ! std::isfinite (logicalPosition))
        return 0.0f;

    double wrappedPosition = std::fmod (logicalPosition, (double) span);
    if (wrappedPosition < 0.0)
        wrappedPosition += (double) span;

    const int base = (int) std::floor (wrappedPosition);
    const float fraction = (float) (wrappedPosition - (double) base);

    auto sampleAt = [this, channel] (int logicalIndex) noexcept
    {
        const int wrappedLogical = wrapIndex (logicalIndex, span);
        const int physical = (oldestPhysicalIndex + wrappedLogical) % capacity;
        return buffer->getSample (channel, physical);
    };

    const float p0 = sampleAt (base - 1);
    const float p1 = sampleAt (base);
    const float p2 = sampleAt (base + 1);
    const float p3 = sampleAt (base + 2);
    const float t2 = fraction * fraction;
    const float t3 = t2 * fraction;

    return 0.5f * ((2.0f * p1)
        + (-p0 + p2) * fraction
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}
}
