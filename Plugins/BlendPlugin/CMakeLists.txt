juce_add_plugin(BlendPlugin
        PLUGIN_MANUFACTURER_CODE Eacp
        PLUGIN_CODE Blnd
        COMPANY_NAME "eyalamirmusic"
        BUNDLE_ID "com.eacp.juce.blendplugin"
        PRODUCT_NAME "EACP Blend"
        FORMATS AU VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(BlendPlugin PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        AuroraView.cpp)

target_compile_definitions(BlendPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(BlendPlugin
        PRIVATE
        eacp_juce
        juce::juce_audio_utils
        juce_eacp_warnings
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
