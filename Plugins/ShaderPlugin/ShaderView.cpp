#include "ShaderView.h"

#include <algorithm>

namespace ShaderPlugin
{
namespace
{
// Two triangles covering clip space. Every pixel of the view is therefore a
// fragment, which is where the whole picture is made — the vertex stage does
// nothing but hand the position through as a coordinate to draw from.
const Vertex quadVertices[] = {
    {{-1.f, -1.f}},
    {{1.f, -1.f}},
    {{-1.f, 1.f}},

    {{-1.f, 1.f}},
    {{1.f, -1.f}},
    {{1.f, 1.f}},
};
} // namespace

void PlasmaShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    // Aspect correction, so the rings stay round in an editor that is much
    // wider than it is tall.
    auto p = float2(uv.x() * aspect, uv.y());
    auto radius = length(p);

    // How hard the audio pushes the picture. The constant is what silence
    // looks like, and it is deliberately not near zero: an editor sitting on a
    // stopped transport should still show that it is alive.
    auto drive = 0.45f + 1.5f * level;

    // Three travelling waves at different speeds and angles. Summing them is
    // what stops the pattern from ever quite repeating.
    auto rings = sin(radius * 9.f - time * 2.4f)
                 + 0.7f * sin(p.x() * 5.f + time * 1.1f)
                 + 0.7f * sin(p.y() * 6.f - time * 0.8f);

    // Falls off towards the corners so the view has a centre rather than a
    // uniform wash.
    auto falloff = 1.f - smoothstep(0.f, 1.4f, radius);
    auto intensity = clamp((0.5f + 0.35f * rings) * falloff * drive, 0.f, 1.f);

    // Deep blue at rest, climbing through cyan into white as it brightens.
    auto colour = float3(intensity * intensity * 0.85f + 0.04f,
                         intensity * 0.6f + 0.05f,
                         intensity * 0.4f + 0.13f);

    setFragment(float4(colour, 1.f));
}

//==============================================================================
ShaderView::ShaderView()
{
    shader.setVertices(quadVertices);

    // The pipeline has to agree with the target it draws into, and MSAA is part
    // of that — sampleCount() is the view's answer, not the shader's.
    shader.prepare(sampleCount());

    // Animation: render() runs every display refresh, synchronized with vsync,
    // instead of only when something calls repaint().
    setContinuous(true);
}

void ShaderView::update(eacp::Threads::FrameTime frameTime)
{
    auto delta = static_cast<float>(frameTime.delta);
    elapsed += delta;

    // The processor publishes a peak per block, which on its own makes the
    // picture flicker at the block rate. Fast to rise so a transient still
    // reads as one, slow to fall so it leaves a trail.
    auto target = levelSource();
    auto perSecond = target > smoothedLevel ? 28.f : 3.5f;

    smoothedLevel += (target - smoothedLevel) * std::min(1.f, perSecond * delta);
}

void ShaderView::render(Frame& frame)
{
    auto bounds = getLocalBounds();

    shader.time = elapsed;
    shader.level = smoothedLevel;
    shader.aspect = bounds.h > 0.f ? bounds.w / bounds.h : 1.f;

    auto pass = frame.beginPass({eacp::Graphics::Color {0.02f, 0.02f, 0.05f}});
    pass.draw(shader);
}

} // namespace ShaderPlugin
