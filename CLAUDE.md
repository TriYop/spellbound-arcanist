# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arcanist is a JUCE audio synthesizer plugin (VST3 / CLAP / Standalone) designed for generating soft pads and drones. The plugin synthesizes rich, sustained pad sounds ranging from synthetic string textures to virtual choir atmospheres, targeting ambient, cinematic, and orchestral production workflows. Example sonic targets: Vangelis and Enya-style sustained chords.

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

### Configure

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

First run downloads JUCE and clap-juce-extensions into `build/_deps/`.

### Build

```bash
cmake --build build --parallel              # all targets
cmake --build build --target Arcanist_Standalone # standalone only
cmake --build build --target Arcanist_VST3       # VST3 only
cmake --build build --target Arcanist_CLAP       # CLAP only
```

### Run standalone

```bash
./build/Arcanist_artefacts/Debug/Standalone/Arcanist
```

### Install plugins (Linux, dev build)

```bash
cp -r build/Arcanist_artefacts/Debug/VST3/Arcanist.vst3 ~/.vst3/
cp    build/Arcanist_artefacts/Debug/CLAP/Arcanist.clap ~/.clap/
```

### Create shippable tarball

```bash
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
cd build-release && cpack
```

## Architecture

### Source layout (proposed)

```
Source/
  PluginProcessor.h/.cpp    — AudioProcessor; APVTS parameter definitions; master 
                              voice management; routes processBlock() through 
                              oscillators → filters → envelopes → output
  PluginEditor.h/.cpp       — AudioProcessorEditor; UI controls for pad parameters
  Synthesis/
    Voice.h/.cpp            — Single polyphonic voice: oscillators, filter, envelope
    Oscillator.h/.cpp       — Wavetable or FM oscillator for rich pad textures
    Filter.h/.cpp           — Multimode filter (LP/BP/HP) with resonance
    Envelope.h/.cpp         — ADSR envelope generator for sustained pads
    LFO.h/.cpp              — Low-frequency modulation for pad movement
```

### Data flow

```
processBlock()
  ├─ for each active Voice:
  │    ├─ Oscillator::process()           [generates waveform]
  │    ├─ LFO::process()                  [modulates filter, amplitude]
  │    ├─ Filter::process()               [shapes spectrum]
  │    ├─ Envelope::process()             [applies ADSR, sustains pad]
  │    └─ output += voice_level * sample
  └─ apply master gain + output limiting
```

### Design considerations for pads

- **Sustained release**: Pads are typically long-release instruments; envelope release times should be measured in seconds, not milliseconds
- **Rich harmonics**: Use wavetable or FM oscillators to generate evolving spectral content; static oscillators feel static
- **Smooth modulation**: LFO should be slow (< 1 Hz) with long ramps to avoid choppiness in long tones
- **Filter movement**: Envelope or LFO modulation of filter frequency adds life to sustained pads
- **Polyphony**: Plan for 8–16 voices minimum to support chord playback (e.g., extended orchestral voicings)
- **Voice stealing**: Oldest note release strategy (not newest) preserves playing notes when polyphony is exceeded

### Parameters (planned APVTS)

Typical controls for a pad synth; refine as design progresses:

| Category | Parameter | Range | Purpose |
|----------|-----------|-------|---------|
| **Oscillator** | waveform | select | sine, triangle, sawtooth, custom |
| | tune | ±2 octaves | pitch shift per voice |
| | detune | ±50 cents | voice spreading for thickness |
| **Filter** | cutoff | 20 Hz–20 kHz | spectral brightness |
| | resonance | 0–1 | filter emphasis |
| | mode | select | low-pass, band-pass, high-pass |
| **Envelope** | attack | 10 ms–5 s | fade-in time |
| | decay | 0–5 s | fall to sustain |
| | sustain | 0–100 % | held level during note |
| | release | 0.5–10 s | fade-out after note-off |
| | filter_mod | ±100 % | envelope modulation depth on filter |
| **LFO** | speed | 0.1–10 Hz | LFO frequency |
| | depth | 0–100 % | LFO modulation amount (filter or amp) |
| | target | select | filter, amplitude, pitch |
| **Master** | polyphony | 1–16 | max simultaneous notes |
| | output_gain | −24 to +12 dB | trim |
| | mix | 0–100 % | dry/wet (if reverb/FX added later) |

### Key design constraints

- **Polyphonic voice management**: Allocate a voice pool in `prepare()`; reuse voices on note-off (FIFO or LRU)
- **Parameter smoothing**: All modulations should be smoothed per-sample to prevent clicks when changing parameters in real-time
- **Sustain behavior**: Pads typically sustain at full volume until note-off; set sensible defaults (e.g., sustain = 100%, release = 3 s)
- **CPU efficiency**: For 16 voices × multi-stage processing, monitor CPU with profiling during development
