// Tests/test_dsputil.cpp
#include "test_runner.h"
#include "../Source/Synthesis/DspUtil.h"
#include <cmath>

int main()
{
    // midiNoteToHz(69) == 440 Hz (A4), the JUCE reference value.
    CHECK (std::abs (dsp::midiNoteToHz (69) - 440.0f) < 0.001f);
    CHECK (std::abs (dsp::midiNoteToHz (57) - 220.0f) < 0.001f); // one octave down
    CHECK (std::abs (dsp::midiNoteToHz (81) - 880.0f) < 0.01f);  // one octave up

    // dbToGain/gainToDb round-trip, and 0 dB == unity gain.
    CHECK (std::abs (dsp::dbToGain (0.0f) - 1.0f) < 0.0001f);
    CHECK (std::abs (dsp::dbToGain (-6.0206f) - 0.5f) < 0.001f);
    CHECK (std::abs (dsp::gainToDb (1.0f, -100.0f) - 0.0f) < 0.001f);
    CHECK (std::abs (dsp::gainToDb (0.5f, -100.0f) - (-6.0206f)) < 0.01f);
    CHECK (dsp::gainToDb (0.0f, -100.0f) == -100.0f); // silence floors to minDb, no -inf

    TEST_SUMMARY();
    return 0;
}
