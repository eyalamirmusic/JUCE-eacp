#include "ModuleView.h"

#include <algorithm>

namespace RackPlugin
{
namespace
{
// Two triangles covering clip space. Every pixel of the view is therefore a
// fragment, which is where the whole picture is made — the vertex stage does
// nothing but hand the position through as a coordinate to draw from.
//
// One copy for all four shaders: they differ in what they compute per pixel,
// never in which pixels they get.
const Vertex quadVertices[] = {
    {{-1.f, -1.f}},
    {{1.f, -1.f}},
    {{-1.f, 1.f}},

    {{-1.f, 1.f}},
    {{1.f, -1.f}},
    {{1.f, 1.f}},
};

// A first-order approach to `target`, at a rate given per second rather than
// per frame. The clamp is what keeps it stable if a frame runs long: without
// it a delta big enough to make the step exceed 1 overshoots and rings.
float approach(float current, float target, float perSecond, float delta)
{
    return current + (target - current) * std::min(1.f, perSecond * delta);
}
} // namespace

void ModuleShader::setModuleFragment(const Float3& colour)
{
    // Rec.601 luma, which is the desaturation everybody's eye already reads as
    // "greyed out" — an unweighted average of the three channels turns a warm
    // picture muddy rather than grey.
    auto grey = dot(colour, float3(constant(0.299f), 0.587f, 0.114f));

    auto faded = mix(float3(grey, grey, grey) * 0.5f, colour, active);

    setFragment(float4(clamp(faded, 0.f, 1.f), 1.f));
}

//==============================================================================
void ModuleViewBase::prepareShader()
{
    auto& shader = getShader();

    shader.setVertices(quadVertices);

    // The pipeline has to agree with the target it draws into, and MSAA is part
    // of that — sampleCount() is the view's answer, not the shader's.
    shader.prepare(sampleCount());

    // Animation: render() runs every display refresh, synchronized with vsync,
    // instead of only when something calls repaint().
    setContinuous(true);

    // The one line this example adds that the single-surface ones do not need.
    //
    // Four continuous views are four display links, and on a 120Hz panel that
    // is 480 draws a second for four backdrops that nobody is reading frame by
    // frame. maxFps caps the work without touching the animation: the link
    // still wakes at every refresh, the skipped ticks are folded into the next
    // frame's delta, and everything here is delta-scaled, so 60 looks like 120
    // and costs half as much.
    setMaxFps(60);
}

void ModuleViewBase::update(eacp::Threads::FrameTime frameTime)
{
    const auto delta = static_cast<float>(frameTime.delta);
    elapsed += delta;

    const auto energy = energySource();

    // A level gets a meter's asymmetry; a balance gets neither end of it. See
    // Ballistics.
    if (ballistics == Ballistics::Peak)
    {
        const auto perSecond = energy > smoothedEnergy ? 26.f : 3.2f;
        smoothedEnergy = approach(smoothedEnergy, energy, perSecond, delta);
    }
    else
    {
        smoothedEnergy = approach(smoothedEnergy, energy, 6.f, delta);
    }

    // The knob and the bypass are both smoothed too, and for the same reason
    // the audio side smooths them: a parameter arrives as a step, and a step is
    // the one thing a picture cannot make look deliberate.
    smoothedAmount = approach(smoothedAmount, amountSource(), 12.f, delta);
    smoothedActive = approach(smoothedActive, activeSource(), 7.f, delta);
}

void ModuleViewBase::render(Frame& frame)
{
    const auto bounds = getLocalBounds();
    auto& shader = getShader();

    shader.time = elapsed;
    shader.energy = smoothedEnergy;
    shader.amount = smoothedAmount;
    shader.active = smoothedActive;
    shader.aspect = bounds.h > 0.f ? bounds.w / bounds.h : 1.f;

    auto pass = frame.beginPass({eacp::Graphics::Color {0.02f, 0.02f, 0.04f}});
    pass.draw(shader);
}

} // namespace RackPlugin
