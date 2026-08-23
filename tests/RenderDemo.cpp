// Renders v0.2 listening cases through the plugin and reports objective
// diagnostics. These WAVs are audition aids, not musical-quality gates.

#include <juce_audio_formats/juce_audio_formats.h>
#include "../src/PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;

struct Case
{
    const char* name;
    float grainSizeMs;
    float densityHz;
    float position;
    float pitch;
    float holdMs;
    float crossfadeMs;
};

constexpr Case renderCases[] {
    { "size-short",        10.0f,  20.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "size-long",        180.0f,  20.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "density-low",       80.0f,   5.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "density-high",      80.0f, 120.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "position-oldest",   80.0f,  20.0f, 0.0f, 1.0f, 1000.0f,  30.0f },
    { "position-middle",   80.0f,  20.0f, 0.5f, 1.0f, 1000.0f,  30.0f },
    { "position-newest",   80.0f,  20.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "hold-short",        80.0f,  20.0f, 0.0f, 1.0f,   80.0f,  30.0f },
    { "hold-long",         80.0f,  20.0f, 0.0f, 1.0f,  400.0f,  30.0f },
    { "pitch-down",        80.0f,  20.0f, 1.0f, 0.5f, 1000.0f,  30.0f },
    { "pitch-unity",       80.0f,  20.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "pitch-up",          80.0f,  20.0f, 1.0f, 2.0f, 1000.0f,  30.0f },
    { "transition-short",  80.0f,  20.0f, 1.0f, 1.0f, 1000.0f,   1.0f },
    { "transition-normal", 80.0f,  20.0f, 1.0f, 1.0f, 1000.0f,  30.0f },
    { "transition-long",   80.0f,  20.0f, 1.0f, 1.0f, 1000.0f, 300.0f },
};

constexpr Case dryReferenceCase {
    "dry-reference", 80.0f, 20.0f, 1.0f, 1.0f, 1000.0f, 30.0f
};

void setParam (GranularFreezeAudioProcessor& processor, const juce::String& id, float value)
{
    if (auto* parameter = processor.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    else
        jassertfalse;
}

void configure (GranularFreezeAudioProcessor& processor, const Case& renderCase, float freeze)
{
    setParam (processor, "freeze", freeze);
    setParam (processor, "pitch", renderCase.pitch);
    setParam (processor, "crossfadeMs", renderCase.crossfadeMs);
    setParam (processor, "holdMs", renderCase.holdMs);
    setParam (processor, "grainSizeMs", renderCase.grainSizeMs);
    setParam (processor, "densityHz", renderCase.densityHz);
    setParam (processor, "position", renderCase.position);
}

// A detuned saw chord through a gentle lowpass is harmonically rich, so grain
// differences are audible rather than hidden by a pure-tone source.
struct Source
{
    double phases[6] {};
    double lowPass[2] {};
    const double frequencies[6] { 110.0, 110.6, 164.81, 165.4, 220.0, 277.18 };

    void next (float& left, float& right)
    {
        double sum = 0.0;
        for (int index = 0; index < 6; ++index)
        {
            phases[index] += frequencies[index] / sampleRate;
            if (phases[index] >= 1.0)
                phases[index] -= 1.0;

            sum += (2.0 * phases[index] - 1.0) * 0.16;
        }

        lowPass[0] += 0.28 * (sum - lowPass[0]);
        lowPass[1] += 0.28 * (lowPass[0] - lowPass[1]);
        left = static_cast<float> (lowPass[1] * 0.9);
        right = left;
    }
};

struct Rendered
{
    std::vector<float> left;
    std::vector<float> right;
    std::vector<float> dryLeft;
    std::vector<float> dryRight;
    size_t freezeOnAt = 0;
    size_t freezeOffAt = 0;
};

void appendProcessedBlocks (GranularFreezeAudioProcessor& processor, Source& source,
                            juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                            int numberOfBlocks, Rendered& output)
{
    for (int block = 0; block < numberOfBlocks; ++block)
    {
        for (int sample = 0; sample < blockSize; ++sample)
        {
            float left = 0.0f;
            float right = 0.0f;
            source.next (left, right);
            buffer.setSample (0, sample, left);
            buffer.setSample (1, sample, right);
            output.dryLeft.push_back (left);
            output.dryRight.push_back (right);
        }

        midi.clear();
        processor.processBlock (buffer, midi);

        for (int sample = 0; sample < blockSize; ++sample)
        {
            output.left.push_back (buffer.getSample (0, sample));
            output.right.push_back (buffer.getSample (1, sample));
        }
    }
}

Rendered renderCase (const Case& caseToRender)
{
    GranularFreezeAudioProcessor processor;
    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);
    configure (processor, caseToRender, 0.0f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    Source source;
    Rendered output;

    appendProcessedBlocks (processor, source, buffer, midi, 40, output);
    output.freezeOnAt = output.left.size();
    setParam (processor, "freeze", 1.0f);
    appendProcessedBlocks (processor, source, buffer, midi, 120, output);
    output.freezeOffAt = output.left.size();
    setParam (processor, "freeze", 0.0f);
    const int requiredTransitionSamples = static_cast<int> (std::ceil (
        static_cast<double> (caseToRender.crossfadeMs) * 0.001 * sampleRate));
    const int transitionBlocks = (requiredTransitionSamples + blockSize - 1) / blockSize;
    const int postUnfreezeBlocks = transitionBlocks + 1;
    appendProcessedBlocks (processor, source, buffer, midi, postUnfreezeBlocks, output);

    return output;
}

Rendered renderDryReference()
{
    GranularFreezeAudioProcessor processor;
    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);
    configure (processor, dryReferenceCase, 0.0f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    Source source;
    Rendered output;
    appendProcessedBlocks (processor, source, buffer, midi, 180, output);
    output.freezeOnAt = output.left.size();
    output.freezeOffAt = output.left.size();
    return output;
}

bool writeWav (const juce::File& file, const Rendered& rendered)
{
    if (rendered.left.size() != rendered.right.size()
        || rendered.left.empty()
        || rendered.left.size() > static_cast<size_t> (std::numeric_limits<int>::max()))
    {
        std::fprintf (stderr, "cannot write %s: invalid stereo render size\n", file.getFullPathName().toRawUTF8());
        return false;
    }

    if (file.exists() && ! file.deleteFile())
    {
        std::fprintf (stderr, "cannot replace %s: delete failed\n", file.getFullPathName().toRawUTF8());
        return false;
    }

    std::unique_ptr<juce::OutputStream> outputStream = file.createOutputStream();
    if (outputStream == nullptr)
    {
        std::fprintf (stderr, "cannot create %s: output stream failed\n", file.getFullPathName().toRawUTF8());
        return false;
    }

    juce::AudioBuffer<float> buffer (2, static_cast<int> (rendered.left.size()));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        buffer.setSample (0, sample, rendered.left[static_cast<size_t> (sample)]);
        buffer.setSample (1, sample, rendered.right[static_cast<size_t> (sample)]);
    }

    juce::WavAudioFormat format;
    const auto options = juce::AudioFormatWriter::Options()
        .withSampleRate (sampleRate)
        .withNumChannels (2)
        .withBitsPerSample (24);

    {
        auto writer = format.createWriterFor (outputStream, options);
        if (writer == nullptr)
        {
            std::fprintf (stderr, "cannot create %s: WAV writer failed\n", file.getFullPathName().toRawUTF8());
            return false;
        }

        if (! writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()))
        {
            std::fprintf (stderr, "cannot write %s: sample write failed\n", file.getFullPathName().toRawUTF8());
            return false;
        }
    }

    if (! file.existsAsFile() || file.getSize() <= 0)
    {
        std::fprintf (stderr, "cannot write %s: output file is empty or absent\n", file.getFullPathName().toRawUTF8());
        return false;
    }

    return true;
}

struct MeasurementRange
{
    size_t start = 0;
    size_t end = 0;
};

MeasurementRange boundedRange (const Rendered& rendered, size_t requestedStart, size_t requestedEnd)
{
    const auto size = std::min (rendered.left.size(), rendered.right.size());
    const auto start = std::min (requestedStart, size);
    return { start, std::min (std::max (requestedEnd, start), size) };
}

struct Measurements
{
    double peak = 0.0;
    double rms = 0.0;
    double dc = 0.0;
    double maximumStep = 0.0;
    double approximateBrightnessHz = 0.0;
    double maximumAbsoluteStereoDifference = 0.0;
    bool finite = true;
};

bool allSamplesFinite (const Rendered& rendered)
{
    if (rendered.left.size() != rendered.right.size())
        return false;

    for (size_t sample = 0; sample < rendered.left.size(); ++sample)
        if (! std::isfinite (rendered.left[sample]) || ! std::isfinite (rendered.right[sample]))
            return false;

    return true;
}

double maximumDryDifference (const Rendered& rendered, size_t start, size_t end)
{
    const auto size = rendered.left.size();
    if (rendered.right.size() != size
        || rendered.dryLeft.size() != size
        || rendered.dryRight.size() != size
        || start > end
        || end > size)
    {
        return std::numeric_limits<double>::infinity();
    }

    double difference = 0.0;
    for (size_t sample = start; sample < end; ++sample)
    {
        difference = std::max (
            difference,
            std::max (std::abs (static_cast<double> (rendered.left[sample])
                                - static_cast<double> (rendered.dryLeft[sample])),
                      std::abs (static_cast<double> (rendered.right[sample])
                                - static_cast<double> (rendered.dryRight[sample]))));
    }
    return difference;
}

Measurements measure (const Rendered& rendered, MeasurementRange range)
{
    Measurements measurements;
    measurements.finite = allSamplesFinite (rendered);
    range = boundedRange (rendered, range.start, range.end);
    const auto numberOfStereoSamples = range.end - range.start;
    if (numberOfStereoSamples == 0)
        return measurements;

    double sumSquares = 0.0;
    double sum = 0.0;
    size_t zeroCrossings = 0;
    size_t zeroCrossingPairs = 0;

    for (size_t sample = range.start; sample < range.end; ++sample)
    {
        const double left = rendered.left[sample];
        const double right = rendered.right[sample];
        measurements.peak = std::max (measurements.peak, std::max (std::abs (left), std::abs (right)));
        sumSquares += left * left + right * right;
        sum += left + right;
        measurements.maximumAbsoluteStereoDifference = std::max (
            measurements.maximumAbsoluteStereoDifference, std::abs (left - right));

        if (sample > range.start)
        {
            const double previousLeft = rendered.left[sample - 1];
            const double previousRight = rendered.right[sample - 1];
            measurements.maximumStep = std::max (measurements.maximumStep,
                                                 std::max (std::abs (left - previousLeft),
                                                           std::abs (right - previousRight)));
            if ((previousLeft < 0.0) != (left < 0.0))
                ++zeroCrossings;
            ++zeroCrossingPairs;
        }
    }

    const auto denominator = static_cast<double> (numberOfStereoSamples);
    measurements.rms = std::sqrt (sumSquares / (2.0 * denominator));
    measurements.dc = sum / (2.0 * denominator);
    measurements.approximateBrightnessHz = zeroCrossingPairs > 0
        ? static_cast<double> (zeroCrossings) / static_cast<double> (zeroCrossingPairs) * sampleRate / 2.0
        : 0.0;
    measurements.finite = measurements.finite
        && std::isfinite (measurements.peak)
        && std::isfinite (measurements.rms)
        && std::isfinite (measurements.dc)
        && std::isfinite (measurements.maximumStep)
        && std::isfinite (measurements.approximateBrightnessHz)
        && std::isfinite (measurements.maximumAbsoluteStereoDifference);
    return measurements;
}

void printMeasurements (const char* name, const char* region,
                        MeasurementRange range, const Measurements& measurements)
{
    std::printf ("%-20s %-14s [%6zu,%6zu) %8.4f %8.4f %9.2e %10.4f %12.0f %7s %10.3e\n",
                 name,
                 region,
                 range.start,
                 range.end,
                 measurements.peak,
                 measurements.rms,
                 measurements.dc,
                 measurements.maximumStep,
                 measurements.approximateBrightnessHz,
                 measurements.finite ? "yes" : "no",
                 measurements.maximumAbsoluteStereoDifference);
}
} // namespace

int main (int argc, char** argv)
{
    const juce::File outputDirectory (argc > 1 ? juce::String (argv[1]) : juce::String ("/tmp/gf-demo"));
    if (! outputDirectory.isDirectory()
        && (! outputDirectory.createDirectory() || ! outputDirectory.isDirectory()))
    {
        std::fprintf (stderr, "cannot create output directory %s\n", outputDirectory.getFullPathName().toRawUTF8());
        return 1;
    }

    std::printf ("measurement regions: dry-reference uses all samples; v0.2 cases use settled frozen samples after two crossfade lengths.\n");
    std::printf ("%-20s %-14s %-15s %8s %8s %9s %10s %12s %7s %10s\n",
                 "case", "region", "samples", "peak", "rms", "dc", "max-step",
                 "brightness-Hz", "finite", "max|L-R|");
    std::printf ("%s\n", juce::String::repeatedString ("-", 138).toRawUTF8());

    const auto dryReference = renderDryReference();
    const auto dryRange = boundedRange (dryReference, 0, dryReference.left.size());
    const auto dryMeasurements = measure (dryReference, dryRange);
    printMeasurements (dryReferenceCase.name, "all", dryRange, dryMeasurements);
    if (! dryMeasurements.finite)
    {
        std::fprintf (stderr, "non-finite rendered sample or measurement in dry-reference\n");
        return 1;
    }

    if (! writeWav (outputDirectory.getChildFile ("dry-reference.wav"), dryReference))
        return 1;

    for (const auto& caseToRender : renderCases)
    {
        const auto rendered = renderCase (caseToRender);
        const auto requiredTransitionSamples = static_cast<size_t> (std::ceil (
            static_cast<double> (caseToRender.crossfadeMs) * 0.001 * sampleRate));
        const auto renderedPostUnfreezeSamples = rendered.left.size() - rendered.freezeOffAt;
        const auto minimumPostUnfreezeSamples = requiredTransitionSamples
                                              + static_cast<size_t> (blockSize);
        if (renderedPostUnfreezeSamples < minimumPostUnfreezeSamples)
        {
            std::fprintf (stderr,
                          "%s: post-Unfreeze render has %zu samples; need at least %zu for the complete transition plus one full live guard block\n",
                          caseToRender.name,
                          renderedPostUnfreezeSamples,
                          minimumPostUnfreezeSamples);
            return 1;
        }
        if (juce::String (caseToRender.name) == "transition-long"
            && renderedPostUnfreezeSamples < static_cast<size_t> (14400 + blockSize))
        {
            std::fprintf (stderr,
                          "transition-long: expected at least 14,400 transition samples plus a 512-sample guard block\n");
            return 1;
        }

        const auto guardStart = rendered.left.size() - static_cast<size_t> (blockSize);
        const double guardDryDifference = maximumDryDifference (
            rendered, guardStart, rendered.left.size());
        constexpr double guardTolerance = 1.0e-7;
        if (! std::isfinite (guardDryDifference) || guardDryDifference > guardTolerance)
        {
            std::fprintf (stderr,
                          "%s: final live guard block differs from corresponding dry input by %.9g (tolerance %.1e)\n",
                          caseToRender.name,
                          guardDryDifference,
                          guardTolerance);
            return 1;
        }
        const auto crossfadeSamples = static_cast<size_t> (std::llround (
            static_cast<double> (caseToRender.crossfadeMs) * 0.001 * sampleRate));
        const auto range = boundedRange (rendered, rendered.freezeOnAt + 2 * crossfadeSamples,
                                         rendered.freezeOffAt);
        const auto measurements = measure (rendered, range);
        printMeasurements (caseToRender.name, "settled-frozen", range, measurements);
        std::printf ("%-20s lifecycle      post=%5zu guard-dry-max=%10.3e\n",
                     caseToRender.name,
                     renderedPostUnfreezeSamples,
                     guardDryDifference);

        if (! measurements.finite)
        {
            std::fprintf (stderr, "non-finite rendered sample or measurement in %s\n", caseToRender.name);
            return 1;
        }

        if (! writeWav (outputDirectory.getChildFile (juce::String (caseToRender.name) + ".wav"), rendered))
            return 1;
    }

    std::printf ("wrote 16 stereo 48 kHz 24-bit WAV files to %s\n",
                 outputDirectory.getFullPathName().toRawUTF8());
    return 0;
}
