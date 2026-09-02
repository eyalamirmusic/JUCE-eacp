# JUCE-eacp

JUCE plugins whose UI is [eacp](https://github.com/eyalamirmusic/eacp).

JUCE is the audio side of a plugin — the processor, the parameters, the formats,
the hosts. eacp is a C++20 framework that wraps the platform's own primitives:
native views, a Metal/D3D12 GPU stack with a shader EDSL, a component tier, a
web view. This repository joins the two, so a plugin keeps everything JUCE is
good at and draws its editor with eacp.

```
Lib/eacp_juce   the glue, as a JUCE module
Plugins/        example plugins
CMake/          CPM, and the dependency finders
```

## What the glue actually is

An eacp `View` is backed by a real platform surface — an `NSView` on macOS, a
composition-hosted `HWND` on Windows. eacp already knows how to put one of those
inside a window somebody else owns, because that is what a plugin editor is:
`eacp::Graphics::EmbeddedView` is that door. What it does not know is *where*
the surface should sit, because only JUCE knows that.

`EACPJuce::ViewComponent` is the join. It opens the door onto the peer's native
handle as soon as the component has a peer, and from then on keeps the surface
tracking the component's position — through moves, resizes, parent changes,
being hidden, and the editor being closed and reopened into a different host
window, which a DAW does routinely.

Using it is three lines of an editor:

```cpp
class Editor final : public juce::AudioProcessorEditor
{
    MyShaderView shaderView;                        // an eacp GPU::GPUView
    EACPJuce::ViewComponent shaderHost {shaderView};

public:
    explicit Editor(Processor& p)
        : AudioProcessorEditor(p)
    {
        addAndMakeVisible(shaderHost);
        setSize(680, 440);
    }

    void resized() override { shaderHost.setBounds(getLocalBounds()); }
};
```

Because the argument is an `eacp::Graphics::View`, none of this is specific to
the GPU: a `UI::ComponentHost`, a `WebView`, a `CameraView` or a plain `View`
that paints goes in the same slot.

One caveat, and it is the same one that applies to an OpenGL context or a web
view: a native surface draws over any JUCE component it overlaps, whatever the
z-order says. Give it its own rectangle and put JUCE widgets beside it, not on
it. Both examples do exactly that.

`Lib/eacp_juce/Helpers/Conversions.h` is the rest of the module: `toEACP` /
`toJUCE` for points, rectangles and colours, since both frameworks have all
three and an editor crosses between them constantly.

## The examples

Two, and the second is the first with something real to draw.

### `Plugins/ShaderPlugin`

A gain plugin whose editor is an animated GPU shader, with a JUCE slider beside
it driving the audio the shader reacts to.

The shader is a C++ struct, not a `.metal` file and a `.hlsl` file that have to
be kept saying the same thing — eacp's EDSL records a graph of value handles and
its emitters turn that one source into MSL and HLSL, so the example ships no
shader assets at all:

```cpp
struct PlasmaShader final : ShaderProgram
{
    PlasmaShader() { compile(); }

    void define() override
    {
        auto position = vertexInput(&Vertex::position);
        auto uv = varying(position);

        setPosition(float4(position, 0.f, 1.f));
        // ...rings, falloff, colour, all from `time` and `level`
    }

    Uniform<Float> time;
    Uniform<Float> level;
    Uniform<Float> aspect;

    EACP_SHADER(time, level, aspect)
};
```

`level` is what makes it a plugin editor rather than a screensaver: it carries
the output peak, so the picture answers to the sound. The channel between the
two threads is one relaxed atomic, written once per block by `processBlock` and
read once per rendered frame by the view's `update()` — no lock, no allocation,
nothing on the audio thread that can block.

### `Plugins/SpectrumPlugin`

A visualizer: the incoming audio goes through an FFT, and the spectrum that
comes out is what the shader draws. The audio itself is untouched — this one is
a tap, so its two parameters shape the picture rather than the sound.

A spectrum is a curve rather than a number, which is the one thing about the GPU
side that differs. It does not fit in a uniform, so it goes to the shader as a
storage buffer, bound whole and subscripted by the fragment stage at an index it
works out from where the pixel is:

```cpp
struct SpectrumShader final : ShaderProgram
{
    void define() override
    {
        // ...x from the varying, then the two bins either side of it
        auto height = mix(spectrum[index], spectrum[index + 1u], slot - lower);
        // ...fill, bloom and ridge, all from where the pixel sits against it
    }

    Uniform<InputBuffer> spectrum;   // one float per display bin
    Uniform<Float> time;
    Uniform<Float> level;

    EACP_SHADER(spectrum, time, level)
};
```

The bin count is not a uniform and does not need to be: the shader is C++, so
the constant lands in the generated MSL and HLSL as a literal.

The channel between the threads is a lock-free SPSC fifo carrying **samples**,
not magnitudes, because the transform runs on the render thread — once per
displayed frame, from the view's `update()`. That leaves the audio thread doing
a sum and a copy whatever the transform size is, refreshes the picture as often
as it is drawn rather than as often as a block arrives, and costs a display
refresh a few tens of microseconds. `Plugins/SpectrumPlugin/SpectrumAnalyser.h`
is where that argument is written down.

Formats built for both: AU, VST3 and Standalone.

## Building

```
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Dependencies (eacp and JUCE) are fetched by CPM; nothing needs installing first.

| Option | Default | |
| --- | --- | --- |
| `JUCE_EACP_ENABLE_EXAMPLES` | on when top-level | Build `Plugins/` |
| `EACP_BUILD_WEBVIEW` | `OFF` here | eacp's WebView module — turn on for a WebView editor |

## Supported platforms

macOS and Windows. The line is eacp's, not this repository's: eacp's Graphics
and GPU modules wrap each platform's own compositor rather than shipping one,
and they build on macOS, Windows and iOS. Configuring anywhere else fails with a
message saying so rather than dying on a missing target.

## Using it in your own plugin

```cmake
CPMAddPackage(
        NAME JUCE-eacp
        GITHUB_REPOSITORY eyalamirmusic/JUCE-eacp
        GIT_TAG main)

target_link_libraries(MyPlugin PRIVATE eacp_juce)
```

`eacp_juce` carries `eacp-gpu` — and so the whole eacp graphics stack — behind
it, so nothing else needs naming. `JUCE_EACP_ENABLE_EXAMPLES` defaults off for a
consumer, so the example plugins do not land in your build tree.

## Licence

MIT.
