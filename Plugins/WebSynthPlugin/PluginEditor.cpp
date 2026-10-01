#include "PluginEditor.h"

namespace WebSynthPlugin
{
namespace
{
// The ResEmbed category the CMakeLists files Web/ under. The view serves it on
// app://local/, so index.html finds style.css and main.js by relative path.
constexpr auto webCategory = "WebSynthPlugin";

// MIDI activity is a light on the page, not a meter. 30Hz is plenty for a
// light, and the api only crosses the bridge when the figure changed.
constexpr auto refreshHz = 30;

eacp::Graphics::WebView::Options webOptions()
{
    auto options = eacp::Graphics::embeddedOptions(webCategory);

    // A plugin ships its page; it does not go looking for a Vite dev server on
    // localhost every time a host opens the editor.
    options.embedded.preferDevServer = false;

    // The browser's own menu (Reload, Back) has no business in a plugin, and
    // the page draws everything a click could need.
    options.defaultContextMenu = false;

    // Keys the page does not use go on to the host, so its transport
    // shortcuts keep working while the editor has focus.
    options.forwardUnhandledKeys = true;

    // A plugin window is rarely the focused one. Without this, the first click
    // on a knob only activates the window, and the drag has to start again.
    options.acceptFirstMouse = true;

    return options;
}
} // namespace

Editor::Editor(Processor& processorToUse)
    : AudioProcessorEditor(processorToUse)
    , audioProcessor(processorToUse)
    , api(processorToUse)
    , webView(webOptions())
{
    addAndMakeVisible(webHost);

    // Resized by the host, not by a JUCE corner grip: a grip is a JUCE
    // component, and the surface would draw over it. The page reflows to
    // whatever size it is given.
    setResizable(true, false);
    setResizeLimits(620, 380, 1400, 900);
    setSize(760, 440);

    startTimerHz(refreshHz);
}

void Editor::timerCallback()
{
    api.setMidiActivity(
        {audioProcessor.getHeldNotes(), audioProcessor.getLastNote()});
}

void Editor::paint(juce::Graphics& g)
{
    // Seen only for the moment before the page's first paint, and in the
    // page's own background colour, so opening the editor does not flash.
    g.fillAll(juce::Colour::fromRGB(14, 15, 21));
}

void Editor::resized()
{
    webHost.setBounds(getLocalBounds());
}

} // namespace WebSynthPlugin
