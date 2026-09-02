#pragma once

#include "SpectrumAnalyser.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace SpectrumPlugin
{

// A visualizer, so the audio side is a tap and nothing else: the buffer leaves
// processBlock exactly as it arrived, and the only thing the block is used for
// is being pushed to the analyser.
//
// That makes the two parameters below unusual for a plugin — they shape the
// picture rather than the sound, which is what a meter's parameters do. They
// still live in the APVTS, because that is what makes them automatable and
// saved with the session like any other.
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

    // The analyser outlives every editor opened onto this processor, which is
    // what lets the picture pick up where it left off when a host closes the
    // window and reopens it.
    SpectrumAnalyser& getAnalyser() noexcept { return analyser; }

    // The two display parameters, read once per rendered frame. Cached pointers
    // rather than getRawParameterValue by name: that is a lookup, and this is
    // on the path a frame takes.
    float getSensitivityDb() const noexcept
    {
        return sensitivity->load(std::memory_order_relaxed);
    }

    float getFalloffSeconds() const noexcept
    {
        return falloff->load(std::memory_order_relaxed) * 0.001f;
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    SpectrumAnalyser analyser;

    // Declared after apvts, which is what makes looking them up here legal:
    // members are initialised in declaration order, so the tree already holds
    // its parameters by the time these two ask it for them.
    std::atomic<float>* sensitivity = apvts.getRawParameterValue("sensitivity");
    std::atomic<float>* falloff = apvts.getRawParameterValue("falloff");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

} // namespace SpectrumPlugin
