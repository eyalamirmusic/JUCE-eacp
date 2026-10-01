#pragma once

#include "ParameterApi.h"
#include "PluginProcessor.h"

#include <eacp_juce/eacp_juce.h>
#include <eacp/WebView/WebView.h>

namespace WebSynthPlugin
{

// The first four examples put a GPU view in the editor. This one puts a web
// view there, and nothing else: the title, the knobs, the waveform buttons, the
// mono switch and every readout are HTML, CSS and a page of JavaScript,
// embedded in the plugin binary and served to the view from memory. The JUCE
// side of the editor draws nothing at all.
//
// It is the same slot. An eacp WebView is an eacp::Graphics::View like the
// shader views are, so EACPJuce::ViewComponent shows it without knowing it is
// a browser, and resized() is still one line.
//
// What the web view brings that a shader does not is a page that has to talk
// back. eacp's WebViewBridge is that channel — typed commands from the page,
// typed events to it — and ParameterApi is everything this editor puts on it.
class Editor final
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit Editor(Processor& processorToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    // Not `processor`: AudioProcessorEditor already has a member of that name,
    // holding the same object as an AudioProcessor&.
    Processor& audioProcessor;

    // Declaration order is the destruction contract, in reverse: the host
    // component lets go of the surface first, then the bridge tears down its
    // handlers and listeners, then the web view goes, and the api — which the
    // bridge's handlers point into — goes last.
    ParameterApi api;
    eacp::Graphics::WebView webView;
    eacp::Graphics::WebViewBridge bridge {webView, api};
    EACPJuce::ViewComponent webHost {webView};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace WebSynthPlugin
