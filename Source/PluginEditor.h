#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"

class NINE50AudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer {
public:
    NINE50AudioProcessorEditor(NINE50AudioProcessor&);
    ~NINE50AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    NINE50AudioProcessor& audioProcessor;

    // Sidechain Compressor controls
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider makeupSlider;
    juce::ComboBox hpfCombo;
    juce::ToggleButton linkButton;

    // SP950 controls
    juce::Slider driveSlider;
    juce::Slider detuneSlider;
    juce::ToggleButton extButton;
    juce::ToggleButton fineButton;
    juce::Slider filterSlider;
    juce::ComboBox layoutCombo;
    juce::Slider mixSlider;
    juce::Slider outSlider;
    juce::ToggleButton sp950LinkButton;

    // Gain reduction meter
    GainReductionMeter grMeter;

    // Sidechain input selector
    juce::ComboBox sidechainInputCombo;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> detuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> extAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> layoutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sp950LinkAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NINE50AudioProcessorEditor)
};
