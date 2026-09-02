#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace BlendPlugin
{

// The same ordinary JUCE processor the other examples have — a gain, and the
// peak that came out of it — with one addition: `blend`.
//
// Blend is a real automatable parameter rather than a value the editor keeps to
// itself, and that is the point of it being here. The opacity of the eacp
// surface is plugin state: it survives the editor being closed and reopened,
// the host can automate it, and a preset can carry it. Nothing about being a
// native surface makes it less of a parameter than the gain is.
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
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // The output peak of the last block, 0 to 1. Safe to call from anywhere.
    float getOutputLevel() const noexcept
    {
        return outputLevel.load(std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    std::atomic<float> outputLevel {0.f};
    juce::SmoothedValue<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

} // namespace BlendPlugin
