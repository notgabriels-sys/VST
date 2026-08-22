// Renders demo audio through the plugin and reports objective measurements.
//
// The offline test suite answers yes/no questions. This produces actual audio
// files to listen to, plus numbers that describe what the freeze is doing:
// loop-seam continuity, DC offset, peak/RMS, and spectral centroid drift.

#include <juce_audio_formats/juce_audio_formats.h>
#include "../src/PluginProcessor.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
constexpr double SR = 48000.0;
constexpr int    BS = 512;

void setParam (GranularFreezeAudioProcessor& p, const juce::String& id, float v)
{
    if (auto* q = p.apvts.getParameter (id)) q->setValueNotifyingHost (q->convertTo0to1 (v));
}

// A detuned saw chord through a gentle lowpass -- harmonically rich, so freeze
// artefacts and loop seams are audible rather than hidden by a pure tone.
struct Source
{
    double ph[6] {};
    double lp[2] {};
    const double freqs[6] { 110.0, 110.6, 164.81, 165.4, 220.0, 277.18 };

    double elapsed = 0.0;

    void next (float& l, float& r)
    {
        // Step the whole chord up two semitones every second. Without a source
        // that changes over time, every hold length would hold the same chord
        // and the parameter would be inaudible.
        elapsed += 1.0 / SR;
        const int step = (int) std::floor (elapsed);
        const double transpose = std::pow (2.0, (step * 2) / 12.0);

        double sum = 0.0;
        for (int i = 0; i < 6; ++i)
        {
            ph[i] += (freqs[i] * transpose) / SR;
            if (ph[i] >= 1.0) ph[i] -= 1.0;
            sum += (2.0 * ph[i] - 1.0) * 0.16;          // saw
        }
        lp[0] += 0.28 * (sum - lp[0]);                   // one-pole LP
        lp[1] += 0.28 * (lp[0] - lp[1]);
        l = (float) (lp[1] * 0.9);
        r = (float) (lp[1] * 0.9);
    }
};

struct Rendered
{
    std::vector<float> l, r;
    std::vector<int> freezeOnAt, freezeOffAt;
};

Rendered render (const juce::String& label, float pitch, float crossfadeMs, float holdMs,
                 int preBlocks, int frozenBlocks, int postBlocks)
{
    GranularFreezeAudioProcessor p;
    p.setPlayConfigDetails (2, 2, SR, BS);
    p.prepareToPlay (SR, BS);
    setParam (p, "pitch", pitch);
    setParam (p, "crossfadeMs", crossfadeMs);
    setParam (p, "holdMs", holdMs);
    setParam (p, "freeze", 0.0f);

    juce::AudioBuffer<float> buf (2, BS);
    juce::MidiBuffer midi;
    Source src;
    Rendered out;

    auto runBlocks = [&] (int n, bool silentIn)
    {
        for (int b = 0; b < n; ++b)
        {
            for (int i = 0; i < BS; ++i)
            {
                float l = 0.0f, r = 0.0f;
                if (! silentIn) src.next (l, r);
                buf.setSample (0, i, l);
                buf.setSample (1, i, r);
            }
            midi.clear();
            p.processBlock (buf, midi);
            for (int i = 0; i < BS; ++i)
            {
                out.l.push_back (buf.getSample (0, i));
                out.r.push_back (buf.getSample (1, i));
            }
        }
    };

    runBlocks (preBlocks, false);
    out.freezeOnAt.push_back ((int) out.l.size());
    setParam (p, "freeze", 1.0f);
    runBlocks (frozenBlocks, false);          // source keeps running underneath
    out.freezeOffAt.push_back ((int) out.l.size());
    setParam (p, "freeze", 0.0f);
    runBlocks (postBlocks, false);

    juce::ignoreUnused (label);
    return out;
}

void writeWav (const juce::File& f, const Rendered& r)
{
    juce::AudioBuffer<float> b (2, (int) r.l.size());
    for (int i = 0; i < (int) r.l.size(); ++i)
    {
        b.setSample (0, i, r.l[(size_t) i]);
        b.setSample (1, i, r.r[(size_t) i]);
    }
    f.deleteFile();
    juce::WavAudioFormat fmt;
    if (auto* os = f.createOutputStream().release())
    {
        std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (os, SR, 2, 24, {}, 0));
        if (w != nullptr) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

double rms (const std::vector<float>& v, size_t a, size_t b)
{
    double s = 0; size_t n = 0;
    for (size_t i = a; i < std::min (b, v.size()); ++i) { s += (double) v[i] * v[i]; ++n; }
    return n ? std::sqrt (s / (double) n) : 0.0;
}
double peak (const std::vector<float>& v, size_t a, size_t b)
{
    double m = 0;
    for (size_t i = a; i < std::min (b, v.size()); ++i) m = std::max (m, (double) std::abs (v[i]));
    return m;
}
double dc (const std::vector<float>& v, size_t a, size_t b)
{
    double s = 0; size_t n = 0;
    for (size_t i = a; i < std::min (b, v.size()); ++i) { s += v[i]; ++n; }
    return n ? s / (double) n : 0.0;
}
double maxStep (const std::vector<float>& v, size_t a, size_t b)
{
    double m = 0;
    for (size_t i = std::max<size_t> (a, 1); i < std::min (b, v.size()); ++i)
        m = std::max (m, (double) std::abs (v[i] - v[i - 1]));
    return m;
}
// Crude spectral centroid via zero-crossing rate -- enough to show whether the
// frozen tail keeps the same brightness as the source.
double zcr (const std::vector<float>& v, size_t a, size_t b)
{
    size_t n = 0, cross = 0;
    for (size_t i = std::max<size_t> (a, 1); i < std::min (b, v.size()); ++i)
    { if ((v[i-1] < 0) != (v[i] < 0)) ++cross; ++n; }
    return n ? (double) cross / (double) n * SR / 2.0 : 0.0;
}
} // namespace

int main (int argc, char** argv)
{
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::String ("/tmp/gf-demo"));
    outDir.createDirectory();

    struct Case { const char* name; float pitch; float xfade; float hold; };
    const Case cases[] {
        // Hold-length sweep. The source steps up two semitones per second, so a
        // short hold locks onto the last step while a long hold loops back
        // through several of them.
        { "hold-0100ms",        1.0f,  30.0f,   100.0f },
        { "hold-0250ms",        1.0f,  30.0f,   250.0f },
        { "hold-0500ms",        1.0f,  30.0f,   500.0f },
        { "hold-1000ms-default",1.0f,  30.0f,  1000.0f },
        { "hold-2000ms",        1.0f,  30.0f,  2000.0f },
        { "hold-4000ms",        1.0f,  30.0f,  4000.0f },
        // Pitch and crossfade behaviour at the default hold.
        { "freeze-octave-up",   2.0f,  30.0f,  1000.0f },
        { "freeze-octave-down", 0.5f,  30.0f,  1000.0f },
        { "freeze-short-xfade", 1.0f,   1.0f,  1000.0f },
    };

    std::printf ("%-22s %8s %8s %9s %10s %9s %9s\n",
                 "case", "peak", "rms", "dc", "maxstep", "zcr_src", "zcr_frz");
    std::printf ("%s\n", juce::String::repeatedString ("-", 78).toRawUTF8());

    // Dry reference row first, so the frozen numbers have a baseline to sit
    // against -- a maxstep only means something relative to the source's own.
    {
        GranularFreezeAudioProcessor p;
        p.setPlayConfigDetails (2, 2, SR, BS);
        p.prepareToPlay (SR, BS);
        setParam (p, "freeze", 0.0f);
        juce::AudioBuffer<float> buf (2, BS); juce::MidiBuffer m; Source src;
        std::vector<float> d;
        for (int b = 0; b < 500; ++b)
        {
            for (int i = 0; i < BS; ++i) { float l, r; src.next (l, r); buf.setSample (0,i,l); buf.setSample (1,i,r); }
            m.clear(); p.processBlock (buf, m);
            for (int i = 0; i < BS; ++i) d.push_back (buf.getSample (0, i));
        }
        std::printf ("%-22s %8.4f %8.4f %9.2e %10.4f %9.0f %9s\n", "dry-reference",
                     peak (d, 0, d.size()), rms (d, 0, d.size()), dc (d, 0, d.size()),
                     maxStep (d, 0, d.size()), zcr (d, 0, d.size()), "-");
    }

    for (const auto& c : cases)
    {
        // 400 blocks live (~4.27 s captured), 200 frozen, 20 back to live.
        auto r = render (c.name, c.pitch, c.xfade, c.hold, 400, 200, 20);
        const size_t on  = (size_t) r.freezeOnAt[0];
        const size_t off = (size_t) r.freezeOffAt[0];
        const size_t xf  = (size_t) std::round (c.xfade * 0.001 * SR);

        // measure the settled frozen region, past the crossfade
        const size_t fa = std::min (on + xf * 2, off), fb = off;

        std::printf ("%-22s %8.4f %8.4f %9.2e %10.4f %9.0f %9.0f\n",
                     c.name, peak (r.l, fa, fb), rms (r.l, fa, fb), dc (r.l, fa, fb),
                     maxStep (r.l, fa, fb), zcr (r.l, 0, on), zcr (r.l, fa, fb));

        writeWav (outDir.getChildFile (juce::String (c.name) + ".wav"), r);
    }

    // A dry reference so the frozen files can be compared against the source.
    {
        GranularFreezeAudioProcessor p;
        p.setPlayConfigDetails (2, 2, SR, BS);
        p.prepareToPlay (SR, BS);
        setParam (p, "freeze", 0.0f);
        juce::AudioBuffer<float> buf (2, BS); juce::MidiBuffer m; Source src; Rendered dry;
        for (int b = 0; b < 500; ++b)
        {
            for (int i = 0; i < BS; ++i) { float l, r; src.next (l, r); buf.setSample (0,i,l); buf.setSample (1,i,r); }
            m.clear(); p.processBlock (buf, m);
            for (int i = 0; i < BS; ++i) { dry.l.push_back (buf.getSample (0,i)); dry.r.push_back (buf.getSample (1,i)); }
        }
        writeWav (outDir.getChildFile ("dry-reference.wav"), dry);
    }

    std::printf ("\nwrote wavs to %s\n", outDir.getFullPathName().toRawUTF8());
    return 0;
}
