#include "Filter.h"
#include <cmath>

Filter::Filter()
{
}

Filter::~Filter()
{
}

void Filter::prepare (double sampleRate)
{
    sampleRate_ = sampleRate;
    z1_.resize (2, 0.f);
    z2_.resize (2, 0.f);
    updateCoefficients();
}

void Filter::process (juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer (ch);
        size_t chIndex = static_cast<size_t> (ch);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            // Smooth cutoff changes
            cutoff_ += (targetCutoff_ - cutoff_) * smoothingFactor_;

            // Update coefficients if cutoff changed significantly
            if (std::abs (targetCutoff_ - cutoff_) > 0.1f)
                updateCoefficients();

            float in = samples[n];
            float out = b0_ * in + z1_[chIndex];
            z1_[chIndex] = b1_ * in - a1_ * out + z2_[chIndex];
            z2_[chIndex] = b2_ * in - a2_ * out;

            samples[n] = out;
        }
    }
}

void Filter::setCutoff (float cutoff)
{
    targetCutoff_ = juce::jlimit (20.f, 20000.f, cutoff);
}

void Filter::setResonance (float resonance)
{
    resonance_ = juce::jlimit (0.f, 1.f, resonance);
    updateCoefficients();
}

void Filter::updateCoefficients()
{
    float wc = 2.f * 3.14159265f * cutoff_ / static_cast<float> (sampleRate_);
    wc = juce::jlimit (0.001f, 3.14159f, wc);

    float sinWc = std::sin (wc);
    float cosWc = std::cos (wc);
    float q = 0.5f + resonance_ * 3.5f;
    float alpha = sinWc / (2.f * q);

    float a0 = 1.f + alpha;
    a1_ = -2.f * cosWc / a0;
    a2_ = (1.f - alpha) / a0;

    switch (mode_)
    {
        case Mode::LowPass:
            b0_ = (1.f - cosWc) / 2.f / a0;
            b1_ = (1.f - cosWc)       / a0;
            b2_ = (1.f - cosWc) / 2.f / a0;
            break;
        case Mode::BandPass:
            b0_ =  alpha / a0;
            b1_ =  0.f;
            b2_ = -alpha / a0;
            break;
        case Mode::HighPass:
            b0_ =  (1.f + cosWc) / 2.f / a0;
            b1_ = -(1.f + cosWc)       / a0;
            b2_ =  (1.f + cosWc) / 2.f / a0;
            break;
    }
}
