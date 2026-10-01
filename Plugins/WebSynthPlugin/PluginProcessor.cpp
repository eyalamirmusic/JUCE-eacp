#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

namespace WebSynthPlugin
{
namespace
{
// The readouts the page shows are the parameters' own text, the same strings a
// host prints in its automation lanes. Formatting them here rather than in
// JavaScript keeps one definition of what "2.4 kHz" means.
juce::String formatHertz(float hz, int)
{
    if (hz >= 1000.f)
        return juce::String(hz / 1000.f, hz >= 10000.f ? 1 : 2) + " kHz";

    return juce::String(juce::roundToInt(hz)) + " Hz";
}

juce::String formatMilliseconds(float ms, int)
{
    if (ms >= 1000.f)
        return juce::String(ms / 1000.f, 2) + " s";

    return juce::String(juce::roundToInt(ms)) + " ms";
}

juce::String formatPercent(float amount, int)
{
    return juce::String(juce::roundToInt(amount * 100.f)) + "%";
}

juce::String formatDecibels(float dB, int)
{
    return juce::String(dB, 1) + " dB";
}

juce::NormalisableRange<float> skewedRange(float start, float end, float centre)
{
    auto range = juce::NormalisableRange<float> {start, end};
    range.setSkewForCentre(centre);
    return range;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout Processor::makeParameters()
{
    using Attributes = juce::AudioParameterFloatAttributes;

    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout {};

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {"waveform", 1},
        "Waveform",
        juce::StringArray {"Sine", "Triangle", "Saw", "Square"},
        2));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"cutoff", 1},
        "Cutoff",
        skewedRange(20.f, 20000.f, 1000.f),
        2400.f,
        Attributes {}.withStringFromValueFunction(formatHertz)));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"resonance", 1},
        "Resonance",
        juce::NormalisableRange<float> {0.f, 1.f},
        0.2f,
        Attributes {}.withStringFromValueFunction(formatPercent)));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"attack", 1},
        "Attack",
        skewedRange(1.f, 2000.f, 100.f),
        10.f,
        Attributes {}.withStringFromValueFunction(formatMilliseconds)));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"release", 1},
        "Release",
        skewedRange(5.f, 5000.f, 400.f),
        400.f,
        Attributes {}.withStringFromValueFunction(formatMilliseconds)));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"gain", 1},
        "Gain",
        juce::NormalisableRange<float> {-48.f, 6.f},
        -6.f,
        Attributes {}.withStringFromValueFunction(formatDecibels)));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID {"mono", 1}, "Mono", false));

    return layout;
}

Processor::Processor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "state", makeParameters())
{
}

void Processor::prepareToPlay(double, int)
{
    noteIsDown.fill(false);
    heldNotes.store(0, std::memory_order_relaxed);
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    return out == juce::AudioChannelSet::mono()
           || out == juce::AudioChannelSet::stereo();
}

void Processor::processBlock(juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midiMessages)
{
    auto denormalGuard = juce::ScopedNoDenormals {};

    // No voices: the output is silence, whatever the parameters say.
    buffer.clear();

    auto latest = lastNote.load(std::memory_order_relaxed);

    for (const auto metadata: midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            latest = message.getNoteNumber();
            noteIsDown[static_cast<size_t>(latest)] = true;
        }
        else if (message.isNoteOff())
        {
            noteIsDown[static_cast<size_t>(message.getNoteNumber())] = false;
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            noteIsDown.fill(false);
        }
    }

    const auto held = std::count(noteIsDown.begin(), noteIsDown.end(), true);

    // Relaxed, as in the other examples: the editor wants the freshest figure
    // it can get, and nothing else is ordered against it.
    heldNotes.store(static_cast<int>(held), std::memory_order_relaxed);
    lastNote.store(latest, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* Processor::createEditor()
{
    return new Editor(*this);
}

void Processor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void Processor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*state));
}

} // namespace WebSynthPlugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WebSynthPlugin::Processor();
}
