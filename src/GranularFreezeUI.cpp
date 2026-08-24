#include "DistrhoUI.hpp"
#include "dsp/Parameters.h"

#include <algorithm>
#include <array>
#include <cstdio>

START_NAMESPACE_DISTRHO
using DGL_NAMESPACE::Color;

class GranularFreezeUI final : public UI
{
public:
    GranularFreezeUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = gf::parameterDescriptors[i].defaultValue;
    }
protected:
    void parameterChanged(uint32_t index, float value) override
    {
        if (index < values.size()) { values[index] = value; repaint(); }
    }
    void onNanoDisplay() override
    {
        beginPath(); rect(0, 0, getWidth(), getHeight()); fillColor(Color(15, 17, 22)); fill(); closePath();
        fontSize(25); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(Color(238, 241, 246));
        text(24, 34, "GRANULAR FREEZE", nullptr);
        fontSize(12); fillColor(Color(130, 141, 158));
        text(25, 57, "DETERMINISTIC LIVE TEXTURE", nullptr);

        drawControl(0, 24, 82, 120, 74);
        for (uint32_t index = 1; index < gf::parameterCount; ++index)
        {
            const uint32_t column = (index - 1) % 3;
            const uint32_t row = (index - 1) / 3;
            drawControl(index, 166 + column * 148, 82 + row * 102, 128, 82);
        }
    }
    bool onMouse(const MouseEvent& event) override
    {
        if (event.button != 1 || ! event.press) return false;
        const int index = hitTest(event.pos.getX(), event.pos.getY());
        if (index < 0) return false;
        const auto& d = gf::parameterDescriptors[static_cast<std::size_t>(index)];
        float value = d.boolean ? (values[index] > 0.5f ? 0.0f : 1.0f)
                                : valueFromY(d, event.pos.getY(), index);
        if (d.integer) value = std::round(value);
        editParameter(index, true); setParameterValue(index, value); editParameter(index, false);
        values[index] = value; repaint(); return true;
    }
private:
    std::array<float, gf::parameterCount> values {};
    static float valueFromY(const gf::ParameterDescriptor& d, double y, int index)
    {
        const int row = index <= 0 ? 0 : (index - 1) / 3;
        const float top = index == 0 ? 82.0f : 82.0f + row * 102.0f;
        const float normalized = std::clamp(1.0f - static_cast<float>((y - top) / 82.0), 0.0f, 1.0f);
        return d.minimum + normalized * (d.maximum - d.minimum);
    }
    static int hitTest(double x, double y)
    {
        if (x >= 24 && x <= 144 && y >= 82 && y <= 156) return 0;
        for (int index = 1; index < static_cast<int>(gf::parameterCount); ++index)
        {
            const int column = (index - 1) % 3, row = (index - 1) / 3;
            const double left = 166 + column * 148, top = 82 + row * 102;
            if (x >= left && x <= left + 128 && y >= top && y <= top + 82) return index;
        }
        return -1;
    }
    void drawControl(uint32_t index, float x, float y, float width, float height)
    {
        const auto& d = gf::parameterDescriptors[index];
        const float normalized = (values[index] - d.minimum) / (d.maximum - d.minimum);
        beginPath(); roundedRect(x, y, width, height, 7); fillColor(Color(27, 31, 39)); fill(); closePath();
        beginPath(); roundedRect(x, y + height - 5, width * std::clamp(normalized, 0.0f, 1.0f), 5, 2);
        fillColor(Color(104, 222, 190)); fill(); closePath();
        fontSize(12); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(Color(145, 155, 171));
        text(x + 12, y + 18, d.name, nullptr);
        char value[48];
        if (d.boolean) std::snprintf(value, sizeof(value), "%s", values[index] > 0.5f ? "FROZEN" : "LIVE");
        else if (d.integer) std::snprintf(value, sizeof(value), "%.0f %s", values[index], d.unit);
        else std::snprintf(value, sizeof(value), "%.2f %s", values[index], d.unit);
        fontSize(index == 0 ? 18 : 16); fillColor(values[index] > 0.5f && index == 0 ? Color(104, 222, 190) : Color(238, 241, 246));
        text(x + 12, y + 47, value, nullptr);
    }
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularFreezeUI)
};
UI* createUI() { return new GranularFreezeUI(); }
END_NAMESPACE_DISTRHO
