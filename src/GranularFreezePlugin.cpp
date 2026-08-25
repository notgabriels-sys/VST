#include "DistrhoPlugin.hpp"
#include "GranularFreezeTelemetry.h"
#include "dsp/GranularFreezeCore.h"
#include <array>

START_NAMESPACE_DISTRHO
class GranularFreezePlugin final : public Plugin,
                                   public gf::GranularFreezeTelemetrySource
{
public:
    GranularFreezePlugin() : Plugin(gf::parameterCount, 0, 0)
    {
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = gf::parameterDescriptors[i].defaultValue;
    }
protected:
    const char* getLabel() const override { return "granularfreeze"; }
    const char* getDescription() const override { return "Deterministic live-input granular freeze effect."; }
    const char* getMaker() const override { return "Gabriel Garcia Alonso"; }
    const char* getHomePage() const override { return "https://github.com/notgabriels-sys/VST"; }
    const char* getLicense() const override { return "MIT"; }
    uint32_t getVersion() const override { return d_version(0, 2, 0); }
    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupStereo;
        Plugin::initAudioPort(input, index, port);
    }
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        const auto& d = gf::parameterDescriptors[index];
        parameter.hints = kParameterIsAutomatable;
        if (d.integer) parameter.hints |= kParameterIsInteger;
        if (d.boolean) parameter.hints |= kParameterIsBoolean;
        parameter.name = d.name;
        parameter.symbol = d.symbol;
        parameter.unit = d.unit;
        parameter.ranges.min = d.minimum;
        parameter.ranges.max = d.maximum;
        parameter.ranges.def = d.defaultValue;
    }
    float getParameterValue(uint32_t index) const override { return values[index]; }
    void setParameterValue(uint32_t index, float value) override { values[index] = value; }
    void activate() override
    {
        core.prepare(getSampleRate(), 16384);
        telemetry.clear();
    }
    void sampleRateChanged(double newRate) override
    {
        core.prepare(newRate, 16384);
        telemetry.clear();
    }
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        const gf::ParameterValues p { values[0], values[1], values[2], values[3],
                                      values[4], values[5], values[6] };
        core.process(inputs, outputs, frames, p);
        publishTelemetry();
    }

    const gf::GranularFreezeTelemetry& granularFreezeTelemetry() const noexcept override
    {
        return telemetry;
    }

private:
    void publishTelemetry() noexcept
    {
        telemetry.activity.store(core.getGrainActivity(), std::memory_order_relaxed);
        telemetry.sequencePhase.store(core.getSequencePhase(), std::memory_order_relaxed);
        telemetry.activeVoices.store(static_cast<uint32_t>(
            core.getActiveVoiceCount()), std::memory_order_relaxed);
        telemetry.launchCount.store(core.getTotalLaunchCount(),
                                    std::memory_order_relaxed);

        const auto& voices = core.getVisualVoiceStates();
        for (std::size_t i = 0; i < gf::visualVoiceCount; ++i)
        {
            const auto& voice = voices[i];
            telemetry.voicePhases[i].store(voice.active ? voice.phase : 0.0f,
                                           std::memory_order_relaxed);
            telemetry.voiceEnvelopes[i].store(voice.active ? voice.envelope : 0.0f,
                                              std::memory_order_relaxed);
        }

        const auto& spectrum = core.getSpectrumLevels();
        for (std::size_t i = 0; i < gf::spectrumBandCount; ++i)
            telemetry.spectrumLevels[i].store(spectrum[i],
                                              std::memory_order_relaxed);
    }

    std::array<float, gf::parameterCount> values {};
    gf::GranularFreezeCore core;
    gf::GranularFreezeTelemetry telemetry;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularFreezePlugin)
};
Plugin* createPlugin() { return new GranularFreezePlugin(); }
END_NAMESPACE_DISTRHO
