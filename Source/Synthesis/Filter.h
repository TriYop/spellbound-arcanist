#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class Filter
{
public:
    enum class Mode { LowPass, BandPass, HighPass };

    Filter();
    ~Filter();

    void prepare (double sampleRate);
    void process (juce::AudioBuffer<float>& buffer);

    void setCutoff (float cutoff);
    void setResonance (float resonance);
    void setMode (Mode m) { mode_ = m; updateCoefficients(); }

private:
    void updateCoefficients();

    Mode   mode_ = Mode::LowPass;
    double sampleRate_ = 44100.0;
    float cutoff_ = 4000.f;
    float resonance_ = 0.f;
    float targetCutoff_ = 4000.f;

    // State variables for each channel
    std::vector<float> z1_, z2_;

    float a1_ = 0.f, a2_ = 0.f, b0_ = 0.f, b1_ = 0.f, b2_ = 0.f;

    // Smoothing for cutoff changes
    float smoothingFactor_ = 0.1f;
};
