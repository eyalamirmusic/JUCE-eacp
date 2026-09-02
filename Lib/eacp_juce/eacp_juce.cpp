#include "eacp_juce.h"

// On Apple platforms this file is not the translation unit — eacp_juce.mm is,
// and it includes this one, so everything below is compiled as Objective-C++
// and the macOS half of native/ can speak to AppKit directly. juce_add_module
// drops this .cpp from the build there for exactly that reason.
#include "Embedding/ViewComponent.cpp"
