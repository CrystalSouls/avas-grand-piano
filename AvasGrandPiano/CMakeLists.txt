cmake_minimum_required(VERSION 3.22)

# macOS: build one plugin that runs on both Apple Silicon and Intel Macs.
# (Must be set before project().)
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal build")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Oldest supported macOS")
endif()

project(AvasGrandPiano VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AVA_COPY_AFTER_BUILD "Copy the finished plugin into your plugin folder after building" OFF)
option(AVA_BUILD_TESTS "Build developer test tools (not needed to use the plugin)" OFF)

# JUCE is the framework that turns our code into a real VST3 / AU plugin.
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.16-security
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# The kawaii artwork is baked into the plugin so it is a single self-contained file.
juce_add_binary_data(AvaAssets SOURCES
    Assets/ava_girl.svg
    Assets/background.svg)

set(AVA_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND AVA_FORMATS AU)   # AU is what GarageBand and Logic use
endif()

juce_add_plugin(AvasGrandPiano
    COMPANY_NAME "Ava Studio"
    BUNDLE_ID com.avastudio.avasgrandpiano
    PLUGIN_MANUFACTURER_CODE AvaS
    PLUGIN_CODE AvGp
    FORMATS ${AVA_FORMATS}
    PRODUCT_NAME "Avas Grand Piano"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${AVA_COPY_AFTER_BUILD}
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

target_sources(AvasGrandPiano PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(AvasGrandPiano PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(AvasGrandPiano
    PRIVATE
        AvaAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(AVA_BUILD_TESTS)
    # Draws the plugin window to a PNG without needing a screen.
    juce_add_console_app(AvaSnapshot PRODUCT_NAME "AvaSnapshot")
    target_sources(AvaSnapshot PRIVATE tests/editor_snapshot.cpp)
    target_include_directories(AvaSnapshot PRIVATE Source)
    target_link_libraries(AvaSnapshot PRIVATE
        AvasGrandPiano
        AvaAssets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)

    juce_add_console_app(AvaProcessorTest PRODUCT_NAME "AvaProcessorTest")
    target_sources(AvaProcessorTest PRIVATE tests/processor_test.cpp)
    target_include_directories(AvaProcessorTest PRIVATE Source)
    target_link_libraries(AvaProcessorTest PRIVATE
        AvasGrandPiano
        AvaAssets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)
endif()
