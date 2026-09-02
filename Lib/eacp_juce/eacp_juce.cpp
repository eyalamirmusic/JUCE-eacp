#ifdef EACP_JUCE_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file
   where you've already included any other headers - just put it inside a file on
   its own, possibly with your config flags preceding it, but don't include
   anything else. That also includes avoiding any automatic prefix header files
   that the compiler may be using. */
#error "Incorrect use of JUCE cpp file"
#endif

#include "eacp_juce.h"

// On Apple platforms this file is not the translation unit — eacp_juce.mm is,
// and it includes this one, so everything below is compiled as Objective-C++
// and the macOS half of native/ can speak to AppKit directly. juce_add_module
// drops this .cpp from the build there for exactly that reason.
#include "Embedding/ViewComponent.cpp"
