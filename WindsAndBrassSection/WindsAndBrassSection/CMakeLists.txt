cmake_minimum_required(VERSION 3.22)

project(WindsAndBrassSection VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(WindsAndBrassSection
    COMPANY_NAME                "WBS Audio"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Wbsa
    PLUGIN_CODE                 Wbsx
    FORMATS                     VST3
    VST3_CATEGORIES             Instrument Synth
    PRODUCT_NAME                "Winds and Brass Section")

target_sources(WindsAndBrassSection PRIVATE
    Source/Engine.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(WindsAndBrassSection PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(WindsAndBrassSection
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
git init -b main
git add .
git commit -m "Winds and Brass Section: versión inicial"
git remote add origin https://github.com/TU_USUARIO/winds-and-brass-section.git
git push -u origin main
