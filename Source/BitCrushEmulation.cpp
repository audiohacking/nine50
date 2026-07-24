#include "BitCrushEmulation.h"
#include <cmath>

BitCrushEmulation::BitCrushEmulation() = default;

float BitCrushEmulation::quantizeDetune (float detune_st, bool ext, bool fine)
{
    const float maxDetune = ext ? -30.0f : -15.0f;
    detune_st = juce::jlimit (maxDetune, 0.0f, detune_st);

    if (fine)
        detune_st = std::round (detune_st * 10.0f) / 10.0f;
    else
        detune_st = std::round (detune_st);

    return detune_st;
}

float BitCrushEmulation::crushAmount (float detune_st, bool ext, bool fine)
{
    detune_st = quantizeDetune (detune_st, ext, fine);
    const float maxDetune = ext ? 30.0f : 15.0f;
    return juce::jlimit (0.0f, 1.0f, -detune_st / maxDetune);
}

float BitCrushEmulation::effectiveSampleRate (float detune_st, bool ext, bool fine)
{
    const float amount = crushAmount (detune_st, ext, fine);
    if (amount <= 0.0f)
        return targetSampleRate;

    // Exponential drop: 26.04 kHz → ~900 Hz (−15) or ~400 Hz (EXT −30).
    // Exponent < 1 pushes more crush into the first half of the knob.
    const float minRate = ext ? minRateExt : minRateNormal;
    const float shaped = std::pow (amount, 0.75f);
    return targetSampleRate * std::pow (minRate / targetSampleRate, shaped);
}

float BitCrushEmulation::bitDepthForDetune (float detune_st, bool ext, bool fine)
{
    const float amount = crushAmount (detune_st, ext, fine);
    // 12-bit SP base → 5-bit (or 4-bit with EXT) at full crush
    const float minBits = ext ? 4.0f : 5.0f;
    const float shaped = std::pow (amount, 0.85f);
    return juce::jmap (shaped, 0.0f, 1.0f, 12.0f, minBits);
}

float BitCrushEmulation::midtreadQuantize (float x, float levels) noexcept
{
    levels = juce::jmax (2.0f, levels);
    const float delta = 2.0f / levels;
    x = juce::jlimit (-1.0f, 1.0f - delta, x);
    const float idx = std::floor ((x + 1.0f) / delta + 1.0e-6f);
    return -1.0f + idx * delta;
}

void BitCrushEmulation::prepare (double sr, int spb, int channels)
{
    sampleRate = sr;
    samplesPerBlock = spb;
    numChannels = juce::jmax (1, channels);

    phaseAccum.fill (0.0);
    heldSample.fill (0.0f);

    lastHpfVal = -1.0f;
    lastLpfVal = -1.0f;
    filtersPrepared = false;

    const int scratchChannels = juce::jmax (2, numChannels);
    const int scratchSamples = juce::jmax (1, samplesPerBlock);
    dryBuffer.setSize (scratchChannels, scratchSamples, false, false, true);
    processScratch.setSize (scratchChannels, scratchSamples, false, false, true);
    untouchedScratch.setSize (scratchChannels, scratchSamples, false, false, true);

    updatePreAA();
    updatePostEQ();
    filtersPrepared = true;
}

void BitCrushEmulation::reset()
{
    phaseAccum.fill (0.0);
    heldSample.fill (0.0f);

    for (auto& ch : preAA)
        for (auto& f : ch)
            f.reset();
    for (auto& ch : postEQ)
        for (auto& f : ch)
            f.reset();
    for (auto& ch : s950Lpf)
        for (auto& f : ch)
            f.reset();
    for (auto& ch : userHpf)
        for (auto& f : ch)
            f.reset();
}

void BitCrushEmulation::updatePreAA()
{
    // Open AA so aliases fold in (pitcher elliptic Wn=0.666).
    const float hostNyquist = static_cast<float> (sampleRate * 0.5);
    const float cutoff = juce::jlimit (1000.0f, hostNyquist * 0.98f,
                                       aaNyquistFraction * hostNyquist);

    static constexpr float kQ[kAAOrderSections] = { 0.541196f, 1.306563f };
    for (auto& ch : preAA)
        for (int s = 0; s < kAAOrderSections; ++s)
            ch[static_cast<size_t> (s)].setCoefficients (
                juce::IIRCoefficients::makeLowPass (sampleRate, cutoff, kQ[s]));
}

void BitCrushEmulation::updatePostEQ()
{
    // Mild SP roll-off — keep top end so bitcrush stairs stay audible.
    // User LPF can darken; a hard 7.5 kHz mask hid the crush.
    const float hostNyquist = static_cast<float> (sampleRate * 0.5);
    const float cutoff = juce::jmin (12000.0f, hostNyquist * 0.9f);

    static constexpr float kQ[kPostOrderSections] = { 0.541196f, 0.707107f, 1.306563f };
    for (auto& ch : postEQ)
        for (int s = 0; s < kPostOrderSections; ++s)
            ch[static_cast<size_t> (s)].setCoefficients (
                juce::IIRCoefficients::makeLowPass (sampleRate, cutoff, kQ[s]));
}

void BitCrushEmulation::updateUserFilters (float hpf_val, float lpf_val)
{
    if (std::abs (hpf_val - lastHpfVal) > 0.01f)
    {
        lastHpfVal = hpf_val;

        if (hpf_val > 0.5f)
        {
            const float n = juce::jlimit (0.0f, 1.0f, hpf_val / 99.0f);
            const float cutoff = 20.0f * std::pow (2000.0f / 20.0f, n);
            static constexpr float kQ[kHpfOrderSections] = { 0.541196f, 1.306563f };
            for (auto& ch : userHpf)
                for (int s = 0; s < kHpfOrderSections; ++s)
                    ch[static_cast<size_t> (s)].setCoefficients (
                        juce::IIRCoefficients::makeHighPass (sampleRate, cutoff, kQ[s]));
        }
    }

    if (std::abs (lpf_val - lastLpfVal) > 0.01f)
    {
        lastLpfVal = lpf_val;

        if (lpf_val < 98.5f)
        {
            const float n = juce::jlimit (0.0f, 1.0f, lpf_val / 99.0f);
            const float cutoff = 100.0f * std::pow (20000.0f / 100.0f, n);
            static constexpr float kQ[kS950OrderSections] = { 0.517638f, 0.707107f, 1.931852f };
            for (auto& ch : s950Lpf)
                for (int s = 0; s < kS950OrderSections; ++s)
                    ch[static_cast<size_t> (s)].setCoefficients (
                        juce::IIRCoefficients::makeLowPass (sampleRate, cutoff, kQ[s]));
        }
    }
}

void BitCrushEmulation::processBitReduction (juce::AudioBuffer<float>& buffer,
                                             float bits,
                                             float crushDrive)
{
    const int numSamples = buffer.getNumSamples();
    const float levels = std::pow (2.0f, juce::jlimit (2.0f, 16.0f, bits));
    const float drive = juce::jmax (1.0f, crushDrive);
    const float invDrive = 1.0f / drive;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            // Drive into the rails so deep crush hits fewer codes harder
            float x = std::tanh (data[i] * drive);
            data[i] = midtreadQuantize (x, levels) * invDrive;
        }
    }
}

void BitCrushEmulation::processSampleRateReduction (juce::AudioBuffer<float>& buffer,
                                                    float holdRatio,
                                                    int phaseChannelBase)
{
    if (! filtersPrepared)
    {
        updatePreAA();
        updatePostEQ();
        filtersPrepared = true;
    }

    const int numSamples = buffer.getNumSamples();
    holdRatio = juce::jmax (1.0f, holdRatio);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const int phaseCh = phaseChannelBase + ch;
        if (phaseCh < 0 || phaseCh >= kMaxCh)
            break;

        float* data = buffer.getWritePointer (ch);

        for (auto& f : preAA[static_cast<size_t> (phaseCh)])
            f.processSamples (data, numSamples);

        double phase = phaseAccum[static_cast<size_t> (phaseCh)];
        float held = heldSample[static_cast<size_t> (phaseCh)];

        // Nearest-neighbour ADC + ZOH DAC
        for (int i = 0; i < numSamples; ++i)
        {
            if (phase < 1.0)
                held = data[i];

            data[i] = held;
            phase += 1.0;
            if (phase >= static_cast<double> (holdRatio))
                phase -= static_cast<double> (holdRatio);
        }

        phaseAccum[static_cast<size_t> (phaseCh)] = phase;
        heldSample[static_cast<size_t> (phaseCh)] = held;

        for (auto& f : postEQ[static_cast<size_t> (phaseCh)])
            f.processSamples (data, numSamples);
    }
}

void BitCrushEmulation::processUserFilters (juce::AudioBuffer<float>& buffer,
                                            float hpf_val,
                                            float lpf_val,
                                            int phaseChannelBase)
{
    updateUserFilters (hpf_val, lpf_val);

    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const int filterCh = phaseChannelBase + ch;
        if (filterCh < 0 || filterCh >= kMaxCh)
            break;

        float* data = buffer.getWritePointer (ch);

        if (hpf_val > 0.5f)
            for (auto& f : userHpf[static_cast<size_t> (filterCh)])
                f.processSamples (data, numSamples);

        if (lpf_val < 98.5f)
            for (auto& f : s950Lpf[static_cast<size_t> (filterCh)])
                f.processSamples (data, numSamples);
    }
}

bool BitCrushEmulation::applyMonoLayout (juce::AudioBuffer<float>& buffer, int layout)
{
    if (buffer.getNumChannels() < 2)
        return false;

    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    switch (layout)
    {
        case MonoSum:
            for (int i = 0; i < numSamples; ++i)
            {
                const float mono = (left[i] + right[i]) * 0.5f;
                left[i] = mono;
                right[i] = mono;
            }
            return true;

        case MonoL:
            for (int i = 0; i < numSamples; ++i)
                right[i] = left[i];
            return true;

        case MonoR:
            for (int i = 0; i < numSamples; ++i)
                left[i] = right[i];
            return true;

        default:
            return false;
    }
}

void BitCrushEmulation::processFullPath (juce::AudioBuffer<float>& buffer,
                                         float drive_lin,
                                         float out_lin,
                                         float holdRatio,
                                         float bits,
                                         float crushDrive,
                                         float hpf_val,
                                         float lpf_val,
                                         int phaseChannelBase)
{
    buffer.applyGain (drive_lin);
    processSampleRateReduction (buffer, holdRatio, phaseChannelBase);
    processBitReduction (buffer, bits, crushDrive);
    processUserFilters (buffer, hpf_val, lpf_val, phaseChannelBase);
    buffer.applyGain (out_lin);
}

void BitCrushEmulation::processPartialLayout (juce::AudioBuffer<float>& buffer,
                                              int layout,
                                              float drive_lin,
                                              float out_lin,
                                              float holdRatio,
                                              float bits,
                                              float crushDrive,
                                              float hpf_val,
                                              float lpf_val)
{
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
    {
        if (layout == StereoSide)
            return;

        processFullPath (buffer, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val, 0);
        return;
    }

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    auto ensureScratch = [numSamples] (juce::AudioBuffer<float>& buf, int chans)
    {
        if (buf.getNumChannels() < chans || buf.getNumSamples() < numSamples)
            buf.setSize (chans, numSamples, false, false, true);
    };

    ensureScratch (processScratch, 2);
    ensureScratch (untouchedScratch, 2);

    switch (layout)
    {
        case StereoL:
        {
            untouchedScratch.copyFrom (1, 0, right, numSamples);
            processScratch.copyFrom (0, 0, left, numSamples);
            {
                juce::AudioBuffer<float> monoView (processScratch.getArrayOfWritePointers(), 1, numSamples);
                processFullPath (monoView, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val, 0);
            }
            buffer.copyFrom (0, 0, processScratch, 0, 0, numSamples);
            buffer.copyFrom (1, 0, untouchedScratch, 1, 0, numSamples);
            break;
        }

        case StereoR:
        {
            untouchedScratch.copyFrom (0, 0, left, numSamples);
            processScratch.copyFrom (0, 0, right, numSamples);
            {
                juce::AudioBuffer<float> monoView (processScratch.getArrayOfWritePointers(), 1, numSamples);
                processFullPath (monoView, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val, 1);
            }
            buffer.copyFrom (0, 0, untouchedScratch, 0, 0, numSamples);
            buffer.copyFrom (1, 0, processScratch, 0, 0, numSamples);
            break;
        }

        case StereoMid:
        case StereoSide:
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float mid = (left[i] + right[i]) * 0.5f;
                const float side = (left[i] - right[i]) * 0.5f;
                processScratch.setSample (0, i, mid);
                processScratch.setSample (1, i, side);
            }

            const int processCh = (layout == StereoMid) ? 0 : 1;
            const int keepCh = 1 - processCh;

            untouchedScratch.copyFrom (keepCh, 0, processScratch, keepCh, 0, numSamples);

            juce::AudioBuffer<float> monoTemp (1, numSamples);
            monoTemp.copyFrom (0, 0, processScratch, processCh, 0, numSamples);
            processFullPath (monoTemp, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val, processCh);
            processScratch.copyFrom (processCh, 0, monoTemp, 0, 0, numSamples);
            processScratch.copyFrom (keepCh, 0, untouchedScratch, keepCh, 0, numSamples);

            for (int i = 0; i < numSamples; ++i)
            {
                const float mid = processScratch.getSample (0, i);
                const float side = processScratch.getSample (1, i);
                left[i] = mid + side;
                right[i] = mid - side;
            }
            break;
        }

        default:
            processFullPath (buffer, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val, 0);
            break;
    }
}

void BitCrushEmulation::applyMix (juce::AudioBuffer<float>& buffer, float mix)
{
    const int numSamples = buffer.getNumSamples();
    const float wetGain = mix;
    const float dryGain = 1.0f - mix;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        if (ch >= dryBuffer.getNumChannels())
            break;

        float* data = buffer.getWritePointer (ch);
        const float* dry = dryBuffer.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
            data[i] = data[i] * wetGain + dry[i] * dryGain;
    }
}

void BitCrushEmulation::process (juce::AudioBuffer<float>& buffer,
                                 float drive_dB,
                                 float detune_st,
                                 bool ext,
                                 bool fine,
                                 float hpf_val,
                                 float lpf_val,
                                 int layout,
                                 float mix,
                                 float out_dB,
                                 bool link)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    layout = juce::jlimit (0, static_cast<int> (NumLayouts) - 1, layout);

    if (buffer.getNumChannels() < 2 && layout == StereoSide)
        return;

    const float amount = crushAmount (detune_st, ext, fine);
    const float effRate = effectiveSampleRate (detune_st, ext, fine);
    const float bits = bitDepthForDetune (detune_st, ext, fine);
    // Drive into the ADC harder as crush deepens
    const float crushDrive = 1.0f + amount * amount * 5.0f;
    const float holdRatio = static_cast<float> (sampleRate / static_cast<double> (effRate));

    float drive_lin = std::pow (10.0f, drive_dB / 20.0f);
    float out_lin = std::pow (10.0f, out_dB / 20.0f);
    if (link)
        out_lin = drive_lin;

    applyMonoLayout (buffer, layout);

    if (dryBuffer.getNumChannels() < buffer.getNumChannels()
        || dryBuffer.getNumSamples() < numSamples)
    {
        dryBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        dryBuffer.copyFrom (ch, 0, buffer.getReadPointer (ch), numSamples);

    if (mix <= 0.0001f)
        return;

    const bool partial = (layout == StereoL || layout == StereoR
                          || layout == StereoMid || layout == StereoSide);

    if (partial)
        processPartialLayout (buffer, layout, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val);
    else
        processFullPath (buffer, drive_lin, out_lin, holdRatio, bits, crushDrive, hpf_val, lpf_val, 0);

    if (mix < 0.9999f)
        applyMix (buffer, mix);
}
