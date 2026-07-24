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
                      std::make_unique<juce::AudioParameterFloat>(kDetune, "Detune", -15.0f, 15.0f, 0.0f),
                      std::make_unique<juce::AudioParameterBool>(kExt, "Ext", false),
                      std::make_unique<juce::AudioParameterBool>(kFine, "Fine", false),
                      std::make_unique<juce::AudioParameterFloat>(kFilter, "Filter", 0.0f, 99.0f, 99.0f),
                      std::make_unique<juce::AudioParameterChoice>(kLayout, "Layout",
                          juce::StringArray{"Mono Sum", "Mono L", "Mono R", "Stereo", "Stereo L", "Stereo R", "Mid/Side"}, 3),
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
void NINE50AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    const int numChannels = juce::jmin(getMainBusNumInputChannels(), 2);
    compressor.prepare(sampleRate, samplesPerBlock, numChannels);
    bitCrush.prepare(sampleRate, samplesPerBlock, numChannels);
}

void NINE50AudioProcessor::releaseResources() {
    compressor.reset();
    bitCrush.reset();
}

void NINE50AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    if (buffer.getNumSamples() == 0)
        return;

    // Get sidechain buffer if available
    juce::AudioBuffer<float> sidechainBuffer;
    if (auto* sidechainBus = getBus(true, 1)) {
        if (sidechainBus->isInput() && sidechainBus->isEnabled()) {
            auto sidechainData = getBusBuffer(buffer, true, 1);
            sidechainBuffer.makeCopyOf(sidechainData);
        }
    }

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

    // Sidechain compressor (bypassable)
    if (compOn && sidechainBuffer.getNumSamples() > 0) {
        compressor.process(sidechainBuffer, buffer, threshold, ratio, attack, release, makeup, hpf_Hz, link);
    }

    // Bitcrush / downsample stage (SP-1200 / S950 character)
    if (crushOn) {
        bitCrush.process(buffer, drive, detune, ext, fine, filter, layout, mix, out, crushLink);
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
