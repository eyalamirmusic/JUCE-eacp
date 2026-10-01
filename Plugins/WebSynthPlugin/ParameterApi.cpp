#include "ParameterApi.h"

namespace WebSynthPlugin
{
namespace
{
ParameterUpdate makeUpdate(const juce::RangedAudioParameter& parameter,
                           float normalised)
{
    return {parameter.paramID.toStdString(),
            normalised,
            parameter.getText(normalised, 32).toStdString()};
}
} // namespace

ParameterApi::ParameterApi(juce::AudioProcessor& processorToUse)
{
    for (auto* candidate: processorToUse.getParameters())
    {
        auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(candidate);

        if (parameter == nullptr)
            continue;

        auto binding = std::make_unique<Binding>(Binding {*parameter, nullptr});

        // The callback receives the denormalised value, on the message thread,
        // however the change arrived. The page speaks normalised, so the
        // conversion back happens once, here.
        binding->attachment = std::make_unique<juce::ParameterAttachment>(
            *parameter,
            [this, parameter](float value)
            {
                parameterChanged.publish(
                    makeUpdate(*parameter, parameter->convertTo0to1(value)));
            });

        bindings.push_back(std::move(binding));
    }
}

ParameterList ParameterApi::getParameters() const
{
    auto list = ParameterList {};

    for (const auto& binding: bindings)
    {
        const auto& parameter = binding->parameter;
        const auto value = parameter.getValue();

        auto info = ParameterInfo {};
        info.id = parameter.paramID.toStdString();
        info.name = parameter.getName(64).toStdString();
        info.value = value;
        info.defaultValue = parameter.getDefaultValue();
        info.numSteps = parameter.isDiscrete() ? parameter.getNumSteps() : 0;
        info.text = parameter.getText(value, 32).toStdString();

        for (const auto& choice: parameter.getAllValueStrings())
            info.choices.push_back(choice.toStdString());

        list.parameters.push_back(std::move(info));
    }

    return list;
}

void ParameterApi::beginGesture(const ParameterId& request)
{
    if (auto* binding = find(request.id))
        binding->attachment->beginGesture();
}

void ParameterApi::setParameter(const ParameterValue& request)
{
    if (auto* binding = find(request.id))
    {
        const auto normalised = juce::jlimit(0.f, 1.f, request.value);

        binding->attachment->setValueAsPartOfGesture(
            binding->parameter.convertFrom0to1(normalised));
    }
}

void ParameterApi::endGesture(const ParameterId& request)
{
    if (auto* binding = find(request.id))
        binding->attachment->endGesture();
}

void ParameterApi::setMidiActivity(const MidiActivity& activity)
{
    const auto& current = midiActivity.snapshot();

    if (activity.heldNotes != current.heldNotes
        || activity.lastNote != current.lastNote)
        midiActivity.publish(activity);
}

ParameterApi::Binding* ParameterApi::find(const std::string& id) const
{
    for (const auto& binding: bindings)
        if (binding->parameter.paramID.toStdString() == id)
            return binding.get();

    return nullptr;
}

} // namespace WebSynthPlugin
