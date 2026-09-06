# Arcanist DPF Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate Arcanist (a 16-voice pad/drone synthesizer, currently JUCE VST3/AU/Standalone via `clap-juce-extensions`) off JUCE onto DPF (ISC-licensed), reusing the pattern Hex/Pugilist/Tank/Outflank already proved, while (a) decoupling its currently JUCE-coupled synthesis engine to framework-free C++ with real unit tests — unlike Tank/Outflank, Arcanist's DSP was never framework-free — (b) porting its existing 42-preset factory library onto `AudioPlugins/Common`'s `PresetBrowser`/`PresetSelector`, and (c) adding an embedded on-screen MIDI keyboard using the exact `hui::dgl::MidiKeyboard` pattern already proven on Pugilist.

**Architecture:** `Source/Synthesis/{Oscillator,Filter,Envelope,LFO,Voice}` currently take `juce::AudioBuffer<float>&` and call `juce::jlimit`/`juce::Decibels::decibelsToGain`/`juce::MidiMessage::getMidiNoteInHertz` — contrary to the stale assumption that this DSP already embeds `juce::dsp::StateVariableTPTFilter` (it doesn't; `Filter` is a hand-rolled RBJ-style biquad SVF that only borrows `juce::AudioBuffer`/`juce::jlimit` at its boundary). Task 2 replaces those JUCE touchpoints with a tiny local `dsp::AudioBuffer` (a drop-in, framework-free class matching the exact subset of `juce::AudioBuffer`'s API actually used: `setSize`/`clear`/`getNumChannels`/`getNumSamples`/`getWritePointer`/`getReadPointer`/`applyGain`/`addFrom`, plus a non-owning `wrapExternal()` mode for DPF's raw `float**` output) and a `Source/Synthesis/DspUtil.h` of three free functions (`midiNoteToHz`, `dbToGain`/`gainToDb`, using `std::clamp` in place of `juce::jlimit`). `LFO` is already framework-free (only `<cmath>`) and needs no change beyond its own new test. The JUCE-era `PluginProcessor`/`PluginEditor`/`PresetManager`/`ArcanistLookAndFeel`/`LedButton` move unchanged to `Source/_juce_reference/` as the porting reference. `Source/ArcanistPluginAdapter` (DPF `Plugin`) owns a `std::vector<Voice>` (16 voices) and ports `PluginProcessor::processBlock()`'s same-note-reuse/free-voice/oldest-voice-steal algorithm verbatim onto DPF's `MidiEvent` array. `Source/ArcanistUI` (DPF/NanoVG `UI`) drives `AudioPlugins/Common`'s DGL widgets (`RotaryKnob` for the ~29 continuous parameters, `ToggleSwitch` for the 7 boolean parameters, a new Arcanist-local `RadioButtonGroup` composed from `hui::dgl::Button` for the 7 mutually-exclusive choice parameters — Common's `Button` has no built-in radio/group state, and per Common's own design note adapter/composition code belongs in the consuming plugin until proven shared by ≥2 consumers), `VuMeter` for the output peak, `PresetSelector`/`Button` for the presets bar, and `hui::dgl::MidiKeyboard` for an embedded 3-octave keyboard starting at MIDI 48 (C3) — pads/drones are chord instruments played from around middle C upward (unlike Pugilist's fixed low GM percussion notes 35–51), so no note-zone tinting is needed; Arcanist is fully chromatic and polyphonic, not pad-per-note.

**Tech Stack:** C++20, CMake + Ninja, DPF (DISTRHO Plugin Framework, pinned commit `4238e1c7f0351bbe488d79f0899c540543ac7583`, ISC), `AudioPlugins/Common` (`AudioPluginsCommon::io`/`hui_dgl`/`presets`, tag TBD per Task 1 — see Global Constraints), pugixml (via Common), CTest for framework-free unit tests.

**Spec:** No separate design-spec doc precedes this plan (unlike Tank/Outflank, which drew on a spec already written in the `Common` repo). This plan is self-contained; every architectural decision below was verified directly against Arcanist's actual `Source/` tree, Pugilist's actual DPF adapter/UI code, and `AudioPlugins/Common`'s actual current git history (not the possibly-stale `CLAUDE.md` files) — see each task's rationale for the specific files/commits inspected.

## Global Constraints

- **Resolved dependency gap (verify, don't re-fix):** `AudioPlugins/Common`'s `hui::dgl::MidiKeyboard` widget (needed for the embedded keyboard) previously had no single tag containing both it and the later `VuMeter`/`MeterTransport` work — `v0.2.1` had `MidiKeyboard` but not the later work; `v0.3.0` had the later work but not `MidiKeyboard` (the widget lived only on `Common`'s stray `worktree-dpf-stage1` branch; PR #1's merge into `master` stopped one commit short of it). This has since been fixed directly in `Common`: the `MidiKeyboard`/`MidiKeyboardLayout`/`KeyZone` commits were cherry-picked onto current `master`, the full suite verified 12/12, and the result tagged and pushed as **`Common v0.4.0`**, which now has everything. Task 1 only needs to *verify* `v0.4.0` is present (`git ls-remote --tags https://github.com/TriYop/spellbound-common.git` — confirmed present 2026-09-06) and pin it; do not re-run any cherry-pick/tag remediation, that work is already done.
- DPF has no tagged releases; pin the exact commit SHA `4238e1c7f0351bbe488d79f0899c540543ac7583` (same commit Hex/Pugilist/Tank/Outflank use), applying **only** `cmake/patches/dpf-clap-state-chunked-read.patch` via `cmake/apply_patch.cmake` — copied verbatim from Pugilist's/Tank's `cmake/patches/`. Do **not** apply `dpf-clap-activate-latency.patch`: Arcanist's JUCE-era code has no `getLatencySamples()`/`setLatency()` call anywhere (confirmed by reading `Source/PluginProcessor.cpp` in full), so there is no latency-reporting-during-activate path for that patch to fix. If a later task discovers Arcanist needs to report latency, add the patch then and note why in that task's commit.
- `CMAKE_POSITION_INDEPENDENT_CODE ON` must be set *before* `FetchContent_MakeAvailable(AudioPluginsCommon)`, and the `AudioPluginsCommon` `FetchContent_Declare`/`MakeAvailable` pair must come *after* `dpf_add_plugin(Arcanist ...)` — both ordering constraints carried over verbatim from Hex/Pugilist/Tank's `CMakeLists.txt`.
- `AudioPluginsCommon::hui_dgl` and `AudioPluginsCommon::presets` link only into the `Arcanist-ui` static lib DPF creates (never into `Arcanist`/`Arcanist-dsp`/the LV2 binary). `AudioPluginsCommon::io` is not needed by the DSP side (Arcanist has no click-prone real-time gain smoothing beyond what `Envelope`/`Filter` already do internally) — do not link it into `Arcanist` unless a later task finds an actual need.
- **CI needs a `COMMON_REPO_TOKEN` secret** to clone the private `spellbound-common` repo over HTTPS — this was missed on both Tank's and Outflank's PRs and broke CI for a day on each. Verify the real repo name first (`git remote -v` in Arcanist — confirmed 2026-09-06: `git@github.com:TriYop/spellbound-arcanist.git`) and run `gh secret set COMMON_REPO_TOKEN --repo TriYop/spellbound-arcanist` with the same PAT already used for Hex/Pugilist/Tank/Outflank, verifying with `gh secret list --repo TriYop/spellbound-arcanist` that it is present — **as an explicit step inside Task 3** (the CI-skeleton task), not deferred to the final verification task.
- Windows/macOS CI legs stay `continue-on-error: true`; only the Linux leg gates the workflow (Win/Mac/`.deb` are Phase 5+, not an exit criterion here).
- "Ticket per task" workspace convention: file one GitHub issue per validator finding or numerical-stability/behavior-preservation bug discovered in Task 2 (decoupling) or Task 7 (validators), and reference the issue number in the commit that fixes it.
- Standing workspace expectation (clean code, hexagonal/DDD, TDD) applies throughout: DSP stays framework-free and unit-tested; DPF/DGL adapter code is a thin shim with no business logic of its own; UI radio-group composition is local to Arcanist, not pushed into `Common` speculatively.
- No copyleft license contamination: DPF is ISC-licensed; do not add any GPL/LGPL dependency.
- Arcanist currently has **zero** existing tests (no `Tests/` directory) — Task 2 is the first test suite this plugin has ever had. There is no pre-migration test baseline to preserve; "baseline" in Task 1 means "the JUCE build still configures/builds/runs," not "tests still pass."

---

## Task 1: Verify the Common v0.4.0 dependency and create an isolated worktree

The `MidiKeyboard` gap described in Global Constraints (no single `Common` tag used to contain both the keyboard widget and the later `VuMeter`/`MeterTransport` work) has already been fixed directly in `Common`: cherry-picked onto current `master`, verified 12/12 tests, tagged and pushed as `v0.4.0`. This task only verifies that resolution and pins it — it does not redo any remediation.

**Files:** none.

**Interfaces:** N/A — this task only confirms the dependency Task 3 pins.

- [ ] **Step 1: Verify `Common v0.4.0` is present and has everything Arcanist needs**

```bash
git ls-remote --tags https://github.com/TriYop/spellbound-common.git | grep -E "v0\.4\.0|v0\.3\.0"
```

Expected: both `v0.3.0` and `v0.4.0` present, `v0.4.0` newer. Then confirm the widget is actually in it (not just the tag name):

```bash
cd /home/yvan/Projects/AudioPlugins/Common
git fetch origin --tags
git show v0.4.0:include/audioplugins/common/hui/dgl/MidiKeyboard.h > /dev/null 2>&1 && echo "MidiKeyboard present in v0.4.0" || echo "STILL MISSING -- stop, this contradicts the resolution notice"
git show v0.4.0:include/audioplugins/common/hui/dgl/VuMeter.h | grep -q Horizontal && echo "VuMeter Horizontal orientation present in v0.4.0" || echo "STILL MISSING -- stop"
```

Expected: both checks print "present" — `v0.4.0` has the restored keyboard widget and everything `v0.3.0` already had. If either check fails, stop and re-open the gap as a blocker rather than proceeding on a bad assumption.

- [ ] **Step 2: Confirm Arcanist's current git state is clean**

```bash
cd /home/yvan/Projects/AudioPlugins/Arcanist
git status --short
git remote -v
```

Expected: `origin` is `git@github.com:TriYop/spellbound-arcanist.git`, branch `main`. Leave any unrelated stray files (e.g. the `Gemini_Generated_Image_*.png` at the repo root) untouched.

- [ ] **Step 3: Create the isolated worktree**

Invoke `superpowers:using-git-worktrees` with branch name `worktree-dpf-stage0` (matching Hex/Pugilist/Tank/Outflank's naming). Fallback if no native tool is available:

```bash
git worktree add .worktrees/worktree-dpf-stage0 -b worktree-dpf-stage0
cd .worktrees/worktree-dpf-stage0
```

- [ ] **Step 4: Verify the JUCE baseline still builds**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
./build/Arcanist_artefacts/Debug/Standalone/Arcanist --help 2>/dev/null || true
```

Expected: configures/builds cleanly. There is no existing test suite to run (see Global Constraints) — a clean build is the only baseline.

- [ ] **Step 5: Report readiness**

State the worktree's full path, confirm `Common v0.4.0` is verified present with both `MidiKeyboard` and `VuMeter`'s `Horizontal` orientation, and confirm the JUCE baseline builds, before starting Task 2.

---

## Task 2: Decouple the synthesis engine from JUCE (framework-free DSP, TDD)

This is the task Tank/Outflank never needed: Arcanist's `Source/Synthesis/*` classes take `juce::AudioBuffer<float>&` and call three JUCE free functions. This task replaces both with plain C++, one class at a time, writing each new/changed behavior's test **before** the implementation change, per this workspace's TDD standard. No `Tests/` directory exists yet — this task creates it.

**Files:**
- Create: `Source/Synthesis/AudioBuffer.h`
- Create: `Source/Synthesis/DspUtil.h`
- Create: `Tests/test_runner.h` (copied verbatim from Common's/Pugilist's/Tank's `tests/test_runner.h` — do not reinvent)
- Create: `Tests/test_dsputil.cpp`
- Create: `Tests/test_audiobuffer.cpp`
- Create: `Tests/test_oscillator.cpp`
- Create: `Tests/test_filter.cpp`
- Create: `Tests/test_envelope.cpp`
- Create: `Tests/test_lfo.cpp`
- Create: `Tests/test_voice.cpp`
- Modify: `Source/Synthesis/Oscillator.h`, `Source/Synthesis/Oscillator.cpp`
- Modify: `Source/Synthesis/Filter.h`, `Source/Synthesis/Filter.cpp`
- Modify: `Source/Synthesis/Envelope.h`, `Source/Synthesis/Envelope.cpp`
- Modify: `Source/Synthesis/LFO.h` (no code change needed — already framework-free; add its test only)
- Modify: `Source/Synthesis/Voice.h`, `Source/Synthesis/Voice.cpp`

**Interfaces:**
- Produces: `dsp::AudioBuffer` (`setSize`/`clear`/`getNumChannels`/`getNumSamples`/`getWritePointer`/`getReadPointer`/`applyGain`/`addFrom`/`wrapExternal`), `dsp::midiNoteToHz(int) -> float`, `dsp::dbToGain(float) -> float`, `dsp::gainToDb(float, float minDb) -> float` — consumed by every `Source/Synthesis/*` class after this task, and by `ArcanistPluginAdapter` in Task 4 (to wrap DPF's raw `float** outputs` without copying).

- [ ] **Step 1: Copy the shared test runner**

```bash
cd /home/yvan/Projects/AudioPlugins/Arcanist/.worktrees/worktree-dpf-stage0
mkdir -p Tests
git -C /home/yvan/Projects/AudioPlugins/Common show HEAD:tests/test_runner.h > Tests/test_runner.h
```

- [ ] **Step 2: Write the failing test for `dsp::AudioBuffer`**

```cpp
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
```

- [ ] **Step 3: Run to verify it fails to compile**

```bash
cd /home/yvan/Projects/AudioPlugins/Arcanist/.worktrees/worktree-dpf-stage0
g++ -std=c++20 -ITests -ISource -c Tests/test_audiobuffer.cpp -o /tmp/test_audiobuffer.o
```

Expected: FAIL — `Source/Synthesis/AudioBuffer.h` does not exist yet.

- [ ] **Step 4: Write `Source/Synthesis/AudioBuffer.h`**

```cpp
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
```

- [ ] **Step 5: Build and run, verify PASS**

```bash
g++ -std=c++20 -ITests -ISource Tests/test_audiobuffer.cpp -o /tmp/test_audiobuffer && /tmp/test_audiobuffer
```

Expected: PASS, 10/10 checks.

- [ ] **Step 6: Write the failing test for `DspUtil.h`**

```cpp
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
```

- [ ] **Step 7: Write `Source/Synthesis/DspUtil.h`**

```cpp
#pragma once
#include <cmath>

namespace dsp
{

// Replaces juce::MidiMessage::getMidiNoteInHertz(note), A4 = 440 Hz at note 69.
inline float midiNoteToHz (int midiNote)
{
    return 440.0f * std::pow (2.0f, (static_cast<float> (midiNote) - 69.0f) / 12.0f);
}

// Replaces juce::Decibels::decibelsToGain(db).
inline float dbToGain (float db)
{
    return std::pow (10.0f, db / 20.0f);
}

// Replaces juce::Decibels::gainToDecibels(gain, minusInfinityDb).
inline float gainToDb (float gain, float minDb)
{
    return gain > 0.0f ? 20.0f * std::log10 (gain) : minDb;
}

} // namespace dsp
```

- [ ] **Step 8: Build and run, verify PASS**

```bash
g++ -std=c++20 -ITests -ISource Tests/test_dsputil.cpp -o /tmp/test_dsputil && /tmp/test_dsputil
```

Expected: PASS, 8/8 checks.

- [ ] **Step 9: Write the failing test for a framework-free `Oscillator`**

```cpp
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
```

- [ ] **Step 10: Update `Oscillator.h`/`Oscillator.cpp` to drop JUCE**

`Oscillator.h`: replace `#include <juce_audio_basics/juce_audio_basics.h>` with `#include "AudioBuffer.h"`, and every `juce::AudioBuffer<float>&` parameter with `dsp::AudioBuffer&` (also `const dsp::AudioBuffer&` for the `fmMod` parameter).

`Oscillator.cpp`: replace both `float baseFreq = juce::MidiMessage::getMidiNoteInHertz (midiNote);` lines with `float baseFreq = dsp::midiNoteToHz (midiNote);` (add `#include "DspUtil.h"`), and replace `juce::jlimit (0.f, 0.5f, modFreq / sr)` with `std::clamp (modFreq / sr, 0.f, 0.5f)` (add `#include <algorithm>`). No other lines change — the sample-generation math (`sine`/`triangle`/`sawtooth`/`square`/`noise`) is already pure C++.

- [ ] **Step 11: Build and run, verify PASS**

```bash
g++ -std=c++20 -ITests -ISource Tests/test_oscillator.cpp Source/Synthesis/Oscillator.cpp -o /tmp/test_oscillator && /tmp/test_oscillator
```

Expected: PASS, 3/3 checks.

- [ ] **Step 12: Write the failing test for a framework-free `Filter`**

```cpp
// Tests/test_filter.cpp
#include "test_runner.h"
#include "../Source/Synthesis/Filter.h"
#include "../Source/Synthesis/AudioBuffer.h"

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
```

- [ ] **Step 13: Update `Filter.h`/`Filter.cpp` to drop JUCE**

`Filter.h`: replace the `juce_audio_basics` include with `#include "AudioBuffer.h"`; `process(juce::AudioBuffer<float>&)` becomes `process(dsp::AudioBuffer&)`.

`Filter.cpp`: replace both `juce::jlimit (20.f, 20000.f, cutoff)` and `juce::jlimit (0.f, 1.f, resonance)` with `std::clamp` (add `#include <algorithm>`), and `juce::jlimit (0.001f, 3.14159f, wc)` likewise.

- [ ] **Step 14: Build and run, verify PASS**

```bash
g++ -std=c++20 -ITests -ISource Tests/test_filter.cpp Source/Synthesis/Filter.cpp -o /tmp/test_filter && /tmp/test_filter
```

Expected: PASS, 1/1 check.

- [ ] **Step 15: Write the failing test for a framework-free `Envelope`**

```cpp
// Tests/test_envelope.cpp
#include "test_runner.h"
#include "../Source/Synthesis/Envelope.h"
#include "../Source/Synthesis/AudioBuffer.h"

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
```

- [ ] **Step 16: Update `Envelope.h`/`Envelope.cpp` to drop JUCE**

`Envelope.h`: replace the `juce_audio_basics` include with `#include "AudioBuffer.h"`; `process(juce::AudioBuffer<float>&)` becomes `process(dsp::AudioBuffer&)`; `setSustain`'s `juce::jlimit (0.f, 1.f, sustainLevel)` becomes `std::clamp (sustainLevel, 0.f, 1.f)` (add `#include <algorithm>`).

`Envelope.cpp`: replace the final `return juce::jlimit (0.f, 1.f, currentLevel_);` in `computeEnvelopeValue()` with `return std::clamp (currentLevel_, 0.f, 1.f);`.

- [ ] **Step 17: Build and run, verify PASS**

```bash
g++ -std=c++20 -ITests -ISource Tests/test_envelope.cpp Source/Synthesis/Envelope.cpp -o /tmp/test_envelope && /tmp/test_envelope
```

Expected: PASS, 7/7 checks.

- [ ] **Step 18: Write the test for `LFO` (already framework-free, first test it's ever had)**

```cpp
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
```

No implementation change needed here — `LFO` never included JUCE. Build and run:

```bash
g++ -std=c++20 -ITests -ISource Tests/test_lfo.cpp Source/Synthesis/LFO.cpp -o /tmp/test_lfo && /tmp/test_lfo
```

Expected: PASS, 1002/1002 checks.

- [ ] **Step 19: Write the failing test for a framework-free `Voice`**

```cpp
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
```

- [ ] **Step 20: Update `Voice.h`/`Voice.cpp` to drop JUCE**

`Voice.h`: replace the `juce_audio_basics` include with `#include "AudioBuffer.h"`; every `juce::AudioBuffer<float>` (both the two private members `voiceBuffer_`/`osc2Buffer_` and the `process(juce::AudioBuffer<float>&)` parameter) becomes `dsp::AudioBuffer`.

`Voice.cpp`: replace both `juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber)` calls in `noteOn()` with `dsp::midiNoteToHz (midiNoteNumber)` (add `#include "DspUtil.h"`); both `juce::jlimit (20.f, 20000.f, c)` / `juce::jlimit (20.f, 20000.f, cutoff)` calls become `std::clamp(...)` (add `#include <algorithm>`); `juce::Decibels::decibelsToGain (outputGain_)` becomes `dsp::dbToGain (outputGain_)`. No other lines change — the mixing/FM/AM/Ring math is already plain arithmetic on buffer pointers.

- [ ] **Step 21: Build and run, verify PASS**

```bash
g++ -std=c++20 -ITests -ISource Tests/test_voice.cpp Source/Synthesis/Voice.cpp Source/Synthesis/Oscillator.cpp Source/Synthesis/Filter.cpp Source/Synthesis/Envelope.cpp Source/Synthesis/LFO.cpp -o /tmp/test_voice && /tmp/test_voice
```

Expected: PASS, all checks green.

- [ ] **Step 22: Commit**

```bash
git add Source/Synthesis/ Tests/
git commit -m "Decouple Synthesis/* from JUCE: framework-free dsp::AudioBuffer + DspUtil, first-ever test suite

Replaces juce::AudioBuffer<float>& with a drop-in dsp::AudioBuffer
(owned or wrapExternal() non-owning view over raw pointers) across
Oscillator/Filter/Envelope/Voice, and juce::jlimit/Decibels/
MidiMessage::getMidiNoteInHertz with plain free functions in
DspUtil.h. LFO was already framework-free. Behavior-preserving: every
class's math is otherwise untouched. Arcanist had zero tests before
this; adds 6 CTest-free executables covering all 5 Synthesis classes."
```

---

## Task 3: DPF skeleton — CMake rewrite, plugin metadata, JUCE-reference move, stub adapter/UI, build-only CI

**Files:**
- Create: `cmake/apply_patch.cmake`
- Create: `cmake/patches/dpf-clap-state-chunked-read.patch`
- Create: `Source/DistrhoPluginInfo.h`
- Create: `Source/ArcanistPluginAdapter.h`/`.cpp` (stub)
- Create: `Source/ArcanistUI.h`/`.cpp` (stub)
- Move: `Source/PluginProcessor.{h,cpp}` → `Source/_juce_reference/PluginProcessor.{h,cpp}`
- Move: `Source/PluginEditor.{h,cpp}` → `Source/_juce_reference/PluginEditor.{h,cpp}`
- Move: `Source/PresetManager.{h,cpp}` → `Source/_juce_reference/PresetManager.{h,cpp}`
- Move: `Source/UI/ArcanistLookAndFeel.{h,cpp}`, `Source/UI/LedButton.h` → `Source/_juce_reference/UI/`
- Modify: `CMakeLists.txt` (full rewrite)
- Modify: `scripts/install.sh`, `scripts/uninstall.sh`
- Create: `.github/workflows/ci.yml`

**Interfaces:**
- Produces: `ArcanistPluginAdapter` (stub `Plugin` subclass) and `ArcanistUI` (stub `UI` subclass) — Task 4 fills in the adapter's parameter/MIDI/voice wiring, Task 6 fills in the UI's widgets.
- Produces: `Source/DistrhoPluginInfo.h`'s `ArcanistParameters` enum and `kArcanistParamCount` — Task 4's parameter table is indexed by this enum.

- [ ] **Step 1: Copy the DPF patch file and apply_patch.cmake from Pugilist's proven build**

```bash
cd /home/yvan/Projects/AudioPlugins/Arcanist/.worktrees/worktree-dpf-stage0
mkdir -p cmake/patches
git -C /home/yvan/Projects/AudioPlugins/Pugilist show HEAD:cmake/apply_patch.cmake > cmake/apply_patch.cmake
git -C /home/yvan/Projects/AudioPlugins/Pugilist show HEAD:cmake/patches/dpf-clap-state-chunked-read.patch > cmake/patches/dpf-clap-state-chunked-read.patch
```

Verify both are non-empty and the patch's `diff --git` header targets `distrho/src/DistrhoPluginCLAP.cpp`.

- [ ] **Step 2: Move the JUCE-era source to `Source/_juce_reference/`**

```bash
mkdir -p Source/_juce_reference/UI
git mv Source/PluginProcessor.h      Source/_juce_reference/PluginProcessor.h
git mv Source/PluginProcessor.cpp    Source/_juce_reference/PluginProcessor.cpp
git mv Source/PluginEditor.h         Source/_juce_reference/PluginEditor.h
git mv Source/PluginEditor.cpp       Source/_juce_reference/PluginEditor.cpp
git mv Source/PresetManager.h        Source/_juce_reference/PresetManager.h
git mv Source/PresetManager.cpp      Source/_juce_reference/PresetManager.cpp
git mv Source/UI/ArcanistLookAndFeel.h   Source/_juce_reference/UI/ArcanistLookAndFeel.h
git mv Source/UI/ArcanistLookAndFeel.cpp Source/_juce_reference/UI/ArcanistLookAndFeel.cpp
git mv Source/UI/LedButton.h             Source/_juce_reference/UI/LedButton.h
rmdir Source/UI 2>/dev/null || true
```

`Source/Synthesis/*` and `Source/Presets/Factory/*.xml` stay exactly where they are (already decoupled by Task 2 / consumed unchanged by Task 5).

- [ ] **Step 3: Write `Source/DistrhoPluginInfo.h`**

```cpp
/*
 * Spellbound Arcanist -- DPF plugin metadata.
 *
 * A polyphonic pad/drone synthesizer: no audio inputs, MIDI-driven, 16
 * voices. Identity values (brand, CLAP ID, 4-char codes) carried over
 * unchanged from the removed juce_add_plugin() call in the old
 * CMakeLists.txt so the plugin keeps the same public identity.
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND      "Spellbound"
#define DISTRHO_PLUGIN_NAME       "Arcanist"
#define DISTRHO_PLUGIN_URI        "https://spellbound.audio/plugins/arcanist"
#define DISTRHO_PLUGIN_CLAP_ID    "com.spellbound.arcanist"

#define DISTRHO_PLUGIN_BRAND_ID   Spbd
#define DISTRHO_PLUGIN_UNIQUE_ID  Arcn

#define DISTRHO_PLUGIN_LABEL       "Arcanist"
#define DISTRHO_PLUGIN_MAKER       "Spellbound"
// Must be a URI -- lv2lint's Plugin License test fails a plain word (see
// Hex's/Pugilist's/Tank's DistrhoPluginInfo.h for the same fix already made).
#define DISTRHO_PLUGIN_LICENSE     "https://spellbound.audio/plugins/arcanist#license"
#define DISTRHO_PLUGIN_DESCRIPTION "Soft pads and drones synthesizer"

#define DISTRHO_PLUGIN_HAS_UI           1
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_IS_SYNTH         1
#define DISTRHO_PLUGIN_NUM_INPUTS       0
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0

#define DISTRHO_UI_DEFAULT_WIDTH  1250
#define DISTRHO_UI_DEFAULT_HEIGHT 860
#define DISTRHO_UI_USER_RESIZABLE 0
#define DISTRHO_UI_USE_NANOVG     1

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
```

(Note: `DISTRHO_UI_DEFAULT_HEIGHT` grows from the JUCE editor's 760 to 860 to make room for the new presets bar and the embedded keyboard strip added in Tasks 5/6 — same reasoning Pugilist's UI followed.)

- [ ] **Step 4: Write stub `Source/ArcanistPluginAdapter.h`/`.cpp`**

```cpp
// Source/ArcanistPluginAdapter.h
#pragma once
#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class ArcanistPluginAdapter : public Plugin
{
public:
    ArcanistPluginAdapter() : Plugin(1, 0, 0) {}

protected:
    const char* getLabel() const override { return "Arcanist"; }
    const char* getDescription() const override { return "Soft pads and drones synthesizer"; }
    const char* getMaker() const override { return "Spellbound"; }
    const char* getLicense() const override { return "https://spellbound.audio/plugins/arcanist#license"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

    void initParameter(uint32_t, Parameter&) override {}
    float getParameterValue(uint32_t) const override { return 0.0f; }
    void setParameterValue(uint32_t, float) override {}

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent*, uint32_t) override
    {
        // Stub: silence. Task 4 replaces this with real voice rendering.
        std::memset(outputs[0], 0, sizeof(float) * frames);
        std::memset(outputs[1], 0, sizeof(float) * frames);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistPluginAdapter)
};

END_NAMESPACE_DISTRHO
```

```cpp
// Source/ArcanistPluginAdapter.cpp
#include "ArcanistPluginAdapter.h"
#include <cstring>

START_NAMESPACE_DISTRHO
Plugin* createPlugin() { return new ArcanistPluginAdapter(); }
END_NAMESPACE_DISTRHO
```

- [ ] **Step 5: Write stub `Source/ArcanistUI.h`/`.cpp`**

```cpp
// Source/ArcanistUI.h
#pragma once
#include "DistrhoUI.hpp"

START_NAMESPACE_DISTRHO

class ArcanistUI : public UI
{
public:
    ArcanistUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT) {}

protected:
    void parameterChanged(uint32_t, float) override {}

    void onNanoDisplay() override
    {
        beginPath();
        rect(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
        fillColor(Color(24, 27, 24, 255)); // ArcanistCol::bg() 0xFF181B18, see Task 6
        fill();
        closePath();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistUI)
};

END_NAMESPACE_DISTRHO
```

```cpp
// Source/ArcanistUI.cpp
#include "ArcanistUI.h"

START_NAMESPACE_DISTRHO
UI* createUI() { return new ArcanistUI(); }
END_NAMESPACE_DISTRHO
```

- [ ] **Step 6: Rewrite `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.22)
project(Arcanist VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# DPF has no tagged releases -- pin the exact commit SHA Hex/Pugilist/Tank/
# Outflank all use.
set(AUDIOPLUGINS_DPF_GIT_TAG "4238e1c7f0351bbe488d79f0899c540543ac7583" CACHE STRING
    "Pinned DPF commit (DISTRHO/DPF has no git tags to pin to instead)")

# CAVEAT (CMake FetchContent): the patch step only runs on a *fresh*
# population of the source dir -- delete build/_deps/dpf-* to force a
# clean re-fetch if you need to be sure the patch actually took effect.
FetchContent_Declare(dpf
    GIT_REPOSITORY https://github.com/DISTRHO/DPF.git
    GIT_TAG        ${AUDIOPLUGINS_DPF_GIT_TAG}
    GIT_SHALLOW    TRUE
    PATCH_COMMAND  ${CMAKE_COMMAND}
                   -DPATCH_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/dpf-clap-state-chunked-read.patch
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patch.cmake
)
FetchContent_MakeAvailable(dpf)

set(Arcanist_DPF_TARGETS vst3 clap lv2 jack)
if(APPLE)
    list(APPEND Arcanist_DPF_TARGETS au)
endif()

dpf_add_plugin(Arcanist
    TARGETS ${Arcanist_DPF_TARGETS}
    UI_TYPE opengl
    FILES_DSP
        Source/ArcanistPluginAdapter.cpp
        Source/Synthesis/Voice.cpp
        Source/Synthesis/Oscillator.cpp
        Source/Synthesis/Filter.cpp
        Source/Synthesis/Envelope.cpp
        Source/Synthesis/LFO.cpp
    FILES_UI
        Source/ArcanistUI.cpp
)

target_include_directories(Arcanist PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(Arcanist PUBLIC cxx_std_20)

# Same DPF WebViewImpl.cpp / GNU `typeof` workaround Hex/Pugilist/Tank need.
if(TARGET dgl-opengl)
    set_target_properties(dgl-opengl PROPERTIES CXX_EXTENSIONS ON)
endif()

# ── Shared HUI/presets library ───────────────────────────────────────────────
# ORDERING (load-bearing): must come after dpf_add_plugin(Arcanist ...) --
# DPF creates its DGL target lazily inside that call.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(AudioPluginsCommon
    GIT_REPOSITORY https://github.com/TriYop/spellbound-common.git
    GIT_TAG        v0.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(AudioPluginsCommon)

target_link_libraries(Arcanist-ui PRIVATE AudioPluginsCommon::hui_dgl AudioPluginsCommon::presets)

# ── Unit tests (no JUCE/DPF dependency) ──────────────────────────────────────
enable_testing()

add_executable(test_audiobuffer Tests/test_audiobuffer.cpp)
target_include_directories(test_audiobuffer PRIVATE Source/ Tests/)
target_compile_features(test_audiobuffer PRIVATE cxx_std_20)
add_test(NAME AudioBuffer COMMAND test_audiobuffer)

add_executable(test_dsputil Tests/test_dsputil.cpp)
target_include_directories(test_dsputil PRIVATE Source/ Tests/)
target_compile_features(test_dsputil PRIVATE cxx_std_20)
add_test(NAME DspUtil COMMAND test_dsputil)

add_executable(test_oscillator Tests/test_oscillator.cpp Source/Synthesis/Oscillator.cpp)
target_include_directories(test_oscillator PRIVATE Source/ Tests/)
target_compile_features(test_oscillator PRIVATE cxx_std_20)
add_test(NAME Oscillator COMMAND test_oscillator)

add_executable(test_filter Tests/test_filter.cpp Source/Synthesis/Filter.cpp)
target_include_directories(test_filter PRIVATE Source/ Tests/)
target_compile_features(test_filter PRIVATE cxx_std_20)
add_test(NAME Filter COMMAND test_filter)

add_executable(test_envelope Tests/test_envelope.cpp Source/Synthesis/Envelope.cpp)
target_include_directories(test_envelope PRIVATE Source/ Tests/)
target_compile_features(test_envelope PRIVATE cxx_std_20)
add_test(NAME Envelope COMMAND test_envelope)

add_executable(test_lfo Tests/test_lfo.cpp Source/Synthesis/LFO.cpp)
target_include_directories(test_lfo PRIVATE Source/ Tests/)
target_compile_features(test_lfo PRIVATE cxx_std_20)
add_test(NAME LFO COMMAND test_lfo)

add_executable(test_voice
    Tests/test_voice.cpp
    Source/Synthesis/Voice.cpp Source/Synthesis/Oscillator.cpp
    Source/Synthesis/Filter.cpp Source/Synthesis/Envelope.cpp Source/Synthesis/LFO.cpp
)
target_include_directories(test_voice PRIVATE Source/ Tests/)
target_compile_features(test_voice PRIVATE cxx_std_20)
add_test(NAME Voice COMMAND test_voice)

# Task 5 (FactoryPresets conversion) and Task 6 (RadioButtonGroup, if it
# gains its own test) each append their own blocks here once those files
# exist -- deliberately not included yet.

# ── Install rules ─────────────────────────────────────────────────────────────
set(_BIN "${CMAKE_BINARY_DIR}/bin")

install(DIRECTORY  "${_BIN}/Arcanist.vst3"
        DESTINATION VST3
        COMPONENT   Runtime
        USE_SOURCE_PERMISSIONS)

if(APPLE)
    install(DIRECTORY  "${_BIN}/Arcanist.clap"
            DESTINATION CLAP
            COMPONENT   Runtime
            USE_SOURCE_PERMISSIONS)
else()
    install(FILES      "${_BIN}/Arcanist.clap"
            DESTINATION CLAP
            COMPONENT   Runtime
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

install(DIRECTORY  "${_BIN}/Arcanist.lv2"
        DESTINATION LV2
        COMPONENT   Runtime
        USE_SOURCE_PERMISSIONS)

install(PROGRAMS   "${_BIN}/Arcanist"
        DESTINATION bin
        COMPONENT   Runtime)

install(PROGRAMS   "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install.sh"
                   "${CMAKE_CURRENT_SOURCE_DIR}/scripts/uninstall.sh"
        DESTINATION .
        COMPONENT   Runtime)

# ── CPack (TGZ) ───────────────────────────────────────────────────────────────
set(CPACK_GENERATOR                 TGZ)
set(CPACK_PACKAGE_NAME              Arcanist)
set(CPACK_PACKAGE_VENDOR            Spellbound)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Soft pads and drones synthesizer")
set(CPACK_PACKAGE_VERSION           ${PROJECT_VERSION})
set(CPACK_SYSTEM_NAME               linux-x86_64)
set(CPACK_PACKAGE_FILE_NAME         "Arcanist-${PROJECT_VERSION}-linux-x86_64")
set(CPACK_PACKAGING_INSTALL_PREFIX  "")
set(CPACK_STRIP_FILES               TRUE)
set(CPACK_PACKAGE_CHECKSUM          SHA256)
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")
include(CPack)
```

- [ ] **Step 7: Update `scripts/install.sh`/`scripts/uninstall.sh` for the DPF layout (add LV2, JACK standalone binary name unchanged)**

```bash
#!/usr/bin/env bash
# Install Arcanist plugins and standalone app (VST3/CLAP/LV2 + JACK standalone).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSTEM=0

for arg in "$@"; do
    case "$arg" in
        --system) SYSTEM=1 ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

if [[ $SYSTEM -eq 1 ]]; then
    VST3_DIR="/usr/lib/vst3"; CLAP_DIR="/usr/lib/clap"; LV2_DIR="/usr/lib/lv2"; BIN_DIR="/usr/local/bin"
else
    VST3_DIR="${HOME}/.vst3"; CLAP_DIR="${HOME}/.clap"; LV2_DIR="${HOME}/.lv2"; BIN_DIR="${HOME}/.local/bin"
fi

echo "Installing Arcanist..."
mkdir -p "${VST3_DIR}"; rm -rf "${VST3_DIR}/Arcanist.vst3"
cp -r "${SCRIPT_DIR}/VST3/Arcanist.vst3" "${VST3_DIR}/"
echo "  VST3  -> ${VST3_DIR}/Arcanist.vst3"

mkdir -p "${CLAP_DIR}"
cp "${SCRIPT_DIR}/CLAP/Arcanist.clap" "${CLAP_DIR}/"
chmod 755 "${CLAP_DIR}/Arcanist.clap"
echo "  CLAP  -> ${CLAP_DIR}/Arcanist.clap"

mkdir -p "${LV2_DIR}"; rm -rf "${LV2_DIR}/Arcanist.lv2"
cp -r "${SCRIPT_DIR}/LV2/Arcanist.lv2" "${LV2_DIR}/"
echo "  LV2   -> ${LV2_DIR}/Arcanist.lv2"

mkdir -p "${BIN_DIR}"
cp "${SCRIPT_DIR}/bin/Arcanist" "${BIN_DIR}/"
chmod 755 "${BIN_DIR}/Arcanist"
echo "  App   -> ${BIN_DIR}/Arcanist"
echo "Done."
```

Update `scripts/uninstall.sh` symmetrically (mirror Tank's `uninstall.sh` pattern shown in that plan's Task 2 Step 7, substituting `Arcanist`/adding the `~/.local/bin/Arcanist` binary removal).

- [ ] **Step 8: Write build-only CI at `.github/workflows/ci.yml`, including the `COMMON_REPO_TOKEN` step**

```yaml
name: CI

on:
  push:
    branches: [ "main" ]
    tags: [ "v*" ]
  pull_request:
    branches: [ "main" ]

jobs:
  build-and-test:
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        build_type: [Release]

    runs-on: ${{ matrix.os }}
    continue-on-error: ${{ matrix.os != 'ubuntu-latest' }}

    steps:
      - uses: actions/checkout@v4

      - name: Install Linux build dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            libasound2-dev libjack-jackd2-dev \
            libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
            libxinerama-dev libxrandr-dev libxrender-dev \
            libfreetype-dev libfontconfig1-dev \
            libglu1-mesa-dev libwebkit2gtk-4.1-dev \
            xvfb

      - name: Configure Common repo access
        # spellbound-common is private -- CI authenticates via a
        # COMMON_REPO_TOKEN fine-grained PAT (same one Hex/Pugilist/Tank/
        # Outflank already use). This secret is set on the spellbound-arcanist
        # repo itself in this same task (see Step 9 below) -- do NOT skip
        # verifying it's present before relying on a green run.
        run: git config --global url."https://x-access-token:${{ secrets.COMMON_REPO_TOKEN }}@github.com/TriYop/spellbound-common".insteadOf "https://github.com/TriYop/spellbound-common"

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

      - name: Build
        run: cmake --build build --config ${{ matrix.build_type }} --parallel

      - name: Test
        working-directory: build
        run: ctest --build-config ${{ matrix.build_type }} --output-on-failure
```

Do **not** add pluginval/clap-validator/lv2lint yet — Task 7 does that once there's real DSP/UI to validate.

- [ ] **Step 9: Set the `COMMON_REPO_TOKEN` secret and verify it — do this now, not at the end**

```bash
gh secret set COMMON_REPO_TOKEN --repo TriYop/spellbound-arcanist
# (paste the same fine-grained PAT already used for spellbound-hex/spellbound-pugilist/spellbound-tank/spellbound-outflank)

gh secret list --repo TriYop/spellbound-arcanist
```

Expected: `COMMON_REPO_TOKEN` appears in the listing. This is the exact step that was missed on Tank's and Outflank's PRs and broke their CI for a day each — do not defer it to Task 9.

- [ ] **Step 10: Configure and build to verify the skeleton compiles**

```bash
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
```

Expected: fetches DPF + `Common` `v0.4.0`, builds `Arcanist.vst3`/`Arcanist.clap`/`Arcanist.lv2` plus the JACK standalone under `build/bin/`, plus the 7 Task 2 test executables. Stub `ArcanistPluginAdapter`/`ArcanistUI` compile and link.

- [ ] **Step 11: Run tests to confirm Task 2's suite still passes after the CMake rewrite**

```bash
ctest --test-dir build --output-on-failure
```

Expected: 7/7 pass (AudioBuffer, DspUtil, Oscillator, Filter, Envelope, LFO, Voice).

- [ ] **Step 12: Commit**

```bash
git add cmake/ Source/ scripts/ CMakeLists.txt .github/workflows/ci.yml
git commit -m "Migrate Arcanist's build off JUCE onto DPF (skeleton)

Rewrite CMakeLists.txt to fetch DPF (pinned commit + chunked-read-only
patch, no latency patch needed) and AudioPlugins/Common v0.4.0 instead
of JUCE + clap-juce-extensions. Move the JUCE-era PluginProcessor/
PluginEditor/PresetManager/ArcanistLookAndFeel/LedButton to
Source/_juce_reference/ as the porting reference; stub
ArcanistPluginAdapter/ArcanistUI as silent placeholders. Build-only CI
(Linux gating, Windows/macOS continue-on-error) with COMMON_REPO_TOKEN
configured up front, per the Phase 2 lesson."
```

---

## Task 4: Wire the voice pool, MIDI note-on/off/steal, and all 43 parameters into ArcanistPluginAdapter

**Files:**
- Modify: `Source/DistrhoPluginInfo.h` (append the parameter enum)
- Create: `Source/ArcanistParams.h` (the parameter spec table)
- Modify: `Source/ArcanistPluginAdapter.h`/`.cpp` (replace the Task 3 stub)

**Interfaces:**
- Consumes: `dsp::AudioBuffer::wrapExternal()` (Task 2), `Voice` (Task 2, unchanged public API), `dsp::midiNoteToHz`/`dsp::dbToGain` (Task 2, used internally by `Voice`/`Oscillator`, not directly here).
- Produces: `ArcanistParameters` enum (`Source/DistrhoPluginInfo.h`) and `kArcanistParamSpecs[]` (`Source/ArcanistParams.h`) — Task 5 (presets) and Task 6 (UI) both index parameters by this same enum and read `kArcanistParamSpecs[i].id` for preset-file id matching.

- [ ] **Step 1: Append the parameter enum to `Source/DistrhoPluginInfo.h`**

Transcribed 1:1, in the same order, from `Source/_juce_reference/PluginProcessor.cpp::createParameterLayout()`:

```cpp
// Host parameter indices, shared between ArcanistPluginAdapter and
// ArcanistUI. Order matches the JUCE-era createParameterLayout() exactly.
enum ArcanistParameters {
    kParamOscWaveform = 0, kParamOscTune, kParamOscDetune,
    kParamFilterMode, kParamFilterCutoff, kParamFilterResonance,
    kParamEnvAttack, kParamEnvDecay, kParamEnvSustain, kParamEnvRelease,
    kParamEnvFilterMod, kParamEnvSustainOn,
    kParamFEnvAttack, kParamFEnvDecay, kParamFEnvSustain, kParamFEnvRelease, kParamFEnvSustainOn,
    kParamLfoTarget, kParamLfoSpeed, kParamLfoDepth,
    kParamOutputGain,
    kParamOsc2On, kParamOsc2Waveform, kParamOsc2Mult, kParamOsc2Phase,
    kParamOsc2MixMode, kParamOsc2MixDepth,
    kParamOsc2EnvOn, kParamOsc2EnvAttack, kParamOsc2EnvDecay, kParamOsc2EnvSustain, kParamOsc2EnvRelease, kParamOsc2EnvSustainOn,
    kParamOsc2FltOn, kParamOsc2FltCutoff, kParamOsc2FltResonance, kParamOsc2FltMode,
    kParamOsc2FEnvAttack, kParamOsc2FEnvDecay, kParamOsc2FEnvSustain, kParamOsc2FEnvRelease, kParamOsc2FEnvSustainOn,
    kParamOsc2FEnvDepth,
    kArcanistParamCount // 43
};
```

- [ ] **Step 2: Write `Source/ArcanistParams.h` — the data-driven parameter spec table**

A table-driven design (one row per parameter: id string for preset matching, display name, kind, range, default) replaces 43 hand-written `initParameter`/`getParameterValue`/`setParameterValue` switch cases with one generic loop plus a plain `std::array<float, kArcanistParamCount>` value store — every value (id, min, max, default) transcribed verbatim from `createParameterLayout()`:

```cpp
#pragma once
#include "DistrhoPluginInfo.h"

// One row per ArcanistParameters entry, in the same order. `kind` decides
// how ArcanistUI represents the parameter (RotaryKnob/ToggleSwitch/
// RadioButtonGroup, see Task 6) -- the DPF host side treats every
// parameter as a plain float regardless of kind, exactly like the JUCE-era
// APVTS did via getRawParameterValue().
enum class ArcanistParamKind { Continuous, Boolean, Choice };

struct ArcanistParamSpec
{
    const char* id;      // matches the JUCE-era APVTS parameter id AND the
                          // "id" attribute in Source/Presets/Factory/*.xml
    const char* name;
    ArcanistParamKind kind;
    float min, max, defaultValue;
    int numChoices;       // only meaningful when kind == Choice
};

inline const ArcanistParamSpec kArcanistParamSpecs[kArcanistParamCount] = {
    /* kParamOscWaveform   */ { "osc_waveform",      "Waveform",              ArcanistParamKind::Choice,     0.f,     3.f,     0.f, 4 },
    /* kParamOscTune       */ { "osc_tune",          "Oscillator Tune",       ArcanistParamKind::Continuous, -24.f,   24.f,    0.f, 0 },
    /* kParamOscDetune     */ { "osc_detune",        "Oscillator Detune",     ArcanistParamKind::Continuous, -50.f,   50.f,    0.f, 0 },
    /* kParamFilterMode    */ { "filter_mode",       "Filter Mode",           ArcanistParamKind::Choice,     0.f,     2.f,     0.f, 3 },
    /* kParamFilterCutoff  */ { "filter_cutoff",     "Filter Cutoff",         ArcanistParamKind::Continuous, 20.f,    20000.f, 4000.f, 0 },
    /* kParamFilterReso    */ { "filter_resonance",  "Filter Resonance",      ArcanistParamKind::Continuous, 0.f,     1.f,     0.f, 0 },
    /* kParamEnvAttack     */ { "env_attack",        "Attack Time",           ArcanistParamKind::Continuous, 0.001f,  5.f,     0.1f, 0 },
    /* kParamEnvDecay      */ { "env_decay",         "Decay Time",            ArcanistParamKind::Continuous, 0.f,     5.f,     0.5f, 0 },
    /* kParamEnvSustain    */ { "env_sustain",       "Sustain Level",         ArcanistParamKind::Continuous, 0.f,     1.f,     0.8f, 0 },
    /* kParamEnvRelease    */ { "env_release",       "Release Time",          ArcanistParamKind::Continuous, 0.5f,    10.f,    3.f, 0 },
    /* kParamEnvFilterMod  */ { "env_filter_mod",    "Filter Env Mod",        ArcanistParamKind::Continuous, -100.f,  100.f,   0.f, 0 },
    /* kParamEnvSustainOn  */ { "env_sustain_on",    "Amp Sustain On",        ArcanistParamKind::Boolean,    0.f,     1.f,     1.f, 0 },
    /* kParamFEnvAttack    */ { "fenv_attack",       "Filter Env Attack",     ArcanistParamKind::Continuous, 0.001f,  5.f,     0.001f, 0 },
    /* kParamFEnvDecay     */ { "fenv_decay",        "Filter Env Decay",      ArcanistParamKind::Continuous, 0.f,     5.f,     1.5f, 0 },
    /* kParamFEnvSustain   */ { "fenv_sustain",      "Filter Env Sustain",    ArcanistParamKind::Continuous, 0.f,     1.f,     0.f, 0 },
    /* kParamFEnvRelease   */ { "fenv_release",      "Filter Env Release",    ArcanistParamKind::Continuous, 0.001f,  10.f,    3.f, 0 },
    /* kParamFEnvSustainOn */ { "fenv_sustain_on",   "Filter Sustain On",     ArcanistParamKind::Boolean,    0.f,     1.f,     1.f, 0 },
    /* kParamLfoTarget     */ { "lfo_target",        "LFO Target",            ArcanistParamKind::Choice,     0.f,     2.f,     0.f, 3 },
    /* kParamLfoSpeed      */ { "lfo_speed",         "LFO Speed",             ArcanistParamKind::Continuous, 0.1f,    10.f,    0.5f, 0 },
    /* kParamLfoDepth      */ { "lfo_depth",         "LFO Depth",             ArcanistParamKind::Continuous, 0.f,     100.f,   0.f, 0 },
    /* kParamOutputGain    */ { "output_gain",       "Output Gain",          ArcanistParamKind::Continuous, -24.f,   12.f,    0.f, 0 },
    /* kParamOsc2On        */ { "osc2_on",           "Osc 2 On",             ArcanistParamKind::Boolean,    0.f,     1.f,     0.f, 0 },
    /* kParamOsc2Waveform  */ { "osc2_waveform",     "Osc 2 Waveform",       ArcanistParamKind::Choice,     0.f,     4.f,     2.f, 5 },
    /* kParamOsc2Mult      */ { "osc2_mult",         "Osc 2 Multiplier",     ArcanistParamKind::Choice,     0.f,     3.f,     1.f, 4 },
    /* kParamOsc2Phase     */ { "osc2_phase",        "Osc 2 Phase",          ArcanistParamKind::Continuous, 0.f,     360.f,   0.f, 0 },
    /* kParamOsc2MixMode   */ { "osc2_mix_mode",     "Osc 2 Mix Mode",       ArcanistParamKind::Choice,     0.f,     3.f,     0.f, 4 },
    /* kParamOsc2MixDepth  */ { "osc2_mix_depth",    "Osc 2 Mix Depth",      ArcanistParamKind::Continuous, 0.f,     100.f,   50.f, 0 },
    /* kParamOsc2EnvOn     */ { "osc2_env_on",       "Osc 2 Env On",         ArcanistParamKind::Boolean,    0.f,     1.f,     0.f, 0 },
    /* kParamOsc2EnvAttack */ { "osc2_env_attack",   "Osc 2 Env Attack",     ArcanistParamKind::Continuous, 0.001f,  5.f,     0.1f, 0 },
    /* kParamOsc2EnvDecay  */ { "osc2_env_decay",    "Osc 2 Env Decay",      ArcanistParamKind::Continuous, 0.f,     5.f,     0.5f, 0 },
    /* kParamOsc2EnvSustain*/ { "osc2_env_sustain",  "Osc 2 Env Sustain",    ArcanistParamKind::Continuous, 0.f,     1.f,     0.8f, 0 },
    /* kParamOsc2EnvRelease*/ { "osc2_env_release",  "Osc 2 Env Release",    ArcanistParamKind::Continuous, 0.001f,  10.f,    3.f, 0 },
    /* kParamOsc2EnvSusOn  */ { "osc2_env_sus_on",   "Osc 2 Env Sustain On", ArcanistParamKind::Boolean,    0.f,     1.f,     1.f, 0 },
    /* kParamOsc2FltOn     */ { "osc2_flt_on",       "Osc 2 Filter On",      ArcanistParamKind::Boolean,    0.f,     1.f,     0.f, 0 },
    /* kParamOsc2FltCutoff */ { "osc2_flt_cutoff",   "Osc 2 Filter Cutoff",  ArcanistParamKind::Continuous, 20.f,    20000.f, 4000.f, 0 },
    /* kParamOsc2FltReso   */ { "osc2_flt_reso",     "Osc 2 Filter Reso",    ArcanistParamKind::Continuous, 0.f,     1.f,     0.f, 0 },
    /* kParamOsc2FltMode   */ { "osc2_flt_mode",     "Osc 2 Filter Mode",    ArcanistParamKind::Choice,     0.f,     2.f,     0.f, 3 },
    /* kParamOsc2FEnvAttack*/ { "osc2_fenv_atk",     "Osc 2 FEnv Attack",    ArcanistParamKind::Continuous, 0.001f,  5.f,     0.001f, 0 },
    /* kParamOsc2FEnvDecay */ { "osc2_fenv_dec",     "Osc 2 FEnv Decay",     ArcanistParamKind::Continuous, 0.f,     5.f,     1.5f, 0 },
    /* kParamOsc2FEnvSustain*/{ "osc2_fenv_sus",     "Osc 2 FEnv Sustain",   ArcanistParamKind::Continuous, 0.f,     1.f,     0.f, 0 },
    /* kParamOsc2FEnvRelease*/{ "osc2_fenv_rel",     "Osc 2 FEnv Release",   ArcanistParamKind::Continuous, 0.001f,  10.f,    3.f, 0 },
    /* kParamOsc2FEnvSusOn */ { "osc2_fenv_sus_on",  "Osc 2 FEnv Sustain On",ArcanistParamKind::Boolean,    0.f,     1.f,     1.f, 0 },
    /* kParamOsc2FEnvDepth */ { "osc2_fenv_depth",   "Osc 2 FEnv Depth",     ArcanistParamKind::Continuous, -100.f,  100.f,   0.f, 0 },
};
```

- [ ] **Step 3: Write `Source/ArcanistPluginAdapter.h`**

```cpp
#pragma once
#include "DistrhoPlugin.hpp"
#include "ArcanistParams.h"
#include "Synthesis/Voice.h"
#include "Synthesis/AudioBuffer.h"
#include <array>
#include <vector>

START_NAMESPACE_DISTRHO

/**
   DPF Plugin adapter for Spellbound Arcanist.

   Thin shim over a 16-voice pool of the existing framework-free Voice
   (Source/Synthesis/Voice.h): reads all 43 host parameters generically via
   kArcanistParamSpecs (Task 4 Step 2), applies them to every voice once per
   block (applyParametersToVoice(), ported verbatim from the JUCE-era
   PluginProcessor::processBlock()'s per-voice setter calls), dispatches
   DPF's MidiEvent array through the same same-note-reuse / free-voice /
   oldest-voice-steal algorithm the JUCE version used, and wraps DPF's raw
   float** run() output in a non-owning dsp::AudioBuffer (see
   AudioBuffer::wrapExternal(), Task 2) so Voice::process() needs no
   modification at all.
 */
class ArcanistPluginAdapter : public Plugin
{
public:
    ArcanistPluginAdapter();

protected:
    const char* getLabel() const override;
    const char* getDescription() const override;
    const char* getMaker() const override;
    const char* getLicense() const override;
    uint32_t getVersion() const override;

    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;

    void activate() override;
    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override;

private:
    void applyParametersToVoice(Voice& voice) const;
    void handleMidiEvent(const MidiEvent& ev);
    void handleNoteOn(int note, float velocity);
    void handleNoteOff(int note);

    std::array<float, kArcanistParamCount> values_;
    std::vector<Voice> voices_; // 16, matches JUCE-era polyphony

    // 0.1 s output ramp applied on a full-state reload (preset change via
    // ArcanistUI, see Task 5) -- ported from the JUCE-era
    // fadeOutSamplesTotal_/fadeOutSamplesRemaining_/allNotesOffPending.
    bool allNotesOffPending_ = false;
    int fadeOutSamplesTotal_ = 0;
    int fadeOutSamplesRemaining_ = 0;

public:
    // Called by ArcanistUI (Task 5) right after applying a preset's values,
    // so the audio thread starts the same silence-then-retrigger ramp the
    // JUCE-era PresetManager triggered via setCurrentProgram().
    void requestAllNotesOff() { allNotesOffPending_ = true; }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistPluginAdapter)
};

END_NAMESPACE_DISTRHO
```

- [ ] **Step 4: Write `Source/ArcanistPluginAdapter.cpp`**

```cpp
#include "ArcanistPluginAdapter.h"
#include <algorithm>
#include <cstring>

START_NAMESPACE_DISTRHO

ArcanistPluginAdapter::ArcanistPluginAdapter()
    : Plugin(kArcanistParamCount, 0, 0)
{
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
        values_[i] = kArcanistParamSpecs[i].defaultValue;
}

const char* ArcanistPluginAdapter::getLabel() const { return "Arcanist"; }
const char* ArcanistPluginAdapter::getDescription() const { return "Soft pads and drones synthesizer"; }
const char* ArcanistPluginAdapter::getMaker() const { return "Spellbound"; }
const char* ArcanistPluginAdapter::getLicense() const { return "https://spellbound.audio/plugins/arcanist#license"; }
uint32_t ArcanistPluginAdapter::getVersion() const { return d_version(0, 1, 0); }

void ArcanistPluginAdapter::initParameter(const uint32_t index, Parameter& parameter)
{
    if (index >= kArcanistParamCount) return;
    const auto& spec = kArcanistParamSpecs[index];

    parameter.hints = kParameterIsAutomatable;
    if (spec.kind == ArcanistParamKind::Boolean) parameter.hints |= kParameterIsBoolean;
    if (spec.kind == ArcanistParamKind::Choice)  parameter.hints |= kParameterIsInteger;

    parameter.name   = spec.name;
    parameter.symbol = spec.id;
    parameter.ranges.min = spec.min;
    parameter.ranges.max = spec.max;
    parameter.ranges.def = spec.defaultValue;
}

float ArcanistPluginAdapter::getParameterValue(const uint32_t index) const
{
    return index < kArcanistParamCount ? values_[index] : 0.0f;
}

void ArcanistPluginAdapter::setParameterValue(const uint32_t index, const float value)
{
    if (index < kArcanistParamCount) values_[index] = value;
}

void ArcanistPluginAdapter::activate()
{
    voices_.clear();
    voices_.resize(16);
    for (auto& v : voices_)
        v.prepare(getSampleRate(), static_cast<int>(getBufferSize()), 3.0);
}

void ArcanistPluginAdapter::applyParametersToVoice(Voice& voice) const
{
    // Ported verbatim from Source/_juce_reference/PluginProcessor.cpp's
    // processBlock() per-voice setter block, indexed by ArcanistParameters
    // instead of apvts.getRawParameterValue(id).
    voice.setWaveform(static_cast<Oscillator::Waveform>(static_cast<int>(values_[kParamOscWaveform])));
    voice.setOscillatorTune(values_[kParamOscTune]);
    voice.setOscillatorDetune(values_[kParamOscDetune]);
    voice.setFilterMode(static_cast<Filter::Mode>(static_cast<int>(values_[kParamFilterMode])));
    voice.setFilterCutoff(values_[kParamFilterCutoff]);
    voice.setFilterResonance(values_[kParamFilterResonance]);
    voice.setEnvelopeAttack(values_[kParamEnvAttack]);
    voice.setEnvelopeDecay(values_[kParamEnvDecay]);
    voice.setEnvelopeSustain(values_[kParamEnvSustain]);
    voice.setEnvelopeRelease(values_[kParamEnvRelease]);
    voice.setEnvelopeFilterMod(values_[kParamEnvFilterMod]);
    voice.setEnvelopeSustainEnabled(values_[kParamEnvSustainOn] > 0.5f);
    voice.setFEnvAttack(values_[kParamFEnvAttack]);
    voice.setFEnvDecay(values_[kParamFEnvDecay]);
    voice.setFEnvSustain(values_[kParamFEnvSustain]);
    voice.setFEnvRelease(values_[kParamFEnvRelease]);
    voice.setFEnvSustainEnabled(values_[kParamFEnvSustainOn] > 0.5f);
    voice.setLFOTarget(static_cast<int>(values_[kParamLfoTarget]));
    voice.setLFOSpeed(values_[kParamLfoSpeed]);
    voice.setLFODepth(values_[kParamLfoDepth]);
    voice.setOutputGain(values_[kParamOutputGain]);

    voice.setOsc2Enabled(values_[kParamOsc2On] > 0.5f);
    voice.setOsc2Waveform(static_cast<Oscillator::Waveform>(static_cast<int>(values_[kParamOsc2Waveform])));
    voice.setOsc2Mult(static_cast<int>(values_[kParamOsc2Mult]));
    voice.setOsc2Phase(values_[kParamOsc2Phase]);
    voice.setOsc2MixMode(static_cast<Voice::MixMode>(static_cast<int>(values_[kParamOsc2MixMode])));
    voice.setOsc2MixDepth(values_[kParamOsc2MixDepth]);
    voice.setOsc2EnvEnabled(values_[kParamOsc2EnvOn] > 0.5f);
    voice.setOsc2EnvAttack(values_[kParamOsc2EnvAttack]);
    voice.setOsc2EnvDecay(values_[kParamOsc2EnvDecay]);
    voice.setOsc2EnvSustain(values_[kParamOsc2EnvSustain]);
    voice.setOsc2EnvRelease(values_[kParamOsc2EnvRelease]);
    voice.setOsc2EnvSustainEnabled(values_[kParamOsc2EnvSusOn] > 0.5f);
    voice.setOsc2FilterEnabled(values_[kParamOsc2FltOn] > 0.5f);
    voice.setOsc2FilterCutoff(values_[kParamOsc2FltCutoff]);
    voice.setOsc2FilterResonance(values_[kParamOsc2FltResonance]);
    voice.setOsc2FilterMode(static_cast<Filter::Mode>(static_cast<int>(values_[kParamOsc2FltMode])));
    voice.setOsc2FEnvAttack(values_[kParamOsc2FEnvAttack]);
    voice.setOsc2FEnvDecay(values_[kParamOsc2FEnvDecay]);
    voice.setOsc2FEnvSustain(values_[kParamOsc2FEnvSustain]);
    voice.setOsc2FEnvRelease(values_[kParamOsc2FEnvRelease]);
    voice.setOsc2FEnvSustainEnabled(values_[kParamOsc2FEnvSusOn] > 0.5f);
    voice.setOsc2FEnvDepth(values_[kParamOsc2FEnvDepth]);
}

void ArcanistPluginAdapter::handleNoteOn(const int note, const float velocity)
{
    // Ported verbatim from PluginProcessor.cpp's MIDI loop: reuse the same
    // note's voice if it's still ringing (cuts the release tail cleanly),
    // else any free voice, else steal the oldest active voice.
    int sameNoteVoice = -1, freeVoice = -1, oldestVoice = -1;
    float oldestTime = -1.0f;

    for (size_t i = 0; i < voices_.size(); ++i)
    {
        if (voices_[i].isActive() && voices_[i].getNoteNumber() == note) { sameNoteVoice = static_cast<int>(i); break; }
        if (!voices_[i].isActive() && freeVoice < 0) freeVoice = static_cast<int>(i);
        if (voices_[i].isActive() && voices_[i].getTimeSinceNoteOn() > oldestTime)
        {
            oldestTime = voices_[i].getTimeSinceNoteOn();
            oldestVoice = static_cast<int>(i);
        }
    }

    const int voiceIndex = sameNoteVoice >= 0 ? sameNoteVoice : (freeVoice >= 0 ? freeVoice : oldestVoice);
    if (voiceIndex >= 0)
        voices_[static_cast<size_t>(voiceIndex)].noteOn(note, velocity);
}

void ArcanistPluginAdapter::handleNoteOff(const int note)
{
    for (auto& voice : voices_)
        if (voice.getNoteNumber() == note)
            voice.noteOff();
}

void ArcanistPluginAdapter::handleMidiEvent(const MidiEvent& ev)
{
    if (ev.size < 3) return;
    const uint8_t status = ev.data[0] & 0xF0;
    const uint8_t note = ev.data[1];
    const uint8_t velocity = ev.data[2];

    if (status == 0x90 && velocity > 0) handleNoteOn(note, static_cast<float>(velocity) / 127.0f);
    else if (status == 0x80 || (status == 0x90 && velocity == 0)) handleNoteOff(note);
}

void ArcanistPluginAdapter::run(const float**, float** outputs, const uint32_t frames,
                                  const MidiEvent* midiEvents, const uint32_t midiEventCount)
{
    // On a full preset reload (Task 5's ArcanistUI::applyPreset() calls
    // requestAllNotesOff()): silence every voice's tail over 0.1 s, then
    // hard-kill, matching the JUCE-era PluginProcessor's ramp exactly.
    if (allNotesOffPending_)
    {
        allNotesOffPending_ = false;
        for (auto& voice : voices_) voice.noteOff();
        fadeOutSamplesTotal_ = static_cast<int>(getSampleRate() * 0.1);
        fadeOutSamplesRemaining_ = fadeOutSamplesTotal_;
    }

    for (auto& voice : voices_) applyParametersToVoice(voice);

    for (uint32_t i = 0; i < midiEventCount; ++i) handleMidiEvent(midiEvents[i]);

    dsp::AudioBuffer out;
    float* channels[2] = { outputs[0], outputs[1] };
    out.wrapExternal(channels, 2, static_cast<int>(frames));
    out.clear();

    for (auto& voice : voices_) voice.process(out);

    if (fadeOutSamplesRemaining_ > 0)
    {
        const float totalF = static_cast<float>(fadeOutSamplesTotal_);
        for (uint32_t n = 0; n < frames; ++n)
        {
            const float gain = fadeOutSamplesRemaining_ > 0
                              ? static_cast<float>(fadeOutSamplesRemaining_--) / totalF : 0.0f;
            outputs[0][n] *= gain;
            outputs[1][n] *= gain;
        }
        if (fadeOutSamplesRemaining_ == 0)
            for (auto& voice : voices_) voice.forceStop();
    }
}

Plugin* createPlugin() { return new ArcanistPluginAdapter(); }

END_NAMESPACE_DISTRHO
```

- [ ] **Step 5: Build**

```bash
cmake --build build --parallel
```

Expected: links successfully; `Voice`/`Oscillator`/`Filter`/`Envelope`/`LFO` from Task 2 are already in `FILES_DSP` (Task 3 Step 6).

- [ ] **Step 6: Run the full test suite to confirm nothing regressed**

```bash
ctest --test-dir build --output-on-failure
```

Expected: 7/7 pass, unchanged from Task 3.

- [ ] **Step 7: Manual smoke test in a real host**

```bash
carla-single vst3 build/bin/Arcanist.vst3 &
```

Play a MIDI chord via Carla's virtual keyboard or a connected MIDI device; confirm audible sustained sound, multiple simultaneous notes ring independently, and releasing a note lets it fade rather than cutting instantly (default `env_release` = 3 s). Play more than 16 notes at once and confirm the oldest note's voice gets stolen (its tail cuts) rather than notes simply failing to sound.

- [ ] **Step 8: Commit**

```bash
git add Source/DistrhoPluginInfo.h Source/ArcanistParams.h Source/ArcanistPluginAdapter.h Source/ArcanistPluginAdapter.cpp
git commit -m "Wire 16-voice pool, MIDI note-on/off/steal, and all 43 parameters into ArcanistPluginAdapter

Data-driven ArcanistParamSpec table (id/name/kind/range/default per
parameter, transcribed from the JUCE-era createParameterLayout())
replaces 43 hand-written parameter switch cases. applyParametersToVoice()
and the note-on/off/steal algorithm are direct ports of
PluginProcessor.cpp's processBlock(), unchanged in logic. Output is
wrapped via dsp::AudioBuffer::wrapExternal() so Voice::process() needed
no DPF-specific changes at all."
```

---

## Task 5: Port the preset library onto Common's PresetBrowser/PresetIO

Arcanist's existing preset XML (`Source/Presets/Factory/*.xml`, 42 files: `<Parameters><PARAM id="…" value="…"/></Parameters>`) is literally `juce::AudioProcessorValueTreeState`'s native `ValueTree::createXml()` output — not a bespoke format. `Common`'s `Preset`/`PresetIO` uses a different, deliberately generic schema (`<AudioPluginsPreset name="…" pluginId="…" version="…"><Parameter id="…" value="…"/></AudioPluginsPreset>`, see `Common/CLAUDE.md`'s design notes). This task converts the 42 files, not the underlying preset *data* — every `id`/`value` pair carries over unchanged.

**Files:**
- Create: `tools/convert_presets_to_common_schema.py`
- Create: `Source/Presets/Factory/*.xml` (42 files, rewritten in place to the new schema — same filenames)
- Create: `Source/FactoryPresets.h`
- Create: `Tests/test_factorypresets.cpp`
- Modify: `Source/ArcanistUI.h`/`.cpp` (Task 6 adds the actual widgets; this task adds the `PresetBrowser` plumbing they call into)
- Modify: `CMakeLists.txt` (append `test_factorypresets`)

**Interfaces:**
- Consumes: `audioplugins::common::presets::Preset`/`ParameterValue`/`PresetBrowser` (`Common`), `kArcanistParamSpecs[]` (Task 4, for id validation).
- Produces: `arcanistFactoryPresets() -> std::vector<audioplugins::common::presets::Preset>`, consumed by `ArcanistUI` in Task 6.

- [ ] **Step 1: Write the conversion script**

```python
#!/usr/bin/env python3
"""Convert Arcanist's factory preset XMLs from the JUCE-era APVTS ValueTree
schema (<Parameters><PARAM id=".." value=".."/></Parameters>) to
AudioPlugins/Common's generic Preset schema
(<AudioPluginsPreset name=".." pluginId=".." version=".."><Parameter id=".." value=".."/></AudioPluginsPreset>).
Data (every id/value pair) is preserved exactly; only the wrapper tags and
attribute names change. Run once, in place, then commit the rewritten files."""
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

PRESETS_DIR = Path(__file__).resolve().parent.parent / "Source" / "Presets" / "Factory"
PLUGIN_ID = "com.spellbound.arcanist"

def preset_name_from_filename(path: Path) -> str:
    # "042_DXBass1.xml" -> "DX Bass 1" is NOT recoverable from the filename
    # alone (case/spacing lost) -- names are supplied by the caller via the
    # NAME_MAP below, sourced from PresetManager.cpp's factoryDefs table.
    raise NotImplementedError

NAME_MAP = {
    "001_CelestialDrift.xml": "Celestial Drift", "002_HeavenlyGlow.xml": "Heavenly Glow",
    "003_CloudNine.xml": "Cloud Nine", "004_Starlight.xml": "Starlight",
    "005_Nebula.xml": "Nebula", "006_InfiniteSpace.xml": "Infinite Space",
    "007_MidnightCathedral.xml": "Midnight Cathedral", "008_OminousSwell.xml": "Ominous Swell",
    "009_CinematicStrings.xml": "Cinematic Strings", "010_DeepMystery.xml": "Deep Mystery",
    "011_ShadowedDream.xml": "Shadowed Dream", "012_RichStrings.xml": "Rich Strings",
    "013_VelvetWarmth.xml": "Velvet Warmth", "014_ChoirShimmer.xml": "Choir Shimmer",
    "015_GoldenHour.xml": "Golden Hour", "016_OrganicBloom.xml": "Organic Bloom",
    "017_CrystalBell.xml": "Crystal Bell", "018_SparklingSunrise.xml": "Sparkling Sunrise",
    "019_LuminousPad.xml": "Luminous Pad", "020_PrismaticGlow.xml": "Prismatic Glow",
    "021_MorphingLandscape.xml": "Morphing Landscape", "022_BreathingPulse.xml": "Breathing Pulse",
    "023_SpiralingCosmos.xml": "Spiraling Cosmos", "024_VintageSynthPad.xml": "Vintage Synth Pad",
    "025_BladeRunnerNight.xml": "Blade Runner Night", "026_AlphaCentauri.xml": "Alpha Centauri",
    "027_CS80Strings.xml": "CS80 Strings", "028_EpicCinema.xml": "Epic Cinema",
    "029_StringMachine.xml": "String Machine", "030_OceanicSweep.xml": "Oceanic Sweep",
    "031_CosmicHorizon.xml": "Cosmic Horizon", "032_ElectricNight.xml": "Electric Night",
    "033_LaserHarp.xml": "Laser Harp", "034_GMFantasia.xml": "GM Fantasia",
    "035_GMCrystal.xml": "GM Crystal", "036_GMAtmosphere.xml": "GM Atmosphere",
    "037_GMBrightness.xml": "GM Brightness", "038_GMEchoes.xml": "GM Echoes",
    "039_GMSciFi.xml": "GM Sci-Fi", "040_GMVoice.xml": "GM Voice",
    "041_GMStrings.xml": "GM Strings", "042_DXBass1.xml": "DX Bass 1",
}

def convert(path: Path) -> None:
    tree = ET.parse(path)
    root = tree.getroot()
    assert root.tag == "Parameters", f"{path}: unexpected root tag {root.tag!r}"

    name = NAME_MAP[path.name]
    new_root = ET.Element("AudioPluginsPreset", {"name": name, "pluginId": PLUGIN_ID, "version": "1"})
    for param in root.findall("PARAM"):
        ET.SubElement(new_root, "Parameter", {"id": param.get("id"), "value": param.get("value")})

    ET.indent(new_root, space="  ")
    new_tree = ET.ElementTree(new_root)
    new_tree.write(path, encoding="UTF-8", xml_declaration=True)

def main() -> int:
    files = sorted(PRESETS_DIR.glob("*.xml"))
    if len(files) != 42:
        print(f"expected 42 factory preset files, found {len(files)}", file=sys.stderr)
        return 1
    for f in files:
        convert(f)
        print(f"converted {f.name}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Run the script and verify the conversion**

```bash
cd /home/yvan/Projects/AudioPlugins/Arcanist/.worktrees/worktree-dpf-stage0
python3 tools/convert_presets_to_common_schema.py
git diff --stat Source/Presets/Factory/ | tail -1   # expect "42 files changed"
cat Source/Presets/Factory/042_DXBass1.xml           # spot-check DX Bass 1's PARAM values are unchanged
```

Expected: 42 files changed; `042_DXBass1.xml` now has an `<AudioPluginsPreset name="DX Bass 1" pluginId="com.spellbound.arcanist" version="1">` root with the same `osc_waveform=2`/`filter_cutoff=1400`/etc. values from the original, now as `<Parameter id="…" value="…"/>` children. Diff every other file too — spot-check at least 3 more (e.g. `001_CelestialDrift.xml`, a "GM" one, a "Vintage" one) to confirm no data was silently dropped or reordered.

- [ ] **Step 3: Write the failing test for `arcanistFactoryPresets()`**

```cpp
// Tests/test_factorypresets.cpp
#include "test_runner.h"
#include "../Source/FactoryPresets.h"
#include "../Source/ArcanistParams.h"

int main()
{
    const auto presets = arcanistFactoryPresets();

    CHECK_MSG (presets.size() == 42, "expected all 42 ported factory presets");

    bool foundDxBass1 = false;
    for (const auto& p : presets)
    {
        CHECK_MSG (p.pluginId == "com.spellbound.arcanist", "every preset must stamp Arcanist's plugin id");
        CHECK_MSG (!p.name.empty(), "every preset must have a name");
        CHECK_MSG (!p.parameters.empty(), "every preset must set at least one parameter");

        // Every parameter id in every preset must be a real Arcanist parameter.
        for (const auto& pv : p.parameters)
        {
            bool known = false;
            for (uint32_t i = 0; i < kArcanistParamCount; ++i)
                if (pv.id == kArcanistParamSpecs[i].id) { known = true; break; }
            CHECK_MSG (known, ("unknown parameter id in preset " + p.name + ": " + pv.id).c_str());
        }

        if (p.name == "DX Bass 1") foundDxBass1 = true;
    }
    CHECK_MSG (foundDxBass1, "DX Bass 1 must survive the conversion");

    // No duplicate names.
    for (size_t i = 0; i < presets.size(); ++i)
        for (size_t j = i + 1; j < presets.size(); ++j)
            CHECK_MSG (presets[i].name != presets[j].name, "duplicate preset name");

    TEST_SUMMARY();
    return 0;
}
```

- [ ] **Step 4: Write `Source/FactoryPresets.h`**

Loads the 42 converted XMLs via `Common`'s `PresetIO` at compile-embed time is impractical (42 files); instead, following Hex's/Tank's precedent of compiling presets in rather than reading from a bundle-relative path, generate this header **from** the converted XML files (not by hand) so the two never drift:

```python
#!/usr/bin/env python3
"""Generate Source/FactoryPresets.h from the converted Source/Presets/Factory/*.xml
files (Task 5 Step 2's output). Run after any factory preset XML changes."""
import xml.etree.ElementTree as ET
from pathlib import Path

PRESETS_DIR = Path(__file__).resolve().parent.parent / "Source" / "Presets" / "Factory"
OUT_PATH = Path(__file__).resolve().parent.parent / "Source" / "FactoryPresets.h"

HEADER = '''#pragma once
// GENERATED FILE -- do not hand-edit. Regenerate with
// tools/generate_factory_presets_header.py after changing
// Source/Presets/Factory/*.xml.
#include "audioplugins/common/presets/Preset.h"
#include <vector>

inline std::vector<audioplugins::common::presets::Preset> arcanistFactoryPresets()
{
    using audioplugins::common::presets::Preset;
    using audioplugins::common::presets::ParameterValue;
    std::vector<Preset> presets;
'''
FOOTER = '''    return presets;
}
'''

def main() -> None:
    parts = [HEADER]
    for f in sorted(PRESETS_DIR.glob("*.xml")):
        root = ET.parse(f).getroot()
        name = root.get("name")
        plugin_id = root.get("pluginId")
        parts.append("    {\n        Preset p;\n")
        parts.append(f'        p.name = "{name}";\n')
        parts.append(f'        p.pluginId = "{plugin_id}";\n')
        parts.append("        p.schemaVersion = 1;\n")
        parts.append("        p.parameters = {\n")
        for param in root.findall("Parameter"):
            parts.append(f'            {{"{param.get("id")}", {param.get("value")}f}},\n')
        parts.append("        };\n        presets.push_back(std::move(p));\n    }\n")
    parts.append(FOOTER)
    OUT_PATH.write_text("".join(parts))
    print(f"wrote {OUT_PATH} ({len(list(PRESETS_DIR.glob('*.xml')))} presets)")

if __name__ == "__main__":
    main()
```

Save this as `tools/generate_factory_presets_header.py`, then run it:

```bash
python3 tools/generate_factory_presets_header.py
```

Expected: writes `Source/FactoryPresets.h` with 42 `Preset` entries.

- [ ] **Step 5: Append the test target to `CMakeLists.txt`**

```cmake
add_executable(test_factorypresets Tests/test_factorypresets.cpp)
target_include_directories(test_factorypresets PRIVATE Source/ Tests/)
target_link_libraries(test_factorypresets PRIVATE AudioPluginsCommon::presets)
target_compile_features(test_factorypresets PRIVATE cxx_std_20)
add_test(NAME FactoryPresets COMMAND test_factorypresets)
```

- [ ] **Step 6: Build and run**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target test_factorypresets
ctest --test-dir build -R FactoryPresets --output-on-failure
```

Expected: PASS. If any parameter id fails the "known id" check, that means the conversion script or the hand-copied `NAME_MAP` has a typo — fix the root cause (likely a mismatched filename-to-name mapping), re-run Steps 2 and 4, and re-test; do not special-case the test.

- [ ] **Step 7: Commit**

```bash
git add tools/ Source/Presets/Factory/ Source/FactoryPresets.h Tests/test_factorypresets.cpp CMakeLists.txt
git commit -m "Port Arcanist's 42 factory presets onto Common's Preset schema

Converts Source/Presets/Factory/*.xml from the JUCE-era APVTS
ValueTree schema to Common's <AudioPluginsPreset>/<Parameter> schema
(tools/convert_presets_to_common_schema.py) -- data preserved exactly,
only the wrapper tags change. Source/FactoryPresets.h is generated
from the converted XMLs (tools/generate_factory_presets_header.py) so
the compiled-in presets and the on-disk XMLs can never drift. All 42
presets, including DX Bass 1, verified present and referencing only
real Arcanist parameter ids."
```

---

## Task 6: Port the UI — RotaryKnob table, ToggleSwitch/RadioButtonGroup selects, VuMeter, presets panel, embedded MidiKeyboard

The JUCE editor has ~29 continuous knobs, 7 boolean LED toggles, and 7 mutually-exclusive LED-button groups (`osc_waveform`×4, `filter_mode`×3, `lfo_target`×3, `osc2_waveform`×5, `osc2_mult`×4, `osc2_mix_mode`×4, `osc2_flt_mode`×3) across two rows (Osc/Filter/VolEnv/FEnv/LFO/Master for Osc 1's chain, then the mirrored Osc 2 chain). `Common`'s `Button` (Task 1's dependency) has no built-in group/radio state — per `Common/CLAUDE.md`'s own design note ("adapter code belongs in the *consuming* plugin ... until a second/third consumer proves a piece of adapter code is truly shared"), this task composes a small Arcanist-local `RadioButtonGroup` from `hui::dgl::Button` rather than adding group logic to `Common` for a single consumer.

**Files:**
- Create: `Source/UI/RadioButtonGroup.h`
- Create: `Source/ArcanistUI.h`/`.cpp` (replace the Task 3 stub)
- Modify: `CMakeLists.txt` (add `Source/ArcanistUI.cpp`'s new includes — already listed in `FILES_UI` from Task 3)

**Interfaces:**
- Consumes: `hui::dgl::RotaryKnob`/`ToggleSwitch`/`Button`/`VuMeter`/`PresetSelector`/`MidiKeyboard` (`Common`), `hui::keyboard::NoteEvent` (`Common`), `kArcanistParamSpecs[]`/`ArcanistParameters` (Task 4), `arcanistFactoryPresets()` (Task 5).
- Produces: `RadioButtonGroup` (`setSelectedIndex(int)`, `getSelectedIndex()`, `std::function<void(int)> onSelected`), reused across all 7 choice-parameter groups.

- [ ] **Step 1: Write `Source/UI/RadioButtonGroup.h`**

```cpp
#pragma once
#include "audioplugins/common/hui/dgl/Button.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui
{

// Composes N Common::hui::dgl::Button instances into a mutually-exclusive
// selector -- Common's Button is a plain momentary click with no persistent
// "selected" visual state of its own, so this class owns that state and
// re-palettes the active button on every change. Local to Arcanist per
// Common's own design note: adapter/composition code lives in the
// consuming plugin until proven shared by a second/third consumer.
class RadioButtonGroup
{
public:
    RadioButtonGroup(DGL_NAMESPACE::NanoTopLevelWidget* parent,
                      std::vector<std::string> labels,
                      audioplugins::common::hui::dgl::ButtonPalette activePalette,
                      audioplugins::common::hui::dgl::ButtonPalette inactivePalette)
        : activePalette_(activePalette), inactivePalette_(inactivePalette)
    {
        for (auto& label : labels)
        {
            auto button = std::make_unique<audioplugins::common::hui::dgl::Button>(parent);
            button->setLabel(label.c_str());
            const int index = static_cast<int>(buttons_.size());
            button->onClick = [this, index]() { select(index); };
            buttons_.push_back(std::move(button));
        }
        applyPalettes();
    }

    audioplugins::common::hui::dgl::Button& buttonAt(int i) { return *buttons_[static_cast<size_t>(i)]; }
    int count() const { return static_cast<int>(buttons_.size()); }

    // Host-driven / programmatic change. Does NOT invoke onSelected.
    void setSelectedIndex(int index)
    {
        if (index < 0 || index >= count()) return;
        selectedIndex_ = index;
        applyPalettes();
    }
    int getSelectedIndex() const noexcept { return selectedIndex_; }

    std::function<void(int)> onSelected;

private:
    void select(int index)
    {
        selectedIndex_ = index;
        applyPalettes();
        if (onSelected) onSelected(index);
    }

    void applyPalettes()
    {
        for (int i = 0; i < count(); ++i)
            buttons_[static_cast<size_t>(i)]->setPalette(i == selectedIndex_ ? activePalette_ : inactivePalette_);
    }

    std::vector<std::unique_ptr<audioplugins::common::hui::dgl::Button>> buttons_;
    audioplugins::common::hui::dgl::ButtonPalette activePalette_, inactivePalette_;
    int selectedIndex_ = 0;
};

} // namespace ui
```

- [ ] **Step 2: Write `Source/ArcanistUI.h`**

```cpp
#pragma once
#include "DistrhoUI.hpp"
#include "ArcanistParams.h"
#include "UI/RadioButtonGroup.h"
#include "audioplugins/common/hui/dgl/RotaryKnob.h"
#include "audioplugins/common/hui/dgl/ToggleSwitch.h"
#include "audioplugins/common/hui/dgl/VuMeter.h"
#include "audioplugins/common/hui/dgl/PresetSelector.h"
#include "audioplugins/common/hui/dgl/Button.h"
#include "audioplugins/common/hui/dgl/MidiKeyboard.h"
#include "audioplugins/common/presets/PresetBrowser.h"
#include <array>
#include <memory>
#include <vector>

START_NAMESPACE_DISTRHO
namespace hui = audioplugins::common::hui;

class ArcanistUI : public UI
{
public:
    ArcanistUI();

protected:
    void parameterChanged(uint32_t index, float value) override;
    void onNanoDisplay() override;
    void uiFileBrowserSelected(const char* filename) override;

private:
    void applyPreset(const audioplugins::common::presets::Preset& preset);
    std::vector<audioplugins::common::presets::ParameterValue> captureCurrentParameters() const;
    void refreshPresetControls();
    void setKnobOrSwitch(uint32_t paramIndex, float value); // dispatches by ArcanistParamKind

    // One entry per continuous parameter (kArcanistParamCount-sized, only
    // Continuous-kind slots populated) / boolean / choice -- indexed
    // directly by ArcanistParameters, avoiding 43 hand-named members.
    std::array<std::unique_ptr<hui::dgl::RotaryKnob>, kArcanistParamCount> knobs_;
    std::array<std::unique_ptr<hui::dgl::ToggleSwitch>, kArcanistParamCount> switches_;
    std::array<std::unique_ptr<ui::RadioButtonGroup>, kArcanistParamCount> radioGroups_;

    std::unique_ptr<hui::dgl::VuMeter> outputMeter_;
    std::unique_ptr<hui::dgl::MidiKeyboard> keyboard_;

    audioplugins::common::presets::PresetBrowser presetBrowser_;
    std::unique_ptr<hui::dgl::PresetSelector> presetSelector_;
    std::unique_ptr<hui::dgl::Button> saveButton_, deleteButton_;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistUI)
};

END_NAMESPACE_DISTRHO
```

- [ ] **Step 3: Write `Source/ArcanistUI.cpp` — construction, layout, and the data-driven knob/switch/radio-group creation loop**

```cpp
#include "ArcanistUI.h"
#include "FactoryPresets.h"
#include <cstdlib>
#include <string>

START_NAMESPACE_DISTRHO

namespace {

// ArcanistCol palette, ported from Source/_juce_reference/UI/ArcanistLookAndFeel.h.
const hui::dgl::RotaryKnobPalette kKnobPalette = {
    /* track        */ {0x13, 0x15, 0x13, 0xff},
    /* valueArc     */ {0xe8, 0x88, 0x14, 0xff},
    /* valueArcGlow */ {0xe8, 0xa8, 0x44, 0xff},
    /* knobTop      */ {0x4e, 0x52, 0x48, 0xff},
    /* knobBottom   */ {0x25, 0x28, 0x25, 0xff},
    /* knobRim      */ {0x5c, 0x60, 0x58, 0xff},
};
const hui::dgl::ToggleSwitchPalette kTogglePalette = {
    /* bezel      */ {0x0a, 0x1a, 0x0a, 0xff},
    /* track      */ {0x1a, 0x20, 0x20, 0xff},
    /* activeGlow */ {0x22, 0xe8, 0x40, 0xff},
    /* thumbTop   */ {0x4e, 0x52, 0x48, 0xff},
    /* thumbBottom*/ {0x25, 0x28, 0x25, 0xff},
    /* thumbRim   */ {0x5c, 0x60, 0x58, 0xff},
};
const hui::dgl::ButtonPalette kRadioActivePalette = {
    /* background         */ {0x2a, 0x30, 0x30, 0xff},
    /* backgroundDisabled */ {0x1a, 0x20, 0x20, 0xff},
    /* border             */ {0x5c, 0x60, 0x58, 0xff},
    /* text               */ {0xcc, 0xc8, 0xb4, 0xff},
    /* textDisabled       */ {0x4c, 0x50, 0x4a, 0xff},
};
const hui::dgl::ButtonPalette kRadioInactivePalette = {
    /* background         */ {0x1a, 0x20, 0x20, 0xff},
    /* backgroundDisabled */ {0x1a, 0x20, 0x20, 0xff},
    /* border             */ {0x32, 0x3a, 0x32, 0xff},
    /* text               */ {0x4c, 0x50, 0x4a, 0xff},
    /* textDisabled       */ {0x4c, 0x50, 0x4a, 0xff},
};
const hui::dgl::VuMeterPalette kMeterPalette = {
    /* background */ {0x11, 0x13, 0x11, 0xff},
    /* border     */ {0x32, 0x3a, 0x32, 0xff},
    /* fillLow    */ {0x22, 0xaa, 0x44, 0xff},
    /* fillHigh   */ {0xe8, 0x88, 0x14, 0xff},
    /* peakLine   */ {0xff, 0xff, 0xff, 0xff},
};

// Choice-parameter label sets, transcribed from the JUCE-era
// PluginEditor.h's LedButton arrays.
const std::vector<std::string> kOscWaveformLabels  = {"SINE", "TRI", "SAW", "SQR"};
const std::vector<std::string> kFilterModeLabels   = {"LP", "BP", "HP"};
const std::vector<std::string> kLfoTargetLabels    = {"FILTER", "AMP", "PITCH"};
const std::vector<std::string> kOsc2WaveformLabels = {"SINE", "TRI", "SAW", "SQR", "NOISE"};
const std::vector<std::string> kOsc2MultLabels     = {"x0.5", "x1", "x2", "x4"};
const std::vector<std::string> kOsc2MixModeLabels  = {"SUM", "AM", "FM", "RING"};

std::string arcanistUserPresetsDirectory()
{
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.config/Arcanist/presets";
}

} // namespace

ArcanistUI::ArcanistUI()
    : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    , presetBrowser_(arcanistFactoryPresets(), arcanistUserPresetsDirectory(), "com.spellbound.arcanist")
{
    loadSharedResources();

    // ── Continuous parameters -> RotaryKnob, boolean -> ToggleSwitch,
    //    choice -> RadioButtonGroup. Positions are assigned by a fixed
    //    per-parameter-index layout table (a std::array<Rect, kArcanistParamCount>
    //    populated the same mechanical way as this loop -- omitted here for
    //    brevity; see the companion layout table this task also adds to
    //    Source/ArcanistUI.cpp, one {x,y} pair per ArcanistParameters value,
    //    grouped visually into the same OSC/FILTER/VOLENV/FENV/LFO/MASTER
    //    (row 1) and OSC2/MIX2/ENV2/FLT2/FENV2 (row 2) sections the JUCE
    //    editor used).
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
    {
        const auto& spec = kArcanistParamSpecs[i];
        switch (spec.kind)
        {
        case ArcanistParamKind::Continuous:
        {
            auto knob = std::make_unique<hui::dgl::RotaryKnob>(this);
            knob->setPalette(kKnobPalette);
            knob->setRange(spec.min, spec.max);
            knob->setDefaultValue(spec.defaultValue);
            knob->setValue(spec.defaultValue);
            knob->onDragStateChanged = [this, i](bool started) { editParameter(i, started); };
            knob->onValueChanged = [this, i](float v) { setParameterValue(i, v); };
            knobs_[i] = std::move(knob);
            break;
        }
        case ArcanistParamKind::Boolean:
        {
            auto sw = std::make_unique<hui::dgl::ToggleSwitch>(this);
            sw->setPalette(kTogglePalette);
            sw->setPosition(spec.defaultValue > 0.5f ? 1 : 0);
            sw->onDragStateChanged = [this, i](bool started) { editParameter(i, started); };
            sw->onPositionChanged = [this, i](int pos) { setParameterValue(i, pos > 0 ? 1.0f : 0.0f); };
            switches_[i] = std::move(sw);
            break;
        }
        case ArcanistParamKind::Choice:
        {
            const std::vector<std::string>* labels =
                  i == kParamOscWaveform  ? &kOscWaveformLabels
                : i == kParamFilterMode   ? &kFilterModeLabels
                : i == kParamLfoTarget    ? &kLfoTargetLabels
                : i == kParamOsc2Waveform ? &kOsc2WaveformLabels
                : i == kParamOsc2Mult     ? &kOsc2MultLabels
                : i == kParamOsc2MixMode  ? &kOsc2MixModeLabels
                : &kFilterModeLabels; // kParamOsc2FltMode reuses LP/BP/HP

            auto group = std::make_unique<ui::RadioButtonGroup>(this, *labels, kRadioActivePalette, kRadioInactivePalette);
            group->setSelectedIndex(static_cast<int>(spec.defaultValue));
            group->onSelected = [this, i](int idx) {
                editParameter(i, true);
                setParameterValue(i, static_cast<float>(idx));
                editParameter(i, false);
            };
            radioGroups_[i] = std::move(group);
            break;
        }
        }
    }

    // ── Output meter ──────────────────────────────────────────────────────
    outputMeter_ = std::make_unique<hui::dgl::VuMeter>(this);
    outputMeter_->setSize(20, 200);
    outputMeter_->setPalette(kMeterPalette);

    // ── Presets bar ───────────────────────────────────────────────────────
    presetSelector_ = std::make_unique<hui::dgl::PresetSelector>(this);
    presetSelector_->setClosedSize(220, 22);
    presetSelector_->onIndexSelected = [this](int index) {
        if (const auto* preset = presetBrowser_.selectIndex(index))
        {
            applyPreset(*preset);
            refreshPresetControls();
        }
    };

    saveButton_ = std::make_unique<hui::dgl::Button>(this);
    saveButton_->setLabel("SAVE AS");
    saveButton_->onClick = [this]() {
        FileBrowserOptions options;
        options.saving = true;
        options.defaultName = "New Preset.xml";
        options.title = "Save Arcanist Preset";
        const std::string dir = arcanistUserPresetsDirectory();
        options.startDir = dir.c_str();
        openFileBrowser(options);
    };

    deleteButton_ = std::make_unique<hui::dgl::Button>(this);
    deleteButton_->setLabel("DELETE");
    deleteButton_->onClick = [this]() {
        if (presetBrowser_.deleteCurrent()) refreshPresetControls();
    };

    // ── Embedded MIDI keyboard ────────────────────────────────────────────
    // 3 octaves starting at MIDI 48 (C3): pad/drone chords are typically
    // voiced from around middle C upward (Vangelis/Enya-style sustained
    // chords), unlike Pugilist's fixed low GM percussion notes (35-51,
    // hence its C1 start) -- Arcanist is fully chromatic and polyphonic, so
    // no note-zone tinting is needed (setZones() is skipped entirely).
    keyboard_ = std::make_unique<hui::dgl::MidiKeyboard>(this, 3);
    keyboard_->setSize(static_cast<uint>(DISTRHO_UI_DEFAULT_WIDTH - 12), 78);
    keyboard_->setAbsolutePos(6, DISTRHO_UI_DEFAULT_HEIGHT - 78 - 6);
    keyboard_->setOctaveShift(48);
    keyboard_->onNote = [this](const hui::keyboard::NoteEvent& e) {
        sendNote(0, e.note, e.isNoteOn ? e.velocity : 0);
    };

    refreshPresetControls();
    // resized() (below, ported layout-only from the JUCE editor's resized())
    // positions every widget above -- omitted here as pure geometry, not
    // migration-relevant logic.
}

void ArcanistUI::parameterChanged(const uint32_t index, const float value)
{
    if (index >= kArcanistParamCount) return;
    setKnobOrSwitch(index, value);
}

void ArcanistUI::setKnobOrSwitch(const uint32_t index, const float value)
{
    switch (kArcanistParamSpecs[index].kind)
    {
    case ArcanistParamKind::Continuous: if (knobs_[index]) knobs_[index]->setValue(value); break;
    case ArcanistParamKind::Boolean:    if (switches_[index]) switches_[index]->setPosition(value > 0.5f ? 1 : 0); break;
    case ArcanistParamKind::Choice:     if (radioGroups_[index]) radioGroups_[index]->setSelectedIndex(static_cast<int>(value)); break;
    }
}

void ArcanistUI::onNanoDisplay()
{
    beginPath();
    rect(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    fillColor(Color(24, 27, 24, 255)); // ArcanistCol::bg()
    fill();
    closePath();
}

void ArcanistUI::uiFileBrowserSelected(const char* filename)
{
    if (filename == nullptr) return;
    std::string path(filename);
    const size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    const size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    if (presetBrowser_.saveAs(base, captureCurrentParameters()))
        refreshPresetControls();
}

void ArcanistUI::applyPreset(const audioplugins::common::presets::Preset& preset)
{
    for (const auto& pv : preset.parameters)
    {
        for (uint32_t i = 0; i < kArcanistParamCount; ++i)
        {
            if (pv.id != kArcanistParamSpecs[i].id) continue;
            setKnobOrSwitch(i, pv.value);
            editParameter(i, true);
            setParameterValue(i, pv.value);
            editParameter(i, false);
            break;
        }
    }
    // Ported from the JUCE-era PresetManager -> setCurrentProgram() ->
    // allNotesOffPending_ path: silence + retrigger on preset change.
    if (auto* adapter = static_cast<ArcanistPluginAdapter*>(getPluginInstancePointer()))
        adapter->requestAllNotesOff();
}

std::vector<audioplugins::common::presets::ParameterValue> ArcanistUI::captureCurrentParameters() const
{
    std::vector<audioplugins::common::presets::ParameterValue> result;
    result.reserve(kArcanistParamCount);
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
    {
        float value = kArcanistParamSpecs[i].defaultValue;
        switch (kArcanistParamSpecs[i].kind)
        {
        case ArcanistParamKind::Continuous: if (knobs_[i]) value = knobs_[i]->getValue(); break;
        case ArcanistParamKind::Boolean:    if (switches_[i]) value = static_cast<float>(switches_[i]->getPosition()); break;
        case ArcanistParamKind::Choice:     if (radioGroups_[i]) value = static_cast<float>(radioGroups_[i]->getSelectedIndex()); break;
        }
        result.push_back({kArcanistParamSpecs[i].id, value});
    }
    return result;
}

void ArcanistUI::refreshPresetControls()
{
    presetSelector_->setEntries(presetBrowser_.getEntries());
    presetSelector_->setCurrentIndex(presetBrowser_.getCurrentIndex());

    const auto entries = presetBrowser_.getEntries();
    const int idx = presetBrowser_.getCurrentIndex();
    const bool isFactory = (idx >= 0 && static_cast<size_t>(idx) < entries.size()) ? entries[static_cast<size_t>(idx)].isFactory : true;
    deleteButton_->setEnabled(!isFactory);
}

UI* createUI() { return new ArcanistUI(); }

END_NAMESPACE_DISTRHO
```

Note: `ArcanistUI::applyPreset()` needs `ArcanistPluginAdapter`'s definition (for `requestAllNotesOff()`) and DPF's `getPluginInstancePointer()`, which requires `DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 1` in `Source/DistrhoPluginInfo.h` and `#include "ArcanistPluginAdapter.h"` at the top of `ArcanistUI.cpp` — add both now.

- [ ] **Step 4: Write the full `resized()`/layout method**

Port the JUCE editor's `resized()` geometry (six row-1 sections: OSC/FILTER/VOLENV/FENV/LFO/MASTER, five row-2 sections: OSC2/MIX2/ENV2/FLT2/FENV2, plus the preset bar at the top and the keyboard strip at the bottom) into a `void ArcanistUI::layoutWidgets()` called once from the constructor after all widgets above are created, using `setAbsolutePos()`/`setSize()` on every knob/switch/radio-group/button exactly as `PluginEditor.cpp`'s `resized()` and `setupKnob()` positioned the JUCE `Slider`s — this is pure geometry (x/y/width/height per widget, no new logic), so port it by reading `Source/_juce_reference/PluginEditor.cpp`'s `resized()` section-by-section rather than re-deriving coordinates from scratch.

- [ ] **Step 5: Build**

```bash
cmake --build build --parallel
```

Expected: links against `AudioPluginsCommon::hui_dgl`/`AudioPluginsCommon::presets` (already wired in `Arcanist-ui` since Task 3).

- [ ] **Step 6: Run the full test suite**

```bash
ctest --test-dir build --output-on-failure
```

Expected: 8/8 pass (Task 3's 7 plus `FactoryPresets` from Task 5).

- [ ] **Step 7: Manual visual + interaction check**

```bash
carla-single vst3 build/bin/Arcanist.vst3 &
```

Confirm: all ~29 knobs, 7 toggle switches, and 7 radio-button groups render and respond to mouse drag/click; selecting a factory preset (e.g. "DX Bass 1") visibly moves every affected knob/switch/radio-group to that preset's values and the output audibly changes; clicking a note on the embedded keyboard produces sound and updates in sync with a connected MIDI keyboard playing the same note; the VU meter tracks output level.

- [ ] **Step 8: Commit**

```bash
git add Source/UI/RadioButtonGroup.h Source/ArcanistUI.h Source/ArcanistUI.cpp Source/DistrhoPluginInfo.h
git commit -m "Port Arcanist's UI onto Common's DGL widgets: knobs, switches, radio groups, presets bar, embedded keyboard

Data-driven widget creation loop over kArcanistParamSpecs replaces ~43
hand-written JUCE Slider/LedButton members. Adds Source/UI/
RadioButtonGroup.h (composed from Common::hui::dgl::Button) for the 7
mutually-exclusive choice parameters Common has no dedicated widget
for -- kept local per Common's own 'adapter code belongs in the
consumer until proven shared' design note. Wires the presets bar
(PresetSelector/Button/PresetBrowser, ported presets from Task 5) and
a 3-octave embedded MidiKeyboard starting at C3 (MIDI 48), matching
Pugilist's proven sendNote()-via-onNote pattern."
```

---

## Task 7: Validator CI legs (pluginval / clap-validator / lv2lint) + fix findings

**Files:** Modify: `.github/workflows/ci.yml`.

**Interfaces:** N/A — CI-only change, plus whatever `ArcanistPluginAdapter`/`ArcanistUI`/`Synthesis/*` fixes the validators' findings require.

- [ ] **Step 1: Append the validator steps to the Linux leg of `.github/workflows/ci.yml`**

Copy the exact `pluginval`/`clap-validator`/`lv2lint` step block from Tank's `docs/superpowers/plans/2026-09-05-tank-dpf-migration.md` Task 8 Step 1 verbatim, substituting `Arcanist.vst3`/`Arcanist.clap`/`https://spellbound.audio/plugins/arcanist` for Tank's equivalents. Since Arcanist is a synth, add `--random-seed 12345` is not needed, but do add `--strictness-level 5` as Tank does; pluginval sends its own MIDI test notes for synths automatically, no extra flag required.

- [ ] **Step 2: Run pluginval locally**

```bash
curl -sL -o pluginval_Linux.zip https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip
unzip -o pluginval_Linux.zip && chmod +x pluginval
xvfb-run -a ./pluginval --strictness-level 5 --validate build/bin/Arcanist.vst3
```

For each distinct failure: `gh issue create --repo TriYop/spellbound-arcanist --title "pluginval: <finding>" --body "<output excerpt + root cause>"`, fix in `ArcanistPluginAdapter`/`ArcanistUI`/`Synthesis/*`, verify locally, close referencing the fixing commit. Investigate the actual output — do not guess findings in advance. Watch specifically for polyphonic-synth-specific findings pluginval may raise that Tank/Outflank never hit (e.g. rapid all-16-voices-stolen note storms, or state-recall while notes are held) since this is the workspace's first fully polyphonic MIDI-driven DPF synth migration.

- [ ] **Step 3: Run clap-validator locally**

```bash
curl -sL -o clap-validator.zip https://github.com/free-audio/clap-validator/releases/download/0.4.1/clap-validator-0.4.1-127-g152b982-ubuntu-22.04.zip
unzip -o clap-validator.zip
tar -xzf clap-validator-*-ubuntu-22.04.tar.gz
chmod +x clap-validator
xvfb-run -a ./clap-validator validate build/bin/Arcanist.clap
```

Same ticket-per-finding process. The chunked-state-read parameter-loss failure (already patched by Task 3) should NOT recur — if it does, re-check Task 3 Step 10's fresh-`build/_deps` caveat before assuming a new bug.

- [ ] **Step 4: Run lv2lint locally**

Copy Tank's Task 8 Step 4 commands verbatim, substituting `https://spellbound.audio/plugins/arcanist`. The known DPF-wide "Plugin Class" finding is already accepted non-blocking; don't re-file it.

- [ ] **Step 5: Commit the CI change and any fixes**

```bash
git add .github/workflows/ci.yml
git commit -m "Add pluginval/clap-validator/lv2lint Linux CI legs"
```

Fixes from Steps 2-4 get their own commits: `git commit -m "Fix <finding>, closes #<issue-number>"`.

- [ ] **Step 6: Push and verify with `gh run list`**

```bash
git push -u origin worktree-dpf-stage0
gh run list --repo TriYop/spellbound-arcanist --branch worktree-dpf-stage0 --limit 5
```

Expected: Linux leg green. If it fails at "Configure Common repo access", `COMMON_REPO_TOKEN` was somehow not actually set despite Task 3 Step 9 — re-run `gh secret set COMMON_REPO_TOKEN --repo TriYop/spellbound-arcanist` and re-run. Do not report CI as passing without checking `gh run list`'s actual output.

---

## Task 8: Rewrite `CLAUDE.md` for the real, DPF-based, implemented state

**Files:** Modify: `CLAUDE.md`.

**Interfaces:** N/A — documentation only.

- [ ] **Step 1: Rewrite `CLAUDE.md`** following Tank's/Outflank's post-migration `CLAUDE.md` as the structural template (Project Overview / Build Commands / Architecture / Parameters table / Key design constraints), replacing every JUCE-era build command with the DPF equivalents from Task 3's `CMakeLists.txt`, documenting:
  - The framework-free `Synthesis/` layer (Task 2) and its `dsp::AudioBuffer`/`DspUtil.h` decoupling — explicitly correct the historical stale claim that this DSP embeds `juce::dsp::StateVariableTPTFilter` (it's a hand-rolled biquad).
  - `ArcanistPluginAdapter`'s data-driven `kArcanistParamSpecs` table (43 parameters) and the 16-voice same-note/free/oldest-steal algorithm.
  - The presets system: 42 factory presets ported from the JUCE-era APVTS-ValueTree XML schema onto `Common`'s `Preset` schema (Task 5), with the two `tools/*.py` scripts documented as the source of truth (never hand-edit `Source/Presets/Factory/*.xml` or `Source/FactoryPresets.h` directly — regenerate).
  - `Source/UI/RadioButtonGroup.h` as an Arcanist-local composition, and why (`Common`'s `Button` has no group state; not pushed upstream for a single consumer).
  - The embedded 3-octave `MidiKeyboard` starting at C3 (MIDI 48), no zone tinting (fully chromatic/polyphonic, unlike Pugilist's fixed pads).
  - The `Common` `v0.4.0` dependency and *why* it's not `v0.3.0` (Task 1's MidiKeyboard restoration).
  - `Source/_juce_reference/` as the preserved porting reference.
- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "Rewrite CLAUDE.md for Arcanist's DPF-based, fully-implemented state"
```

---

## Task 9: Final full verification pass and wrap-up

**Files:** none (manual verification + a GitHub issue + PR).

**Interfaces:** N/A.

- [ ] **Step 1: Full local verification pass from a clean build**

```bash
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Expected: clean configure/build, 8/8 tests passing (AudioBuffer, DspUtil, Oscillator, Filter, Envelope, LFO, Voice, FactoryPresets).

- [ ] **Step 2: Manual host pass in Carla covering the full instrument**

Load `build/bin/Arcanist.vst3` (or the CLAP/LV2) in Carla and confirm:
1. Playing a chord via a connected MIDI device (or Carla's virtual keyboard) produces a sustained pad sound with all 16 voices independently ringing and releasing.
2. The embedded on-screen keyboard produces the same notes as external MIDI, in sync.
3. Selecting each of several factory presets (at minimum: one "Ethereal", one "Vintage", one "GM", and "DX Bass 1") audibly changes the sound and visibly moves the correct knobs/switches/radio groups.
4. Saving a modified preset via SAVE AS's native file dialog appears in the dropdown with DELETE enabled; selecting a factory preset again disables DELETE; deleting the saved preset removes it from the dropdown.
5. Changing `filter_cutoff`/`osc_tune`/`lfo_depth` knobs live while a note is held produces click-free, audible changes (no zipper noise).
6. Playing more than 16 notes simultaneously steals the oldest voice rather than dropping new notes silently.

- [ ] **Step 3: File the tracked verification issue**

```bash
gh issue create --repo TriYop/spellbound-arcanist \
  --title "Manual host verification: full instrument pass (DPF migration)" \
  --body "$(cat <<'EOF'
Tracks the final manual verification pass for Arcanist's DPF migration,
per docs/superpowers/plans/2026-09-06-arcanist-dpf-migration.md's Task 9.

- [ ] 16-voice polyphony + independent release confirmed in Carla
- [ ] Embedded keyboard matches external MIDI input
- [ ] Representative factory presets (Ethereal/Vintage/GM/DX Bass 1)
      recall correctly, audibly and visually
- [ ] Save/Delete user preset round-trip works via the native file dialog
- [ ] Live knob changes are click-free while a note is held
- [ ] >16-note voice stealing behaves correctly
EOF
)"
```

Close it once Step 2's checklist is confirmed.

- [ ] **Step 4: Open the pull request**

```bash
git push -u origin worktree-dpf-stage0
gh pr create --repo TriYop/spellbound-arcanist \
  --title "Migrate Arcanist off JUCE onto DPF; decouple DSP; port presets; add embedded keyboard" \
  --body "$(cat <<'EOF'
## Summary
- Decouples Source/Synthesis/* (Oscillator/Filter/Envelope/Voice) from
  JUCE onto a framework-free dsp::AudioBuffer + DspUtil.h, with
  Arcanist's first-ever unit test suite (7 executables).
- Rewrites the build off JUCE + clap-juce-extensions onto DPF (pinned
  commit + chunked-read patch) and AudioPlugins/Common v0.4.0 -- note
  v0.4.0, not v0.3.0: this migration also restored Common's MidiKeyboard
  widget, dropped from master by an earlier partial merge (see linked
  spellbound-common PR).
- Ports the 16-voice same-note/free/oldest-steal MIDI algorithm and all
  43 parameters (data-driven ArcanistParamSpec table) into
  ArcanistPluginAdapter.
- Ports the 42 factory presets (including DX Bass 1) onto Common's
  Preset schema via a from-scratch conversion script.
- Ports the UI onto Common's DGL widgets, adding a local
  RadioButtonGroup composition for choice parameters, and an embedded
  3-octave MIDI keyboard (Pugilist's proven pattern).
- Adds pluginval/clap-validator/lv2lint Linux CI legs.

## Test plan
- [x] `ctest --test-dir build --output-on-failure` -- 8/8 passing
- [x] Manual host verification in Carla -- see linked issue
- [ ] CI green on this PR (`gh run list --repo TriYop/spellbound-arcanist`)
EOF
)"
```

- [ ] **Step 5: Verify CI on the PR before considering this plan complete**

```bash
gh run list --repo TriYop/spellbound-arcanist --branch worktree-dpf-stage0 --limit 5
```

Do not report the migration as complete without this coming back green on the Linux leg (Windows/macOS remain `continue-on-error` and are not a completion criterion, per Global Constraints).
