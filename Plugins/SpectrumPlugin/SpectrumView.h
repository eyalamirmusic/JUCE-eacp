#pragma once

#include "SpectrumAnalyser.h"

#include <eacp/GPU/GPU.h>

#include <functional>

namespace SpectrumPlugin
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

// The spectrum, as a shader.
//
// What is new here against the sibling example is the one uniform that is not a
// number: `spectrum` is a storage buffer, bound whole and subscripted by the
// fragment stage at an index it works out from where the pixel is. That is what
// carries a *curve* to the GPU rather than a level — 96 floats uploaded once a
// frame, read by every pixel of the view.
//
// The bin count itself is not a uniform, and does not need to be: the shader is
// C++, so SpectrumAnalyser::binCount is a constant expression here and lands in
// the generated MSL and HLSL as a literal.
struct SpectrumShader final : ShaderProgram
{
    // compile() has to run from the most-derived constructor: it walks the
    // uniforms EACP_SHADER names, then calls define() through the vtable.
    SpectrumShader() { compile(); }

    void define() override;

    // One float per display bin, 0 at the floor of the dB window and 1 at the
    // top of it. The picture is entirely this, plus what time it is.
    Uniform<InputBuffer> spectrum;
    Uniform<Float> time;
    Uniform<Float> level;

    EACP_SHADER(spectrum, time, level)
};

// A GPUView is an eacp::Graphics::View that owns a swapchain and hands you a
// Frame per tick — which is the only reason this class can be dropped into a
// JUCE editor at all: EACPJuce::ViewComponent takes a View, and this is one.
//
// It is also where the FFT runs. update() is called on the main thread once per
// display refresh, and it does three things in order: drain the audio thread's
// fifo, transform it, and upload the result to the GPU buffer the shader reads.
// Nothing here touches the audio thread and nothing here blocks it — see
// SpectrumAnalyser for why the transform is on this side of the fifo at all.
class SpectrumView final : public GPUView
{
public:
    explicit SpectrumView(SpectrumAnalyser& analyserToUse);

    // The two display controls, read once per rendered frame on the main
    // thread. The defaults are what a view nobody has wired up shows.
    std::function<float()> sensitivityDb = [] { return 0.f; };
    std::function<float()> falloffSeconds = [] { return 0.4f; };

    void update(eacp::Threads::FrameTime frameTime) override;
    void render(Frame& frame) override;

private:
    // Not owned: the analyser belongs to the processor, which outlives every
    // editor opened onto it.
    SpectrumAnalyser& analyser;

    SpectrumShader shader;

    // The one GPU resource this view owns beyond the shader's own. Allocated
    // once and rewritten each frame rather than recreated, which is what
    // Buffer::update is for.
    Buffer spectrum;

    float elapsed = 0.f;
};

} // namespace SpectrumPlugin
