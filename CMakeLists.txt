cmake_minimum_required(VERSION 3.22)
project(JohnPorkCall VERSION 0.2.0)

# JUCE is downloaded into a folder named JUCE next to this file (the GitHub workflow does this for you)
add_subdirectory(JUCE)

juce_add_plugin(JohnPorkCall
    COMPANY_NAME "JohnPorkCall"
    PLUGIN_MANUFACTURER_CODE Jpkc
    PLUGIN_CODE Jpc1
    FORMATS VST3
    PRODUCT_NAME "John Pork Call"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Picture and ringtone get built into the plugin (BinaryData.h)
juce_add_binary_data(JohnPorkAssets SOURCES
    background.png
    ringtone.wav)

target_sources(JohnPorkCall PRIVATE
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp)

target_compile_definitions(JohnPorkCall PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(JohnPorkCall
    PRIVATE
        JohnPorkAssets
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
