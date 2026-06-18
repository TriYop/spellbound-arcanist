#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class Oscillator
{
public:
    enum class Waveform { Sine, Triangle, Sawtooth, Square };

    Oscillator();
    ~Oscillator();

    void prepare (double sampleRate);
    void process (juce::AudioBuffer<float>& buffer, int midiNote, float tune, float detune);

    void setFrequency (float hz) { frequency_ = hz; }
    void setWaveform (Waveform w) { waveform_ = w; }

private:
    float generateSample (float phase);
    float sine (float phase);
    float triangle (float phase);
    float sawtooth (float phase);
    float square (float phase);

    double sampleRate_ = 44100.0;
    float frequency_ = 440.f;
    float phase_ = 0.f;
    Waveform waveform_ = Waveform::Sine;

    static constexpr float TWO_PI = 6.28318530718f;
};
