#!/usr/bin/env python3
"""
Update factory presets to make creative use of Osc 2.
Each preset gets Osc 2 settings tailored to its sonic character.

Mix modes: 0=SUM, 1=AM, 2=FM, 3=RING
Waveforms:  0=Sine, 1=Tri, 2=Saw, 3=Sqr, 4=Noise
Multipliers: 0=x0.5, 1=x1, 2=x2, 3=x4

Output gain adjustments compensate for added signal level:
  SUM depth=20  → +1.7dB  → reduce gain ~2
  SUM depth=30  → +2.3dB  → reduce gain ~2
  SUM depth=38  → +2.8dB  → reduce gain ~3
  AM  depth=35  → +2.6dB  → reduce gain ~3
  AM  depth=45  → +3.2dB  → reduce gain ~3
  AM  depth=65+ → +4dB+   → reduce gain ~4-5
  RING          → perceived louder or quieter → compensate +2
  FM            → same loudness, just different harmonics → no change
"""

import re
import os
from pathlib import Path

PRESETS_DIR = Path(__file__).parent.parent / "Source" / "Presets" / "Factory"

# Per-preset Osc 2 updates.
# Only keys listed here are changed; all others keep their current value.
# output_gain is adjusted to compensate for added Osc 2 signal level.
UPDATES = {
    # ─────────────────────────────────────────────────────────────────────────
    # 001 CelestialDrift — Sine pad, filter LFO
    # FM x2 with SAW: evolving spectral shimmer without adding volume
    "001_CelestialDrift.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "20",
        # FM doesn't add volume — no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 002 HeavenlyGlow — Slow sine, warm
    # SUM sub-octave SINE at 90° phase: adds body and warmth
    "002_HeavenlyGlow.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "0",           # x0.5 (-1 octave)
        "osc2_phase": "90",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "22",
        "output_gain": "-4",        # was -2, +2dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 003 CloudNine — Triangle, wide detune, amplitude LFO
    # SUM x2 TRI through low-pass: adds airy upper-octave shimmer
    "003_CloudNine.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "18",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "5500",
        "osc2_flt_reso": "0.15",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-4",        # was -3, +1dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 004 Starlight — Sine, resonant filter LFO
    # RING x2 SINE: sum=3f, diff=1f → adds perfect 5th partial = bell sparkle
    "004_Starlight.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "3",      # RING
        "osc2_mix_depth": "100",   # depth unused in RING but set for clarity
        "output_gain": "2",         # was 0, ring mod reduces perceived level → boost
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 005 Nebula — Triangle, slow, low cutoff, muted
    # FM x1 SAW through LP 600Hz: smooth FM via low-pass filtered modulator,
    # adds harmonic complexity without brightness
    "005_Nebula.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "10",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "600",
        "osc2_flt_reso": "0.1",
        "osc2_flt_mode": "0",      # LP
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 006 InfiniteSpace — Sine, very wide detune, very slow
    # SUM x0.5 SINE 180° phase: sub-octave adds spatial depth
    "006_InfiniteSpace.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "0",           # x0.5
        "osc2_phase": "180",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "18",
        "output_gain": "-6",        # was -5, +1dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 007 MidnightCathedral — SAW, resonant, dark, strong filter env
    # SUM x1 TRI 120° phase: triangle fills harmonic gaps in the SAW,
    # darkens and enriches cathedral resonance
    "007_MidnightCathedral.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "1",           # x1
        "osc2_phase": "120",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "28",
        "output_gain": "-4",        # was -2, +2dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 008 OminousSwell — SAW, very resonant, heavy filter env
    # SUM x0.5 SAW with slow-attack envelope: sub-octave SAW swells in
    # slightly after the main, adding an ominous low-end rumble
    "008_OminousSwell.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "0",           # x0.5
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "38",
        "osc2_env_on": "1",
        "osc2_env_attack": "0.6",  # delayed swell
        "osc2_env_decay": "2.0",
        "osc2_env_sustain": "0.7",
        "osc2_env_release": "5.0",
        "osc2_env_sus_on": "1",    # ADSR
        "output_gain": "-3",        # was -1, +3dB from SUM (env-gated, partial)
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 009 CinematicStrings — SAW, mild detune, pitch LFO
    # SUM x1 TRI: classic string machine SAW+TRI layer, adds warmth
    "009_CinematicStrings.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "35",
        "output_gain": "-3",        # was 0, +3dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 010 DeepMystery — Triangle, amplitude LFO, dark
    # FM x1 SINE 45° phase: gentle FM adds mystery without brightening
    "010_DeepMystery.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "1",           # x1
        "osc2_phase": "45",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "12",
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 011 ShadowedDream — Triangle, very wide detune, amplitude LFO
    # SUM x2 TRI through LP: upper-octave triangle adds dream shimmer
    "011_ShadowedDream.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "18",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "4500",
        "osc2_flt_reso": "0.1",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-5",        # was -4, +1dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 012 RichStrings — SAW, detune=10, pitch LFO
    # SUM x0.5 SAW: sub-octave dramatically thickens the string foundation
    "012_RichStrings.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "0",           # x0.5
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "32",
        "output_gain": "-3",        # was -1, +2.5dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 013 VelvetWarmth — Triangle, warm filter
    # SUM x1 SINE 90° phase: quadrature sine creates subtle beating and depth
    "013_VelvetWarmth.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "1",           # x1
        "osc2_phase": "90",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "28",
        "output_gain": "-4",        # was -2, +2dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 014 ChoirShimmer — Triangle, filter LFO
    # AM x2 TRI through LP: AM at 2× creates sidebands at 3f (sideband = f±2f),
    # adding vowel-like shimmer characteristic of choir voices
    "014_ChoirShimmer.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "1",      # AM
        "osc2_mix_depth": "35",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "5000",
        "osc2_flt_reso": "0.1",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-4",        # was -1, +2.6dB from AM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 015 GoldenHour — Triangle, amplitude LFO, warm
    # SUM x0.5 SINE: sub-fundamental adds golden warmth and depth
    "015_GoldenHour.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "0",           # x0.5
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "22",
        "output_gain": "-2",        # was 0, +2dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 016 OrganicBloom — SAW, very wide detune, filter LFO
    # FM x2 SAW: FM at 2× adds rich, naturally evolving upper harmonics
    "016_OrganicBloom.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "18",
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 017 CrystalBell — Sine, fast attack=0.05, resonant
    # AM x2 SINE with ADR envelope: AM at 2× creates 3rd partial (bell character);
    # envelope shapes the metallic ring that decays to clean sine
    "017_CrystalBell.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "1",      # AM
        "osc2_mix_depth": "65",
        "osc2_env_on": "1",
        "osc2_env_attack": "0.001",
        "osc2_env_decay": "1.5",
        "osc2_env_sustain": "0.0",
        "osc2_env_release": "2.5",
        "osc2_env_sus_on": "0",    # ADR (no sustain — fades out)
        "output_gain": "-4",        # was 0, +4dB peak from AM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 018 SparklingSunrise — SAW, bright, pitch LFO
    # SUM x4 SINE through high LP: +2 octave sine adds ultra-bright air
    "018_SparklingSunrise.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "3",           # x4
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "15",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "9000",
        "osc2_flt_reso": "0.1",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "0",         # was 1, -1dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 019 LuminousPad — Triangle, bright, amplitude LFO
    # FM x1 SAW: FM with SAW modulator enriches the triangle with complex harmonics
    "019_LuminousPad.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "15",
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 020 PrismaticGlow — SAW, bright filter LFO
    # SUM x4 SINE through 10kHz LP: extreme upper-octave shimmer for prismatic quality
    "020_PrismaticGlow.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "3",           # x4
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "12",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "10000",
        "osc2_flt_reso": "0.1",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-1",        # was 0, +1dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 021 MorphingLandscape — SAW, very wide detune, heavy filter env
    # FM x1 SAW through LP 3000Hz with filter env: filtered FM modulator
    # creates morphing timbral shifts that compound the landscape's evolution
    "021_MorphingLandscape.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "28",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "3000",
        "osc2_flt_reso": "0.2",
        "osc2_flt_mode": "0",      # LP
        "osc2_fenv_atk": "0.5",
        "osc2_fenv_dec": "2.0",
        "osc2_fenv_sus": "0.4",
        "osc2_fenv_rel": "3.0",
        "osc2_fenv_sus_on": "1",   # ADSR
        "osc2_fenv_depth": "50",   # modulator LP opens on note-on, morphs FM timbre
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 022 BreathingPulse — Triangle, heavy amplitude LFO depth=60
    # AM x1 SINE 90° phase: self-AM at fundamental creates 2nd harmonic enrichment;
    # combined with the LFO's breathing, produces complex rhythmic texture
    "022_BreathingPulse.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "1",           # x1
        "osc2_phase": "90",
        "osc2_mix_mode": "1",      # AM
        "osc2_mix_depth": "45",
        "output_gain": "-4",        # was -1, +3dB peak from AM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 023 SpiralingCosmos — SAW, detune=25, pitch LFO
    # RING x1 SAW 180°: ring mod of two SAWs at same freq but inverted phase
    # produces even harmonics only (2f, 4f...) — otherworldly, cosmic texture
    "023_SpiralingCosmos.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "1",           # x1
        "osc2_phase": "180",
        "osc2_mix_mode": "3",      # RING
        "osc2_mix_depth": "100",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "4000",
        "osc2_flt_reso": "0.25",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-1",        # was -3, ring mod → boost +2
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 024 VintageSynthPad — Square, high resonance=0.7, no detune
    # SUM x1 SAW: classic SAW+SQR poly-synth layering, the vintage sound
    "024_VintageSynthPad.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "35",
        "output_gain": "-4",        # was -1, +3dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 025 BladeRunnerNight — SAW, very slow attack=2.5, filter mod=-20
    # FM x1 SINE through LP 800Hz: low-pass filtered sine FM modulator adds
    # subtle metallic texture that stays dark (matching the -20 filter mod)
    "025_BladeRunnerNight.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "22",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "800",
        "osc2_flt_reso": "0.15",
        "osc2_flt_mode": "0",      # LP
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 026 AlphaCentauri — SAW, very long release=8, gradual
    # SUM x0.5 SAW: sub-octave gives this celestial pad stellar weight
    "026_AlphaCentauri.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "0",           # x0.5
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "30",
        "output_gain": "-2",        # was 0, +2.3dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 027 CS80Strings — SAW, very wide detune=18, amplitude LFO
    # SUM x1 SINE 120°: the CS-80 sound comes from layering a soft sine against
    # the SAW — the 120° phase creates subtle phase beating, the "breath"
    "027_CS80Strings.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "1",           # x1
        "osc2_phase": "120",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "42",
        "output_gain": "-3",        # was 0, +3.2dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 028 EpicCinema — Triangle, very long, filter env=40, pitch LFO
    # SUM x4 TRI with ADR envelope + LP: +2 octave triangle attack burst;
    # provides an epic harmonic "hit" at note onset that fades quickly
    "028_EpicCinema.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "3",           # x4
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "18",
        "osc2_env_on": "1",
        "osc2_env_attack": "0.1",
        "osc2_env_decay": "0.5",
        "osc2_env_sustain": "0.0",
        "osc2_env_release": "3.0",
        "osc2_env_sus_on": "0",    # ADR
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "8000",
        "osc2_flt_reso": "0.1",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-1",        # was 0, partial (+1 at peak, fades)
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 029 StringMachine — SAW, mild detune=6, fast attack=0.35
    # SUM x1 TRI: vintage string machine used SAW+TRI for warmth; adds body
    "029_StringMachine.xml": {
        "osc2_on": "1",
        "osc2_waveform": "1",      # TRI
        "osc2_mult": "1",           # x1
        "osc2_phase": "0",
        "osc2_mix_mode": "0",      # SUM
        "osc2_mix_depth": "38",
        "output_gain": "-3",        # was 0, +3dB from SUM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 030 OceanicSweep — SAW, heavy filter LFO depth=70
    # AM x1 SINE 90°: self-AM creates complex beating and sideband movement
    # that adds to the oceanic sweeping character
    "030_OceanicSweep.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "1",           # x1
        "osc2_phase": "90",
        "osc2_mix_mode": "1",      # AM
        "osc2_mix_depth": "45",
        "output_gain": "-3",        # was 0, +3.2dB peak from AM
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 031 CosmicHorizon — SAW, very long (attack=2.5, release=9), filter LFO
    # FM x2 SINE with long ADSR envelope: FM brightness that slowly fades in
    # with the cosmic swell and gently decays — matches the vast timescale
    "031_CosmicHorizon.xml": {
        "osc2_on": "1",
        "osc2_waveform": "0",      # SINE
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "2",      # FM
        "osc2_mix_depth": "18",
        "osc2_env_on": "1",
        "osc2_env_attack": "2.5",  # matches main envelope attack
        "osc2_env_decay": "3.0",
        "osc2_env_sustain": "0.4",
        "osc2_env_release": "6.0",
        "osc2_env_sus_on": "1",    # ADSR
        # FM → no output_gain change
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 032 ElectricNight — SAW, filter mod=-25, very long
    # RING x2 SAW through LP 2500Hz: ring mod creates electric/metallic sidebands;
    # LP tames harshness while keeping the nighttime edge
    "032_ElectricNight.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "3",      # RING
        "osc2_mix_depth": "100",
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "2500",
        "osc2_flt_reso": "0.3",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "1",         # was -1, ring mod → boost +2
    },

    # ─────────────────────────────────────────────────────────────────────────
    # 033 LaserHarp — SAW, instant attack=0.001, filter mod=80 (dramatic sweep)
    # AM x2 SAW with very fast ADR + LP: AM at 2× creates metallic attack partials
    # that burst then decay, layering onto the existing filter sweep for a zap+tail
    "033_LaserHarp.xml": {
        "osc2_on": "1",
        "osc2_waveform": "2",      # SAW
        "osc2_mult": "2",           # x2
        "osc2_phase": "0",
        "osc2_mix_mode": "1",      # AM
        "osc2_mix_depth": "70",
        "osc2_env_on": "1",
        "osc2_env_attack": "0.001",
        "osc2_env_decay": "0.15",
        "osc2_env_sustain": "0.0",
        "osc2_env_release": "1.0",
        "osc2_env_sus_on": "0",    # ADR
        "osc2_flt_on": "1",
        "osc2_flt_cutoff": "3000",
        "osc2_flt_reso": "0.25",
        "osc2_flt_mode": "0",      # LP
        "output_gain": "-5",        # was -1, +4.6dB peak from AM (brief)
    },
}


def update_preset(filepath: Path, changes: dict) -> None:
    content = filepath.read_text(encoding="utf-8")
    for param_id, new_value in changes.items():
        pattern = rf'(<PARAM id="{re.escape(param_id)}" value=")[^"]*(")'
        replacement = rf'\g<1>{new_value}\g<2>'
        new_content, count = re.subn(pattern, replacement, content)
        if count == 0:
            print(f"  WARNING: param '{param_id}' not found in {filepath.name}")
        else:
            content = new_content
    filepath.write_text(content, encoding="utf-8")


def main():
    if not PRESETS_DIR.exists():
        print(f"ERROR: Presets directory not found: {PRESETS_DIR}")
        return

    updated = 0
    skipped = 0

    for filename, changes in UPDATES.items():
        filepath = PRESETS_DIR / filename
        if not filepath.exists():
            print(f"SKIP (not found): {filename}")
            skipped += 1
            continue
        print(f"Updating {filename} ({len(changes)} params)")
        update_preset(filepath, changes)
        updated += 1

    print(f"\nDone: {updated} presets updated, {skipped} skipped.")


if __name__ == "__main__":
    main()
