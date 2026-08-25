#include "SpectralAnalyzer.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float lowestBandHz = 35.0f;
constexpr float highestBandRatio = 0.42f;
}

namespace gf
{
void SpectralAnalyzer::prepare(const double sampleRate) noexcept
{
    const double safeRate = std::isfinite(sampleRate) && sampleRate > 0.0
        ? sampleRate : 44100.0;
    const float highestBandHz = static_cast<float>(std::max(
        static_cast<double>(lowestBandHz * 2.0f), safeRate * highestBandRatio));
    const float logarithmicSpan = std::log(highestBandHz / lowestBandHz);

    for (std::size_t band = 0; band < bandCount; ++band)
    {
        const float position = bandCount > 1
            ? static_cast<float>(band) / static_cast<float>(bandCount - 1)
            : 0.0f;
        const float frequency = lowestBandHz
            * std::exp(logarithmicSpan * position);
        const float omega = 2.0f * pi * frequency
            / static_cast<float>(safeRate);
        coefficients[band] = 2.0f * std::cos(omega);
    }
    reset();
}

void SpectralAnalyzer::reset() noexcept
{
    stateOne.fill(0.0f);
    stateTwo.fill(0.0f);
    levels.fill(0.0f);
    samplesInWindow = 0;
}

void SpectralAnalyzer::push(const float rawSample) noexcept
{
    const float sample = std::isfinite(rawSample)
        ? std::clamp(rawSample, -4.0f, 4.0f) : 0.0f;
    for (std::size_t band = 0; band < bandCount; ++band)
    {
        const float next = sample + coefficients[band] * stateOne[band]
            - stateTwo[band];
        stateTwo[band] = stateOne[band];
        stateOne[band] = next;
    }

    if (++samplesInWindow < analysisWindow)
        return;

    const float normaliser = 2.5f / static_cast<float>(analysisWindow);
    for (std::size_t band = 0; band < bandCount; ++band)
    {
        const float power = stateOne[band] * stateOne[band]
            + stateTwo[band] * stateTwo[band]
            - coefficients[band] * stateOne[band] * stateTwo[band];
        const float amplitude = std::sqrt(std::max(0.0f, power)) * normaliser;
        levels[band] = std::clamp(amplitude * 3.0f, 0.0f, 1.0f);
    }

    stateOne.fill(0.0f);
    stateTwo.fill(0.0f);
    samplesInWindow = 0;
}
}
