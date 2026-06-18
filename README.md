# PM0 — Soft Pads and Drones Synthesizer

A modern JUCE-based polyphonic synthesizer plugin for creating lush, sustained pad sounds ranging from synthetic string textures to virtual choir atmospheres. Designed for ambient, cinematic, orchestral, and electronic music production.

**Status**: Beta 0.1.0 | **Formats**: VST3, CLAP, Standalone | **Platforms**: Linux (macOS/Windows support planned)

---

## 🎵 Features

- **16-voice polyphony** — unlimited sustained chord depth
- **4 waveforms** — Sine, Triangle, Sawtooth, Square (expandable to wavetables)
- **Resonant filter** — Biquad low-pass with cutoff and resonance control
- **ADSR envelope** — Attack, Decay, Sustain, Release with filter modulation
- **Slow LFO** — 0.1–10 Hz sine wave for pad animation and movement
- **Velocity sensitivity** — Natural dynamics from MIDI input
- **Real-time metering** — Output level display with peak hold
- **Professional UI** — Clean rotary interface with organized parameter groups

---

## 🚀 Quick Start

### 1. **Download or Clone**

```bash
# If using the AudioPlugins workspace:
cd ~/Projects/AudioPlugins/PM0
```

### 2. **Build the Plugin**

```bash
# Configure (first run downloads ~200 MB of JUCE and dependencies)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build all targets (VST3, CLAP, Standalone)
cmake --build build --parallel
```

**Build time**: ~3–5 minutes (depends on system)

### 3. **Install Plugins** (Optional)

For use in your DAW:

```bash
# Install to user directories (no admin required)
./scripts/install.sh

# OR install system-wide (requires sudo)
./scripts/install.sh --system
```

Plugins are installed to:
- **VST3**: `~/.vst3/PM0.vst3` or `/usr/lib/vst3/`
- **CLAP**: `~/.clap/PM0.clap` or `/usr/lib/clap/`
- **Standalone**: `~/.local/bin/PM0`

### 4. **Launch Standalone**

```bash
./build/PM0_artefacts/Debug/Standalone/PM0
```

Or if installed:

```bash
PM0
```

---

## 📋 System Requirements

### Build Requirements
- **OS**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: GCC/Clang with C++20 support
- **Build tools**: CMake 3.22+, Ninja
- **Audio libraries**: ALSA dev, Jack dev

```bash
# One-time setup (Ubuntu/Debian):
sudo apt install cmake ninja-build build-essential git \
    libasound2-dev libjack-jackd2-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
    libxinerama-dev libxrandr-dev libxrender-dev \
    libfreetype-dev libfontconfig1-dev \
    libglu1-mesa-dev libwebkit2gtk-4.1-dev
```

### Runtime Requirements
- **DAW with VST3 or CLAP support** (Reaper, Ardour, Bitwig, Studio One, etc.)
- **MIDI input** (hardware keyboard, MIDI controller, or DAW's piano roll)
- **~15 MB disk space** (plugin files)
- **~50 MB RAM** per instance (worst case, 16 voices at 192 kHz)

---

## 🎹 Getting Started — First Sounds

### **In a DAW** (e.g., Reaper, Ardour, Bitwig)

1. **Add plugin** to a new track
   - VST3: Instruments → PM0
   - CLAP: Instruments → PM0

2. **Arm for MIDI** — ensure track receives MIDI input

3. **Play a note** — your keyboard should trigger the synth

4. **You should hear** a smooth, rising pad tone (default settings)

### **Standalone App**

1. **Open PM0** — window appears with default settings
2. **Configure audio** — click "Options" → select audio device & sample rate
3. **Play notes** — use your MIDI controller or on-screen keyboard (if available in future builds)

---

## 🎚️ Understanding the Controls

PM0 is organized into **6 logical sections**:

### **Oscillator** (Left)
```
Tune        -24 to +24 semitones   (global pitch shift)
Detune      -50 to +50 cents       (voice spreading, creates thickness)
```
**Use for**: Adjusting overall pitch or adding richness through detuned voices.

### **Filter** (Left-Center)
```
Cutoff      20 Hz to 20 kHz        (frequency brightness)
Resonance   0 to 100%              (peak emphasis at cutoff)
```
**Use for**: Shaping the spectral character of the pad.

### **Envelope** (Center, 5 Controls)
```
Attack      10 ms to 5 s           (fade-in time)
Decay       0 to 5 s               (fall to sustain)
Sustain     0 to 100%              (held level)
Release     0.5 to 10 s            (fade-out after note-off)
Filter Mod  -100 to +100%          (envelope modulation on filter cutoff)
```
**Use for**: Shaping dynamics and adding movement to the pad.

### **LFO** (Right)
```
Speed       0.1 to 10 Hz           (modulation frequency)
Depth       0 to 100%              (modulation amount on filter)
```
**Use for**: Adding slow, evolving texture to long pads.

### **Master** (Far Right)
```
Output Gain -24 to +12 dB          (trim / makeup gain)
```
**Use for**: Preventing clipping or boosting quiet patches.

---

## 🎵 Sound Design Quick Recipes

### **Warm String Pad** (Vangelis-style)
```
Oscillator:  Tune 0, Detune 20
Filter:      Cutoff 3000, Resonance 40%
Envelope:    Attack 0.3s, Decay 0.5s, Sustain 85%, Release 4s, Filter Mod 60%
LFO:         Speed 0.3 Hz, Depth 20%
Master:      Output Gain 0 dB
```
→ Play a chord and let it evolve. You'll hear the filter slowly opening from the envelope.

### **Ethereal Choir** (Enya-style)
```
Oscillator:  Tune 0, Detune 30 (helps voice separation)
Filter:      Cutoff 4500, Resonance 20%
Envelope:    Attack 1.5s, Decay 0.3s, Sustain 80%, Release 5s, Filter Mod -30%
LFO:         Speed 0.2 Hz, Depth 15%
Master:      Output Gain -3 dB
```
→ The slow attack creates a breathy, choir-like entrance. High detune prevents harshness.

### **Ambient Drone** (Minimal, evolving)
```
Oscillator:  Tune 0, Detune 5
Filter:      Cutoff 2000, Resonance 10%
Envelope:    Attack 3s, Decay 0s, Sustain 60%, Release 8s, Filter Mod 0%
LFO:         Speed 0.1 Hz, Depth 25%
Master:      Output Gain -6 dB (keeps headroom)
```
→ Slow, meditative. The LFO brings subtle movement. Release is long for droning.

### **Bright Synth Pad** (Modern, punchy)
```
Oscillator:  Tune 0, Detune 0
Filter:      Cutoff 6000, Resonance 60%
Envelope:    Attack 0.1s, Decay 1s, Sustain 70%, Release 2s, Filter Mod 80%
LFO:         Speed 1.5 Hz, Depth 30%
Master:      Output Gain 0 dB
```
→ Faster LFO and high resonance create a modern, animated texture.

---

## 🔧 Parameter Reference

| Section | Parameter | Min | Max | Unit | Default | Notes |
|---------|-----------|-----|-----|------|---------|-------|
| **Oscillator** | Tune | −24 | +24 | semitones | 0 | Global pitch |
| | Detune | −50 | +50 | cents | 0 | Voice spreading |
| **Filter** | Cutoff | 20 | 20000 | Hz | 4000 | Brightness |
| | Resonance | 0 | 100 | % | 0 | Peak emphasis |
| **Envelope** | Attack | 0.01 | 5 | s | 0.1 | Fade-in |
| | Decay | 0 | 5 | s | 0.5 | Fall to sustain |
| | Sustain | 0 | 100 | % | 80 | Held level |
| | Release | 0.5 | 10 | s | 3 | Fade-out |
| | Filter Mod | −100 | +100 | % | 0 | Envelope → Filter |
| **LFO** | Speed | 0.1 | 10 | Hz | 0.5 | Modulation rate |
| | Depth | 0 | 100 | % | 0 | Modulation amount |
| **Master** | Output Gain | −24 | +12 | dB | 0 | Trim level |

---

## 🎯 Tips for Best Results

### **Polyphony & Chord Playback**
- PM0 supports 16 simultaneous notes. Play chords freely without worrying about voice limits.
- **Older notes are naturally released** if you exceed 16 notes (voice stealing).

### **Avoiding Clicks & Artifacts**
- All parameters are **smoothed internally** — you won't hear zipper noise when adjusting controls in real-time.
- Set reasonable Attack times (> 10 ms) for smooth notes.

### **CPU Efficiency**
- Each active voice uses ~1% CPU at 44.1 kHz (varies by DAW).
- 8 simultaneous voices ≈ 8% CPU usage — very efficient.

### **Mixing in Your DAW**
- PM0 outputs in **stereo** (identical channels currently, stereo panning can be added in future versions).
- Use track faders and EQ to blend with other instruments.
- Consider adding reverb/delay on a send track for classic pad sound.

---

## 📂 File Structure

```
PM0/
├── README.md                 ← You are here
├── CLAUDE.md                 ← Developer documentation
├── CMakeLists.txt            ← Build configuration
├── Source/
│   ├── PluginProcessor.h/cpp ← JUCE AudioProcessor, APVTS setup
│   ├── PluginEditor.h/cpp    ← UI controls & metering
│   └── Synthesis/
│       ├── Voice.h/cpp       ← Polyphonic voice manager
│       ├── Oscillator.h/cpp  ← Waveform generator
│       ├── Filter.h/cpp      ← Biquad low-pass filter
│       ├── Envelope.h/cpp    ← ADSR envelope
│       └── LFO.h/cpp         ← Sine wave modulation
├── scripts/
│   ├── install.sh            ← Plugin installation helper
│   └── uninstall.sh          ← Plugin removal
└── build/                    ← Generated (after cmake build)
    └── PM0_artefacts/
        ├── VST3/PM0.vst3/    ← VST3 plugin
        ├── CLAP/PM0.clap     ← CLAP plugin
        └── Standalone/PM0    ← Standalone app
```

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| **CMake not found** | `sudo apt install cmake` |
| **JUCE not downloading** | Check internet connection; CMake will retry automatically |
| **Plugin not detected in DAW** | Rescan plugins in your DAW; ensure install path is correct |
| **No sound from plugin** | Verify DAW is receiving MIDI; check Output Gain isn't at minimum |
| **Audio crackling/clipping** | Lower Output Gain; reduce polyphony or Resonance setting |
| **High CPU usage** | Reduce polyphony; lower LFO depth; use fewer instances |

---

## 📊 Technical Specs

- **Architecture**: 16-voice polyphonic, voice-per-note routing
- **Oscillators**: 4 basic waveforms (expansion to wavetables planned)
- **Filter**: 2nd-order Butterworth biquad low-pass
- **Envelope**: ADSR with cubic interpolation for smooth transitions
- **LFO**: Sine wave, 0.1–10 Hz range
- **Buffer management**: Automatic block-wise processing
- **State persistence**: Full APVTS XML serialization (presets auto-save in DAW)

---

## 🔮 Planned Features

- [ ] Wavetable oscillator with morph control
- [ ] Multi-mode filter (high-pass, band-pass, notch)
- [ ] Dual LFO with shape selection
- [ ] Arpeggiator / chord memory
- [ ] Preset browser and management
- [ ] Macro knobs for quick sound design
- [ ] Stereo width control
- [ ] Built-in reverb/delay effects

---

## 📝 Building for macOS / Windows

Currently optimized for **Linux**. To build on other platforms:

- **macOS**: Edit `CMakeLists.txt` to enable AU format, requires Xcode
- **Windows**: Requires Visual Studio 2019+ or Clang toolchain

See `CLAUDE.md` for architecture details and cross-platform notes.

---

## 🤝 Contributing & Development

See `CLAUDE.md` for developer setup, architecture overview, and voice synthesis details.

For bug reports or feature requests, contact the developer or open an issue in the project repository.

---

## 📜 License

TBD

---

## 👤 Author

**Yvan Janet**  
Audio Software Engineer  
Specializing in DSP, synthesizer design, and real-time audio systems

---

## 🙏 Acknowledgments

- **JUCE Framework** — Powerful audio plugin development toolkit
- **CLAP Initiative** — Open-source plugin standard
- **Inspiration** — Vangelis, Enya, Ólafur Arnalds (masters of pad synthesis)

---

**Questions or feedback?** Refer to the developer documentation in `CLAUDE.md` or reach out to the maintainer.

**Happy pad designing! 🎹✨**
