#include "PluginProcessor.h"
#include "PluginEditor.h"

NINE50AudioProcessor::NINE50AudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      parameters(*this, nullptr,
                  juce::Identifier("NINE50Parameters"),
                  {
                      std::make_unique<juce::AudioParameterFloat>(kThreshold, "Threshold", -60.0f, 0.0f, -15.0f),
                      std::make_unique<juce::AudioParameterFloat>(kRatio, "Ratio", 1.0f, 10.0f, 8.0f),
                      std::make_unique<juce::AudioParameterFloat>(kAttack, "Attack", 0.1f, 50.0f, 10.0f),
                      std::make_unique<juce::AudioParameterFloat>(kRelease, "Release", 10.0f, 500.0f, 100.0f),
                      std::make_unique<juce::AudioParameterFloat>(kMakeup, "Makeup", 0.0f, 30.0f, 0.0f),
                      std::make_unique<juce::AudioParameterChoice>(kSidechainHPF, "HPF", juce::StringArray{"Off", "100", "200", "300"}, 0),
                      std::make_unique<juce::AudioParameterBool>(kLink, "Link", false),
                      std::make_unique<juce::AudioParameterBool>(kCompOn, "Compressor On", true),

                      std::make_unique<juce::AudioParameterFloat>(kDrive, "Drive", -12.0f, 12.0f, 0.0f),
                      // 0 st = base 26.04 kHz ADC; more negative = pitched-up-source / detune workflow
                      // (EXT clamps DSP to -30, otherwise -15). Pitch/duration stay constant.
                      std::make_unique<juce::AudioParameterFloat>(kDetune, "Detune", -30.0f, 0.0f, 0.0f),
                      std::make_unique<juce::AudioParameterBool>(kExt, "Ext", false),
                      std::make_unique<juce::AudioParameterBool>(kFine, "Fine", false),
                      std::make_unique<juce::AudioParameterFloat>(kHpf, "HPF", 0.0f, 99.0f, 0.0f),
                      std::make_unique<juce::AudioParameterFloat>(kFilter, "LPF", 0.0f, 99.0f, 99.0f),
                      std::make_unique<juce::AudioParameterChoice>(kLayout, "Layout",
                          juce::StringArray{"Mono Sum", "Mono L", "Mono R", "Stereo",
                                            "Stereo L", "Stereo R", "Stereo Mid", "Stereo Side"}, 3),
                      std::make_unique<juce::AudioParameterFloat>(kMix, "Mix", 0.0f, 100.0f, 100.0f),
                      std::make_unique<juce::AudioParameterFloat>(kOut, "Out", -12.0f, 12.0f, 0.0f),
                      std::make_unique<juce::AudioParameterBool>(kCrushLink, "Drive/Out Link", false),
                      std::make_unique<juce::AudioParameterBool>(kCrushOn, "Bitcrush On", true),
                  }),
      presetManager (parameters) {
}

NINE50AudioProcessor::~NINE50AudioProcessor() {
}

//==============================================================================
const juce::String NINE50AudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool NINE50AudioProcessor::acceptsMidi() const {
    return false;
}

bool NINE50AudioProcessor::producesMidi() const {
    return false;
}

double NINE50AudioProcessor::getTailLatencySeconds() const {
    return 0.0;
}

double NINE50AudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

bool NINE50AudioProcessor::hasEditor() const {
    return true;
}

bool NINE50AudioProcessor::isSidechainEnabled() const {
    if (auto* sidechainBus = getBus(true, 1))
        return sidechainBus->isInput() && sidechainBus->isEnabled();
    return false;
}

juce::AudioProcessorEditor* NINE50AudioProcessor::createEditor() {
    return new NINE50AudioProcessorEditor(*this);
}

//==============================================================================
void NINE50AudioProcessor::prepareDSP (double sampleRate, int samplesPerBlock) {
    const int numChannels = juce::jmax (1, juce::jmin (getMainBusNumInputChannels(), 2));
    compressor.prepare (sampleRate, samplesPerBlock, numChannels);
    bitCrush.prepare (sampleRate, samplesPerBlock, numChannels);

    // Preallocate sidechain scratch so processBlock never allocates on the audio thread.
    sidechainScratch.setSize (2, juce::jmax (1, samplesPerBlock), false, false, true);
    sidechainScratch.clear();
}

void NINE50AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    prepareDSP (sampleRate, samplesPerBlock);
}

void NINE50AudioProcessor::releaseResources() {
    compressor.reset();
    bitCrush.reset();
}

void NINE50AudioProcessor::processorLayoutsChanged() {
    // Hosts (e.g. Bitwig) can enable the sidechain bus after prepareToPlay.
    // Re-prepare so scratch buffers / channel counts stay valid.
    if (getSampleRate() > 0.0 && getBlockSize() > 0)
        prepareDSP (getSampleRate(), getBlockSize());
}

void NINE50AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    if (buffer.getNumSamples() == 0)
        return;

    // Read parameters
    const float threshold = *parameters.getRawParameterValue(kThreshold);
    const float ratio = *parameters.getRawParameterValue(kRatio);
    const float attack = *parameters.getRawParameterValue(kAttack);
    const float release = *parameters.getRawParameterValue(kRelease);
    const float makeup = *parameters.getRawParameterValue(kMakeup);
    const int hpfChoice = static_cast<int>(*parameters.getRawParameterValue(kSidechainHPF));
    const bool link = static_cast<bool>(*parameters.getRawParameterValue(kLink));
    const bool compOn = static_cast<bool>(*parameters.getRawParameterValue(kCompOn));

    const float drive = *parameters.getRawParameterValue(kDrive);
    const float detune = *parameters.getRawParameterValue(kDetune);
    const bool ext = static_cast<bool>(*parameters.getRawParameterValue(kExt));
    const bool fine = static_cast<bool>(*parameters.getRawParameterValue(kFine));
    const float hpf = *parameters.getRawParameterValue(kHpf);
    const float filter = *parameters.getRawParameterValue(kFilter);
    const int layout = static_cast<int>(*parameters.getRawParameterValue(kLayout));
    const float mix = *parameters.getRawParameterValue(kMix) / 100.0f;
    const float out = *parameters.getRawParameterValue(kOut);
    const bool crushLink = static_cast<bool>(*parameters.getRawParameterValue(kCrushLink));
    const bool crushOn = static_cast<bool>(*parameters.getRawParameterValue(kCrushOn));

    // Map HPF choice to Hz
    float hpf_Hz = 0.0f;
    if (hpfChoice == 1) hpf_Hz = 100.0f;
    else if (hpfChoice == 2) hpf_Hz = 200.0f;
    else if (hpfChoice == 3) hpf_Hz = 300.0f;

    // Sidechain compressor (bypassable). Copy into preallocated scratch — never allocate here.
    if (compOn) {
        if (auto* sidechainBus = getBus (true, 1)) {
            if (sidechainBus->isEnabled()) {
                auto sidechainData = getBusBuffer (buffer, true, 1);
                const int scChannels = juce::jmin (sidechainData.getNumChannels(), sidechainScratch.getNumChannels());
                const int scSamples = juce::jmin (sidechainData.getNumSamples(), sidechainScratch.getNumSamples());

                if (scChannels > 0 && scSamples > 0) {
                    for (int ch = 0; ch < scChannels; ++ch)
                        sidechainScratch.copyFrom (ch, 0, sidechainData, ch, 0, scSamples);

                    juce::AudioBuffer<float> sidechainView (sidechainScratch.getArrayOfWritePointers(),
                                                           scChannels,
                                                           scSamples);
                    compressor.process (sidechainView, buffer, threshold, ratio, attack, release,
                                        makeup, hpf_Hz, link);
                }
            }
        }
    }

    // Bitcrush / downsample stage (SP-1200 / S950 character)
    if (crushOn) {
        bitCrush.process(buffer, drive, detune, ext, fine, hpf, filter, layout, mix, out, crushLink);
    }
}

//==============================================================================
const juce::String NINE50AudioProcessor::getInputChannelName(int channelIndex) const {
    return juce::String(channelIndex + 1);
}

const juce::String NINE50AudioProcessor::getOutputChannelName(int channelIndex) const {
    return juce::String(channelIndex + 1);
}

bool NINE50AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Main output must be mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }

    // Main input must match output
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet()) {
        return false;
    }

    // Sidechain bus must be either disabled or stereo
    if (layouts.getChannelSet(true, 1) != juce::AudioChannelSet::disabled()
        && layouts.getChannelSet(true, 1) != juce::AudioChannelSet::stereo()) {
        return false;
    }

    return true;
}

//==============================================================================
int NINE50AudioProcessor::getNumPrograms() {
    return juce::jmax (1, presetManager.getNumFactoryPrograms());
}

int NINE50AudioProcessor::getCurrentProgram() {
    const int factoryCount = presetManager.getNumFactoryPrograms();
    const int idx = presetManager.getCurrentIndex();
    return juce::jlimit (0, juce::jmax (0, factoryCount - 1), idx);
}

void NINE50AudioProcessor::setCurrentProgram(int index) {
    presetManager.loadFactoryProgram (index);
}

const juce::String NINE50AudioProcessor::getProgramName(int index) {
    return presetManager.getFactoryProgramName (index);
}

void NINE50AudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void NINE50AudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NINE50AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {
        juce::ValueTree state = juce::ValueTree::fromXml(*xmlState);
        if (state.isValid()) {
            parameters.replaceState(state);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* createPluginFilter() {
    return new NINE50AudioProcessor();
}
