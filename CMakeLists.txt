cmake_minimum_required(VERSION 3.31)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (PROJECT_IS_TOP_LEVEL)
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
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(CPM)
set(EACP_BUILD_WEBVIEW OFF CACHE BOOL "Build eacp's WebView module")

set(CMAKE_FOLDER External)
find_package(EACP REQUIRED)
find_package(JUCE REQUIRED)
unset(CMAKE_FOLDER)

include(JuceEacpWarnings)

add_subdirectory(Lib)

option(JUCE_EACP_ENABLE_EXAMPLES "Build the JUCE-eacp example plugins"
        ${PROJECT_IS_TOP_LEVEL})

if (JUCE_EACP_ENABLE_EXAMPLES)
    add_subdirectory(Plugins)
endif ()
