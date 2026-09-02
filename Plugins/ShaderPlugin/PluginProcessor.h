#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ShaderPlugin
{

// An ordinary JUCE processor — a gain, and the peak that came out of it. It is
// deliberately the least interesting file in the example: the point being made
// is that nothing about the audio side changes when the editor is eacp's.
//
// The one thing worth looking at is getOutputLevel(). It is the whole channel
// between the audio thread and the shader: one relaxed atomic, written once per
// block and read once per rendered frame, with no lock and no allocation on
// either side of it.
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

} // namespace ShaderPlugin
