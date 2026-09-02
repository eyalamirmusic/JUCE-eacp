#pragma once

#include <eacp/GPU/GPU.h>

#include <functional>

namespace RackPlugin
{

// Inside this namespace only, so a shader reads the way eacp's own examples do
// — float4(), sin(), Uniform<Float> — rather than being three-quarters
// qualification.
using namespace eacp::GPU;

// One vertex of a full-screen quad. The struct is the vertex layout: define()
// pulls its fields with vertexInput(), so there is no second declaration of the
// same thing to keep in step.
struct Vertex
{
    float position[2];
};

// The uniforms every module's picture is built from, and the reason there are
// four shaders in this example but only one view class.
//
// A module has exactly four things to say to its shader, and they are the same
// four whichever module it is:
//
//   time    — seconds since the view opened, for anything that moves;
//   energy  — what the audio is doing *in that module*, 0 to 1. Each module
//             measures a different quantity; see the processor;
//   amount  — where that module's knob sits, normalised 0 to 1, so a shader
//             never has to know a parameter's units or range;
//   active  — 0 when the module is bypassed, 1 when it is not, faded rather
//             than switched so the picture dims instead of snapping.
//
// Plus `aspect`, which is the view's shape and not the audio's business at all.
//
// EACP_SHADER lives here rather than in the four subclasses because these are
// the members it names. A subclass adds a define() and nothing else, which is
// the whole point: what differs between the modules is the picture, and the
// picture is the one function.
struct ModuleShader : ShaderProgram
{
    Uniform<Float> time;
    Uniform<Float> energy;
    Uniform<Float> amount;
    Uniform<Float> active;
    Uniform<Float> aspect;

    EACP_SHADER(time, energy, amount, active, aspect)

protected:
    // The last line of all four define()s, and the only thing they have in
    // common besides their uniforms: the bypass fade and the final clamp.
    //
    // Worth factoring out rather than repeating, because it is a decision and
    // not a formality — a bypassed module keeps its picture and loses its
    // colour, which reads as "off" without going dark and leaving a hole in
    // the editor where a panel used to be.
    void setModuleFragment(const Float3& colour);
};

// How a module's energy figure is smoothed on its way to the shader.
//
// Not a detail: half the modules here report a *size* and half report a
// *position*, and the two want opposite treatment. A level wants a meter's
// asymmetry — instant on the way up so a transient reads as one, slow on the
// way down so it leaves a trail. A balance does not: it has no "up", and
// treating it as if it did would make the picture lurch to one side and ooze
// back, which is a lie about a figure that was never a peak.
enum class Ballistics
{
    // Fast to rise, slow to fall. For the modules whose figure is a level.
    Peak,

    // Symmetric. For the ones whose figure is a position between two ends.
    Balance
};

// Everything a module's view does that is not its define().
//
// A GPUView is an eacp::Graphics::View that owns a swapchain and hands you a
// Frame per tick — which is the only reason any of this can be dropped into a
// JUCE editor at all: EACPJuce::ViewComponent takes a View, and this is one.
//
// Nothing here runs on the audio thread and nothing here blocks it. The three
// sources below are called on the main thread once per rendered frame; each
// reads one atomic and returns.
class ModuleViewBase : public GPUView
{
public:
    // Where the picture's three inputs come from. The defaults are what a view
    // nobody has wired up shows: a lit but idle module.
    std::function<float()> energySource = [] { return 0.f; };
    std::function<float()> amountSource = [] { return 0.f; };
    std::function<float()> activeSource = [] { return 1.f; };

    void setBallistics(Ballistics newBallistics) { ballistics = newBallistics; }

    void update(eacp::Threads::FrameTime frameTime) override;
    void render(Frame& frame) override;

protected:
    // Hands the shader its geometry, builds its pipeline, and starts the view
    // animating. Called from the *derived* constructor, because that is the
    // first moment the shader member below exists — see ModuleView.
    void prepareShader();

    // The shader this module draws with. Supplied by the subclass that owns it.
    virtual ModuleShader& getShader() = 0;

private:
    Ballistics ballistics = Ballistics::Peak;

    float elapsed = 0.f;
    float smoothedEnergy = 0.f;
    float smoothedAmount = 0.f;
    float smoothedActive = 1.f;
};

// A module's view, for one shader.
//
// The two-step — a base holding the behaviour, a template holding the shader —
// is what lets four modules share one view class. It also puts prepareShader()
// where it has to be: the shader is a member of *this* class, so it is built by
// the time this constructor's body runs, and getShader() already dispatches
// here because the vtable is complete before a constructor body starts.
template <typename ShaderType>
class ModuleView final : public ModuleViewBase
{
public:
    ModuleView() { prepareShader(); }

private:
    ModuleShader& getShader() override { return shader; }

    ShaderType shader;
};

} // namespace RackPlugin
