#pragma once

#include "PluginProcessor.h"
#include "ShaderView.h"

#include <eacp_juce/eacp_juce.h>

namespace ShaderPlugin
{

// The editor is where the two frameworks meet, and the meeting is three lines
// of it: a ShaderView, an EACPJuce::ViewComponent showing that view, and
// setBounds in resized(). Everything else — the slider, the attachment, the
// resize limits — is JUCE exactly as it always was.
//
// The two halves are kept side by side rather than stacked. A native surface
// draws over whatever JUCE component it overlaps, whatever the z-order says,
// which is true of an OpenGL context and a web view as well; giving the shader
// its own rectangle is how that stops being a problem.
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
    ShaderView shaderView;
    EACPJuce::ViewComponent shaderHost {shaderView};

    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace ShaderPlugin
