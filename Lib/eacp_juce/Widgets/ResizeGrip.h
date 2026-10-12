#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <eacp/UI/Component/Component.h>

#include <functional>

namespace EACPJuce
{

/** The bottom-right resize corner of an editor whose content is an eacp tree.

    JUCE's ResizableCornerComponent is a JUCE component, and a JUCE component
    under a native eacp surface is never seen or clicked. This is the same
    corner as an eacp component: place it in the tree, usually in the bottom
    right of the root, and a drag on it resizes the JUCE component it targets,
    normally the AudioProcessorEditor, through that component's
    ComponentBoundsConstrainer, fixed aspect ratio included.

    It draws nothing: products draw their own grip and lay this over it.

    Drag distances arrive in surface points. When the target is scaled, as a
    plugin editor is by AudioProcessorEditor::setScaleFactor, they are divided
    by that scale before being applied to the target's unscaled bounds.

    onResize receives the constrained bounds. It applies them to the target by
    default; replace it to route the size somewhere else.
*/
class ResizeGrip : public eacp::UI::Component
{
public:
    ResizeGrip();

    explicit ResizeGrip(juce::Component& componentToResize,
                        juce::ComponentBoundsConstrainer* constrainerToUse = nullptr);

    /** The component to resize, and the constrainer to resize it through. Neither
        is owned. Passing nullptr for the component disables the grip. */
    void setTarget(juce::Component* componentToResize,
                   juce::ComponentBoundsConstrainer* constrainerToUse = nullptr);

    juce::Component* getTarget() const noexcept { return target.getComponent(); }

    juce::ComponentBoundsConstrainer* getConstrainer() const noexcept
    {
        return constrainer;
    }

    bool isDragging() const noexcept { return dragging; }

    std::function<void(juce::Rectangle<int>)> onResize =
        [this](juce::Rectangle<int> bounds) { applyBounds(bounds); };

    /** @internal */
    void mouseDown(const eacp::UI::MouseEvent& event) override;
    /** @internal */
    void mouseDrag(const eacp::UI::MouseEvent& event) override;
    /** @internal */
    void mouseUp(const eacp::UI::MouseEvent& event) override;
    /** @internal */
    void mouseCancel(const eacp::UI::MouseEvent& event) override;

private:
    float getTargetScale() const;
    juce::Rectangle<int> getLimits() const;
    void applyBounds(juce::Rectangle<int> bounds);
    void endDrag();

    juce::Component::SafePointer<juce::Component> target;
    juce::ComponentBoundsConstrainer* constrainer = nullptr;

    juce::Rectangle<int> startBounds;
    eacp::Graphics::Point startPosition;
    float startScale = 1.f;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizeGrip)
};

} // namespace EACPJuce
