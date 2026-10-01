#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace WebSynthPlugin
{

// An instrument with the parameters of a small subtractive synth — a waveform,
// a filter, an envelope, an output gain, a mono switch — and no voices behind
// them. This example is about the editor: every one of those parameters is
// drawn, dragged and automated inside a web page, and the page is the only
// thing the editor has in it. A silent synth is still a synth to a host: it is
// registered as an instrument, takes MIDI, and its parameters automate.
//
// What does come back from the audio thread is what the MIDI is doing, so the
// page has something live to show besides the parameters: how many keys are
// held and which one went down last.
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

    bool acceptsMidi() const override { return true; }
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

    // How many keys are down, and the last one pressed (-1 before the first).
    // Written once per block by the audio thread, safe to read from anywhere.
    int getHeldNotes() const noexcept
    {
        return heldNotes.load(std::memory_order_relaxed);
    }

    int getLastNote() const noexcept
    {
        return lastNote.load(std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    // Audio-thread bookkeeping: one flag per MIDI note, so a repeated note-on
    // or a stray note-off cannot push the count out of true.
    std::array<bool, 128> noteIsDown {};

    std::atomic<int> heldNotes {0};
    std::atomic<int> lastNote {-1};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

} // namespace WebSynthPlugin
