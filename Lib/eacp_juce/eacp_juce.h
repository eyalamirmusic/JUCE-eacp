/*
  ==============================================================================

   This file is part of JUCE-eacp, released under the MIT licence.

  ==============================================================================

  BEGIN_JUCE_MODULE_DECLARATION

    ID:                     eacp_juce
    vendor:                 eyalamirmusic
    version:                0.1.0
    name:                   eacp for JUCE
    description:            Hosts eacp views, and the eacp GPU stack, inside JUCE components.
    website:                https://github.com/eyalamirmusic/JUCE-eacp
    license:                MIT
    minimumCppStandard:     20

    dependencies:           juce_gui_basics
    OSXFrameworks:          Cocoa

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#define EACP_JUCE_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>

// The umbrella header, which brings View, EmbeddedView and the primitives with
// it. Nothing platform-specific reaches a consumer through it: every eacp type
// hides its backend behind a Pimpl, so no Cocoa or Win32 header is pulled into
// a plugin's translation units by including this module.
#include <eacp/Graphics/Graphics.h>

#include "Helpers/Conversions.h"
#include "Embedding/ViewComponent.h"
