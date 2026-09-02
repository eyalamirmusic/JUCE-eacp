#pragma once

#include <eacp/GPU/GPU.h>

#include <functional>

namespace BlendPlugin
{

// Inside this namespace only, so a shader reads the way eacp's own examples do
// — float3(), sin(), Uniform<Float> — rather than being three-quarters
// qualification.
using namespace eacp::GPU;

struct Vertex
{
    float position[2];
};

// Soft vertical curtains that drift and shimmer, brightening with the audio.
// Chosen for what this example is about rather than for its own sake: it is
// mostly dark, with light in a few wide, soft bands, so at a middling blend the
// JUCE drawing underneath stays legible through the gaps and is tinted rather
// than buried where the curtains are. A shader that filled the frame with solid
// colour would demonstrate the same compositing and show none of it.
struct AuroraShader final : ShaderProgram
{
    AuroraShader() { compile(); }

    void define() override;

    Uniform<Float> time;
    Uniform<Float> level;

    EACP_SHADER(time, level)
};

// A GPUView, so EACPJuce::ViewComponent can host it, with one thing added: a
// blend control.
//
// setBlend is the whole subject of this example, and it is three eacp calls.
// See the comment on it for why it is not just setOpacity.
class AuroraView final : public GPUView
{
public:
    AuroraView();

    // Where the picture's energy comes from. Called once per rendered frame on
    // the main thread; the default reports silence.
    std::function<float()> levelSource = [] { return 0.f; };

    // How much of this surface reaches the screen, 0 to 1.
    void setBlend(float amount);

    void update(eacp::Threads::FrameTime frameTime) override;
    void render(Frame& frame) override;

private:
    AuroraShader shader;

    float blend = 1.f;
    float elapsed = 0.f;
    float smoothedLevel = 0.f;
};

} // namespace BlendPlugin
