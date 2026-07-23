#include "SP950Emulation.h"

SP950Emulation::SP950Emulation() {
}

void SP950Emulation::prepare(double sr, int spb, int channels) {
    sampleRate = sr;
    samplesPerBlock = spb;
    numChannels = channels;

    phaseAccum.assign(static_cast<size_t>(numChannels), 0.0);
    interpolators.clear();
    interpolators.reserve(static_cast<size_t>(numChannels));

    for (int i = 0; i < numChannels; ++i) {
        interpolators.push_back(std::make_unique<juce::LagrangeInterpolator>());
    }

    lpf_L = std::make_unique<juce::IIRFilter>();
    lpf_R = std::make_unique<juce::IIRFilter>();

    srRatio = sampleRate / targetSampleRate;
    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
}

void SP950Emulation::reset() {
    std::fill(phaseAccum.begin(), phaseAccum.end(), 0.0);

    for (auto& interp : interpolators) {
        interp->reset();
    }

    if (lpf_L) lpf_L->reset();
    if (lpf_R) lpf_R->reset();
}

void SP950Emulation::processBitReduction(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            // TPDF dither noise
            float dither = random.nextFloat() - random.nextFloat();

            // Truncate to 12-bit with dither
            float truncated = std::floor(data[i] * bitScale + dither) / bitScale;

            data[i] = truncated;
        }
    }
}

void SP950Emulation::processSampleRateReduction(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);

        double phase = phaseAccum[static_cast<size_t>(ch)];
        double out = 0.0;

        for (int i = 0; i < numSamples; ++i) {
            out += data[i];

            phase += 1.0;
            if (phase >= srRatio) {
                phase -= srRatio;
                // Hold the averaged value for the sample period
                data[i] = static_cast<float>(out / srRatio);
                out = 0.0;
            } else {
                // Hold previous value
                data[i] = static_cast<float>(out / phase);
            }
        }

        phaseAccum[static_cast<size_t>(ch)] = phase;
    }
}

void SP950Emulation::processDetune(juce::AudioBuffer<float>& buffer, float detune_st, bool ext, bool fine) {
    // Calculate pitch ratio
    float maxDetune = ext ? 30.0f : 15.0f;
    detune_st = juce::jlimit(-maxDetune, maxDetune, detune_st);

    if (fine) {
        // 0.1 semitone steps
        detune_st = std::round(detune_st * 10.0f) / 10.0f;
    } else {
        // Semitone steps
        detune_st = std::round(detune_st);
    }

    if (std::abs(detune_st) < 0.01f)
        return;

    float pitchRatio = std::pow(2.0f, detune_st / 12.0f);

    // Use Lagrange interpolator for pitch shifting
    for (int ch = 0; ch < buffer.getNumChannels() && ch < static_cast<int>(interpolators.size()); ++ch) {
        float* data = buffer.getWritePointer(ch);
        interpolators[static_cast<size_t>(ch)]->process(pitchRatio, data, data, buffer.getNumSamples());
    }
}

void SP950Emulation::processFilter(juce::AudioBuffer<float>& buffer, float filter_val) {
    // Map 0-99 to cutoff frequency (99 = bypass)
    if (filter_val >= 98.5f) {
        // Bypass filter
        return;
    }

    // Map 0-99 to 100 Hz - 20 kHz (logarithmic)
    float minFreq = 100.0f;
    float maxFreq = 20000.0f;
    float normalized = filter_val / 99.0f;
    float cutoff = minFreq * std::pow(maxFreq / minFreq, normalized);

    // Create 6th-order Butterworth by cascading 3 2nd-order sections
    auto coeffs = juce::IIRCoefficients::makeLowPass(sampleRate, cutoff);

    if (buffer.getNumChannels() >= 1 && lpf_L) {
        lpf_L->setCoefficients(coeffs);
        lpf_L->processSamples(buffer.getWritePointer(0), buffer.getNumSamples());
    }
    if (buffer.getNumChannels() >= 2 && lpf_R) {
        lpf_R->setCoefficients(coeffs);
        lpf_R->processSamples(buffer.getWritePointer(1), buffer.getNumSamples());
    }
}

void SP950Emulation::applyLayout(juce::AudioBuffer<float>& buffer, int layout) {
    const int numSamples = buffer.getNumSamples();

    if (buffer.getNumChannels() < 2)
        return;

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    switch (layout) {
        case MonoSum: {
            // Sum to mono
            for (int i = 0; i < numSamples; ++i) {
                float mono = (left[i] + right[i]) * 0.5f;
                left[i] = mono;
                right[i] = mono;
            }
            break;
        }
        case MonoL: {
            // Mono left only
            for (int i = 0; i < numSamples; ++i) {
                right[i] = left[i];
            }
            break;
        }
        case MonoR: {
            // Mono right only
            for (int i = 0; i < numSamples; ++i) {
                left[i] = right[i];
            }
            break;
        }
        case Stereo:
            // No change
            break;
        case StereoL: {
            // Stereo, left only to both
            for (int i = 0; i < numSamples; ++i) {
                right[i] = left[i];
            }
            break;
        }
        case StereoR: {
            // Stereo, right only to both
            for (int i = 0; i < numSamples; ++i) {
                left[i] = right[i];
            }
            break;
        }
        case MidSide: {
            // Convert to mid/side, process, convert back
            for (int i = 0; i < numSamples; ++i) {
                float mid = (left[i] + right[i]) * 0.7071f;
                float side = (left[i] - right[i]) * 0.7071f;
                left[i] = mid;
                right[i] = side;
            }
            break;
        }
        default:
            break;
    }
}

void SP950Emulation::applyMix(juce::AudioBuffer<float>& buffer, float mix) {
    const int numSamples = buffer.getNumSamples();
    const float wetGain = mix;
    const float dryGain = 1.0f - mix;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        const float* dry = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            data[i] = data[i] * wetGain + dry[i] * dryGain;
        }
    }
}

void SP950Emulation::process(juce::AudioBuffer<float>& buffer,
                             float drive_dB,
                             float detune_st,
                             bool ext,
                             bool fine,
                             float filter_val,
                             int layout,
                             float mix,
                             float out_dB,
                             bool link) {
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Store dry signal for mix
    dryBuffer.setSize(buffer.getNumChannels(), numSamples, false, false, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        dryBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), numSamples);
    }

    // Apply drive (input gain)
    float drive_lin = std::pow(10.0f, drive_dB / 20.0f);
    float out_lin = std::pow(10.0f, out_dB / 20.0f);

    // If link is on, tie out to drive
    if (link) {
        out_lin = drive_lin;
    }

    buffer.applyGain(drive_lin);

    // Apply detune (pitch shift)
    processDetune(buffer, detune_st, ext, fine);

    // Apply sample rate reduction
    processSampleRateReduction(buffer);

    // Apply bit reduction
    processBitReduction(buffer);

    // Apply filter
    processFilter(buffer, filter_val);

    // Apply output gain
    buffer.applyGain(out_lin);

    // Apply mix (dry/wet)
    applyMix(buffer, mix);

    // Apply layout routing (after mix so it affects final output)
    applyLayout(buffer, layout);
}
