#pragma once
#include "DistrhoPluginInfo.h"

// One row per ArcanistParameters entry, in the same order. `kind` decides
// how ArcanistUI represents the parameter (RotaryKnob/ToggleSwitch/
// RadioButtonGroup, see Task 6) -- the DPF host side treats every
// parameter as a plain float regardless of kind, exactly like the JUCE-era
// APVTS did via getRawParameterValue().
enum class ArcanistParamKind { Continuous, Boolean, Choice };

struct ArcanistParamSpec
{
    const char* id;      // matches the JUCE-era APVTS parameter id AND the
                          // "id" attribute in Source/Presets/Factory/*.xml
    const char* name;
    ArcanistParamKind kind;
    float min, max, defaultValue;
    int numChoices;       // only meaningful when kind == Choice
};

inline const ArcanistParamSpec kArcanistParamSpecs[kArcanistParamCount] = {
    /* kParamOscWaveform    */ { "osc_waveform",       "Waveform",                  ArcanistParamKind::Choice,     0.f,     3.f,     0.f,   4 },
    /* kParamOscTune        */ { "osc_tune",           "Oscillator Tune",           ArcanistParamKind::Continuous, -24.f,   24.f,    0.f,   0 },
    /* kParamOscDetune      */ { "osc_detune",         "Oscillator Detune",        ArcanistParamKind::Continuous, -50.f,   50.f,    0.f,   0 },
    /* kParamFilterMode     */ { "filter_mode",        "Filter Mode",              ArcanistParamKind::Choice,     0.f,     2.f,     0.f,   3 },
    /* kParamFilterCutoff   */ { "filter_cutoff",      "Filter Cutoff",            ArcanistParamKind::Continuous, 20.f,    20000.f, 4000.f, 0 },
    /* kParamFilterResonance*/ { "filter_resonance",   "Filter Resonance",         ArcanistParamKind::Continuous, 0.f,     1.f,     0.f,   0 },
    /* kParamEnvAttack      */ { "env_attack",         "Attack Time",              ArcanistParamKind::Continuous, 0.001f,  5.f,     0.1f,  0 },
    /* kParamEnvDecay       */ { "env_decay",          "Decay Time",               ArcanistParamKind::Continuous, 0.f,     5.f,     0.5f,  0 },
    /* kParamEnvSustain     */ { "env_sustain",        "Sustain Level",            ArcanistParamKind::Continuous, 0.f,     1.f,     0.8f,  0 },
    /* kParamEnvRelease     */ { "env_release",        "Release Time",             ArcanistParamKind::Continuous, 0.5f,    10.f,    3.f,   0 },
    /* kParamEnvFilterMod   */ { "env_filter_mod",     "Filter Envelope Modulation", ArcanistParamKind::Continuous, -100.f, 100.f,   0.f,   0 },
    /* kParamEnvSustainOn   */ { "env_sustain_on",     "Amp Sustain On",           ArcanistParamKind::Boolean,    0.f,     1.f,     1.f,   0 },
    /* kParamFEnvAttack     */ { "fenv_attack",        "Filter Env Attack",        ArcanistParamKind::Continuous, 0.001f,  5.f,     0.001f, 0 },
    /* kParamFEnvDecay      */ { "fenv_decay",         "Filter Env Decay",         ArcanistParamKind::Continuous, 0.f,     5.f,     1.5f,  0 },
    /* kParamFEnvSustain    */ { "fenv_sustain",       "Filter Env Sustain",       ArcanistParamKind::Continuous, 0.f,     1.f,     0.f,   0 },
    /* kParamFEnvRelease    */ { "fenv_release",       "Filter Env Release",       ArcanistParamKind::Continuous, 0.001f,  10.f,    3.f,   0 },
    /* kParamFEnvSustainOn  */ { "fenv_sustain_on",    "Filter Sustain On",        ArcanistParamKind::Boolean,    0.f,     1.f,     1.f,   0 },
    /* kParamLfoTarget      */ { "lfo_target",         "LFO Target",               ArcanistParamKind::Choice,     0.f,     2.f,     0.f,   3 },
    /* kParamLfoSpeed       */ { "lfo_speed",          "LFO Speed",                ArcanistParamKind::Continuous, 0.1f,    10.f,    0.5f,  0 },
    /* kParamLfoDepth       */ { "lfo_depth",          "LFO Depth",                ArcanistParamKind::Continuous, 0.f,     100.f,   0.f,   0 },
    /* kParamOutputGain     */ { "output_gain",        "Output Gain",              ArcanistParamKind::Continuous, -24.f,   12.f,    0.f,   0 },
    /* kParamOsc2On         */ { "osc2_on",            "Osc 2 On",                 ArcanistParamKind::Boolean,    0.f,     1.f,     0.f,   0 },
    /* kParamOsc2Waveform   */ { "osc2_waveform",      "Osc 2 Waveform",           ArcanistParamKind::Choice,     0.f,     4.f,     2.f,   5 },
    /* kParamOsc2Mult       */ { "osc2_mult",          "Osc 2 Multiplier",         ArcanistParamKind::Choice,     0.f,     3.f,     1.f,   4 },
    /* kParamOsc2Phase      */ { "osc2_phase",         "Osc 2 Phase",              ArcanistParamKind::Continuous, 0.f,     360.f,   0.f,   0 },
    /* kParamOsc2MixMode    */ { "osc2_mix_mode",      "Osc 2 Mix Mode",           ArcanistParamKind::Choice,     0.f,     3.f,     0.f,   4 },
    /* kParamOsc2MixDepth   */ { "osc2_mix_depth",     "Osc 2 Mix Depth",          ArcanistParamKind::Continuous, 0.f,     100.f,   50.f,  0 },
    /* kParamOsc2EnvOn      */ { "osc2_env_on",        "Osc 2 Env On",             ArcanistParamKind::Boolean,    0.f,     1.f,     0.f,   0 },
    /* kParamOsc2EnvAttack  */ { "osc2_env_attack",    "Osc 2 Env Attack",         ArcanistParamKind::Continuous, 0.001f,  5.f,     0.1f,  0 },
    /* kParamOsc2EnvDecay   */ { "osc2_env_decay",     "Osc 2 Env Decay",          ArcanistParamKind::Continuous, 0.f,     5.f,     0.5f,  0 },
    /* kParamOsc2EnvSustain */ { "osc2_env_sustain",   "Osc 2 Env Sustain",        ArcanistParamKind::Continuous, 0.f,     1.f,     0.8f,  0 },
    /* kParamOsc2EnvRelease */ { "osc2_env_release",   "Osc 2 Env Release",        ArcanistParamKind::Continuous, 0.001f,  10.f,    3.f,   0 },
    /* kParamOsc2EnvSustainOn*/{ "osc2_env_sus_on",    "Osc 2 Env Sustain On",     ArcanistParamKind::Boolean,    0.f,     1.f,     1.f,   0 },
    /* kParamOsc2FltOn      */ { "osc2_flt_on",        "Osc 2 Filter On",          ArcanistParamKind::Boolean,    0.f,     1.f,     0.f,   0 },
    /* kParamOsc2FltCutoff  */ { "osc2_flt_cutoff",    "Osc 2 Filter Cutoff",      ArcanistParamKind::Continuous, 20.f,    20000.f, 4000.f, 0 },
    /* kParamOsc2FltResonance*/{ "osc2_flt_reso",      "Osc 2 Filter Resonance",   ArcanistParamKind::Continuous, 0.f,     1.f,     0.f,   0 },
    /* kParamOsc2FltMode    */ { "osc2_flt_mode",      "Osc 2 Filter Mode",        ArcanistParamKind::Choice,     0.f,     2.f,     0.f,   3 },
    /* kParamOsc2FEnvAttack */ { "osc2_fenv_atk",      "Osc 2 FEnv Attack",        ArcanistParamKind::Continuous, 0.001f,  5.f,     0.001f, 0 },
    /* kParamOsc2FEnvDecay  */ { "osc2_fenv_dec",      "Osc 2 FEnv Decay",         ArcanistParamKind::Continuous, 0.f,     5.f,     1.5f,  0 },
    /* kParamOsc2FEnvSustain*/ { "osc2_fenv_sus",      "Osc 2 FEnv Sustain",       ArcanistParamKind::Continuous, 0.f,     1.f,     0.f,   0 },
    /* kParamOsc2FEnvRelease*/ { "osc2_fenv_rel",      "Osc 2 FEnv Release",       ArcanistParamKind::Continuous, 0.001f,  10.f,    3.f,   0 },
    /* kParamOsc2FEnvSustainOn*/{ "osc2_fenv_sus_on",  "Osc 2 FEnv Sustain On",    ArcanistParamKind::Boolean,    0.f,     1.f,     1.f,   0 },
    /* kParamOsc2FEnvDepth  */ { "osc2_fenv_depth",    "Osc 2 FEnv Depth",         ArcanistParamKind::Continuous, -100.f,  100.f,   0.f,   0 },
};
