#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>

struct PresetInfo
{
    juce::String name;
    juce::String category; // Only populated for factory presets
    bool isFactory = false;
    int index = -1; // Position in getPresetList(); -1 if not yet assigned
};

class PresetManager
{
public:
    // Constructor takes reference to APVTS for state serialization
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);
    ~PresetManager() = default;

    // Load preset by index (0-23 factory, 24+ user)
    bool loadPreset (int index);

    // Save current APVTS state as a user preset with given name
    bool savePreset (const juce::String& presetName);

    // Delete user preset by index (factory presets return false)
    bool deletePreset (int index);

    // Get current active preset index
    int getCurrentPresetIndex() const;

    // Get current active preset name
    juce::String getCurrentPresetName() const;

    // Get all presets (factory first, user alphabetically)
    std::vector<PresetInfo> getPresetList() const;

    // Reload user presets from disk (call after save/delete to update list)
    void refreshUserPresets();

private:
    juce::AudioProcessorValueTreeState& apvts_;
    int currentPresetIndex_ = 0; // Default to first factory preset

    std::vector<PresetInfo> factoryPresets_;
    std::vector<PresetInfo> userPresets_;

    // Platform-specific user presets directory
    juce::File getUserPresetsDirectory() const;

    // Ensure user presets directory exists
    void ensureUserPresetsDirectory() const;

    // Load factory presets from embedded binary data
    void loadFactoryPresets();

    // Load user presets from disk
    void loadUserPresetsFromDisk();

    // Serialize APVTS state to XML
    std::unique_ptr<juce::XmlElement> serializeAPVTS() const;

    // Deserialize XML to APVTS state
    bool deserializeAPVTS (const juce::XmlElement& xmlElement);

    // Internal load implementation
    bool loadPresetInternal (const PresetInfo& preset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
