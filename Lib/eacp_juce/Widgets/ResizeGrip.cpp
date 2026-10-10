#include "ResizeGrip.h"

#include <limits>
#include <utility>

namespace EACPJuce
{

ResizeGrip::ResizeGrip()
{
    setInterceptsMouseClicks(true);
}

ResizeGrip::ResizeGrip(juce::Component& componentToResize,
                       juce::ComponentBoundsConstrainer* constrainerToUse)
    : ResizeGrip()
{
    setTarget(&componentToResize, constrainerToUse);
}

void ResizeGrip::setTarget(juce::Component* componentToResize,
                           juce::ComponentBoundsConstrainer* constrainerToUse)
{
    endDrag();

    target = componentToResize;
    constrainer = constrainerToUse;
}

void ResizeGrip::mouseDown(const eacp::UI::MouseEvent& event)
{
    if (target == nullptr)
        return;

    startBounds = target->getBounds();
    startPosition = localPointToRoot(event.position);
    startScale = getTargetScale();
    dragging = true;

    if (constrainer != nullptr)
        constrainer->resizeStart();
}

void ResizeGrip::mouseDrag(const eacp::UI::MouseEvent& event)
{
    if (!dragging || target == nullptr)
        return;

    auto position = localPointToRoot(event.position);
    auto distanceX = (position.x - startPosition.x) / startScale;
    auto distanceY = (position.y - startPosition.y) / startScale;

    auto bounds = startBounds.withSize(
        startBounds.getWidth() + juce::roundToInt(distanceX),
        startBounds.getHeight() + juce::roundToInt(distanceY));

    if (constrainer != nullptr)
    {
        constrainer->checkBounds(
            bounds, target->getBounds(), getLimits(), false, false, true, true);
    }

    onResize(bounds);
}

void ResizeGrip::mouseUp(const eacp::UI::MouseEvent&)
{
    endDrag();
}

void ResizeGrip::mouseCancel(const eacp::UI::MouseEvent&)
{
    endDrag();
}

void ResizeGrip::endDrag()
{
    if (!std::exchange(dragging, false))
        return;

    if (constrainer != nullptr)
        constrainer->resizeEnd();
}

float ResizeGrip::getTargetScale() const
{
    auto* top = target->getTopLevelComponent();

    if (top == target.getComponent() || target->getWidth() <= 0)
        return 1.f;

    auto area = top->getLocalArea(target, target->getLocalBounds().toFloat());
    auto scale = area.getWidth() / (float) target->getWidth();

    return scale > 0.f ? scale : 1.f;
}

juce::Rectangle<int> ResizeGrip::getLimits() const
{
    if (auto* parent = target->getParentComponent())
        return {parent->getWidth(), parent->getHeight()};

    auto max = std::numeric_limits<int>::max();
    return {max, max};
}

void ResizeGrip::applyBounds(juce::Rectangle<int> bounds)
{
    if (target == nullptr)
        return;

    if (auto* positioner = target->getPositioner())
        positioner->applyNewBounds(bounds);
    else
        target->setBounds(bounds);
}

} // namespace EACPJuce
