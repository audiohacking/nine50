#include "SidechainCompressor.h"

SidechainCompressor::SidechainCompressor() {
}

void SidechainCompressor::prepare(double sr, int /*samplesPerBlock*/, int channels) {
    sampleRate = sr;
    numChannels = channels;

    envelopeState.assign(static_cast<size_t>(numChannels), 0.0f);
    hpfFilters.clear();
    hpfFilters.reserve(static_cast<size_t>(numChannels));

    for (int i = 0; i < numChannels; ++i) {
        hpfFilters.push_back(std::make_unique<juce::IIRFilter>());
    }
}

void SidechainCompressor::reset() {
    std::fill(envelopeState.begin(), envelopeState.end(), 0.0f);
    currentGainReduction = 0.0f;

    for (auto& f : hpfFilters) {
        f->reset();
    }
}

void SidechainCompressor::updateFilters(float hpf_Hz) {
    if (hpf_Hz <= 0.0f) {
        // Bypass - reset filters
        for (auto& f : hpfFilters) {
            f->reset();
        }
    } else {
        auto coeffs = juce::IIRCoefficients::makeHighPass(sampleRate, hpf_Hz);
        for (auto& f : hpfFilters) {
            f->setCoefficients(coeffs);
        }
    }
}

void SidechainCompressor::process(juce::AudioBuffer<float>& sidechainBuffer,
                                  juce::AudioBuffer<float>& mainBuffer,
                                  float threshold_dB,
                                  float ratio,
                                  float attack_ms,
                                  float release_ms,
                                  float makeup_dB,
                                  float hpf_Hz,
                                  bool linkChannels) {
    if (sampleRate <= 0.0 || mainBuffer.getNumSamples() == 0)
        return;

    if (envelopeState.empty() || hpfFilters.empty())
        return;

    const int numSamples = juce::jmin(sidechainBuffer.getNumSamples(), mainBuffer.getNumSamples());
    if (numSamples == 0)
        return;

    const int scChannels = juce::jmin (numChannels,
                                       sidechainBuffer.getNumChannels(),
                                       static_cast<int> (envelopeState.size()),
                                       static_cast<int> (hpfFilters.size()));
    const int mainChannels = juce::jmin (numChannels, mainBuffer.getNumChannels());
    if (scChannels <= 0 || mainChannels <= 0)
        return;

    // Update HPF if needed
    updateFilters(hpf_Hz);

    // Apply HPF to sidechain signal (scratch copy only — never the host buffer)
    if (hpf_Hz > 0.0f) {
        for (int ch = 0; ch < scChannels; ++ch) {
            hpfFilters[static_cast<size_t>(ch)]->processSamples(sidechainBuffer.getWritePointer(ch), numSamples);
        }
    }

    // Compute attack/release coefficients
    attackCoeff = std::exp(-1.0f / (attack_ms * 0.001f * static_cast<float>(sampleRate)));
    releaseCoeff = std::exp(-1.0f / (release_ms * 0.001f * static_cast<float>(sampleRate)));

    // Compute threshold and ratio in linear
    const float threshold_lin = std::pow(10.0f, threshold_dB / 20.0f);
    const float ratioInv = 1.0f / juce::jmax(0.1f, ratio);
    const float makeup_lin = std::pow(10.0f, makeup_dB / 20.0f);

    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample) {
        // RMS detection from sidechain
        float sidechainLevel = 0.0f;
        for (int ch = 0; ch < scChannels; ++ch) {
            float s = sidechainBuffer.getReadPointer(ch)[sample];
            sidechainLevel += s * s;
        }
        sidechainLevel = std::sqrt(sidechainLevel / static_cast<float>(scChannels));

        // Update envelope with attack/release smoothing
        if (sidechainLevel > envelopeState[0]) {
            envelopeState[0] = attackCoeff * (envelopeState[0] - sidechainLevel) + sidechainLevel;
        } else {
            envelopeState[0] = releaseCoeff * (envelopeState[0] - sidechainLevel) + sidechainLevel;
        }

        // If linked, copy envelope to all channels
        if (linkChannels) {
            for (int ch = 1; ch < scChannels; ++ch) {
                envelopeState[static_cast<size_t>(ch)] = envelopeState[0];
            }
        } else {
            // Per-channel detection
            for (int ch = 1; ch < scChannels; ++ch) {
                float s = sidechainBuffer.getReadPointer(ch)[sample];
                float level = std::abs(s);
                if (level > envelopeState[static_cast<size_t>(ch)]) {
                    envelopeState[static_cast<size_t>(ch)] = attackCoeff * (envelopeState[static_cast<size_t>(ch)] - level) + level;
                } else {
                    envelopeState[static_cast<size_t>(ch)] = releaseCoeff * (envelopeState[static_cast<size_t>(ch)] - level) + level;
                }
            }
        }

        // Compute gain reduction
        float env = envelopeState[0]; // Use linked envelope or channel 0
        if (!linkChannels) {
            // Use max envelope across channels for unlinked mode
            env = 0.0f;
            for (int ch = 0; ch < scChannels; ++ch) {
                env = juce::jmax(env, envelopeState[static_cast<size_t>(ch)]);
            }
        }

        float gainReduction_dB = 0.0f;
        if (env > threshold_lin) {
            // Above threshold: apply compression
            float env_dB = 20.0f * std::log10(juce::jmax(env, 1e-10f));
            gainReduction_dB = (env_dB - threshold_dB) * (1.0f - ratioInv);
            gainReduction_dB = juce::jmax(0.0f, gainReduction_dB);
        } else {
            // Below threshold: no reduction
            gainReduction_dB = 0.0f;
        }

        currentGainReduction = gainReduction_dB;

        // Apply gain reduction and makeup gain to main buffer
        float gain = std::pow(10.0f, -gainReduction_dB / 20.0f) * makeup_lin;

        for (int ch = 0; ch < mainChannels; ++ch) {
            mainBuffer.getWritePointer(ch)[sample] *= gain;
        }
    }
}
