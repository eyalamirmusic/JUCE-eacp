#include <eacp/Core/Utils/WinInclude.h>

namespace EACPJuce::Native
{

void prepare(void*)
{
    // Nothing to undo: EmbeddedView's child window is sized by whoever calls
    // SetWindowPos on it and by nothing else, so unlike AppKit's autoresizing
    // there is no second party to switch off here.
}

void setBounds(void* handle, juce::Rectangle<int> area, double scale)
{
    // The child window is measured in physical pixels while JUCE's rectangle is
    // in logical points, so the peer's scale factor is the whole conversion.
    // getSmallestIntegerContainer rather than a round: a surface rounded down
    // leaves a seam of host window showing along its right and bottom edges.
    auto physical = (area.toFloat() * (float) scale).getSmallestIntegerContainer();

    SetWindowPos((HWND) handle,
                 nullptr,
                 physical.getX(),
                 physical.getY(),
                 physical.getWidth(),
                 physical.getHeight(),
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void setVisible(void* handle, bool shouldBeVisible)
{
    // SW_SHOWNA, not SW_SHOW: showing the surface must not take the focus off
    // whatever in the editor had it.
    ShowWindow((HWND) handle, shouldBeVisible ? SW_SHOWNA : SW_HIDE);
}

} // namespace EACPJuce::Native
