// Tests/test_audiobuffer.cpp
#include "test_runner.h"
#include "../Source/Synthesis/AudioBuffer.h"

int main()
{
    // Owned mode: setSize allocates and clear() zeroes.
    {
        dsp::AudioBuffer buf;
        buf.setSize (2, 4);
        CHECK (buf.getNumChannels() == 2);
        CHECK (buf.getNumSamples() == 4);
        buf.getWritePointer (0)[0] = 1.0f;
        buf.getWritePointer (1)[3] = 2.0f;
        buf.clear();
        CHECK (buf.getReadPointer (0)[0] == 0.0f);
        CHECK (buf.getReadPointer (1)[3] == 0.0f);
    }

    // applyGain scales every sample on every channel.
    {
        dsp::AudioBuffer buf;
        buf.setSize (1, 3);
        float* w = buf.getWritePointer (0);
        w[0] = 1.0f; w[1] = 2.0f; w[2] = 3.0f;
        buf.applyGain (2.0f);
        CHECK (buf.getReadPointer (0)[0] == 2.0f);
        CHECK (buf.getReadPointer (0)[1] == 4.0f);
        CHECK (buf.getReadPointer (0)[2] == 6.0f);
    }

    // addFrom accumulates (does not overwrite).
    {
        dsp::AudioBuffer dest, src;
        dest.setSize (1, 2);
        src.setSize (1, 2);
        dest.getWritePointer (0)[0] = 1.0f;
        dest.getWritePointer (0)[1] = 1.0f;
        src.getWritePointer (0)[0]  = 5.0f;
        src.getWritePointer (0)[1]  = 5.0f;
        dest.addFrom (0, 0, src, 0, 0, 2);
        CHECK (dest.getReadPointer (0)[0] == 6.0f);
        CHECK (dest.getReadPointer (0)[1] == 6.0f);
    }

    // wrapExternal: a non-owning view over raw pointers (DPF's float**).
    {
        float chanL[4] = { 0.f, 0.f, 0.f, 0.f };
        float chanR[4] = { 0.f, 0.f, 0.f, 0.f };
        float* channels[2] = { chanL, chanR };

        dsp::AudioBuffer view;
        view.wrapExternal (channels, 2, 4);
        view.getWritePointer (0)[1] = 9.0f;
        CHECK (chanL[1] == 9.0f); // wrote straight into the caller's array, not a copy
        CHECK (view.getNumChannels() == 2);
        CHECK (view.getNumSamples() == 4);
    }

    TEST_SUMMARY();
    return 0;
}
