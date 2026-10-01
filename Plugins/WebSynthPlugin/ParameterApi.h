#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <Miro/Bridge.h>
#include <Miro/Reflect.h>

#include <memory>
#include <string>
#include <vector>

namespace WebSynthPlugin
{

// What the page is told about a parameter when it loads. Values cross the
// bridge normalised, 0 to 1, which is what RangedAudioParameter speaks and what
// a knob's angle is; the text beside it is the parameter's own, so the page
// never has to know that cutoff is skewed or that gain is in decibels.
struct ParameterInfo
{
    std::string id;
    std::string name;
    float value = 0.f;
    float defaultValue = 0.f;
    int numSteps = 0;
    std::vector<std::string> choices;
    std::string text;

    MIRO_REFLECT(id, name, value, defaultValue, numSteps, choices, text)
};

struct ParameterList
{
    std::vector<ParameterInfo> parameters;

    MIRO_REFLECT(parameters)
};

struct ParameterId
{
    std::string id;

    MIRO_REFLECT(id)
};

struct ParameterValue
{
    std::string id;
    float value = 0.f;

    MIRO_REFLECT(id, value)
};

// Pushed to the page whenever a parameter moves, whoever moved it: the page
// itself, the host's automation, a preset load, a generic editor in the host.
struct ParameterUpdate
{
    std::string id;
    float value = 0.f;
    std::string text;

    MIRO_REFLECT(id, value, text)
};

struct MidiActivity
{
    int heldNotes = 0;
    int lastNote = -1;

    MIRO_REFLECT(heldNotes, lastNote)
};

// The editor's whole bridged surface, and the only place in the plugin where
// the page and the parameters meet.
//
// Page to host is four commands. A drag is beginGesture, any number of
// setParameter calls, endGesture — the bracket a host needs to record touch and
// release in an automation lane — and a click is the same three at once.
//
// Host to page is one event. Every parameter has a juce::ParameterAttachment,
// which is JUCE's own answer to "tell me when this moves, on the message
// thread", and each callback publishes a ParameterUpdate. That is also how the
// page hears back about its own drags; it keeps the readout and ignores the
// position of the knob it is holding, so a late echo cannot pull the knob back.
//
// Nothing here is specific to this synth. The parameters are found by walking
// the processor, so the same class would bind any plugin's page.
class ParameterApi
{
public:
    explicit ParameterApi(juce::AudioProcessor& processorToUse);

    void reflect(Miro::ApiReflector& r)
    {
        using T = ParameterApi;

        r.commands<&T::getParameters,
                   &T::beginGesture,
                   &T::setParameter,
                   &T::endGesture>();

        r.events<&T::parameterChanged, &T::midiActivity>();
    }

    ParameterList getParameters() const;

    void beginGesture(const ParameterId& request);
    void setParameter(const ParameterValue& request);
    void endGesture(const ParameterId& request);

    // Called from the editor's timer. Publishes only when something changed,
    // so an idle keyboard costs the page nothing.
    void setMidiActivity(const MidiActivity& activity);

    Miro::Event<ParameterUpdate> parameterChanged;
    Miro::Event<MidiActivity> midiActivity;

private:
    struct Binding
    {
        juce::RangedAudioParameter& parameter;
        std::unique_ptr<juce::ParameterAttachment> attachment;
    };

    Binding* find(const std::string& id) const;

    std::vector<std::unique_ptr<Binding>> bindings;
};

} // namespace WebSynthPlugin
