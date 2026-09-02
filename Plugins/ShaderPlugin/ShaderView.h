#pragma once

#include <eacp/GPU/GPU.h>

#include <functional>

namespace ShaderPlugin
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

// A shader written as a C++ struct rather than as a .metal file and a .hlsl
// file that have to be kept saying the same thing. define() records a graph of
// value handles, and eacp's emitters turn that one source into MSL for Metal
// and HLSL for D3D12 — so this example ships no shader assets at all, and the
// two backends cannot drift apart.
//
// The uniforms are named members. `level` is what makes this a plugin editor
// and not a screensaver: it carries the audio the processor is passing, so the
// picture answers to the sound.
struct PlasmaShader final : ShaderProgram
{
    // compile() has to run from the most-derived constructor: it walks the
    // uniforms EACP_SHADER names, then calls define() through the vtable.
    PlasmaShader() { compile(); }

    void define() override;

    Uniform<Float> time;
    Uniform<Float> level;
    Uniform<Float> aspect;

    EACP_SHADER(time, level, aspect)
};

// A GPUView is an eacp::Graphics::View that owns a swapchain and hands you a
// Frame per tick — which is the only reason this class can be dropped into a
// JUCE editor at all: EACPJuce::ViewComponent takes a View, and this is one.
//
// Continuous mode drives it from a display link, so update() runs once per
// display refresh and render() draws the frame. Nothing here runs on the audio
// thread and nothing here blocks it: the level arrives through levelSource,
// which reads one atomic the processor writes.
class ShaderView final : public GPUView
{
public:
    ShaderView();

    // Where the picture's energy comes from. Called once per rendered frame on
    // the main thread; the default reports silence.
    std::function<float()> levelSource = [] { return 0.f; };

    void update(eacp::Threads::FrameTime frameTime) override;
    void render(Frame& frame) override;

private:
    PlasmaShader shader;

    float elapsed = 0.f;
    float smoothedLevel = 0.f;
};

} // namespace ShaderPlugin
