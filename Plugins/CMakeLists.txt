set(CMAKE_FOLDER Examples)

# The starting point: one JUCE plugin whose editor is an eacp GPU view running a
# shader, with a JUCE control beside it driving the same audio the shader reacts
# to.
add_subdirectory(ShaderPlugin)
