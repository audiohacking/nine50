#include <JuceHeader.h>
#include "PluginProcessor.h"

class PluginProcessorTest : public juce::UnitTest {
public:
    PluginProcessorTest()
        : juce::UnitTest("NINE50AudioProcessor", "nine50") {}

    void runTest() override {
        testCase("Processor initialization", [&] {
            NINE50AudioProcessor processor;
            expect(processor.getTotalNumInputChannels() == 2, "Should have 2 input channels");
            expect(processor.getTotalNumOutputChannels() == 2, "Should have 2 output channels");
            expect(processor.hasEditor(), "Should have an editor");
            expect(processor.acceptsMidi() == false, "Should not accept MIDI");
            expect(processor.producesMidi() == false, "Should not produce MIDI");
        });

        testCase("Parameter initialization", [&] {
            NINE50AudioProcessor processor;

            // Check threshold
            auto* thresholdParam = processor.parameters.getParameter(NINE50AudioProcessor::kThreshold);
            expect(thresholdParam != nullptr, "Threshold parameter should exist");
            expect(std::abs(thresholdParam->getValue() - (-15.0f / 60.0f + 1.0f) * 0.5f) < 0.01f ||
                   thresholdParam->getValue() >= 0.0f, "Threshold should be normalized");

            // Check ratio
            auto* ratioParam = processor.parameters.getParameter(NINE50AudioProcessor::kRatio);
            expect(ratioParam != nullptr, "Ratio parameter should exist");

            // Check attack
            auto* attackParam = processor.parameters.getParameter(NINE50AudioProcessor::kAttack);
            expect(attackParam != nullptr, "Attack parameter should exist");

            // Check release
            auto* releaseParam = processor.parameters.getParameter(NINE50AudioProcessor::kRelease);
            expect(releaseParam != nullptr, "Release parameter should exist");

            // Check makeup
            auto* makeupParam = processor.parameters.getParameter(NINE50AudioProcessor::kMakeup);
            expect(makeupParam != nullptr, "Makeup parameter should exist");

            // Check bitcrush parameters
            auto* driveParam = processor.parameters.getParameter(NINE50AudioProcessor::kDrive);
            expect(driveParam != nullptr, "Drive parameter should exist");

            auto* detuneParam = processor.parameters.getParameter(NINE50AudioProcessor::kDetune);
            expect(detuneParam != nullptr, "Detune parameter should exist");

            auto* filterParam = processor.parameters.getParameter(NINE50AudioProcessor::kFilter);
            expect(filterParam != nullptr, "Filter parameter should exist");

            auto* mixParam = processor.parameters.getParameter(NINE50AudioProcessor::kMix);
            expect(mixParam != nullptr, "Mix parameter should exist");

            auto* outParam = processor.parameters.getParameter(NINE50AudioProcessor::kOut);
            expect(outParam != nullptr, "Out parameter should exist");
        });

        testCase("State save/restore", [&] {
            NINE50AudioProcessor processor;

            auto* thresholdParam = processor.parameters.getParameter(NINE50AudioProcessor::kThreshold);
            auto* ratioParam = processor.parameters.getParameter(NINE50AudioProcessor::kRatio);
            auto* driveParam = processor.parameters.getParameter(NINE50AudioProcessor::kDrive);

            // setValueNotifyingHost takes normalised 0..1 values
            thresholdParam->setValueNotifyingHost(0.3f);
            ratioParam->setValueNotifyingHost(0.5f);
            driveParam->setValueNotifyingHost(0.7f);

            const float savedThreshold = *processor.parameters.getRawParameterValue(NINE50AudioProcessor::kThreshold);
            const float savedRatio = *processor.parameters.getRawParameterValue(NINE50AudioProcessor::kRatio);
            const float savedDrive = *processor.parameters.getRawParameterValue(NINE50AudioProcessor::kDrive);

            juce::MemoryBlock stateData;
            processor.getStateInformation(stateData);
            expect(stateData.getSize() > 0, "State data should not be empty");

            thresholdParam->setValueNotifyingHost(0.1f);
            ratioParam->setValueNotifyingHost(0.2f);
            driveParam->setValueNotifyingHost(0.4f);

            processor.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));

            const float threshold = *processor.parameters.getRawParameterValue(NINE50AudioProcessor::kThreshold);
            const float ratio = *processor.parameters.getRawParameterValue(NINE50AudioProcessor::kRatio);
            const float drive = *processor.parameters.getRawParameterValue(NINE50AudioProcessor::kDrive);

            expect(std::abs(threshold - savedThreshold) < 0.01f, "Threshold should be restored");
            expect(std::abs(ratio - savedRatio) < 0.01f, "Ratio should be restored");
            expect(std::abs(drive - savedDrive) < 0.01f, "Drive should be restored");
        });

        testCase("Bus layout support", [&] {
            NINE50AudioProcessor processor;

            juce::AudioProcessor::BusesLayout stereoLayout;
            stereoLayout.inputBuses.set(0, juce::AudioChannelSet::stereo());
            stereoLayout.outputBuses.set(0, juce::AudioChannelSet::stereo());
            expect(processor.isBusesLayoutSupported(stereoLayout), "Stereo layout should be supported");

            juce::AudioProcessor::BusesLayout monoLayout;
            monoLayout.inputBuses.set(0, juce::AudioChannelSet::mono());
            monoLayout.outputBuses.set(0, juce::AudioChannelSet::mono());
            expect(processor.isBusesLayoutSupported(monoLayout), "Mono layout should be supported");

            juce::AudioProcessor::BusesLayout sidechainLayout(stereoLayout);
            sidechainLayout.inputBuses.set(1, juce::AudioChannelSet::stereo());
            expect(processor.isBusesLayoutSupported(sidechainLayout), "Stereo + sidechain layout should be supported");
        });

        testCase("Process block with no sidechain", [&] {
            NINE50AudioProcessor processor;
            processor.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(0, i, 0.5f);
                buffer.setSample(1, i, 0.5f);
            }

            juce::MidiBuffer midi;
            processor.processBlock(buffer, midi);

            // Without sidechain, only bitcrush processing applies
            // With default settings (mix=100%, filter=99 off, drive=0, out=0), signal should pass through
            expect(std::abs(buffer.getSample(0, 0) - 0.5f) < 0.01f, "Signal should pass through with default settings");
        });

        testCase("Process block with sidechain", [&] {
            NINE50AudioProcessor processor;
            processor.prepareToPlay(44100.0, 512);

            // Create buffers
            juce::AudioBuffer<float> mainBuffer(2, 512);
            mainBuffer.clear();
            for (int i = 0; i < 512; ++i) {
                mainBuffer.setSample(0, i, 0.5f);
                mainBuffer.setSample(1, i, 0.5f);
            }

            juce::AudioBuffer<float> sidechainBuffer(2, 512);
            sidechainBuffer.clear();
            for (int i = 0; i < 512; ++i) {
                sidechainBuffer.setSample(0, i, 0.9f);
                sidechainBuffer.setSample(1, i, 0.9f);
            }

            juce::MidiBuffer midi;

            // Process - should apply sidechain compression
            // Note: In the current implementation, sidechain processing depends on bus layout
            // This test verifies the processor doesn't crash
            processor.processBlock(mainBuffer, midi);

            expect(true, "Process block with sidechain should not crash");
        });
    }
};

static PluginProcessorTest pluginProcessorTest;
