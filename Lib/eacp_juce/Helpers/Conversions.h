#pragma once

// Both frameworks have a rectangle, a point and a colour, and a plugin that
// lays out an eacp view from JUCE bounds crosses between them constantly. The
// conversions are one-liners; having them written once means a stray
// getHeight()/getBottom() mix-up happens in one place rather than in every
// editor.
//
// eacp measures in float points with y down, which is JUCE's convention too, so
// nothing here flips or rescales — these are renames, not transforms.

namespace EACPJuce
{

inline eacp::Graphics::Point toEACP(juce::Point<float> point)
{
    return {point.x, point.y};
}

inline eacp::Graphics::Rect toEACP(juce::Rectangle<float> rect)
{
    return {rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight()};
}

inline eacp::Graphics::Rect toEACP(juce::Rectangle<int> rect)
{
    return toEACP(rect.toFloat());
}

// juce::Colour is 8-bit sRGB; eacp's Color is float, and every eacp API that
// takes one wants the components already divided down.
inline eacp::Graphics::Color toEACP(juce::Colour colour)
{
    return {colour.getFloatRed(),
            colour.getFloatGreen(),
            colour.getFloatBlue(),
            colour.getFloatAlpha()};
}

inline juce::Point<float> toJUCE(const eacp::Graphics::Point& point)
{
    return {point.x, point.y};
}

inline juce::Rectangle<float> toJUCE(const eacp::Graphics::Rect& rect)
{
    return {rect.x, rect.y, rect.w, rect.h};
}

inline juce::Colour toJUCE(const eacp::Graphics::Color& colour)
{
    return juce::Colour::fromFloatRGBA(colour.r, colour.g, colour.b, colour.a);
}

} // namespace EACPJuce
