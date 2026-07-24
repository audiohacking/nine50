#include "NINE50LookAndFeel.h"

NINE50LookAndFeel::NINE50LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, NINE50Colours::lcdText);
    setColour (juce::Slider::textBoxBackgroundColourId, NINE50Colours::lcdBg);
    setColour (juce::Slider::textBoxOutlineColourId, NINE50Colours::sectionLine);
    setColour (juce::Slider::textBoxHighlightColourId, NINE50Colours::amberDim);

    setColour (juce::ComboBox::backgroundColourId, NINE50Colours::lcdBg);
    setColour (juce::ComboBox::outlineColourId, NINE50Colours::sectionLine);
    setColour (juce::ComboBox::textColourId, NINE50Colours::lcdText);
    setColour (juce::ComboBox::arrowColourId, NINE50Colours::amber);
    setColour (juce::ComboBox::buttonColourId, NINE50Colours::buttonOff);

    setColour (juce::PopupMenu::backgroundColourId, NINE50Colours::panelTop);
    setColour (juce::PopupMenu::textColourId, NINE50Colours::label);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, NINE50Colours::buttonOn);
    setColour (juce::PopupMenu::highlightedTextColourId, NINE50Colours::amberHot);

    setColour (juce::Label::textColourId, NINE50Colours::label);
    setColour (juce::ToggleButton::textColourId, NINE50Colours::label);
    setColour (juce::ToggleButton::tickColourId, NINE50Colours::amber);
    setColour (juce::ToggleButton::tickDisabledColourId, NINE50Colours::labelDim);
}

juce::Font NINE50LookAndFeel::makeFont (float height, bool bold) const
{
    const int style = bold ? juce::Font::bold : juce::Font::plain;
    juce::Font font (juce::FontOptions ("Avenir Next Condensed", height, style));

    if (font.getTypefaceName().isEmpty()
        || font.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
        font = juce::Font (juce::FontOptions ("Helvetica Neue", height, style));

    return font;
}

juce::Font NINE50LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return makeFont (12.0f);
}

juce::Font NINE50LookAndFeel::getLabelFont (juce::Label& label)
{
    const auto h = static_cast<float> (label.getHeight());
    return makeFont (juce::jlimit (10.0f, 13.0f, h * 0.7f));
}

juce::Font NINE50LookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return makeFont (static_cast<float> (buttonHeight) * 0.45f);
}

juce::Slider::SliderLayout NINE50LookAndFeel::getSliderLayout (juce::Slider& slider)
{
    auto layout = LookAndFeel_V4::getSliderLayout (slider);

    if (slider.isRotary())
    {
        auto bounds = slider.getLocalBounds();
        layout.textBoxBounds = bounds.removeFromBottom (18).reduced (4, 0);
        layout.sliderBounds = bounds.reduced (4, 2);
    }

    return layout;
}

void NINE50LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                static_cast<float> (y),
                                                static_cast<float> (width),
                                                static_cast<float> (height)).reduced (3.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto knobRadius = radius * 0.72f;

    // Track arc (recessed)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (NINE50Colours::headerBg);
    g.strokePath (backgroundArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

    // Value arc
    if (radius > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                rotaryStartAngle, toAngle, true);
        g.setColour (NINE50Colours::amber);
        g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // Knob body
    juce::ColourGradient bodyGrad (NINE50Colours::knobRim.brighter (0.15f),
                                   centre.x, centre.y - knobRadius,
                                   NINE50Colours::knobBody.darker (0.35f),
                                   centre.x, centre.y + knobRadius, false);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius,
                   knobRadius * 2.0f, knobRadius * 2.0f);

    g.setColour (NINE50Colours::knobRim.darker (0.4f));
    g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius,
                   knobRadius * 2.0f, knobRadius * 2.0f, 1.2f);

    // Pointer
    juce::Path pointer;
    const float pointerLength = knobRadius * 0.78f;
    const float pointerThickness = 2.2f;
    pointer.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength,
                                 pointerThickness, pointerLength * 0.72f, 1.0f);
    pointer.applyTransform (juce::AffineTransform::rotation (toAngle)
                                .translated (centre.x, centre.y));
    g.setColour (NINE50Colours::knobPointer);
    g.fillPath (pointer);

    // Center dimple
    g.setColour (NINE50Colours::headerBg);
    g.fillEllipse (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

void NINE50LookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
        const float alpha = label.isEnabled() ? 1.0f : 0.5f;

        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (getLabelFont (label));
        g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                          juce::jmax (1, static_cast<int> (textArea.getHeight() / 12.0f)),
                          label.getMinimumHorizontalScale());
    }
}

void NINE50LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();

    auto fill = on ? NINE50Colours::buttonOn : NINE50Colours::buttonOff;
    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.12f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.06f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (on ? NINE50Colours::amber : NINE50Colours::sectionLine);
    g.drawRoundedRectangle (bounds, 3.0f, on ? 1.4f : 1.0f);

    // Lit indicator pip
    auto pip = bounds.removeFromLeft (10.0f).reduced (2.5f, bounds.getHeight() * 0.32f);
    g.setColour (on ? NINE50Colours::amberHot : NINE50Colours::amberDim);
    g.fillEllipse (pip);

    g.setColour (on ? NINE50Colours::amberHot : NINE50Colours::label);
    g.setFont (makeFont (11.5f, true));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().reduced (12, 0),
                      juce::Justification::centred, 1);
}

void NINE50LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (backgroundColour);

    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto fill = NINE50Colours::buttonOff;
    if (shouldDrawButtonAsDown)
        fill = NINE50Colours::buttonOn;
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.08f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (shouldDrawButtonAsDown ? NINE50Colours::amber : NINE50Colours::sectionLine);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
}

void NINE50LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    g.setFont (makeFont (11.0f, true));
    g.setColour (NINE50Colours::label);
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 0),
                      juce::Justification::centred, 1);
}

void NINE50LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH, isButtonDown);

    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                          static_cast<float> (width),
                                          static_cast<float> (height));

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    const auto arrowZone = bounds.removeFromRight (18.0f).reduced (4.0f, 7.0f);
    juce::Path arrow;
    arrow.addTriangle (arrowZone.getX(), arrowZone.getY(),
                       arrowZone.getRight(), arrowZone.getY(),
                       arrowZone.getCentreX(), arrowZone.getBottom());
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void NINE50LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void NINE50LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (NINE50Colours::sectionLine);
    g.drawRect (0, 0, width, height, 1);
}

void NINE50LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                           bool isSeparator, bool isActive, bool isHighlighted,
                                           bool isTicked, bool hasSubMenu,
                                           const juce::String& text, const juce::String& shortcutKeyText,
                                           const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    juce::ignoreUnused (shortcutKeyText, icon, hasSubMenu);

    if (isSeparator)
    {
        auto r = area.reduced (8, 0);
        g.setColour (NINE50Colours::sectionLine);
        g.fillRect (r.removeFromTop (1).withY (r.getCentreY()));
        return;
    }

    auto r = area.reduced (1);

    if (isHighlighted && isActive)
    {
        g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRect (r);
    }

    g.setColour (textColourToUse != nullptr ? *textColourToUse
                 : isHighlighted ? findColour (juce::PopupMenu::highlightedTextColourId)
                                 : findColour (juce::PopupMenu::textColourId));
    g.setFont (makeFont (12.5f));

    auto textR = r.reduced (8, 0);
    if (isTicked)
    {
        g.setColour (NINE50Colours::amber);
        g.fillEllipse (static_cast<float> (textR.getX()),
                       static_cast<float> (r.getCentreY() - 3), 6.0f, 6.0f);
        textR.removeFromLeft (12);
    }

    g.drawFittedText (text, textR, juce::Justification::centredLeft, 1);
}
