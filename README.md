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
it. The first three examples do exactly that — the third one four times over,
with the surface a tile in the middle of each panel and the panel's widgets
arranged around it.

"Draws over", though, is the compositor's answer for an *opaque* surface, and a
surface does not have to be opaque. `eacp::Graphics::View::setOpacity` is group
opacity for a whole view — chrome, children and GPU content — and what it
composites over is the layer behind, which in a plugin is the JUCE peer's own
layer, holding everything JUCE just painted. Turn it down and the JUCE panel
comes through the shader, still repainting, with no readback and no shared
texture. The fourth example is built on that. What it does not buy back is the
mouse: the platform hit-tests the surface before JUCE sees the event, so a JUCE
control under a visible surface can be seen and not touched.

`Lib/eacp_juce/Helpers/Conversions.h` is the rest of the module: `toEACP` /
`toJUCE` for points, rectangles and colours, since both frameworks have all
three and an editor crosses between them constantly.

## The examples

Four. The second is the first with something real to draw; the third is both of
them four times over, inside a UI that looks like a plugin; the fourth stops
giving the surface a rectangle of its own.

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

### `Plugins/RackPlugin`

Both of the above put one surface in an editor. This one puts four, inside a UI
made of a dozen JUCE widgets: a rack of four modules — Drive, Tone, Space,
Width — each with its own shader tile, its own knob, its own bypass and its own
meter, plus an output trim in the header.

Nothing in `ViewComponent` needed changing for that. It watches the component
hierarchy, so a panel moving moves the surface inside it; four of them is four
watchers, and that they share a peer is not something any of them has to know.
The layout reflows between a row of four and a grid of two by two as the host
resizes the window, which moves and resizes all four surfaces at once.

The four shaders are four `define()`s and nothing else. They share their
uniforms, their geometry, their pipeline setup, their smoothing and their bypass
fade through an ordinary base class, because the shader is a C++ member function
rather than a file of MSL and a file of HLSL:

```cpp
struct ModuleShader : ShaderProgram
{
    Uniform<Float> time;
    Uniform<Float> energy;   // what this module's audio is doing
    Uniform<Float> amount;   // where its knob sits, normalised 0..1
    Uniform<Float> active;   // 0 bypassed, 1 not — faded, not switched
    Uniform<Float> aspect;

    EACP_SHADER(time, energy, amount, active, aspect)
};

struct DriveShader final : ModuleShader { void define() override; /* ... */ };
```

One view class serves all four (`ModuleView<ShaderType>`), and one table in
`Modules.h` drives everything downstream of it: the processor creates its
parameters from that table, the editor builds one panel per row, and each panel
finds its shader, its parameter, its bypass, its meter and its accent colour by
the row it was handed. The editor never names a module.

What is worth reading on the audio side, unusually, is the processor. A
visualizer publishes a level, and a level is the same quantity wherever it is
taken from; four modules have four different things to say, and saying them is
what makes four pictures rather than one picture repeated:

| Module | What its meter is |
| --- | --- |
| Drive | saturation — how far the shaper bent the loudest sample off a straight line |
| Tone | balance — where the energy sits, low to high, after the filter |
| Space | tail — the delay's *own* output, so it outlives the input by the length of the repeats |
| Width | correlation between the output channels, and a red picture when it goes negative |

Two of those are levels and get a meter's asymmetry; two are positions between
two ends, for which "fast up, slow down" would be a lie. `Ballistics` is that
distinction, and it is picked beside the shader rather than in the panel.

The one line this example needs that the single-surface ones do not is
`setMaxFps(60)`: four continuous views are four display links, and on a 120Hz
panel that is 480 draws a second for four backdrops. The cap costs nothing
visible — the skipped ticks fold into the next frame's delta and everything here
is delta-scaled.

### `Plugins/BlendPlugin`

The other three keep the two frameworks in separate rectangles. This one puts
them in the same rectangle.

The stage is a single JUCE drawing — a grid, five rings, a level's worth of
spokes and a sweep hand — painted edge to edge, and the eacp surface covers the
right half of it. The rings are centred on the seam, so every curve runs out
from under the shader into the open and the two halves can be compared on the
same line. `Blend` sweeps the right half between them:

```cpp
void AuroraView::setBlend(float amount)
{
    blend = std::clamp(amount, 0.f, 1.f);

    setOpacity(blend);          // the whole mechanism

    const auto lit = blend > 0.f;

    if (lit != isVisible())
    {
        setVisible(lit);        // not the same as opacity 0
        setContinuous(lit);
    }
}
```

`setVisible` is there because a fully transparent view is still a live view: the
compositor keeps it in the tree and the display link keeps waking to render a
picture nobody can see. It also hands the mouse back, a hidden surface not being
hit-tested.

Everything JUCE paints under the surface keeps animating while it is under
there — this is the OS compositing two live layers, not a picture of one pasted
over the other — and `blend` is an ordinary automatable parameter, so the
opacity is plugin state a host can automate and a preset can carry.

Two things it does not get you, and the example is arranged around both:

- **Uniform opacity, not a per-pixel alpha channel.** The surface fades as one.
  A shader writing alpha per fragment does not punch holes in itself: on macOS
  the `CAMetalLayer` behind a `GPUView` is opaque, so its alpha channel is
  discarded, and eacp has no switch for that. Which is why the aurora is mostly
  dark and the stage is painted brighter than the header — at blend 0.5 the
  shader's black background is a 50% black wash over the JUCE drawing, and
  anything drawn at the header's contrast would vanish under it.
- **No mouse.** Both sliders live in the strip below the stage, not on it.

Formats built for all four: AU, VST3 and Standalone.

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
