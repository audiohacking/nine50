#include "GainReductionMeter.h"
#include "NINE50LookAndFeel.h"

GainReductionMeter::GainReductionMeter()
{
    startTimerHz (30);
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (NINE50Colours::lcdBg);
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (NINE50Colours::sectionLine);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    auto valueArea = bounds.removeFromBottom (18.0f);
    auto ledArea = bounds.reduced (5.0f, 4.0f);

    const float maxReduction = 30.0f;
    const float gap = 2.0f;
    const int numLEDs = juce::jlimit (10, 18,
                                      static_cast<int> ((ledArea.getHeight() + gap) / (6.0f + gap)));
    const float ledH = (ledArea.getHeight() - gap * static_cast<float> (numLEDs - 1))
                       / static_cast<float> (numLEDs);
    const float activeFrac = juce::jlimit (0.0f, 1.0f, currentGainReduction / maxReduction);

    for (int i = 0; i < numLEDs; ++i)
    {
        const float y = ledArea.getBottom() - static_cast<float> (i + 1) * (ledH + gap) + gap;
        const float threshold = static_cast<float> (i) / static_cast<float> (numLEDs);
        const bool on = threshold < activeFrac;

        juce::Colour c;
        if (! on)
        {
            c = NINE50Colours::amberDim.withAlpha (0.35f);
        }
        else if (threshold < 0.55f)
        {
            c = NINE50Colours::amber;
        }
        else if (threshold < 0.8f)
        {
            c = NINE50Colours::amberHot;
        }
        else
        {
            c = juce::Colour (0xffff7a2e);
        }

        g.setColour (c);
        g.fillRoundedRectangle (ledArea.getX(), y, ledArea.getWidth(), ledH, 1.2f);
    }

    g.setColour (NINE50Colours::lcdText);
    g.setFont (juce::Font (juce::FontOptions ("Menlo", 10.0f, juce::Font::plain)));
    g.drawFittedText (juce::String (currentGainReduction, 1) + " dB",
                      valueArea.toNearestInt(),
                      juce::Justification::centred, 1);
}

void GainReductionMeter::resized()
{
}

void GainReductionMeter::setGainReduction (float gr_dB)
{
    targetGainReduction = juce::jlimit (0.0f, 30.0f, gr_dB);
}

void GainReductionMeter::timerCallback()
{
    constexpr float smoothingFactor = 0.3f;
    currentGainReduction = currentGainReduction * (1.0f - smoothingFactor)
                           + targetGainReduction * smoothingFactor;

    if (std::abs (currentGainReduction - targetGainReduction) > 0.05f)
        repaint();
    else
        currentGainReduction = targetGainReduction;
}
