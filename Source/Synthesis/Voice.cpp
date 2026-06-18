#include "Voice.h"

Voice::Voice()
{
}

Voice::~Voice()
{
}

void Voice::prepare (double sampleRate, int samplesPerBlock, double tailLength)
{
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;

    oscillator_.prepare (sampleRate);
    filter_.prepare (sampleRate);
    envelope_.prepare (sampleRate, tailLength);
    lfo_.prepare (sampleRate);

    voiceBuffer_.setSize (2, samplesPerBlock);
    voiceBuffer_.clear();
}

void Voice::process (juce::AudioBuffer<float>& buffer)
{
    if (!isActive_)
        return;

    voiceBuffer_.clear();

    // Generate oscillator signal
    oscillator_.process (voiceBuffer_, midiNoteNumber_, oscTune_, oscDetune_);

    // Update filter cutoff with envelope modulation and LFO
    float envValue = envelope_.getCurrentLevel();
    float lfoValue = lfo_.process();

    float filterCutoffModulated = filterCutoff_;

    // Apply envelope modulation (convert cents to ratio)
    if (std::abs (envelopeFilterMod_) > 0.1f)
        filterCutoffModulated *= std::pow (2.f, envelopeFilterMod_ * envValue / 1200.f);

    // Apply LFO modulation
    if (lfoDepth_ > 0.1f)
        filterCutoffModulated *= (1.f + lfoValue * lfoDepth_ * 0.1f);

    filterCutoffModulated = juce::jlimit (20.f, 20000.f, filterCutoffModulated);

    filter_.setCutoff (filterCutoffModulated);
    filter_.setResonance (filterResonance_);
    filter_.process (voiceBuffer_);

    // Apply envelope
    envelope_.process (voiceBuffer_);

    // Apply velocity and output gain
    float gainLinear = velocity_ * juce::Decibels::decibelsToGain (outputGain_);
    voiceBuffer_.applyGain (gainLinear);

    // Mix to output buffer
    for (int ch = 0; ch < buffer.getNumChannels() && ch < voiceBuffer_.getNumChannels(); ++ch)
        buffer.addFrom (ch, 0, voiceBuffer_, ch, 0, buffer.getNumSamples());

    // Update time tracking
    timeSinceNoteOn_ += buffer.getNumSamples() / static_cast<float> (sampleRate_);

    // Check if voice should stop
    if (!envelope_.isActive())
        isActive_ = false;
}

void Voice::noteOn (int midiNoteNumber, float velocity)
{
    midiNoteNumber_ = midiNoteNumber;
    velocity_ = velocity;
    isActive_ = true;

    oscillator_.setFrequency (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    envelope_.noteOn();
}

void Voice::noteOff()
{
    envelope_.noteOff();
}
