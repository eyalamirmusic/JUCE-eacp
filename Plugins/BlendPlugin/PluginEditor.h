#pragma once

#include "AuroraView.h"
#include "PluginProcessor.h"

#include <eacp_juce/eacp_juce.h>

namespace BlendPlugin
{

// The three examples before this one all obey the same rule: give the eacp
// surface its own rectangle, and put JUCE widgets beside it, never under it. A
// native surface draws over whatever JUCE component it overlaps, whatever the
// z-order says, so sharing a rectangle looked like something to avoid.
//
// It is not, quite. "Draws over" is the compositor's answer when the surface is
// opaque, and the surface does not have to be. eacp::Graphics::View::setOpacity
// is group opacity for a whole view — chrome, children and GPU content — and
// what it composites over is the layer behind, which in a plugin is the JUCE
// peer's own layer, holding everything JUCE just painted. Turn it down and the
// JUCE panel comes through the shader.
//
// So this editor deliberately breaks the rule the others follow. The stage is
// one JUCE drawing, painted edge to edge; the eacp surface covers the right
// half of it. The same rings, the same grid and the same meter run across the
// boundary, crisp on the left and under the aurora on the right, and the Blend
// slider sweeps the right half between the two. Everything JUCE paints there
// keeps animating while it is under the surface, because this is the OS
// compositing two live layers and not a picture of one pasted over the other.
//
// Two things it does not get you, both visible in this editor:
//
//  - Uniform opacity, not a per-pixel alpha channel. The surface fades as one.
//    A shader writing alpha per fragment does not punch holes in itself,
//    because the Metal layer eacp gives a GPUView is opaque and its alpha
//    channel is discarded.
//
//  - No mouse. The platform hit-tests the surface before JUCE sees the event,
//    so a JUCE control under a visible surface can be seen and not touched.
//    Which is why both sliders live in the strip below the stage, and why
//    AuroraView::setBlend hides the surface outright at zero.
class Editor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor(Processor& processorToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    // The JUCE half of the stage drawing. Called once, over the whole stage,
    // with no idea that half of it is about to be composited under a shader.
    void paintStage(juce::Graphics& g, juce::Rectangle<int> stage) const;

    // Not `processor`: AudioProcessorEditor already has a member of that name,
    // holding the same object as an AudioProcessor&.
    Processor& audioProcessor;

    // Declared before the component that shows it: the view has to outlive the
    // ViewComponent, and member destruction runs in reverse.
    AuroraView auroraView;
    EACPJuce::ViewComponent auroraHost {auroraView};

    juce::Slider blendSlider;
    juce::Label blendLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment blendAttachment;

    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;

    juce::Rectangle<int> stageBounds;

    // The JUCE drawing's own animation state, advanced on the timer. It exists
    // so that what is under the surface is visibly live rather than a still:
    // the sweep turns and the meter moves whether the blend is 0 or 1.
    float sweep = 0.f;
    float smoothedLevel = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace BlendPlugin
