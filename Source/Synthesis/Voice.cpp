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
    oscillator2_.prepare (sampleRate);
    filter_.prepare (sampleRate);
    envelope_.prepare (sampleRate, tailLength);
    lfo_.prepare (sampleRate, samplesPerBlock);

    voiceBuffer_.setSize (2, samplesPerBlock);
    voiceBuffer_.clear();
}

void Voice::process (juce::AudioBuffer<float>& buffer)
{
    if (!isActive_)
        return;

    voiceBuffer_.clear();

    float lfoValue = lfo_.process(); // -1 to +1

    // Pitch LFO: ±50 cents at full depth
    float pitchMod = (lfoTarget_ == 2 && lfoDepth_ > 0.1f)
                   ? lfoValue * lfoDepth_ * 0.5f
                   : 0.f;

    // Two oscillators detuned in opposite directions; normalize combined output
    oscillator_.process  (voiceBuffer_, midiNoteNumber_, oscTune_ + pitchMod,  oscDetune_);
    oscillator2_.process (voiceBuffer_, midiNoteNumber_, oscTune_ + pitchMod, -oscDetune_);
    voiceBuffer_.applyGain (0.5f);

    // Filter modulation (envelope + LFO when target is filter)
    float envValue = envelope_.getCurrentLevel();
    float filterCutoffModulated = filterCutoff_;

    if (std::abs (envelopeFilterMod_) > 0.1f)
        filterCutoffModulated *= std::pow (2.f, envelopeFilterMod_ * envValue / 1200.f);

    if (lfoTarget_ == 0 && lfoDepth_ > 0.1f)
        filterCutoffModulated *= (1.f + lfoValue * lfoDepth_ * 0.1f);

    filter_.setCutoff (juce::jlimit (20.f, 20000.f, filterCutoffModulated));
    filter_.setResonance (filterResonance_);
    filter_.process (voiceBuffer_);

    // Envelope
    envelope_.process (voiceBuffer_);

    // Output gain + amplitude LFO (tremolo)
    float gainLinear = velocity_ * juce::Decibels::decibelsToGain (outputGain_);
    if (lfoTarget_ == 1 && lfoDepth_ > 0.1f)
        gainLinear *= (1.f + lfoValue * lfoDepth_ * 0.005f);
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
