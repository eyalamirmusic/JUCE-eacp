#pragma once

#include <juce_graphics/juce_graphics.h>

// The three things JUCE has to be able to say to the platform surface that
// eacp's EmbeddedView created, and the only place in this module where a
// platform header is reachable.
//
// Everything above this line is portable: ViewComponent decides *when* to move
// or hide the surface and computes the rectangle, in JUCE's own coordinates —
// logical points, y down, relative to the peer. These three turn that into
// AppKit or Win32.

namespace EACPJuce::Native
{

// Called once, on the handle EmbeddedView just created.
//
// EmbeddedView gives its surface whatever a view *filling* a host window wants,
// because filling the window is what a plugin editor usually does. Here the
// frame is set explicitly on every move instead, so anything the platform does
// on its own to that frame is something to switch off.
void prepare(void* handle);

// `area` is what ComponentPeer::getAreaCoveredBy reported: logical points, y
// down, relative to the peer's client area. `scale` is the peer's platform
// scale factor, which is 1 everywhere except Windows, where the surface is
// sized in physical pixels.
void setBounds(void* handle, juce::Rectangle<int> area, double scale);

void setVisible(void* handle, bool shouldBeVisible);

} // namespace EACPJuce::Native
