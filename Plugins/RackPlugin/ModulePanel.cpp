#include "ModulePanel.h"
#include "ModuleShaders.h"

namespace RackPlugin
{
namespace
{
juce::Colour toColour(const unsigned char (&rgb)[3])
{
    return juce::Colour::fromRGB(rgb[0], rgb[1], rgb[2]);
}

juce::Font labelFont(float height, bool bold)
{
    auto options = juce::FontOptions {}.withHeight(height);

    return juce::Font {bold ? options.withStyle("Bold") : options};
}

const auto panelFill = juce::Colour::fromRGB(25, 27, 36);
const auto panelEdge = juce::Colour::fromRGB(46, 50, 66);
const auto captionInk = juce::Colour::fromRGB(122, 133, 158);
} // namespace

ModulePanel::ModulePanel(Processor& processorToUse, const ModuleSpec& specToUse)
    : audioProcessor(processorToUse)
    , spec(specToUse)
    , view(makeModuleView(specToUse.id))
    , sliderAttachment(audioProcessor.apvts, specToUse.paramId, slider)
    , enableAttachment(audioProcessor.apvts, specToUse.enableId, enableButton)
{
    const auto accent = toColour(spec.accent);

    // The whole channel from the plugin into the picture, and all three ends of
    // it are read on the main thread once per rendered frame.
    //
    // The energy comes from the processor as one relaxed atomic. The other two
    // come from the parameters themselves, normalised — getValue() is 0 to 1
    // whatever the range is, which is exactly what a shader wants and the
    // reason no shader here knows a decibel from a percent.
    auto* amountParameter = audioProcessor.apvts.getParameter(spec.paramId);
    auto* enableParameter = audioProcessor.apvts.getParameter(spec.enableId);

    view->energySource = [this] { return audioProcessor.getEnergy(spec.id); };
    view->amountSource = [amountParameter] { return amountParameter->getValue(); };
    view->activeSource = [enableParameter] { return enableParameter->getValue(); };

    host.setView(view.get());
    addAndMakeVisible(host);

    titleLabel.setText(spec.title, juce::dontSendNotification);
    titleLabel.setFont(labelFont(13.f, true));
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    // What this panel's picture is lit by. Different in every module, and not
    // guessable from looking at it, so the UI says so.
    meterLabel.setText(spec.meter, juce::dontSendNotification);
    meterLabel.setFont(labelFont(10.f, false));
    meterLabel.setColour(juce::Label::textColourId, captionInk);
    meterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabel);

    enableButton.setColour(juce::ToggleButton::tickColourId, accent);
    enableButton.setTooltip("Bypass " + juce::String {spec.title});
    addAndMakeVisible(enableButton);

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 16);

    // The unit, which the attachment does not bring with it: it takes the
    // parameter's *text* for the readout, and a parameter's label is a separate
    // field from the text it formats.
    slider.setTextValueSuffix(" " + amountParameter->getLabel());
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::thumbColourId, accent.brighter(0.4f));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, panelEdge);
    slider.setColour(juce::Slider::textBoxOutlineColourId,
                     juce::Colours::transparentBlack);
    addAndMakeVisible(slider);
}

void ModulePanel::paint(juce::Graphics& g)
{
    // The panel's chrome, and only its chrome. The middle of it is the eacp
    // surface, and anything painted under that is painted for nobody.
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(panelFill);
    g.fillRoundedRectangle(bounds, 6.f);

    g.setColour(panelEdge);
    g.drawRoundedRectangle(bounds, 6.f, 1.f);
}

void ModulePanel::resized()
{
    auto bounds = getLocalBounds().reduced(6, 5);

    auto header = bounds.removeFromTop(20);
    enableButton.setBounds(header.removeFromRight(24));
    titleLabel.setBounds(header);

    // A rotary's diameter is whatever height it is handed, so this is a share
    // of what is left rather than a fixed number: the layout above reflows into
    // two rows when the window narrows, which halves every panel's height
    // without warning, and a fixed knob would swallow the picture when it did.
    const auto knobHeight = juce::jlimit(52, 96, bounds.getHeight() / 3);

    slider.setBounds(bounds.removeFromBottom(knobHeight));
    meterLabel.setBounds(bounds.removeFromBottom(14));

    // The surface follows this call: ViewComponent is watching the component
    // hierarchy and moves the native view to wherever the bounds land — which
    // in this editor happens four times per resize, and again every time the
    // layout reflows between one row and two.
    host.setBounds(bounds.reduced(0, 4));
}

} // namespace RackPlugin
