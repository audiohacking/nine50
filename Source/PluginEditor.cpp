#include "PluginEditor.h"

namespace
{
    constexpr int kHeaderH   = 48;
    constexpr int kWindowW   = 760;
    constexpr int kPadX      = 14;
    constexpr int kPadTop    = 6;
    constexpr int kPadBottom = 2;
    constexpr int kTitleH    = 20;
    constexpr int kKnobH     = 112;
    constexpr int kGap       = 4;
    constexpr int kBottomH   = 40; // label + combo footer
    constexpr int kMeterW    = 48;

    // Knob block only — meter shares this ceiling/floor
    constexpr int kKnobBandH = kKnobH + kGap + kKnobH;
    constexpr int kContentH  = kTitleH + kKnobBandH + kGap + kBottomH;
    constexpr int kWindowH   = kHeaderH + kPadTop + kContentH + kPadBottom;
}

NINE50AudioProcessorEditor::NINE50AudioProcessorEditor (NINE50AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (kWindowW, kWindowH);

    setupRotary (thresholdSlider, " dB");
    setupRotary (ratioSlider, " :1");
    setupRotary (attackSlider, " ms");
    setupRotary (releaseSlider, " ms");
    setupRotary (makeupSlider, " dB");
    setupRotary (driveSlider, " dB");
    setupRotary (detuneSlider, " st");
    setupRotary (crushHpfSlider, "");
    setupRotary (filterSlider, "");
    setupRotary (mixSlider, " %");
    setupRotary (outSlider, " dB");

    setupLabel (thresholdLabel, "THRESH");
    setupLabel (ratioLabel, "RATIO");
    setupLabel (attackLabel, "ATTACK");
    setupLabel (releaseLabel, "RELEASE");
    setupLabel (makeupLabel, "MAKEUP");
    setupLabel (driveLabel, "DRIVE");
    setupLabel (detuneLabel, "DETUNE");
    setupLabel (crushHpfLabel, "HPF");
    setupLabel (filterLabel, "LPF");
    setupLabel (mixLabel, "MIX");
    setupLabel (outLabel, "OUT");
    setupLabel (hpfLabel, "SC HPF");
    setupLabel (sidechainLabel, "SIDECHAIN IN");
    setupLabel (layoutLabel, "LAYOUT");
    setupLabel (grLabel, "GR");
    grLabel.setJustificationType (juce::Justification::centred);
    grLabel.setColour (juce::Label::textColourId, NINE50Colours::amber);

    hpfCombo.addItemList ({ "Off", "100 Hz", "200 Hz", "300 Hz" }, 1);
    layoutCombo.addItemList ({ "Mono Sum", "Mono L", "Mono R", "Stereo",
                               "Stereo L", "Stereo R", "Stereo Mid", "Stereo Side" }, 1);
    layoutCombo.setSelectedId (4);
    sidechainInputCombo.addItem ("None", 1);

    linkButton.setButtonText ("LINK");
    crushLinkButton.setButtonText ("LINK");
    extButton.setButtonText ("EXT");
    fineButton.setButtonText ("FINE");

    compOnButton.setButtonText ("ON");
    crushOnButton.setButtonText ("ON");
    compOnButton.setClickingTogglesState (true);
    crushOnButton.setClickingTogglesState (true);

    // Preset controls
    refreshPresetList();
    presetCombo.onChange = [this] { loadSelectedPreset(); };
    savePresetButton.onClick = [this] { savePresetClicked(); };
    presetMenuButton.onClick = [this] { showPresetMenu(); };

    addAndMakeVisible (presetCombo);
    addAndMakeVisible (savePresetButton);
    addAndMakeVisible (presetMenuButton);
    addAndMakeVisible (compOnButton);
    addAndMakeVisible (crushOnButton);

    addAndMakeVisible (thresholdSlider);
    addAndMakeVisible (ratioSlider);
    addAndMakeVisible (attackSlider);
    addAndMakeVisible (releaseSlider);
    addAndMakeVisible (makeupSlider);
    addAndMakeVisible (hpfCombo);
    addAndMakeVisible (linkButton);

    addAndMakeVisible (thresholdLabel);
    addAndMakeVisible (ratioLabel);
    addAndMakeVisible (attackLabel);
    addAndMakeVisible (releaseLabel);
    addAndMakeVisible (makeupLabel);
    addAndMakeVisible (hpfLabel);
    addAndMakeVisible (sidechainLabel);

    addAndMakeVisible (driveSlider);
    addAndMakeVisible (detuneSlider);
    addAndMakeVisible (extButton);
    addAndMakeVisible (fineButton);
    addAndMakeVisible (crushHpfSlider);
    addAndMakeVisible (filterSlider);
    addAndMakeVisible (layoutCombo);
    addAndMakeVisible (mixSlider);
    addAndMakeVisible (outSlider);
    addAndMakeVisible (crushLinkButton);

    addAndMakeVisible (driveLabel);
    addAndMakeVisible (detuneLabel);
    addAndMakeVisible (crushHpfLabel);
    addAndMakeVisible (filterLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (outLabel);
    addAndMakeVisible (layoutLabel);

    addAndMakeVisible (grMeter);
    addAndMakeVisible (grLabel);
    addAndMakeVisible (sidechainInputCombo);

    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "threshold", thresholdSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "ratio", ratioSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "attack", attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "release", releaseSlider);
    makeupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "makeup", makeupSlider);
    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.parameters, "sidechain_hpf", hpfCombo);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "link", linkButton);
    compOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "comp_on", compOnButton);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "drive", driveSlider);
    detuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "detune", detuneSlider);
    extAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "ext", extButton);
    fineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "fine", fineButton);
    crushHpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "hpf", crushHpfSlider);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "filter", filterSlider);
    layoutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.parameters, "layout", layoutCombo);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "mix", mixSlider);
    outAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "out", outSlider);
    crushLinkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "crush_link", crushLinkButton);
    crushOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "crush_on", crushOnButton);

    syncStageEnableLabels();
    startTimerHz (30);
}

NINE50AudioProcessorEditor::~NINE50AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void NINE50AudioProcessorEditor::setupRotary (juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 16);
    slider.setTextValueSuffix (suffix);
    slider.setColour (juce::Slider::textBoxTextColourId, NINE50Colours::lcdText);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, NINE50Colours::lcdBg);
    slider.setColour (juce::Slider::textBoxOutlineColourId, NINE50Colours::sectionLine);
}

void NINE50AudioProcessorEditor::setupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, NINE50Colours::label);
    label.setInterceptsMouseClicks (false, false);
}

void NINE50AudioProcessorEditor::layoutKnob (juce::Slider& slider, juce::Label& label,
                                             juce::Rectangle<int> area)
{
    label.setBounds (area.removeFromTop (16));
    slider.setBounds (area);
}

juce::Rectangle<int> NINE50AudioProcessorEditor::getContentBand() const
{
    // Shared knob-band: ceiling = knob labels, floor = bottom of row-2 controls
    return { kPadX,
             kHeaderH + kPadTop + kTitleH,
             getWidth() - 2 * kPadX,
             kKnobBandH };
}

void NINE50AudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bg (NINE50Colours::panelTop, 0.0f, 0.0f,
                             NINE50Colours::panelBottom, 0.0f, bounds.getHeight(), false);
    g.setGradientFill (bg);
    g.fillRect (bounds);

    juce::Random rng (0x4e3950);
    g.setColour (juce::Colours::white.withAlpha (0.015f));
    for (int i = 0; i < 700; ++i)
    {
        const float x = rng.nextFloat() * bounds.getWidth();
        const float y = rng.nextFloat() * bounds.getHeight();
        g.fillRect (x, y, 1.0f, 1.0f);
    }

    auto header = bounds.removeFromTop (static_cast<float> (kHeaderH));
    g.setColour (NINE50Colours::headerBg);
    g.fillRect (header);

    juce::ColourGradient headerSheen (juce::Colours::white.withAlpha (0.04f), 0.0f, 0.0f,
                                      juce::Colours::transparentBlack, 0.0f, header.getHeight(), false);
    g.setGradientFill (headerSheen);
    g.fillRect (header);

    g.setColour (NINE50Colours::amber.withAlpha (0.55f));
    g.fillRect (header.getX(), header.getBottom() - 2.0f, header.getWidth(), 2.0f);

    g.setColour (NINE50Colours::brand);
    g.setFont (juce::Font (juce::FontOptions ("Avenir Next Condensed", 26.0f, juce::Font::bold)));
    g.drawText ("NINE50", header.reduced (18.0f, 0.0f), juce::Justification::centredLeft, false);

    const auto band = getContentBand().toFloat();
    const float sideW = (band.getWidth() - static_cast<float> (kMeterW)) * 0.5f;
    const float titleY = static_cast<float> (kHeaderH + kPadTop);

    g.setColour (NINE50Colours::amber);
    g.setFont (juce::Font (juce::FontOptions ("Avenir Next Condensed", 13.0f, juce::Font::bold)));
    g.drawText ("COMPRESSOR",
                juce::Rectangle<float> (band.getX(), titleY, sideW - 78.0f, 16.0f),
                juce::Justification::centredLeft, false);
    g.drawText ("BITCRUSH",
                juce::Rectangle<float> (band.getX() + sideW + static_cast<float> (kMeterW), titleY,
                                        sideW - 200.0f, 16.0f),
                juce::Justification::centredLeft, false);

    g.setColour (NINE50Colours::sectionLine);
    g.fillRect (band.getX(), titleY + 17.0f, sideW - 6.0f, 1.0f);
    g.fillRect (band.getX() + sideW + static_cast<float> (kMeterW), titleY + 17.0f, sideW - 6.0f, 1.0f);

    // Three matched wells — identical ceiling/floor across compressor / meter / bitcrush
    auto drawWell = [&g] (juce::Rectangle<float> r)
    {
        g.setColour (NINE50Colours::headerBg.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (NINE50Colours::sectionLine);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    };

    drawWell ({ band.getX(), band.getY(), sideW - 2.0f, band.getHeight() });
    drawWell ({ band.getX() + sideW + 3.0f, band.getY(),
                static_cast<float> (kMeterW) - 6.0f, band.getHeight() });
    drawWell ({ band.getX() + sideW + static_cast<float> (kMeterW) + 2.0f,
                band.getY(), sideW - 2.0f, band.getHeight() });
}

void NINE50AudioProcessorEditor::resized()
{
    // Header preset bar
    {
        auto header = getLocalBounds().removeFromTop (kHeaderH).reduced (14, 10);
        header.removeFromLeft (110); // brand space
        presetMenuButton.setBounds (header.removeFromRight (34));
        header.removeFromRight (6);
        savePresetButton.setBounds (header.removeFromRight (56));
        header.removeFromRight (8);
        presetCombo.setBounds (header);
    }

    // Stage ON/BYPASS toggles sit in the section title row
    {
        auto titleRow = getLocalBounds();
        titleRow.removeFromTop (kHeaderH + kPadTop);
        titleRow = titleRow.removeFromTop (kTitleH).reduced (kPadX, 0);
        const int colW = (titleRow.getWidth() - kMeterW) / 2;
        auto leftTitle = titleRow.removeFromLeft (colW);
        titleRow.removeFromLeft (kMeterW);
        auto rightTitle = titleRow;

        compOnButton.setBounds (leftTitle.removeFromRight (72).withSizeKeepingCentre (72, 18));
        crushOnButton.setBounds (rightTitle.removeFromRight (72).withSizeKeepingCentre (72, 18));

        // EXT / FINE / LINK sit beside the bitcrush title
        auto crushToggles = rightTitle.withSizeKeepingCentre (rightTitle.getWidth(), 18);
        crushToggles.removeFromRight (4);
        const int tw = crushToggles.getWidth() / 3;
        extButton.setBounds (crushToggles.removeFromLeft (tw).reduced (2, 0));
        fineButton.setBounds (crushToggles.removeFromLeft (tw).reduced (2, 0));
        crushLinkButton.setBounds (crushToggles.reduced (2, 0));
    }

    auto band = getContentBand();
    meterWellBounds = band;

    const int colW = (band.getWidth() - kMeterW) / 2;
    auto left = band.removeFromLeft (colW);
    auto meterArea = band.removeFromLeft (kMeterW);
    auto right = band;

    auto layoutFive = [this] (juce::Rectangle<int> area,
                              juce::Slider& k1, juce::Label& l1,
                              juce::Slider& k2, juce::Label& l2,
                              juce::Slider& k3, juce::Label& l3,
                              juce::Slider& k4, juce::Label& l4,
                              juce::Slider& k5, juce::Label& l5)
    {
        area = area.reduced (2, 0);
        const int knobW = area.getWidth() / 3;

        auto row1 = area.removeFromTop (kKnobH);
        layoutKnob (k1, l1, row1.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k2, l2, row1.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k3, l3, row1.reduced (3, 0));

        area.removeFromTop (kGap);

        auto row2 = area;
        layoutKnob (k4, l4, row2.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k5, l5, row2.removeFromLeft (knobW).reduced (3, 0));

        auto extras = row2.reduced (3, 6);
        hpfLabel.setBounds (extras.removeFromTop (14));
        hpfCombo.setBounds (extras.removeFromTop (22));
        extras.removeFromTop (4);
        linkButton.setBounds (extras.removeFromTop (22));
    };

    auto layoutSix = [this] (juce::Rectangle<int> area,
                             juce::Slider& k1, juce::Label& l1,
                             juce::Slider& k2, juce::Label& l2,
                             juce::Slider& k3, juce::Label& l3,
                             juce::Slider& k4, juce::Label& l4,
                             juce::Slider& k5, juce::Label& l5,
                             juce::Slider& k6, juce::Label& l6)
    {
        area = area.reduced (2, 0);
        const int knobW = area.getWidth() / 3;

        auto row1 = area.removeFromTop (kKnobH);
        layoutKnob (k1, l1, row1.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k2, l2, row1.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k3, l3, row1.reduced (3, 0));

        area.removeFromTop (kGap);

        auto row2 = area;
        layoutKnob (k4, l4, row2.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k5, l5, row2.removeFromLeft (knobW).reduced (3, 0));
        layoutKnob (k6, l6, row2.reduced (3, 0));
    };

    layoutFive (left,
                thresholdSlider, thresholdLabel,
                ratioSlider, ratioLabel,
                attackSlider, attackLabel,
                releaseSlider, releaseLabel,
                makeupSlider, makeupLabel);

    layoutSix (right,
               detuneSlider, detuneLabel,
               filterSlider, filterLabel,
               crushHpfSlider, crushHpfLabel,
               driveSlider, driveLabel,
               mixSlider, mixLabel,
               outSlider, outLabel);

    // Meter shares exact knob-band ceiling/floor
    {
        auto m = meterArea.reduced (5, 0);
        grLabel.setBounds (m.removeFromTop (14));
        grMeter.setBounds (m);
    }

    // Footer row under the shared band — flush to window bottom padding
    const int footerY = kHeaderH + kPadTop + kTitleH + kKnobBandH + kGap;
    const int footerX = kPadX;
    const int footerW = (getWidth() - 2 * kPadX - kMeterW) / 2;

    auto placeFooter = [] (int x, int y, int w, juce::Label& label, juce::Component& control)
    {
        label.setBounds (x, y, w, 14);
        control.setBounds (x, y + 14, w, kBottomH - 14);
    };

    placeFooter (footerX, footerY, footerW - 4, sidechainLabel, sidechainInputCombo);
    placeFooter (footerX + footerW + kMeterW + 4, footerY, footerW - 4, layoutLabel, layoutCombo);
}

void NINE50AudioProcessorEditor::timerCallback()
{
    syncStageEnableLabels();

    const bool compOn = compOnButton.getToggleState();
    grMeter.setGainReduction (compOn ? audioProcessor.compressor.getGainReduction() : 0.0f);
}

void NINE50AudioProcessorEditor::syncStageEnableLabels()
{
    const auto sync = [] (juce::ToggleButton& button)
    {
        const auto* expected = button.getToggleState() ? "ON" : "BYPASS";
        if (button.getButtonText() != expected)
            button.setButtonText (expected);
    };

    sync (compOnButton);
    sync (crushOnButton);
}

void NINE50AudioProcessorEditor::refreshPresetList()
{
    auto& presets = audioProcessor.presetManager;
    presets.refreshUserPresets();

    presetCombo.clear (juce::dontSendNotification);
    const auto names = presets.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetCombo.addItem (names[i], i + 1);

    const int idx = presets.getCurrentIndex();
    if (presets.getNumPresets() > 0)
        presetCombo.setSelectedItemIndex (idx, juce::dontSendNotification);
}

void NINE50AudioProcessorEditor::loadSelectedPreset()
{
    const int idx = presetCombo.getSelectedItemIndex();
    if (idx >= 0)
        audioProcessor.presetManager.loadPreset (idx);
}

void NINE50AudioProcessorEditor::savePresetClicked()
{
    auto& presets = audioProcessor.presetManager;

    if (! presets.isFactoryPreset (presets.getCurrentIndex()))
    {
        if (presets.saveCurrentUserPreset())
            return;
    }

    showSaveAsDialog();
}

void NINE50AudioProcessorEditor::showSaveAsDialog()
{
    auto& presets = audioProcessor.presetManager;
    auto* aw = new juce::AlertWindow ("Save Preset As",
                                      "Name for the new preset:",
                                      juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", presets.getCurrentName().isNotEmpty()
                                   ? presets.getCurrentName() + " copy"
                                   : "My Preset");
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int result)
    {
        if (result == 1)
        {
            const auto name = aw->getTextEditorContents ("name");
            if (audioProcessor.presetManager.saveAsUserPreset (name))
                refreshPresetList();
        }
    }), true);
}

void NINE50AudioProcessorEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Save As...");
    menu.addItem (2, "Delete User Preset",
                  ! audioProcessor.presetManager.isFactoryPreset (
                      audioProcessor.presetManager.getCurrentIndex()));
    menu.addSeparator();
    menu.addItem (3, "Reveal Presets Folder");
    menu.addItem (4, "Init");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetMenuButton),
                        [this] (int result)
                        {
                            auto& presets = audioProcessor.presetManager;

                            if (result == 1)
                            {
                                showSaveAsDialog();
                            }
                            else if (result == 2)
                            {
                                auto* aw = new juce::AlertWindow (
                                    "Delete Preset",
                                    "Delete \"" + presets.getCurrentName() + "\"?",
                                    juce::MessageBoxIconType::WarningIcon);
                                aw->addButton ("Delete", 1);
                                aw->addButton ("Cancel", 0);
                                aw->enterModalState (true, juce::ModalCallbackFunction::create (
                                                               [this, aw] (int r)
                                                               {
                                                                   juce::ignoreUnused (aw);
                                                                   if (r == 1
                                                                       && audioProcessor.presetManager
                                                                              .deleteCurrentUserPreset())
                                                                       refreshPresetList();
                                                               }),
                                                     true);
                            }
                            else if (result == 3)
                            {
                                presets.getUserPresetsDirectory().revealToUser();
                            }
                            else if (result == 4)
                            {
                                presets.loadFactoryProgram (0);
                                refreshPresetList();
                            }
                        });
}
