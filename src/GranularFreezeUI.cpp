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
const Color backdrop(11, 13, 16);
const Color plate(22, 24, 28);
const Color plateLifted(29, 31, 35);
const Color ink(236, 232, 223);
const Color quiet(143, 143, 139);
const Color whisper(91, 94, 95);
const Color accent(181, 208, 190);
const Color accentDim(78, 104, 89);
const Color hairline(47, 49, 53);
const Color track(37, 39, 43);

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

        beginPath();
        rect(0, 0, width, height);
        fillColor(backdrop);
        fill();
        closePath();

        // One quiet rule gives the title room to breathe and keeps the body
        // aligned without turning the interface into a stack of panels.
        beginPath();
        moveTo(contentMargin, headerRuleY);
        lineTo(width - contentMargin, headerRuleY);
        strokeWidth(1.0f);
        strokeColor(hairline);
        stroke();
        closePath();

        fontSize(10);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(accent);
        text(contentMargin, 28, "G / 02", nullptr);

        fontSize(25);
        fillColor(ink);
        text(contentMargin, 55, "GRANULAR FREEZE", nullptr);

        fontSize(10);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(quiet);
        text(width - contentMargin, 54, "A MOMENT, PRESERVED", nullptr);

        // The thin spine separates the memory control from the parameters,
        // while the generous empty space keeps the composition ceremonial.
        beginPath();
        moveTo(250, parameterStartY + 2);
        lineTo(250, parameterStartY + parameterStepY + parameterHeight - 2);
        strokeWidth(1.0f);
        strokeColor(hairline);
        stroke();
        closePath();

        drawFreeze();
        for (uint32_t index = 1; index < gf::parameterCount; ++index)
            drawParameter(index);

        fontSize(9);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(whisper);
        text(contentMargin, height - 22, "VERTICAL GESTURE  ·  SHIFT / FINE", nullptr);

        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(frozen ? accentDim : whisper);
        text(width - contentMargin, height - 22,
             frozen ? "MEMORY / HELD" : "SIGNAL / LIVE", nullptr);
    }

    bool onMouse(const MouseEvent& event) override
    {
        if (event.button != 1)
            return false;

        if (event.press)
        {
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

        if (active >= 0)
        {
            editParameter(static_cast<uint32_t>(active), false);
            active = -1;
            repaint();
            return true;
        }

        return false;
    }

    bool onMotion(const MotionEvent& event) override
    {
        const int nextHover = hitTest(event.pos.getX(), event.pos.getY());
        if (nextHover != hovered)
        {
            hovered = nextHover;
            repaint();
        }

        if (active < 0)
            return nextHover >= 0;

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
    int hovered = -1;
    int active = -1;
    double dragOriginY = 0.0;
    float dragOriginValue = 0.0f;

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
        constexpr float cx = freezeX + freezeWidth * 0.5f;
        constexpr float cy = 198.0f;
        constexpr float radius = 63.0f;

        beginPath();
        circle(cx, cy, radius + (focus ? 2.0f : 0.0f));
        fillColor(frozen ? Color(24, 40, 34) : plate);
        fill();
        strokeWidth(frozen ? 1.5f : 1.0f);
        strokeColor(frozen ? accent : (focus ? Color(91, 96, 101) : hairline));
        stroke();
        closePath();

        // A partial orbit gives the control a sense of captured time without
        // leaning on the familiar power-button, snowflake, or waveform icons.
        beginPath();
        arc(cx, cy, radius - 10.0f, -2.15f,
            frozen ? 3.60f : -0.55f, CCW);
        strokeWidth(2.0f);
        strokeColor(frozen ? accent : accentDim);
        stroke();
        closePath();

        fontSize(10);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(quiet);
        text(cx, cy - 29, "MEMORY", nullptr);

        fontSize(19);
        fillColor(frozen ? accent : ink);
        text(cx, cy + 2, frozen ? "HELD" : "LIVE", nullptr);

        fontSize(9);
        fillColor(accentDim);
        text(cx, cy + 77, frozen ? "RELEASE" : "CAPTURE", nullptr);
    }

    void drawParameter(const uint32_t index)
    {
        float x, y, width, height;
        controlBounds(static_cast<int>(index), x, y, width, height);
        const auto& descriptor = gf::parameterDescriptors[index];
        const float normalized = std::clamp(
            (values[index] - descriptor.minimum) /
                (descriptor.maximum - descriptor.minimum),
            0.0f, 1.0f);
        const bool focus = hovered == static_cast<int>(index) ||
                           active == static_cast<int>(index);

        // The normal state is nearly unmarked: typography and a single rail
        // carry the control. A lifted surface appears only when the hand is
        // over it, which keeps the idle interface still and self-possessed.
        if (focus)
        {
            beginPath();
            roundedRect(x, y, width, height, 4);
            fillColor(plateLifted);
            fill();
            strokeWidth(1.0f);
            strokeColor(Color(59, 63, 67));
            stroke();
            closePath();
        }

        beginPath();
        rect(x + 1, y + 14, 1, height - 28);
        fillColor(focus ? accent : hairline);
        fill();
        closePath();

        fontSize(10);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(quiet);
        text(x + 14, y + 18, descriptor.name, nullptr);

        char value[48];
        formatValue(index, value, sizeof(value));
        fontSize(17);
        fillColor(ink);
        text(x + 14, y + 46, value, nullptr);

        beginPath();
        roundedRect(x + 14, y + height - 14, width - 28, 2, 1);
        fillColor(track);
        fill();
        closePath();

        if (normalized > 0)
        {
            beginPath();
            roundedRect(x + 14, y + height - 14,
                        (width - 28) * normalized, 2, 1);
            fillColor(focus ? accent : accentDim);
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
