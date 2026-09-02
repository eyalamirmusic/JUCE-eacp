juce_add_plugin(ShaderPlugin
        PLUGIN_MANUFACTURER_CODE Eacp
        PLUGIN_CODE Shdr
        COMPANY_NAME "eyalamirmusic"
        BUNDLE_ID "com.eacp.juce.shaderplugin"
        PRODUCT_NAME "EACP Shader"
        FORMATS AU VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD ${JUCE_EACP_COPY_PLUGIN_AFTER_BUILD})

target_sources(ShaderPlugin PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        ShaderView.cpp)

target_compile_definitions(ShaderPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# eacp_juce carries eacp-gpu with it, so nothing here has to name the graphics
# stack. juce_audio_utils rather than juce_audio_processors alone: the
# Standalone format needs the device manager and its settings window.
target_link_libraries(ShaderPlugin
        PRIVATE
        eacp_juce
        juce::juce_audio_utils
        juce_eacp_warnings
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
