#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace RackPlugin
{
namespace
{
// Where the tone module splits low from high. High enough to be above the body
// of most material, low enough that "high" still contains something on a bass
// part.
constexpr auto crossoverHz = 700.0;

// The window the balance reading spans, either side of level. Wider than this
// and real material never leaves the middle; narrower and it pins at the ends.
constexpr auto balanceWindowDb = 48.f;

// Two times rather than one, and prime-ish against each other, so the repeats
// interleave instead of arriving together and reading as one mono echo.
constexpr auto leftDelaySeconds = 0.26;
constexpr auto rightDelaySeconds = 0.337;

// Smoothing for every parameter that reaches a sample. 20ms is short enough to
// feel immediate on a knob and long enough that automation does not step.
constexpr auto smoothingSeconds = 0.02;
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout Processor::makeParameters()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout {};

    // How hard the signal is pushed into the shaper. Also a gain, and honestly
    // so: a drive control is a level into a nonlinearity, and the makeup below
    // it only takes the peak back, not the loudness.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"drive", 1},
        "Drive",
        juce::NormalisableRange<float> {0.f, 24.f, 0.1f},
        6.f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")));

    // A tilt rather than a filter: negative leans the whole band into the low
    // half, positive into the high one, and the middle is a straight wire.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"tone", 1},
        "Tone",
        juce::NormalisableRange<float> {-100.f, 100.f, 1.f},
        0.f,
        juce::AudioParameterFloatAttributes {}.withLabel("%")));

    // One knob for both the wet level and the feedback, which is what makes it
    // a size control rather than two controls that have to be set together.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"space", 1},
        "Space",
        juce::NormalisableRange<float> {0.f, 100.f, 1.f},
        25.f,
        juce::AudioParameterFloatAttributes {}.withLabel("%")));

    // 0 is mono, 100 is untouched, 200 is twice the side signal — which is the
    // setting that can put a mix out of phase, and the reason this module's
    // meter reads correlation.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"width", 1},
        "Width",
        juce::NormalisableRange<float> {0.f, 200.f, 1.f},
        100.f,
        juce::AudioParameterFloatAttributes {}.withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"output", 1},
        "Output",
        juce::NormalisableRange<float> {-24.f, 12.f, 0.1f},
        0.f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")));

    // The four bypasses, from the same table the editor builds its panels from,
    // so a module's two parameters cannot drift apart from its panel.
    for (const auto& spec: moduleSpecs)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID {spec.enableId, 1},
            juce::String {spec.title} + " on",
            true));

    return layout;
}

Processor::Processor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "state", makeParameters())
{
    for (auto index = 0; index < moduleCount; ++index)
        enables[(std::size_t) index] =
            apvts.getRawParameterValue(moduleSpecs[index].enableId);
}

void Processor::prepareToPlay(double sampleRate, int)
{
    for (auto* smoothed:
         {&drivePre, &lowGain, &highGain, &spaceMix, &widthAmount, &outputGain})
        smoothed->reset(sampleRate, smoothingSeconds);

    drivePre.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(drive->load(std::memory_order_relaxed)));
    outputGain.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(output->load(std::memory_order_relaxed)));

    // The one-pole's coefficient, from the time constant rather than from a
    // bilinear transform: at this crossover the difference is inaudible, and
    // this is one line.
    toneCoefficient = (float) (1.0
                               - std::exp(-2.0 * juce::MathConstants<double>::pi
                                          * crossoverHz / sampleRate));
    toneState.fill(0.f);

    echoes[0].prepare(sampleRate, leftDelaySeconds);
    echoes[1].prepare(sampleRate, rightDelaySeconds);

    for (auto& value: energy)
        value.store(0.f, std::memory_order_relaxed);
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

    processDrive(buffer);
    processTone(buffer);
    processSpace(buffer);
    processWidth(buffer);

    outputGain.setTargetValue(
        juce::Decibels::decibelsToGain(output->load(std::memory_order_relaxed)));
    outputGain.applyGain(buffer, buffer.getNumSamples());
}

void Processor::processDrive(juce::AudioBuffer<float>& buffer) noexcept
{
    if (!isEnabled(ModuleId::drive))
    {
        // Nothing bent anything: the figure a bypassed shaper reports is zero,
        // and the panel greys its picture out besides.
        publish(ModuleId::drive, 0.f);
        return;
    }

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = juce::jmin(2, buffer.getNumChannels());
    auto* const* channels = buffer.getArrayOfWritePointers();

    const auto target =
        juce::Decibels::decibelsToGain(drive->load(std::memory_order_relaxed));

    // The peak going *in*, taken before anything is written, because the figure
    // this module publishes is about what the curve did to it.
    auto peak = 0.f;

    for (auto channel = 0; channel < numChannels; ++channel)
        peak = juce::jmax(peak, buffer.getMagnitude(channel, 0, numSamples));

    drivePre.setTargetValue(target);

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto pre = drivePre.getNextValue();

        // Makeup, so the shaper is not also a fader: a sample at full scale
        // comes back out at full scale whatever the drive is. Everything below
        // full scale still gets louder, which is what a drive control does.
        const auto makeup = 1.f / std::tanh(pre);

        for (auto channel = 0; channel < numChannels; ++channel)
            channels[channel][sample] =
                std::tanh(channels[channel][sample] * pre) * makeup;
    }

    // How far the curve bent the loudest sample away from the straight line it
    // starts out as. tanh(x)/x is 1 for a signal the shaper is not touching and
    // falls towards 0 as it squares off.
    const auto driven = peak * target;
    const auto saturation =
        driven > 1.0e-5f ? 1.f - std::tanh(driven) / driven : 0.f;

    publish(ModuleId::drive, saturation);
}

void Processor::processTone(juce::AudioBuffer<float>& buffer) noexcept
{
    if (!isEnabled(ModuleId::tone))
    {
        // A position, so its idle reading is the middle rather than zero.
        publish(ModuleId::tone, 0.5f);
        return;
    }

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = juce::jmin(2, buffer.getNumChannels());
    auto* const* channels = buffer.getArrayOfWritePointers();

    const auto tilt = tone->load(std::memory_order_relaxed) * 0.01f;

    lowGain.setTargetValue(1.f - tilt);
    highGain.setTargetValue(1.f + tilt);

    auto lowEnergy = 0.0;
    auto highEnergy = 0.0;

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto lowScale = lowGain.getNextValue();
        const auto highScale = highGain.getNextValue();

        for (auto channel = 0; channel < numChannels; ++channel)
        {
            auto& state = toneState[(std::size_t) channel];
            const auto input = channels[channel][sample];

            state += toneCoefficient * (input - state);

            const auto low = state * lowScale;
            const auto high = (input - state) * highScale;

            channels[channel][sample] = low + high;

            lowEnergy += (double) low * low;
            highEnergy += (double) high * high;
        }
    }

    // Measured after the tilt, so the reading is what the module produced and
    // not what it was given. Compared in dB rather than as a ratio of energies,
    // because a ratio pins to the bass on anything with a rhythm section and
    // stops moving.
    const auto lowDb = juce::Decibels::gainToDecibels((float) std::sqrt(lowEnergy));
    const auto highDb =
        juce::Decibels::gainToDecibels((float) std::sqrt(highEnergy));

    const auto quiet = lowEnergy + highEnergy < 1.0e-9;
    const auto balance = 0.5f + (highDb - lowDb) / balanceWindowDb;

    // Silence has no balance. Reporting the middle is what stops the picture
    // lurching to one end every time the transport stops.
    publish(ModuleId::tone, quiet ? 0.5f : balance);
}

void Processor::processSpace(juce::AudioBuffer<float>& buffer) noexcept
{
    if (!isEnabled(ModuleId::space))
    {
        publish(ModuleId::space, 0.f);
        return;
    }

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = juce::jmin(2, buffer.getNumChannels());
    auto* const* channels = buffer.getArrayOfWritePointers();

    const auto mix = space->load(std::memory_order_relaxed) * 0.01f;

    spaceMix.setTargetValue(mix);

    // One knob, two jobs: a wider setting is both louder and longer, which is
    // how a size control behaves and how a pair of independent ones does not.
    // Capped short of 1, since a delay line at unity feedback never decays.
    const auto feedback = 0.15f + 0.6f * mix;

    auto wetPeak = 0.f;

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto wetLevel = spaceMix.getNextValue();

        for (auto channel = 0; channel < numChannels; ++channel)
        {
            const auto wet = echoes[(std::size_t) channel].process(
                channels[channel][sample], feedback);

            wetPeak = juce::jmax(wetPeak, std::abs(wet));
            channels[channel][sample] += wet * wetLevel;
        }
    }

    // The delay's own output, not the module's. That is what lets the picture
    // outlive the playing by exactly as long as the repeats do.
    publish(ModuleId::space, wetPeak);
}

void Processor::processWidth(juce::AudioBuffer<float>& buffer) noexcept
{
    // Mono has no width to set and no correlation to report: a channel is
    // perfectly correlated with itself, which is what 1 means here.
    if (buffer.getNumChannels() < 2 || !isEnabled(ModuleId::width))
    {
        publish(ModuleId::width, 1.f);
        return;
    }

    const auto numSamples = buffer.getNumSamples();

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    widthAmount.setTargetValue(width->load(std::memory_order_relaxed) * 0.01f);

    auto sumLeftRight = 0.0;
    auto sumLeft = 0.0;
    auto sumRight = 0.0;

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto amount = widthAmount.getNextValue();

        const auto mid = 0.5f * (left[sample] + right[sample]);
        const auto side = 0.5f * (left[sample] - right[sample]) * amount;

        const auto outLeft = mid + side;
        const auto outRight = mid - side;

        left[sample] = outLeft;
        right[sample] = outRight;

        sumLeftRight += (double) outLeft * outRight;
        sumLeft += (double) outLeft * outLeft;
        sumRight += (double) outRight * outRight;
    }

    // Pearson correlation over the block, on the output rather than the input,
    // so the reading is what leaves the plugin. -1 is a phase-inverted pair,
    // which is the reading the picture turns red on.
    const auto denominator = std::sqrt(sumLeft * sumRight);
    const auto correlation = denominator > 1.0e-9 ? sumLeftRight / denominator : 1.0;

    publish(ModuleId::width, 0.5f + 0.5f * (float) correlation);
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

} // namespace RackPlugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RackPlugin::Processor();
}
