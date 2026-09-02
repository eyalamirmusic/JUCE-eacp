#pragma once

#include "ModuleView.h"
#include "Modules.h"

#include <memory>

namespace RackPlugin
{

// The four shaders, which are four define()s and nothing else.
//
// That is the claim this example is making about the EDSL. A shader here is not
// a file of MSL and a file of HLSL kept saying the same thing, and it is not a
// string handed to a compiler at runtime — it is a C++ member function that
// records a graph of value handles, so four of them cost four function bodies
// and share every other line of their machinery through an ordinary base class.
// The uniforms, the geometry, the pipeline, the smoothing and the bypass fade
// are all in ModuleView; what is below is only the picture.
//
// Each reads the same five uniforms — see ModuleShader — and each is about the
// thing its module does to the sound:

// Drive: a wave field pushed through a fold, which is the same shape the audio
// takes. Turning the knob up folds it harder and the bands multiply, which is
// what the extra harmonics look like.
struct DriveShader final : ModuleShader
{
    DriveShader() { compile(); }

    void define() override;
};

// Tone: a band tilted by the knob, with the *measured* balance marked on it as
// a bright vertical. The two disagree constantly, which is the point: one is
// what you asked for and the other is what the music has.
struct ToneShader final : ModuleShader
{
    ToneShader() { compile(); }

    void define() override;
};

// Space: rings leaving the centre, one per repeat, reaching further as the
// feedback rises. Lit by the delay's own output rather than by the input, so
// the picture keeps pulsing after the playing stops — for exactly as long as
// the tail does.
struct SpaceShader final : ModuleShader
{
    SpaceShader() { compile(); }

    void define() override;
};

// Width: two beams, one per channel, pushed apart by the knob and tinted by
// the correlation between them. A width control's real hazard is phase, so the
// picture says so before a mono fold-down does.
struct WidthShader final : ModuleShader
{
    WidthShader() { compile(); }

    void define() override;
};

// The view for one module, ready to be shown by an EACPJuce::ViewComponent.
//
// The only place the four shader types are named. Everything in the editor is
// written against ModuleViewBase, which is why a panel can be built from a row
// of the spec table without knowing what is going to be drawn in it.
std::unique_ptr<ModuleViewBase> makeModuleView(ModuleId id);

} // namespace RackPlugin
