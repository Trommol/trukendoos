cmake_minimum_required(VERSION 3.22)

project(SpectralRot VERSION 0.1.5)

add_subdirectory(external/JUCE)

juce_add_plugin(SpectralRot
    COMPANY_NAME "Tremm Audio"
    BUNDLE_ID "com.tremmaudio.trukendoos"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Trem
    PLUGIN_CODE Truk
    FORMATS VST3 Standalone
    PRODUCT_NAME "trukendoos")

target_sources(SpectralRot
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        Source/dsp/SpectralCompressor.cpp
        Source/dsp/SpectralRotEngine.cpp)

juce_add_binary_data(SpectralRotAssets
    SOURCES
        Assets/ui/backplate.png
        Assets/ui/global_bus_plate.png
        Assets/ui/header_glass.png
        Assets/ui/header_melt.png
        Assets/ui/header_rust.png
        Assets/ui/header_squash.png
        Assets/ui/input_badge.png
        Assets/ui/knob_glass.png
        Assets/ui/knob_global.png
        Assets/ui/knob_harmonizer.png
        Assets/ui/knob_melt.png
        Assets/ui/knob_rust.png
        Assets/ui/knob_squash.png
        Assets/ui/output_badge.png
        Assets/ui/pedal_glass.png
        Assets/ui/pedal_harmonizer.png
        Assets/ui/pedal_melt.png
        Assets/ui/pedal_rust.png
        Assets/ui/pedal_squash.png
        Assets/ui/preset_plate.png)

target_compile_features(SpectralRot PRIVATE cxx_std_17)

target_compile_definitions(SpectralRot
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_ASIO=1
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_include_directories(SpectralRot
    PRIVATE
        Source)

target_link_libraries(SpectralRot
    PRIVATE
        SpectralRotAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
