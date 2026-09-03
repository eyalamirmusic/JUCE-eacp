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

    dependencies:           juce_gui_basics eacp-gpu

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

// Just the module's public headers: each one includes what it needs of JUCE and
// of eacp, so this file has nothing left to bring in. Include it to get the
// module, or include either header on its own — both compile from cold.
//
// Nothing platform-specific reaches a consumer through here, and nothing
// platform-specific is behind it either: every eacp type hides its backend
// behind a Pimpl, so no Cocoa or Win32 header is pulled into a plugin's
// translation units, and the module declares no frameworks of its own because
// it names no platform type — eacp links what its own surfaces need.
#include "Helpers/Conversions.h"
#include "Embedding/ViewComponent.h"
