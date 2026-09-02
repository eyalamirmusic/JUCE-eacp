set(CMAKE_FOLDER JUCE-eacp)

# A JUCE module rather than a static library, so it is consumed the way every
# other piece of a JUCE plugin is: added to the module list, and compiled into
# whatever links it. That also settles the Objective-C question for us —
# juce_add_module builds eacp_juce.mm instead of eacp_juce.cpp on Apple, which
# is how the macOS half of the embedding gets to be Objective-C++ without the
# consumer configuring anything.
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/eacp_juce")

# The module target is an INTERFACE library, so everything its headers and
# sources need has to be INTERFACE too. eacp-gpu pulls the whole graphics stack
# in behind it, and carries the /Zc:preprocessor that EACP_SHADER needs on MSVC.
target_link_libraries(eacp_juce INTERFACE eacp-gpu)
