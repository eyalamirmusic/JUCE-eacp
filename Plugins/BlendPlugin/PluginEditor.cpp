#include "PluginEditor.h"

#include <cmath>

namespace BlendPlugin
{
namespace
{
constexpr auto headerHeight = 46;
constexpr auto controlsHeight = 60;

// The JUCE drawing is animated by a timer rather than by a display link: it is
// ordinary JUCE painting, and 30Hz is what an ordinary JUCE meter runs at. The
// point of animating it at all is that it keeps moving while it is underneath
// the surface, which a still image would not show.
constexpr auto refreshHz = 30;

const auto background = juce::Colour::fromRGB(14, 15, 21);
const auto titleInk = juce::Colour::fromRGB(232, 238, 250);
const auto captionInk = juce::Colour::fromRGB(112, 122, 148);

// The stage palette, and it is brighter than the header's on purpose. Group
// opacity is uniform: at blend 0.5 the aurora's dark background is a 50% black
// wash over half this drawing, so anything drawn at the header's contrast
// disappears on the right and the picture reads as "the left half is lit"
// rather than as two layers sharing a rectangle. Drawing the stage bright
// enough to survive the wash is what keeps the comparison about the blend.
const auto panelInk = juce::Colour::fromRGB(31, 36, 50);
const auto gridInk = juce::Colour::fromRGB(58, 68, 92);
const auto ringInk = juce::Colour::fromRGB(150, 178, 226);
const auto sweepInk = juce::Colour::fromRGB(255, 206, 102);
const auto stageCaptionInk = juce::Colour::fromRGB(158, 172, 204);

juce::Font uiFont(float height, bool bold)
{
    auto options = juce::FontOptions {}.withHeight(height);

    return juce::Font {bold ? options.withStyle("Bold") : options};
}
} // namespace

Editor::Editor(Processor& processorToUse)
    : AudioProcessorEditor(processorToUse)
    , audioProcessor(processorToUse)
    , blendAttachment(audioProcessor.apvts, "blend", blendSlider)
    , gainAttachment(audioProcessor.apvts, "gain", gainSlider)
{
    auroraView.levelSource = [this] { return audioProcessor.getOutputLevel(); };

    addAndMakeVisible(auroraHost);

    blendSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    blendSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);

    // The one line that moves the picture. Hung off the slider rather than read
    // on the timer so it runs when the value actually changes — and it still
    // runs for host automation, because that is what the attachment does to the
    // slider. Called once here for the value the attachment has just restored.
    blendSlider.onValueChange = [this]
    { auroraView.setBlend(static_cast<float>(blendSlider.getValue())); };

    blendSlider.onValueChange();

    addAndMakeVisible(blendSlider);

    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    addAndMakeVisible(gainSlider);

    for (auto& [label, text]: {std::pair {&blendLabel, "Blend"},
                               std::pair {&gainLabel, "Gain"}})
    {
        label->setText(text, juce::dontSendNotification);
        label->setFont(uiFont(12.f, false));
        label->setColour(juce::Label::textColourId, captionInk);
        addAndMakeVisible(*label);
    }

    setResizable(true, true);
    setResizeLimits(560, 380, 1600, 1100);
    setSize(820, 520);

    startTimerHz(refreshHz);
}

void Editor::timerCallback()
{
    sweep += juce::MathConstants<float>::twoPi / static_cast<float>(refreshHz * 6);

    if (sweep > juce::MathConstants<float>::twoPi)
        sweep -= juce::MathConstants<float>::twoPi;

    // The same smoothing shape the shader uses on the same figure, so the JUCE
    // meter and the aurora answer a transient together rather than drifting
    // apart at the seam.
    const auto target = audioProcessor.getOutputLevel();
    const auto perSecond = target > smoothedLevel ? 26.f : 3.2f;

    smoothedLevel += (target - smoothedLevel)
                     * juce::jmin(1.f, perSecond / static_cast<float>(refreshHz));

    repaint(stageBounds);
}

void Editor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    auto header = getLocalBounds().removeFromTop(headerHeight);
    auto text = header.reduced(16, 0);

    g.setColour(titleInk);
    g.setFont(uiFont(15.f, true));

    const auto titleWidth =
        juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), "EACP BLEND");

    g.drawText("EACP BLEND",
               text.removeFromLeft(titleWidth + 12),
               juce::Justification::centredLeft);

    g.setColour(captionInk);
    g.setFont(uiFont(11.f, false));
    g.drawText("one JUCE drawing, half of it under the shader",
               text,
               juce::Justification::centredLeft);

    g.setColour(gridInk);
    g.fillRect(header.removeFromBottom(1));

    // And here is the departure from the other three examples, which all say
    // some version of "painting under a surface is painting for nobody". Half
    // of what this call draws lands under the surface, and all of it is meant
    // to be seen.
    paintStage(g, stageBounds);
}

void Editor::paintStage(juce::Graphics& g, juce::Rectangle<int> stage) const
{
    if (stage.isEmpty())
        return;

    const auto area = stage.toFloat();
    const auto centre = area.getCentre();

    g.setColour(panelInk);
    g.fillRect(area);

    // A grid, because straight thin lines are the thing a GPU wash is most
    // obviously *not*, and the contrast is what makes the blend readable.
    constexpr auto gridStep = 28.f;

    g.setColour(gridInk);

    for (auto column = 0; static_cast<float>(column) * gridStep < area.getWidth();
         ++column)
        g.fillRect(area.getX() + static_cast<float>(column) * gridStep,
                   area.getY(),
                   1.f,
                   area.getHeight());

    for (auto row = 0; static_cast<float>(row) * gridStep < area.getHeight(); ++row)
        g.fillRect(area.getX(),
                   area.getY() + static_cast<float>(row) * gridStep,
                   area.getWidth(),
                   1.f);

    // Rings centred on the stage, which puts their centre exactly on the edge
    // of the surface: every one of them runs out from under the aurora and into
    // the open, so the two halves can be compared on the same curve.
    const auto maxRadius = juce::jmin(area.getWidth(), area.getHeight()) * 0.46f;

    for (auto ring = 1; ring <= 5; ++ring)
    {
        const auto radius = maxRadius * static_cast<float>(ring) / 5.f;

        g.setColour(ringInk.withAlpha(0.75f - 0.09f * static_cast<float>(ring)));
        g.drawEllipse(
            centre.x - radius, centre.y - radius, radius * 2.f, radius * 2.f, 1.4f);
    }

    // The level, drawn as spokes around the rings. This is the part that has to
    // keep moving under the surface for the example to have made its point.
    const auto spokes = 48;

    for (auto spoke = 0; spoke < spokes; ++spoke)
    {
        const auto angle = juce::MathConstants<float>::twoPi
                           * static_cast<float>(spoke)
                           / static_cast<float>(spokes);

        const auto wobble = 0.5f + 0.5f * std::sin(angle * 3.f + sweep * 2.f);
        const auto reach = maxRadius * (0.24f + 0.72f * wobble * smoothedLevel);

        const auto inner = maxRadius * 0.2f;
        const auto outer = inner + reach;

        g.setColour(ringInk.withAlpha(0.35f + 0.5f * wobble));
        g.drawLine(centre.x + std::cos(angle) * inner,
                   centre.y + std::sin(angle) * inner,
                   centre.x + std::cos(angle) * outer,
                   centre.y + std::sin(angle) * outer,
                   1.6f);
    }

    // A sweep hand, so there is one thing on the panel whose motion is obvious
    // at a glance even in silence.
    g.setColour(sweepInk.withAlpha(0.85f));
    g.drawLine(centre.x,
               centre.y,
               centre.x + std::cos(sweep) * maxRadius,
               centre.y + std::sin(sweep) * maxRadius,
               2.f);

    // The seam, and a caption on each side of it. The right-hand caption is
    // under the surface: being able to read JUCE text through the aurora is a
    // shorter argument than any of the comments above.
    const auto seam = area.getCentreX();

    g.setColour(stageCaptionInk.withAlpha(0.5f));

    for (auto dash = 0; static_cast<float>(dash) * 10.f < area.getHeight(); ++dash)
        g.fillRect(
            seam - 0.5f, area.getY() + 4.f + static_cast<float>(dash) * 10.f, 1.f, 5.f);

    auto labels = stage.reduced(14, 10).removeFromTop(16);
    auto left = labels.removeFromLeft(labels.getWidth() / 2);

    g.setColour(stageCaptionInk);
    g.setFont(uiFont(11.f, true));
    g.drawText("JUCE ONLY", left, juce::Justification::centredLeft);
    g.drawText("JUCE UNDER EACP", labels, juce::Justification::centredRight);
}

void Editor::resized()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(headerHeight);

    auto controls = bounds.removeFromBottom(controlsHeight).reduced(16, 12);

    stageBounds = bounds.reduced(16, 12);

    // Exactly the right half of the stage, so the boundary falls on the centre
    // the rings are drawn around. The surface follows this call: ViewComponent
    // is watching the component hierarchy and moves the native view to wherever
    // the bounds land.
    auroraHost.setBounds(stageBounds.withTrimmedLeft(stageBounds.getWidth() / 2));

    auto right = controls.removeFromRight(controls.getWidth() / 2);

    gainLabel.setBounds(right.removeFromLeft(44));
    gainSlider.setBounds(right.withTrimmedRight(12));

    blendLabel.setBounds(controls.removeFromLeft(44));
    blendSlider.setBounds(controls.withTrimmedRight(12));
}

} // namespace BlendPlugin
