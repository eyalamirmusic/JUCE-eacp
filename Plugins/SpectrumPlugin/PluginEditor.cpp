#include "PluginEditor.h"

namespace SpectrumPlugin
{
namespace
{
void configureSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 22);
}

void configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
}
} // namespace

Editor::Editor(Processor& processorToUse)
    : AudioProcessorEditor(processorToUse)
    , audioProcessor(processorToUse)
    , spectrumView(processorToUse.getAnalyser())
    , sensitivityAttachment(audioProcessor.apvts, "sensitivity", sensitivitySlider)
    , falloffAttachment(audioProcessor.apvts, "falloff", falloffSlider)
{
    // The two controls' route into the picture. Called on the main thread once
    // per rendered frame, and neither of them reads anything the audio thread
    // is writing — the parameters are atomics the host sets.
    spectrumView.sensitivityDb = [this]
    { return audioProcessor.getSensitivityDb(); };

    spectrumView.falloffSeconds = [this]
    { return audioProcessor.getFalloffSeconds(); };

    addAndMakeVisible(spectrumHost);

    configureSlider(sensitivitySlider);
    configureSlider(falloffSlider);
    addAndMakeVisible(sensitivitySlider);
    addAndMakeVisible(falloffSlider);

    configureLabel(sensitivityLabel, "Sens");
    configureLabel(falloffLabel, "Fall");
    addAndMakeVisible(sensitivityLabel);
    addAndMakeVisible(falloffLabel);

    setResizable(true, true);
    setResizeLimits(480, 320, 1900, 1200);
    setSize(760, 460);
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
    spectrumHost.setBounds(bounds);

    auto left = controls.removeFromLeft(controls.getWidth() / 2);
    left.removeFromRight(12);

    sensitivityLabel.setBounds(left.removeFromLeft(42));
    sensitivitySlider.setBounds(left);

    falloffLabel.setBounds(controls.removeFromLeft(42));
    falloffSlider.setBounds(controls);
}

} // namespace SpectrumPlugin
