set(CMAKE_FOLDER Examples)

# The starting point: one JUCE plugin whose editor is an eacp GPU view running a
# shader, with a JUCE control beside it driving the same audio the shader reacts
# to.
add_subdirectory(ShaderPlugin)

# The same editor with something real to draw: the incoming audio goes through
# an FFT, and the spectrum that comes out is what the shader reads — as a
# storage buffer rather than a uniform, since it is a curve and not a number.
add_subdirectory(SpectrumPlugin)
