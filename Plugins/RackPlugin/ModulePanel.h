#pragma once

#include "ModuleView.h"
#include "Modules.h"
#include "PluginProcessor.h"

#include <eacp_juce/eacp_juce.h>

#include <memory>

namespace RackPlugin
{

// One module of the rack: a JUCE control group with an eacp GPU surface of its
// own inside it.
//
// This is the class the example exists for. The two sibling plugins each put
// one surface in an editor and one row of JUCE controls beside it; here there
// are four of each, built from a table, and the surfaces sit *within* the
// controls rather than next to them — a tile in the middle of a panel that has
// a title bar above it and a knob below.
//
// Which is as close to "behind" as a native surface goes, and the reason is the
// same one that applies to an OpenGL context or a web view: the surface draws
// over any JUCE component it overlaps, whatever the z-order says. A slider
// drawn on top of it would simply not be there. So the panel gives the picture
// its own rectangle and arranges the JUCE widgets around that rectangle, and
// the two read as one control because they are painted to agree — the knob
// takes its accent from the same colours the shader is written in.
//
// Nothing about the four is written down twice. The panel is handed a row of
// the spec table and finds everything from it: which shader to build, which
// parameter the knob drives, which bypass the checkbox drives, what its meter
// is called, and what colour it is.
class ModulePanel final : public juce::Component
{
public:
    ModulePanel(Processor& processorToUse, const ModuleSpec& specToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Not `processor`: an AudioProcessorEditor already has a member of that
    // name, and a panel that borrows the convention is easier to read beside
    // one.
    Processor& audioProcessor;

    // A row of the constexpr table, which has static storage duration, so the
    // reference outlives every editor.
    const ModuleSpec& spec;

    // Declared before the component that shows it: the view has to outlive the
    // ViewComponent, and member destruction runs in reverse.
    std::unique_ptr<ModuleViewBase> view;
    EACPJuce::ViewComponent host;

    juce::Label titleLabel;
    juce::Label meterLabel;
    juce::ToggleButton enableButton;
    juce::Slider slider;

    juce::AudioProcessorValueTreeState::SliderAttachment sliderAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment enableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePanel)
};

} // namespace RackPlugin
