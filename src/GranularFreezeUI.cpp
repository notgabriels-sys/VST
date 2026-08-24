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
const Color ink(232, 230, 224);
const Color quiet(119, 123, 130);
const Color surface(22, 24, 29);
const Color surfaceLifted(28, 31, 37);
const Color accent(177, 222, 202);
const Color accentDim(73, 106, 94);
const Color hairline(45, 48, 55);
}

class GranularFreezeUI final : public UI
{
public:
    GranularFreezeUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = gf::parameterDescriptors[i].defaultValue;
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
        beginPath(); rect(0, 0, getWidth(), getHeight());
        fillColor(Color(12, 14, 18)); fill(); closePath();

        beginPath(); moveTo(24, 72); lineTo(getWidth() - 24, 72);
        strokeWidth(1.0f); strokeColor(hairline); stroke(); closePath();

        fontSize(11); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(accent);
        text(24, 25, "G / 02", nullptr);
        fontSize(20); fillColor(ink);
        text(79, 25, "GRANULAR FREEZE", nullptr);
        fontSize(10); fillColor(quiet); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        text(getWidth() - 24, 25, "TIME, HELD JUST BEFORE IT VANISHES", nullptr);

        drawFreeze();
        for (uint32_t index = 1; index < gf::parameterCount; ++index)
            drawParameter(index);

        fontSize(9); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(Color(76, 80, 88));
        text(24, getHeight() - 15, "DRAG  ·  SHIFT FOR FINE CONTROL", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        text(getWidth() - 24, getHeight() - 15,
             values[0] > 0.5f ? "MEMORY SUSPENDED" : "LISTENING", nullptr);
    }

    bool onMouse(const MouseEvent& event) override
    {
        if (event.button != 1) return false;
        if (event.press)
        {
            const int index = hitTest(event.pos.getX(), event.pos.getY());
            if (index < 0) return false;
            if (index == static_cast<int>(gf::freezeIndex))
            {
                const float value = values[0] > 0.5f ? 0.0f : 1.0f;
                editParameter(0, true); setParameterValue(0, value); editParameter(0, false);
                values[0] = value; repaint(); return true;
            }
            active = index;
            dragOriginY = event.pos.getY();
            dragOriginValue = values[static_cast<std::size_t>(index)];
            editParameter(static_cast<uint32_t>(index), true);
            repaint(); return true;
        }
        if (active >= 0)
        {
            editParameter(static_cast<uint32_t>(active), false);
            active = -1; repaint(); return true;
        }
        return false;
    }

    bool onMotion(const MotionEvent& event) override
    {
        const int nextHover = hitTest(event.pos.getX(), event.pos.getY());
        if (nextHover != hovered) { hovered = nextHover; repaint(); }
        if (active < 0) return nextHover >= 0;

        const auto& descriptor = gf::parameterDescriptors[static_cast<std::size_t>(active)];
        const float range = descriptor.maximum - descriptor.minimum;
        const float fine = (event.mod & DGL_NAMESPACE::kModifierShift) != 0 ? 0.1f : 1.0f;
        const float delta = static_cast<float>(dragOriginY - event.pos.getY()) / 150.0f;
        float value = std::clamp(dragOriginValue + delta * range * fine,
                                 descriptor.minimum, descriptor.maximum);
        if (descriptor.integer) value = std::round(value);
        setParameterValue(static_cast<uint32_t>(active), value);
        values[static_cast<std::size_t>(active)] = value;
        repaint(); return true;
    }

private:
    std::array<float, gf::parameterCount> values {};
    int hovered = -1;
    int active = -1;
    double dragOriginY = 0.0;
    float dragOriginValue = 0.0f;

    static void controlBounds(const int index, float& x, float& y, float& width, float& height)
    {
        if (index == 0) { x = 24; y = 88; width = 126; height = 156; return; }
        const int column = (index - 1) % 3;
        const int row = (index - 1) / 3;
        x = 174.0f + static_cast<float>(column) * 142.0f;
        y = 88.0f + static_cast<float>(row) * 83.0f;
        width = 124; height = 68;
    }

    static int hitTest(const double px, const double py)
    {
        for (int index = 0; index < static_cast<int>(gf::parameterCount); ++index)
        {
            float x, y, width, height;
            controlBounds(index, x, y, width, height);
            if (px >= x && px <= x + width && py >= y && py <= y + height) return index;
        }
        return -1;
    }

    void drawFreeze()
    {
        const bool frozen = values[0] > 0.5f;
        const bool focus = hovered == 0;
        constexpr float cx = 87, cy = 151, radius = 42;

        beginPath(); circle(cx, cy, radius + (focus ? 3.0f : 0.0f));
        fillColor(frozen ? Color(27, 47, 40) : surface); fill();
        strokeWidth(frozen ? 1.5f : 1.0f);
        strokeColor(frozen ? accent : (focus ? Color(88, 94, 103) : hairline));
        stroke(); closePath();

        // An incomplete orbit suggests captured time without a literal
        // snowflake, waveform, or glowing power-button cliché.
        beginPath();
        arc(cx, cy, radius - 9.0f, -2.35f, frozen ? 2.65f : 0.35f, CCW);
        strokeWidth(2.0f); strokeColor(frozen ? accent : accentDim); stroke(); closePath();

        fontSize(10); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(quiet);
        text(cx, 105, "STATE", nullptr);
        fontSize(16); fillColor(frozen ? accent : ink);
        text(cx, cy + 1, frozen ? "HELD" : "LIVE", nullptr);
        fontSize(9); fillColor(Color(88, 92, 100));
        text(cx, 215, frozen ? "RELEASE" : "CAPTURE", nullptr);
    }

    void drawParameter(const uint32_t index)
    {
        float x, y, width, height;
        controlBounds(static_cast<int>(index), x, y, width, height);
        const auto& descriptor = gf::parameterDescriptors[index];
        const float normalized = std::clamp((values[index] - descriptor.minimum) /
                                            (descriptor.maximum - descriptor.minimum), 0.0f, 1.0f);
        const bool focus = hovered == static_cast<int>(index) || active == static_cast<int>(index);

        beginPath(); roundedRect(x, y, width, height, 5);
        fillColor(focus ? surfaceLifted : surface); fill();
        if (focus) { strokeWidth(1); strokeColor(Color(58, 64, 70)); stroke(); }
        closePath();

        fontSize(9); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(quiet);
        text(x + 10, y + 14, descriptor.name, nullptr);
        char value[48]; formatValue(index, value, sizeof(value));
        fontSize(15); fillColor(ink); text(x + 10, y + 36, value, nullptr);

        beginPath(); roundedRect(x + 10, y + height - 11, width - 20, 2, 1);
        fillColor(hairline); fill(); closePath();
        if (normalized > 0)
        {
            beginPath(); roundedRect(x + 10, y + height - 11, (width - 20) * normalized, 2, 1);
            fillColor(focus ? accent : accentDim); fill(); closePath();
        }
    }

    void formatValue(const uint32_t index, char* const output, const std::size_t size) const
    {
        const float value = values[index];
        switch (index)
        {
        case gf::pitchIndex: std::snprintf(output, size, "%.2f ×", value); break;
        case gf::holdMsIndex:
            if (value >= 1000) std::snprintf(output, size, "%.2f s", value / 1000);
            else std::snprintf(output, size, "%.0f ms", value);
            break;
        case gf::positionIndex: std::snprintf(output, size, "%.0f %%", value * 100); break;
        default: std::snprintf(output, size, "%.0f %s", value, gf::parameterDescriptors[index].unit); break;
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularFreezeUI)
};

UI* createUI() { return new GranularFreezeUI(); }
END_NAMESPACE_DISTRHO
