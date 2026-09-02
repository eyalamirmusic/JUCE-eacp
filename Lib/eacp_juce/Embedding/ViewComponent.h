#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <eacp/Graphics/View/View.h>

#include <memory>

namespace EACPJuce
{

/** A juce::Component that shows an eacp View.

    This is the whole boundary between the two frameworks. An eacp View — and a
    GPU::GPUView is one — is backed by a real platform surface: an NSView on
    macOS, a composition-hosted HWND on Windows. eacp already knows how to put
    one of those inside a window somebody else owns, because that is what a
    plugin editor is; `eacp::Graphics::EmbeddedView` is that door. What it does
    not know is where the surface should sit, because only JUCE knows that.

    So this class is the join: it opens the door onto the peer's native handle
    as soon as the component has a peer, and from then on keeps the surface
    tracking the component's position on screen — through moves, resizes,
    parent changes, being hidden, and the editor being reopened into a
    different window, which a plugin host does routinely.

    Use it the way you would any other component:

    @code
    class Editor : public juce::AudioProcessorEditor
    {
        MyShaderView shaderView;                    // an eacp GPU::GPUView
        EACPJuce::ViewComponent host { shaderView };

        Editor (Processor& p) : AudioProcessorEditor (p)
        {
            addAndMakeVisible (host);
            setSize (600, 400);
        }

        void resized() override { host.setBounds (getLocalBounds()); }
    };
    @endcode

    The view is not owned: it must outlive the ViewComponent showing it, which
    is what declaring it as an earlier member gets you.

    Being a native surface, it draws over any JUCE component it overlaps,
    whatever the z-order says — the same caveat that applies to
    juce::NSViewComponent, juce::HWNDComponent and every OpenGL or web view.
    Give it its own rectangle and put JUCE widgets beside it, not on it.
*/
class ViewComponent : public juce::Component
{
public:
    /** Creates a container with nothing in it yet. */
    ViewComponent();

    /** Creates a container showing `viewToShow`, which must outlive it. */
    explicit ViewComponent(eacp::Graphics::View& viewToShow);

    ~ViewComponent() override;

    /** Shows an eacp view, or nothing when passed nullptr.

        The view is not owned and must outlive this component, or the call that
        replaces it. Safe to call before the component is on screen: the
        platform surface is created when a peer appears, not here.
    */
    void setView(eacp::Graphics::View* viewToShow);

    eacp::Graphics::View* getView() const noexcept { return content; }

    /** @internal */
    void paint(juce::Graphics&) override {}

private:
    class Attachment;

    eacp::Graphics::View* content = nullptr;
    std::unique_ptr<Attachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewComponent)
};

} // namespace EACPJuce
