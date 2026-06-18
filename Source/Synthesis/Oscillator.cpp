#include "Oscillator.h"

Oscillator::Oscillator()
{
}

Oscillator::~Oscillator()
{
}

void Oscillator::prepare (double sampleRate)
{
    sampleRate_ = sampleRate;
    phase_ = 0.f;
}

void Oscillator::process (juce::AudioBuffer<float>& buffer, int midiNote, float tune, float detune)
{
    float baseFreq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    baseFreq *= std::pow (2.f, tune / 1200.f);
    baseFreq *= std::pow (2.f, detune / 1200.f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer (ch);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            samples[n] += generateSample (phase_);

            float phaseIncrement = baseFreq / static_cast<float> (sampleRate_);
            phase_ += phaseIncrement;

            if (phase_ >= 1.f)
                phase_ -= 1.f;
        }
    }
}

float Oscillator::generateSample (float phase)
{
    switch (waveform_)
    {
        case Waveform::Sine:
            return sine (phase);
        case Waveform::Triangle:
            return triangle (phase);
        case Waveform::Sawtooth:
            return sawtooth (phase);
        case Waveform::Square:
            return square (phase);
        default:
            return 0.f;
    }
}

float Oscillator::sine (float phase)
{
    return std::sin (TWO_PI * phase);
}

float Oscillator::triangle (float phase)
{
    if (phase < 0.25f)
        return 4.f * phase;
    else if (phase < 0.75f)
        return 2.f - 4.f * phase;
    else
        return 4.f * phase - 4.f;
}

float Oscillator::sawtooth (float phase)
{
    return 2.f * (phase - std::floor (0.5f + phase));
}

float Oscillator::square (float phase)
{
    return phase < 0.5f ? 1.f : -1.f;
}
