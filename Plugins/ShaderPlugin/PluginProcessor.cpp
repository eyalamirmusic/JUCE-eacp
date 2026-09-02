#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ShaderPlugin
{

juce::AudioProcessorValueTreeState::ParameterLayout Processor::makeParameters()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout {};

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"gain", 1},
        "Gain",
        juce::NormalisableRange<float> {0.f, 2.f, 0.f, 0.5f},
        1.f));

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
    gain.reset(sampleRate, 0.02);
    gain.setCurrentAndTargetValue(apvts.getRawParameterValue("gain")->load());

    outputLevel.store(0.f, std::memory_order_relaxed);
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

    gain.setTargetValue(apvts.getRawParameterValue("gain")->load());
    gain.applyGain(buffer, buffer.getNumSamples());

    auto peak = 0.f;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = juce::jmax(peak,
                          buffer.getMagnitude(channel, 0, buffer.getNumSamples()));

    // Relaxed: the editor wants the freshest figure it can get, and nothing
    // else is ordered against it. A missed update costs one frame of animation.
    outputLevel.store(juce::jlimit(0.f, 1.f, peak), std::memory_order_relaxed);
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

} // namespace ShaderPlugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShaderPlugin::Processor();
}
