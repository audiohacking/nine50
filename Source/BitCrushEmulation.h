#pragma once

#include <JuceHeader.h>

class BitCrushEmulation {
public:
    enum Layout {
        MonoSum = 0,
        MonoL,
        MonoR,
        Stereo,
        StereoL,
        StereoR,
        MidSide,
        NumLayouts
    };

    BitCrushEmulation();
    ~BitCrushEmulation() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer,
                 float drive_dB,
                 float detune_st,
                 bool ext,
                 bool fine,
                 float filter_val,
                 int layout,
                 float mix,
                 float out_dB,
                 bool link);

private:
    void processBitReduction(juce::AudioBuffer<float>& buffer);
    void processSampleRateReduction(juce::AudioBuffer<float>& buffer);
    void processDetune(juce::AudioBuffer<float>& buffer, float detune_st, bool ext, bool fine);
    void processFilter(juce::AudioBuffer<float>& buffer, float filter_val);
    void applyLayout(juce::AudioBuffer<float>& buffer, int layout);
    void applyMix(juce::AudioBuffer<float>& buffer, float mix);

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    int numChannels = 2;

    // SP-1200 / S950 constants
    static constexpr float targetSampleRate = 26040.0f;
    static constexpr int bitDepth = 12;
    static constexpr float bitScale = 4096.0f;

    // Sample rate reduction state
    double srRatio = 1.0;
    std::vector<double> phaseAccum;

    // Detune state
    std::vector<std::unique_ptr<juce::LagrangeInterpolator>> interpolators;

    // Filter state (6th-order Butterworth LPF)
    std::unique_ptr<juce::IIRFilter> lpf_L;
    std::unique_ptr<juce::IIRFilter> lpf_R;

    // Dry buffer for mix
    juce::AudioBuffer<float> dryBuffer;

    // Random generator for dither
    juce::Random random;
};
