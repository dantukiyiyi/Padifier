cmake_minimum_required(VERSION 3.22)
project(Padifier VERSION 1.0.0)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Padifier
    COMPANY_NAME "Dantuki"
    PLUGIN_MANUFACTURER_CODE Dntk
    PLUGIN_CODE Pdfr
    PRODUCT_NAME "Padifier"
    BUNDLE_ID com.dantuki.padifier
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(Padifier PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_compile_definitions(Padifier PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Padifier PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
)
