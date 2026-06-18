#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Oscillator.h"
#include "Filter.h"
#include "Envelope.h"
#include "LFO.h"

class Voice
{
public:
    Voice();
    ~Voice();

    void prepare (double sampleRate, int samplesPerBlock, double tailLength);
    void process (juce::AudioBuffer<float>& buffer);

    void noteOn (int midiNoteNumber, float velocity);
    void noteOff();

    int getNoteNumber() const { return midiNoteNumber_; }
    bool isActive() const { return isActive_; }
    float getTimeSinceNoteOn() const { return timeSinceNoteOn_; }

    void setWaveform (Oscillator::Waveform w) { oscillator_.setWaveform (w); oscillator2_.setWaveform (w); }
    void setOscillatorTune (float tune) { oscTune_ = tune; }
    void setOscillatorDetune (float detune) { oscDetune_ = detune; }
    void setFilterCutoff (float cutoff) { filterCutoff_ = cutoff; }
    void setFilterResonance (float resonance) { filterResonance_ = resonance; }
    void setEnvelopeAttack (float attack) { envelope_.setAttack (attack); }
    void setEnvelopeDecay (float decay) { envelope_.setDecay (decay); }
    void setEnvelopeSustain (float sustain) { envelope_.setSustain (sustain); }
    void setEnvelopeRelease (float release) { envelope_.setRelease (release); }
    void setEnvelopeSustainEnabled (bool on) { envelope_.setSustainEnabled (on); }
    void setEnvelopeFilterMod (float mod) { envelopeFilterMod_ = mod; }
    // Filter envelope (dedicated ADSR for filter cutoff modulation)
    void setFEnvAttack  (float attack)  { fenv_.setAttack (attack); }
    void setFEnvDecay   (float decay)   { fenv_.setDecay (decay); }
    void setFEnvSustain (float sustain) { fenv_.setSustain (sustain); }
    void setFEnvRelease (float release) { fenv_.setRelease (release); }
    void setFEnvSustainEnabled (bool on) { fenv_.setSustainEnabled (on); }
    void setLFOSpeed (float speed) { lfo_.setSpeed (speed); }
    void setLFODepth (float depth) { lfoDepth_ = depth; }
    void setLFOTarget (int target) { lfoTarget_ = target; }
    void setFilterMode (Filter::Mode m) { filter_.setMode (m); }
    void setOutputGain (float gain) { outputGain_ = gain; }

private:
    Oscillator oscillator_;
    Oscillator oscillator2_;
    Filter     filter_;
    Envelope   envelope_;
    Envelope   fenv_; // dedicated filter envelope
    LFO        lfo_;

    bool isActive_ = false;
    int midiNoteNumber_ = -1;
    float velocity_ = 0.f;
    float timeSinceNoteOn_ = 0.f;

    float oscTune_ = 0.f;
    float oscDetune_ = 0.f;
    float filterCutoff_ = 4000.f;
    float filterResonance_ = 0.f;
    float envelopeFilterMod_ = 0.f;
    float lfoDepth_ = 0.f;
    int   lfoTarget_ = 0; // 0=filter, 1=amplitude, 2=pitch
    float outputGain_ = 0.f;

    double sampleRate_ = 44100.0;
    int samplesPerBlock_ = 256;

    juce::AudioBuffer<float> voiceBuffer_;
};
