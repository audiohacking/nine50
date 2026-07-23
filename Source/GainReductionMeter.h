#pragma once

#include <JuceHeader.h>

class GainReductionMeter : public juce::Component,
                             private juce::Timer {
public:
    GainReductionMeter();
    ~GainReductionMeter() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setGainReduction(float gr_dB);
    float getGainReduction() const { return currentGainReduction; }

private:
    void timerCallback() override;

    float currentGainReduction = 0.0f;
    float targetGainReduction = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainReductionMeter)
};
