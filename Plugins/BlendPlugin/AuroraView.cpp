#include "AuroraView.h"

#include <algorithm>

namespace BlendPlugin
{
namespace
{
// Two triangles covering clip space, so every pixel of the view is a fragment.
const Vertex quadVertices[] = {
    {{-1.f, -1.f}},
    {{1.f, -1.f}},
    {{-1.f, 1.f}},

    {{-1.f, 1.f}},
    {{1.f, -1.f}},
    {{1.f, 1.f}},
};
} // namespace

void AuroraShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    auto x = uv.x();
    auto y = uv.y();

    // What silence looks like is deliberately not nothing: an editor on a
    // stopped transport should still show that the surface is alive, or the
    // blend slider looks broken at rest.
    auto drive = 0.42f + 1.35f * level;

    // Three curtains. Each is a vertical band whose centre snakes as it rises,
    // which is the whole shape of the effect — the rest is falloff and colour.
    // Different periods and drift speeds, so the three never line up twice.
    auto centre1 = sin(y * 1.7f + time * 0.55f) * 0.45f - 0.42f;
    auto centre2 = sin(y * 2.3f - time * 0.40f) * 0.38f + 0.08f;
    auto centre3 = sin(y * 1.1f + time * 0.70f) * 0.50f + 0.56f;

    auto band1 = 1.f - smoothstep(0.f, 0.34f, abs(x - centre1));
    auto band2 = 1.f - smoothstep(0.f, 0.26f, abs(x - centre2));
    auto band3 = 1.f - smoothstep(0.f, 0.30f, abs(x - centre3));

    // Bright along the bottom, thinning out towards the top, the way the real
    // thing hangs. Tall rather than a strip along the floor: at a middling
    // blend the dark parts of this surface are a grey wash over the JUCE panel,
    // so a shader that is mostly dark reads as "the left half is brighter"
    // rather than as two things sharing a rectangle.
    auto vertical = (1.f - smoothstep(-0.9f, 0.95f, y)) * smoothstep(-1.f, -0.9f, y);

    // Fine horizontal structure, so the curtains have grain rather than being
    // three smooth blurs.
    auto shimmer = 0.82f + 0.18f * sin(y * 13.f - time * 2.6f);

    auto energy = (band1 * 1.15f + band2 * 0.95f + band3 * 0.8f) * vertical;
    auto amount = clamp(energy * shimmer * drive, 0.f, 1.f);

    // Green at the base climbing to violet at the tips. Written as three mixes
    // of literals rather than a mix of two float3 constants, because the EDSL
    // needs a value to anchor a call and `height` is the value here.
    auto height = clamp(y * 0.5f + 0.5f, 0.f, 1.f);

    auto colour = float3(mix(0.06f, 0.62f, height),
                         mix(0.72f, 0.34f, height),
                         mix(0.52f, 0.95f, height));

    setFragment(float4(colour * amount, 1.f));
}

//==============================================================================
AuroraView::AuroraView()
{
    shader.setVertices(quadVertices);
    shader.prepare(sampleCount());

    setContinuous(true);
}

void AuroraView::setBlend(float amount)
{
    blend = std::clamp(amount, 0.f, 1.f);

    // Group opacity for the whole view, which the platform compositor applies
    // when it draws this surface over what is behind it — and what is behind it
    // is the JUCE peer's own layer, holding everything JUCE painted. That is
    // the entire mechanism: no readback, no shared texture, and nothing either
    // framework has to be told about the other.
    setOpacity(blend);

    // Not folded into the opacity, and not the same as opacity 0. A fully
    // transparent view is still a live view: the compositor keeps it in the
    // tree, the display link keeps waking, and render() keeps submitting GPU
    // work for a picture nobody can see. Taking it out of the tree costs one
    // call and gives all of that back.
    //
    // It also hands the mouse back. A native surface is hit-tested by the
    // platform before JUCE sees the event, so JUCE controls underneath one are
    // visible but not clickable — and a hidden surface is not hit-tested at
    // all, which is the difference between a shader over a panel and a shader
    // instead of one.
    const auto lit = blend > 0.f;

    if (lit != isVisible())
    {
        setVisible(lit);
        setContinuous(lit);
    }
}

void AuroraView::update(eacp::Threads::FrameTime frameTime)
{
    auto delta = static_cast<float>(frameTime.delta);
    elapsed += delta;

    // The processor publishes a peak per block, which on its own makes the
    // picture flicker at the block rate. Fast to rise so a transient still
    // reads as one, slow to fall so it leaves a trail.
    auto target = levelSource();
    auto perSecond = target > smoothedLevel ? 26.f : 3.2f;

    smoothedLevel += (target - smoothedLevel) * std::min(1.f, perSecond * delta);
}

void AuroraView::render(Frame& frame)
{
    shader.time = elapsed;
    shader.level = smoothedLevel;

    // Black rather than a dark blue, and it matters here in a way it does not
    // in the other examples. This colour is not a background — at blend 0.6 it
    // is a 60% black wash over the JUCE panel, so anything but black would tint
    // the whole picture rather than only the parts the curtains reach.
    auto pass = frame.beginPass({eacp::Graphics::Color {0.f, 0.f, 0.f}});
    pass.draw(shader);
}

} // namespace BlendPlugin
