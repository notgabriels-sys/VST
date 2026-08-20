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

void configureAndPrepare (GranularFreezeAudioProcessor& processor)
{
    processor.setPlayConfigDetails (2, 2, kSampleRate, kBlockSize);
    processor.prepareToPlay (kSampleRate, kBlockSize);
}

std::vector<float> renderPrototypeFreezeTail (GranularFreezeAudioProcessor& processor)
{
    juce::AudioBuffer<float> block (2, kBlockSize);
    juce::MidiBuffer midi;
    std::vector<float> tail;
    double phase = 0.0;
    const auto phaseIncrement = juce::MathConstants<double>::twoPi * 220.0 / kSampleRate;

    setParam (processor, "freeze", 0.0f);
    for (int blockIndex = 0; blockIndex < 8; ++blockIndex)
    {
        for (int sample = 0; sample < kBlockSize; ++sample)
        {
            const auto value = (float) std::sin (phase);
            block.setSample (0, sample, value);
            block.setSample (1, sample, value);
            phase += phaseIncrement;
        }

        midi.clear();
        processor.processBlock (block, midi);
    }

    setParam (processor, "freeze", 1.0f);
    for (int blockIndex = 0; blockIndex < 36; ++blockIndex)
    {
        block.clear();
        midi.clear();
        processor.processBlock (block, midi);

        if (blockIndex >= 20)
            for (int sample = 0; sample < kBlockSize; ++sample)
                tail.push_back (block.getSample (0, sample));
    }

    return tail;
}

float maxDifference (const std::vector<float>& left, const std::vector<float>& right)
{
    if (left.size() != right.size())
        return std::numeric_limits<float>::infinity();

    float difference = 0.0f;
    for (size_t index = 0; index < left.size(); ++index)
        difference = std::max (difference, std::abs (left[index] - right[index]));

    return difference;
}

float maxDifference (const std::vector<float>& left,
                     const std::vector<float>& right,
                     size_t from)
{
    if (left.size() != right.size() || from > left.size())
        return std::numeric_limits<float>::infinity();

    float difference = 0.0f;
    for (size_t index = from; index < left.size(); ++index)
        difference = std::max (difference, std::abs (left[index] - right[index]));
    return difference;
}

struct Rig
{
    GranularFreezeAudioProcessor proc;
    double sampleRate;
    int preparedBlockSize;
    juce::AudioBuffer<float> block;
    juce::MidiBuffer midi;
    double phase = 0.0;

    explicit Rig (double newSampleRate = kSampleRate,
                  int newPreparedBlockSize = kBlockSize)
        : sampleRate (newSampleRate),
          preparedBlockSize (newPreparedBlockSize),
          block (2, newPreparedBlockSize)
    {
        proc.setPlayConfigDetails (2, 2, sampleRate, preparedBlockSize);
        proc.prepareToPlay (sampleRate, preparedBlockSize);
    }

    // Fills the block with a sine on both channels and processes it.
    // Captured output is appended to `out` if provided.
    void run (int numBlocks, double freq, std::vector<float>* outL = nullptr,
              std::vector<float>* outR = nullptr, bool silentInput = false)
    {
        const double inc = juce::MathConstants<double>::twoPi * freq / sampleRate;

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < preparedBlockSize; ++i)
            {
                const float s = silentInput ? 0.0f : (float) std::sin (phase);
                block.setSample (0, i, s);
                block.setSample (1, i, s);
                phase += inc;
            }

            midi.clear();
            proc.processBlock (block, midi);

            if (outL != nullptr)
                for (int i = 0; i < preparedBlockSize; ++i) outL->push_back (block.getSample (0, i));
            if (outR != nullptr)
                for (int i = 0; i < preparedBlockSize; ++i) outR->push_back (block.getSample (1, i));
        }
    }

    void processBuffer (juce::AudioBuffer<float>& audio,
                        std::vector<float>* outL = nullptr,
                        std::vector<float>* outR = nullptr)
    {
        midi.clear();
        proc.processBlock (audio, midi);

        if (outL != nullptr)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                outL->push_back (audio.getSample (0, sample));
        if (outR != nullptr)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                outR->push_back (audio.getSample (1, sample));
    }

    void runConstant (int numSamples, float left, float right,
                      std::vector<float>* outL = nullptr,
                      std::vector<float>* outR = nullptr)
    {
        juce::AudioBuffer<float> audio (2, numSamples);
        audio.clear();
        for (int sample = 0; sample < numSamples; ++sample)
        {
            audio.setSample (0, sample, left);
            audio.setSample (1, sample, right);
        }
        processBuffer (audio, outL, outR);
    }

    void runSine (int numSamples, double frequency,
                  std::vector<float>* outL = nullptr,
                  std::vector<float>* outR = nullptr,
                  bool silentInput = false)
    {
        juce::AudioBuffer<float> audio (2, numSamples);
        const double increment = juce::MathConstants<double>::twoPi
                               * frequency / sampleRate;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float value = silentInput ? 0.0f : (float) std::sin (phase);
            audio.setSample (0, sample, value);
            audio.setSample (1, sample, value);
            phase += increment;
        }
        processBuffer (audio, outL, outR);
    }

    void runSegmented (const std::vector<std::array<float, 3>>& segments,
                       std::vector<float>* outL = nullptr,
                       std::vector<float>* outR = nullptr)
    {
        int totalSamples = 0;
        for (const auto& segment : segments)
            totalSamples += (int) segment[0];

        juce::AudioBuffer<float> audio (2, totalSamples);
        int destination = 0;
        for (const auto& segment : segments)
        {
            const int segmentSamples = (int) segment[0];
            for (int sample = 0; sample < segmentSamples; ++sample)
            {
                audio.setSample (0, destination, segment[1]);
                audio.setSample (1, destination, segment[2]);
                ++destination;
            }
        }
        processBuffer (audio, outL, outR);
    }

    void runArbitrary (const std::vector<float>& left,
                       const std::vector<float>& right,
                       int hostBlockSize,
                       std::vector<float>* outL = nullptr,
                       std::vector<float>* outR = nullptr)
    {
        if (left.size() != right.size())
        {
            check (false, "rig: arbitrary stereo input lengths match");
            return;
        }

        const int boundedHostBlockSize = juce::jmax (1, hostBlockSize);
        for (size_t offset = 0; offset < left.size(); offset += (size_t) boundedHostBlockSize)
        {
            const int chunkSamples = (int) std::min (
                (size_t) boundedHostBlockSize, left.size() - offset);
            juce::AudioBuffer<float> audio (2, chunkSamples);
            for (int sample = 0; sample < chunkSamples; ++sample)
            {
                audio.setSample (0, sample, left[offset + (size_t) sample]);
                audio.setSample (1, sample, right[offset + (size_t) sample]);
            }
            processBuffer (audio, outL, outR);
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

float mean (const std::vector<float>& values, size_t from = 0)
{
    if (from >= values.size())
        return 0.0f;

    double sum = 0.0;
    for (size_t index = from; index < values.size(); ++index)
        sum += values[index];
    return (float) (sum / (double) (values.size() - from));
}

bool allFinite (const std::vector<float>& values)
{
    for (const auto value : values)
        if (! std::isfinite (value))
            return false;
    return true;
}

float maxStereoDifference (const std::vector<float>& left,
                           const std::vector<float>& right,
                           float rightScale = 1.0f)
{
    if (left.size() != right.size())
        return std::numeric_limits<float>::infinity();

    float difference = 0.0f;
    for (size_t index = 0; index < left.size(); ++index)
        difference = std::max (
            difference, std::abs (right[index] - left[index] * rightScale));
    return difference;
}

void checkSequence (const std::vector<float>& actual,
                    std::initializer_list<float> expected,
                    const juce::String& name)
{
    bool matches = actual.size() == expected.size();
    size_t index = 0;
    for (const auto value : expected)
    {
        matches = matches && index < actual.size() && near (actual[index], value);
        ++index;
    }

    juce::String detail = "actual [";
    for (size_t sample = 0; sample < actual.size(); ++sample)
    {
        if (sample != 0)
            detail += ", ";
        detail += juce::String (actual[sample], 6);
    }
    detail += "]";
    check (matches, name, detail);
}

float hannAt (int index, int length)
{
    return 0.5f - 0.5f * std::cos (
        juce::MathConstants<float>::twoPi * (float) index / (float) (length - 1));
}

enum class RawClassification
{
    finiteBelowRange,
    finiteAboveRange,
    nan,
    positiveInfinity,
    negativeInfinity
};

const char* rawClassificationName (RawClassification classification)
{
    switch (classification)
    {
        case RawClassification::finiteBelowRange: return "finite below range";
        case RawClassification::finiteAboveRange: return "finite above range";
        case RawClassification::nan: return "NaN";
        case RawClassification::positiveInfinity: return "+Inf";
        case RawClassification::negativeInfinity: return "-Inf";
    }
    return "invalid classification";
}

bool matchesRawClassification (float value, float minimum, float maximum,
                               RawClassification classification)
{
    switch (classification)
    {
        case RawClassification::finiteBelowRange:
            return std::isfinite (value) && value < minimum;
        case RawClassification::finiteAboveRange:
            return std::isfinite (value) && value > maximum;
        case RawClassification::nan:
            return std::isnan (value);
        case RawClassification::positiveInfinity:
            return std::isinf (value) && value > 0.0f;
        case RawClassification::negativeInfinity:
            return std::isinf (value) && value < 0.0f;
    }
    return false;
}

void storeRawParameter (GranularFreezeAudioProcessor& processor,
                        const char* parameterId,
                        float value,
                        float minimum,
                        float maximum,
                        RawClassification classification)
{
    const auto caseName = juce::String (parameterId) + " "
                        + rawClassificationName (classification);
    check (matchesRawClassification (value, minimum, maximum, classification),
           "raw injection: classification is " + caseName);

    auto* raw = processor.apvts.getRawParameterValue (parameterId);
    check (raw != nullptr, "raw injection: pointer exists for " + caseName);
    if (raw != nullptr)
        raw->store (value);
}

std::vector<float> renderParameterBehaviour (
    const char* testedParameter,
    float value,
    bool injectRaw,
    RawClassification classification = RawClassification::finiteBelowRange)
{
    constexpr double sampleRate = 1000.0;
    Rig rig (sampleRate, 64);
    setParam (rig.proc, "freeze", 0.0f);
    setParam (rig.proc, "pitch", 1.0f);
    setParam (rig.proc, "crossfadeMs", 30.0f);
    setParam (rig.proc, "grainSizeMs", 80.0f);
    setParam (rig.proc, "densityHz", 20.0f);
    setParam (rig.proc, "position", 1.0f);

    std::vector<float> captureLeft (512);
    std::vector<float> captureRight (512);
    for (size_t sample = 0; sample < captureLeft.size(); ++sample)
    {
        const float polarity = sample < captureLeft.size() / 2 ? -1.0f : 1.0f;
        const float detail = 0.18f * (float) std::sin (
            juce::MathConstants<double>::twoPi * 37.0 * (double) sample / sampleRate);
        captureLeft[sample] = polarity * 0.62f + detail;
        captureRight[sample] = captureLeft[sample];
    }
    rig.runArbitrary (captureLeft, captureRight, 64);

    float minimum = 0.0f;
    float maximum = 1.0f;
    if (juce::String (testedParameter) == "pitch")
    {
        minimum = 0.5f;
        maximum = 2.0f;
    }
    else if (juce::String (testedParameter) == "crossfadeMs")
    {
        minimum = 1.0f;
        maximum = 500.0f;
    }
    else if (juce::String (testedParameter) == "grainSizeMs")
    {
        minimum = 5.0f;
        maximum = 200.0f;
    }
    else if (juce::String (testedParameter) == "densityHz")
    {
        minimum = 0.0f;
        maximum = 200.0f;
    }

    if (injectRaw)
        storeRawParameter (rig.proc, testedParameter, value,
                           minimum, maximum, classification);
    else
        setParam (rig.proc, testedParameter, value);

    if (juce::String (testedParameter) != "freeze")
        setParam (rig.proc, "freeze", 1.0f);

    std::vector<float> continuationLeft (1200);
    std::vector<float> continuationRight (1200);
    for (size_t sample = 0; sample < continuationLeft.size(); ++sample)
    {
        const float input = 0.23f + 0.17f * (float) std::sin (
            juce::MathConstants<double>::twoPi * 13.0 * (double) sample / sampleRate);
        continuationLeft[sample] = input;
        continuationRight[sample] = input;
    }

    std::vector<float> output;
    rig.runArbitrary (continuationLeft, continuationRight, 64, &output);
    return output;
}

std::vector<float> renderEndpointLifecycle (float position,
                                            bool zeroDistanceReversal,
                                            std::vector<float>* noOpOutput = nullptr)
{
    Rig rig (4000.0, 64);
    setParam (rig.proc, "crossfadeMs", 1.0f);
    setParam (rig.proc, "grainSizeMs", 5.0f);
    setParam (rig.proc, "densityHz", 20.0f);
    setParam (rig.proc, "pitch", 1.0f);
    setParam (rig.proc, "position", position);
    rig.runConstant (40, -0.4f, -0.4f);

    setParam (rig.proc, "freeze", 1.0f);
    if (zeroDistanceReversal)
    {
        juce::AudioBuffer<float> zeroSampleBlock (2, 0);
        rig.processBuffer (zeroSampleBlock);
        setParam (rig.proc, "freeze", 0.0f);
        rig.runConstant (1, 0.9f, 0.9f, noOpOutput);
    }
    else
    {
        rig.runConstant (24, 0.0f, 0.0f);
        setParam (rig.proc, "freeze", 0.0f);
        const std::vector<float> fadeInput { 0.2f, 0.3f, 1.0f };
        rig.runArbitrary (fadeInput, fadeInput, 3);
    }

    rig.runConstant (5, 0.6f, 0.6f);
    setParam (rig.proc, "freeze", 1.0f);
    std::vector<float> frozenOutput;
    rig.runConstant (20, 0.0f, 0.0f, &frozenOutput);
    return frozenOutput;
}

std::array<std::vector<float>, 2> renderPositionSegments (float position)
{
    Rig rig (4000.0, 64);
    setParam (rig.proc, "crossfadeMs", 1.0f);
    setParam (rig.proc, "grainSizeMs", 20.0f);
    setParam (rig.proc, "densityHz", 100.0f);
    setParam (rig.proc, "position", position);
    rig.runSegmented ({ { 256.0f, -0.75f, -0.75f },
                        { 256.0f,  0.75f,  0.75f } });
    setParam (rig.proc, "freeze", 1.0f);

    std::array<std::vector<float>, 2> output;
    rig.runConstant (1024, 0.0f, 0.0f, &output[0], &output[1]);
    return output;
}

std::array<std::vector<float>, 2> renderPartitionedFreeze (
    int preparedBlockSize, int hostBlockSize)
{
    constexpr double sampleRate = 48000.0;
    Rig rig (sampleRate, preparedBlockSize);
    setParam (rig.proc, "crossfadeMs", 1.0f);
    setParam (rig.proc, "grainSizeMs", 20.0f);
    setParam (rig.proc, "densityHz", 100.0f);
    setParam (rig.proc, "position", 0.7f);
    setParam (rig.proc, "pitch", 1.0f);

    std::vector<float> captureLeft (2048);
    std::vector<float> captureRight (2048);
    for (size_t sample = 0; sample < captureLeft.size(); ++sample)
    {
        const double absoluteSample = (double) sample;
        captureLeft[sample] = 0.62f * (float) std::sin (
            juce::MathConstants<double>::twoPi * 317.0 * absoluteSample / sampleRate)
            + 0.21f * (float) std::sin (
                juce::MathConstants<double>::twoPi * 811.0 * absoluteSample / sampleRate);
        captureRight[sample] = -0.5f * captureLeft[sample];
    }
    rig.runArbitrary (captureLeft, captureRight, hostBlockSize);

    setParam (rig.proc, "freeze", 1.0f);
    std::vector<float> silence (4096, 0.0f);
    std::array<std::vector<float>, 2> output;
    rig.runArbitrary (silence, silence, hostBlockSize, &output[0], &output[1]);
    return output;
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
        const auto* freezeBool = dynamic_cast<const juce::AudioParameterBool*> (freeze);
        check (freezeBool != nullptr, "parameters: freeze remains AudioParameterBool");
        check (freeze != nullptr && freeze->getNumSteps() == 2,
               "parameters: freeze has exactly two host steps");
        check (freeze != nullptr && near (freeze->convertFrom0to1 (0.0f), 0.0f)
               && near (freeze->convertFrom0to1 (1.0f), 1.0f),
               "parameters: freeze normalised endpoints convert exactly to 0 and 1");
        check (freeze != nullptr && near (freeze->convertTo0to1 (0.0f), 0.0f)
               && near (freeze->convertTo0to1 (1.0f), 1.0f),
               "parameters: freeze raw endpoints convert exactly to 0 and 1");

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
        configureAndPrepare (restoredBeforePrepare);
        verifyMigratedState (restoredBeforePrepare, "state before prepareToPlay after prepareToPlay");
        const auto beforePrepareMigratedState = restoredBeforePrepare.apvts.copyState();
        processFiniteAudio (restoredBeforePrepare);

        GranularFreezeAudioProcessor restoredAfterPrepare;
        configureAndPrepare (restoredAfterPrepare);
        restoredAfterPrepare.setStateInformation (v01Binary.getData(), (int) v01Binary.getSize());
        verifyMigratedState (restoredAfterPrepare, "state after prepareToPlay");
        const auto afterPrepareMigratedState = restoredAfterPrepare.apvts.copyState();
        check (sameTree (beforePrepareMigratedState, afterPrepareMigratedState),
               "state timing: complete migrated state matches before and after prepareToPlay");
        processFiniteAudio (restoredAfterPrepare);

        configureAndPrepare (restoredBeforePrepare);
        configureAndPrepare (restoredAfterPrepare);
        const auto beforePrepareFreezeTail = renderPrototypeFreezeTail (restoredBeforePrepare);
        const auto afterPrepareFreezeTail = renderPrototypeFreezeTail (restoredAfterPrepare);
        check (maxAbs (beforePrepareFreezeTail) > 0.3f,
               "state timing: restored legacy freeze playback is meaningfully non-silent");
        check (beforePrepareFreezeTail == afterPrepareFreezeTail,
               "state timing: restored legacy freeze playback is sample-identical across restore timing");

        GranularFreezeAudioProcessor defaultControl;
        configureAndPrepare (defaultControl);
        const auto defaultFreezeTail = renderPrototypeFreezeTail (defaultControl);
        check (maxDifference (beforePrepareFreezeTail, defaultFreezeTail) > 0.1f,
               "state timing: restored legacy pitch and crossfade differ from default control");
    }

    // ---------------------------------------------------------------- 3
    // Non-PARAM future subtrees can use current parameter IDs. They must never
    // become APVTS adapters or gain a value property during restoration.
    {
        GranularFreezeAudioProcessor legacySource;
        auto noSizeParameterState = legacySource.apvts.copyState();
        for (int index = noSizeParameterState.getNumChildren(); --index >= 0;)
        {
            const auto id = noSizeParameterState.getChild (index).getProperty ("id").toString();
            if (id == "grainSizeMs" || id == "densityHz" || id == "position")
                noSizeParameterState.removeChild (index, nullptr);
        }

        juce::ValueTree noSizeCollision ("FUTURE_SIZE");
        noSizeCollision.setProperty ("id", "grainSizeMs", nullptr);
        noSizeCollision.setProperty ("value", "13", nullptr);
        noSizeCollision.setProperty ("futureMetadata", "keep-me", nullptr);
        noSizeCollision.addChild (juce::ValueTree ("FUTURE_LEAF"), -1, nullptr);
        const auto noSizeCollisionExpected = noSizeCollision.createCopy();
        noSizeParameterState.addChild (noSizeCollision, -1, nullptr);
        const auto noSizeCollisionIndex = noSizeParameterState.getNumChildren() - 1;

        juce::MemoryBlock noSizeCollisionBinary;
        copyStateToBinary (noSizeParameterState, noSizeCollisionBinary);
        GranularFreezeAudioProcessor restoredMissingSize;
        restoredMissingSize.setStateInformation (noSizeCollisionBinary.getData(), (int) noSizeCollisionBinary.getSize());
        const auto afterMissingSizeCopy = restoredMissingSize.apvts.copyState();
        check (near (rawParam (restoredMissingSize, "grainSizeMs"), 80.0f),
               "state collision: absent PARAM size appends and restores the real default");
        check (afterMissingSizeCopy.getChild (noSizeCollisionIndex).hasType ("FUTURE_SIZE")
               && sameTree (afterMissingSizeCopy.getChild (noSizeCollisionIndex), noSizeCollisionExpected),
               "state collision: non-PARAM size collision stays semantically identical after copyState");

        GranularFreezeAudioProcessor currentSource;
        setParam (currentSource, "grainSizeMs", 123.0f);
        auto realSizeState = currentSource.apvts.copyState();
        juce::ValueTree realSizeCollision ("FUTURE_SIZE");
        realSizeCollision.setProperty ("id", "grainSizeMs", nullptr);
        realSizeCollision.setProperty ("value", "13", nullptr);
        realSizeCollision.setProperty ("futureMetadata", "do-not-bind", nullptr);
        realSizeCollision.addChild (juce::ValueTree ("FUTURE_LEAF"), -1, nullptr);
        const auto realSizeCollisionExpected = realSizeCollision.createCopy();
        realSizeState.addChild (realSizeCollision, -1, nullptr);
        const auto realSizeCollisionIndex = realSizeState.getNumChildren() - 1;

        juce::MemoryBlock realSizeCollisionBinary;
        copyStateToBinary (realSizeState, realSizeCollisionBinary);
        GranularFreezeAudioProcessor restoredRealSize;
        restoredRealSize.setStateInformation (realSizeCollisionBinary.getData(), (int) realSizeCollisionBinary.getSize());
        const auto afterRealSizeCopy = restoredRealSize.apvts.copyState();
        const auto afterSecondRealSizeCopy = restoredRealSize.apvts.copyState();
        check (near (rawParam (restoredRealSize, "grainSizeMs"), 123.0f),
               "state collision: last real PARAM size remains active after later non-PARAM collision");
        check (afterRealSizeCopy.getChild (realSizeCollisionIndex).hasType ("FUTURE_SIZE")
               && sameTree (afterRealSizeCopy.getChild (realSizeCollisionIndex), realSizeCollisionExpected),
               "state collision: later non-PARAM child retains exact type and properties");
        check (sameTree (afterSecondRealSizeCopy.getChild (realSizeCollisionIndex), realSizeCollisionExpected),
               "state collision: repeated copyState does not mutate later non-PARAM child");
    }

    // ---------------------------------------------------------------- 4
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

        const size_t transitionEnd = transitionStart
                                   + (size_t) std::round (30.0 * 0.001 * kSampleRate);
        const float insideTransition = maxDelta (l, transitionStart, transitionEnd);
        const float afterTransition = maxDelta (l, transitionEnd, l.size());
        check (insideTransition < baseline * 4.0f, "freeze transition: no click",
               "baseline " + juce::String (baseline)
                   + ", inside " + juce::String (insideTransition)
                   + ", after " + juce::String (afterTransition));
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

    // ------------------------------------------------------- Grain processor
    // Position must map to chronology, not physical circular-buffer indices.
    // Both controls are signal-bearing so a dry or silent implementation fails.
    {
        const auto oldPosition = renderPositionSegments (0.0f);
        const auto recentPosition = renderPositionSegments (1.0f);
        const float oldPositionMean = mean (oldPosition[0], 256);
        const float recentPositionMean = mean (recentPosition[0], 256);
        check (recentPositionMean > 0.10f,
               "processor: position one freezes newest positive segment",
               "mean = " + juce::String (recentPositionMean));
        check (oldPositionMean < -0.10f,
               "processor: position zero freezes oldest negative segment",
               "mean = " + juce::String (oldPositionMean));
        check (maxAbs (recentPosition[0], 256) > 0.10f
               && maxAbs (oldPosition[0], 256) > 0.10f,
               "processor: position controls render meaningful frozen signal");
    }

    // One GrainEngine timeline must read both channels. A scaled stereo source
    // must preserve that exact relationship while remaining meaningfully wet.
    const auto prepared64Partition = renderPartitionedFreeze (64, 64);
    {
        check (maxAbs (prepared64Partition[0], 1024) > 0.05f,
               "processor: shared stereo timeline is meaningfully non-silent");
        check (maxStereoDifference (prepared64Partition[0],
                                    prepared64Partition[1], -0.5f) < 1.0e-6f,
               "processor: grain timeline is shared across stereo",
               "max scaled stereo error = "
                   + juce::String (maxStereoDifference (
                       prepared64Partition[0], prepared64Partition[1], -0.5f)));
    }

    // Empty and very short snapshots must be safe. The short case also has to
    // produce held signal, which rules out treating every short view as empty.
    {
        Rig empty (4000.0, 64);
        setParam (empty.proc, "crossfadeMs", 1.0f);
        setParam (empty.proc, "grainSizeMs", 5.0f);
        setParam (empty.proc, "densityHz", 20.0f);
        setParam (empty.proc, "freeze", 1.0f);
        std::vector<float> emptyFreezeOutput;
        empty.runConstant (64, 1.0f, 1.0f, &emptyFreezeOutput);
        check (allFinite (emptyFreezeOutput),
               "processor: freeze before capture is finite");
        check (maxAbs (emptyFreezeOutput, 4) == 0.0f,
               "processor: empty capture settles to silence");

        Rig shortCapture (4000.0, 64);
        setParam (shortCapture.proc, "crossfadeMs", 1.0f);
        setParam (shortCapture.proc, "grainSizeMs", 5.0f);
        setParam (shortCapture.proc, "densityHz", 20.0f);
        const std::vector<float> threeSamples { 0.7f, -0.4f, 0.9f };
        shortCapture.runArbitrary (threeSamples, threeSamples, 3);
        setParam (shortCapture.proc, "freeze", 1.0f);
        std::vector<float> shortCaptureOutput;
        shortCapture.runConstant (64, 0.0f, 0.0f, &shortCaptureOutput);
        check (allFinite (shortCaptureOutput),
               "processor: short capture wraps safely");
        check (maxAbs (shortCaptureOutput, 4) > 0.10f,
               "processor: short capture remains meaningfully audible",
               "max = " + juce::String (maxAbs (shortCaptureOutput, 4)));
    }

    // Exact transition convention. Empty capture makes wet exactly zero, so
    // output from constant-one input is the dry coefficient itself.
    {
        Rig n4 (4000.0, 64);
        setParam (n4.proc, "crossfadeMs", 1.0f);
        setParam (n4.proc, "freeze", 1.0f);
        std::vector<float> n4Dry;
        n4.runConstant (4, 1.0f, 1.0f, &n4Dry);
        checkSequence (n4Dry, { 1.0f, 0.75f, 0.25f, 0.0f },
                       "transition: live-to-wet N=4 exact dry coefficients");

        Rig reverse (4000.0, 64);
        setParam (reverse.proc, "crossfadeMs", 1.0f);
        setParam (reverse.proc, "freeze", 1.0f);
        reverse.runConstant (2, 1.0f, 1.0f);
        setParam (reverse.proc, "freeze", 0.0f);
        std::vector<float> reverseDry;
        reverse.runConstant (3, 1.0f, 1.0f, &reverseDry);
        checkSequence (reverseDry, { 0.25f, 0.625f, 1.0f },
                       "transition: wet .75-to-live N=3 exact dry coefficients");

        Rig n2 (2000.0, 64);
        setParam (n2.proc, "crossfadeMs", 1.0f);
        setParam (n2.proc, "freeze", 1.0f);
        std::vector<float> n2Dry;
        n2.runConstant (2, 1.0f, 1.0f, &n2Dry);
        checkSequence (n2Dry, { 1.0f, 0.0f },
                       "transition: live-to-wet N=2 exact dry coefficients");
    }

    // The sample whose post-sample advance reaches fully live is the first
    // recaptured sample. Hand-derived Hann points prove no earlier fade sample
    // entered the immutable view, and no endpoint or continuation was skipped.
    {
        const auto oldest = renderEndpointLifecycle (0.0f, false);
        const auto newest = renderEndpointLifecycle (1.0f, false);
        check (oldest.size() == 20 && near (oldest[10], -0.4f * hannAt (10, 20)),
               "capture lifecycle: position zero retains old pre-freeze chronology");
        check (newest.size() == 20
               && near (newest[12], -0.4f * hannAt (12, 20))
               && near (newest[13], -0.4f * hannAt (13, 20)),
               "capture lifecycle: fade-to-live samples do not mutate frozen capture");
        check (newest.size() == 20
               && near (newest[14], 1.0f * hannAt (14, 20))
               && near (newest[15], 0.6f * hannAt (15, 20)),
               "capture lifecycle: exact live endpoint and continuation are chronological");
        check (maxAbs (newest) > 0.10f,
               "capture lifecycle: endpoint proof is meaningfully non-silent");
    }

    // A zero-distance reversal is still a one-sample lifecycle. It retains the
    // snapshot through that sample, reaches live only post-sample, and captures
    // that exact sample at the existing chronological write position.
    {
        std::vector<float> noOpOutput;
        const auto newest = renderEndpointLifecycle (1.0f, true, &noOpOutput);
        checkSequence (noOpOutput, { 0.9f },
                       "transition: zero-distance reversal emits one exact dry sample");
        check (newest.size() == 20
               && near (newest[13], -0.4f * hannAt (13, 20))
               && near (newest[14], 0.9f * hannAt (14, 20))
               && near (newest[15], 0.6f * hannAt (15, 20)),
               "transition: zero-distance reversal completes then captures without reset drift");
    }

    // Reversing before fully live must preserve both the immutable view and
    // active GrainEngine voices. Once the distance-scaled return reaches wet,
    // it must rejoin an always-frozen control at the same engine timeline.
    {
        Rig reversed (4000.0, 64);
        Rig control (4000.0, 64);
        for (auto* rig : { &reversed, &control })
        {
            setParam (rig->proc, "crossfadeMs", 10.0f);
            setParam (rig->proc, "grainSizeMs", 80.0f);
            setParam (rig->proc, "densityHz", 20.0f);
            rig->runSine (512, 133.0);
            setParam (rig->proc, "freeze", 1.0f);
            rig->runConstant (80, 0.0f, 0.0f);
        }

        setParam (reversed.proc, "freeze", 0.0f);
        std::vector<float> reversalLead;
        reversed.runConstant (10, 0.0f, 0.0f, &reversalLead);
        control.runConstant (10, 0.0f, 0.0f);
        setParam (reversed.proc, "freeze", 1.0f);

        std::vector<float> reversedOutput;
        std::vector<float> controlOutput;
        reversed.runConstant (128, 0.0f, 0.0f, &reversedOutput);
        control.runConstant (128, 0.0f, 0.0f, &controlOutput);
        const float rapidToggleStep = maxDelta (reversedOutput, 0, reversedOutput.size());
        const float dryBaselineStep = maxDelta (controlOutput, 0, controlOutput.size());
        check (rapidToggleStep < dryBaselineStep * 4.0f,
               "processor: transition reversal has no endpoint jump",
               juce::String (rapidToggleStep) + " vs " + juce::String (dryBaselineStep));
        check (maxDifference (reversedOutput, controlOutput, 8) < 1.0e-6f,
               "processor: rapid reversal preserves active voices and frozen timeline");
        check (maxAbs (reversedOutput, 8) > 0.05f,
               "processor: rapid reversal settles to meaningful wet signal");
    }

    // Every direct atomic injection proves its own classification and pointer,
    // then compares sample-for-sample with the exact clamp/fallback control.
    {
        struct RawContract
        {
            const char* id;
            float minimum;
            float maximum;
            float below;
            float above;
            float fallback;
        };

        const RawContract contracts[] {
            { "freeze",       0.0f, 1.0f,   -1.0f,   2.0f,   0.0f },
            { "pitch",        0.5f, 2.0f,    0.0f,   3.0f,   1.0f },
            { "crossfadeMs",  1.0f, 500.0f, 0.0f, 600.0f,  30.0f },
            { "grainSizeMs",  5.0f, 200.0f, 0.0f, 250.0f,  80.0f },
            { "densityHz",    0.0f, 200.0f,-10.0f, 250.0f,  20.0f },
            { "position",     0.0f, 1.0f,   -1.0f,   2.0f,   1.0f },
        };

        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float positiveInfinity = std::numeric_limits<float>::infinity();
        const float negativeInfinity = -std::numeric_limits<float>::infinity();

        for (const auto& contract : contracts)
        {
            const auto minimumControl = renderParameterBehaviour (
                contract.id, contract.minimum, false);
            const auto maximumControl = renderParameterBehaviour (
                contract.id, contract.maximum, false);
            const auto fallbackControl = renderParameterBehaviour (
                contract.id, contract.fallback, false);

            check (maxDifference (minimumControl, maximumControl) > 1.0e-3f,
                   juce::String ("raw boundary controls differ meaningfully: ") + contract.id);

            struct InvalidCase
            {
                float raw;
                RawClassification classification;
                const std::vector<float>* expected;
            };
            const InvalidCase cases[] {
                { contract.below, RawClassification::finiteBelowRange, &minimumControl },
                { contract.above, RawClassification::finiteAboveRange, &maximumControl },
                { nan, RawClassification::nan, &fallbackControl },
                { positiveInfinity, RawClassification::positiveInfinity, &fallbackControl },
                { negativeInfinity, RawClassification::negativeInfinity, &fallbackControl },
            };

            for (const auto& invalid : cases)
            {
                const auto actual = renderParameterBehaviour (
                    contract.id, invalid.raw, true, invalid.classification);
                const auto caseName = juce::String (contract.id) + " "
                                    + rawClassificationName (invalid.classification);
                check (allFinite (actual),
                       "raw DSP boundary stays finite: " + caseName);
                check (actual == *invalid.expected,
                       "raw DSP boundary equals exact clamp/fallback: " + caseName,
                       "max difference = "
                           + juce::String (maxDifference (actual, *invalid.expected)));
            }

            if (juce::String (contract.id) == "freeze")
            {
                const auto freezeNan = renderParameterBehaviour (
                    "freeze", nan, true, RawClassification::nan);
                check (freezeNan == fallbackControl && maxAbs (fallbackControl) > 0.30f,
                       "raw freeze NaN is directly equivalent to explicit live pass-through");
            }
        }
    }

    // Density zero is an exact settled silence contract, paired with the same
    // captured signal at non-zero density to prove the snapshot is not empty.
    {
        const auto densityZero = renderParameterBehaviour ("densityHz", 0.0f, false);
        const auto densityTwenty = renderParameterBehaviour ("densityHz", 20.0f, false);
        check (maxAbs (densityZero, 600) == 0.0f,
               "processor: density zero launches no voices and settles to silence");
        check (maxAbs (densityTwenty, 600) > 0.10f,
               "processor: density control proves captured signal is meaningful");
    }

    // Prepared chunk size and host partition must not alter the settled engine
    // timeline. The comparison excludes the transition but requires wet signal.
    {
        const auto prepared512Partition = renderPartitionedFreeze (512, 512);
        const float partitionDifference = maxDifference (
            prepared64Partition[0], prepared512Partition[0], 1024);
        check (maxAbs (prepared512Partition[0], 1024) > 0.05f,
               "processor: partition comparison uses meaningful non-silent output");
        check (partitionDifference < 1.0e-5f,
               "processor: static output is block-partition invariant",
               "settled difference = " + juce::String (partitionDifference));

        const auto oversized = renderPartitionedFreeze (64, 2048);
        check (allFinite (oversized[0]) && allFinite (oversized[1]),
               "processor: oversized block is chunked safely");
        check (maxAbs (oversized[0], 1024) > 0.05f,
               "processor: oversized block renders meaningful wet output");
        check (maxDifference (prepared64Partition[0], oversized[0], 1024) < 1.0e-5f,
               "processor: oversized block preserves exact settled timeline");
        check (maxStereoDifference (oversized[0], oversized[1], -0.5f) < 1.0e-6f,
               "processor: oversized stereo host block preserves shared timeline");
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
