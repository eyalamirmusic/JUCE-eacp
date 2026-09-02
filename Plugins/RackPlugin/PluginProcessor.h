#pragma once

#include "Modules.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <vector>

namespace RackPlugin
{

// A fixed-time echo, one per channel, and deliberately the smallest thing that
// still makes a tail: no interpolation, because nothing modulates the delay
// time, and no filtering in the loop, because the picture is what this module
// is here to feed.
class Echo
{
public:
    // Allocates, so it belongs in prepareToPlay and nowhere near process().
    void prepare(double sampleRate, double seconds)
    {
        const auto length = juce::jmax(1, juce::roundToInt(sampleRate * seconds));
        line.assign((size_t) length, 0.f);
        position = 0;
    }

    void reset() noexcept { std::fill(line.begin(), line.end(), 0.f); }

    // Returns what went in `seconds` ago, having written this sample plus that
    // much of the old one back into the line.
    float process(float input, float feedback) noexcept
    {
        const auto delayed = line[(size_t) position];

        line[(size_t) position] = input + delayed * feedback;
        position = position + 1 < (int) line.size() ? position + 1 : 0;

        return delayed;
    }

private:
    // One zero rather than nothing, so process() is safe on a processor that
    // has not been prepared — which is a state a host can put a plugin in.
    std::vector<float> line = std::vector<float>(1, 0.f);
    int position = 0;
};

//==============================================================================
// Four effects in a row, and four meters that measure four different things.
//
// That last part is the reason this processor is worth reading, where the two
// sibling examples' deliberately are not. A visualizer publishes a level, and a
// level is the same quantity wherever it is taken from; a rack of modules has
// something specific to say about each one, and saying it is what makes four
// pictures rather than one picture repeated:
//
//   Drive  — saturation. How far the shaper has bent the loudest sample of the
//            block away from a straight line. 0 is clean, 1 is a square wave.
//            Not a level: a quiet passage driven hard reads high, a loud clean
//            one reads zero.
//   Tone   — balance. Where the energy sits after the filter, low to high, on
//            a +/-24dB window. A position, not a size.
//   Space  — tail. The peak of the delay's own output, so the figure outlives
//            the input by exactly as long as the repeats do.
//   Width  — correlation between the two output channels, mapped so 1 is mono,
//            0.5 is decorrelated and 0 is phase-inverted. The one reading here
//            that is a warning rather than a reading.
//
// All four cross to the editor the same way the sibling examples' single figure
// does: one relaxed atomic each, written once per block by the audio thread and
// read once per rendered frame by a view's update(). No lock, no allocation,
// and nothing on the audio thread that can block.
class Processor final : public juce::AudioProcessor
{
public:
    Processor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // What a module's shader is lit by, 0 to 1. Safe to call from anywhere;
    // see the table above for what each one means.
    float getEnergy(ModuleId id) const noexcept
    {
        return energy[indexOf(id)].load(std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    static std::size_t indexOf(ModuleId id) { return (std::size_t) id; }

    // One per module, in chain order. Each publishes its own figure, including
    // when it is bypassed and has nothing to say.
    void processDrive(juce::AudioBuffer<float>& buffer) noexcept;
    void processTone(juce::AudioBuffer<float>& buffer) noexcept;
    void processSpace(juce::AudioBuffer<float>& buffer) noexcept;
    void processWidth(juce::AudioBuffer<float>& buffer) noexcept;

    void publish(ModuleId id, float value) noexcept
    {
        energy[indexOf(id)].store(juce::jlimit(0.f, 1.f, value),
                                  std::memory_order_relaxed);
    }

    bool isEnabled(ModuleId id) const noexcept
    {
        return enables[indexOf(id)]->load(std::memory_order_relaxed) > 0.5f;
    }

    std::array<std::atomic<float>, moduleCount> energy {};

    // Declared after apvts, which is what makes looking these up here legal:
    // members are initialised in declaration order, so the tree already holds
    // its parameters by the time these ask it for them. Cached pointers rather
    // than getRawParameterValue by name, because that is a lookup and this is
    // on the path a block takes.
    std::atomic<float>* drive = apvts.getRawParameterValue("drive");
    std::atomic<float>* tone = apvts.getRawParameterValue("tone");
    std::atomic<float>* space = apvts.getRawParameterValue("space");
    std::atomic<float>* width = apvts.getRawParameterValue("width");
    std::atomic<float>* output = apvts.getRawParameterValue("output");

    // Filled from the spec table in the constructor, so the four bypasses are
    // reachable by module rather than by name.
    std::array<std::atomic<float>*, moduleCount> enables {};

    juce::SmoothedValue<float> drivePre;
    juce::SmoothedValue<float> lowGain;
    juce::SmoothedValue<float> highGain;
    juce::SmoothedValue<float> spaceMix;
    juce::SmoothedValue<float> widthAmount;
    juce::SmoothedValue<float> outputGain;

    // The tone module's one-pole split, one state per channel.
    std::array<float, 2> toneState {};
    float toneCoefficient = 0.f;

    std::array<Echo, 2> echoes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

} // namespace RackPlugin
