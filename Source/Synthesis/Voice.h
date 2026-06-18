#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Oscillator.h"
#include "Filter.h"
#include "Envelope.h"
#include "LFO.h"

class Voice
{
public:
    // How Osc 2's processed signal is combined with Osc 1's processed signal.
    enum class MixMode { Sum, AM, FM, Ring };

    Voice();
    ~Voice();

    void prepare (double sampleRate, int samplesPerBlock, double tailLength);
    void process (juce::AudioBuffer<float>& buffer);

    void noteOn  (int midiNoteNumber, float velocity);
    void noteOff ();

    int   getNoteNumber    () const { return midiNoteNumber_; }
    bool  isActive         () const { return isActive_; }
    float getTimeSinceNoteOn () const { return timeSinceNoteOn_; }

    // ── Osc 1 ──────────────────────────────────────────────────────────────────
    void setWaveform           (Oscillator::Waveform w) { oscillator_.setWaveform (w); oscillator2_.setWaveform (w); }
    void setOscillatorTune     (float tune)    { oscTune_   = tune; }
    void setOscillatorDetune   (float detune)  { oscDetune_ = detune; }

    // ── Filter 1 ───────────────────────────────────────────────────────────────
    void setFilterCutoff       (float c) { filterCutoff_     = c; }
    void setFilterResonance    (float r) { filterResonance_  = r; }
    void setFilterMode         (Filter::Mode m) { filter_.setMode (m); }
    void setEnvelopeFilterMod  (float m) { envelopeFilterMod_ = m; }

    // ── Vol envelope 1 ─────────────────────────────────────────────────────────
    void setEnvelopeAttack         (float v) { envelope_.setAttack (v); }
    void setEnvelopeDecay          (float v) { envelope_.setDecay (v); }
    void setEnvelopeSustain        (float v) { envelope_.setSustain (v); }
    void setEnvelopeRelease        (float v) { envelope_.setRelease (v); }
    void setEnvelopeSustainEnabled (bool  b) { envelope_.setSustainEnabled (b); }

    // ── Filter envelope 1 ──────────────────────────────────────────────────────
    void setFEnvAttack         (float v) { fenv_.setAttack (v); }
    void setFEnvDecay          (float v) { fenv_.setDecay (v); }
    void setFEnvSustain        (float v) { fenv_.setSustain (v); }
    void setFEnvRelease        (float v) { fenv_.setRelease (v); }
    void setFEnvSustainEnabled (bool  b) { fenv_.setSustainEnabled (b); }

    // ── LFO ────────────────────────────────────────────────────────────────────
    void setLFOSpeed  (float s) { lfo_.setSpeed (s); }
    void setLFODepth  (float d) { lfoDepth_  = d; }
    void setLFOTarget (int   t) { lfoTarget_ = t; }

    // ── Master ─────────────────────────────────────────────────────────────────
    void setOutputGain (float g) { outputGain_ = g; }

    // ── Osc 2 chain ────────────────────────────────────────────────────────────
    void setOsc2Enabled    (bool  b) { osc2Enabled_ = b; }
    void setOsc2Waveform   (Oscillator::Waveform w) { oscB_.setWaveform (w); }
    void setOsc2Mult       (int   i) { osc2Mult_    = juce::jlimit (0, 3, i); }
    void setOsc2Phase      (float d) { osc2Phase_   = d; }        // degrees
    void setOsc2MixMode    (MixMode m) { osc2MixMode_ = m; }
    void setOsc2MixDepth   (float d) { osc2MixDepth_ = d; }       // 0-100

    // Vol envelope 2 (bypassable)
    void setOsc2EnvEnabled       (bool  b) { osc2EnvEnabled_ = b; }
    void setOsc2EnvAttack        (float v) { env2_.setAttack (v); }
    void setOsc2EnvDecay         (float v) { env2_.setDecay (v); }
    void setOsc2EnvSustain       (float v) { env2_.setSustain (v); }
    void setOsc2EnvRelease       (float v) { env2_.setRelease (v); }
    void setOsc2EnvSustainEnabled(bool  b) { env2_.setSustainEnabled (b); }

    // Filter 2 (bypassable)
    void setOsc2FilterEnabled    (bool  b) { osc2FilterEnabled_ = b; }
    void setOsc2FilterCutoff     (float c) { osc2FilterCutoff_  = c; }
    void setOsc2FilterResonance  (float r) { osc2FilterResonance_ = r; }
    void setOsc2FilterMode       (Filter::Mode m) { filter2_.setMode (m); }

    // Filter envelope 2 (for filter 2)
    void setOsc2FEnvAttack        (float v) { fenv2_.setAttack (v); }
    void setOsc2FEnvDecay         (float v) { fenv2_.setDecay (v); }
    void setOsc2FEnvSustain       (float v) { fenv2_.setSustain (v); }
    void setOsc2FEnvRelease       (float v) { fenv2_.setRelease (v); }
    void setOsc2FEnvSustainEnabled(bool  b) { fenv2_.setSustainEnabled (b); }
    void setOsc2FEnvDepth         (float d) { osc2FenvDepth_ = d; }

private:
    // ── Osc 1 chain ────────────────────────────────────────────────────────────
    Oscillator oscillator_, oscillator2_; // detuned unison pair
    Filter     filter_;
    Envelope   envelope_; // volume envelope
    Envelope   fenv_;     // filter envelope

    // ── Osc 2 chain ────────────────────────────────────────────────────────────
    Oscillator oscB_;
    Envelope   env2_;    // volume envelope for Osc 2
    Filter     filter2_;
    Envelope   fenv2_;   // filter envelope for Filter 2

    // ── LFO ────────────────────────────────────────────────────────────────────
    LFO lfo_;

    // ── Voice state ────────────────────────────────────────────────────────────
    bool  isActive_       = false;
    int   midiNoteNumber_ = -1;
    float velocity_       = 0.f;
    float timeSinceNoteOn_ = 0.f;

    // ── Osc 1 params ───────────────────────────────────────────────────────────
    float oscTune_           = 0.f;
    float oscDetune_         = 0.f;
    float filterCutoff_      = 4000.f;
    float filterResonance_   = 0.f;
    float envelopeFilterMod_ = 0.f;
    float lfoDepth_          = 0.f;
    int   lfoTarget_         = 0;
    float outputGain_        = 0.f;

    // ── Osc 2 params ───────────────────────────────────────────────────────────
    bool     osc2Enabled_       = false;
    int      osc2Mult_          = 1;    // 0=0.5x 1=1x 2=2x 3=4x
    float    osc2Phase_         = 0.f;  // degrees
    MixMode  osc2MixMode_       = MixMode::Sum;
    float    osc2MixDepth_      = 50.f;
    bool     osc2EnvEnabled_    = false;
    bool     osc2FilterEnabled_ = false;
    float    osc2FilterCutoff_  = 4000.f;
    float    osc2FilterResonance_ = 0.f;
    float    osc2FenvDepth_     = 0.f;

    // ── Buffers ────────────────────────────────────────────────────────────────
    double sampleRate_    = 44100.0;
    int    samplesPerBlock_ = 256;

    juce::AudioBuffer<float> voiceBuffer_;
    juce::AudioBuffer<float> osc2Buffer_; // mono: Osc 2 chain output
};
