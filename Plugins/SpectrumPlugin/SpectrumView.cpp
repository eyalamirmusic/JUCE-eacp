#include "SpectrumView.h"

namespace SpectrumPlugin
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

constexpr auto binCount = SpectrumAnalyser::binCount;
constexpr auto spectrumBytes = sizeof(float) * (size_t) binCount;
} // namespace

void SpectrumShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    // Clip space is -1..1 with y up. The spectrum is easier to write in the
    // space it is actually in: x is frequency, left to right, and y is level,
    // floor to ceiling — both 0 to 1.
    //
    // There is no aspect uniform here, unlike the sibling example, and that is
    // not an omission: nothing in this picture is a circle. Both axes mean
    // something, and stretching one of them is what resizing the editor is
    // supposed to do.
    auto x = uv.x() * 0.5f + 0.5f;
    auto y = uv.y() * 0.5f + 0.5f;

    // Where this column falls among the display bins, and the two bins either
    // side of it. Mixing between them is what stops the curve being a visible
    // staircase of `binCount` steps once the view is wider than that.
    auto slot = clamp(x, 0.f, 1.f) * (float) (binCount - 1);
    auto lower = floor(slot);
    auto index = toUInt(lower);

    // The trailing factor is headroom: a bin at full scale would otherwise put
    // its ridge exactly on the top edge, and lose the half of its bloom that
    // reaches past it.
    auto height = mix(spectrum[index],
                      spectrum[min(index + 1u, (unsigned) (binCount - 1))],
                      slot - lower)
                  * 0.93f;

    // Everything below is built from where the pixel sits relative to that
    // curve. Positive is above it, in the empty part of the picture.
    auto above = y - height;

    // Three layers, added rather than composited, because they are light:
    //
    //   fill  — the body under the curve, brightest where it meets the floor;
    //   bloom — a glow that only reaches upwards, and only as far as the bin
    //           is loud, so a peak throws light and a quiet band does not;
    //   ridge — the lit edge on the curve itself, narrow and close to white.
    //
    // The fill's edge is a smoothstep rather than a step: at one pixel wide it
    // is the difference between a curve and a comb of jagged columns.
    auto edge = 0.005f;
    auto fill = 1.f - smoothstep(height - edge, height + edge, y);
    auto bloom = exp(max(above, 0.f) * -13.f) * height;
    auto ridge = exp(abs(above) * -70.f);

    // Colour by frequency rather than by level, so a glance at the picture says
    // *where* the energy is: deep blue at the bottom of the range, through
    // magenta, into amber at the top.
    auto tint = mix(float3(constant(0.16f), 0.42f, 1.f),
                    float3(constant(0.92f), 0.24f, 0.86f),
                    smoothstep(0.f, 0.62f, x));

    tint = mix(tint, float3(constant(1.f), 0.72f, 0.26f), smoothstep(0.62f, 1.f, x));

    // A slow wash across the background, brightened by the overall level. Like
    // the sibling example's silent-but-alive constant, this is what an editor
    // sitting on a stopped transport shows instead of a black rectangle.
    auto wash = sin(x * 5.f - time * 0.7f + y * 2.f) * 0.5f + 0.5f;
    auto backdrop =
        float3(constant(0.030f), 0.034f, 0.055f)
        + float3(constant(0.02f), 0.03f, 0.08f) * wash * (level * 0.7f + 0.3f);

    auto colour = backdrop + tint * fill * (1.f - y * 0.55f) * 0.55f
                  + tint * bloom * 0.4f
                  // The ridge desaturates towards white at its centre, which is
                  // what reads as "lit" rather than "a brighter blue".
                  + mix(tint, float3(constant(1.f), 1.f, 1.f), 0.55f) * ridge;

    setFragment(float4(clamp(colour, 0.f, 1.f), 1.f));
}

//==============================================================================
SpectrumView::SpectrumView(SpectrumAnalyser& analyserToUse)
    : analyser(analyserToUse)
    , spectrum(Device::shared().makeBuffer(spectrumBytes, BufferUsage::Storage))
{
    shader.setVertices(quadVertices);

    // Bound once: the uniform holds a pointer to the buffer, not a copy of it,
    // so every update() below is seen by the next draw without rebinding.
    shader.spectrum = spectrum;

    // The pipeline has to agree with the target it draws into, and MSAA is part
    // of that — sampleCount() is the view's answer, not the shader's.
    shader.prepare(sampleCount());

    // Animation: render() runs every display refresh, synchronized with vsync,
    // instead of only when something calls repaint().
    setContinuous(true);
}

void SpectrumView::update(eacp::Threads::FrameTime frameTime)
{
    auto delta = static_cast<float>(frameTime.delta);
    elapsed += delta;

    // The FFT, on the main thread, once per displayed frame. Everything the
    // audio thread contributed is already in the fifo; this is what turns it
    // into the curve.
    analyser.analyse(delta, sensitivityDb(), falloffSeconds());
}

void SpectrumView::render(Frame& frame)
{
    // One upload per displayed frame, which is the rate Buffer::update asks to
    // be called at — it reuses the GPU allocation rather than pacing against
    // frames still in flight.
    spectrum.update(analyser.getBins().data(), spectrumBytes);

    shader.time = elapsed;
    shader.level = analyser.getLevel();

    auto pass = frame.beginPass({eacp::Graphics::Color {0.02f, 0.02f, 0.04f}});
    pass.draw(shader);
}

} // namespace SpectrumPlugin
