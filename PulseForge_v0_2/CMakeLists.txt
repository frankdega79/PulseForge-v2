cmake_minimum_required(VERSION 3.22)
project(PulseForge VERSION 0.2.0)

if(NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Pass -DJUCE_DIR=/path/to/JUCE when configuring this project.")
endif()
add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_plugin(PulseForge
    COMPANY_NAME "PulseForge Audio"
    BUNDLE_ID "audio.pulseforge.drum"
    PLUGIN_MANUFACTURER_CODE PlFg
    PLUGIN_CODE PlD2
    FORMATS VST3
    PRODUCT_NAME "Pulse Forge"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)
target_sources(PulseForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
target_compile_features(PulseForge PRIVATE cxx_std_17)
target_compile_definitions(PulseForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(PulseForge PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
