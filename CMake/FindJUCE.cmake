include(CPM)

# JUCE's own examples and extras are what most of its build time goes on, and
# nothing here uses them. They default off, but say so: a consumer that turned
# them on for its own JUCE copy would otherwise pull them into this build too.
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Build JUCE's example projects")
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Build JUCE's extra tools")

CPMAddPackage(
        NAME JUCE
        GITHUB_REPOSITORY juce-framework/JUCE
        GIT_TAG 9.0.1)
