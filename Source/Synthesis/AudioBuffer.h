#pragma once
#include <vector>
#include <cstring>

namespace dsp
{

// Minimal, framework-free stand-in for juce::AudioBuffer<float>, matching
// exactly the subset of its API Arcanist's Synthesis/ classes actually use.
// Two modes, selected by which setup call you use:
//   - setSize(ch, n)       -- owns its storage (used by Voice's scratch
//                             buffers, prepared once in prepare()).
//   - wrapExternal(ptrs..) -- a non-owning view over caller-supplied raw
//                             channel pointers (used by ArcanistPluginAdapter
//                             to wrap DPF's float** run() output without a copy).
// Both modes support the same read/write/mix operations below.
class AudioBuffer
{
public:
    void setSize (int numChannels, int numSamples)
    {
        owned_.assign (static_cast<size_t> (numChannels),
                        std::vector<float> (static_cast<size_t> (numSamples), 0.0f));
        channels_.resize (static_cast<size_t> (numChannels));
        for (size_t ch = 0; ch < channels_.size(); ++ch)
            channels_[ch] = owned_[ch].data();
        numChannels_ = numChannels;
        numSamples_  = numSamples;
    }

    void wrapExternal (float* const* externalChannels, int numChannels, int numSamples)
    {
        owned_.clear();
        channels_.assign (externalChannels, externalChannels + numChannels);
        numChannels_ = numChannels;
        numSamples_  = numSamples;
    }

    void clear()
    {
        for (int ch = 0; ch < numChannels_; ++ch)
            std::memset (channels_[static_cast<size_t> (ch)], 0, sizeof (float) * static_cast<size_t> (numSamples_));
    }

    int getNumChannels() const { return numChannels_; }
    int getNumSamples()  const { return numSamples_; }

    float*       getWritePointer (int channel)       { return channels_[static_cast<size_t> (channel)]; }
    const float* getReadPointer  (int channel) const  { return channels_[static_cast<size_t> (channel)]; }

    void applyGain (float gain)
    {
        for (int ch = 0; ch < numChannels_; ++ch)
        {
            float* w = getWritePointer (ch);
            for (int n = 0; n < numSamples_; ++n)
                w[n] *= gain;
        }
    }

    void addFrom (int destChannel, int destStartSample,
                  const AudioBuffer& source, int sourceChannel, int sourceStartSample,
                  int numSamplesToAdd)
    {
        float*       dst = getWritePointer (destChannel) + destStartSample;
        const float* src = source.getReadPointer (sourceChannel) + sourceStartSample;
        for (int n = 0; n < numSamplesToAdd; ++n)
            dst[n] += src[n];
    }

private:
    std::vector<std::vector<float>> owned_;
    std::vector<float*> channels_;
    int numChannels_ = 0;
    int numSamples_  = 0;
};

} // namespace dsp
