#include "PluginEditor.h"

namespace ShaderPlugin
{

Editor::Editor(Processor& processorToUse)
    : AudioProcessorEditor(processorToUse)
    , audioProcessor(processorToUse)
    , gainAttachment(audioProcessor.apvts, "gain", gainSlider)
{
    // The whole channel from the audio to the picture. Called on the main
    // thread once per rendered frame; it reads the atomic the audio thread
    // publishes and nothing else.
    shaderView.levelSource = [this] { return audioProcessor.getOutputLevel(); };

    addAndMakeVisible(shaderHost);

    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    addAndMakeVisible(gainSlider);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(gainLabel);

    setResizable(true, true);
    setResizeLimits(420, 300, 1600, 1100);
    setSize(680, 440);
}

void Editor::paint(juce::Graphics& g)
{
    // Only the control strip: the rest of the editor is the eacp surface, and
    // anything painted under it is painted for nobody.
    g.fillAll(juce::Colour::fromRGB(18, 19, 26));
}

void Editor::resized()
{
    auto bounds = getLocalBounds();
    auto controls = bounds.removeFromBottom(56).reduced(12, 10);

    // The surface follows this call: ViewComponent is watching the component
    // hierarchy and moves the native view to wherever the bounds land.
    shaderHost.setBounds(bounds);

    gainLabel.setBounds(controls.removeFromLeft(48));
    gainSlider.setBounds(controls);
}

} // namespace ShaderPlugin
