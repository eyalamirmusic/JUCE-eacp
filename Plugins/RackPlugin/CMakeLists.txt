juce_add_plugin(RackPlugin
        PLUGIN_MANUFACTURER_CODE Eacp
        PLUGIN_CODE Rack
        COMPANY_NAME "eyalamirmusic"
        BUNDLE_ID "com.eacp.juce.rackplugin"
        PRODUCT_NAME "EACP Rack"
        FORMATS AU VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(RackPlugin PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        ModulePanel.cpp
        ModuleView.cpp
        ModuleShaders.cpp)

target_compile_definitions(RackPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# The same list the ShaderPlugin has, and for the same reasons: eacp_juce
# carries eacp-gpu with it, so the graphics stack goes unnamed, and
# juce_audio_utils rather than juce_audio_processors alone because the
# Standalone format needs the device manager and its settings window. Four
# shaders need no more of a build than one does — they are C++.
target_link_libraries(RackPlugin
        PRIVATE
        eacp_juce
        juce::juce_audio_utils
        juce_eacp_warnings
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
