// Offline behavioural tests for GranularFreeze.
//
// These drive the AudioProcessor directly with generated signals and assert on
// the output. They cover the things a compiler and auval cannot: whether freeze
// actually holds audio, whether the stereo channels stay aligned, and whether
// the crossfade is free of discontinuities.

#include "../src/PluginProcessor.h"

#include <cmath>
#include <cstdio>
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

    // Processes an exact number of supplied samples, including a final partial
    // block. This lets boundary tests place a marker at a sample-exact age.
    void runInput (const std::vector<float>& input)
    {
        size_t offset = 0;
        while (offset < input.size())
        {
            const int chunk = static_cast<int> (std::min<size_t> (kBlockSize, input.size() - offset));
            juce::AudioBuffer<float> exactBlock { 2, chunk };
            for (int i = 0; i < chunk; ++i)
            {
                const float sample = input[offset + static_cast<size_t> (i)];
                exactBlock.setSample (0, i, sample);
                exactBlock.setSample (1, i, sample);
            }

            midi.clear();
            proc.processBlock (exactBlock, midi);
            offset += static_cast<size_t> (chunk);
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

    // ---------------------------------------------------------------- 8
    // holdMs decides HOW MUCH of the recent capture freeze holds. Feed a long
    // stretch of low tone, then a short stretch of high tone, then freeze. With
    // a short hold the frozen output must reflect the RECENT (high) material; a
    // long hold spans both and lands lower. Before holdMs existed, freeze always
    // replayed the whole capture starting at its oldest sample, so a short hold
    // was impossible and this test could not pass.
    {
        auto frozenRateForHold = [] (float holdMs)
        {
            Rig r;
            setParam (r.proc, "freeze", 0.0f);
            setParam (r.proc, "crossfadeMs", 5.0f);
            setParam (r.proc, "holdMs", holdMs);
            r.run (40, 200.0);                 // old material, ~0.43 s
            r.run (16, 1600.0);                // recent material, ~0.17 s
            setParam (r.proc, "freeze", 1.0f);
            std::vector<float> l;
            r.run (24, 1600.0, &l, nullptr, /*silentInput*/ true);
            return zeroCrossings (l, l.size() / 2, l.size());
        };

        const int shortHold = frozenRateForHold (80.0f);    // inside the recent tone
        const int longHold  = frozenRateForHold (500.0f);   // spans back into the low tone

        check (shortHold > longHold * 2, "short hold captures recent audio, not the whole buffer",
               "crossings: 80ms hold = " + juce::String (shortHold)
                   + ", 500ms hold = " + juce::String (longHold));
    }

    // ---------------------------------------------------------------- 9
    // Logic and GarageBand may restore AU automation by parameter index. JUCE
    // keeps that index stable only when later parameters use a higher version
    // hint than every parameter from the earlier release. Hold was added after
    // Freeze, Pitch and Crossfade, so it must live in a later generation.
    {
        GranularFreezeAudioProcessor p;
        const auto* freeze    = p.apvts.getParameter ("freeze");
        const auto* pitch     = p.apvts.getParameter ("pitch");
        const auto* crossfade = p.apvts.getParameter ("crossfadeMs");
        const auto* hold      = p.apvts.getParameter ("holdMs");

        const bool stableHints = freeze != nullptr && freeze->getVersionHint() == 1
                              && pitch != nullptr && pitch->getVersionHint() == 1
                              && crossfade != nullptr && crossfade->getVersionHint() == 1
                              && hold != nullptr && hold->getVersionHint() == 2;

        check (stableHints, "AU parameter generations encode compatibility intent",
               "expected legacy hints 1 and Hold hint 2");
    }

    // ---------------------------------------------------------------- 10
    // The public Hold endpoint is exactly ten seconds. Capture a single marker
    // at a known offset from the oldest sample of that window, then measure its
    // repeat period. Even a one-sample-short physical buffer changes the period;
    // the old eight-second buffer discards the marker entirely.
    {
        Rig r;
        setParam (r.proc, "freeze", 0.0f);
        setParam (r.proc, "crossfadeMs", 1.0f);
        setParam (r.proc, "holdMs", 10000.0f);

        constexpr int holdSamples = static_cast<int> (kSampleRate * 10.0);
        constexpr int renderedBlocks = 938;
        constexpr int captureSamples = renderedBlocks * kBlockSize;
        constexpr int loopFadeSamples = static_cast<int> (kSampleRate * 0.001);
        constexpr int markerInWindow = loopFadeSamples + 16;
        constexpr int markerInCapture = captureSamples - holdSamples + markerInWindow;
        static_assert (markerInCapture >= 0 && markerInCapture < captureSamples);

        std::vector<float> capture (captureSamples, 0.0f);
        capture[static_cast<size_t> (markerInCapture)] = 1.0f;
        r.runInput (capture);
        setParam (r.proc, "freeze", 1.0f);

        std::vector<float> l;
        r.run (renderedBlocks, 200.0, &l, nullptr, /*silentInput*/ true);

        std::vector<size_t> markerPositions;
        for (size_t i = 0; i < l.size(); ++i)
            if (std::abs (l[i]) > 0.9f)
                markerPositions.push_back (i);

        const size_t expectedPeriod = static_cast<size_t> (holdSamples - loopFadeSamples);
        const bool exactEndpoint = markerPositions.size() == 2
                                && markerPositions[1] - markerPositions[0] == expectedPeriod;
        const juce::String detail = markerPositions.size() == 2
            ? "measured marker period "
                  + juce::String (static_cast<juce::int64> (markerPositions[1] - markerPositions[0]))
                  + ", expected " + juce::String (static_cast<juce::int64> (expectedPeriod))
            : "detected " + juce::String (static_cast<int> (markerPositions.size()))
                  + " marker samples, expected 2";

        check (exactEndpoint, "ten-second Hold endpoint is physically available", detail);
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
