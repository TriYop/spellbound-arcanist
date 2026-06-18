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
    computeEnvelopeValue(); // updates currentLevel_

    float samplesPerSecond = static_cast<float> (sampleRate_);
    stageProgress_ += 1.f / samplesPerSecond;

    if (stage_ == Stage::Attack && stageProgress_ >= attack_)
    {
        stage_ = Stage::Decay;
        stageProgress_ = 0.f;
    }
    else if (stage_ == Stage::Decay && stageProgress_ >= decay_)
    {
        // ADR mode: bypass Sustain, go directly to Release
        stage_ = sustainEnabled_ ? Stage::Sustain : Stage::Release;
        stageProgress_ = 0.f;
    }
    else if (stage_ == Stage::Release && stageProgress_ >= release_)
    {
        stage_ = Stage::Inactive;
        stageProgress_ = 0.f;
    }
}

void Envelope::process (juce::AudioBuffer<float>& buffer)
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

    return juce::jlimit (0.f, 1.f, currentLevel_);
}
