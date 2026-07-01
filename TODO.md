# Arcanist TODO

## Synthesis

- [x] **Unison / second oscillator per voice** — two `Oscillator` instances per `Voice`, detuned at `+detune` / `-detune` cents respectively, mixed additively and normalized.

- [x] **LFO target selection** — `lfo_target` parameter (Filter / Amplitude / Pitch). Filter modulates cutoff, Amplitude applies tremolo (±50% at full depth), Pitch applies vibrato (±50 cents at full depth). LFO rate bug also fixed (was running at 1/sampleRate rate instead of block rate).

- [x] **Filter mode** — `filter_mode` parameter (Low Pass / Band Pass / High Pass) with correct RBJ biquad coefficients for each mode.

## Synthesis

- [ ] **Velocity curve** — current response is linear (velocity/127); a power curve (e.g. `v²`) would make soft playing feel softer and the dynamic range more expressive. Could expose a `velocity_curve` parameter (0=linear, 1=quadratic, or a continuous exponent knob).

## UI

- [ ] **Group controls visually** — the current flat layout of knobs needs grouping by section (Oscillator, Filter, Envelope, LFO, Master) with visual separators or panels. Needs design thinking before implementing.
