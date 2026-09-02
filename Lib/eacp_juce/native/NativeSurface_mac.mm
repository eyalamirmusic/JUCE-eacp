#import <Cocoa/Cocoa.h>

namespace EACPJuce::Native
{

void prepare(void* handle)
{
    // EmbeddedView asks AppKit to keep the surface the size of its superview.
    // JUCE is about to take that job over, and the two disagree the moment the
    // view is anything but the full editor: AppKit would resize the surface on
    // the window's own resize, before JUCE has said where it should be.
    [(NSView*) handle setAutoresizingMask:NSViewNotSizable];
}

void setBounds(void* handle, juce::Rectangle<int> area, double)
{
    // No flip and no conversion: JUCE's peer view answers YES to isFlipped, so
    // a frame set in its coordinates is already top-left based, which is what
    // getAreaCoveredBy hands us. The scale factor is 1 on macOS — AppKit
    // measures in points and does the backing-store maths itself.
    [(NSView*) handle setFrame:NSMakeRect(area.getX(),
                                          area.getY(),
                                          area.getWidth(),
                                          area.getHeight())];
}

void setVisible(void* handle, bool shouldBeVisible)
{
    [(NSView*) handle setHidden:! shouldBeVisible];
}

} // namespace EACPJuce::Native
