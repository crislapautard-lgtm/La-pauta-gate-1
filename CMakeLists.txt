cmake_minimum_required(VERSION 3.22)

# Runtime de Visual C++ enlazado de forma estática: el instalador no necesita redistribuibles
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(LaPautaGateC1 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LaPautaGateC1
    COMPANY_NAME            "La Pauta"
    PRODUCT_NAME            "La Pauta Gate C1"
    PLUGIN_MANUFACTURER_CODE LaPa
    PLUGIN_CODE             Gtc1
    FORMATS                 VST3
    VST3_CATEGORIES         Fx Dynamics
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(LaPautaGateC1 PRIVATE
    Source/GateDSP.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(LaPautaGateC1 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LaPautaGateC1
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
