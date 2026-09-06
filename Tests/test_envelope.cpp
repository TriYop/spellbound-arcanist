// Tests/test_envelope.cpp
#include "test_runner.h"
#include "../Source/Synthesis/Envelope.h"
#include "../Source/Synthesis/AudioBuffer.h"
#include <cmath>

int main()
{
    Envelope env;
    env.prepare (100.0, 3.0); // 100 Hz sample rate for easy arithmetic
    env.setAttack (0.1f);     // 10 samples
    env.setDecay (0.1f);      // 10 samples
    env.setSustain (0.5f);
    env.setRelease (0.2f);    // 20 samples
    env.setSustainEnabled (true);

    env.noteOn();
    CHECK (env.isActive());

    // Midway through attack (5 of 10 samples), level should be ~0.5 (linear
    // ramp from 0 to 1).
    env.advance (5);
    CHECK_MSG (std::abs (env.getCurrentLevel() - 0.5f) < 0.05f, "attack ramp not linear as expected");

    // Finish attack, run through decay to sustain.
    env.advance (5);  // end of attack, level == 1.0
    env.advance (10); // end of decay, level == sustain (0.5)
    CHECK_MSG (std::abs (env.getCurrentLevel() - 0.5f) < 0.01f, "should have settled at sustain level");
    CHECK (env.isActive());

    // Sustain holds until noteOff.
    env.advance (50);
    CHECK_MSG (std::abs (env.getCurrentLevel() - 0.5f) < 0.01f, "sustain must hold, not decay further");

    env.noteOff();
    env.advance (20); // full release
    CHECK_MSG (env.getCurrentLevel() < 0.01f, "should be silent after release completes");
    CHECK_MSG (!env.isActive(), "should be inactive after release completes");

    TEST_SUMMARY();
    return 0;
}
