#pragma once

#include <JuceHeader.h>

namespace NINE50Colours
{
    inline const juce::Colour panelTop      { 0xff22252a };
    inline const juce::Colour panelBottom   { 0xff141618 };
    inline const juce::Colour headerBg      { 0xff0c0d0f };
    inline const juce::Colour sectionLine   { 0xff3a3d42 };
    inline const juce::Colour label         { 0xffc8c4bb };
    inline const juce::Colour labelDim      { 0xff7a7670 };
    inline const juce::Colour brand         { 0xfff2ebe0 };
    inline const juce::Colour amber         { 0xffe8a030 };
    inline const juce::Colour amberHot      { 0xffffc14d };
    inline const juce::Colour amberDim      { 0xff6a4a18 };
    inline const juce::Colour lcdBg         { 0xff0a0b0c };
    inline const juce::Colour lcdText       { 0xffe8a030 };
    inline const juce::Colour knobBody      { 0xff2c3036 };
    inline const juce::Colour knobRim       { 0xff4a4e54 };
    inline const juce::Colour knobPointer   { 0xfff5f0e6 };
    inline const juce::Colour buttonOff     { 0xff1e2126 };
    inline const juce::Colour buttonOn      { 0xff3a2a10 };
}

class NINE50LookAndFeel : public juce::LookAndFeel_V4
{
public:
    NINE50LookAndFeel();
    ~NINE50LookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override;

private:
    juce::Font makeFont (float height, bool bold = false) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NINE50LookAndFeel)
};
