// The Apple translation unit. juce_add_module compiles this instead of
// eacp_juce.cpp on macOS and iOS, which is how the module's sources get to be
// Objective-C++ without a consumer having to arrange anything.
#include "eacp_juce.cpp"
