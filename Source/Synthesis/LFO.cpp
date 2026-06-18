#include "LFO.h"

LFO::LFO()
{
}

LFO::~LFO()
{
}

void LFO::prepare (double sampleRate, int samplesPerBlock)
{
    callRate_ = sampleRate / static_cast<double> (samplesPerBlock);
    phase_ = 0.f;
}

float LFO::process()
{
    // Sine wave LFO
    float sample = std::sin (phase_ * TWO_PI);

    // Advance phase
    float phaseIncrement = speed_ / static_cast<float> (callRate_);
    phase_ += phaseIncrement;

    if (phase_ >= 1.f)
        phase_ -= 1.f;

    return sample;
}
