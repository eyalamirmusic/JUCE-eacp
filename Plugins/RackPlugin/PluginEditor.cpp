#include "PluginEditor.h"

namespace RackPlugin
{
namespace
{
// Below this the four panels stop fitting side by side and the rack folds into
// two rows of two. Measured against the area the panels get, not the window, so
// the header's width does not enter into it.
constexpr auto singleRowWidth = 660;

constexpr auto headerHeight = 46;

const auto background = juce::Colour::fromRGB(15, 16, 22);
const auto titleInk = juce::Colour::fromRGB(232, 238, 250);
const auto captionInk = juce::Colour::fromRGB(112, 122, 148);
const auto ruleInk = juce::Colour::fromRGB(38, 42, 56);

juce::Font headerFont(float height, bool bold)
{
    auto options = juce::FontOptions {}.withHeight(height);

    return juce::Font {bold ? options.withStyle("Bold") : options};
}
} // namespace

Editor::Editor(Processor& processorToUse)
    : AudioProcessorEditor(processorToUse)
    , audioProcessor(processorToUse)
    , outputAttachment(audioProcessor.apvts, "output", outputSlider)
{
    // One panel per row of the table, in the order the table lists them, which
    // is also the order the audio goes through them. The editor never names a
    // module, a parameter or a shader.
    for (const auto& spec: moduleSpecs)
        addAndMakeVisible(panels.add(new ModulePanel(audioProcessor, spec)));

    outputLabel.setText("Output", juce::dontSendNotification);
    outputLabel.setFont(headerFont(12.f, false));
    outputLabel.setColour(juce::Label::textColourId, captionInk);
    outputLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(outputLabel);

    outputSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
    outputSlider.setColour(juce::Slider::textBoxOutlineColourId,
                           juce::Colours::transparentBlack);
    addAndMakeVisible(outputSlider);

    setResizable(true, true);

    // The floor is set by the folded layout rather than by the single row: two
    // rows of two is where a panel's height gets tight, and a panel with no
    // room left for its picture is the one arrangement worth refusing.
    setResizeLimits(520, 440, 1800, 1200);
    setSize(900, 500);
}

void Editor::paint(juce::Graphics& g)
{
    // The header and the gaps between the panels. Everything else is either a
    // panel's own chrome or one of the four surfaces, and painting under a
    // surface is painting for nobody.
    g.fillAll(background);

    auto header = getLocalBounds().removeFromTop(headerHeight);
    auto text = header.reduced(16, 0);

    g.setColour(titleInk);
    g.setFont(headerFont(15.f, true));

    const auto titleWidth =
        juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), "EACP RACK");

    g.drawText("EACP RACK",
               text.removeFromLeft(titleWidth + 12),
               juce::Justification::centredLeft);

    // ASCII, deliberately: juce::String's char* constructor decodes as ASCII
    // and asserts on anything above 127, so a typographer's separator here
    // would need a CharPointer_UTF8 wrapper to survive the trip.
    g.setColour(captionInk);
    g.setFont(headerFont(11.f, false));
    g.drawText("four modules, four surfaces, four shaders",
               text,
               juce::Justification::centredLeft);

    g.setColour(ruleInk);
    g.fillRect(header.removeFromBottom(1));
}

void Editor::resized()
{
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(headerHeight).reduced(16, 11);

    outputSlider.setBounds(header.removeFromRight(190));
    outputLabel.setBounds(header.removeFromRight(56).withTrimmedRight(8));

    bounds.reduce(10, 8);

    // A row of four while there is room for it, two rows of two when there is
    // not. Reflowing rather than squeezing is worth the eight lines: a panel
    // narrower than its knob's text box stops being a control, and the surface
    // inside it stops being a picture and becomes a stripe.
    const auto columns = bounds.getWidth() >= singleRowWidth ? moduleCount : 2;
    const auto rows = moduleCount / columns;

    const auto cellWidth = bounds.getWidth() / columns;
    const auto cellHeight = bounds.getHeight() / rows;

    for (auto index = 0; index < panels.size(); ++index)
    {
        const auto cell =
            juce::Rectangle<int> {bounds.getX() + (index % columns) * cellWidth,
                                  bounds.getY() + (index / columns) * cellHeight,
                                  cellWidth,
                                  cellHeight};

        // Each of these moves a native surface, by way of the panel's own
        // resized(). Four in one pass, and again on every frame of a drag.
        panels[index]->setBounds(cell.reduced(5));
    }
}

} // namespace RackPlugin
