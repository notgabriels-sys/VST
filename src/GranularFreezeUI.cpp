#include "DistrhoUI.hpp"
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
    Color ambient;
    Color bodyTop;
    Color bodyBottom;
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
    Color texture;
};

// The palettes are intentionally quiet: color changes the atmosphere, while
// the geometry and interaction remain the same. T cycles through these
// options, and the three small header swatches select one directly.
const ThemePalette themePalettes[] = {
    {
        Color(14, 16, 20), Color(7, 9, 12), Color(166, 198, 177),
        Color(24, 28, 33), Color(11, 14, 18),
        Color(36, 40, 46), Color(17, 20, 24),
        Color(48, 54, 59), Color(25, 29, 34),
        Color(236, 232, 223), Color(143, 143, 139), Color(91, 94, 95),
        Color(181, 208, 190), Color(78, 104, 89), Color(47, 49, 53),
        Color(37, 39, 43), Color(118, 129, 128)
    },
    {
        Color(20, 15, 14), Color(9, 8, 9), Color(207, 145, 102),
        Color(34, 27, 25), Color(15, 11, 12),
        Color(48, 37, 33), Color(22, 17, 18),
        Color(61, 46, 39), Color(30, 22, 23),
        Color(241, 230, 218), Color(164, 145, 133), Color(104, 91, 85),
        Color(218, 169, 127), Color(128, 83, 59), Color(67, 50, 45),
        Color(48, 36, 34), Color(139, 111, 99)
    },
    {
        Color(15, 15, 23), Color(8, 8, 13), Color(171, 153, 216),
        Color(25, 24, 35), Color(11, 10, 18),
        Color(38, 36, 51), Color(18, 17, 27),
        Color(50, 46, 65), Color(25, 23, 37),
        Color(235, 231, 244), Color(153, 149, 171), Color(94, 91, 112),
        Color(188, 174, 229), Color(103, 87, 143), Color(50, 48, 66),
        Color(39, 37, 54), Color(119, 111, 143)
    }
};

constexpr std::size_t themeCount = sizeof(themePalettes) / sizeof(themePalettes[0]);
constexpr float themeSwatchY = 28.0f;
constexpr float themeSwatchGap = 17.0f;
constexpr float themeSwatchRadius = 3.5f;

constexpr float contentMargin = 32.0f;
constexpr float headerRuleY = 82.0f;
constexpr float freezeX = 32.0f;
constexpr float freezeY = 106.0f;
constexpr float freezeWidth = 192.0f;
constexpr float freezeHeight = 190.0f;
constexpr float parameterStartX = 278.0f;
constexpr float parameterStepX = 142.0f;
constexpr float parameterWidth = 126.0f;
constexpr float parameterStartY = 106.0f;
constexpr float parameterStepY = 93.0f;
constexpr float parameterHeight = 74.0f;
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

    void onNanoDisplay() override
    {
        const float width = static_cast<float>(getWidth());
        const float height = static_cast<float>(getHeight());
        const bool frozen = values[gf::freezeIndex] > 0.5f;
        const auto& p = palette();

        beginPath();
        rect(0, 0, width, height);
        fillPaint(linearGradient(0, 0, 0, height,
                                 p.backgroundTop, p.backgroundBottom));
        fill();
        closePath();

        // A shallow listening field gives the body a material boundary while
        // keeping the composition open and almost architectural.
        beginPath();
        roundedRect(20, 94, width - 40, height - 132, 12);
        fillPaint(boxGradient(20, 94, width - 40, height - 132, 12, 18,
                              p.bodyTop.withAlpha(0.20f),
                              p.bodyBottom.withAlpha(0.02f)));
        fill();
        strokeWidth(1.0f);
        strokeColor(p.hairline.withAlpha(0.58f));
        stroke();
        closePath();

        // Sparse, deterministic micro-lines create a tactile grain without
        // turning the background into visible noise or visual decoration.
        save();
        scissor(20, 94, width - 40, height - 132);
        beginPath();
        strokeWidth(0.5f);
        strokeColor(p.texture.withAlpha(0.11f));
        for (int row = 0; row < 24; ++row)
        {
            const float y = 101.0f + static_cast<float>(row) * 9.0f;
            const float x = 29.0f + static_cast<float>((row * 31) % 87);
            moveTo(x, y);
            lineTo(x + 18.0f + static_cast<float>((row % 4) * 9), y);

            if ((row % 3) == 0)
            {
                moveTo(x + 240.0f, y + 3.0f);
                lineTo(x + 264.0f + static_cast<float>((row % 2) * 12), y + 3.0f);
            }
        }
        stroke();
        closePath();
        restore();

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

        // The thin spine separates the memory control from the parameters,
        // while the generous empty space keeps the composition ceremonial.
        beginPath();
        moveTo(250, parameterStartY + 2);
        lineTo(250, parameterStartY + parameterStepY + parameterHeight - 2);
        strokeWidth(1.0f);
        strokeColor(p.hairline);
        stroke();
        closePath();

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
            x = freezeX;
            y = freezeY;
            width = freezeWidth;
            height = freezeHeight;
            return;
        }

        const int column = (index - 1) % 3;
        const int row = (index - 1) / 3;
        x = parameterStartX + static_cast<float>(column) * parameterStepX;
        y = parameterStartY + static_cast<float>(row) * parameterStepY;
        width = parameterWidth;
        height = parameterHeight;
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

    void drawFreeze()
    {
        const bool frozen = values[gf::freezeIndex] > 0.5f;
        const bool focus = hovered == static_cast<int>(gf::freezeIndex);
        const auto& p = palette();
        constexpr float cx = freezeX + freezeWidth * 0.5f;
        constexpr float cy = 198.0f;
        constexpr float radius = 63.0f;

        beginPath();
        circle(cx, cy, radius + 15.0f + (focus ? 2.0f : 0.0f));
        fillPaint(radialGradient(cx - 12.0f, cy - 18.0f, radius - 18.0f,
                                 radius + 22.0f,
                                 p.ambient.withAlpha(frozen ? 0.16f : 0.07f),
                                 p.ambient.withAlpha(0.0f)));
        fill();
        closePath();

        beginPath();
        circle(cx, cy, radius + (focus ? 2.0f : 0.0f));
        const Color medallionTop = frozen
            ? Color(p.surfaceTop, p.accentDim, 0.26f)
            : p.surfaceTop;
        fillPaint(radialGradient(cx - 18.0f, cy - 22.0f, 5.0f,
                                 radius + 16.0f, medallionTop, p.surfaceBottom));
        fill();
        strokeWidth(frozen ? 1.5f : 1.0f);
        strokeColor(frozen ? p.accent
                           : (focus ? p.quiet : p.hairline));
        stroke();
        closePath();

        beginPath();
        circle(cx, cy, radius - 5.0f);
        strokeWidth(0.7f);
        strokeColor(p.ink.withAlpha(frozen ? 0.12f : 0.07f));
        stroke();
        closePath();

        // A partial orbit gives the control a sense of captured time without
        // leaning on the familiar power-button, snowflake, or waveform icons.
        beginPath();
        arc(cx, cy, radius - 10.0f, -2.15f,
            frozen ? 3.60f : -0.55f, CCW);
        strokeWidth(2.0f);
        strokeColor(frozen ? p.accent : p.accentDim);
        stroke();
        closePath();

        beginPath();
        arc(cx, cy, radius - 6.0f, -2.40f, -1.22f, CCW);
        strokeWidth(0.8f);
        strokeColor(p.ink.withAlpha(0.14f));
        stroke();
        closePath();

        fontSize(10);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(p.quiet);
        text(cx, cy - 29, "MEMORY", nullptr);

        fontSize(19);
        fillColor(frozen ? p.accent : p.ink);
        text(cx, cy + 2, frozen ? "HELD" : "LIVE", nullptr);

        fontSize(9);
        fillColor(p.accentDim);
        text(cx, cy + 77, frozen ? "RELEASE" : "CAPTURE", nullptr);
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

        // The resting state is still, but not flat: a very low-contrast
        // material gradient gives each control a surface, while focus raises
        // it with a clearer edge and a cooler highlight.
        beginPath();
        roundedRect(x, y, width, height, 4);
        fillPaint(boxGradient(x, y, width, height, 4, 12,
                              focus ? p.surfaceLiftedTop.withAlpha(0.88f)
                                    : p.surfaceTop.withAlpha(0.23f),
                              focus ? p.surfaceLiftedBottom.withAlpha(0.58f)
                                    : p.surfaceBottom.withAlpha(0.04f)));
        fill();
        strokeWidth(focus ? 1.0f : 0.65f);
        strokeColor(focus ? p.hairline.withAlpha(0.90f)
                          : p.hairline.withAlpha(0.32f));
        stroke();
        closePath();

        beginPath();
        moveTo(x + 8, y + 1);
        lineTo(x + width - 8, y + 1);
        strokeWidth(0.7f);
        strokeColor(p.ink.withAlpha(focus ? 0.15f : 0.045f));
        stroke();
        closePath();

        beginPath();
        rect(x + 1, y + 14, 1, height - 28);
        fillColor(focus ? p.accent : p.hairline);
        fill();
        closePath();

        fontSize(10);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(p.quiet);
        text(x + 14, y + 18, descriptor.name, nullptr);

        char value[48];
        formatValue(index, value, sizeof(value));
        fontSize(17);
        fillColor(p.ink);
        text(x + 14, y + 46, value, nullptr);

        beginPath();
        roundedRect(x + 14, y + height - 14, width - 28, 2, 1);
        fillColor(p.track);
        fill();
        closePath();

        if (normalized > 0)
        {
            beginPath();
            roundedRect(x + 14, y + height - 14,
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
