# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Spellbound Arcanist is a polyphonic pad/drone synthesizer plugin (VST3 / CLAP / LV2 / Standalone) for generating rich, sustained pad sounds — synthetic string textures through virtual choir atmospheres — targeting ambient, cinematic, and orchestral production workflows (Vangelis- and Enya-style sustained chords).

**Current state: implemented and migrated off JUCE onto [DPF](https://github.com/DISTRHO/DPF)** (DISTRHO Plugin Framework, ISC-licensed — see `AudioPlugins/CLAUDE.md`'s workspace-wide migration note). The DSP (`Source/Synthesis/*.h/.cpp`) was decoupled from JUCE during the migration and is framework-free, backed by its own JUCE-free CTest suite (`Tests/test_audiobuffer.cpp`, `test_dsputil.cpp`, `test_oscillator.cpp`, `test_filter.cpp`, `test_envelope.cpp`, `test_lfo.cpp`, `test_voice.cpp`, `test_factorypresets.cpp`). `Source/ArcanistPluginAdapter.{h,cpp}` is a thin DPF `Plugin` shim over a 16-voice pool of the unchanged `Voice` class, declaring all 43 host parameters generically via a data table and reproducing the JUCE-era same-note/free/oldest-steal voice-allocation algorithm exactly. `Source/ArcanistUI.{h,cpp}` has the full ported editor — per-parameter `RotaryKnob`/`ToggleSwitch`/`RadioButtonGroup` controls, an output `VuMeter`, a presets bar (`PresetSelector` + SAVE AS/DELETE, backed by `AudioPluginsCommon::presets::PresetBrowser`), and an embedded 3-octave `MidiKeyboard`. The JUCE-era `PluginProcessor`/`PluginEditor`/`PresetManager` are preserved unchanged under `Source/_juce_reference/` as the porting reference this was migrated from.

## Build Commands

### Linux prerequisites (one-time)

```bash
sudo apt install cmake ninja-build build-essential git \
    libasound2-dev libjack-jackd2-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
    libxinerama-dev libxrandr-dev libxrender-dev \
    libfreetype-dev libfontconfig1-dev \
    libglu1-mesa-dev libwebkit2gtk-4.1-dev
```

(freetype/fontconfig/webkit2gtk are required by DPF's DGL/NanoVG UI backend and its WebView-leak workaround, not JUCE leftovers.)

### Configure / build / run

```bash
# Configure (first run fetches DPF + AudioPluginsCommon into build/_deps/)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build build --parallel

# Build outputs (DPF layout, under build/bin/)
#   build/bin/Arcanist               (JACK/ALSA standalone -- Arcanist is a
#                                      synth, so unlike the effect plugins in
#                                      this workspace it does get one)
#   build/bin/Arcanist.vst3/
#   build/bin/Arcanist.clap
#   build/bin/Arcanist.lv2/

# Run the standalone directly
./build/bin/Arcanist

# Unit tests (DSP + factory-presets, no DPF/JUCE dependency)
ctest --test-dir build --output-on-failure

# Install plugins (Linux, dev build)
cp -r build/bin/Arcanist.vst3 ~/.vst3/
cp    build/bin/Arcanist.clap ~/.clap/
```

### Release packaging

```bash
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
cd build-release && cpack
```

Produces a portable `Arcanist-<version>-linux-x86_64.tar.gz` with `scripts/install.sh`/`scripts/uninstall.sh` for installing VST3/CLAP/LV2 to the user's plugin directories.

## Architecture

### Source layout

```
Source/
  ArcanistPluginAdapter.h/.cpp  — DPF Plugin: 43 parameters, 16-voice pool, MIDI dispatch
  ArcanistUI.h/.cpp             — DPF UI: knobs/switches/radio groups, presets bar, keyboard
  ArcanistParams.h              — kArcanistParamSpecs: id/name/kind/range/default per parameter
  DistrhoPluginInfo.h           — plugin identity + the ArcanistParameters index enum
  FactoryPresets.h              — GENERATED, do not hand-edit (see Presets below)
  Synthesis/                    — framework-free DSP, no DPF/JUCE dependency
    AudioBuffer.h                — dsp::AudioBuffer: minimal juce::AudioBuffer<float> stand-in
    DspUtil.h                    — midiNoteToHz/dbToGain/gainToDb (replace juce::MidiMessage/Decibels)
    Voice.h/.cpp                 — one polyphonic voice: two oscillator chains, two filters,
                                    four envelopes (amp/filter × osc1/osc2), one LFO
    Oscillator.h/.cpp            — band-naive multi-waveform oscillator
    Filter.h/.cpp                — hand-rolled RBJ biquad (LP/BP/HP), NOT juce::dsp
    Envelope.h/.cpp               — ADSR with optional sustain-hold
    LFO.h/.cpp                    — low-frequency modulator
  UI/
    RadioButtonGroup.h            — Arcanist-local composition over Common's Button (see below)
  Presets/Factory/*.xml           — 42 factory presets, Common's Preset schema
  _juce_reference/                — preserved pre-migration JUCE source, porting reference only
tools/
  convert_presets_to_common_schema.py — one-shot: JUCE APVTS ValueTree XML -> Common Preset XML
  generate_factory_presets_header.py  — Presets/Factory/*.xml -> Source/FactoryPresets.h
  update_presets_osc2.py              — one-shot: backfilled osc2_* params into pre-Osc2 presets
  analyze_sample.py                   — offline preset-design helper, not part of the build
```

### `Synthesis/` is framework-free (Task 2 of the migration)

`Source/Synthesis/*` has zero DPF/JUCE dependency, compiles standalone (see the `Tests/*.cpp` CTest targets in `CMakeLists.txt`, each pulling in only the `.cpp` files it needs), and is a straight behavioral port of the JUCE-era DSP — not a rewrite:

- `dsp::AudioBuffer` (`AudioBuffer.h`) replaces `juce::AudioBuffer<float>`, supporting the same two modes: `setSize()` (owns storage, used by `Voice`'s scratch buffers) and `wrapExternal()` (a non-owning view over raw channel pointers, used by `ArcanistPluginAdapter::run()` to wrap DPF's `float**` output without a copy).
- `DspUtil.h` replaces the handful of `juce::MidiMessage`/`juce::Decibels` static helpers the DSP used (`midiNoteToHz`, `dbToGain`, `gainToDb`).
- **`Filter` is a hand-rolled RBJ (Audio EQ Cookbook) biquad**, not `juce::dsp::StateVariableTPTFilter` — correcting a stale claim in this file's pre-migration text. `Filter::updateCoefficients()` computes standard RBJ LP/BP/HP coefficients from `cutoff_`/`resonance_` (resonance mapped to Q = 0.5 + resonance × 3.5) and `Filter::process()` runs a Direct Form II Transposed biquad per channel, with the cutoff itself one-pole-smoothed per sample to avoid zipper noise on parameter changes. This was already the shape of the JUCE-era DSP; only the surrounding buffer/utility types changed.

### `ArcanistPluginAdapter`: data-driven parameters + voice allocation

`Source/ArcanistParams.h` defines `kArcanistParamSpecs[kArcanistParamCount]` (43 entries) — one `{id, name, kind, min, max, defaultValue, numChoices}` row per parameter, in the exact order of the `ArcanistParameters` enum in `DistrhoPluginInfo.h`. `ArcanistPluginAdapter::initParameter()` derives every DPF `Parameter` (name, symbol, range, default, boolean/integer hints) from this one table instead of 43 hand-written `initParameter()` bodies; `ArcanistUI` reads the same table via `ArcanistParamKind` to decide whether a parameter gets a `RotaryKnob`, `ToggleSwitch`, or `RadioButtonGroup`. `ArcanistParamSpec::id` matches both the JUCE-era APVTS parameter id and the `id` attribute in `Source/Presets/Factory/*.xml`, so presets need no remapping.

MIDI note-on allocates across a fixed pool of 16 `Voice`s (`ArcanistPluginAdapter::handleNoteOn()`), ported verbatim from the JUCE-era per-block MIDI loop:
1. **Same-note reuse** — if the incoming note is already ringing on a voice, reuse that voice (cuts the release tail cleanly instead of stacking a second voice on the same pitch).
2. **Free voice** — else, the first inactive voice.
3. **Oldest-steal** — else, the active voice with the largest `getTimeSinceNoteOn()`.

A full preset reload (`ArcanistUI::applyPreset()` → `ArcanistPluginAdapter::requestAllNotesOff()` via `DISTRHO_PLUGIN_WANT_DIRECT_ACCESS`) triggers a 0.1 s silence ramp across all voices before hard-killing them, matching the JUCE-era `PresetManager::setCurrentProgram()` behavior exactly.

### Parameters

43 parameters across two near-identical oscillator/filter/envelope chains (Osc 1 always on; Osc 2 bypassable and independently modulatable) plus one shared LFO and master gain. Full definitions live in `Source/ArcanistParams.h`; summarized by section:

| Section | Parameters | Notes |
|---|---|---|
| Osc 1 | `osc_waveform` (4 choices), `osc_tune` (±24 semitones), `osc_detune` (±50 cents) | drives both `oscillator_` and `oscillator2_` (detuned unison pair) |
| Filter 1 | `filter_mode` (LP/BP/HP), `filter_cutoff` (20 Hz–20 kHz), `filter_resonance` (0–1) | RBJ biquad, see above |
| Amp Env 1 | `env_attack`/`env_decay`/`env_sustain`/`env_release`, `env_sustain_on` | seconds-scale ADSR, sustain-hold optional |
| Filter Env 1 | `fenv_attack`/`fenv_decay`/`fenv_sustain`/`fenv_release`, `fenv_sustain_on`, `env_filter_mod` (±100%) | modulates Filter 1 cutoff |
| LFO | `lfo_target` (filter/amp/pitch), `lfo_speed` (0.1–10 Hz), `lfo_depth` (0–100%) | shared, not per-oscillator |
| Master | `output_gain` (−24…+12 dB) | applied per-voice before mixdown |
| Osc 2 | `osc2_on`, `osc2_waveform` (5 choices), `osc2_mult` (0.5×/1×/2×/4×), `osc2_phase` (0–360°), `osc2_mix_mode` (Sum/AM/FM/Ring), `osc2_mix_depth` | independently bypassable |
| Amp Env 2 | `osc2_env_on`, `osc2_env_attack/decay/sustain/release`, `osc2_env_sus_on` | bypassable — Osc 2 can also just follow Env 1 |
| Filter 2 | `osc2_flt_on`, `osc2_flt_cutoff`, `osc2_flt_reso`, `osc2_flt_mode` | independent second filter, bypassable |
| Filter Env 2 | `osc2_fenv_atk/dec/sus/rel`, `osc2_fenv_sus_on`, `osc2_fenv_depth` (±100%) | modulates Filter 2 cutoff |

### Presets: 42 factory presets, generated from XML — never hand-edit the header

`Source/Presets/Factory/*.xml` (Common's `<AudioPluginsPreset>` schema, one `<Parameter id=".." value=".."/>` per parameter) is the source of truth; `Source/FactoryPresets.h` is a **generated file** (`arcanistFactoryPresets()`, consumed by both `ArcanistUI`'s presets bar and `Tests/test_factorypresets.cpp`). The two `tools/*.py` scripts document the pipeline and are themselves the source of truth for *how* to regenerate:

- `tools/convert_presets_to_common_schema.py` — one-shot migration script that already ran once, converting the JUCE-era APVTS `<Parameters><PARAM id=".." value=".."/></Parameters>` XML into Common's schema in place. Preserved for reference/reruns, not part of the normal edit loop.
- `tools/generate_factory_presets_header.py` — run this after **any** change to `Source/Presets/Factory/*.xml` to regenerate `Source/FactoryPresets.h`.
- `tools/update_presets_osc2.py` — one-shot backfill that added `osc2_*` parameter values to presets authored before Osc 2 existed.

If you need to change a factory preset's values, edit the XML and rerun `generate_factory_presets_header.py` — do not hand-edit `FactoryPresets.h` directly.

### `RadioButtonGroup`: why it's local to Arcanist, not in `Common`

`Source/UI/RadioButtonGroup.h` composes N `Common::hui::dgl::Button` instances into a mutually-exclusive selector. `Common`'s `Button` is a plain momentary click with no persistent "selected" visual state of its own, so `RadioButtonGroup` owns that state (`selectedIndex_`) and re-palettes the active/inactive buttons on every change. This lives in Arcanist rather than `Common` per `Common`'s own design convention: adapter/composition code stays in the consuming plugin until a second or third consumer needs it too — Arcanist is currently the only plugin in this workspace using a choice parameter rendered as a button group rather than a dropdown.

### Embedded `MidiKeyboard`: 3 octaves from C3, fully chromatic

`ArcanistUI` embeds `Common::hui::dgl::MidiKeyboard(this, 3)` (3 octaves) shifted to start at MIDI 48 / C3 (`keyboard_->setOctaveShift(48)`), spanning C3–B5. Pad/drone chords are typically voiced from around middle C upward, unlike Pugilist's fixed low GM percussion notes (35–51, hence its C1 start with one fixed colour zone per pad). Arcanist is fully chromatic and polyphonic — any key plays any pitch through the same 16-voice pool — so **no note-zone tinting is used** (`setZones()` is skipped entirely); every key renders in the plain keyboard palette. `keyboard_->onNote` forwards straight to DPF's `UI::sendNote()`.

### `Common` `v0.4.0` dependency

`CMakeLists.txt` pins `AudioPluginsCommon` to `v0.4.0`, not `v0.3.0`. `v0.3.0` was cut before `Common::hui::dgl::MidiKeyboard` existed; the widget was cherry-picked onto `Common`'s `master` afterward and released as `v0.4.0` specifically so Arcanist (and any future keyboard-driven synth) could depend on a tagged release rather than an untagged commit. `v0.4.0` also carries `Common`'s `VuMeter` `Horizontal` orientation forward from `v0.3.0` unchanged — Arcanist doesn't use that orientation itself (its output meter is a plain `VuMeter`), but the tag is a superset, not a fork.

### `Source/_juce_reference/`

The pre-migration JUCE `PluginProcessor`/`PluginEditor`/`PresetManager`/UI sources are preserved unchanged under `Source/_juce_reference/` (including its own `UI/` subfolder). This is the porting reference the DPF port was checked against line-by-line — several comments in `ArcanistPluginAdapter.cpp`, `ArcanistUI.cpp`, and the preset-conversion tools explicitly cite it ("ported verbatim from Source/_juce_reference/PluginProcessor.cpp's ..."). It is not built by `CMakeLists.txt` and is not test-covered; treat it as read-only historical documentation, not a source directory to extend.

## Key design constraints

- **Synth, not effect** — `DISTRHO_PLUGIN_IS_SYNTH 1`, 0 audio inputs, 2 outputs, MIDI input only. Gets a Standalone (JACK/ALSA) target, unlike this workspace's effect plugins.
- **DSP is framework-free and unit-tested independently of DPF** (`Source/Synthesis/*.h/.cpp` + `Tests/*.cpp`, CTest) — `ArcanistPluginAdapter`/`ArcanistUI` are thin adapters with no DSP logic of their own, per this workspace's hexagonal-architecture convention.
- **Data-driven parameters**: adding/removing a parameter means editing `ArcanistParameters` (`DistrhoPluginInfo.h`) and `kArcanistParamSpecs` (`ArcanistParams.h`) — `initParameter()`/`getParameterValue()`/`setParameterValue()` never need touching, and `ArcanistUI` picks up the new control automatically via `ArcanistParamKind`.
- **Presets are generated, not hand-authored in C++** — see "Presets" above; `Source/FactoryPresets.h` regenerates from `Source/Presets/Factory/*.xml` via `tools/generate_factory_presets_header.py`.
- **DPF pinned to a commit SHA** (`4238e1c7f0351bbe488d79f0899c540543ac7583`, no tagged DPF releases exist) with the same `dpf-clap-state-chunked-read.patch` applied via `cmake/apply_patch.cmake` as Hex/Pugilist/Tank/Outflank, for consistency across the workspace's DPF-migrated plugins.
- **Sustained-pad envelope defaults carry over from the JUCE era**: attack/decay/release are seconds-scale (not ms), and `env_sustain_on`/`fenv_sustain_on` default on — pads sustain at a held level until note-off rather than decaying away, per this plugin's design brief.
