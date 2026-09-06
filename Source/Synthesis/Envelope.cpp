#include "Envelope.h"

Envelope::Envelope()
{
}

Envelope::~Envelope()
{
}

void Envelope::prepare (double sampleRate, double tailLength)
{
    sampleRate_ = sampleRate;
    tailTime_   = static_cast<float> (tailLength);
}

void Envelope::tickSample()
{
    // Advance progress (and handle stage transitions) *before* computing the
    // envelope value for this sample, so computeEnvelopeValue() always sees
    // the post-increment progress. Computing the value first (against the
    // pre-increment progress) introduced a systematic one-sample lag: attack
    // never quite reached 1.0, decay never quite settled at the sustain
    // level, and release never quite reached 0 before the stage flipped to
    // Inactive. See TriYop/spellbound-arcanist#1.
    float samplesPerSecond = static_cast<float> (sampleRate_);
    stageProgress_ += 1.f / samplesPerSecond;

    // Repeated float addition of 1/sampleRate accumulates rounding error, so
    // an exact >= comparison against the stage length can fire one sample
    // late (e.g. 10 additions of 1.f/100.f sums to just under the float
    // literal 0.1f). A small epsilon absorbs that drift without materially
    // changing stage timing. See TriYop/spellbound-arcanist#2.
    constexpr float kEpsilon = 1e-5f;

    if (stage_ == Stage::Attack && stageProgress_ >= attack_ - kEpsilon)
    {
        stage_ = Stage::Decay;
        stageProgress_ = 0.f;
    }
    else if (stage_ == Stage::Decay && stageProgress_ >= decay_ - kEpsilon)
    {
        // ADR mode: bypass Sustain, go directly to Release
        stage_ = sustainEnabled_ ? Stage::Sustain : Stage::Release;
        stageProgress_ = 0.f;
    }
    else if (stage_ == Stage::Release && stageProgress_ >= release_ - kEpsilon)
    {
        stage_ = Stage::Inactive;
        stageProgress_ = 0.f;
    }

    computeEnvelopeValue(); // updates currentLevel_
}

void Envelope::process (dsp::AudioBuffer& buffer)
{
    int numChannels = buffer.getNumChannels();

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        tickSample();
        float v = currentLevel_;
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[n] *= v;
    }
}

void Envelope::advance (int numSamples)
{
    for (int n = 0; n < numSamples; ++n)
        tickSample();
}

void Envelope::noteOn()
{
    levelAtNoteOn_ = currentLevel_;
    stage_         = Stage::Attack;
    stageProgress_ = 0.f;
    noteOffTriggered_ = false;
}

void Envelope::noteOff()
{
    noteOffTriggered_ = true;
    if (stage_ != Stage::Inactive)
    {
        stage_         = Stage::Release;
        stageProgress_ = 0.f;
    }
}

float Envelope::computeEnvelopeValue()
{
    switch (stage_)
    {
        case Stage::Attack:
            if (attack_ > 0.f)
                currentLevel_ = levelAtNoteOn_ + (1.f - levelAtNoteOn_) * (stageProgress_ / attack_);
            else
                currentLevel_ = 1.f;
            break;

        case Stage::Decay:
            if (decay_ > 0.f)
            {
                float progress = stageProgress_ / decay_;
                currentLevel_ = 1.f - progress * (1.f - sustain_);
            }
            else
                currentLevel_ = sustain_;
            break;

        case Stage::Sustain:
            currentLevel_ = sustain_;
            break;

        case Stage::Release:
            if (release_ > 0.f)
            {
                float progress = stageProgress_ / release_;
                currentLevel_ = sustain_ * (1.f - progress);
            }
            else
                currentLevel_ = 0.f;
            break;

        case Stage::Inactive:
            currentLevel_ = 0.f;
            break;
    }

    return std::clamp (currentLevel_, 0.f, 1.f);
}
