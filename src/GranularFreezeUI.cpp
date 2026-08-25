#include "DistrhoUI.hpp"
#include "DistrhoPlugin.hpp"
#include "GranularFreezeTelemetry.h"
#include "dsp/Parameters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO
using DGL_NAMESPACE::Color;

namespace
{
struct ThemePalette
{
    Color backgroundTop;
    Color backgroundBottom;
    Color bodyTop;
    Color surfaceTop;
    Color surfaceBottom;
    Color surfaceLiftedTop;
    Color surfaceLiftedBottom;
    Color ink;
    Color quiet;
    Color whisper;
    Color accent;
    Color accentDim;
    Color hairline;
    Color track;
};

// The palettes are intentionally quiet: color changes the atmosphere, while
// the geometry and interaction remain the same. T cycles through these
// options, and the three small header swatches select one directly.
const ThemePalette themePalettes[] = {
    {
        Color(15, 16, 17), Color(15, 16, 17),
        Color(18, 19, 21),
        Color(22, 23, 25), Color(22, 23, 25),
        Color(27, 28, 30), Color(27, 28, 30),
        Color(233, 234, 236), Color(160, 164, 171), Color(106, 110, 116),
        Color(201, 207, 214), Color(88, 92, 98), Color(46, 47, 51),
        Color(31, 32, 35)
    },
    {
        Color(18, 17, 16), Color(18, 17, 16),
        Color(21, 20, 19),
        Color(25, 24, 23), Color(25, 24, 23),
        Color(29, 28, 27), Color(29, 28, 27),
        Color(237, 234, 230), Color(165, 160, 157), Color(111, 106, 104),
        Color(204, 196, 190), Color(109, 102, 99), Color(51, 49, 48),
        Color(34, 32, 31)
    },
    {
        Color(16, 16, 19), Color(16, 16, 19),
        Color(19, 19, 22),
        Color(24, 24, 28), Color(24, 24, 28),
        Color(29, 28, 33), Color(29, 28, 33),
        Color(234, 233, 239), Color(159, 158, 170), Color(107, 106, 117),
        Color(202, 199, 214), Color(107, 103, 122), Color(48, 47, 55),
        Color(32, 31, 37)
    }
};

constexpr std::size_t themeCount = sizeof(themePalettes) / sizeof(themePalettes[0]);
constexpr float themeSwatchY = 28.0f;
constexpr float themeSwatchGap = 17.0f;
constexpr float themeSwatchRadius = 3.5f;

constexpr float contentMargin = 32.0f;
constexpr float headerRuleY = 76.0f;
constexpr float topControlY = 85.0f;
constexpr float topControlWidth = 156.5f;
constexpr float topControlStep = 166.5f;
constexpr float topControlHeight = 46.0f;
constexpr float bottomControlY = 256.0f;
constexpr float bottomControlWidth = 212.0f;
constexpr float bottomControlStep = 222.0f;
constexpr float bottomControlHeight = 47.0f;
constexpr float screenY = 140.0f;
constexpr float screenWidth = 656.0f;
constexpr float screenHeight = 104.0f;
}

class GranularFreezeUI final : public UI
{
public:
    GranularFreezeUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = gf::parameterDescriptors[i].defaultValue;

#ifndef DGL_NO_SHARED_RESOURCES
        loadSharedResources();
        fontFace("__dpf_dejavusans_ttf__");
#endif
    }

protected:
    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index < values.size())
        {
            values[index] = value;
            repaint();
        }
    }

    void uiIdle() override
    {
        if (telemetrySource == nullptr)
            telemetrySource = findTelemetrySource();

        const bool hadSource = telemetryAvailable;
        const float previousActivity = grainActivity;
        const float previousSequencePhase = sequencePhase;
        const float previousLaunchPulse = launchPulse;
        const std::uint32_t previousActiveVoiceCount = activeVoiceCount;
        bool voicesChanged = false;
        bool spectrumChanged = false;
        const auto* source = telemetrySource;
        if (source != nullptr)
        {
            const auto& telemetry = source->granularFreezeTelemetry();
            const std::uint64_t launchCount = telemetry.launchCount.load(
                std::memory_order_relaxed);
            if (! telemetryAvailable)
                lastLaunchCount = launchCount;
            else if (launchCount != lastLaunchCount)
                launchPulse = 1.0f;
            lastLaunchCount = launchCount;

            targetActivity = std::clamp(telemetry.activity.load(
                std::memory_order_relaxed), 0.0f, 1.0f);
            targetSequencePhase = std::clamp(telemetry.sequencePhase.load(
                std::memory_order_relaxed), 0.0f, 1.0f);
            activeVoiceCount = std::min<std::uint32_t>(
                telemetry.activeVoices.load(std::memory_order_relaxed),
                static_cast<std::uint32_t>(gf::visualVoiceCount));
            for (std::size_t i = 0; i < gf::visualVoiceCount; ++i)
            {
                const float nextPhase = std::clamp(telemetry.voicePhases[i].load(
                    std::memory_order_relaxed), 0.0f, 1.0f);
                const float nextEnvelope = std::clamp(telemetry.voiceEnvelopes[i].load(
                    std::memory_order_relaxed), 0.0f, 1.0f);
                voicesChanged = voicesChanged
                    || std::abs(nextPhase - voicePhases[i]) > 0.002f
                    || std::abs(nextEnvelope - voiceEnvelopes[i]) > 0.002f;
                voicePhases[i] = nextPhase;
                voiceEnvelopes[i] = nextEnvelope;
            }
            for (std::size_t i = 0; i < gf::spectrumBandCount; ++i)
            {
                const float nextLevel = std::clamp(telemetry.spectrumLevels[i].load(
                    std::memory_order_relaxed), 0.0f, 1.0f);
                const float smoothed = spectrumLevels[i]
                    + (nextLevel - spectrumLevels[i]) * 0.34f;
                const float peak = std::max(smoothed, spectrumPeaks[i] * 0.972f);
                spectrumChanged = spectrumChanged
                    || std::abs(smoothed - spectrumLevels[i]) > 0.001f
                    || std::abs(peak - spectrumPeaks[i]) > 0.001f;
                spectrumLevels[i] = smoothed;
                spectrumPeaks[i] = peak;
            }
            telemetryAvailable = true;
        }
        else
        {
            targetActivity = 0.0f;
            targetSequencePhase = 0.0f;
            activeVoiceCount = 0;
            telemetryAvailable = false;
            for (std::size_t i = 0; i < gf::visualVoiceCount; ++i)
                voiceEnvelopes[i] = 0.0f;
            for (std::size_t i = 0; i < gf::spectrumBandCount; ++i)
            {
                const float smoothed = spectrumLevels[i] * 0.70f;
                const float peak = spectrumPeaks[i] * 0.94f;
                spectrumChanged = spectrumChanged
                    || std::abs(smoothed - spectrumLevels[i]) > 0.001f
                    || std::abs(peak - spectrumPeaks[i]) > 0.001f;
                spectrumLevels[i] = smoothed;
                spectrumPeaks[i] = peak;
            }
        }

        grainActivity += (targetActivity - grainActivity) * 0.24f;
        sequencePhase = targetSequencePhase;
        launchPulse *= 0.78f;

        const bool valuesChanged = std::abs(grainActivity - previousActivity) > 0.0005f
            || std::abs(sequencePhase - previousSequencePhase) > 0.0005f
            || std::abs(launchPulse - previousLaunchPulse) > 0.0005f
            || activeVoiceCount != previousActiveVoiceCount;
        if (valuesChanged || voicesChanged || spectrumChanged
            || (hadSource && ! telemetryAvailable))
            repaint();
    }

    void onNanoDisplay() override
    {
        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());
        const bool frozen = values[gf::freezeIndex] > 0.5f;
        const auto& p = palette();

        beginPath();
        rect(0, 0, width, height);
        fillColor(p.backgroundTop);
        fill();
        closePath();

        // The shell follows the website's graphite language: one quiet field,
        // one hairline boundary, and no simulated material texture.
        beginPath();
        roundedRect(20, 80, width - 40, height - 118, 12);
        fillColor(p.bodyTop);
        fill();
        strokeWidth(1.0f);
        strokeColor(p.hairline.withAlpha(0.68f));
        stroke();
        closePath();

        // One quiet rule gives the title room to breathe and keeps the body
        // aligned without turning the interface into a stack of panels.
        beginPath();
        moveTo(contentMargin, headerRuleY);
        lineTo(width - contentMargin, headerRuleY);
        strokeWidth(1.0f);
        strokeColor(p.hairline);
        stroke();
        closePath();

        fontSize(10);
        textLetterSpacing(1.2f);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(p.accent);
        text(contentMargin, 28, "G / 02", nullptr);

        fontSize(25);
        textLetterSpacing(1.0f);
        fillColor(p.ink);
        text(contentMargin, 55, "GRANULAR FREEZE", nullptr);

        fontSize(10);
        textLetterSpacing(0.7f);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(p.quiet);
        text(width - contentMargin, 54, "A MOMENT, PRESERVED", nullptr);
        textLetterSpacing(0.0f);

        drawThemeSelector(width);

        drawSpectralScreen(width);

        drawFreeze();
        for (uint32_t index = 1; index < gf::parameterCount; ++index)
            drawParameter(index);

        fontSize(9);
        textLetterSpacing(0.35f);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(p.whisper);
        text(contentMargin, height - 22,
             "VERTICAL GESTURE  ·  SHIFT / FINE  ·  T / THEME", nullptr);

        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(frozen ? p.accentDim : p.whisper);
        text(width - contentMargin, height - 22,
             frozen ? "MEMORY / HELD" : "SIGNAL / LIVE", nullptr);
        textLetterSpacing(0.0f);
    }

    bool onMouse(const MouseEvent& event) override
    {
        if (event.button != 1)
            return false;

        if (event.press)
        {
            const int selectedTheme = themeAt(event.pos.getX(), event.pos.getY());
            if (selectedTheme >= 0)
            {
                themeIndex = static_cast<std::size_t>(selectedTheme);
                repaint();
                return true;
            }

            const int index = hitTest(event.pos.getX(), event.pos.getY());
            if (index < 0)
                return false;

            if (index == static_cast<int>(gf::freezeIndex))
            {
                const float value = values[gf::freezeIndex] > 0.5f ? 0.0f : 1.0f;
                editParameter(static_cast<uint32_t>(gf::freezeIndex), true);
                setParameterValue(static_cast<uint32_t>(gf::freezeIndex), value);
                editParameter(static_cast<uint32_t>(gf::freezeIndex), false);
                values[gf::freezeIndex] = value;
                repaint();
                return true;
            }

            active = index;
            dragOriginY = event.pos.getY();
            dragOriginValue = values[static_cast<std::size_t>(index)];
            editParameter(static_cast<uint32_t>(index), true);
            repaint();
            return true;
        }

        if (themeAt(event.pos.getX(), event.pos.getY()) >= 0)
            return true;

        if (active >= 0)
        {
            editParameter(static_cast<uint32_t>(active), false);
            active = -1;
            repaint();
            return true;
        }

        return false;
    }

    bool onKeyboard(const KeyboardEvent& event) override
    {
        if (!event.press || event.key != static_cast<uint32_t>('t'))
            return false;

        themeIndex = (themeIndex + 1) % themeCount;
        repaint();
        return true;
    }

    bool onMotion(const MotionEvent& event) override
    {
        const int nextThemeHovered = themeAt(event.pos.getX(), event.pos.getY());
        if (nextThemeHovered != themeHovered)
        {
            themeHovered = nextThemeHovered;
            repaint();
        }

        const int nextHover = hitTest(event.pos.getX(), event.pos.getY());
        if (nextHover != hovered)
        {
            hovered = nextHover;
            repaint();
        }

        if (active < 0)
            return nextHover >= 0 || nextThemeHovered >= 0;

        const auto& descriptor = gf::parameterDescriptors[static_cast<std::size_t>(active)];
        const float range = descriptor.maximum - descriptor.minimum;
        const float fine = (event.mod & DGL_NAMESPACE::kModifierShift) != 0 ? 0.1f : 1.0f;
        const float delta = static_cast<float>(dragOriginY - event.pos.getY()) / 180.0f;
        float value = std::clamp(dragOriginValue + delta * range * fine,
                                 descriptor.minimum, descriptor.maximum);
        if (descriptor.integer)
            value = std::round(value);

        setParameterValue(static_cast<uint32_t>(active), value);
        values[static_cast<std::size_t>(active)] = value;
        repaint();
        return true;
    }

private:
    std::array<float, gf::parameterCount> values {};
    std::size_t themeIndex = 0;
    int themeHovered = -1;
    int hovered = -1;
    int active = -1;
    double dragOriginY = 0.0;
    float dragOriginValue = 0.0f;
    const gf::GranularFreezeTelemetrySource* telemetrySource = nullptr;
    bool telemetryAvailable = false;
    std::uint64_t lastLaunchCount = 0;
    float targetActivity = 0.0f;
    float grainActivity = 0.0f;
    float targetSequencePhase = 0.0f;
    float sequencePhase = 0.0f;
    float launchPulse = 0.0f;
    std::uint32_t activeVoiceCount = 0;
    std::array<float, gf::visualVoiceCount> voicePhases {};
    std::array<float, gf::visualVoiceCount> voiceEnvelopes {};
    std::array<float, gf::spectrumBandCount> spectrumLevels {};
    std::array<float, gf::spectrumBandCount> spectrumPeaks {};

    const gf::GranularFreezeTelemetrySource* findTelemetrySource() const noexcept
    {
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        void* const pointer = getPluginInstancePointer();
        if (pointer == nullptr)
            return nullptr;

        Plugin* const plugin = static_cast<Plugin*>(pointer);
        return dynamic_cast<const gf::GranularFreezeTelemetrySource*>(plugin);
#else
        return nullptr;
#endif
    }

    const ThemePalette& palette() const noexcept
    {
        return themePalettes[themeIndex];
    }

    void drawThemeSelector(const float width)
    {
        const auto& p = palette();
        const float end = width - contentMargin;
        const float first = end - themeSwatchGap * 2.0f;

        fontSize(8);
        textLetterSpacing(0.8f);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(p.whisper);
        text(first - 11.0f, themeSwatchY, "THEMES", nullptr);
        textLetterSpacing(0.0f);

        for (std::size_t i = 0; i < themeCount; ++i)
        {
            const float x = first + static_cast<float>(i) * themeSwatchGap;
            const bool selected = i == themeIndex;
            const bool focus = static_cast<int>(i) == themeHovered;

            beginPath();
            circle(x, themeSwatchY, themeSwatchRadius + (focus ? 2.0f : 1.0f));
            strokeWidth(selected ? 1.0f : 0.6f);
            strokeColor(selected ? p.accent.withAlpha(0.85f)
                                 : p.quiet.withAlpha(focus ? 0.55f : 0.24f));
            stroke();
            closePath();

            beginPath();
            circle(x, themeSwatchY, themeSwatchRadius);
            fillColor(themePalettes[i].accentDim.withAlpha(selected ? 0.95f : 0.72f));
            fill();
            closePath();
        }
    }

    int themeAt(const double px, const double py) const
    {
        if (std::abs(py - themeSwatchY) > 11.0)
            return -1;

        const float end = static_cast<float>(getWidth()) - contentMargin;
        const float first = end - themeSwatchGap * 2.0f;
        for (int i = 0; i < static_cast<int>(themeCount); ++i)
        {
            const float x = first + static_cast<float>(i) * themeSwatchGap;
            if (std::abs(px - x) <= 10.0)
                return i;
        }

        return -1;
    }

    static void controlBounds(const int index, float& x, float& y,
                              float& width, float& height)
    {
        if (index == static_cast<int>(gf::freezeIndex))
        {
            x = contentMargin;
            y = topControlY;
            width = topControlWidth;
            height = topControlHeight;
            return;
        }

        if (index <= static_cast<int>(gf::holdMsIndex))
        {
            const int column = index;
            x = contentMargin + static_cast<float>(column) * topControlStep;
            y = topControlY;
            width = topControlWidth;
            height = topControlHeight;
            return;
        }

        const int column = index - static_cast<int>(gf::grainSizeMsIndex);
        x = contentMargin + static_cast<float>(column) * bottomControlStep;
        y = bottomControlY;
        width = bottomControlWidth;
        height = bottomControlHeight;
    }

    static int hitTest(const double px, const double py)
    {
        for (int index = 0; index < static_cast<int>(gf::parameterCount); ++index)
        {
            float x, y, width, height;
            controlBounds(index, x, y, width, height);
            if (px >= x && px <= x + width && py >= y && py <= y + height)
                return index;
        }
        return -1;
    }

    void drawSpectralScreen(const float width)
    {
        const auto& p = palette();
        const float actualWidth = std::min(width - contentMargin * 2.0f, screenWidth);
        const float actualX = (width - actualWidth) * 0.5f;
        const float actualRight = actualX + actualWidth;
        const float innerX = actualX + 13.0f;
        const float innerY = screenY + 12.0f;
        const float innerWidth = actualWidth - 26.0f;
        const float innerHeight = screenHeight - 24.0f;
        const float floor = innerY + innerHeight - 4.0f;
        float spectralPeak = 0.0f;
        for (const float peak : spectrumPeaks)
            spectralPeak = std::max(spectralPeak, peak);
        const bool hasSignal = telemetryAvailable && (spectralPeak > 0.008f
            || grainActivity > 0.01f || activeVoiceCount > 0
            || launchPulse > 0.01f);

        beginPath();
        roundedRect(actualX, screenY, actualWidth, screenHeight, 8.0f);
        fillColor(p.surfaceBottom);
        fill();
        strokeWidth(hasSignal ? 1.0f : 0.75f);
        strokeColor(hasSignal ? p.accentDim.withAlpha(0.72f)
                              : p.hairline.withAlpha(0.62f));
        stroke();
        closePath();

        beginPath();
        roundedRect(innerX, innerY, innerWidth, innerHeight, 5.0f);
        fillColor(p.backgroundBottom);
        fill();
        strokeWidth(0.55f);
        strokeColor(p.hairline.withAlpha(0.36f));
        stroke();
        closePath();

        // The line grid stays almost hidden until the signal gives it a reason
        // to breathe. The screen itself is the only place where motion lives.
        for (int row = 0; row < 3; ++row)
        {
            const float y = innerY + 12.0f + static_cast<float>(row) * 22.0f;
            beginPath();
            moveTo(innerX + 4.0f, y);
            lineTo(actualRight - 4.0f, y);
            strokeWidth(0.45f);
            strokeColor(p.hairline.withAlpha(0.15f));
            stroke();
            closePath();
        }

        beginPath();
        moveTo(innerX + 4.0f, floor);
        lineTo(actualRight - 4.0f, floor);
        strokeWidth(0.65f);
        strokeColor(p.hairline.withAlpha(0.36f));
        stroke();
        closePath();

        const float step = (innerWidth - 8.0f)
            / static_cast<float>(gf::spectrumBandCount - 1);
        const auto pointX = [innerX, step](const std::size_t index) {
            return innerX + 4.0f + static_cast<float>(index) * step;
        };
        const auto pointY = [floor, innerHeight](const float level) {
            const float shaped = std::pow(std::clamp(level, 0.0f, 1.0f), 0.72f);
            return floor - 4.0f - shaped * (innerHeight - 15.0f);
        };

        // A single continuous ribbon keeps the display spectral rather than
        // turning it into a collection of unrelated columns. The tiny
        // under-trace gives the meter a physical presence without gloss.
        const auto drawTrace = [&](const bool fillArea, const Color color,
                                   const float widthValue) {
            beginPath();
            const float firstX = pointX(0);
            const float firstY = pointY(spectrumLevels[0]);
            moveTo(firstX, firstY);
            for (std::size_t i = 1; i < gf::spectrumBandCount; ++i)
            {
                const float previousX = pointX(i - 1);
                const float previousY = pointY(spectrumLevels[i - 1]);
                const float currentX = pointX(i);
                const float currentY = pointY(spectrumLevels[i]);
                bezierTo(previousX + step * 0.42f, previousY,
                         currentX - step * 0.42f, currentY,
                         currentX, currentY);
            }

            if (fillArea)
            {
                lineTo(pointX(gf::spectrumBandCount - 1), floor);
                lineTo(firstX, floor);
                closePath();
                fillColor(color);
                fill();
            }
            else
            {
                strokeWidth(widthValue);
                strokeColor(color);
                stroke();
            }
        };

        drawTrace(true, p.accent.withAlpha(hasSignal ? 0.045f : 0.012f), 0.0f);
        drawTrace(false, p.accent.withAlpha(hasSignal ? 0.17f : 0.055f), 2.2f);
        drawTrace(false,
                  p.accent.withAlpha(hasSignal
                      ? 0.68f + std::min(grainActivity, 1.0f) * 0.10f : 0.16f),
                  hasSignal ? 1.15f : 0.65f);

        // Peak ticks have their own decay, so transients remain legible after
        // the main ribbon has moved on without leaving a noisy history trail.
        for (std::size_t i = 0; i < gf::spectrumBandCount; ++i)
        {
            const float peak = std::clamp(spectrumPeaks[i], 0.0f, 1.0f);
            const float level = std::clamp(spectrumLevels[i], 0.0f, 1.0f);
            if (! hasSignal || peak <= 0.018f || peak <= level + 0.015f)
                continue;

            const float peakY = pointY(peak);
            const float tickLength = std::min(8.0f, 1.5f
                + (peak - level) * 26.0f);
            beginPath();
            moveTo(pointX(i), peakY);
            lineTo(pointX(i), peakY - tickLength);
            strokeWidth(0.55f);
            strokeColor(p.ink.withAlpha(0.16f + peak * 0.20f));
            stroke();
            closePath();
        }

        // The scheduler is retained as one quiet reference line, never as a
        // second animated object competing with the frequency display.
        if (hasSignal && activeVoiceCount > 0)
        {
            const float playheadX = innerX + 4.0f
                + sequencePhase * (innerWidth - 8.0f);
            beginPath();
            moveTo(playheadX, innerY + 4.0f);
            lineTo(playheadX, floor + 1.0f);
            strokeWidth(0.7f + launchPulse * 0.25f);
            strokeColor(p.accent.withAlpha(0.22f + launchPulse * 0.28f));
            stroke();
            closePath();
        }
    }

    void drawFreeze()
    {
        float x, y, width, height;
        controlBounds(static_cast<int>(gf::freezeIndex), x, y, width, height);
        const bool frozen = values[gf::freezeIndex] > 0.5f;
        const bool focus = hovered == static_cast<int>(gf::freezeIndex);
        const auto& p = palette();

        beginPath();
        roundedRect(x, y, width, height, 4.0f);
        fillColor(frozen ? p.surfaceLiftedTop : p.surfaceTop);
        fill();
        strokeWidth(focus || frozen ? 1.0f : 0.65f);
        strokeColor(frozen ? p.accent.withAlpha(0.82f)
                           : (focus ? p.quiet : p.hairline.withAlpha(0.34f)));
        stroke();
        closePath();

        beginPath();
        rect(x + 1.0f, y + 12.0f, 1.0f, height - 23.0f);
        fillColor(frozen ? p.accent : p.hairline);
        fill();
        closePath();

        fontSize(9);
        textLetterSpacing(0.55f);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(p.quiet);
        text(x + 14.0f, y + 13.0f, "FREEZE", nullptr);
        textLetterSpacing(0.0f);

        fontSize(16);
        fillColor(frozen ? p.accent : p.ink);
        text(x + 14.0f, y + 32.0f, frozen ? "HELD" : "LIVE", nullptr);

        fontSize(8);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(p.whisper);
        text(x + width - 14.0f, y + 13.0f, "MEMORY", nullptr);

        beginPath();
        roundedRect(x + 14.0f, y + height - 8.0f, width - 28.0f, 2.0f, 1.0f);
        fillColor(p.track);
        fill();
        closePath();
        if (frozen)
        {
            beginPath();
            roundedRect(x + 14.0f, y + height - 8.0f,
                        width - 28.0f, 2.0f, 1.0f);
            fillColor(p.accentDim);
            fill();
            closePath();
        }
    }

    void drawParameter(const uint32_t index)
    {
        float x, y, width, height;
        controlBounds(static_cast<int>(index), x, y, width, height);
        const auto& descriptor = gf::parameterDescriptors[index];
        const auto& p = palette();
        const float normalized = std::clamp(
            (values[index] - descriptor.minimum) /
                (descriptor.maximum - descriptor.minimum),
            0.0f, 1.0f);
        const bool focus = hovered == static_cast<int>(index) ||
                           active == static_cast<int>(index);

        // The website uses flat graphite cards. Focus is communicated by a
        // hairline and a restrained lift, never by gloss, texture, or shadow.
        beginPath();
        roundedRect(x, y, width, height, 4);
        fillColor(focus ? p.surfaceLiftedTop : p.surfaceTop);
        fill();
        strokeWidth(focus ? 1.0f : 0.65f);
        strokeColor(focus ? p.hairline.withAlpha(0.90f)
                          : p.hairline.withAlpha(0.32f));
        stroke();
        closePath();

        beginPath();
        rect(x + 1, y + 12, 1, height - 22);
        fillColor(focus ? p.accent : p.hairline);
        fill();
        closePath();

        fontSize(9);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(p.quiet);
        text(x + 14, y + 13, descriptor.name, nullptr);

        char value[48];
        formatValue(index, value, sizeof(value));
        fontSize(16);
        fillColor(p.ink);
        text(x + 14, y + 31, value, nullptr);

        beginPath();
        roundedRect(x + 14, y + height - 8, width - 28, 2, 1);
        fillColor(p.track);
        fill();
        closePath();

        if (normalized > 0)
        {
            beginPath();
            roundedRect(x + 14, y + height - 8,
                        (width - 28) * normalized, 2, 1);
            fillColor(focus ? p.accent : p.accentDim);
            fill();
            closePath();
        }
    }

    void formatValue(const uint32_t index, char* const output,
                     const std::size_t size) const
    {
        const float value = values[index];
        switch (index)
        {
        case gf::pitchIndex:
            std::snprintf(output, size, "%.2f ×", value);
            break;
        case gf::holdMsIndex:
            if (value >= 1000)
                std::snprintf(output, size, "%.2f s", value / 1000);
            else
                std::snprintf(output, size, "%.0f ms", value);
            break;
        case gf::positionIndex:
            std::snprintf(output, size, "%.0f %%", value * 100);
            break;
        default:
            std::snprintf(output, size, "%.0f %s", value,
                          gf::parameterDescriptors[index].unit);
            break;
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularFreezeUI)
};

UI* createUI() { return new GranularFreezeUI(); }
END_NAMESPACE_DISTRHO
