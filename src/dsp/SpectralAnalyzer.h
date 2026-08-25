#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gf
{
class SpectralAnalyzer
{
public:
    static constexpr std::size_t bandCount = 48;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void push(float sample) noexcept;

    const std::array<float, bandCount>& getLevels() const noexcept
    {
        return levels;
    }

private:
    static constexpr std::uint32_t analysisWindow = 128;

    std::array<float, bandCount> coefficients {};
    std::array<float, bandCount> stateOne {};
    std::array<float, bandCount> stateTwo {};
    std::array<float, bandCount> levels {};
    std::uint32_t samplesInWindow = 0;
};
}
