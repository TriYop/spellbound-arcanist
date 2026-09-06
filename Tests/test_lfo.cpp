// Tests/test_lfo.cpp
#include "test_runner.h"
#include "../Source/Synthesis/LFO.h"
#include <cmath>

int main()
{
    LFO lfo;
    lfo.prepare (44100.0, 512); // callRate_ = 44100/512 ~= 86.13 Hz
    lfo.setSpeed (0.5f);

    // First call: phase starts at 0, so sin(0) == 0.
    CHECK_MSG (std::abs (lfo.process()) < 0.0001f, "first sample must be sin(0) == 0");

    // Output stays within [-1, 1] over many calls (sine wave bound).
    for (int i = 0; i < 1000; ++i)
    {
        float v = lfo.process();
        CHECK_MSG (v >= -1.0001f && v <= 1.0001f, "LFO output must stay within [-1, 1]");
    }

    TEST_SUMMARY();
    return 0;
}
