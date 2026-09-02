#pragma once

#include "ModulePanel.h"
#include "PluginProcessor.h"

namespace RackPlugin
{

// Four eacp surfaces in one JUCE editor, and a JUCE editor around them that
// looks like a plugin rather than like a demo.
//
// The two sibling examples answer "can an eacp view be a plugin editor". This
// one answers the question that comes next, which is whether the arrangement
// survives being one part of a real UI: four independent surfaces, each with
// its own shader and its own swapchain, laid out among a dozen JUCE widgets, in
// a window the host resizes — and reflowing between a row of four and a grid of
// two by two while it does, so every surface moves and resizes at once.
//
// Nothing in ViewComponent needed changing for that. It watches the component
// hierarchy, so a panel moving moves the surface inside it; four of them is
// four watchers, and the fact that they share a peer is not something any of
// them has to know.
//
// Everything below is built from the spec table in Modules.h. The editor knows
// there are modules; it does not know what they are.
class Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor& processorToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Not `processor`: AudioProcessorEditor already has a member of that name,
    // holding the same object as an AudioProcessor&.
    Processor& audioProcessor;

    juce::OwnedArray<ModulePanel> panels;

    // The one control that is not a module's: the rack's output trim, in the
    // header strip where a chain's master usually sits.
    juce::Label outputLabel;
    juce::Slider outputSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;

    // Deliberately not `{this}`, which is the usual spelling. A TooltipWindow
    // given a parent is a JUCE component inside the editor, and a panel's
    // surface would draw straight over it — the tooltip for the bypass at the
    // top of a panel pops downwards, which is exactly where the picture is.
    // Left to itself it takes a desktop window of its own and floats above
    // everything, native surfaces included.
    juce::TooltipWindow tooltips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace RackPlugin
