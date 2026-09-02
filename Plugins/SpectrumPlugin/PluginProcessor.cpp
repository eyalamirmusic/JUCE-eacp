#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace SpectrumPlugin
{

juce::AudioProcessorValueTreeState::ParameterLayout Processor::makeParameters()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout {};

    // Where the top of the picture sits, in dB relative to full scale. A
    // mastering chain peaking near 0dBFS wants none of it; a single quiet
    // track wants a lot.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sensitivity", 1},
        "Sensitivity",
        juce::NormalisableRange<float> {-12.f, 36.f, 0.1f},
        0.f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")));

    // How long a bin takes to fall away from a peak. Short is a twitchy,
    // literal picture; long is the smeared one that makes a mix's shape
    // readable at a glance.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"falloff", 1},
        "Falloff",
        juce::NormalisableRange<float> {40.f, 2000.f, 1.f, 0.4f},
        350.f,
        juce::AudioParameterFloatAttributes {}.withLabel("ms")));

    return layout;
}

Processor::Processor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "state", makeParameters())
{
}

void Processor::prepareToPlay(double sampleRate, int)
{
    analyser.prepare(sampleRate);
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono()
        && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto denormalGuard = juce::ScopedNoDenormals {};

    for (auto channel = getTotalNumInputChannels();
         channel < getTotalNumOutputChannels();
         ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    // The whole audio side of the plugin. The buffer is not touched: what
    // leaves is what arrived, and this is a tap on the way past.
    analyser.pushBlock(buffer);
    buffer.clear();
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

} // namespace SpectrumPlugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrumPlugin::Processor();
}
