#include "GainReductionMeter.h"

GainReductionMeter::GainReductionMeter() {
    startTimerHz(30); // Update at ~30fps
}

void GainReductionMeter::paint(juce::Graphics& g) {
    juce::Rectangle<int> bounds = getLocalBounds().reduced(2);

    // Background
    g.fillAll(juce::Colours::black.darker(0.5f));

    // Draw border
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(bounds, 1);

    // Calculate bar height
    const float maxReduction = 30.0f; // Max 30dB of gain reduction
    const float boundsHeight = static_cast<float>(bounds.getHeight());
    const float boundsY = static_cast<float>(bounds.getY());

    // Draw LED-style bar
    const int numLEDs = 20;
    const float ledHeight = boundsHeight / static_cast<float>(numLEDs);

    for (int i = 0; i < numLEDs; ++i) {
        float ledTop = boundsY + boundsHeight - static_cast<float>(i + 1) * ledHeight;

        juce::Colour ledColor;
        float threshold = static_cast<float>(i) / static_cast<float>(numLEDs);

        if (threshold < currentGainReduction / maxReduction) {
            // Active LED
            if (threshold < 0.33f) {
                ledColor = juce::Colours::green;
            } else if (threshold < 0.66f) {
                ledColor = juce::Colours::yellow;
            } else {
                ledColor = juce::Colours::red;
            }
        } else {
            // Inactive LED
            ledColor = juce::Colours::darkgrey.darker(0.5f);
        }

        g.setColour(ledColor);
        g.fillRect(bounds.getX(), static_cast<int>(ledTop), bounds.getWidth(), static_cast<int>(ledHeight) - 1);
    }

    // Draw current value text
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);
    auto textBounds = bounds.removeFromBottom(12);
    g.drawText(juce::String(currentGainReduction, 1) + " dB",
               textBounds,
               juce::Justification::centred, false);
}

void GainReductionMeter::resized() {
    // No child components
}

void GainReductionMeter::setGainReduction(float gr_dB) {
    targetGainReduction = juce::jlimit(0.0f, 30.0f, gr_dB);
}

void GainReductionMeter::timerCallback() {
    // Smooth the gain reduction value
    const float smoothingFactor = 0.3f;
    currentGainReduction = currentGainReduction * (1.0f - smoothingFactor) + targetGainReduction * smoothingFactor;

    if (std::abs(currentGainReduction - targetGainReduction) > 0.1f) {
        repaint();
    } else {
        currentGainReduction = targetGainReduction;
    }
}
