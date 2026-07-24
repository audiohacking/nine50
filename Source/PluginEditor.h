#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "NINE50LookAndFeel.h"

class NINE50AudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    NINE50AudioProcessorEditor (NINE50AudioProcessor&);
    ~NINE50AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupRotary (juce::Slider& slider, const juce::String& suffix);
    void setupLabel (juce::Label& label, const juce::String& text);
    void layoutKnob (juce::Slider& slider, juce::Label& label, juce::Rectangle<int> area);
    juce::Rectangle<int> getContentBand() const;
    void syncStageEnableLabels();

    void refreshPresetList();
    void loadSelectedPreset();
    void savePresetClicked();
    void showSaveAsDialog();
    void showPresetMenu();

    NINE50AudioProcessor& audioProcessor;
    NINE50LookAndFeel lookAndFeel;
    juce::Rectangle<int> meterWellBounds;

    // Preset bar
    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton presetMenuButton { "..." };

    // Stage enable (ON / BYPASS)
    juce::ToggleButton compOnButton;
    juce::ToggleButton crushOnButton;

    // Sidechain Compressor controls
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider makeupSlider;
    juce::ComboBox hpfCombo;
    juce::ToggleButton linkButton;

    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, makeupLabel;
    juce::Label hpfLabel, sidechainLabel;

    // Bitcrush / downsample (SP-1200 / S950) controls
    juce::Slider driveSlider;
    juce::Slider detuneSlider;
    juce::ToggleButton extButton;
    juce::ToggleButton fineButton;
    juce::Slider filterSlider;
    juce::ComboBox layoutCombo;
    juce::Slider mixSlider;
    juce::Slider outSlider;
    juce::ToggleButton crushLinkButton;

    juce::Label driveLabel, detuneLabel, filterLabel, mixLabel, outLabel, layoutLabel;

    // Gain reduction meter
    GainReductionMeter grMeter;
    juce::Label grLabel;

    // Sidechain input selector
    juce::ComboBox sidechainInputCombo;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> crushOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> detuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> extAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> layoutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> crushLinkAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NINE50AudioProcessorEditor)
};
