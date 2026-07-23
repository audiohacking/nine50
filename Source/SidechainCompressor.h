#pragma once

#include <JuceHeader.h>

class SidechainCompressor {
public:
    SidechainCompressor();
    ~SidechainCompressor() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& sidechainBuffer,
                 juce::AudioBuffer<float>& mainBuffer,
                 float threshold_dB,
                 float ratio,
                 float attack_ms,
                 float release_ms,
                 float makeup_dB,
                 float hpf_Hz,
                 bool linkChannels);

    float getGainReduction() const { return currentGainReduction; }

private:
    void updateFilters(float hpf_Hz);

    double sampleRate = 44100.0;
    int numChannels = 2;

    // RMS detection state
    std::vector<float> envelopeState;

    // Attack/release smoothing coefficients
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Sidechain high-pass filters
    std::vector<std::unique_ptr<juce::IIRFilter>> hpfFilters;

    // Current gain reduction (dB)
    float currentGainReduction = 0.0f;
};
