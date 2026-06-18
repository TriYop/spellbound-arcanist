#include "Oscillator.h"

Oscillator::Oscillator() {}
Oscillator::~Oscillator() {}

void Oscillator::prepare (double sampleRate)
{
    sampleRate_ = sampleRate;
    phase_ = 0.f;
}

void Oscillator::process (juce::AudioBuffer<float>& buffer, int midiNote, float tune, float detune)
{
    float baseFreq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    baseFreq *= std::pow (2.f, tune   / 1200.f);
    baseFreq *= std::pow (2.f, detune / 1200.f);

    const float phaseIncrement = baseFreq / static_cast<float> (sampleRate_);
    const int numChannels = buffer.getNumChannels();

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        const float sample = generateSample (phase_);
        phase_ += phaseIncrement;
        if (phase_ >= 1.f) phase_ -= 1.f;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[n] += sample;
    }
}

void Oscillator::processWithFM (juce::AudioBuffer<float>& buffer, int midiNote,
                                  float tune, float detune,
                                  const juce::AudioBuffer<float>& fmMod,
                                  float fmDepthSemitones)
{
    float baseFreq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    baseFreq *= std::pow (2.f, tune   / 1200.f);
    baseFreq *= std::pow (2.f, detune / 1200.f);

    const float sr          = static_cast<float> (sampleRate_);
    const int   numChannels = buffer.getNumChannels();
    const int   numSamples  = buffer.getNumSamples();
    const float* mod        = fmMod.getReadPointer (0);

    for (int n = 0; n < numSamples; ++n)
    {
        const float modValue = (n < fmMod.getNumSamples()) ? mod[n] : 0.f;
        const float modFreq  = baseFreq * std::pow (2.f, modValue * fmDepthSemitones / 12.f);
        const float phaseInc = juce::jlimit (0.f, 0.5f, modFreq / sr);

        const float sample = generateSample (phase_);
        phase_ += phaseInc;
        if (phase_ >= 1.f) phase_ -= 1.f;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[n] += sample;
    }
}

float Oscillator::generateSample (float phase)
{
    switch (waveform_)
    {
        case Waveform::Sine:     return sine     (phase);
        case Waveform::Triangle: return triangle (phase);
        case Waveform::Sawtooth: return sawtooth (phase);
        case Waveform::Square:   return square   (phase);
        case Waveform::Noise:    return noise    ();
        default:                 return 0.f;
    }
}

float Oscillator::sine (float phase)    { return std::sin (TWO_PI * phase); }

float Oscillator::triangle (float phase)
{
    if (phase < 0.25f) return 4.f * phase;
    if (phase < 0.75f) return 2.f - 4.f * phase;
    return 4.f * phase - 4.f;
}

float Oscillator::sawtooth (float phase)
{
    return 2.f * (phase - std::floor (0.5f + phase));
}

float Oscillator::square (float phase) { return phase < 0.5f ? 1.f : -1.f; }

float Oscillator::noise()
{
    // Xorshift32 — cheap, well-distributed white noise
    noiseState_ ^= noiseState_ << 13;
    noiseState_ ^= noiseState_ >> 17;
    noiseState_ ^= noiseState_ << 5;
    return static_cast<float> (noiseState_) * (1.f / 2147483648.f);
}
