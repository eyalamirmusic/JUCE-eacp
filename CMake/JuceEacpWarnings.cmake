# Named JuceEacpWarnings rather than Warnings because a host project's
# CMAKE_MODULE_PATH is searched before its dependencies'. eacp includes
# `Warnings`-style modules from its own CMake/ directory, and a file of that
# name here would shadow one of theirs.
#
# The seam is deliberately thin. Everything this repository compiles is compiled
# into a JUCE target — the eacp_juce module has no translation unit of its own,
# it is #included into whatever links it — so a stricter set than JUCE's own
# headers build clean under would fire inside JUCE rather than inside our code.
# juce_recommended_warning_flags is the set that holds. This target exists so
# there is still one place to tighten if that ever changes.
add_library(juce_eacp_warnings INTERFACE)

target_link_libraries(juce_eacp_warnings INTERFACE
        juce::juce_recommended_warning_flags)
