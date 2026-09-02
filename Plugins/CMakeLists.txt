set(CMAKE_FOLDER Examples)

# The starting point: one JUCE plugin whose editor is an eacp GPU view running a
# shader, with a JUCE control beside it driving the same audio the shader reacts
# to.
add_subdirectory(ShaderPlugin)

# The same editor with something real to draw: the incoming audio goes through
# an FFT, and the spectrum that comes out is what the shader reads — as a
# storage buffer rather than a uniform, since it is a curve and not a number.
add_subdirectory(SpectrumPlugin)

# Both of the above put one surface in an editor. This one puts four, inside a
# UI made of a dozen JUCE widgets: a rack of modules, each with its own shader,
# its own knob, its own bypass and its own meter — and a layout that reflows
# under the host's resize, so all four surfaces move at once.
add_subdirectory(RackPlugin)

# All three above keep the two frameworks in separate rectangles, because a
# native surface draws over any JUCE component it overlaps. This one puts them
# in the same rectangle instead: the surface is given a group opacity, so the
# JUCE panel underneath composites through the shader, and a slider sweeps
# between the two while both halves keep animating.
add_subdirectory(BlendPlugin)
