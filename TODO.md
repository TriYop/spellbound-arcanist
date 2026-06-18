# PM0 TODO

## Synthesis

- [ ] **Unison / second oscillator per voice** — add a second `Oscillator` per `Voice`, tuned at `+detune` / `-detune` cents relative to the first, panned L/R. This is what produces the classic thick pad sound through beating between the two oscillators.

- [ ] **LFO target selection** — add an `lfo_target` parameter (filter cutoff / amplitude / pitch). Currently the LFO only modulates filter cutoff. Tremolo (amplitude LFO) and vibrato (pitch LFO) are the two other essential pad motion types.

- [ ] **Filter mode** — add a `filter_mode` parameter (LP / BP / HP) and implement the corresponding biquad coefficient sets in `Filter`. Currently always low-pass.

## UI

- [ ] **Group controls visually** — the current flat layout of knobs needs grouping by section (Oscillator, Filter, Envelope, LFO, Master) with visual separators or panels. Needs design thinking before implementing: how to handle the added waveform ComboBox, future filter mode selector, and LFO target selector in the same space.
