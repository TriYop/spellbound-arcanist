#include "Voice.h"

// Multiplier index → offset in cents: 0.5x=-1200, 1x=0, 2x=+1200, 4x=+2400
static constexpr float kMultCents[4] = { -1200.f, 0.f, 1200.f, 2400.f };

Voice::Voice() {}
Voice::~Voice() {}

void Voice::prepare (double sampleRate, int samplesPerBlock, double tailLength)
{
    sampleRate_     = sampleRate;
    samplesPerBlock_ = samplesPerBlock;

    oscillator_.prepare  (sampleRate);
    oscillator2_.prepare (sampleRate);
    filter_.prepare      (sampleRate);
    envelope_.prepare    (sampleRate, tailLength);
    fenv_.prepare        (sampleRate, tailLength);
    lfo_.prepare         (sampleRate, samplesPerBlock);

    oscB_.prepare    (sampleRate);
    env2_.prepare    (sampleRate, tailLength);
    filter2_.prepare (sampleRate);
    fenv2_.prepare   (sampleRate, tailLength);

    voiceBuffer_.setSize (2, samplesPerBlock);
    osc2Buffer_.setSize  (1, samplesPerBlock);
    voiceBuffer_.clear();
    osc2Buffer_.clear();
}

void Voice::noteOn (int midiNoteNumber, float velocity)
{
    midiNoteNumber_ = midiNoteNumber;
    velocity_       = velocity;
    isActive_       = true;
    timeSinceNoteOn_ = 0.f;

    oscillator_.setFrequency  (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    oscillator2_.setFrequency (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    envelope_.noteOn();
    fenv_.noteOn();

    oscB_.resetPhase (osc2Phase_ / 360.f);
    env2_.noteOn();
    fenv2_.noteOn();
}

void Voice::noteOff()
{
    envelope_.noteOff();
    fenv_.noteOff();
    env2_.noteOff();
    fenv2_.noteOff();
}

void Voice::process (juce::AudioBuffer<float>& buffer)
{
    if (!isActive_) return;

    const int numSamples = buffer.getNumSamples();
    voiceBuffer_.clear();

    // ── LFO ────────────────────────────────────────────────────────────────────
    const float lfoValue = lfo_.process();

    const float pitchMod = (lfoTarget_ == 2 && lfoDepth_ > 0.1f)
                         ? lfoValue * lfoDepth_ * 0.5f : 0.f;

    const float osc2TuneCents = kMultCents[osc2Mult_];

    // ── Osc 2 signal (needed before Osc 1 for FM mode) ────────────────────────
    if (osc2Enabled_)
    {
        osc2Buffer_.clear();
        oscB_.process (osc2Buffer_, midiNoteNumber_, osc2TuneCents, 0.f);

        if (osc2EnvEnabled_)
            env2_.process (osc2Buffer_);
        else
            env2_.advance (numSamples);

        if (osc2FilterEnabled_)
        {
            const float fenv2Val = fenv2_.getCurrentLevel();
            fenv2_.advance (numSamples);
            const float c = osc2FilterCutoff_
                          * std::pow (2.f, osc2FenvDepth_ * fenv2Val / 25.f);
            filter2_.setCutoff    (juce::jlimit (20.f, 20000.f, c));
            filter2_.setResonance (osc2FilterResonance_);
            filter2_.process      (osc2Buffer_);
        }
        else
        {
            fenv2_.advance (numSamples);
        }
    }

    // ── Osc 1 generation ───────────────────────────────────────────────────────
    if (osc2Enabled_ && osc2MixMode_ == MixMode::FM)
    {
        // Osc 2 is the modulator: depth 0-100 → 0-24 semitones peak deviation
        const float fmDepth = osc2MixDepth_ * 0.24f;
        oscillator_.processWithFM  (voiceBuffer_, midiNoteNumber_,
                                     oscTune_ + pitchMod,  oscDetune_,
                                     osc2Buffer_, fmDepth);
        oscillator2_.processWithFM (voiceBuffer_, midiNoteNumber_,
                                     oscTune_ + pitchMod, -oscDetune_,
                                     osc2Buffer_, fmDepth);
    }
    else
    {
        oscillator_.process  (voiceBuffer_, midiNoteNumber_, oscTune_ + pitchMod,  oscDetune_);
        oscillator2_.process (voiceBuffer_, midiNoteNumber_, oscTune_ + pitchMod, -oscDetune_);
    }
    voiceBuffer_.applyGain (0.5f); // normalise unison pair

    // ── Filter 1 ───────────────────────────────────────────────────────────────
    {
        const float fenvValue = fenv_.getCurrentLevel();
        fenv_.advance (numSamples);

        float cutoff = filterCutoff_;
        if (std::abs (envelopeFilterMod_) > 0.1f)
            cutoff *= std::pow (2.f, envelopeFilterMod_ * fenvValue / 25.f);
        if (lfoTarget_ == 0 && lfoDepth_ > 0.1f)
            cutoff *= std::pow (2.f, lfoValue * lfoDepth_ * 0.02f);

        filter_.setCutoff    (juce::jlimit (20.f, 20000.f, cutoff));
        filter_.setResonance (filterResonance_);
        filter_.process      (voiceBuffer_);
    }

    // ── Volume envelope 1 ──────────────────────────────────────────────────────
    envelope_.process (voiceBuffer_);

    // ── Mix Osc 2 into Osc 1 (SUM / AM / RING; FM was handled above) ──────────
    if (osc2Enabled_ && osc2MixMode_ != MixMode::FM)
    {
        const float depth  = osc2MixDepth_ * 0.01f;
        const int   numCh  = voiceBuffer_.getNumChannels();
        const float* b     = osc2Buffer_.getReadPointer (0);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* a = voiceBuffer_.getWritePointer (ch);

            switch (osc2MixMode_)
            {
                case MixMode::Sum:
                    for (int n = 0; n < numSamples; ++n)
                        a[n] += depth * b[n];
                    break;

                case MixMode::AM:
                    // carrier × (1 + depth × modulator)
                    for (int n = 0; n < numSamples; ++n)
                        a[n] *= (1.f + depth * b[n]);
                    break;

                case MixMode::Ring:
                    // pure multiplication (suppresses carrier)
                    for (int n = 0; n < numSamples; ++n)
                        a[n] *= b[n];
                    break;

                case MixMode::FM: // FM is applied at Osc A generation time
                default:
                    break;
            }
        }
    }

    // ── Output gain + amplitude LFO ────────────────────────────────────────────
    float gainLinear = velocity_ * juce::Decibels::decibelsToGain (outputGain_);
    if (lfoTarget_ == 1 && lfoDepth_ > 0.1f)
        gainLinear *= (1.f + lfoValue * lfoDepth_ * 0.005f);
    voiceBuffer_.applyGain (gainLinear);

    // ── Mix to output ───────────────────────────────────────────────────────────
    for (int ch = 0; ch < buffer.getNumChannels() && ch < voiceBuffer_.getNumChannels(); ++ch)
        buffer.addFrom (ch, 0, voiceBuffer_, ch, 0, numSamples);

    timeSinceNoteOn_ += numSamples / static_cast<float> (sampleRate_);

    if (!envelope_.isActive())
        isActive_ = false;
}
