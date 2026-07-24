#include "PresetManager.h"
#include "PluginProcessor.h"

namespace
{
    using PV = std::map<juce::String, float>;

    PV makePreset (float threshold, float ratio, float attack, float release, float makeup,
                   float scHpf, float link, float compOn,
                   float drive, float detune, float ext, float fine,
                   float crushHpf, float filter,
                   float layout, float mix, float out, float crushLink, float crushOn)
    {
        return {
            { NINE50AudioProcessor::kThreshold, threshold },
            { NINE50AudioProcessor::kRatio, ratio },
            { NINE50AudioProcessor::kAttack, attack },
            { NINE50AudioProcessor::kRelease, release },
            { NINE50AudioProcessor::kMakeup, makeup },
            { NINE50AudioProcessor::kSidechainHPF, scHpf },
            { NINE50AudioProcessor::kLink, link },
            { NINE50AudioProcessor::kCompOn, compOn },
            { NINE50AudioProcessor::kDrive, drive },
            { NINE50AudioProcessor::kDetune, detune },
            { NINE50AudioProcessor::kExt, ext },
            { NINE50AudioProcessor::kFine, fine },
            { NINE50AudioProcessor::kHpf, crushHpf },
            { NINE50AudioProcessor::kFilter, filter },
            { NINE50AudioProcessor::kLayout, layout },
            { NINE50AudioProcessor::kMix, mix },
            { NINE50AudioProcessor::kOut, out },
            { NINE50AudioProcessor::kCrushLink, crushLink },
            { NINE50AudioProcessor::kCrushOn, crushOn },
        };
    }
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    buildFactoryPresets();
    refreshUserPresets();
}

void PresetManager::buildFactoryPresets()
{
    factoryPresets.clear();

    auto add = [this] (const juce::String& name, PV values)
    {
        Preset p;
        p.name = name;
        p.isFactory = true;
        p.values = std::move (values);
        factoryPresets.push_back (std::move (p));
    };

    // Defaults matching parameter constructors
    add ("Init",
         makePreset (-15.0f, 8.0f, 10.0f, 100.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 99.0f, 3.0f, 100.0f, 0.0f, 0.0f, 1.0f));

    // Classic French-touch pump — deep duck, fast release, mild grit
    add ("Homework Pump",
         makePreset (-18.0f, 8.0f, 8.0f, 80.0f, 4.0f, 1.0f, 1.0f, 1.0f,
                     2.0f, -1.0f, 0.0f, 1.0f, 0.0f, 72.0f, 3.0f, 100.0f, 0.0f, 1.0f, 1.0f));

    // Soft sidechain for pads/buses
    add ("Soft Duck",
         makePreset (-12.0f, 3.0f, 20.0f, 220.0f, 1.5f, 2.0f, 1.0f, 1.0f,
                     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 99.0f, 3.0f, 85.0f, 0.0f, 0.0f, 1.0f));

    // Aggressive club bus duck + bitcrush body
    add ("Club Bus",
         makePreset (-22.0f, 10.0f, 5.0f, 60.0f, 6.0f, 1.0f, 1.0f, 1.0f,
                     4.0f, 0.0f, 0.0f, 0.0f, 8.0f, 55.0f, 3.0f, 100.0f, -1.0f, 1.0f, 1.0f));

    // Bitcrush character first, light duck
    add ("Sampler Dust",
         makePreset (-10.0f, 4.0f, 15.0f, 150.0f, 2.0f, 0.0f, 0.0f, 1.0f,
                     5.0f, -2.0f, 0.0f, 1.0f, 12.0f, 40.0f, 3.0f, 100.0f, -2.0f, 1.0f, 1.0f));

    // Filtered house — darker top, strong pump
    add ("Filter House",
         makePreset (-20.0f, 7.0f, 6.0f, 90.0f, 3.0f, 2.0f, 1.0f, 1.0f,
                     1.5f, -1.0f, 0.0f, 1.0f, 18.0f, 28.0f, 3.0f, 100.0f, 0.0f, 0.0f, 1.0f));

    // Mid/Side bitcrush width
    add ("MidSide Grit",
         makePreset (-16.0f, 6.0f, 12.0f, 120.0f, 2.0f, 1.0f, 1.0f, 1.0f,
                     3.0f, -3.0f, 1.0f, 1.0f, 10.0f, 50.0f, 6.0f, 90.0f, -1.0f, 1.0f, 1.0f));

    // Mono sum crunch — old sampler stacked mono
    add ("Mono Crush",
         makePreset (-14.0f, 5.0f, 10.0f, 100.0f, 3.0f, 0.0f, 0.0f, 1.0f,
                     6.0f, -5.0f, 1.0f, 0.0f, 15.0f, 35.0f, 0.0f, 100.0f, -3.0f, 1.0f, 1.0f));
}

juce::File PresetManager::getUserPresetsDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Audio")
                   .getChildFile ("Presets")
                   .getChildFile ("NINE50");
    dir.createDirectory();
    return dir;
}

void PresetManager::refreshUserPresets()
{
    const auto previousName = getCurrentName();
    userPresets.clear();

    for (const auto& file : getUserPresetsDirectory().findChildFiles (juce::File::findFiles, false, "*.nine50"))
    {
        Preset p;
        if (readPresetFile (file, p))
        {
            p.isFactory = false;
            p.file = file;
            userPresets.push_back (std::move (p));
        }
    }

    std::sort (userPresets.begin(), userPresets.end(),
               [] (const Preset& a, const Preset& b) { return a.name.compareIgnoreCase (b.name) < 0; });

    // Try to keep selection on the same named preset after refresh
    currentIndex = 0;
    for (int i = 0; i < getNumPresets(); ++i)
    {
        if (getPresetName (i) == previousName)
        {
            currentIndex = i;
            break;
        }
    }
}

int PresetManager::getNumPresets() const
{
    return static_cast<int> (factoryPresets.size() + userPresets.size());
}

int PresetManager::getNumFactoryPrograms() const
{
    return static_cast<int> (factoryPresets.size());
}

juce::String PresetManager::getPresetName (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return {};

    const auto factoryCount = static_cast<int> (factoryPresets.size());
    if (index < factoryCount)
        return factoryPresets[static_cast<size_t> (index)].name;

    return userPresets[static_cast<size_t> (index - factoryCount)].name;
}

juce::String PresetManager::getFactoryProgramName (int index) const
{
    if (index < 0 || index >= getNumFactoryPrograms())
        return {};
    return factoryPresets[static_cast<size_t> (index)].name;
}

bool PresetManager::isFactoryPreset (int index) const
{
    return index >= 0 && index < static_cast<int> (factoryPresets.size());
}

juce::String PresetManager::getCurrentName() const
{
    return getPresetName (currentIndex);
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (int i = 0; i < getNumPresets(); ++i)
        names.add (getPresetName (i));
    return names;
}

bool PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= getNumPresets())
        return false;

    const auto factoryCount = static_cast<int> (factoryPresets.size());
    const auto& preset = (index < factoryCount)
                             ? factoryPresets[static_cast<size_t> (index)]
                             : userPresets[static_cast<size_t> (index - factoryCount)];

    applyValues (preset.values);
    currentIndex = index;
    return true;
}

bool PresetManager::loadFactoryProgram (int index)
{
    if (index < 0 || index >= getNumFactoryPrograms())
        return false;

    applyValues (factoryPresets[static_cast<size_t> (index)].values);
    currentIndex = index;
    return true;
}

bool PresetManager::saveCurrentUserPreset()
{
    if (isFactoryPreset (currentIndex))
        return false;

    const auto factoryCount = static_cast<int> (factoryPresets.size());
    auto& preset = userPresets[static_cast<size_t> (currentIndex - factoryCount)];
    preset.values = captureCurrentValues();
    return writePresetFile (preset.file, preset.name, preset.values);
}

bool PresetManager::saveAsUserPreset (const juce::String& name)
{
    auto trimmed = name.trim();
    if (trimmed.isEmpty())
        return false;

    // Avoid colliding with factory names
    for (const auto& factory : factoryPresets)
        if (factory.name.equalsIgnoreCase (trimmed))
            trimmed << " User";

    auto file = getUserPresetsDirectory().getChildFile (trimmed + ".nine50");
    auto values = captureCurrentValues();

    if (! writePresetFile (file, trimmed, values))
        return false;

    refreshUserPresets();

    for (int i = 0; i < getNumPresets(); ++i)
    {
        if (getPresetName (i) == trimmed)
        {
            currentIndex = i;
            break;
        }
    }

    return true;
}

bool PresetManager::deleteCurrentUserPreset()
{
    if (isFactoryPreset (currentIndex))
        return false;

    const auto factoryCount = static_cast<int> (factoryPresets.size());
    auto file = userPresets[static_cast<size_t> (currentIndex - factoryCount)].file;

    if (! file.deleteFile())
        return false;

    refreshUserPresets();
    currentIndex = juce::jlimit (0, juce::jmax (0, getNumPresets() - 1), currentIndex);
    if (getNumPresets() > 0)
        loadPreset (currentIndex);

    return true;
}

void PresetManager::applyValues (const std::map<juce::String, float>& values)
{
    for (const auto& [id, value] : values)
    {
        if (auto* param = apvts.getParameter (id))
        {
            const float normalised = param->convertTo0to1 (value);
            param->beginChangeGesture();
            param->setValueNotifyingHost (normalised);
            param->endChangeGesture();
        }
    }
}

std::map<juce::String, float> PresetManager::captureCurrentValues() const
{
    std::map<juce::String, float> values;
    static const char* ids[] = {
        NINE50AudioProcessor::kThreshold, NINE50AudioProcessor::kRatio,
        NINE50AudioProcessor::kAttack, NINE50AudioProcessor::kRelease,
        NINE50AudioProcessor::kMakeup, NINE50AudioProcessor::kSidechainHPF,
        NINE50AudioProcessor::kLink, NINE50AudioProcessor::kDrive,
        NINE50AudioProcessor::kDetune, NINE50AudioProcessor::kExt,
        NINE50AudioProcessor::kFine, NINE50AudioProcessor::kHpf, NINE50AudioProcessor::kFilter,
        NINE50AudioProcessor::kLayout, NINE50AudioProcessor::kMix,
        NINE50AudioProcessor::kOut, NINE50AudioProcessor::kCrushLink,
        NINE50AudioProcessor::kCompOn, NINE50AudioProcessor::kCrushOn
    };

    for (auto* id : ids)
        if (auto* raw = apvts.getRawParameterValue (id))
            values[id] = raw->load();

    return values;
}

bool PresetManager::writePresetFile (const juce::File& file, const juce::String& name,
                                     const std::map<juce::String, float>& values) const
{
    auto xml = std::make_unique<juce::XmlElement> ("NINE50Preset");
    xml->setAttribute ("name", name);
    xml->setAttribute ("version", 1);

    auto* params = xml->createNewChildElement ("Parameters");
    for (const auto& [id, value] : values)
    {
        auto* p = params->createNewChildElement ("Param");
        p->setAttribute ("id", id);
        p->setAttribute ("value", value);
    }

    return xml->writeTo (file);
}

bool PresetManager::readPresetFile (const juce::File& file, Preset& out) const
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr || ! xml->hasTagName ("NINE50Preset"))
        return false;

    out.name = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    out.values.clear();

    if (auto* params = xml->getChildByName ("Parameters"))
    {
        for (auto* p : params->getChildWithTagNameIterator ("Param"))
        {
            const auto id = p->getStringAttribute ("id");
            if (id.isNotEmpty())
                out.values[id] = static_cast<float> (p->getDoubleAttribute ("value"));
        }
    }

    return ! out.values.empty();
}
