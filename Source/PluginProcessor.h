#pragma once

#include <JuceHeader.h>
#include "SidechainCompressor.h"
#include "SP950Emulation.h"

class NINE50AudioProcessor : public juce::AudioProcessor {
public:
    NINE50AudioProcessor();
    ~NINE50AudioProcessor() override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLatencySeconds() const;
    double getTailLengthSeconds() const override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    const juce::String getInputChannelName(int channelIndex) const override;
    const juce::String getOutputChannelName(int channelIndex) const override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    SidechainCompressor compressor;
    SP950Emulation sp950;

    bool isSidechainEnabled() const;

    //==============================================================================
    // Parameter IDs
    static constexpr const char* kThreshold = "threshold";
    static constexpr const char* kRatio = "ratio";
    static constexpr const char* kAttack = "attack";
    static constexpr const char* kRelease = "release";
    static constexpr const char* kMakeup = "makeup";
    static constexpr const char* kSidechainHPF = "sidechain_hpf";
    static constexpr const char* kLink = "link";

    static constexpr const char* kDrive = "drive";
    static constexpr const char* kDetune = "detune";
    static constexpr const char* kExt = "ext";
    static constexpr const char* kFine = "fine";
    static constexpr const char* kFilter = "filter";
    static constexpr const char* kLayout = "layout";
    static constexpr const char* kMix = "mix";
    static constexpr const char* kOut = "out";
    static constexpr const char* kSP950Link = "sp950_link";

private:
    void initializeParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NINE50AudioProcessor)
};
