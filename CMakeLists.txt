cmake_minimum_required(VERSION 3.22)
project(ReverseReverb VERSION 0.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.dependencies/JUCE" CACHE PATH "JUCE source folder")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found. Run scripts/build.ps1 first, or set JUCE_DIR.")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

juce_add_plugin(ReverseReverb
    COMPANY_NAME "Personal Audio"
    BUNDLE_ID "local.personal.reverse-reverb"
    PLUGIN_MANUFACTURER_CODE Prsn
    PLUGIN_CODE Rvrb
    FORMATS VST3 Standalone
    PRODUCT_NAME "ReverseReverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Reverb
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(ReverseReverb PRIVATE
    Source/AudioEngine.cpp Source/AudioEngine.h
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h
    Source/DarkTheme.cpp Source/DarkTheme.h
    Source/TrackEngine.cpp Source/TrackEngine.h
    Source/ShapeGraph.cpp Source/ShapeGraph.h
    Source/Waveform.cpp Source/Waveform.h)
target_compile_definitions(ReverseReverb PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    RR_EXPORT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/exports")
target_link_libraries(ReverseReverb PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
target_include_directories(ReverseReverb PUBLIC Source)

option(RR_BUILD_TESTS "Build offline audio and integration checks" ON)
if(RR_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(ReverseReverbChecks PRODUCT_NAME "ReverseReverbChecks")
    target_sources(ReverseReverbChecks PRIVATE Tests/Checks.cpp)
    target_link_libraries(ReverseReverbChecks PRIVATE ReverseReverb juce::juce_audio_utils juce::juce_dsp)
    add_dependencies(ReverseReverbChecks ReverseReverb_VST3)
    add_test(NAME ReverseReverbChecks COMMAND ReverseReverbChecks)
endif()
