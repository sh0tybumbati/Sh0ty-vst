cmake_minimum_required(VERSION 3.22)
project(Sh0tyKTG1 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Format list: VST3 + Standalone everywhere, AU on macOS.
set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(Sh0tyKTG1
    COMPANY_NAME "Sh0ty"
    PLUGIN_MANUFACTURER_CODE Sh0t
    PLUGIN_CODE Ktg1
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Sh0ty KTG-1 Tube Preamp"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Sh0tyKTG1 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(Sh0tyKTG1 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Sh0tyKTG1 PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

juce_add_plugin(Sh0tyFZ3
    COMPANY_NAME "Sh0ty"
    PLUGIN_MANUFACTURER_CODE Sh0t
    PLUGIN_CODE Fz3x
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Sh0ty FZ-3 Fuzz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Sh0tyFZ3 PRIVATE Source/FuzzProcessor.cpp Source/FuzzProcessor.h)
target_compile_definitions(Sh0tyFZ3 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Sh0tyFZ3 PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# JUCE-free DSP test (harmonics, DC, gain, stability)
option(KTG1_BUILD_TESTS "Build DSP tests" ON)
if(KTG1_BUILD_TESTS)
    enable_testing()
    add_executable(ktg1_dsp_test tests/dsp_test.cpp)
    target_include_directories(ktg1_dsp_test PRIVATE Source)
    add_test(NAME dsp COMMAND ktg1_dsp_test)
    add_executable(fz3_dsp_test tests/fuzz_test.cpp)
    target_include_directories(fz3_dsp_test PRIVATE Source)
    add_test(NAME fuzz COMMAND fz3_dsp_test)
endif()
