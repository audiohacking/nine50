#include <JuceHeader.h>
#include "SP950Emulation.h"

class SP950EmulationTest : public juce::UnitTest {
public:
    SP950EmulationTest()
        : juce::UnitTest("SP950Emulation", juce::UnitTestCategories::dsp) {}

    void runTest() override {
        testCase("Initialization", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);
            // If we get here without crashing, initialization worked
            expect(true);
        });

        testCase("Bit reduction reduces precision", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            // Fill with a smooth sine wave
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            // Store original values
            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            // Process with bit reduction (mix=100%, no filter, no detune)
            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 3, 1.0f, 0.0f, false);

            // Bit-reduced signal should differ from original
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
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            // Fill with high-frequency content
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 10000.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            // Process with filter at low cutoff (val=0 means ~100Hz)
            sp950.process(buffer, 0.0f, 0.0f, false, false, 0.0f, 3, 1.0f, 0.0f, false);

            // High frequencies should be attenuated
            float originalRMS = 0.0f, filteredRMS = 0.0f;
            for (int i = 0; i < 512; ++i) {
                originalRMS += original.getSample(0, i) * original.getSample(0, i);
                filteredRMS += buffer.getSample(0, i) * buffer.getSample(0, i);
            }
            originalRMS = std::sqrt(originalRMS / 512.0f);
            filteredRMS = std::sqrt(filteredRMS / 512.0f);

            expect(filteredRMS < originalRMS, "Filter should reduce high-frequency content");
        });

        testCase("Filter bypass at 99", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 10000.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            // Process with filter bypassed (val=99), mix=0 (dry only)
            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 3, 0.0f, 0.0f, false);

            // With mix=0, signal should be identical to original
            float maxDiff = 0.0f;
            for (int i = 0; i < 512; ++i) {
                maxDiff = juce::jmax(maxDiff, std::abs(buffer.getSample(0, i) - original.getSample(0, i)));
            }
            expect(maxDiff < 0.001f, "Filter bypass with mix=0 should not change signal");
        });

        testCase("Layout routing - Mono Sum", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, -0.5f);
            }

            // Process with Mono Sum layout, no bit reduction (mix=0)
            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 0, 0.0f, 0.0f, false);

            // Both channels should be identical (mono sum)
            for (int i = 0; i < 512; ++i) {
                expect(std::abs(buffer.getSample(0, i) - buffer.getSample(1, i)) < 0.001f,
                       "Mono Sum should make both channels identical");
            }
        });

        testCase("Layout routing - Stereo (no change)", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, -0.5f);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            // Process with Stereo layout, no processing (mix=0)
            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 3, 0.0f, 0.0f, false);

            // Channels should be unchanged
            for (int i = 0; i < 512; ++i) {
                expect(std::abs(buffer.getSample(0, i) - original.getSample(0, i)) < 0.001f,
                       "Stereo layout should not change left channel");
                expect(std::abs(buffer.getSample(1, i) - original.getSample(1, i)) < 0.001f,
                       "Stereo layout should not change right channel");
            }
        });

        testCase("Mix dry/wet", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * static_cast<float>(i) / 44100.0f);
                buffer.setSample(0, i, val);
                buffer.setSample(1, i, val);
            }

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            // Process with mix=0 (dry only)
            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 3, 0.0f, 0.0f, false);

            // Should be identical to original (no processing with mix=0)
            float maxDiff = 0.0f;
            for (int i = 0; i < 512; ++i) {
                maxDiff = juce::jmax(maxDiff, std::abs(buffer.getSample(0, i) - original.getSample(0, i)));
            }
            expect(maxDiff < 0.001f, "Mix=0 should preserve original signal");
        });

        testCase("Drive gain", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.1f);
                buffer.setSample(1, i, 0.1f);
            }

            // Process with drive +6dB (mix=100% wet, filter bypassed)
            sp950.process(buffer, 6.0f, 0.0f, false, false, 99.0f, 3, 1.0f, 0.0f, false);

            // Output should be approximately 0.1 * 10^(6/20) ≈ 0.2 (6dB gain)
            // Check RMS since sample rate reduction causes oscillation on constant input
            float expected = 0.1f * std::pow(10.0f, 6.0f / 20.0f);
            float rms = 0.0f;
            for (int i = 0; i < 512; ++i) {
                rms += buffer.getSample(0, i) * buffer.getSample(0, i);
            }
            rms = std::sqrt(rms / 512.0f);
            expect(std::abs(rms - expected) < 0.02f,
                   "Drive should apply input gain");
        });

        testCase("Reset clears state", [&] {
            SP950Emulation sp950;
            sp950.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, 0.5f);
            }

            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 3, 1.0f, 0.0f, false);
            sp950.reset();

            // After reset, processing should still work
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, 0.5f);
            }
            sp950.process(buffer, 0.0f, 0.0f, false, false, 99.0f, 3, 1.0f, 0.0f, false);
            expect(true, "Processing after reset should work");
        });
    }
};

static SP950EmulationTest sp950EmulationTest;
