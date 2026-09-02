cmake_minimum_required(VERSION 3.31)

# Plugin formats are MODULE libraries that link static libs from both frameworks,
# so every object in them has to be position-independent. Set before any target
# is defined, including the CPM-fetched dependencies configured below.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# eacp claims this itself, but only when it is the top-level project — which it
# never is here. Without it every binary inherits the build machine's SDK
# version, and a plugin built that way refuses to load on anything older. A
# -DCMAKE_OSX_DEPLOYMENT_TARGET=... on the command line still wins.
if (APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
            "Minimum macOS version" FORCE)
endif ()

project(JUCE-eacp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)

    # CMake already puts -lobjc on the link line of everything that pulls in
    # Objective-C, and Xcode adds -fobjc-link-runtime on top for any target with
    # its own .mm sources; ld warns about the duplicate. Mirrors eacp's own root.
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(CPM)

# eacp builds its Graphics and GPU modules on Apple and Windows only, and an
# editor here is a GPUView in a host window — so say that up front rather than
# letting the configure fail on a missing eacp-gpu target further down.
if (NOT APPLE AND NOT WIN32)
    message(FATAL_ERROR
            "JUCE-eacp needs eacp's GPU module, which builds on macOS, iOS and "
            "Windows. This platform is not one of them.")
endif ()

# eacp's WebView module links WebKit on macOS and downloads the WebView2 SDK on
# Windows. Nothing here embeds a web view, so it is off by default — a plain
# `set(... CACHE ...)` rather than a FORCE, so -DEACP_BUILD_WEBVIEW=ON still
# wins for anyone building a WebView editor alongside the GPU one.
set(EACP_BUILD_WEBVIEW OFF CACHE BOOL "Build eacp's WebView module")

# Groups every target the two frameworks bring in under one IDE folder
# (CLion/Xcode/VS); our own targets set their own below.
set(CMAKE_FOLDER External)
find_package(EACP REQUIRED)
find_package(JUCE REQUIRED)
unset(CMAKE_FOLDER)

# After find_package(JUCE): the warnings target defers to one of JUCE's.
include(JuceEacpWarnings)

add_subdirectory(Lib)

option(JUCE_EACP_ENABLE_EXAMPLES "Build the JUCE-eacp example plugins"
        ${PROJECT_IS_TOP_LEVEL})

# After each build, copy the built plugins into the OS plugin folders so a DAW
# picks them up immediately: ~/Library/Audio/Plug-Ins on macOS, the per-user
# VST3 folder on Windows. Same default as EACP-ClapWrapper, and the reason is
# the same: an example plugin nobody can open in a host is only half built.
option(JUCE_EACP_COPY_PLUGIN_AFTER_BUILD
        "Copy built plugins into the OS plugin folders after each build" ON)

if (JUCE_EACP_ENABLE_EXAMPLES)
    add_subdirectory(Plugins)
endif ()
