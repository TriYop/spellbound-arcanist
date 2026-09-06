// Tests/test_oscillator.cpp
#include "test_runner.h"
#include "../Source/Synthesis/Oscillator.h"
#include "../Source/Synthesis/AudioBuffer.h"

int main()
{
    // Sine at A4 (note 69), no tune/detune: phase starts at 0, so sample 0
    // must be sin(0) == 0. process() is additive (it accumulates into the
    // buffer, matching the JUCE-era contract) -- pre-seed with a known value
    // to prove it doesn't overwrite.
    {
        Oscillator osc;
        osc.prepare (44100.0);
        osc.setWaveform (Oscillator::Waveform::Sine);

        dsp::AudioBuffer buf;
        buf.setSize (1, 4);
        buf.getWritePointer (0)[0] = 0.5f; // pre-existing content

        osc.process (buf, 69, 0.0f, 0.0f);
        CHECK_MSG (std::abs (buf.getReadPointer (0)[0] - 0.5f) < 0.0001f,
                   "process() must add to existing buffer content, not overwrite");
    }

    // Square wave is +1 for the first half of the cycle.
    {
        Oscillator osc;
        osc.prepare (44100.0);
        osc.setWaveform (Oscillator::Waveform::Square);

        dsp::AudioBuffer buf;
        buf.setSize (1, 1);
        osc.process (buf, 69, 0.0f, 0.0f);
        CHECK (buf.getReadPointer (0)[0] == 1.0f);
    }

    TEST_SUMMARY();
    return 0;
}
