cmake_minimum_required(VERSION 3.22)

project(PythonHooker VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE (recommended as a git submodule)
add_subdirectory(third_party/JUCE)

juce_add_plugin(PythonHooker
    COMPANY_NAME "YourCompany"
    BUNDLE_ID "com.yourcompany.pythonhooker"
    PLUGIN_MANUFACTURER_CODE Ycmp
    PLUGIN_CODE Pyhk
    FORMATS VST3
    PRODUCT_NAME "PythonHooker"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
)

target_sources(PythonHooker PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ConfigLoader.cpp
    Source/ConfigLoader.h
)

target_link_libraries(PythonHooker PRIVATE
    juce::juce_audio_utils
)

target_compile_definitions(PythonHooker PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
