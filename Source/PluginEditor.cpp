#include "PluginEditor.h"

NINE50AudioProcessorEditor::NINE50AudioProcessorEditor(NINE50AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    // Set window size
    setSize(600, 400);

    // Sidechain Compressor sliders
    thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    thresholdSlider.setRange(-60.0, 0.0, 0.1);
    thresholdSlider.setValue(-15.0);

    ratioSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    ratioSlider.setRange(1.0, 10.0, 0.1);
    ratioSlider.setValue(8.0);

    attackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    attackSlider.setRange(0.1, 50.0, 0.1);
    attackSlider.setValue(10.0);

    releaseSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    releaseSlider.setRange(10.0, 500.0, 1.0);
    releaseSlider.setValue(100.0);

    makeupSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    makeupSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    makeupSlider.setRange(0.0, 30.0, 0.1);
    makeupSlider.setValue(0.0);

    hpfCombo.addItemList({"Off", "100 Hz", "200 Hz", "300 Hz"}, 1);

    linkButton.setButtonText("Link");

    // SP950 sliders
    driveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    driveSlider.setRange(-12.0, 12.0, 0.1);
    driveSlider.setValue(0.0);

    detuneSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    detuneSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    detuneSlider.setRange(-15.0, 15.0, 0.1);
    detuneSlider.setValue(0.0);

    extButton.setButtonText("Ext");
    fineButton.setButtonText("Fine");

    filterSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    filterSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    filterSlider.setRange(0.0, 99.0, 1.0);
    filterSlider.setValue(99.0);

    layoutCombo.addItemList({"Mono Sum", "Mono L", "Mono R", "Stereo", "Stereo L", "Stereo R", "Mid/Side"}, 1);
    layoutCombo.setSelectedId(4);

    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    mixSlider.setRange(0.0, 100.0, 1.0);
    mixSlider.setValue(100.0);

    outSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    outSlider.setRange(-12.0, 12.0, 0.1);
    outSlider.setValue(0.0);

    sp950LinkButton.setButtonText("Link");

    // Sidechain input selector
    sidechainInputCombo.addItem("None", 1);

    // Add and make visible all components
    addAndMakeVisible(thresholdSlider);
    addAndMakeVisible(ratioSlider);
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(releaseSlider);
    addAndMakeVisible(makeupSlider);
    addAndMakeVisible(hpfCombo);
    addAndMakeVisible(linkButton);

    addAndMakeVisible(driveSlider);
    addAndMakeVisible(detuneSlider);
    addAndMakeVisible(extButton);
    addAndMakeVisible(fineButton);
    addAndMakeVisible(filterSlider);
    addAndMakeVisible(layoutCombo);
    addAndMakeVisible(mixSlider);
    addAndMakeVisible(outSlider);
    addAndMakeVisible(sp950LinkButton);

    addAndMakeVisible(grMeter);
    addAndMakeVisible(sidechainInputCombo);

    // Create attachments
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "threshold", thresholdSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "ratio", ratioSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "attack", attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "release", releaseSlider);
    makeupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "makeup", makeupSlider);
    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "sidechain_hpf", hpfCombo);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "link", linkButton);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "drive", driveSlider);
    detuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "detune", detuneSlider);
    extAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "ext", extButton);
    fineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "fine", fineButton);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "filter", filterSlider);
    layoutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.parameters, "layout", layoutCombo);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "mix", mixSlider);
    outAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "out", outSlider);
    sp950LinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "sp950_link", sp950LinkButton);

    // Start timer for gain reduction meter updates
    startTimerHz(30);
}

NINE50AudioProcessorEditor::~NINE50AudioProcessorEditor() {
}

void NINE50AudioProcessorEditor::paint(juce::Graphics& g) {
    // Background
    g.fillAll(juce::Colours::darkgrey.darker(0.3f));

    // Title bars
    g.setColour(juce::Colours::black);
    g.fillRect(juce::Rectangle<int>(0, 0, getWidth(), 25));

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("SIDECHAIN COMPRESSOR", juce::Rectangle<int>(0, 0, 300, 25), juce::Justification::centredLeft, false);
    g.drawText("SP950", juce::Rectangle<int>(300, 0, 300, 25), juce::Justification::centredLeft, false);
}

void NINE50AudioProcessorEditor::resized() {
    const int margin = 10;
    const int sliderWidth = 180;
    const int sliderHeight = 20;
    const int rowHeight = 25;

    // Sidechain Compressor section (left half)
    int y = 35;
    int x = margin;

    thresholdSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    ratioSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    attackSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    releaseSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    makeupSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;

    // HPF combo and link button
    hpfCombo.setBounds(x, y, 100, sliderHeight);
    linkButton.setBounds(x + 110, y, 80, sliderHeight);
    y += rowHeight;

    // Gain reduction meter
    grMeter.setBounds(x + sliderWidth + 10, 35, 30, 150);

    // SP950 section (right half)
    y = 35;
    x = 310;

    driveSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    detuneSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;

    // Ext and Fine buttons side by side
    extButton.setBounds(x, y, 60, sliderHeight);
    fineButton.setBounds(x + 70, y, 60, sliderHeight);
    y += rowHeight;

    filterSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    layoutCombo.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    mixSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    outSlider.setBounds(x, y, sliderWidth, sliderHeight);
    y += rowHeight;
    sp950LinkButton.setBounds(x, y, 80, sliderHeight);
    y += rowHeight;

    // Sidechain input selector at bottom
    sidechainInputCombo.setBounds(margin, getHeight() - 30, getWidth() - 2 * margin, 20);
}

void NINE50AudioProcessorEditor::timerCallback() {
    // Update gain reduction meter
    grMeter.setGainReduction(audioProcessor.compressor.getGainReduction());
}
