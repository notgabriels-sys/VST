// Offline behavioural tests for GranularFreeze.
//
// These drive the AudioProcessor directly with generated signals and assert on
// the output. They cover the things a compiler and auval cannot: whether freeze
// actually holds audio, whether the stereo channels stay aligned, and whether
// the crossfade is free of discontinuities.

#include <JuceHeader.h>
#include "../src/PluginProcessor.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <vector>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;

int failures = 0;

void check (bool ok, const juce::String& name, const juce::String& detail = {})
{
    std::printf ("%-52s %s\n", name.toRawUTF8(), ok ? "PASS" : "FAIL");
    if (! ok)
    {
        ++failures;
        if (detail.isNotEmpty())
            std::printf ("      %s\n", detail.toRawUTF8());
    }
}

void setParam (GranularFreezeAudioProcessor& p, const juce::String& id, float value)
{
    if (auto* param = p.apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

float rawParam (const GranularFreezeAudioProcessor& p, const juce::String& id)
{
    if (auto* value = p.apvts.getRawParameterValue (id))
        return value->load();

    return std::numeric_limits<float>::quiet_NaN();
}

bool near (float actual, float expected, float tolerance = 1.0e-5f)
{
    return std::abs (actual - expected) <= tolerance;
}

bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
                return false;

    return true;
}

void processFiniteAudio (GranularFreezeAudioProcessor& processor)
{
    juce::AudioBuffer<float> block (2, kBlockSize);
    juce::MidiBuffer midi;

    for (int sample = 0; sample < kBlockSize; ++sample)
    {
        const auto value = (float) std::sin (juce::MathConstants<double>::twoPi * 220.0
                                               * (double) sample / kSampleRate);
        block.setSample (0, sample, value);
        block.setSample (1, sample, value);
    }

    processor.processBlock (block, midi);
    check (allFinite (block), "state: restored processor output is finite");
}

void copyStateToBinary (const juce::ValueTree& state, juce::MemoryBlock& destination)
{
    if (auto xml = state.createXml())
        juce::AudioProcessor::copyXmlToBinary (*xml, destination);
}

bool sameTree (const juce::ValueTree& left, const juce::ValueTree& right)
{
    return left.isEquivalentTo (right);
}

struct Rig
{
    GranularFreezeAudioProcessor proc;
    juce::AudioBuffer<float> block { 2, kBlockSize };
    juce::MidiBuffer midi;
    double phase = 0.0;

    Rig()
    {
        proc.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
        proc.prepareToPlay (kSampleRate, kBlockSize);
    }

    // Fills the block with a sine on both channels and processes it.
    // Captured output is appended to `out` if provided.
    void run (int numBlocks, double freq, std::vector<float>* outL = nullptr,
              std::vector<float>* outR = nullptr, bool silentInput = false)
    {
        const double inc = juce::MathConstants<double>::twoPi * freq / kSampleRate;

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < kBlockSize; ++i)
            {
                const float s = silentInput ? 0.0f : (float) std::sin (phase);
                block.setSample (0, i, s);
                block.setSample (1, i, s);
                phase += inc;
            }

            midi.clear();
            proc.processBlock (block, midi);

            if (outL != nullptr)
                for (int i = 0; i < kBlockSize; ++i) outL->push_back (block.getSample (0, i));
            if (outR != nullptr)
                for (int i = 0; i < kBlockSize; ++i) outR->push_back (block.getSample (1, i));
        }
    }
};

float maxAbs (const std::vector<float>& v, size_t from = 0)
{
    float m = 0.0f;
    for (size_t i = from; i < v.size(); ++i) m = std::max (m, std::abs (v[i]));
    return m;
}

// Largest sample-to-sample jump -- a click shows up here.
float maxDelta (const std::vector<float>& v, size_t from, size_t to)
{
    float m = 0.0f;
    for (size_t i = std::max<size_t> (from, 1); i < std::min (to, v.size()); ++i)
        m = std::max (m, std::abs (v[i] - v[i - 1]));
    return m;
}

int zeroCrossings (const std::vector<float>& v, size_t from, size_t to)
{
    int n = 0;
    for (size_t i = std::max<size_t> (from, 1); i < std::min (to, v.size()); ++i)
        if ((v[i - 1] < 0.0f) != (v[i] < 0.0f)) ++n;
    return n;
}
} // namespace

int main()
{
    std::printf ("GranularFreeze offline tests @ %.0f Hz / %d frames\n\n", kSampleRate, kBlockSize);

    // ---------------------------------------------------------------- 1
    // This is the host-visible contract. Reordering, changing a legacy range,
    // or changing a new parameter's normalisation would break automation or
    // session restoration even if the DSP continued to run.
    {
        struct ExpectedParameter
        {
            const char* id;
            float minimum;
            float maximum;
            float defaultValue;
            float interval;
        };

        constexpr const char* expectedOrder[] {
            "freeze", "pitch", "crossfadeMs", "grainSizeMs", "densityHz", "position"
        };
        const ExpectedParameter expected[] {
            { "pitch",        0.5f, 2.0f,   1.0f,  0.01f },
            { "crossfadeMs",  1.0f, 500.0f, 30.0f, 1.0f },
            { "grainSizeMs",  5.0f, 200.0f, 80.0f, 1.0f },
            { "densityHz",    0.0f, 200.0f, 20.0f, 1.0f },
            { "position",     0.0f, 1.0f,   1.0f,  0.01f },
        };

        GranularFreezeAudioProcessor processor;
        const auto& parameters = processor.getParameters();
        check (processor.apvts.state.getType().toString() == "PARAMS",
               "parameters: APVTS root is exactly PARAMS");
        check (parameters.size() == (int) std::size (expectedOrder),
               "parameters: exactly six host parameters exist");

        for (int index = 0; index < parameters.size() && index < (int) std::size (expectedOrder); ++index)
        {
            const auto* ranged = dynamic_cast<const juce::RangedAudioParameter*> (parameters[index]);
            check (ranged != nullptr && ranged->getParameterID() == expectedOrder[index],
                   "parameters: exact host parameter order at " + juce::String (index));
        }

        auto* freeze = processor.apvts.getParameter ("freeze");
        check (freeze != nullptr, "parameters: freeze ID remains available");
        check (freeze != nullptr && near (freeze->getDefaultValue(), 0.0f),
               "parameters: freeze default remains off");

        for (const auto& item : expected)
        {
            auto* parameter = processor.apvts.getParameter (item.id);
            const auto name = juce::String ("parameters: ") + item.id;
            check (parameter != nullptr, name + " exists");

            if (parameter != nullptr)
            {
                const auto* floatParameter = dynamic_cast<const juce::AudioParameterFloat*> (parameter);
                check (floatParameter != nullptr, name + " is continuous");

                check (near (parameter->convertFrom0to1 (0.0f), item.minimum), name + " minimum");
                check (near (parameter->convertFrom0to1 (1.0f), item.maximum), name + " maximum");
                check (near (parameter->getDefaultValue(), parameter->convertTo0to1 (item.defaultValue)),
                       name + " default");

                if (floatParameter != nullptr)
                {
                    const auto& range = floatParameter->range;
                    check (near (range.interval, item.interval), name + " step");
                    check (near (range.skew, 1.0f) && ! range.symmetricSkew,
                           name + " linear normalisation");
                }
            }
        }
    }

    // ---------------------------------------------------------------- 2
    // A v0.1 state can contain future data and duplicated PARAM records. The
    // migration is allowed to append only the three missing v0.2 records.
    {
        GranularFreezeAudioProcessor source;
        setParam (source, "freeze", 1.0f);
        setParam (source, "pitch", 1.11f);
        setParam (source, "crossfadeMs", 177.0f);
        auto v01State = source.apvts.copyState();

        for (int index = v01State.getNumChildren(); --index >= 0;)
        {
            const auto id = v01State.getChild (index).getProperty ("id").toString();
            if (id == "grainSizeMs" || id == "densityHz" || id == "position")
                v01State.removeChild (index, nullptr);
        }

        v01State.setProperty ("futureProperty", "preserve-me", nullptr);
        juce::ValueTree unknownSubtree ("FUTURE_SUBTREE");
        unknownSubtree.setProperty ("futureKey", "future-value", nullptr);
        unknownSubtree.addChild (juce::ValueTree ("FUTURE_LEAF"), -1, nullptr);
        v01State.addChild (unknownSubtree, -1, nullptr);

        juce::ValueTree duplicatePitch ("PARAM");
        duplicatePitch.setProperty ("id", "pitch", nullptr);
        duplicatePitch.setProperty ("value", "1.73", nullptr);
        duplicatePitch.setProperty ("duplicateMetadata", "must-stay", nullptr);
        v01State.addChild (duplicatePitch, -1, nullptr);

        juce::MemoryBlock v01Binary;
        copyStateToBinary (v01State, v01Binary);

        const auto verifyMigratedState = [&] (GranularFreezeAudioProcessor& processor, const juce::String& timing)
        {
            const auto restored = processor.apvts.copyState();
            check (near (rawParam (processor, "freeze"), 1.0f), timing + ": preserves v0.1 freeze value");
            check (near (rawParam (processor, "pitch"), 1.73f), timing + ": last duplicate pitch remains active");
            check (near (rawParam (processor, "crossfadeMs"), 177.0f), timing + ": preserves v0.1 crossfade value");
            check (near (rawParam (processor, "grainSizeMs"), 80.0f), timing + ": missing size receives default");
            check (near (rawParam (processor, "densityHz"), 20.0f), timing + ": missing density receives default");
            check (near (rawParam (processor, "position"), 1.0f), timing + ": missing position receives default");
            check (restored.getProperty ("futureProperty").toString() == "preserve-me",
                   timing + ": preserves unknown root property");
            check (restored.getNumChildren() == v01State.getNumChildren() + 3,
                   timing + ": appends exactly three missing PARAM children");

            for (int index = 0; index < v01State.getNumChildren() && index < restored.getNumChildren(); ++index)
                check (sameTree (restored.getChild (index), v01State.getChild (index)),
                       timing + ": preserves child " + juce::String (index) + " byte-equivalently");

            check (restored.getChild (v01State.getNumChildren()).getProperty ("id").toString() == "grainSizeMs",
                   timing + ": appends size after existing children");
            check (restored.getChild (v01State.getNumChildren() + 1).getProperty ("id").toString() == "densityHz",
                   timing + ": appends density after existing children");
            check (restored.getChild (v01State.getNumChildren() + 2).getProperty ("id").toString() == "position",
                   timing + ": appends position after existing children");
        };

        GranularFreezeAudioProcessor restoredBeforePrepare;
        restoredBeforePrepare.setStateInformation (v01Binary.getData(), (int) v01Binary.getSize());
        verifyMigratedState (restoredBeforePrepare, "state before prepareToPlay");
        restoredBeforePrepare.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
        restoredBeforePrepare.prepareToPlay (kSampleRate, kBlockSize);
        processFiniteAudio (restoredBeforePrepare);

        GranularFreezeAudioProcessor restoredAfterPrepare;
        restoredAfterPrepare.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
        restoredAfterPrepare.prepareToPlay (kSampleRate, kBlockSize);
        restoredAfterPrepare.setStateInformation (v01Binary.getData(), (int) v01Binary.getSize());
        verifyMigratedState (restoredAfterPrepare, "state after prepareToPlay");
        processFiniteAudio (restoredAfterPrepare);
    }

    // ---------------------------------------------------------------- 3
    // A current project must restore every parameter value, while malformed or
    // foreign state must leave the current project exactly untouched.
    {
        GranularFreezeAudioProcessor source;
        setParam (source, "freeze", 1.0f);
        setParam (source, "pitch", 1.37f);
        setParam (source, "crossfadeMs", 311.0f);
        setParam (source, "grainSizeMs", 149.0f);
        setParam (source, "densityHz", 73.0f);
        setParam (source, "position", 0.42f);

        juce::MemoryBlock v02Binary;
        source.getStateInformation (v02Binary);

        GranularFreezeAudioProcessor restored;
        restored.setStateInformation (v02Binary.getData(), (int) v02Binary.getSize());
        check (near (rawParam (restored, "freeze"), 1.0f), "state round-trip: freeze");
        check (near (rawParam (restored, "pitch"), 1.37f), "state round-trip: pitch");
        check (near (rawParam (restored, "crossfadeMs"), 311.0f), "state round-trip: crossfade");
        check (near (rawParam (restored, "grainSizeMs"), 149.0f), "state round-trip: size");
        check (near (rawParam (restored, "densityHz"), 73.0f), "state round-trip: density");
        check (near (rawParam (restored, "position"), 0.42f), "state round-trip: position");

        const auto beforeRejectedState = restored.apvts.copyState();
        juce::ValueTree wrongRoot ("WRONG_ROOT");
        wrongRoot.setProperty ("id", "pitch", nullptr);
        juce::MemoryBlock wrongRootBinary;
        copyStateToBinary (wrongRoot, wrongRootBinary);
        restored.setStateInformation (wrongRootBinary.getData(), (int) wrongRootBinary.getSize());
        check (sameTree (restored.apvts.copyState(), beforeRejectedState),
               "state rejects wrong-root XML as exact no-op");

        const std::array<unsigned char, 4> invalidBinary { 0x00, 0x7f, 0xa5, 0xff };
        restored.setStateInformation (invalidBinary.data(), (int) invalidBinary.size());
        check (sameTree (restored.apvts.copyState(), beforeRejectedState),
               "state rejects invalid binary as exact no-op");
    }

    // ---------------------------------------------------------------- 4
    // Unfrozen, the plugin must pass audio through unchanged.
    {
        Rig r;
        setParam (r.proc, "freeze", 0.0f);
        std::vector<float> l, rr;
        r.run (8, 220.0, &l, &rr);
        check (maxAbs (l) > 0.5f, "passthrough: output is not silent",
               "max|L| = " + juce::String (maxAbs (l)));
        check (l == rr, "passthrough: L and R identical for identical input");
    }

    // ---------------------------------------------------------------- 2
    // Identical input on both channels must produce identical output. Any
    // divergence means the channels are reading/writing different buffer
    // offsets -- the stereo desync bug.
    {
        Rig r;
        setParam (r.proc, "freeze", 0.0f);
        r.run (6, 220.0);
        setParam (r.proc, "freeze", 1.0f);
        std::vector<float> l, rr;
        r.run (16, 220.0, &l, &rr, /*silentInput*/ true);

        float worst = 0.0f;
        for (size_t i = 0; i < l.size(); ++i) worst = std::max (worst, std::abs (l[i] - rr[i]));
        check (worst < 1.0e-6f, "frozen: L and R stay aligned",
               "max |L-R| = " + juce::String (worst));
    }

    // ---------------------------------------------------------------- 3
    // Once frozen, silence at the input must NOT silence the output -- the
    // buffer should keep playing back.
    {
        Rig r;
        setParam (r.proc, "freeze", 0.0f);
        r.run (6, 220.0);
        setParam (r.proc, "freeze", 1.0f);
        std::vector<float> l;
        r.run (20, 220.0, &l, nullptr, /*silentInput*/ true);

        // Skip the crossfade region, then measure.
        const size_t tail = l.size() / 2;
        check (maxAbs (l, tail) > 0.3f, "frozen: holds audio when input goes silent",
               "max|L| after crossfade = " + juce::String (maxAbs (l, tail)));
    }

    // ---------------------------------------------------------------- 4
    // The crossfade exists to avoid a click. The largest sample-to-sample jump
    // across the freeze transition should stay close to that of the source
    // signal itself.
    {
        Rig r;
        setParam (r.proc, "freeze", 0.0f);
        setParam (r.proc, "crossfadeMs", 30.0f);
        std::vector<float> l;
        r.run (6, 220.0, &l);
        const float baseline = maxDelta (l, 0, l.size());

        const size_t transitionStart = l.size();
        setParam (r.proc, "freeze", 1.0f);
        r.run (8, 220.0, &l);

        const float atTransition = maxDelta (l, transitionStart, l.size());
        check (atTransition < baseline * 4.0f, "freeze transition: no click",
               "baseline step " + juce::String (baseline) + " vs transition step " + juce::String (atTransition));
    }

    // ---------------------------------------------------------------- 5
    // Unfreezing must also be click-free, and must return to live audio.
    {
        Rig r;
        setParam (r.proc, "freeze", 0.0f);
        setParam (r.proc, "crossfadeMs", 30.0f);
        std::vector<float> l;
        r.run (6, 220.0, &l);
        const float baseline = maxDelta (l, 0, l.size());

        setParam (r.proc, "freeze", 1.0f);
        r.run (8, 220.0, &l);
        const size_t unfreezeStart = l.size();
        setParam (r.proc, "freeze", 0.0f);
        r.run (8, 220.0, &l);

        check (maxDelta (l, unfreezeStart, l.size()) < baseline * 4.0f, "unfreeze transition: no click",
               "step " + juce::String (maxDelta (l, unfreezeStart, l.size())));
    }

    // ---------------------------------------------------------------- 6
    // Pitch 2.0 should roughly double the rate of the frozen playback.
    {
        auto crossingsAtPitch = [] (float pitch)
        {
            Rig r;
            setParam (r.proc, "freeze", 0.0f);
            setParam (r.proc, "pitch", pitch);
            r.run (8, 220.0);
            setParam (r.proc, "freeze", 1.0f);
            std::vector<float> l;
            r.run (24, 220.0, &l, nullptr, true);
            return zeroCrossings (l, l.size() / 2, l.size());
        };

        const int atOne = crossingsAtPitch (1.0f);
        const int atTwo = crossingsAtPitch (2.0f);
        const double ratio = atOne > 0 ? (double) atTwo / (double) atOne : 0.0;
        check (ratio > 1.7 && ratio < 2.3, "pitch 2.0 doubles frozen playback rate",
               "crossings " + juce::String (atOne) + " -> " + juce::String (atTwo)
                   + " (ratio " + juce::String (ratio, 3) + ")");
    }

    // ---------------------------------------------------------------- 7
    // No NaN/Inf anywhere, under freeze, unfreeze and extreme pitch.
    {
        Rig r;
        std::vector<float> l, rr;
        setParam (r.proc, "pitch", 0.5f);
        r.run (4, 220.0, &l, &rr);
        setParam (r.proc, "freeze", 1.0f);
        r.run (8, 220.0, &l, &rr, true);
        setParam (r.proc, "pitch", 2.0f);
        r.run (8, 220.0, &l, &rr, true);
        setParam (r.proc, "freeze", 0.0f);
        r.run (4, 220.0, &l, &rr);

        bool finite = true;
        for (float v : l)  finite = finite && std::isfinite (v);
        for (float v : rr) finite = finite && std::isfinite (v);
        check (finite, "no NaN or Inf across freeze/pitch sweeps");
        check (maxAbs (l) <= 1.5f, "output stays within sane range",
               "max|L| = " + juce::String (maxAbs (l)));
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
