#pragma once

#include <JuceHeader.h>
#include <array>

/**
 * SP-1200 / S950 bitcrush + downsample.
 *
 * Detune is a single control: pitch/duration stay constant while crush intensity
 * ramps from mild SP character (26.04 kHz / 12-bit) to heavy bitcrush
 * (sub-kHz holds + few-bit quantize). Techniques from Yeh / pitcher: open AA,
 * nearest-neighbour ADC, hard midtread quantize, ZOH, SP-flavoured output EQ.
 */
class BitCrushEmulation {
public:
    enum Layout {
        MonoSum = 0,
        MonoL,
        MonoR,
        Stereo,
        StereoL,
        StereoR,
        StereoMid,
        StereoSide,
        NumLayouts
    };

    BitCrushEmulation();
    ~BitCrushEmulation() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer,
                  float drive_dB,
                  float detune_st,
                  bool ext,
                  bool fine,
                  float hpf_val,
                  float lpf_val,
                  int layout,
                  float mix,
                  float out_dB,
                  bool link);

    static float quantizeDetune (float detune_st, bool ext, bool fine);

    /** 0 = mild SP base, 1 = max crush (respects EXT range). */
    static float crushAmount (float detune_st, bool ext, bool fine);

    /** Effective ADC rate for a detune setting (pitch stays constant). */
    static float effectiveSampleRate (float detune_st, bool ext, bool fine);

    /** Quantizer bit depth for a detune setting (12 → few bits). */
    static float bitDepthForDetune (float detune_st, bool ext, bool fine);

private:
    static constexpr int kMaxCh = 2;
    static constexpr int kAAOrderSections = 2;
    static constexpr int kS950OrderSections = 3;
    static constexpr int kHpfOrderSections = 2;
    static constexpr int kPostOrderSections = 3;

    void processBitReduction (juce::AudioBuffer<float>& buffer,
                              float bits,
                              float crushDrive);
    void processSampleRateReduction (juce::AudioBuffer<float>& buffer,
                                     float holdRatio,
                                     int phaseChannelBase);
    void processUserFilters (juce::AudioBuffer<float>& buffer,
                             float hpf_val,
                             float lpf_val,
                             int phaseChannelBase);
    void updatePreAA();
    void updatePostEQ();
    void updateUserFilters (float hpf_val, float lpf_val);

    static bool applyMonoLayout (juce::AudioBuffer<float>& buffer, int layout);

    void processPartialLayout (juce::AudioBuffer<float>& buffer,
                               int layout,
                               float drive_lin,
                               float out_lin,
                               float holdRatio,
                               float bits,
                               float crushDrive,
                               float hpf_val,
                               float lpf_val);

    void processFullPath (juce::AudioBuffer<float>& buffer,
                          float drive_lin,
                          float out_lin,
                          float holdRatio,
                          float bits,
                          float crushDrive,
                          float hpf_val,
                          float lpf_val,
                          int phaseChannelBase);

    void applyMix (juce::AudioBuffer<float>& buffer, float mix);

    static float midtreadQuantize (float x, float levels) noexcept;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    int numChannels = 2;

    static constexpr float targetSampleRate = 26040.0f;
    static constexpr float minRateNormal = 900.0f;   // max crush without EXT
    static constexpr float minRateExt = 400.0f;      // max crush with EXT
    static constexpr float aaNyquistFraction = 0.666f;

    std::array<double, kMaxCh> phaseAccum {};
    std::array<float, kMaxCh> heldSample {};

    std::array<std::array<juce::IIRFilter, kAAOrderSections>, kMaxCh> preAA {};
    std::array<std::array<juce::IIRFilter, kPostOrderSections>, kMaxCh> postEQ {};
    std::array<std::array<juce::IIRFilter, kS950OrderSections>, kMaxCh> s950Lpf {};
    std::array<std::array<juce::IIRFilter, kHpfOrderSections>, kMaxCh> userHpf {};

    float lastHpfVal = -1.0f;
    float lastLpfVal = -1.0f;
    bool filtersPrepared = false;

    juce::AudioBuffer<float> processScratch;
    juce::AudioBuffer<float> untouchedScratch;
    juce::AudioBuffer<float> dryBuffer;
};
