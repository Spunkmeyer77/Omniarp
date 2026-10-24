cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
endif()

project(Omniarp VERSION 1.4.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(ARP_FORMATS VST3)
if(APPLE)
    list(APPEND ARP_FORMATS AU)
elseif(UNIX)
    list(APPEND ARP_FORMATS LV2)
endif()

juce_add_plugin(Omniarp
    COMPANY_NAME "Ticco Ross"
    COMPANY_EMAIL "ticcoross@gmail.com"
    PLUGIN_MANUFACTURER_CODE Tcro
    PLUGIN_CODE Omar
    PRODUCT_NAME "Omniarp"
    BUNDLE_ID com.ticcoross.omniarp
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    LV2URI "https://ticcoross.github.io/omniarp"
    FORMATS ${ARP_FORMATS})

target_sources(Omniarp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Omniarp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Omniarp
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

option(TROSS_SNAPSHOT "Build UI snapshot tool" OFF)
if(TROSS_SNAPSHOT)
    juce_add_console_app(OmniarpSnap PRODUCT_NAME "OmniarpSnap")
    target_sources(OmniarpSnap PRIVATE Tools/Snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(OmniarpSnap PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_VersionString="${PROJECT_VERSION}" JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(OmniarpSnap PRIVATE juce::juce_audio_utils juce::juce_gui_extra)
endif()
