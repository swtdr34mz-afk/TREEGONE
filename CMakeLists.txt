# TREEGONE — the plugin (VST3, AU on the Mac) and the standalone app, with JUCE 8.
#   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
# JUCE comes from GitHub (or -DTREEGONE_JUCE_DIR=/path/to/JUCE).
cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(TREEGONE VERSION 1.0.8 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TREEGONE_JUCE_DIR "" CACHE PATH "a JUCE checkout (empty: from GitHub)")
if(TREEGONE_JUCE_DIR)
    add_subdirectory(${TREEGONE_JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

set(TREEGONE_FORMATS VST3 Standalone)
if(TREEGONE_LOCAL_TEST)
    set(TREEGONE_FORMATS VST3)
endif()
if(APPLE)
    list(APPEND TREEGONE_FORMATS AU)
endif()

juce_add_plugin(Treegone
    PRODUCT_NAME "TREEGONE"
    COMPANY_NAME "swtdr34mz"
    BUNDLE_ID "com.swtdr34mz.treegone"
    PLUGIN_MANUFACTURER_CODE Swtd
    PLUGIN_CODE Tgn6
    FORMATS ${TREEGONE_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_add_binary_data(TreegoneData SOURCES
    fonts/FiraSans-SemiBold.ttf
    fonts/FiraSans-Bold.ttf
    fonts/FiraSans-ExtraBold.ttf
    fonts/AnonymousPro-Bold.ttf
    res/swtdr34mz-tag.png)

set(TREEGONE_SOURCES
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/ui/Look.cpp
    dsp/Engine.cpp
    dsp/Params.cpp
    dsp/Presets.cpp
    dsp/Sysex.cpp)
target_sources(Treegone PRIVATE ${TREEGONE_SOURCES})
target_include_directories(Treegone PRIVATE dsp src)

target_compile_definitions(Treegone PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Treegone PRIVATE
    TreegoneData
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# a picture of the editor, without a window (tools/snapshot.cpp): -DTREEGONE_SNAPSHOT=ON
option(TREEGONE_SNAPSHOT "build TreegoneShot, the editor's picture" OFF)
if(TREEGONE_SNAPSHOT)
    juce_add_console_app(TreegoneShot PRODUCT_NAME "TreegoneShot")
    target_sources(TreegoneShot PRIVATE tools/snapshot.cpp ${TREEGONE_SOURCES})
    target_include_directories(TreegoneShot PRIVATE dsp src)
    target_compile_definitions(TreegoneShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1
        JucePlugin_Build_Standalone=0)
    target_link_libraries(TreegoneShot PRIVATE TreegoneData juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
