#include "PluginEditor.h"

// ── Constructor ───────────────────────────────────────────────────────────────

PM0AudioProcessorEditor::PM0AudioProcessorEditor (PM0AudioProcessor& p)
    : AudioProcessorEditor (&p), proc_ (p)
{
    setLookAndFeel (&laf_);
    setSize (1250, 760);

    // ── Preset bar ────────────────────────────────────────────────────────────
    presetLabel_.setJustificationType (juce::Justification::centredRight);
    presetLabel_.setFont (juce::Font (juce::FontOptions{}.withHeight (10.f).withStyle ("Bold")));
    addAndMakeVisible (presetLabel_);
    addAndMakeVisible (presetSelector_);
    addAndMakeVisible (saveAsButton_);
    addAndMakeVisible (deleteButton_);
    saveAsButton_.addListener (this);
    deleteButton_.addListener (this);
    updatePresetList();
    presetSelector_.onChange = [this] { onPresetSelected(); };

    // ── ROW 1 — Osc 1 chain ───────────────────────────────────────────────────

    // Oscillator 1
    for (auto& btn : oscWaveformBtns_) { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    setupKnob (oscTuneKnob_,   oscTuneLbl_,   "TUNE");
    setupKnob (oscDetuneKnob_, oscDetuneLbl_, "DETUNE");
    oscTuneAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "osc_tune",   oscTuneKnob_);
    oscDetuneAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc_detune", oscDetuneKnob_);

    // Filter 1
    for (auto& btn : filterModeBtns_) { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    setupKnob (filterCutoffKnob_,    filterCutoffLbl_,    "CUTOFF");
    setupKnob (filterResonanceKnob_, filterResonanceLbl_, "RESO");
    filterCutoffAtt_    = std::make_unique<SliderAttachment> (proc_.apvts, "filter_cutoff",    filterCutoffKnob_);
    filterResonanceAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "filter_resonance", filterResonanceKnob_);

    // Volume envelope 1
    setupKnob (envAttackKnob_,  envAttackLbl_,  "ATTACK");
    setupKnob (envDecayKnob_,   envDecayLbl_,   "DECAY");
    setupKnob (envSustainKnob_, envSustainLbl_, "SUSTAIN");
    setupKnob (envReleaseKnob_, envReleaseLbl_, "RELEASE");
    envAttackAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "env_attack",   envAttackKnob_);
    envDecayAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "env_decay",    envDecayKnob_);
    envSustainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_sustain",  envSustainKnob_);
    envReleaseAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_release",  envReleaseKnob_);
    envSustainBtn_.setClickingTogglesState (true);
    addAndMakeVisible (envSustainBtn_);
    envSustainBtnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "env_sustain_on", envSustainBtn_);

    // Filter envelope 1
    setupKnob (fenvAttackKnob_,  fenvAttackLbl_,  "ATTACK");
    setupKnob (fenvDecayKnob_,   fenvDecayLbl_,   "DECAY");
    setupKnob (fenvSustainKnob_, fenvSustainLbl_, "SUSTAIN");
    setupKnob (fenvReleaseKnob_, fenvReleaseLbl_, "RELEASE");
    setupKnob (envFilterModKnob_, envFilterModLbl_, "DEPTH");
    fenvAttackAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_attack",     fenvAttackKnob_);
    fenvDecayAtt_    = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_decay",      fenvDecayKnob_);
    fenvSustainAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_sustain",    fenvSustainKnob_);
    fenvReleaseAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_release",    fenvReleaseKnob_);
    envFilterModAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_filter_mod",  envFilterModKnob_);
    fenvSustainBtn_.setClickingTogglesState (true);
    addAndMakeVisible (fenvSustainBtn_);
    fenvSustainBtnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "fenv_sustain_on", fenvSustainBtn_);

    // LFO
    for (auto& btn : lfoTargetBtns_) { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    setupKnob (lfoSpeedKnob_, lfoSpeedLbl_, "SPEED");
    setupKnob (lfoDepthKnob_, lfoDepthLbl_, "DEPTH");
    lfoSpeedAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_speed", lfoSpeedKnob_);
    lfoDepthAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_depth", lfoDepthKnob_);

    // Master
    setupKnob (outputGainKnob_, outputGainLbl_, "GAIN");
    outputGainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "output_gain", outputGainKnob_);

    // ── ROW 2 — Osc 2 chain ───────────────────────────────────────────────────

    // Osc 2 oscillator
    osc2OnBtn_.setClickingTogglesState (true);
    addAndMakeVisible (osc2OnBtn_);
    osc2OnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "osc2_on", osc2OnBtn_);

    for (auto& btn : osc2WaveformBtns_) { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    for (auto& btn : osc2MultBtns_)     { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    setupKnob (osc2PhaseKnob_, osc2PhaseLbl_, "PHASE");
    osc2PhaseAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_phase", osc2PhaseKnob_);

    // Mix mode
    for (auto& btn : osc2MixModeBtns_) { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    setupKnob (osc2MixDepthKnob_, osc2MixDepthLbl_, "DEPTH");
    osc2MixDepthAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_mix_depth", osc2MixDepthKnob_);

    // Vol envelope 2
    osc2EnvOnBtn_.setClickingTogglesState (true);
    addAndMakeVisible (osc2EnvOnBtn_);
    osc2EnvOnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "osc2_env_on", osc2EnvOnBtn_);
    setupKnob (osc2EnvAtkKnob_, osc2EnvAtkLbl_, "ATTACK");
    setupKnob (osc2EnvDecKnob_, osc2EnvDecLbl_, "DECAY");
    setupKnob (osc2EnvSusKnob_, osc2EnvSusLbl_, "SUSTAIN");
    setupKnob (osc2EnvRelKnob_, osc2EnvRelLbl_, "RELEASE");
    osc2EnvAtkAtt_     = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_env_attack",  osc2EnvAtkKnob_);
    osc2EnvDecAtt_     = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_env_decay",   osc2EnvDecKnob_);
    osc2EnvSusKnobAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_env_sustain", osc2EnvSusKnob_);
    osc2EnvRelAtt_     = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_env_release", osc2EnvRelKnob_);
    osc2EnvSusBtn_.setClickingTogglesState (true);
    addAndMakeVisible (osc2EnvSusBtn_);
    osc2EnvSusAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "osc2_env_sus_on", osc2EnvSusBtn_);

    // Filter 2
    osc2FltOnBtn_.setClickingTogglesState (true);
    addAndMakeVisible (osc2FltOnBtn_);
    osc2FltOnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "osc2_flt_on", osc2FltOnBtn_);
    for (auto& btn : osc2FltModeBtns_) { btn.setClickingTogglesState(false); addAndMakeVisible(btn); btn.addListener(this); }
    setupKnob (osc2FltCutKnob_,  osc2FltCutLbl_,  "CUTOFF");
    setupKnob (osc2FltResoKnob_, osc2FltResoLbl_, "RESO");
    osc2FltCutAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_flt_cutoff", osc2FltCutKnob_);
    osc2FltResoAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_flt_reso",   osc2FltResoKnob_);

    // Filter envelope 2
    setupKnob (osc2FenvAtkKnob_,   osc2FenvAtkLbl_,   "ATTACK");
    setupKnob (osc2FenvDecKnob_,   osc2FenvDecLbl_,   "DECAY");
    setupKnob (osc2FenvSusKnob_,   osc2FenvSusLbl_,   "SUSTAIN");
    setupKnob (osc2FenvRelKnob_,   osc2FenvRelLbl_,   "RELEASE");
    setupKnob (osc2FenvDepthKnob_, osc2FenvDepthLbl_, "DEPTH");
    osc2FenvAtkAtt_     = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_fenv_atk",   osc2FenvAtkKnob_);
    osc2FenvDecAtt_     = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_fenv_dec",   osc2FenvDecKnob_);
    osc2FenvSusKnobAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_fenv_sus",   osc2FenvSusKnob_);
    osc2FenvRelAtt_     = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_fenv_rel",   osc2FenvRelKnob_);
    osc2FenvDepthAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "osc2_fenv_depth", osc2FenvDepthKnob_);
    osc2FenvSusBtn_.setClickingTogglesState (true);
    addAndMakeVisible (osc2FenvSusBtn_);
    osc2FenvSusAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "osc2_fenv_sus_on", osc2FenvSusBtn_);

    startTimer (30);
}

PM0AudioProcessorEditor::~PM0AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::setupKnob (juce::Slider& knob, juce::Label& lbl,
                                          const juce::String& labelText)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 16);
    addAndMakeVisible (knob);

    lbl.setText (labelText, juce::dontSendNotification);
    lbl.setJustificationType (juce::Justification::centred);
    lbl.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f).withStyle ("Bold")));
    addAndMakeVisible (lbl);
}

// ── paintSection() ────────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::paintSection (juce::Graphics& g,
                                              juce::Rectangle<int> rect,
                                              const juce::String& title,
                                              juce::Colour accent,
                                              bool dimmed) const
{
    const float alpha = dimmed ? 0.35f : 1.f;
    auto sb = rect.toFloat().reduced (2.f);

    g.setColour (PM0Col::panel().withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (sb, 4.f);

    auto strip = sb.withHeight (20.f);
    g.setColour (accent.withAlpha (0.12f * alpha));
    g.fillRoundedRectangle (strip, 4.f);
    g.fillRect (strip.withTrimmedTop (4.f));

    g.setColour (PM0Col::panelBorder().withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (sb.reduced (0.5f), 4.f, 0.8f);

    g.setColour (juce::Colour (0xFF505858).withMultipliedAlpha (alpha));
    g.drawLine (sb.getX() + 6.f, sb.getY() + 0.5f,
                sb.getRight() - 6.f, sb.getY() + 0.5f, 0.7f);
    g.setColour (juce::Colours::black.withAlpha (0.25f * alpha));
    g.drawLine (sb.getX() + 6.f, sb.getBottom() - 0.5f,
                sb.getRight() - 6.f, sb.getBottom() - 0.5f, 0.7f);

    g.setColour (accent.withMultipliedAlpha (alpha));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.f).withStyle ("Bold")));
    g.drawText (title, rect.withHeight (20).reduced (8, 0),
                juce::Justification::centredLeft, false);
}

// ── paint() ───────────────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (PM0Col::bg());

    g.setColour (juce::Colour (0xFF141614));
    g.fillRect (0, 0, getWidth(), 50);
    g.setColour (PM0Col::panelBorder());
    g.drawLine (0.f, 49.5f, (float)getWidth(), 49.5f, 0.8f);

    // Row 1 sections
    paintSection (g, oscSect_,    "OSCILLATOR",  PM0Col::osc());
    paintSection (g, filterSect_, "FILTER",       PM0Col::filter());
    paintSection (g, volEnvSect_, "VOLUME ENV",   PM0Col::volEnv());
    paintSection (g, fEnvSect_,   "FILTER ENV",   PM0Col::fEnv());
    paintSection (g, lfoSect_,    "LFO",          PM0Col::lfo());
    paintSection (g, masterSect_, "MASTER",        PM0Col::master());

    // Output meter (within masterSect_)
    {
        auto meterArea = masterSect_.reduced (8, 4).withTrimmedTop (masterSect_.getHeight() / 2);
        g.setColour (PM0Col::trackArc());
        g.fillRect (meterArea);
        g.setColour (PM0Col::panelBorder());
        g.drawRect (meterArea);
        float fill = juce::jmap (displayOutputPeak_, -60.f, 0.f, 0.f, (float)meterArea.getHeight());
        int fillH  = juce::jlimit (0, meterArea.getHeight(), (int)fill);
        if (fillH > 0) {
            auto fillRect  = meterArea.withTop (meterArea.getBottom() - fillH);
            auto meterCol  = displayOutputPeak_ > -6.f  ? juce::Colour (0xFFE84020)
                           : displayOutputPeak_ > -18.f ? PM0Col::valueArc()
                                                         : PM0Col::ledOn();
            g.setColour (meterCol.withAlpha (0.8f));
            g.fillRect (fillRect);
        }
    }

    // Row 2 sections (dim when Osc 2 is off)
    const bool osc2On = *proc_.apvts.getRawParameterValue ("osc2_on") > 0.5f;
    paintSection (g, osc2Sect_,  "OSC 2",       PM0Col::osc());
    paintSection (g, mix2Sect_,  "MIX MODE",     PM0Col::master(),  !osc2On);
    paintSection (g, env2Sect_,  "VOL ENV 2",    PM0Col::volEnv(),  !osc2On);
    paintSection (g, flt2Sect_,  "FILTER 2",     PM0Col::filter(),  !osc2On);
    paintSection (g, fenv2Sect_, "FILT ENV 2",   PM0Col::fEnv(),    !osc2On);
}

// ── resized() ─────────────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int kHeaderH = 22;
    const int kPad     = 6;
    const int kBtnH    = 22;
    const int kGap     = 3;
    const int kKnobH   = 58;
    const int kTbH     = 16;
    const int kLblH    = 12;

    // ── Preset bar ────────────────────────────────────────────────────────────
    {
        auto bar = bounds.removeFromTop (50).reduced (8, 8);
        presetLabel_.setBounds  (bar.removeFromLeft (60));
        bar.removeFromLeft (4);
        presetSelector_.setBounds (bar.removeFromLeft (300));
        bar.removeFromLeft (8);
        saveAsButton_.setBounds (bar.removeFromLeft (90));
        bar.removeFromLeft (4);
        deleteButton_.setBounds (bar.removeFromLeft (80));
    }

    const int halfH  = bounds.getHeight() / 2;
    auto row1 = bounds.removeFromTop (halfH);
    auto row2 = bounds;

    auto content1 = row1.reduced (kPad);
    auto content2 = row2.reduced (kPad);

    // ── ROW 1 section widths ─────────────────────────────────────────────────
    oscSect_    = content1.removeFromLeft (180);
    filterSect_ = content1.removeFromLeft (180);
    volEnvSect_ = content1.removeFromLeft (268);
    fEnvSect_   = content1.removeFromLeft (268);
    lfoSect_    = content1.removeFromLeft (175);
    masterSect_ = content1;

    // ── ROW 2 section widths ─────────────────────────────────────────────────
    osc2Sect_  = content2.removeFromLeft (255);
    mix2Sect_  = content2.removeFromLeft (160);
    env2Sect_  = content2.removeFromLeft (265);
    flt2Sect_  = content2.removeFromLeft (175);
    fenv2Sect_ = content2;

    // Helper: content rect below section header
    auto contentOf = [&] (juce::Rectangle<int> sect) {
        return sect.reduced (kPad).withTrimmedTop (kHeaderH + kGap);
    };

    // Helper: place knob + label from the top of `col`
    auto placeKnob = [&] (juce::Rectangle<int>& col, juce::Slider& k, juce::Label& l) {
        auto slot = col.removeFromTop (kKnobH + kTbH);
        k.setBounds (slot.reduced (4, 0));
        l.setBounds (col.removeFromTop (kLblH));
        col.removeFromTop (kGap);
    };

    // ── OSCILLATOR 1 ─────────────────────────────────────────────────────────
    {
        auto area = contentOf (oscSect_);
        for (auto& btn : oscWaveformBtns_)
        { btn.setBounds (area.removeFromTop (kBtnH)); area.removeFromTop (kGap); }
        area.removeFromTop (kGap);
        int half = area.getWidth() / 2;
        auto c1 = area.removeFromLeft (half);
        placeKnob (c1,   oscTuneKnob_,   oscTuneLbl_);
        placeKnob (area, oscDetuneKnob_, oscDetuneLbl_);
    }

    // ── FILTER 1 ─────────────────────────────────────────────────────────────
    {
        auto area = contentOf (filterSect_);
        for (auto& btn : filterModeBtns_)
        { btn.setBounds (area.removeFromTop (kBtnH)); area.removeFromTop (kGap); }
        area.removeFromTop (kGap);
        int half = area.getWidth() / 2;
        auto c1 = area.removeFromLeft (half);
        placeKnob (c1,   filterCutoffKnob_,    filterCutoffLbl_);
        placeKnob (area, filterResonanceKnob_, filterResonanceLbl_);
    }

    // ── VOLUME ENV 1 ─────────────────────────────────────────────────────────
    {
        auto area = contentOf (volEnvSect_);
        int half = area.getWidth() / 2;
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1, envAttackKnob_, envAttackLbl_);
          placeKnob (row, envDecayKnob_, envDecayLbl_); }
        area.removeFromTop (kGap);
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  envSustainKnob_, envSustainLbl_);
          placeKnob (row, envReleaseKnob_, envReleaseLbl_); }
        area.removeFromTop (kGap);
        envSustainBtn_.setBounds (area.removeFromTop (kBtnH).withSizeKeepingCentre (120, kBtnH));
    }

    // ── FILTER ENV 1 ─────────────────────────────────────────────────────────
    {
        auto area = contentOf (fEnvSect_);
        int half = area.getWidth() / 2;
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  fenvAttackKnob_, fenvAttackLbl_);
          placeKnob (row, fenvDecayKnob_,  fenvDecayLbl_); }
        area.removeFromTop (kGap);
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  fenvSustainKnob_, fenvSustainLbl_);
          placeKnob (row, fenvReleaseKnob_, fenvReleaseLbl_); }
        area.removeFromTop (kGap);
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1, envFilterModKnob_, envFilterModLbl_);
          fenvSustainBtn_.setBounds (row.withSizeKeepingCentre (110, kBtnH)); }
    }

    // ── LFO ──────────────────────────────────────────────────────────────────
    {
        auto area = contentOf (lfoSect_);
        for (auto& btn : lfoTargetBtns_)
        { btn.setBounds (area.removeFromTop (kBtnH)); area.removeFromTop (kGap); }
        area.removeFromTop (kGap);
        int half = area.getWidth() / 2;
        auto c1 = area.removeFromLeft (half);
        placeKnob (c1,   lfoSpeedKnob_, lfoSpeedLbl_);
        placeKnob (area, lfoDepthKnob_, lfoDepthLbl_);
    }

    // ── MASTER ────────────────────────────────────────────────────────────────
    {
        auto area = contentOf (masterSect_);
        int kw = juce::jmin (area.getWidth() - 8, 70);
        outputGainKnob_.setBounds (area.removeFromTop (kKnobH + kTbH).withSizeKeepingCentre (kw, kKnobH + kTbH));
        outputGainLbl_.setBounds  (area.removeFromTop (kLblH));
    }

    // ══════════════════════════════════════════════════════════════════════════
    //  ROW 2
    // ══════════════════════════════════════════════════════════════════════════

    // ── OSC 2 ─────────────────────────────────────────────────────────────────
    {
        auto area = contentOf (osc2Sect_);

        osc2OnBtn_.setBounds (area.removeFromTop (kBtnH));
        area.removeFromTop (kGap + 2);

        // Waveforms (left half) and multipliers (right half) side-by-side
        int half = area.getWidth() / 2;
        auto leftCol  = area.removeFromLeft (half);
        auto rightCol = area;

        for (auto& btn : osc2WaveformBtns_)
        { btn.setBounds (leftCol.removeFromTop (kBtnH)); leftCol.removeFromTop (kGap); }

        for (auto& btn : osc2MultBtns_)
        { btn.setBounds (rightCol.removeFromTop (kBtnH)); rightCol.removeFromTop (kGap); }

        // Phase knob: vertically aligned with the space remaining in the right column
        // (after 4 mult buttons; waveform col has 5, so 1 btn row taller)
        int kw = juce::jmin (half - 8, 60);
        // leftCol is now positioned after 5 waveform buttons
        // rightCol is positioned after 4 mult buttons
        // PHASE goes in the right column remainder, slightly below
        rightCol.removeFromTop (kGap);
        osc2PhaseKnob_.setBounds (rightCol.removeFromTop (kKnobH + kTbH).withSizeKeepingCentre (kw, kKnobH + kTbH));
        osc2PhaseLbl_.setBounds  (rightCol.removeFromTop (kLblH));
    }

    // ── MIX MODE ─────────────────────────────────────────────────────────────
    {
        auto area = contentOf (mix2Sect_);
        for (auto& btn : osc2MixModeBtns_)
        { btn.setBounds (area.removeFromTop (kBtnH)); area.removeFromTop (kGap); }
        area.removeFromTop (kGap);
        placeKnob (area, osc2MixDepthKnob_, osc2MixDepthLbl_);
    }

    // ── VOL ENV 2 ─────────────────────────────────────────────────────────────
    {
        auto area = contentOf (env2Sect_);
        osc2EnvOnBtn_.setBounds (area.removeFromTop (kBtnH));
        area.removeFromTop (kGap + 2);
        int half = area.getWidth() / 2;
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  osc2EnvAtkKnob_, osc2EnvAtkLbl_);
          placeKnob (row, osc2EnvDecKnob_, osc2EnvDecLbl_); }
        area.removeFromTop (kGap);
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  osc2EnvSusKnob_, osc2EnvSusLbl_);
          placeKnob (row, osc2EnvRelKnob_, osc2EnvRelLbl_); }
        area.removeFromTop (kGap);
        osc2EnvSusBtn_.setBounds (area.removeFromTop (kBtnH).withSizeKeepingCentre (110, kBtnH));
    }

    // ── FILTER 2 ─────────────────────────────────────────────────────────────
    {
        auto area = contentOf (flt2Sect_);
        osc2FltOnBtn_.setBounds (area.removeFromTop (kBtnH));
        area.removeFromTop (kGap + 2);
        for (auto& btn : osc2FltModeBtns_)
        { btn.setBounds (area.removeFromTop (kBtnH)); area.removeFromTop (kGap); }
        area.removeFromTop (kGap);
        int half = area.getWidth() / 2;
        auto c1 = area.removeFromLeft (half);
        placeKnob (c1,   osc2FltCutKnob_,  osc2FltCutLbl_);
        placeKnob (area, osc2FltResoKnob_, osc2FltResoLbl_);
    }

    // ── FILT ENV 2 ────────────────────────────────────────────────────────────
    {
        auto area = contentOf (fenv2Sect_);
        if (area.getWidth() > 340) area = area.withWidth (340);

        int half = area.getWidth() / 2;
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  osc2FenvAtkKnob_, osc2FenvAtkLbl_);
          placeKnob (row, osc2FenvDecKnob_, osc2FenvDecLbl_); }
        area.removeFromTop (kGap);
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1,  osc2FenvSusKnob_, osc2FenvSusLbl_);
          placeKnob (row, osc2FenvRelKnob_, osc2FenvRelLbl_); }
        area.removeFromTop (kGap);
        { auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
          auto c1 = row.removeFromLeft (half);
          placeKnob (c1, osc2FenvDepthKnob_, osc2FenvDepthLbl_);
          osc2FenvSusBtn_.setBounds (row.withSizeKeepingCentre (110, kBtnH)); }
    }
}

// ── timerCallback() ───────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::timerCallback()
{
    float newPeak = proc_.outputPeakDb.load (std::memory_order_relaxed);
    if (std::abs (newPeak - displayOutputPeak_) > 0.5f)
    { displayOutputPeak_ = newPeak; repaint (masterSect_); }

    auto readInt = [&] (const char* id) {
        return static_cast<int> (*proc_.apvts.getRawParameterValue (id));
    };

    // Row 1 radio groups
    int wf = readInt ("osc_waveform");
    for (int i = 0; i < 4; ++i) oscWaveformBtns_[i].setToggleState (i == wf, juce::dontSendNotification);
    int fm = readInt ("filter_mode");
    for (int i = 0; i < 3; ++i) filterModeBtns_[i].setToggleState (i == fm, juce::dontSendNotification);
    int lt = readInt ("lfo_target");
    for (int i = 0; i < 3; ++i) lfoTargetBtns_[i].setToggleState (i == lt, juce::dontSendNotification);

    // Row 2 radio groups
    int wf2 = readInt ("osc2_waveform");
    for (int i = 0; i < 5; ++i) osc2WaveformBtns_[i].setToggleState (i == wf2, juce::dontSendNotification);
    int mt2 = readInt ("osc2_mult");
    for (int i = 0; i < 4; ++i) osc2MultBtns_[i].setToggleState (i == mt2, juce::dontSendNotification);
    int mm2 = readInt ("osc2_mix_mode");
    for (int i = 0; i < 4; ++i) osc2MixModeBtns_[i].setToggleState (i == mm2, juce::dontSendNotification);
    int fm2 = readInt ("osc2_flt_mode");
    for (int i = 0; i < 3; ++i) osc2FltModeBtns_[i].setToggleState (i == fm2, juce::dontSendNotification);

    // Repaint Row 2 sections when osc2_on changes (dim effect)
    const bool osc2On = *proc_.apvts.getRawParameterValue ("osc2_on") > 0.5f;
    static bool lastOsc2On = !osc2On;
    if (osc2On != lastOsc2On)
    {
        lastOsc2On = osc2On;
        repaint (mix2Sect_);
        repaint (env2Sect_);
        repaint (flt2Sect_);
        repaint (fenv2Sect_);
    }
}

// ── buttonClicked() ───────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::buttonClicked (juce::Button* btn)
{
    if (btn == &saveAsButton_) { onSaveAsPressed(); return; }
    if (btn == &deleteButton_) { onDeletePressed();  return; }

    auto setChoice = [&] (const char* id, int idx, int total) {
        if (auto* p = proc_.apvts.getParameter (id))
            p->setValueNotifyingHost (static_cast<float>(idx) / static_cast<float>(juce::jmax (1, total - 1)));
    };

    for (int i = 0; i < 4; ++i) if (btn == &oscWaveformBtns_[i]) { setChoice("osc_waveform", i, 4); return; }
    for (int i = 0; i < 3; ++i) if (btn == &filterModeBtns_[i])  { setChoice("filter_mode",  i, 3); return; }
    for (int i = 0; i < 3; ++i) if (btn == &lfoTargetBtns_[i])   { setChoice("lfo_target",   i, 3); return; }

    for (int i = 0; i < 5; ++i) if (btn == &osc2WaveformBtns_[i]) { setChoice("osc2_waveform", i, 5); return; }
    for (int i = 0; i < 4; ++i) if (btn == &osc2MultBtns_[i])     { setChoice("osc2_mult",     i, 4); return; }
    for (int i = 0; i < 4; ++i) if (btn == &osc2MixModeBtns_[i])  { setChoice("osc2_mix_mode", i, 4); return; }
    for (int i = 0; i < 3; ++i) if (btn == &osc2FltModeBtns_[i])  { setChoice("osc2_flt_mode", i, 3); return; }
}

// ── Preset management ─────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::updatePresetList()
{
    presetSelector_.clear (juce::dontSendNotification);
    auto* pm = proc_.getPresetManager();
    if (!pm) return;

    auto presets = pm->getPresetList();
    for (size_t i = 0; i < presets.size(); ++i)
        presetSelector_.addItem (presets[i].name, static_cast<int>(i + 1));

    int idx = pm->getCurrentPresetIndex();
    presetSelector_.setSelectedItemIndex (idx, juce::dontSendNotification);
    bool isFactory = (idx >= 0 && idx < (int)presets.size()) ? presets[(size_t)idx].isFactory : true;
    deleteButton_.setEnabled (!isFactory);
}

void PM0AudioProcessorEditor::onPresetSelected()
{
    int idx = presetSelector_.getSelectedItemIndex();
    if (idx >= 0) proc_.setCurrentProgram (idx);
    auto* pm = proc_.getPresetManager();
    if (!pm) return;
    auto presets = pm->getPresetList();
    bool isFactory = (idx >= 0 && idx < (int)presets.size()) ? presets[(size_t)idx].isFactory : true;
    deleteButton_.setEnabled (!isFactory);
}

void PM0AudioProcessorEditor::onSaveAsPressed()
{
    auto* w = new juce::AlertWindow ("Save Preset As", "Enter preset name:", juce::AlertWindow::NoIcon);
    w->addTextEditor ("name", "", "Preset name:");
    w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PM0AudioProcessorEditor> safe (this);
    w->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe, w] (int result) {
            if (result == 1 && safe != nullptr) {
                auto name = w->getTextEditorContents ("name").trim();
                if (name.isNotEmpty()) {
                    auto* pm = safe->proc_.getPresetManager();
                    if (pm && pm->savePreset (name)) safe->updatePresetList();
                }
            }
        }), true);
}

void PM0AudioProcessorEditor::onDeletePressed()
{
    auto* pm = proc_.getPresetManager();
    if (!pm) return;
    int idx = pm->getCurrentPresetIndex();
    auto presets = pm->getPresetList();
    if (idx < 0 || idx >= (int)presets.size() || presets[(size_t)idx].isFactory) return;

    juce::Component::SafePointer<PM0AudioProcessorEditor> safe (this);
    juce::AlertWindow::showOkCancelBox (
        juce::AlertWindow::WarningIcon, "Delete Preset",
        "Delete preset '" + presets[(size_t)idx].name + "'?", "Delete", "Cancel", nullptr,
        juce::ModalCallbackFunction::create ([safe, idx] (int result) {
            if (result == 1 && safe != nullptr) {
                auto* pm2 = safe->proc_.getPresetManager();
                if (pm2 && pm2->deletePreset (idx)) safe->updatePresetList();
            }
        }));
}
