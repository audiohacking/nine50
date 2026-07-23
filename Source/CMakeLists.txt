# Plugin target
juce_add_plugin(NINE50
    PLUGIN_NAME "NINE50"
    PLUGIN_CODE "N950"
    PLUGIN_CODE_DESCRIPTIVE "NINE50 VST3"
    FORMATS VST3 AU
    PRODUCT_NAME "NINE50"
    COMPANY_NAME "NINE50"
    VERSION 1.0.0
    AAX_CATEGORY "AudioEffect"
    VST3_CATEGORY "Effect"
)

target_sources(NINE50
    PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
        SidechainCompressor.cpp
        SidechainCompressor.h
        SP950Emulation.cpp
        SP950Emulation.h
        GainReductionMeter.cpp
        GainReductionMeter.h
)

juce_generate_juce_header(NINE50)

target_compile_definitions(NINE50 PRIVATE JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1)

target_compile_features(NINE50 PRIVATE cxx_std_17)

target_link_libraries(NINE50
    PRIVATE
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Testing
enable_testing()

if(BUILD_TESTING)
    juce_add_console_app(NINE50Tests
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE)

    juce_generate_juce_header(NINE50Tests)

    target_include_directories(NINE50Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_sources(NINE50Tests
        PRIVATE
            Tests/Main.cpp
            Tests/SidechainCompressorTests.cpp
            Tests/SP950EmulationTests.cpp
            Tests/PluginProcessorTests.cpp
            PluginProcessor.cpp
            PluginProcessor.h
            PluginEditor.cpp
            PluginEditor.h
            GainReductionMeter.cpp
            GainReductionMeter.h
            SidechainCompressor.cpp
            SidechainCompressor.h
            SP950Emulation.cpp
            SP950Emulation.h)

    target_compile_definitions(NINE50Tests PRIVATE
        JUCE_UNIT_TESTS=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="NINE50")

    target_link_libraries(NINE50Tests
        PRIVATE
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_audio_formats
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME NINE50Tests COMMAND NINE50Tests)
endif()
