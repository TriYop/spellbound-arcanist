#pragma once
#include <cmath>

namespace dsp
{

// Replaces juce::MidiMessage::getMidiNoteInHertz(note), A4 = 440 Hz at note 69.
inline float midiNoteToHz (int midiNote)
{
    return 440.0f * std::pow (2.0f, (static_cast<float> (midiNote) - 69.0f) / 12.0f);
}

// Replaces juce::Decibels::decibelsToGain(db).
inline float dbToGain (float db)
{
    return std::pow (10.0f, db / 20.0f);
}

// Replaces juce::Decibels::gainToDecibels(gain, minusInfinityDb).
inline float gainToDb (float gain, float minDb)
{
    return gain > 0.0f ? 20.0f * std::log10 (gain) : minDb;
}

} // namespace dsp
