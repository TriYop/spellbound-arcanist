#pragma once
#include "AudioBuffer.h"
#include <cmath>
#include <cstdint>

class Oscillator
{
public:
    enum class Waveform { Sine, Triangle, Sawtooth, Square, Noise };

    Oscillator();
    ~Oscillator();

    void prepare (double sampleRate);

    // Adds generated signal to buffer.
    void process (dsp::AudioBuffer& buffer, int midiNote, float tune, float detune);

    // FM variant: per-sample frequency deviation = fmMod[ch0] × fmDepthSemitones.
    void processWithFM (dsp::AudioBuffer& buffer, int midiNote, float tune, float detune,
                        const dsp::AudioBuffer& fmMod, float fmDepthSemitones);

    void setFrequency (float hz)      { frequency_ = hz; }
    void setWaveform  (Waveform w)    { waveform_  = w;  }
    void resetPhase   (float phase01) { phase_ = phase01; }

private:
    float generateSample (float phase);
    float sine     (float phase);
    float triangle (float phase);
    float sawtooth (float phase);
    float square   (float phase);
    float noise    ();

    double   sampleRate_ = 44100.0;
    float    frequency_  = 440.f;
    float    phase_      = 0.f;
    Waveform waveform_   = Waveform::Sine;
    uint32_t noiseState_ = 2463534242u; // Xorshift seed

    static constexpr float TWO_PI = 6.28318530718f;
};
