#include <JuceHeader.h>
#include "SidechainCompressor.h"

class SidechainCompressorTest : public juce::UnitTest {
public:
    SidechainCompressorTest()
        : juce::UnitTest("SidechainCompressor", "nine50") {}

    void runTest() override {
        testCase("Initialization", [&] {
            SidechainCompressor comp;
            comp.prepare(44100.0, 512, 2);
            expect(comp.getGainReduction() == 0.0f, "Gain reduction should be 0 after init");
        });

        testCase("No sidechain signal - no gain reduction", [&] {
            SidechainCompressor comp;
            comp.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> sidechain(2, 512);
            sidechain.clear();
            juce::AudioBuffer<float> main(2, 512);
            main.clear();
            // Fill with test signal
            for (int i = 0; i < 512; ++i) {
                float val = 0.5f;
                main.setSample(0, i, val);
                main.setSample(1, i, val);
            }

            comp.process(sidechain, main, -20.0f, 8.0f, 10.0f, 100.0f, 0.0f, 0.0f, false);

            expect(comp.getGainReduction() == 0.0f, "No gain reduction with silent sidechain");
            // Main buffer should be unchanged (no makeup gain)
            expect(std::abs(main.getSample(0, 0) - 0.5f) < 0.001f, "Main buffer should be unchanged");
        });

        testCase("Sidechain signal above threshold - gain reduction", [&] {
            SidechainCompressor comp;
            comp.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> sidechain(2, 512);
            sidechain.clear();
            // Strong sidechain signal (above threshold)
            for (int i = 0; i < 512; ++i) {
                sidechain.setSample(0, i, 0.9f);
                sidechain.setSample(1, i, 0.9f);
            }

            juce::AudioBuffer<float> main(2, 512);
            main.clear();
            for (int i = 0; i < 512; ++i) {
                main.setSample(0, i, 0.5f);
                main.setSample(1, i, 0.5f);
            }

            comp.process(sidechain, main, -20.0f, 8.0f, 10.0f, 100.0f, 0.0f, 0.0f, false);

            expect(comp.getGainReduction() > 0.0f, "Should have gain reduction with loud sidechain");
            expect(main.getSample(0, 511) < 0.5f, "Main buffer should be attenuated");
        });

        testCase("Makeup gain", [&] {
            SidechainCompressor comp;
            comp.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> sidechain(2, 512);
            sidechain.clear();
            for (int i = 0; i < 512; ++i) {
                sidechain.setSample(0, i, 0.9f);
                sidechain.setSample(1, i, 0.9f);
            }

            juce::AudioBuffer<float> main(2, 512);
            main.clear();
            for (int i = 0; i < 512; ++i) {
                main.setSample(0, i, 0.5f);
                main.setSample(1, i, 0.5f);
            }

            comp.process(sidechain, main, -20.0f, 8.0f, 10.0f, 100.0f, 10.0f, 0.0f, false);

            // With makeup gain, the output should be louder than without
            expect(main.getSample(0, 511) > 0.0f, "Output should have some signal with makeup gain");
        });

        testCase("HPF filtering", [&] {
            SidechainCompressor comp;
            comp.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> sidechain(2, 512);
            sidechain.clear();
            // Low frequency signal (should be filtered by HPF)
            for (int i = 0; i < 512; ++i) {
                float val = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 50.0f * static_cast<float>(i) / 44100.0f);
                sidechain.setSample(0, i, val);
                sidechain.setSample(1, i, val);
            }

            juce::AudioBuffer<float> main(2, 512);
            main.clear();
            for (int i = 0; i < 512; ++i) {
                main.setSample(0, i, 0.5f);
                main.setSample(1, i, 0.5f);
            }

            // With HPF at 100Hz, the 50Hz signal should be filtered, reducing gain reduction
            comp.process(sidechain, main, -20.0f, 8.0f, 10.0f, 100.0f, 0.0f, 100.0f, false);

            // The gain reduction should be less than without HPF
            expect(comp.getGainReduction() < 10.0f, "HPF should reduce low-frequency sidechain signal");
        });

        testCase("Reset clears state", [&] {
            SidechainCompressor comp;
            comp.prepare(44100.0, 512, 2);

            juce::AudioBuffer<float> sidechain(2, 512);
            sidechain.clear();
            for (int i = 0; i < 512; ++i) {
                sidechain.setSample(0, i, 0.9f);
                sidechain.setSample(1, i, 0.9f);
            }

            juce::AudioBuffer<float> main(2, 512);
            main.clear();
            for (int i = 0; i < 512; ++i) {
                main.setSample(0, i, 0.5f);
                main.setSample(1, i, 0.5f);
            }

            comp.process(sidechain, main, -20.0f, 8.0f, 10.0f, 100.0f, 0.0f, 0.0f, false);
            expect(comp.getGainReduction() > 0.0f, "Should have gain reduction before reset");

            comp.reset();
            expect(comp.getGainReduction() == 0.0f, "Gain reduction should be 0 after reset");
        });
    }
};

static SidechainCompressorTest sidechainCompressorTest;
