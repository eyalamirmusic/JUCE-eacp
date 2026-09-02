juce_add_plugin(SpectrumPlugin
        PLUGIN_MANUFACTURER_CODE Eacp
        PLUGIN_CODE Spec
        COMPANY_NAME "eyalamirmusic"
        BUNDLE_ID "com.eacp.juce.spectrumplugin"
        PRODUCT_NAME "EACP Spectrum"
        FORMATS AU VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(SpectrumPlugin PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        SpectrumAnalyser.cpp
        SpectrumView.cpp)

target_compile_definitions(SpectrumPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# juce_dsp is the one line that separates this from the ShaderPlugin's list: the
# FFT and the Hann window come from it. eacp_juce still carries eacp-gpu with
# it, so the graphics stack goes unnamed here as it does there.
target_link_libraries(SpectrumPlugin
        PRIVATE
        eacp_juce
        juce::juce_audio_utils
        juce::juce_dsp
        juce_eacp_warnings
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
