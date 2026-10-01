juce_add_plugin(WebSynthPlugin
        PLUGIN_MANUFACTURER_CODE Eacp
        PLUGIN_CODE Wsyn
        COMPANY_NAME "eyalamirmusic"
        BUNDLE_ID "com.eacp.juce.websynthplugin"
        PRODUCT_NAME "EACP Web Synth"
        FORMATS AU VST3 Standalone
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(WebSynthPlugin PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        ParameterApi.cpp)

target_compile_definitions(WebSynthPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# The page is plain HTML, CSS and JavaScript — no package.json, no bundler —
# so it is embedded as it stands in the source tree. BASE_DIRECTORY keys each
# file by its path under Web/, which is the path the page asks for it by.
# JUCE_WEB_BROWSER stays 0 above: the web view here is eacp's, not JUCE's.
res_embed_add(WebSynthPlugin
        DIRECTORY      ${CMAKE_CURRENT_SOURCE_DIR}/Web
        BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Web
        NAMESPACE      WebSynthResources
        CATEGORY       WebSynthPlugin)

# eacp-webview is the one addition to the other examples' list: eacp_juce
# carries the graphics stack, and the web view is a module of its own.
#
# PUBLIC, not PRIVATE, and it matters. The web view embeds scripts of its own —
# the bridge shim among them — and ResEmbed registers them from a source it
# hands to whichever binary links last, not to the static libraries on the way.
# Here the static library is this target, JUCE's shared code, and the binaries
# are the format wrappers; a PRIVATE dependency of a static library reaches them
# as link-only, without that source, and the editor throws on first open.
target_link_libraries(WebSynthPlugin
        PRIVATE
        eacp_juce
        juce::juce_audio_utils
        juce_eacp_warnings
        PUBLIC
        eacp-webview
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
