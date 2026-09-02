#include "ModuleShaders.h"

namespace RackPlugin
{

void DriveShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    // Aspect correction, because this picture is made of diagonals and a panel
    // that is twice as wide as it is tall would shear them.
    auto p = float2(uv.x() * aspect, uv.y());

    // The signal the picture is made of: three travelling waves, summed so the
    // pattern never quite repeats.
    auto wave = sin(p.x() * 2.6f + time * 0.85f)
                + 0.8f * sin(p.y() * 3.4f - time * 0.65f)
                + 0.6f * sin((p.x() + p.y()) * 5.1f + time * 1.4f);

    // Pushed through the same shape the audio is — except that this is a fold
    // rather than a soft clip, and deliberately. A tanh flattens, and a flat
    // region is a picture that stops changing exactly where the interesting
    // part starts; a fold turns back on itself instead, so every extra decibel
    // of drive adds another visible band the way it adds another harmonic.
    auto pre = 0.5f + amount * 5.f;
    auto folded = abs(fract(wave * pre * 0.25f) * 2.f - 1.f);

    // How lit the fold is. `energy` is how hard the shaper is actually working
    // — not how loud the signal is — so a quiet passage through a lot of drive
    // still glows, and a loud one through none of it stays cold.
    auto heat = clamp(folded * (0.3f + energy * 1.6f), 0.f, 1.f);
    auto core = heat * heat * heat;

    // An ember ramp: near-black through deep red and orange, into white where
    // the fold is sharpest.
    auto colour = float3(
        heat * 1.15f + 0.03f, heat * heat * 0.72f + 0.02f, core * 0.85f + 0.04f);

    setModuleFragment(colour);
}

//==============================================================================
void ToneShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    // Frequency left to right, level bottom to top: the two axes a tone
    // control is about. No aspect correction here, unlike the module either
    // side of it, and that is not an omission — both axes mean something, so
    // stretching them is exactly what resizing the editor should do.
    auto x = uv.x() * 0.5f + 0.5f;
    auto y = uv.y() * 0.5f + 0.5f;

    // The knob, as the line it draws. 0.5 is flat; below that the band leans
    // into the bass, above it into the treble.
    auto tilt = amount * 2.f - 1.f;
    auto line = 0.5f + tilt * (x - 0.5f) * 0.72f;

    // Under the line is solid, over it is empty, and the edge between them is
    // a smoothstep rather than a step: at one pixel wide that is the whole
    // difference between a band and a row of jagged columns.
    auto body = 1.f - smoothstep(line - 0.012f, line + 0.012f, y);
    auto bars = 0.62f + 0.38f * sin(x * 42.f - time * 1.3f);

    // Where the energy actually ended up, measured after the filter: a bright
    // marker sliding along the band. Showing it beside the knob is the point —
    // the knob is the instruction and the marker is the result, and on real
    // material they are rarely in the same place.
    //
    // Faded upwards, so it reads as a marker standing on the band rather than
    // as a column crossing the whole panel.
    auto marker = exp(abs(x - energy) * -24.f) * (1.f - y * 0.55f);

    auto tint = mix(
        float3(constant(0.24f), 0.54f, 1.f), float3(constant(1.f), 0.62f, 0.22f), x);

    auto backdrop = float3(constant(0.032f), 0.038f, 0.058f)
                    + float3(constant(0.f), 0.018f, 0.05f) * (1.f - y);

    auto colour = backdrop + tint * body * bars * 0.5f + tint * marker * 0.45f
                  + float3(constant(1.f), 1.f, 1.f) * marker * body * 0.3f;

    setModuleFragment(colour);
}

//==============================================================================
void SpaceShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    auto p = float2(uv.x() * aspect, uv.y());
    auto radius = length(p);

    // Rings leaving the centre at a fixed speed. How far one gets before it
    // fades is the feedback, which is what the knob sets — so a short setting
    // is a ripple around the middle and a long one fills the panel.
    auto reach = 0.45f + amount * 1.1f;
    auto travel = fract(radius * 1.7f - time * 0.55f);
    auto ring = exp(travel * -4.f) * exp(radius * -1.f / reach);

    // Lit by the delay's own output, which is the whole reason this module
    // publishes a wet level and not an output level: stop playing and the
    // rings keep coming for as long as the tail is still audible, which is
    // information a meter on the output could not give.
    //
    // The constant is what silence looks like, and it is deliberately not near
    // zero: an editor sitting on a stopped transport should still show that it
    // is alive.
    auto glow = ring * (0.35f + energy * 1.5f);

    auto tint = mix(float3(constant(0.20f), 0.40f, 0.85f),
                    float3(constant(0.55f), 0.95f, 0.88f),
                    clamp(energy * 1.5f, 0.f, 1.f));

    // A soft core, so the middle of the panel is where the rings are coming
    // from rather than a hole they leave behind.
    auto centre = exp(radius * -5.5f) * (0.35f + energy * 0.9f);

    auto backdrop = float3(constant(0.026f), 0.032f, 0.052f)
                    + float3(constant(0.f), 0.014f, 0.036f) * (1.f - radius * 0.6f);

    auto colour =
        backdrop + tint * glow + float3(constant(0.88f), 0.96f, 1.f) * centre * 0.3f;

    setModuleFragment(colour);
}

//==============================================================================
void WidthShader::define()
{
    auto position = vertexInput(&Vertex::position);
    auto uv = varying(position);

    setPosition(float4(position, 0.f, 1.f));

    auto x = uv.x() * aspect;
    auto y = uv.y();

    // One beam per channel, pushed apart by the knob: at the bottom of the
    // range they sit on top of each other and the picture is as mono as the
    // signal is, at the top they are out at the edges.
    auto spread = amount * 0.75f;
    auto sway = sin(y * 3.2f + time * 1.05f) * 0.05f;

    auto left = exp(abs(x + spread + sway) * -6.5f);
    auto right = exp(abs(x - spread - sway) * -6.5f);

    // `energy` is the correlation between the two channels, mapped so that 1 is
    // mono, 0.5 is a decorrelated pair, and 0 is phase-inverted. That last case
    // is the one worth drawing: it is the failure a width control can actually
    // cause, it is inaudible until something folds the mix down to mono, and a
    // picture can say it before that happens. Under a half the beams go red.
    auto phase = clamp(energy * 2.f, 0.f, 1.f);

    auto warning = float3(constant(1.f), 0.22f, 0.26f);
    auto leftTint = mix(warning, float3(constant(0.32f), 0.68f, 1.f), phase);
    auto rightTint = mix(warning, float3(constant(1.f), 0.76f, 0.32f), phase);

    // Where the two beams overlap is what survives a fold-down, so the seam
    // between them is drawn as the brightest thing in the panel.
    auto centre = left * right;

    auto colour = float3(constant(0.028f), 0.032f, 0.05f) + leftTint * left * 0.75f
                  + rightTint * right * 0.75f
                  + float3(constant(1.f), 1.f, 1.f) * centre * 0.85f;

    setModuleFragment(colour);
}

//==============================================================================
std::unique_ptr<ModuleViewBase> makeModuleView(ModuleId id)
{
    auto view = std::unique_ptr<ModuleViewBase> {};

    switch (id)
    {
        case ModuleId::drive:
            view = std::make_unique<ModuleView<DriveShader>>();
            break;

        case ModuleId::tone:
            view = std::make_unique<ModuleView<ToneShader>>();
            break;

        case ModuleId::space:
            view = std::make_unique<ModuleView<SpaceShader>>();
            break;

        case ModuleId::width:
            view = std::make_unique<ModuleView<WidthShader>>();
            break;
    }

    // Which ballistics a module wants follows from what its figure *is*, which
    // is why it is decided here beside the shader rather than in the panel: two
    // of these are levels and want a meter's asymmetry, and two are positions
    // between two ends, for which "fast up, slow down" would be a lie.
    const auto isLevel = id == ModuleId::drive || id == ModuleId::space;
    view->setBallistics(isLevel ? Ballistics::Peak : Ballistics::Balance);

    return view;
}

} // namespace RackPlugin
