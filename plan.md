# Plan: JUCE's plugin demos, rebuilt on the eacp UI tier

## Goal

Reproduce the plugin examples JUCE ships in `examples/Plugins/` with their
processor code kept as-is and their editors rebuilt on eacp's UI module
(`eacp::UI`, the lightweight component tier), hosted inside the JUCE editor
through `EACPJuce::ViewComponent`. The point is to find out, demo by demo,
what the component tier is missing to be a plugin UI toolkit, and to fill it.

"As-is" means the audio side is untouched: the same `AudioProcessor`, the same
parameters, the same state. Only `createEditor()` changes.

## Where things stand (survey of 2026-09-02)

What already works, with no changes:

- `UI::ComponentHost` is a `GPU::GPUView`, so it goes into `ViewComponent`
  exactly as the shader views do. Nothing in the glue needs to change.
- The host converts native events into component-local ones: mouse, hover,
  drag capture, wheel, keyboard with Tab focus traversal. It grabs the
  keyboard on mouse down.
- `eacp::Threads::Timer(callback, hz)` and `Threads::callAsync` run under a
  foreign host loop by design (see `Core/Threads/EventLoop.h`). On macOS they
  need nothing; on Windows creating the `EmbeddedView` attaches the thread.
- Text: system families by name, `Text::registerMemoryFont` for embedded
  fonts, a default UI face per platform. Single-line drawing only.
- Vector shapes via `PathShape` (fill and stroke), gradients, shadows, images
  through `ImageCache`, group opacity through `Layer`.
- `libeacp-ui.a` is already built in this tree; eacp builds every module.
- eacp's `Rect` has `removeFromTop/Left/Right/Bottom` and `inset`, so JUCE
  `resized()` bodies port almost line for line.

The widget set today: `Label`, `Button` (momentary or latching), `Checkbox`,
`TextEditor` (single line), `Slider` (linear, normalised 0..1), `Knob`
(rotary, normalised, vertical drag), `ScrollPanel` (wheel only). Colours come
from one `Theme` struct. Every widget is `final` with fixed drawing.

## The demos, and what each one needs

| Demo | Processor class | Editor needs | Blockers |
| --- | --- | --- | --- |
| GainPluginDemo | `GainProcessor` (final) | `GenericAudioProcessorEditor` | generic editor |
| NoiseGatePluginDemo | `NoiseGate` (final) | `GenericAudioProcessorEditor` | generic editor |
| ArpeggiatorPluginDemo | `Arpeggiator` (final) | `GenericAudioProcessorEditor` | generic editor |
| MultiOutSynthPluginDemo | `MultiOutSynth` (final) | `GenericAudioProcessorEditor` | generic editor |
| AudioPluginDemo | `JuceDemoPluginAudioProcessor` (final) | 2 rotary sliders with captions, 30Hz timecode label, `MidiKeyboardComponent`, resizable with limits, size persisted, track-colour background | MIDI keyboard component, resize grip |
| SurroundPluginDemo | `ProcessorWithLevels` base is open, `SurroundProcessor` final | Custom level meters at 60Hz, a label per bus, a button per channel, resizable | none, all custom paint |
| DSPModulePluginDemo | `DspModulePluginDemo` base is open, `DspModulePluginDemoAudioProcessor` final | Effect ComboBox, a ComboBox per choice parameter, rotary sliders with an editable value box, toggles, right-click host parameter menu, fixed size | ComboBox, value box |
| MidiLoggerPluginDemo | `MidiLoggerPluginDemoProcessor` (final) | `TableListBox` with 4 columns, Clear button, timer refresh, resizable | list/table widget |
| SamplerPluginDemo | `SamplerAudioProcessor` (final) | Tabs, 7 ComboBoxes, toggles, buttons, file chooser, external file drag-and-drop, waveform with draggable loop markers, undo/redo | tabs, ComboBox; file drop cannot be reproduced |
| AUv3SynthPluginDemo | final | Material-styled button and slider, path icon, timer | custom-drawn widgets; optional, late |

Plugin characteristics to carry over into `juce_add_plugin`:

| Demo | Characteristics |
| --- | --- |
| Gain, NoiseGate, Surround, DSPModule | none (audio effect) |
| Arpeggiator, MidiLogger | `NEEDS_MIDI_INPUT`, `NEEDS_MIDI_OUTPUT`, `IS_MIDI_EFFECT` |
| MultiOutSynth, Sampler, AUv3Synth | `IS_SYNTH`, `NEEDS_MIDI_INPUT` |
| AudioPluginDemo | `IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_MIDI_OUTPUT`, `EDITOR_WANTS_KEYBOARD_FOCUS` |

MultiOutSynth, DSPModule, Sampler and AUv3Synth include
`examples/Assets/DemoUtilities.h`, which resolves asset files from JUCE's
examples folder at runtime. Each copy has to either point at that folder or
embed what it loads. Check per demo what is actually read.

Out of scope, and why:

- **WebViewPluginDemo**: the eacp WebView module, not the UI tier.
  `EACP_BUILD_WEBVIEW` is off here.
- **HostPluginDemo**: shows the hosted plugin's own JUCE editor.
- **ReaperEmbeddedViewPluginDemo**: REAPER's embedded-bitmap API.
- **ARAPluginDemo**: needs the ARA SDK, and is a 2,300-line document view
  with tooltips and viewports. Not a UI-tier test.

## Gaps

### In this repository

1. **Link the UI tier.** `Lib/eacp_juce/eacp_juce.h` declares
   `dependencies: juce_gui_basics eacp-gpu`. JUCE links those names as
   targets, so adding `eacp-ui` is the whole change.
2. **Parameter attachments.** eacp's Slider and Knob speak normalised 0..1,
   which is what `RangedAudioParameter` speaks, so attachments over
   `juce::ParameterAttachment` are short. They need gesture callbacks and a
   set-without-notify from eacp (see below).
3. **A generic parameter editor** covering four demos at once.
4. **A MIDI keyboard component** driving `juce::MidiKeyboardState`.
5. **Resizing.** JUCE's corner grip is a JUCE component and sits under the
   surface. Draw a grip in eacp that calls `setSize` on the editor, and pass
   `false` for the corner resizer.
6. **Host context menus and tooltips.** Keep `juce::PopupMenu` and
   `juce::TooltipWindow` as parentless desktop windows. They float above
   native surfaces, which is how `RackPlugin` already handles tooltips.

### In eacp

7. **ComboBox**, with its list drawn as an overlay inside the same host.
   Needed by DSPModule (12), Sampler (7) and the generic editor.
8. **Slider and Knob parity**: `onDragStart` / `onDragEnd` for automation
   gestures; a `notify` argument on `setValue`, the way `Checkbox` and
   `TextEditor` already have one, so a parameter update does not echo back;
   double-click to a default value (`clickCount` is already in the event).
9. **Tabs** and a **ListBox / table** widget.
10. **External file drops.** Nothing in Graphics or UI accepts them. Not
    planned; documented as the one Sampler feature that stays JUCE-side.
11. **Restyling.** Fixed drawing and a colours-only `Theme`. Not planned;
    AUv3Synth's look, if done, is custom components.

## Phases

Each phase ends with every format built, the Standalone run, and the plugin
opened in a DAW. Nothing is committed until the user says so.

### Phase 0: workspace and glue

- Stage a workspace so this build uses the sibling `../eacp` checkout
  (`develop`, currently four commits ahead of the CPM fetch, none of them
  UI) instead of fetching from GitHub. Invoke the `workspace` skill; do not
  configure from inside a subrepo.
- Add `eacp-ui` to the module's dependency line.
- Add `Lib/eacp_juce/Attachments/ParameterAttachments.h`:
  `KnobAttachment`, `SliderAttachment`, `ButtonAttachment` (latching Button),
  `CheckboxAttachment`, later `ComboBoxAttachment`. Each wraps a
  `juce::ParameterAttachment`, forwards `onDragStart` / `onDragEnd` to
  `beginGesture` / `endGesture`, and sets the widget without notifying on
  parameter change. Until Phase 1 lands, a re-entrancy guard stands in for
  the notify flag.
- Add `Lib/eacp_juce/Widgets/ResizeGrip.h`: a small eacp component in the
  bottom-right corner whose drag resizes the owning `AudioProcessorEditor`
  through its constrainer.
- Add `Lib/eacp_juce/Widgets/MidiKeyboard.h`: keys painted in eacp, mouse
  down/drag/up to `MidiKeyboardState`, computer-keyboard mapping when the
  component has focus, a `setVisible` for `hostMIDIControllerIsAvailable`.

### Phase 1: eacp widget parity (in `../eacp`)

- `Slider` / `Knob`: `onDragStart`, `onDragEnd`; `setValue(float, bool
  notify = false)` matching `Checkbox::setChecked`, with the mouse paths
  passing `true`; `setDefaultValue(std::optional<float>)` honoured on a
  double-click. Audit call sites in `Apps/UI` and `Tests/UI` for the changed
  default.
- `ComboBox`: items, selected index, `onChange`, keyboard up/down/return/
  escape. The popup is a component added to the host's root, covering the
  root so a click outside dismisses it, drawn with `paintOver` so it sits
  above everything. Opens upward when there is no room below; scrolls when
  the list is taller than the host.
- `TabBar` + `TabbedComponent`: a row of latching buttons and one visible
  page.
- `ListBox` with a row model (`getNumRows`, `paintRow`, row height,
  selection) over the ScrollPanel mechanism, and a header row for a table.
- Tests in `Tests/UI`: ComboBox open/select/dismiss, Slider notify and
  gesture semantics, ListBox selection.

### Phase 2: the four generic-editor demos

- `Lib/eacp_juce/Widgets/GenericEditor.h`: an `AudioProcessorEditor` hosting
  a `ComponentHost` whose root walks `processor.getParameterTree()`. Group
  headings as `Label`s; bool parameters as `Checkbox`; choice parameters as
  `ComboBox`; float and int as `Slider` with a name label and a value label
  fed by `getText(normalised)` plus the parameter's label suffix. Scrolls
  when taller than the window. Same fixed width JUCE's uses.
- `Plugins/JUCEDemos/Gain`, `NoiseGate`, `Arpeggiator`, `MultiOutSynth`:
  each a copy of the ISC-licensed PIP header with only the `createEditor`
  line changed, a `CMakeLists.txt` with `juce_add_plugin` carrying the
  characteristics above, and `juce_generate_juce_header` because the PIP
  headers rely on `using namespace juce` from `JuceHeader.h`.
- Choice parameters wait for the ComboBox from Phase 1; until then the
  generic editor shows them as a cycling button so the four demos build.

### Phase 3: AudioPluginDemo

- Copy the header; drop the JUCE editor class; keep the processor,
  `keyboardState`, the `uiState` size persistence and track-colour logic.
- Editor root: two `Knob`s with `Label` captions and attachments, a monospace
  `Label` updated by a 30Hz `Threads::Timer`, the `MidiKeyboard`, the
  `ResizeGrip`, background from the track colour via `toEACP`.
- `setResizable(true, false)` plus the eacp grip; `setResizeLimits` as JUCE.
- First real test of keyboard input in a host: the surface takes first
  responder on click, and the wrapper passes keys with
  `EDITOR_WANTS_KEYBOARD_FOCUS`. Test in AU, VST3 and Standalone, on macOS
  and Windows.

### Phase 4: SurroundPluginDemo

- Include the original header from JUCE's tree (via `JUCE_SOURCE_DIR`) and
  subclass `ProcessorWithLevels`, which is not final. No copy.
- `InputBusViewer` / `OutputBusViewer` as eacp components: meters painted
  from the processor's level array on a 60Hz timer, a `Label` per bus, a
  momentary `Button` per output channel calling `channelClicked`.
- Resizable, with the grip.

### Phase 5: DSPModulePluginDemo (after Phase 1)

- Subclass `DspModulePluginDemo`, which is not final; the JUCE final struct
  only adds `createEditor`.
- `AttachedSlider` = `Knob` + name `Label` + value `TextEditor` whose
  `onReturnKey` goes through `getValueForText` to
  `setValueAsCompleteGesture`. `AttachedToggle` = `Checkbox`.
  `AttachedCombo` = `ComboBox`.
- Effect `ComboBox` in the header, one control group per effect shown or
  hidden by selection.
- Right-click on a control: from the eacp `mouseDown` with `MouseButton::
  Right`, build the host menu with `getHostContext()->
  getContextMenuForParameter` and show it as a `juce::PopupMenu` with a
  screen-area target converted through the `ViewComponent`.
- Fixed size, as JUCE.

### Phase 6: MidiLoggerPluginDemo (after Phase 1)

- Copy the header. Table with four columns over the processor's
  `ValueTree` model, a Clear `Button`, timer refresh, resizable.

### Phase 7: SamplerPluginDemo (after Phase 1)

- Copy the header; the editor is the last third of it.
- Tabs for the sample page and the MPE settings page; the MPE pages are
  `ComboBox`es built by the demo's consecutive-integer helper, `Checkbox`es
  and `Button`s.
- Main view: Load, Undo, Redo `Button`s; centre-frequency `Slider`; loop-kind
  latching `Button` group; the waveform as custom components: a ruler,
  the waveform painted from `AudioThumbnail` data, draggable loop-point
  markers, a playback-position overlay on a timer, wheel zoom.
- File loading through `juce::FileChooser::launchAsync` from the eacp click,
  since some hosts forbid modal dialogs and eacp's `chooseFile` blocks.
- File drag-and-drop is not reproduced: a native surface takes the drag
  before JUCE's `FileDragAndDropTarget` sees it. Say so in the README.

### Phase 8: AUv3SynthPluginDemo (optional)

- Custom-drawn Material-style button and slider as eacp components, the
  icon as a `PathShape` converted from the JUCE binary path asset, Record
  button state on a timer. Only worth doing once the others are in.

## Cross-cutting decisions

- **Where code lives.** Reusable JUCE-facing pieces go in the module under
  `Lib/eacp_juce/Attachments/` and `Lib/eacp_juce/Widgets/`. Generic widgets
  with no JUCE in them go in eacp. Ported demos go in
  `Plugins/JUCEDemos/<Name>/`, each with its own `CMakeLists.txt`, listed in
  `Plugins/CMakeLists.txt` with a line saying what it exercises.
- **Formats.** AU, VST3 and Standalone, like the existing examples.
- **Threading.** All eacp UI on the message thread. Audio-to-UI through the
  atomics and fifos the existing examples use. Periodic UI through
  `Threads::Timer`; eacp repaints only what asked, so a meter repaints from
  its timer and nothing else does.
- **Copying headers.** The PIP headers are ISC-licensed; each copy keeps
  JUCE's licence block and states which JUCE version it came from (9.0.1)
  and what changed (the `createEditor` line, the removed editor class).
- **Verification per demo.** Build all formats; run Standalone; open in a
  DAW and check automation gestures are bracketed (touch/release recorded),
  the editor reopens cleanly, resizing tracks, hi-DPI is right on both
  platforms. Run pluginval if it is installed.
- **README.** A section on the ported demos and on what the exercise found
  the component tier was missing, once Phase 2 lands.

## Risks and open questions

- Keyboard focus inside hosts is untested for a component tree. Phase 3 is
  the early test; if a host does not route keys to the child view, the
  MIDI keyboard falls back to mouse only.
- Windows: the surface is a child HWND. Mouse capture across the surface
  edge and the eacp grip's drag need checking there specifically.
- The `setValue` notify default is a behaviour change in eacp. Audit call
  sites before landing it.
- A ComboBox popup is clipped to the surface. Fine for a plugin editor,
  but a combo at the very bottom of a fixed-size editor needs the open-upward
  path to work.
- eacp changes go to `develop`, which this repository tracks by tag. The
  workspace covers the gap; once merged, the plain CPM fetch picks them up.
