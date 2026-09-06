// Tests/test_filter.cpp
#include "test_runner.h"
#include "../Source/Synthesis/Filter.h"
#include "../Source/Synthesis/AudioBuffer.h"
#include <cmath>

int main()
{
    // A low-pass RBJ-style biquad has unity DC gain: feeding a constant 1.0
    // signal for long enough (past the cutoff-smoothing ramp) must converge
    // the output to ~1.0, not attenuate or blow up.
    Filter f;
    f.prepare (44100.0);
    f.setMode (Filter::Mode::LowPass);
    f.setCutoff (1000.0f);
    f.setResonance (0.0f);

    dsp::AudioBuffer buf;
    buf.setSize (1, 1);

    float last = 0.0f;
    for (int block = 0; block < 2000; ++block)
    {
        buf.getWritePointer (0)[0] = 1.0f;
        f.process (buf);
        last = buf.getReadPointer (0)[0];
    }
    CHECK_MSG (std::abs (last - 1.0f) < 0.01f, "LowPass must converge to ~unity DC gain");

    TEST_SUMMARY();
    return 0;
}
