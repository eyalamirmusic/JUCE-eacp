#include "ViewComponent.h"

#include "../Helpers/Conversions.h"

#include <eacp/Graphics/Window/EmbeddedView.h>

#include <memory>
#include <utility>

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
            embedded->setVisible(owner.isShowing());
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

        // A new surface follows the platform's own scale until it is told
        // otherwise, so whatever was last pushed at the old one is not in
        // force here, even when the figure has not changed.
        currentScale = 0.f;

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
        auto* peer = owner.getTopLevelComponent()->getPeer();

        if (peer == nullptr)
            return;

        // Before the bounds, which are measured with it. JUCE's figure rather
        // than the one the surface would read off its own window: in a plugin
        // the scale is the host's to decide, JUCE has already been told what it
        // is, and a surface placed by one number inside a window laid out with
        // another is a surface in the wrong place.
        auto scale = (float) peer->getPlatformScaleFactor();

        if (!juce::exactlyEqual(std::exchange(currentScale, scale), scale))
            embedded->setPixelsPerPoint(scale);

        // getAreaCoveredBy is in the space eacp places a surface in already —
        // points, y-down, relative to the peer — so this is a conversion of
        // types and not of coordinates.
        embedded->setBounds(toEACP(peer->getAreaCoveredBy(owner)));
    }

    juce::Component& owner;
    eacp::Graphics::View& content;

    juce::ComponentPeer* currentPeer = nullptr;

    // Only pushed at the surface when it changes: telling it re-places the
    // surface, and every move would otherwise do that twice.
    float currentScale = 0.f;

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
