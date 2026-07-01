#!/usr/bin/env python3
"""
Analyze a WAV sample and suggest Arcanist synthesis parameters.
Uses only numpy + scipy (no librosa needed).
"""

import sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import find_peaks, butter, sosfilt

# ─── Load ─────────────────────────────────────────────────────────────────────

path = sys.argv[1] if len(sys.argv) > 1 else ""
if not path:
    print("Usage: analyze_sample.py <file.wav>")
    sys.exit(1)

sr, data = wavfile.read(path)
if data.ndim > 1:
    data = data[:, 0]                       # take left channel
data = data.astype(np.float64)
data /= np.iinfo(np.int16).max if data.max() < 33000 else np.iinfo(np.int32).max

duration = len(data) / sr
print(f"\n{'='*60}")
print(f"  File   : {path.split('/')[-1]}")
print(f"  Length : {duration*1000:.0f} ms   SR: {sr} Hz   Samples: {len(data)}")
print(f"{'='*60}\n")

# ─── Amplitude envelope ────────────────────────────────────────────────────────

frame = max(1, int(sr * 0.005))            # 5 ms frames
hop   = max(1, frame // 2)
rms   = np.array([
    np.sqrt(np.mean(data[i:i+frame]**2))
    for i in range(0, len(data)-frame, hop)
])
t_env = np.arange(len(rms)) * hop / sr

peak_rms  = rms.max()
peak_idx  = rms.argmax()
peak_time = t_env[peak_idx]

# Attack: time from 10% to 90% of peak
thr10 = 0.1 * peak_rms
thr90 = 0.9 * peak_rms
cross10 = np.where(rms >= thr10)[0]
cross90 = np.where(rms >= thr90)[0]
attack_s = (t_env[cross90[0]] - t_env[cross10[0]]) if len(cross10) and len(cross90) else 0.001
attack_s = max(0.001, attack_s)

# After peak: find where rms falls to sustain then to near-silence
tail = rms[peak_idx:]
t_tail = t_env[peak_idx:]

# Sustain estimate: level the tail settles to (median of last 30%)
last30 = tail[int(len(tail)*0.7):]
sustain_level = float(np.median(last30)) / peak_rms if len(last30) else 0.0
sustain_level = np.clip(sustain_level, 0.0, 1.0)

# Decay: time from peak to sustain level
sus_thr = sustain_level * peak_rms + 1e-9
decay_cross = np.where(tail <= sus_thr + 0.02 * peak_rms)[0]
decay_s = float(t_tail[decay_cross[0]] - peak_time) if len(decay_cross) else 0.5
decay_s = max(0.05, decay_s)

# Release: estimate from tail slope after sustain
# (for a short sample we approximate from the final fall)
if sustain_level > 0.01:
    # Estimate: time for sustain level to fall to near 0
    near_zero = np.where(tail <= 0.02 * peak_rms)[0]
    if len(near_zero):
        total_fall = t_tail[near_zero[0]] - peak_time
        release_s  = max(0.1, total_fall - decay_s)
    else:
        release_s = duration * 1.5   # sample is still ringing at end
else:
    release_s = max(0.5, duration - peak_time - decay_s)

release_s = np.clip(release_s, 0.1, 10.0)
decay_s   = np.clip(decay_s,   0.05, 5.0)

print("── Amplitude Envelope ──────────────────────────────────")
print(f"  Attack  : {attack_s*1000:.1f} ms  →  env_attack  = {attack_s:.3f}")
print(f"  Decay   : {decay_s*1000:.0f} ms  →  env_decay   = {decay_s:.2f}")
print(f"  Sustain : {sustain_level*100:.0f} %   →  env_sustain = {sustain_level:.2f}")
print(f"  Release : {release_s*1000:.0f} ms  →  env_release = {release_s:.1f}")
sus_on = 1 if sustain_level > 0.05 else 0
print(f"  Mode    : {'ADSR' if sus_on else 'ADR'}          →  env_sustain_on = {sus_on}")

# ─── Spectral analysis ─────────────────────────────────────────────────────────

# Use a window around the first 100 ms (attack character)
win_len = min(len(data), int(sr * 0.1))
window  = np.hanning(win_len)
fft     = np.abs(np.fft.rfft(data[:win_len] * window))
freqs   = np.fft.rfftfreq(win_len, 1.0/sr)

fft_db  = 20 * np.log10(fft / (fft.max() + 1e-12) + 1e-12)

# Fundamental: highest peak in 20–1000 Hz range
f_lo, f_hi = 20, 1000
mask = (freqs >= f_lo) & (freqs <= f_hi)
peaks, props = find_peaks(fft[mask], height=fft[mask].max() * 0.05, distance=int(f_lo//(freqs[1]-freqs[0])+1))
f_mask = freqs[mask]
f0 = float(f_mask[peaks[np.argmax(props['peak_heights'])]]) if len(peaks) else 55.0

print(f"\n── Spectral Analysis (attack window) ──────────────────")
print(f"  Fundamental : {f0:.1f} Hz")

# Harmonic analysis: check energy at integer multiples of f0
harmonics = []
harm_amps  = []
for n in range(1, 16):
    fn = f0 * n
    if fn > sr / 2 * 0.95:
        break
    idx = int(fn * win_len / sr)
    idx = np.clip(idx, 0, len(fft)-1)
    radius = max(1, int(0.3 * f0 * win_len / sr))
    lo, hi = max(0, idx-radius), min(len(fft), idx+radius+1)
    peak_here = float(fft[lo:hi].max())
    harmonics.append(n)
    harm_amps.append(peak_here)

harm_amps = np.array(harm_amps)
harm_norm = harm_amps / (harm_amps[0] + 1e-12)

print(f"  Harmonic series (normalised to fundamental):")
for n, a in zip(harmonics[:8], harm_norm[:8]):
    bar = "█" * int(a * 20)
    print(f"    H{n:2d} ({f0*n:6.1f} Hz): {a:.3f}  {bar}")

# Waveform identification from harmonic ratios
odd_sum  = sum(harm_norm[n-1] for n in harmonics if n % 2 != 0)
even_sum = sum(harm_norm[n-1] for n in harmonics if n % 2 == 0)
odd_ratio = odd_sum / (odd_sum + even_sum + 1e-9)

# Expected slopes: sine=only H1, tri=odd/n^2, saw=all/n, square=odd/n
# Compute fit scores
h_arr   = np.array(harmonics[:min(8, len(harmonics))], dtype=float)
a_arr   = harm_norm[:len(h_arr)]

sine_fit = np.corrcoef([1.0] + [0.0]*7, a_arr.tolist() + [0]*(8-len(a_arr)))[0,1]

saw_ideal   = 1.0 / h_arr
tri_ideal   = np.where(h_arr % 2 != 0, 1.0 / h_arr**2, 0.0)
sqr_ideal   = np.where(h_arr % 2 != 0, 1.0 / h_arr,    0.0)

def norm_corr(a, b):
    a, b = np.array(a, float), np.array(b, float)
    if a.std() < 1e-9 or b.std() < 1e-9:
        return 0.0
    return float(np.corrcoef(a, b)[0, 1])

score_saw = norm_corr(saw_ideal, a_arr)
score_tri = norm_corr(tri_ideal, a_arr)
score_sqr = norm_corr(sqr_ideal, a_arr)

# Also check if near-pure sine (very low harmonic energy above H1)
h2plus = harm_norm[1:].sum()
score_sine = 1.0 - min(1.0, h2plus / max(0.01, harm_norm[0]))

scores = {"Sine": score_sine, "Triangle": score_tri, "Sawtooth": score_saw, "Square": score_sqr}
best_wave = max(scores, key=scores.get)
wave_idx  = {"Sine": 0, "Triangle": 1, "Sawtooth": 2, "Square": 3}[best_wave]

print(f"\n  Waveform scores: " + "  ".join(f"{k}={v:.2f}" for k,v in scores.items()))
print(f"  → Best match  : {best_wave} (osc_waveform={wave_idx})")

# ─── Filter estimate ───────────────────────────────────────────────────────────

# Spectral centroid of sustained portion
sustain_start = int(sr * (peak_time + decay_s * 0.8))
sustain_start = min(sustain_start, max(0, len(data)-512))
sus_win = min(max(512, len(data) - sustain_start), int(sr * 0.05))
if sus_win >= 64 and sustain_start + sus_win <= len(data):
    sus_fft   = np.abs(np.fft.rfft(data[sustain_start:sustain_start+sus_win]))
    sus_freqs = np.fft.rfftfreq(sus_win, 1.0/sr)
    centroid  = float(np.sum(sus_freqs * sus_fft) / (sus_fft.sum() + 1e-12))
else:
    centroid = 3000.0

# Rolloff frequency (95% of spectral energy)
cumsum = np.cumsum(fft**2)
rolloff_idx = np.searchsorted(cumsum, 0.85 * cumsum[-1])
rolloff = float(freqs[rolloff_idx]) if rolloff_idx < len(freqs) else 4000.0

filter_cutoff = float(np.clip(rolloff * 0.7, 200, 18000))

# Resonance: check if there's a spectral peak just below cutoff
peak_region = (freqs > filter_cutoff * 0.6) & (freqs < filter_cutoff * 1.2)
if peak_region.any():
    region_fft  = fft[peak_region]
    region_mean = region_fft.mean()
    region_max  = region_fft.max()
    resonance   = float(np.clip((region_max / (region_mean + 1e-12) - 1.0) * 0.15, 0.0, 0.9))
else:
    resonance = 0.2

print(f"\n── Filter Estimate ─────────────────────────────────────")
print(f"  Spectral centroid (sustained) : {centroid:.0f} Hz")
print(f"  85% rolloff                   : {rolloff:.0f} Hz")
print(f"  → filter_cutoff    = {filter_cutoff:.0f}")
print(f"  → filter_resonance = {resonance:.2f}")
print(f"  → filter_mode      = 0 (LP)")

# ─── FM / inharmonic transient detection ──────────────────────────────────────

# Compare attack vs sustained spectral centroid: large shift → FM transient
atk_win = min(int(sr * 0.02), len(data))
atk_fft = np.abs(np.fft.rfft(data[:atk_win]))
atk_f   = np.fft.rfftfreq(atk_win, 1.0/sr)
atk_centroid = float(np.sum(atk_f * atk_fft) / (atk_fft.sum() + 1e-12))

brightness_ratio = atk_centroid / max(centroid, 50.0)

# Check for inharmonic energy (energy not near harmonic positions)
inharmonic_energy = 0.0
harmonic_energy   = 0.0
for i, f in enumerate(freqs[:len(fft)]):
    if f < 20:
        continue
    ratio = f / max(f0, 1e-3)
    nearest = round(ratio)
    if nearest < 1:
        continue
    deviation = abs(ratio - nearest)
    if deviation < 0.08:
        harmonic_energy += fft[i]**2
    else:
        inharmonic_energy += fft[i]**2
inharm_ratio = inharmonic_energy / (harmonic_energy + inharmonic_energy + 1e-12)

print(f"\n── FM / Transient Character ────────────────────────────")
print(f"  Attack centroid : {atk_centroid:.0f} Hz")
print(f"  Sustain centroid: {centroid:.0f} Hz")
print(f"  Brightness ratio (atk/sus): {brightness_ratio:.2f}")
print(f"  Inharmonic energy fraction : {inharm_ratio:.3f}")

fm_on    = brightness_ratio > 1.5 or inharm_ratio > 0.15
fm_depth = int(np.clip((brightness_ratio - 1.0) * 25 + inharm_ratio * 60, 5, 80))
if fm_on:
    print(f"  → FM transient detected: osc2_mix_mode=2 (FM), osc2_mix_depth={fm_depth}")
    print(f"     osc2_env_on=1, osc2_env_sus_on=0 (ADR), osc2_env_decay≈{min(decay_s, 0.4):.2f}")
else:
    print(f"  → No strong FM signature")

# ─── Detune / chorus estimate ─────────────────────────────────────────────────

# Look for slight pitch spread (beating) in partials — sign of detuned unison
if len(harm_amps) > 2:
    # High fundamental harmonics have wider relative spread if detuned
    beat_indicator = harm_norm[1:4].std() if len(harm_norm) > 4 else 0
    detune_cents = int(np.clip(beat_indicator * 30, 0, 30))
else:
    detune_cents = 5
print(f"\n── Detuning ────────────────────────────────────────────")
print(f"  Estimated spread: osc_detune ≈ {detune_cents}")

# ─── LFO estimate ─────────────────────────────────────────────────────────────

# Look for amplitude modulation in the envelope (periodic variation in RMS)
if len(rms) > 32:
    rms_norm = rms / (rms.max() + 1e-12)
    # Smooth envelope and look for residual oscillation
    kernel   = np.ones(8) / 8
    rms_smooth = np.convolve(rms_norm, kernel, mode='same')
    rms_residual = rms_norm - rms_smooth
    rms_fft  = np.abs(np.fft.rfft(rms_residual))
    rms_f    = np.fft.rfftfreq(len(rms_residual), hop / sr)
    lfo_mask = (rms_f > 0.3) & (rms_f < 12.0)
    if lfo_mask.any() and rms_fft[lfo_mask].max() > rms_fft[lfo_mask].mean() * 3:
        lfo_speed = float(rms_f[lfo_mask][rms_fft[lfo_mask].argmax()])
        lfo_depth = int(np.clip(rms_fft[lfo_mask].max() / (rms_fft.mean() + 1e-9) * 5, 0, 60))
    else:
        lfo_speed = 0.5
        lfo_depth = 0
else:
    lfo_speed, lfo_depth = 0.5, 0

print(f"\n── LFO ─────────────────────────────────────────────────")
print(f"  lfo_speed = {lfo_speed:.2f},  lfo_depth = {lfo_depth}")

# ─── Output gain estimate ─────────────────────────────────────────────────────

# Target perceived loudness -6 dBFS for the peak hit
peak_amp  = float(np.abs(data).max())
target_db = -6.0
current_db = 20 * np.log10(peak_amp + 1e-12)
gain_db = np.clip(target_db - current_db, -24, 12)

print(f"\n── Output Gain ─────────────────────────────────────────")
print(f"  Peak: {current_db:.1f} dBFS  →  output_gain ≈ {gain_db:.0f}")

# ─── Summary preset ───────────────────────────────────────────────────────────

print(f"\n{'='*60}")
print("  SUGGESTED Arcanist PRESET PARAMETERS")
print(f"{'='*60}")
print(f"  osc_waveform    = {wave_idx}      ({best_wave})")
print(f"  osc_detune      = {detune_cents}")
print(f"  filter_cutoff   = {filter_cutoff:.0f}")
print(f"  filter_resonance= {resonance:.2f}")
print(f"  filter_mode     = 0 (LP)")
print(f"  env_attack      = {attack_s:.3f}")
print(f"  env_decay       = {decay_s:.2f}")
print(f"  env_sustain     = {sustain_level:.2f}")
print(f"  env_release     = {release_s:.1f}")
print(f"  env_sustain_on  = {sus_on}")
print(f"  env_filter_mod  = 0   (no sweep detected, adjust manually)")
print(f"  lfo_speed       = {lfo_speed:.2f}")
print(f"  lfo_depth       = {lfo_depth}")
print(f"  lfo_target      = 0 (filter)")
print(f"  output_gain     = {gain_db:.0f}")
if fm_on:
    print(f"  -- Osc 2 (FM transient) --")
    print(f"  osc2_on         = 1")
    print(f"  osc2_waveform   = 2 (Saw)")
    print(f"  osc2_mult       = 2 (x2)")
    print(f"  osc2_mix_mode   = 2 (FM)")
    print(f"  osc2_mix_depth  = {fm_depth}")
    print(f"  osc2_env_on     = 1")
    print(f"  osc2_env_attack = 0.001")
    print(f"  osc2_env_decay  = {min(decay_s, 0.4):.2f}")
    print(f"  osc2_env_sustain= 0.0")
    print(f"  osc2_env_release= {release_s * 0.3:.1f}")
    print(f"  osc2_env_sus_on = 0 (ADR)")
else:
    print(f"  osc2_on         = 0")
print(f"{'='*60}\n")
