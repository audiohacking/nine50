#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>

/** Factory + user preset management for NINE50. */
class PresetManager
{
public:
    struct Preset
    {
        juce::String name;
        bool isFactory = false;
        juce::File file; // valid for user presets only
        std::map<juce::String, float> values;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    void refreshUserPresets();
    int getNumPresets() const;
    juce::String getPresetName (int index) const;
    bool isFactoryPreset (int index) const;
    int getCurrentIndex() const { return currentIndex; }
    juce::String getCurrentName() const;

    /** Apply preset by list index. Returns false if index invalid. */
    bool loadPreset (int index);

    /** Save current parameters over an existing user preset (or Save As if factory/init). */
    bool saveCurrentUserPreset();

    /** Save current parameters as a new user preset. Returns false on cancel/failure. */
    bool saveAsUserPreset (const juce::String& name);

    /** Delete the currently selected user preset. */
    bool deleteCurrentUserPreset();

    juce::File getUserPresetsDirectory() const;
    juce::StringArray getPresetNames() const;

    /** Index used by AudioProcessor program API (factory only). */
    int getNumFactoryPrograms() const;
    juce::String getFactoryProgramName (int index) const;
    bool loadFactoryProgram (int index);

private:
    void buildFactoryPresets();
    void applyValues (const std::map<juce::String, float>& values);
    std::map<juce::String, float> captureCurrentValues() const;
    bool writePresetFile (const juce::File& file, const juce::String& name,
                          const std::map<juce::String, float>& values) const;
    bool readPresetFile (const juce::File& file, Preset& out) const;

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> factoryPresets;
    std::vector<Preset> userPresets;
    int currentIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
