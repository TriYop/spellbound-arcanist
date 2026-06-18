# PM0 Preset System Design

**Date:** 2026-06-18  
**Status:** Approved for implementation

## Overview

PM0 will support a complete preset system allowing users to save, load, and organize synthesizer configurations. The system includes 24 factory presets organized by sonic character, unlimited user presets, and full integration with JUCE's program system for MIDI Program Change control and DAW compatibility.

## Requirements

- **Factory presets:** 24 built-in presets (embedded in plugin binary, read-only)
- **User presets:** Unlimited custom presets saved to disk
- **UI control:** Preset selector at top of editor with save/load/delete workflow
- **MIDI control:** Program Change messages switch presets
- **DAW integration:** Compatible with host program change UI and automation
- **Cross-platform:** Linux (`~/.config/PM0/presets/`), macOS (`~/Library/Application Support/PM0/presets/`), Windows (`%APPDATA%/PM0/presets/`)

## Factory Preset Catalog (24 presets)

### Ethereal & Atmospheric (6)
1. Celestial Drift
2. Heavenly Glow
3. Cloud Nine
4. Starlight
5. Nebula
6. Infinite Space

### Dark & Cinematic (5)
7. Midnight Cathedral
8. Ominous Swell
9. Cinematic Strings
10. Deep Mystery
11. Shadowed Dream

### Warm & Organic (5)
12. Rich Strings
13. Velvet Warmth
14. Choir Shimmer
15. Golden Hour
16. Organic Bloom

### Bright & Shimmering (4)
17. Crystal Bell
18. Sparkling Sunrise
19. Luminous Pad
20. Prismatic Glow

### Evolving & Movement (3)
21. Morphing Landscape
22. Breathing Pulse
23. Spiraling Cosmos

### Synth & Electronic (1)
24. Vintage Synth Pad

## Architecture

### PresetManager Class

A new utility class responsible for all preset lifecycle operations.

**Responsibilities:**
- Load/save presets from disk and embedded binary data
- Manage preset metadata: `name` (string), `is_factory` (bool), optional `category` (for factory presets only)
- Serialize/deserialize presets as XML (reusing APVTS state serialization)
- Track current preset index and handle switching
- Enumerate all available presets (factory first, then user alphabetically)

**Key Methods:**
- `loadPreset(index)` → loads preset by index, updates APVTS state; returns success/failure
- `savePreset(name)` → saves current APVTS state to user directory with given name; returns success/failure
- `deletePreset(index)` → removes user preset from disk (factory presets return error); returns success/failure
- `getCurrentPresetIndex()` → returns index of active preset
- `getCurrentPresetName()` → returns name of active preset
- `getPresetList()` → returns vector<PresetInfo> with all presets (factory first, then user alphabetically)
- `ensureUserPresetsDirectory()` → creates user preset directory if missing; creates platform-specific path

**Storage Format:**
Presets stored as XML files matching APVTS state format (same as plugin state file). Each preset is a `.xml` file in the user presets directory or embedded as binary data for factory presets.

### PluginProcessor Updates

Implement the program change methods to integrate with the preset system:

- `getNumPrograms()` → returns 24 (factory) + number of user presets
- `getCurrentProgram()` → returns index of active preset
- `setCurrentProgram(index)` → calls `PresetManager::loadPreset(index)`
- `getProgramName(index)` → returns name of preset at index
- `changeProgramName(index, newName)` → renames user preset (factory presets are read-only; DAW rename ignored for factory)

### PluginEditor Updates

**UI Layout:** Preset selector at top of editor, above parameter controls.

```
┌─────────────────────────────────────────────────────────┐
│ Preset: [Factory Pad 01: Celestial Drift ▼]             │
│         [Save As...] [Delete]                           │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ Parameter Controls (Oscillator, Filter, Envelope, LFO)  │
└─────────────────────────────────────────────────────────┘
```

**Components:**
- `ComboBox presetSelector_` — lists all presets; selecting loads preset
- `TextButton saveAsButton_` — opens "Save As" dialog
- `TextButton deleteButton_` — deletes current user preset (disabled for factory presets)

**Behavior:**
- Preset ComboBox displays the current preset name; when user adjusts parameters, the selection remains but the preset is considered "active" (not necessarily saved)
- "Save As" button opens a dialog with text input for preset name; saves current parameter state as a user preset
- "Delete" button is enabled only for user presets; clicking removes preset file and updates list; button is disabled for factory presets
- On startup, loads the last-used preset (stored in plugin state) or defaults to first factory preset if none previously saved
- Changing any parameter does not auto-save; explicit "Save As" is required to persist changes

### Factory Preset Embedding

Factory presets (24 XML files) are compiled into the plugin binary using JUCE's binary data generator (`bin2c` or CMake custom command). This ensures they're always available and can't be accidentally deleted.

**Implementation:**
1. Create 24 XML files in `Source/Presets/Factory/` with factory preset configurations
2. Add CMake rule to embed as binary data (using JUCE helper)
3. PresetManager loads factory presets from binary data at startup
4. User presets loaded from disk directory separately

## Data Flow

### Load Preset
```
User selects preset in ComboBox
  ↓
Editor calls: proc_.setCurrentProgram(index)
  ↓
PluginProcessor calls: PresetManager::loadPreset(index)
  ↓
PresetManager loads XML (factory or user disk)
  ↓
PresetManager deserializes into ValueTree
  ↓
apvts.replaceState(ValueTree)
  ↓
All APVTS parameter attachments update automatically
  ↓
Sliders/controls in UI reflect new values
```

### Save Preset
```
User clicks "Save As" button
  ↓
Dialog opens: user enters preset name
  ↓
Editor calls: PresetManager::savePreset(name)
  ↓
PresetManager serializes current apvts.state to XML
  ↓
XML written to user presets directory
  ↓
ComboBox list refreshed, new preset selected
```

### MIDI Program Change
```
DAW sends Program Change message to plugin
  ↓
JUCE routes to: PluginProcessor::setCurrentProgram(programIndex)
  ↓
PresetManager loads preset at programIndex
  ↓
(Same flow as Load Preset above)
```

## Error Handling

- **Missing user presets directory:** Created on first save
- **Corrupted user preset XML:** Logged as warning, preset skipped in list
- **Invalid program index:** Clamped to valid range [0, getNumPrograms()-1]
- **Delete on corrupted file:** Proceeds silently (file removal succeeds)

## Testing Considerations

- Load each factory preset and verify all parameters match expected values
- Save a custom preset, close/reopen plugin, verify it loads correctly
- MIDI Program Change (CC #0/32, Program Change events) switches presets
- DAW sees all presets in its program selector
- Delete user preset, verify it's removed from disk and UI list
- Rename user preset via DAW, verify change persists
- Factory presets cannot be deleted or renamed
- Factory presets always appear first in list

## Implementation Order

1. Create PresetManager class with load/save/delete logic
2. Embed factory preset XML files into binary
3. Update PluginProcessor program methods
4. Design and add UI components to PluginEditor
5. Wire UI callbacks to PresetManager
6. Test MIDI Program Change integration
7. Manual testing of save/load workflows

## Files to Create/Modify

**New files:**
- `Source/PresetManager.h` — PresetManager class definition
- `Source/PresetManager.cpp` — PresetManager implementation
- `Source/Presets/Factory/*.xml` — 24 factory preset XML files
- `CMakeLists.txt` — binary data embedding rule

**Modified files:**
- `Source/PluginProcessor.h` — add PresetManager member
- `Source/PluginProcessor.cpp` — implement program methods, initialize PresetManager
- `Source/PluginEditor.h` — add preset UI components
- `Source/PluginEditor.cpp` — implement preset UI layout and callbacks

## Notes

- Preset system decouples from APVTS state serialization; presets are snapshots of APVTS state at save time
- Factory presets are created by hand-tuning parameters to achieve target sonic character; this is a one-time manual step
- User preset directory is shared across all DAWs on the same system
- Preset names are user-editable; factory preset names are fixed
