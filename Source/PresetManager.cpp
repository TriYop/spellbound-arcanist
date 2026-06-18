#include "PresetManager.h"
#include <juce_core/juce_core.h>
#include <BinaryData.h>

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{
    ensureUserPresetsDirectory();
    loadFactoryPresets();
    loadUserPresetsFromDisk();
}

int PresetManager::getCurrentPresetIndex() const
{
    return currentPresetIndex_;
}

juce::String PresetManager::getCurrentPresetName() const
{
    auto allPresets = getPresetList();
    if (currentPresetIndex_ >= 0 && currentPresetIndex_ < static_cast<int> (allPresets.size()))
        return allPresets[static_cast<size_t> (currentPresetIndex_)].name;
    return "Unknown";
}

juce::File PresetManager::getUserPresetsDirectory() const
{
    // Platform-specific directory selection
#if JUCE_WINDOWS
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return appData.getChildFile ("PM0").getChildFile ("presets");
#elif JUCE_MAC
    auto appSupport = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                 .getChildFile ("Application Support");
    return appSupport.getChildFile ("PM0").getChildFile ("presets");
#else // JUCE_LINUX
    auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    return home.getChildFile (".config").getChildFile ("PM0").getChildFile ("presets");
#endif
}

void PresetManager::ensureUserPresetsDirectory() const
{
    auto dir = getUserPresetsDirectory();
    if (!dir.exists())
        dir.createDirectory();
}

std::unique_ptr<juce::XmlElement> PresetManager::serializeAPVTS() const
{
    auto state = apvts_.state;
    return state.createXml();
}

bool PresetManager::deserializeAPVTS (const juce::XmlElement& xmlElement)
{
    try
    {
        auto tree = juce::ValueTree::fromXml (xmlElement);
        if (tree.isValid() && tree.getType() == apvts_.state.getType())
        {
            apvts_.replaceState (tree);
            return true;
        }
    }
    catch (const std::exception&)
    {
        return false;
    }
    return false;
}

void PresetManager::loadFactoryPresets()
{
    // Factory preset definitions: name, category pairs
    const std::vector<std::pair<juce::String, juce::String>> factoryDefs = {
        // Ethereal & Atmospheric
        { "Celestial Drift",  "Ethereal" },
        { "Heavenly Glow",    "Ethereal" },
        { "Cloud Nine",       "Ethereal" },
        { "Starlight",        "Ethereal" },
        { "Nebula",           "Ethereal" },
        { "Infinite Space",   "Ethereal" },
        // Dark & Cinematic
        { "Midnight Cathedral", "Dark" },
        { "Ominous Swell",      "Dark" },
        { "Cinematic Strings",  "Dark" },
        { "Deep Mystery",       "Dark" },
        { "Shadowed Dream",     "Dark" },
        // Warm & Organic
        { "Rich Strings",   "Warm" },
        { "Velvet Warmth",  "Warm" },
        { "Choir Shimmer",  "Warm" },
        { "Golden Hour",    "Warm" },
        { "Organic Bloom",  "Warm" },
        // Bright & Shimmering
        { "Crystal Bell",      "Bright" },
        { "Sparkling Sunrise", "Bright" },
        { "Luminous Pad",      "Bright" },
        { "Prismatic Glow",    "Bright" },
        // Evolving & Movement
        { "Morphing Landscape", "Evolving" },
        { "Breathing Pulse",    "Evolving" },
        { "Spiraling Cosmos",   "Evolving" },
        // Synth & Electronic
        { "Vintage Synth Pad", "Synth" }
    };

    factoryPresets_.clear();
    for (size_t i = 0; i < factoryDefs.size(); ++i)
    {
        PresetInfo info;
        info.name      = factoryDefs[i].first;
        info.category  = factoryDefs[i].second;
        info.isFactory = true;
        info.index     = static_cast<int> (i);
        factoryPresets_.push_back (info);
    }
}

void PresetManager::loadUserPresetsFromDisk()
{
    userPresets_.clear();
    auto dir = getUserPresetsDirectory();

    if (!dir.exists())
        return;

    // Load all .xml files from user presets directory
    for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        PresetInfo info;
        info.name      = file.getFileNameWithoutExtension();
        info.category  = "";
        info.isFactory = false;
        info.index     = static_cast<int> (factoryPresets_.size() + userPresets_.size());
        userPresets_.push_back (info);
    }

    // Sort user presets alphabetically by name
    std::sort (userPresets_.begin(), userPresets_.end(),
               [] (const PresetInfo& a, const PresetInfo& b) { return a.name < b.name; });

    // Re-index after sort
    for (size_t i = 0; i < userPresets_.size(); ++i)
        userPresets_[i].index = static_cast<int> (factoryPresets_.size() + i);
}

std::vector<PresetInfo> PresetManager::getPresetList() const
{
    std::vector<PresetInfo> result = factoryPresets_;
    result.insert (result.end(), userPresets_.begin(), userPresets_.end());
    return result;
}

void PresetManager::refreshUserPresets()
{
    loadUserPresetsFromDisk();
}

bool PresetManager::loadPreset (int index)
{
    auto allPresets = getPresetList();

    if (index < 0 || index >= static_cast<int> (allPresets.size()))
        return false;

    const auto& preset = allPresets[static_cast<size_t> (index)];
    return loadPresetInternal (preset);
}

bool PresetManager::loadPresetInternal (const PresetInfo& preset)
{
    std::unique_ptr<juce::XmlElement> xmlElement;

    if (preset.isFactory)
    {
        // Build the resource name from the preset index and name.
        // Format: _NNN_PresetNameNoSpaces_xml
        // e.g. index 0, "Celestial Drift" → "_001_CelestialDrift_xml"
        juce::String paddedIndex = juce::String (preset.index + 1).paddedLeft ('0', 3);
        juce::String nameNoSpaces = preset.name.removeCharacters (" ");
        juce::String resourceName = "_" + paddedIndex + "_" + nameNoSpaces + "_xml";

        int dataSize = 0;
        const char* data = PM0_BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

        if (data == nullptr || dataSize <= 0)
        {
            DBG ("PresetManager: binary resource not found: " + resourceName);
            return false;
        }

        xmlElement = juce::parseXML (juce::String::fromUTF8 (data, dataSize));
        if (!xmlElement)
        {
            DBG ("PresetManager: failed to parse factory preset XML: " + resourceName);
            return false;
        }
    }
    else
    {
        // Load user preset from disk
        auto dir        = getUserPresetsDirectory();
        auto presetFile = dir.getChildFile (preset.name + ".xml");

        if (!presetFile.exists())
            return false;

        xmlElement = juce::parseXML (presetFile);
        if (!xmlElement)
        {
            DBG("PresetManager: failed to parse preset XML: " + presetFile.getFullPathName());
            return false;
        }
    }

    // Deserialize and apply to APVTS
    bool success = deserializeAPVTS (*xmlElement);
    if (success)
        currentPresetIndex_ = preset.index;

    return success;
}

bool PresetManager::savePreset (const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    ensureUserPresetsDirectory();

    // Serialize current APVTS state
    auto xmlElement = serializeAPVTS();
    if (!xmlElement)
        return false;

    // Write to disk
    auto dir        = getUserPresetsDirectory();
    auto presetFile = dir.getChildFile (presetName + ".xml");

    bool success = xmlElement->writeTo (presetFile);

    if (success)
    {
        // Refresh list and set current to newly saved preset
        refreshUserPresets();
        auto allPresets = getPresetList();
        for (const auto& p : allPresets)
        {
            if (!p.isFactory && p.name == presetName)
            {
                currentPresetIndex_ = p.index;
                break;
            }
        }
    }

    return success;
}

bool PresetManager::deletePreset (int index)
{
    auto allPresets = getPresetList();

    if (index < 0 || index >= static_cast<int> (allPresets.size()))
        return false;

    const auto& preset = allPresets[static_cast<size_t> (index)];

    // Factory presets cannot be deleted
    if (preset.isFactory)
        return false;

    // Delete user preset file
    auto dir        = getUserPresetsDirectory();
    auto presetFile = dir.getChildFile (preset.name + ".xml");

    if (presetFile.exists() && !presetFile.deleteFile())
        return false;

    // Refresh list
    refreshUserPresets();

    // If deleted preset was current, reset to first factory preset
    if (currentPresetIndex_ == index)
        currentPresetIndex_ = 0;

    // Clamp currentPresetIndex_ to valid range after refresh (indices may have shifted)
    auto updatedPresets = getPresetList();
    if (currentPresetIndex_ >= static_cast<int> (updatedPresets.size()))
        currentPresetIndex_ = 0;

    return true;
}

bool PresetManager::renamePreset (int index, const juce::String& newName)
{
    if (newName.isEmpty())
        return false;

    auto allPresets = getPresetList();

    if (index < 0 || index >= static_cast<int> (allPresets.size()))
        return false;

    const auto& preset = allPresets[static_cast<size_t> (index)];

    // Factory presets cannot be renamed
    if (preset.isFactory)
        return false;

    auto dir        = getUserPresetsDirectory();
    auto oldFile    = dir.getChildFile (preset.name + ".xml");
    auto newFile    = dir.getChildFile (newName + ".xml");

    if (!oldFile.exists())
        return false;

    // Avoid clobbering an existing preset with the target name
    if (newFile.exists())
        return false;

    bool success = oldFile.moveFileTo (newFile);

    if (success)
    {
        bool wasCurrentPreset = (currentPresetIndex_ == index);
        refreshUserPresets();

        if (wasCurrentPreset)
        {
            // Find the renamed preset in the updated list and point current at it
            auto updatedPresets = getPresetList();
            for (const auto& p : updatedPresets)
            {
                if (!p.isFactory && p.name == newName)
                {
                    currentPresetIndex_ = p.index;
                    break;
                }
            }
        }
    }

    return success;
}
