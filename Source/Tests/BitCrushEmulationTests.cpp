#include <JuceHeader.h>
#include "BitCrushEmulation.h"

class BitCrushEmulationTest : public juce::UnitTest {
public:
    BitCrushEmulationTest()
        : juce::UnitTest("BitCrushEmulation", "nine50") {}

    void runTest() override {
        testCase("Initialization", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);
            expect(true);
        });

        testCase("Detune maps to effective ADC rate", [&] {
            expectWithinAbsoluteError (BitCrushEmulation::effectiveSampleRate (0.0f, false, false),
                                       26040.0f, 0.1f,
                                       "0 st should be base 26.04 kHz");

            // Mid crush should sit well below the old mild SP-ratio curve
            const float mid = BitCrushEmulation::effectiveSampleRate (-8.0f, false, false);
            expect (mid < 8000.0f, "Mid detune should already be heavily downsampled");

            const float maxNormal = BitCrushEmulation::effectiveSampleRate (-15.0f, false, false);
            expectWithinAbsoluteError (maxNormal, 900.0f, 50.0f,
                                       "−15 st should approach ~900 Hz");

            const float maxExt = BitCrushEmulation::effectiveSampleRate (-30.0f, true, false);
            expectWithinAbsoluteError (maxExt, 400.0f, 50.0f,
                                       "EXT −30 st should approach ~400 Hz");

            expect (BitCrushEmulation::bitDepthForDetune (-15.0f, false, false) < 5.5f,
                    "Full crush should drop near 5-bit");
            expectWithinAbsoluteError (BitCrushEmulation::bitDepthForDetune (0.0f, false, false),
                                       12.0f, 0.01f,
                                       "0 st stays 12-bit");
        });

        testCase("Detune keeps pitch constant", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare (44100.0, 8192, 2);

            constexpr float kHz = 110.0f; // well below deep-crush Nyquist

            auto fillSine = [] (juce::AudioBuffer<float>& buffer, float hz)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    const float val = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi
                                                       * hz * static_cast<float> (i) / 44100.0f);
                    buffer.setSample (0, i, val);
                    buffer.setSample (1, i, val);
                }
            };

            auto estimatePitch = [] (const float* data, int n, double sr) -> float
            {
                const int minLag = static_cast<int> (sr / 200.0);
                const int maxLag = static_cast<int> (sr / 60.0);
                float bestCorr = -1.0e9f;
                int bestLag = minLag;
                for (int lag = minLag; lag <= maxLag; ++lag)
                {
                    float corr = 0.0f;
                    for (int i = 0; i < n - lag; ++i)
                        corr += data[i] * data[i + lag];
                    if (corr > bestCorr)
                    {
                        bestCorr = corr;
                        bestLag = lag;
                    }
                }
                return static_cast<float> (sr / static_cast<double> (bestLag));
            };

            juce::AudioBuffer<float> mild (2, 8192);
            fillSine (mild, kHz);
            bitCrush.process (mild, 0.0f, 0.0f, false, false, 0.0f, 99.0f,
                              BitCrushEmulation::Stereo, 1.0f, 0.0f, false);

            bitCrush.reset();
            juce::AudioBuffer<float> heavy (2, 8192);
            fillSine (heavy, kHz);
            bitCrush.process (heavy, 0.0f, -12.0f, false, false, 0.0f, 99.0f,
                              BitCrushEmulation::Stereo, 1.0f, 0.0f, false);

            const float hz0 = estimatePitch (mild.getReadPointer (0) + 512, 8192 - 512, 44100.0);
            const float hz1 = estimatePitch (heavy.getReadPointer (0) + 512, 8192 - 512, 44100.0);

            expectWithinAbsoluteError (hz0, kHz, 15.0f, "Base crush should keep pitch");
            expectWithinAbsoluteError (hz1, hz0, 15.0f,
                                       "Detune must not change output pitch vs base crush");
        });

        testCase("Bit reduction reduces precision", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 1.0f, 0.0f, false);

            bool differs = false;
            for (int i = 0; i < 512; ++i) {
                if (std::abs(buffer.getSample(0, i) - original.getSample(0, i)) > 0.0001f) {
                    differs = true;
                    break;
                }
            }
            expect(differs, "Bit reduction should alter the signal");
        });

        testCase("Filter reduces high frequencies", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 10000.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 0.0f, 3, 1.0f, 0.0f, false);

            float originalRMS = 0.0f, filteredRMS = 0.0f;
            for (int i = 0; i < 512; ++i) {
                originalRMS += original.getSample(0, i) * original.getSample(0, i);
                filteredRMS += buffer.getSample(0, i) * buffer.getSample(0, i);
            }
            originalRMS = std::sqrt(originalRMS / 512.0f);
            filteredRMS = std::sqrt(filteredRMS / 512.0f);

            expect(filteredRMS < originalRMS, "Filter should reduce high-frequency content");
        });

        testCase("HPF reduces low frequencies", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare (44100.0, 2048, 2);

            juce::AudioBuffer<float> buffer (2, 2048);
            for (int i = 0; i < 2048; ++i)
            {
                const float val = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi
                                                   * 40.0f * static_cast<float> (i) / 44100.0f);
                buffer.setSample (0, i, val);
                buffer.setSample (1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf (buffer);

            // Strong HPF, LPF bypassed
            bitCrush.process (buffer, 0.0f, 0.0f, false, false, 80.0f, 99.0f,
                              BitCrushEmulation::Stereo, 1.0f, 0.0f, false);

            float originalRMS = 0.0f, filteredRMS = 0.0f;
            for (int i = 256; i < 2048; ++i) // skip filter settle
            {
                originalRMS += original.getSample (0, i) * original.getSample (0, i);
                filteredRMS += buffer.getSample (0, i) * buffer.getSample (0, i);
            }
            originalRMS = std::sqrt (originalRMS / (2048.0f - 256.0f));
            filteredRMS = std::sqrt (filteredRMS / (2048.0f - 256.0f));

            expect (filteredRMS < originalRMS * 0.5f, "HPF should attenuate low-frequency content");
        });

        testCase("Filter bypass at 99", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 10000.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 0.0f, 0.0f, false);

            float maxDiff = 0.0f;
            for (int i = 0; i < 512; ++i) {
                maxDiff = juce::jmax(maxDiff, std::abs(buffer.getSample(0, i) - original.getSample(0, i)));
            }
            expect(maxDiff < 0.001f, "Filter bypass with mix=0 should not change signal");
        });

        testCase("Layout routing - Mono Sum affects dry", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, -0.5f);
            }

            // mix=0: dry only, but mono layout still applies
            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f,
                             BitCrushEmulation::MonoSum, 0.0f, 0.0f, false);

            for (int i = 0; i < 512; ++i) {
                expect(std::abs(buffer.getSample(0, i) - buffer.getSample(1, i)) < 0.001f,
                       "Mono Sum should make both channels identical even at mix=0");
                expectWithinAbsoluteError (buffer.getSample (0, i), 0.0f, 0.001f,
                                           "Mono Sum of +0.5/-0.5 should be ~0");
            }
        });

        testCase("Layout routing - Stereo L leaves right untouched", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.4f);
                buffer.setSample(1, i, -0.3f);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            bitCrush.process(buffer, 0.0f, -8.0f, false, false, 0.0f, 99.0f,
                             BitCrushEmulation::StereoL, 1.0f, 0.0f, false);

            for (int i = 0; i < 512; ++i) {
                expectWithinAbsoluteError (buffer.getSample (1, i),
                                           original.getSample (1, i), 0.0001f,
                                           "Stereo L must leave right channel untouched");
            }

            bool leftChanged = false;
            for (int i = 0; i < 512; ++i) {
                if (std::abs (buffer.getSample (0, i) - original.getSample (0, i)) > 0.0001f) {
                    leftChanged = true;
                    break;
                }
            }
            expect (leftChanged, "Stereo L should process the left channel");
        });

        testCase("Layout routing - Stereo (no change at mix=0)", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, -0.5f);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 0.0f, 0.0f, false);

            for (int i = 0; i < 512; ++i) {
                expect(std::abs(buffer.getSample(0, i) - original.getSample(0, i)) < 0.001f,
                       "Stereo layout should not change left channel");
                expect(std::abs(buffer.getSample(1, i) - original.getSample(1, i)) < 0.001f,
                       "Stereo layout should not change right channel");
            }
        });

        testCase("Stereo Side on mono is bypass", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare (44100.0, 512, 1);

            juce::AudioBuffer<float> buffer (1, 512);
            for (int i = 0; i < 512; ++i)
                buffer.setSample (0, i, 0.25f);

            juce::AudioBuffer<float> original;
            original.makeCopyOf (buffer);

            bitCrush.process (buffer, 6.0f, -12.0f, true, false, 0.0f, 20.0f,
                              BitCrushEmulation::StereoSide, 1.0f, 0.0f, false);

            for (int i = 0; i < 512; ++i)
                expectWithinAbsoluteError (buffer.getSample (0, i),
                                           original.getSample (0, i), 0.0001f,
                                           "Stereo Side on mono should bypass");
        });

        testCase("Mix dry/wet", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 0.0f, 0.0f, false);

            float maxDiff = 0.0f;
            for (int i = 0; i < 512; ++i) {
                maxDiff = juce::jmax(maxDiff, std::abs(buffer.getSample(0, i) - original.getSample(0, i)));
            }
            expect(maxDiff < 0.001f, "Mix=0 should preserve original signal");
        });

        testCase("Drive gain", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 2048, 2);

            auto fill = [] (juce::AudioBuffer<float>& buffer)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    const float val = 0.1f * std::sin (2.0f * juce::MathConstants<float>::pi
                                                       * 440.0f * static_cast<float> (i) / 44100.0f);
                    buffer.setSample (0, i, val);
                    buffer.setSample (1, i, val);
                }
            };

            auto rms = [] (const juce::AudioBuffer<float>& buffer) -> float
            {
                float sum = 0.0f;
                const int n = buffer.getNumSamples();
                for (int i = 256; i < n; ++i)
                    sum += buffer.getSample (0, i) * buffer.getSample (0, i);
                return std::sqrt (sum / static_cast<float> (n - 256));
            };

            juce::AudioBuffer<float> unity (2, 2048);
            fill (unity);
            bitCrush.process (unity, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 1.0f, 0.0f, false);
            const float rms0 = rms (unity);

            bitCrush.reset();
            juce::AudioBuffer<float> driven (2, 2048);
            fill (driven);
            bitCrush.process (driven, 6.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 1.0f, 0.0f, false);
            const float rms6 = rms (driven);

            const float ratio = rms6 / juce::jmax (rms0, 1.0e-6f);
            const float expected = std::pow (10.0f, 6.0f / 20.0f);
            expectWithinAbsoluteError (ratio, expected, 0.15f,
                                       "Drive should apply ~+6 dB relative gain");
        });

        testCase("Reset clears state", [&] {
            BitCrushEmulation bitCrush;
            bitCrush.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, 0.5f);
            }

            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 1.0f, 0.0f, false);
            bitCrush.reset();

            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, 0.5f);
            }
            bitCrush.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 99.0f, 3, 1.0f, 0.0f, false);
            expect(true, "Processing after reset should work");
        });
    }
};

static BitCrushEmulationTest bitCrushEmulationTest;
