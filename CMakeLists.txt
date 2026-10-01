cmake_minimum_required(VERSION 3.22)

# macOS: build universal (Apple Silicon + Intel) binaries for a 10.13+ target. Must be set before project().
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()
project(Sh0tyKTG1 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Artwork compiled into the binaries (the FF-1's marker drawing).
juce_add_binary_data(Sh0tyAssets SOURCES Assets/fuzzface.jpg)
set_target_properties(Sh0tyAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
    Source/PluginProcessor.h
    Source/KtgEditor.cpp
    Source/KtgEditor.h)

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
target_sources(Sh0tyFZ3 PRIVATE Source/FuzzProcessor.cpp Source/FuzzProcessor.h Source/FuzzEditor.cpp Source/FuzzEditor.h)
target_compile_definitions(Sh0tyFZ3 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Sh0tyFZ3 PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_plugin(Sh0tyBD2
    COMPANY_NAME "Sh0ty"
    PLUGIN_MANUFACTURER_CODE Sh0t
    PLUGIN_CODE Bd2x
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Sh0ty BD-2 Blues Driver"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Sh0tyBD2 PRIVATE Source/BluesProcessor.cpp Source/BluesProcessor.h Source/BluesEditor.cpp Source/BluesEditor.h)
target_compile_definitions(Sh0tyBD2 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Sh0tyBD2 PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_plugin(Sh0tyGS424
    COMPANY_NAME "Sh0ty"
    PLUGIN_MANUFACTURER_CODE Sh0t
    PLUGIN_CODE Gs42
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Sh0ty GS-424 Gain Stage"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Sh0tyGS424 PRIVATE Source/GsProcessor.cpp Source/GsProcessor.h Source/GsEditor.cpp Source/GsEditor.h)
target_compile_definitions(Sh0tyGS424 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Sh0tyGS424 PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_plugin(Sh0tyFF1
    COMPANY_NAME "Sh0ty"
    PLUGIN_MANUFACTURER_CODE Sh0t
    PLUGIN_CODE Ff1x
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Sh0ty FF-1 Fuzz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Sh0tyFF1 PRIVATE Source/FfProcessor.cpp Source/FfProcessor.h Source/FfEditor.cpp Source/FfEditor.h)
target_compile_definitions(Sh0tyFF1 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Sh0tyFF1 PRIVATE Sh0tyAssets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_plugin(Sh0tyRE201
    COMPANY_NAME "Sh0ty"
    PLUGIN_MANUFACTURER_CODE Sh0t
    PLUGIN_CODE Re20
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Sh0ty RE-201 Space Echo"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Sh0tyRE201 PRIVATE Source/ReProcessor.cpp Source/ReProcessor.h Source/ReEditor.cpp Source/ReEditor.h)
target_compile_definitions(Sh0tyRE201 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Sh0tyRE201 PRIVATE juce::juce_audio_utils juce::juce_dsp
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
    add_executable(bd2_dsp_test tests/blues_test.cpp)
    target_include_directories(bd2_dsp_test PRIVATE Source)
    add_test(NAME blues COMMAND bd2_dsp_test)
    add_executable(gs424_dsp_test tests/gs424_test.cpp)
    target_include_directories(gs424_dsp_test PRIVATE Source)
    add_test(NAME gs424 COMMAND gs424_dsp_test)
    add_executable(ff1_dsp_test tests/ff1_test.cpp)
    target_include_directories(ff1_dsp_test PRIVATE Source)
    add_test(NAME ff1 COMMAND ff1_dsp_test)
    add_executable(re201_dsp_test tests/re201_test.cpp)
    target_include_directories(re201_dsp_test PRIVATE Source)
    add_test(NAME re201 COMMAND re201_dsp_test)
endif()

# Renders the FZ-3 / BD-2 editors to a PNG (used to review the UI headlessly under xvfb).
option(KTG1_BUILD_UI_SNAPSHOT "Build UI snapshot tool" OFF)
if(KTG1_BUILD_UI_SNAPSHOT)
    juce_add_console_app(FZ3Snapshot PRODUCT_NAME "FZ3Snapshot")
    target_sources(FZ3Snapshot PRIVATE tests/snapshot.cpp Source/FuzzProcessor.cpp Source/FuzzEditor.cpp)
    target_compile_definitions(FZ3Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(FZ3Snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    juce_add_console_app(BD2Snapshot PRODUCT_NAME "BD2Snapshot")
    target_sources(BD2Snapshot PRIVATE tests/snapshot_bd2.cpp Source/BluesProcessor.cpp Source/BluesEditor.cpp)
    target_compile_definitions(BD2Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(BD2Snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    juce_add_console_app(KTG1Snapshot PRODUCT_NAME "KTG1Snapshot")
    target_sources(KTG1Snapshot PRIVATE tests/snapshot_ktg.cpp Source/PluginProcessor.cpp Source/KtgEditor.cpp)
    target_compile_definitions(KTG1Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(KTG1Snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    juce_add_console_app(GS424Snapshot PRODUCT_NAME "GS424Snapshot")
    target_sources(GS424Snapshot PRIVATE tests/snapshot_gs.cpp Source/GsProcessor.cpp Source/GsEditor.cpp)
    target_compile_definitions(GS424Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(GS424Snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()

# ---------------------------------------------------------------------------
# Pedalboard: a standalone app that hosts all four plugins in a patchable graph.
# ---------------------------------------------------------------------------
set(BOARD_PLUGIN_SOURCES
    Source/PluginProcessor.cpp Source/KtgEditor.cpp
    Source/FuzzProcessor.cpp   Source/FuzzEditor.cpp
    Source/BluesProcessor.cpp  Source/BluesEditor.cpp
    Source/GsProcessor.cpp     Source/GsEditor.cpp
    Source/FfProcessor.cpp     Source/FfEditor.cpp
    Source/ReProcessor.cpp     Source/ReEditor.cpp)

option(KTG1_BUILD_PEDALBOARD "Build the pedalboard app" ON)
if(KTG1_BUILD_PEDALBOARD)
    juce_add_gui_app(Sh0tyPedalboard
        COMPANY_NAME "Sh0ty"
        PRODUCT_NAME "Sh0ty Pedalboard"
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "Sh0ty Pedalboard needs your audio input to process your guitar.")
    target_sources(Sh0tyPedalboard PRIVATE
        Pedalboard/Main.cpp Pedalboard/BoardComponent.cpp Pedalboard/BoardModel.cpp ${BOARD_PLUGIN_SOURCES})
    target_include_directories(Sh0tyPedalboard PRIVATE Source Pedalboard)
    target_compile_definitions(Sh0tyPedalboard PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_APPLICATION_NAME_STRING="Sh0ty Pedalboard"
        JUCE_APPLICATION_VERSION_STRING="0.1.0" SH0TY_STANDALONE_BOARD=1)
    target_link_libraries(Sh0tyPedalboard PRIVATE Sh0tyAssets juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    # headless test of the audio graph (patching, loops, save/load)
    juce_add_console_app(PedalboardTest PRODUCT_NAME "PedalboardTest")
    target_sources(PedalboardTest PRIVATE tests/board_test.cpp Pedalboard/BoardModel.cpp ${BOARD_PLUGIN_SOURCES})
    target_include_directories(PedalboardTest PRIVATE Source Pedalboard)
    target_compile_definitions(PedalboardTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 SH0TY_STANDALONE_BOARD=1)
    target_link_libraries(PedalboardTest PRIVATE Sh0tyAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    if(KTG1_BUILD_TESTS)
        add_test(NAME pedalboard COMMAND PedalboardTest)
    endif()
    if(KTG1_BUILD_PEDALBOARD AND KTG1_BUILD_UI_SNAPSHOT)
        juce_add_console_app(PedalboardSnapshot PRODUCT_NAME "PedalboardSnapshot")
        target_sources(PedalboardSnapshot PRIVATE tests/snapshot_board.cpp Pedalboard/BoardComponent.cpp Pedalboard/BoardModel.cpp ${BOARD_PLUGIN_SOURCES})
        target_include_directories(PedalboardSnapshot PRIVATE Source Pedalboard)
        target_compile_definitions(PedalboardSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 SH0TY_STANDALONE_BOARD=1)
        target_link_libraries(PedalboardSnapshot PRIVATE Sh0tyAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    endif()
    juce_add_console_app(RE201Snapshot PRODUCT_NAME "RE201Snapshot")
    target_sources(RE201Snapshot PRIVATE tests/snapshot_re.cpp Source/ReProcessor.cpp Source/ReEditor.cpp)
    target_compile_definitions(RE201Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(RE201Snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    juce_add_console_app(FF1Snapshot PRODUCT_NAME "FF1Snapshot")
    target_sources(FF1Snapshot PRIVATE tests/snapshot_ff.cpp Source/FfProcessor.cpp Source/FfEditor.cpp)
    target_compile_definitions(FF1Snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(FF1Snapshot PRIVATE Sh0tyAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
