#pragma once

#include "PluginProcessor.h"
#include "SpectrumView.h"

#include <eacp_juce/eacp_juce.h>

namespace SpectrumPlugin
{

// The same three lines the sibling example is built on — a view, an
// EACPJuce::ViewComponent showing it, and setBounds in resized() — with two
// JUCE sliders beside them instead of one.
//
// The two halves are kept side by side rather than stacked, for the reason
// every native surface forces: it draws over whatever JUCE component it
// overlaps, whatever the z-order says. Giving the spectrum its own rectangle is
// how that stops being a problem.
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

    // Declared before the component that shows it: the view has to outlive the
    // ViewComponent, and member destruction runs in reverse.
    SpectrumView spectrumView;
    EACPJuce::ViewComponent spectrumHost {spectrumView};

    juce::Slider sensitivitySlider;
    juce::Label sensitivityLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment sensitivityAttachment;

    juce::Slider falloffSlider;
    juce::Label falloffLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment falloffAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace SpectrumPlugin
