// Tests/test_voice.cpp
#include "test_runner.h"
#include "../Source/Synthesis/Voice.h"
#include "../Source/Synthesis/AudioBuffer.h"
#include <cmath>

int main()
{
    Voice v;
    v.prepare (44100.0, 512, 3.0);
    v.setWaveform (Oscillator::Waveform::Sine);
    v.setEnvelopeAttack (0.0f);   // instant attack, so output is audible immediately
    v.setEnvelopeDecay (0.0f);
    v.setEnvelopeSustain (1.0f);
    v.setEnvelopeRelease (1.0f);
    v.setEnvelopeSustainEnabled (true);
    v.setOutputGain (0.0f);       // 0 dB, unity

    CHECK (!v.isActive());

    v.noteOn (69, 1.0f); // A4, full velocity
    CHECK (v.isActive());
    CHECK (v.getNoteNumber() == 69);

    dsp::AudioBuffer buf;
    buf.setSize (2, 512);
    buf.clear();
    v.process (buf);

    // Some non-silent output must have been added to the buffer.
    bool anyNonZero = false;
    for (int n = 0; n < 512; ++n)
        if (std::abs (buf.getReadPointer (0)[n]) > 0.001f) anyNonZero = true;
    CHECK_MSG (anyNonZero, "an active voice must produce non-silent output");

    v.noteOff();
    // Advance well past the 1 s release at 44100 Hz -- run enough blocks that
    // the voice's envelope reaches Inactive and self-deactivates.
    for (int block = 0; block < 100; ++block)
    {
        buf.clear();
        v.process (buf);
    }
    CHECK_MSG (!v.isActive(), "voice must deactivate once its release finishes");

    TEST_SUMMARY();
    return 0;
}
