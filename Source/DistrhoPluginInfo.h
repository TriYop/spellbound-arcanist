/*
 * Spellbound Arcanist -- DPF plugin metadata.
 *
 * A polyphonic pad/drone synthesizer: no audio inputs, MIDI-driven, 16
 * voices. Identity values (brand, CLAP ID, 4-char codes) carried over
 * unchanged from the removed juce_add_plugin() call in the old
 * CMakeLists.txt so the plugin keeps the same public identity.
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND      "Spellbound"
#define DISTRHO_PLUGIN_NAME       "Arcanist"
#define DISTRHO_PLUGIN_URI        "https://spellbound.audio/plugins/arcanist"
#define DISTRHO_PLUGIN_CLAP_ID    "com.spellbound.arcanist"

#define DISTRHO_PLUGIN_BRAND_ID   Spbd
#define DISTRHO_PLUGIN_UNIQUE_ID  Arcn

#define DISTRHO_PLUGIN_LABEL       "Arcanist"
#define DISTRHO_PLUGIN_MAKER       "Spellbound"
// Must be a URI -- lv2lint's Plugin License test fails a plain word (see
// Hex's/Pugilist's/Tank's DistrhoPluginInfo.h for the same fix already made).
#define DISTRHO_PLUGIN_LICENSE     "https://spellbound.audio/plugins/arcanist#license"
#define DISTRHO_PLUGIN_DESCRIPTION "Soft pads and drones synthesizer"

#define DISTRHO_PLUGIN_HAS_UI           1
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_IS_SYNTH         1
#define DISTRHO_PLUGIN_NUM_INPUTS       0
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0
// ArcanistUI::applyPreset() (Task 6) calls ArcanistPluginAdapter::
// requestAllNotesOff() directly after a preset change, via
// getPluginInstancePointer() -- the JUCE-era PresetManager's
// setCurrentProgram() equivalent.
#define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 1

#define DISTRHO_UI_DEFAULT_WIDTH  1250
#define DISTRHO_UI_DEFAULT_HEIGHT 860
#define DISTRHO_UI_USER_RESIZABLE 0
#define DISTRHO_UI_USE_NANOVG     1
// ArcanistUI's "SAVE AS" button (Task 6) opens a native save dialog via
// UI::openFileBrowser()/uiFileBrowserSelected() -- DPF gates both behind
// this flag (default 0, see DistrhoPluginChecks.h).
#define DISTRHO_UI_FILE_BROWSER   1

// Host parameter indices, shared between ArcanistPluginAdapter and
// ArcanistUI. Order matches the JUCE-era createParameterLayout() exactly.
enum ArcanistParameters {
    kParamOscWaveform = 0, kParamOscTune, kParamOscDetune,
    kParamFilterMode, kParamFilterCutoff, kParamFilterResonance,
    kParamEnvAttack, kParamEnvDecay, kParamEnvSustain, kParamEnvRelease,
    kParamEnvFilterMod, kParamEnvSustainOn,
    kParamFEnvAttack, kParamFEnvDecay, kParamFEnvSustain, kParamFEnvRelease, kParamFEnvSustainOn,
    kParamLfoTarget, kParamLfoSpeed, kParamLfoDepth,
    kParamOutputGain,
    kParamOsc2On, kParamOsc2Waveform, kParamOsc2Mult, kParamOsc2Phase,
    kParamOsc2MixMode, kParamOsc2MixDepth,
    kParamOsc2EnvOn, kParamOsc2EnvAttack, kParamOsc2EnvDecay, kParamOsc2EnvSustain, kParamOsc2EnvRelease, kParamOsc2EnvSustainOn,
    kParamOsc2FltOn, kParamOsc2FltCutoff, kParamOsc2FltResonance, kParamOsc2FltMode,
    kParamOsc2FEnvAttack, kParamOsc2FEnvDecay, kParamOsc2FEnvSustain, kParamOsc2FEnvRelease, kParamOsc2FEnvSustainOn,
    kParamOsc2FEnvDepth,
    kArcanistParamCount // 43
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
