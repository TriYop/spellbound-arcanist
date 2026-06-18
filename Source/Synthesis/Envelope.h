#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class Envelope
{
public:
    Envelope();
    ~Envelope();

    void prepare (double sampleRate, double tailLength);
    void process (juce::AudioBuffer<float>& buffer);
    void advance (int numSamples); // advance state without touching audio (for filter envelope)

    void noteOn();
    void noteOff();

    void setAttack  (float attackSeconds)  { attack_  = attackSeconds; }
    void setDecay   (float decaySeconds)   { decay_   = decaySeconds; }
    void setSustain (float sustainLevel)   { sustain_ = juce::jlimit (0.f, 1.f, sustainLevel); }
    void setRelease (float releaseSeconds) { release_ = releaseSeconds; }
    void setSustainEnabled (bool enabled)  { sustainEnabled_ = enabled; }

    float getCurrentLevel() const { return currentLevel_; }
    bool  isActive()        const { return stage_ != Stage::Inactive; }

private:
    enum class Stage { Attack, Decay, Sustain, Release, Inactive };

    Stage stage_ = Stage::Inactive;
    double sampleRate_ = 44100.0;

    float attack_  = 0.1f;
    float decay_   = 0.5f;
    float sustain_ = 0.8f;
    float release_ = 3.f;

    bool  sustainEnabled_  = true; // false = ADR: Decay→Release without waiting for note-off

    float currentLevel_    = 0.f;
    float levelAtNoteOn_   = 0.f;
    float stageProgress_   = 0.f;
    float tailTime_        = 0.f;
    bool  noteOffTriggered_ = false;

    float computeEnvelopeValue();
    void  tickSample(); // advance one sample: updates currentLevel_ and stageProgress_
};
