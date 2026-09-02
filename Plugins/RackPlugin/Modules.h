#pragma once

namespace RackPlugin
{

// The four modules, in the order they run in the chain and appear on screen.
enum class ModuleId
{
    drive,
    tone,
    space,
    width
};

inline constexpr auto moduleCount = 4;

// Everything about a module that both halves of the plugin have to agree on.
//
// It is a table rather than four blocks of code because everything downstream
// of it is a loop: the processor creates its parameters from this, the editor
// builds one panel per row, and each panel finds its own parameter, its own
// bypass and its own meter by the row it was handed. Adding a fifth module is
// a line here, a `define()`, and a case in makeModuleView.
struct ModuleSpec
{
    ModuleId id;

    // The two APVTS ids: the module's one continuous control, and its bypass.
    const char* paramId;
    const char* enableId;

    const char* title;

    // What this module's energy figure actually is. Worth spelling out in the
    // UI, because it is different in every module — see Processor.
    const char* meter;

    // The panel's accent, as plain bytes rather than a juce::Colour: this
    // header is included by the shaders, and they have no business knowing
    // about JUCE. It is here at all so a knob matches the picture above it —
    // four panels in a row look like one instrument or like four, and the
    // difference is entirely whether the chrome agrees with the pixels.
    unsigned char accent[3];
};

inline constexpr ModuleSpec moduleSpecs[moduleCount] = {
    {ModuleId::drive, "drive", "driveOn", "Drive", "saturation", {255, 138, 72}},
    {ModuleId::tone, "tone", "toneOn", "Tone", "balance", {120, 158, 255}},
    {ModuleId::space, "space", "spaceOn", "Space", "tail", {110, 222, 210}},
    {ModuleId::width, "width", "widthOn", "Width", "correlation", {236, 170, 90}},
};

} // namespace RackPlugin
