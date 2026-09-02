#include "ViewComponent.h"

#include "../native/NativeSurface.h"

#include <eacp/Graphics/Window/EmbeddedView.h>

#include <memory>
#include <utility>

// The platform halves of NativeSurface. Not translation units of their own:
// juce_add_module compiles only the sources named after the module itself, so
// everything under a subdirectory reaches the build by being included from one
// of those — here, by way of this file.
#if JUCE_MAC
#include "../native/NativeSurface_mac.mm"
#elif JUCE_WINDOWS
#include "../native/NativeSurface_windows.cpp"
#endif

namespace EACPJuce
{

// A ComponentMovementWatcher rather than a set of Component overrides, for the
// reason JUCE's own NSViewComponent and HWNDComponent use one: a native surface
// is positioned relative to the *window*, so it has to move when any ancestor
// moves, not only when its immediate parent does. Component::moved() does not
// report that; this does.
//
// The surface itself is created and destroyed here too, and not in
// ViewComponent's constructor, because it cannot exist before there is a peer
// to attach it to and must not outlive that peer. A plugin editor is opened,
// closed and reopened into a fresh host window over a session, so peer changes
// are the normal case rather than the exception.
class ViewComponent::Attachment final : public juce::ComponentMovementWatcher
{
public:
    Attachment(juce::Component& ownerToUse, eacp::Graphics::View& viewToShow)
        : ComponentMovementWatcher(&ownerToUse)
        , owner(ownerToUse)
        , content(viewToShow)
    {
        if (owner.isShowing())
            componentPeerChanged();
    }

    ~Attachment() override = default;

    void componentMovedOrResized(bool, bool) override { updateBounds(); }

    // The ComponentMovementWatcher version of this deliberately skips the case
    // where the watched component is itself the top-level one, which in a
    // plugin it can be: an editor that hosts nothing but the eacp view, and is
    // resized by the host, would otherwise never tell the surface about it.
    void componentMovedOrResized(juce::Component& comp,
                                 bool wasMoved,
                                 bool wasResized) override
    {
        ComponentMovementWatcher::componentMovedOrResized(
            comp, wasMoved, wasResized);

        if (comp.isOnDesktop() && wasResized)
            updateBounds();
    }

    void componentPeerChanged() override
    {
        auto* peer = owner.getPeer();

        if (std::exchange(currentPeer, peer) != peer)
        {
            // Destroy first, always: the surface is a child of the old peer's
            // native window, and on Windows that window may already be on its
            // way out.
            embedded = nullptr;

            if (peer != nullptr)
                attachTo(*peer);
        }

        if (embedded != nullptr)
            Native::setVisible(embedded->getHandle(), owner.isShowing());
    }

    void componentVisibilityChanged() override { componentPeerChanged(); }

    using ComponentMovementWatcher::componentVisibilityChanged;

private:
    void attachTo(juce::ComponentPeer& peer)
    {
        auto options = eacp::Graphics::EmbeddedViewOptions {};
        options.width = juce::jmax(1, owner.getWidth());
        options.height = juce::jmax(1, owner.getHeight());

        embedded = std::make_unique<eacp::Graphics::EmbeddedView>(
            peer.getNativeHandle(), options);

        Native::prepare(embedded->getHandle());
        embedded->setContentView(content);

        updateBounds();
    }

    void updateBounds()
    {
        if (embedded == nullptr)
            return;

        // The top-level component's peer, not the owner's: they are the same
        // window, but during a teardown the owner can already have been
        // detached while the rectangle is still being asked for.
        if (auto* peer = owner.getTopLevelComponent()->getPeer())
            Native::setBounds(embedded->getHandle(),
                              peer->getAreaCoveredBy(owner),
                              peer->getPlatformScaleFactor());
    }

    juce::Component& owner;
    eacp::Graphics::View& content;

    juce::ComponentPeer* currentPeer = nullptr;
    std::unique_ptr<eacp::Graphics::EmbeddedView> embedded;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Attachment)
};

//==============================================================================
ViewComponent::ViewComponent() = default;

ViewComponent::ViewComponent(eacp::Graphics::View& viewToShow)
{
    setView(&viewToShow);
}

// Out of line, and not defaulted in the header, because Attachment is only a
// forward declaration there.
ViewComponent::~ViewComponent() = default;

void ViewComponent::setView(eacp::Graphics::View* viewToShow)
{
    if (viewToShow == content)
        return;

    // Torn down before the new one is built: two attachments would both be
    // trying to own a surface in the same place.
    attachment = nullptr;
    content = viewToShow;

    if (content != nullptr)
        attachment = std::make_unique<Attachment>(*this, *content);
}

} // namespace EACPJuce
