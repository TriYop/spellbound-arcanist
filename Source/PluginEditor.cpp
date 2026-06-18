#include "PluginEditor.h"

// ── Constructor ───────────────────────────────────────────────────────────────

PM0AudioProcessorEditor::PM0AudioProcessorEditor (PM0AudioProcessor& p)
    : AudioProcessorEditor (&p), proc_ (p)
{
    setLookAndFeel (&laf_);
    setSize (1150, 560);

    // ── Preset bar ────────────────────────────────────────────────────────────
    presetLabel_.setJustificationType (juce::Justification::centredRight);
    presetLabel_.setFont (juce::Font (juce::FontOptions{}.withHeight (10.f).withStyle ("Bold")));
    addAndMakeVisible (presetLabel_);

    addAndMakeVisible (presetSelector_);
    // onChange lambda wired at the bottom of the constructor

    addAndMakeVisible (saveAsButton_);
    addAndMakeVisible (deleteButton_);
    saveAsButton_.addListener (this);
    deleteButton_.addListener (this);

    updatePresetList();

    // ── Oscillator ────────────────────────────────────────────────────────────
    for (auto& btn : oscWaveformBtns_) { btn.setClickingTogglesState (false); addAndMakeVisible (btn); btn.addListener (this); }

    setupKnob (oscTuneKnob_,   oscTuneLbl_,   "TUNE");
    setupKnob (oscDetuneKnob_, oscDetuneLbl_, "DETUNE");
    oscTuneAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "osc_tune",    oscTuneKnob_);
    oscDetuneAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc_detune",  oscDetuneKnob_);

    // ── Filter ────────────────────────────────────────────────────────────────
    for (auto& btn : filterModeBtns_) { btn.setClickingTogglesState (false); addAndMakeVisible (btn); btn.addListener (this); }

    setupKnob (filterCutoffKnob_,    filterCutoffLbl_,    "CUTOFF");
    setupKnob (filterResonanceKnob_, filterResonanceLbl_, "RESO");
    filterCutoffAtt_    = std::make_unique<SliderAttachment> (proc_.apvts, "filter_cutoff",    filterCutoffKnob_);
    filterResonanceAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "filter_resonance", filterResonanceKnob_);

    // ── Volume envelope ───────────────────────────────────────────────────────
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

    // ── Filter envelope ───────────────────────────────────────────────────────
    setupKnob (fenvAttackKnob_,  fenvAttackLbl_,  "ATTACK");
    setupKnob (fenvDecayKnob_,   fenvDecayLbl_,   "DECAY");
    setupKnob (fenvSustainKnob_, fenvSustainLbl_, "SUSTAIN");
    setupKnob (fenvReleaseKnob_, fenvReleaseLbl_, "RELEASE");
    setupKnob (envFilterModKnob_, envFilterModLbl_, "DEPTH");
    fenvAttackAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_attack",     fenvAttackKnob_);
    fenvDecayAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_decay",      fenvDecayKnob_);
    fenvSustainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_sustain",    fenvSustainKnob_);
    fenvReleaseAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_release",    fenvReleaseKnob_);
    envFilterModAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_filter_mod", envFilterModKnob_);

    fenvSustainBtn_.setClickingTogglesState (true);
    addAndMakeVisible (fenvSustainBtn_);
    fenvSustainBtnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "fenv_sustain_on", fenvSustainBtn_);

    // ── LFO ───────────────────────────────────────────────────────────────────
    for (auto& btn : lfoTargetBtns_) { btn.setClickingTogglesState (false); addAndMakeVisible (btn); btn.addListener (this); }

    setupKnob (lfoSpeedKnob_, lfoSpeedLbl_, "SPEED");
    setupKnob (lfoDepthKnob_, lfoDepthLbl_, "DEPTH");
    lfoSpeedAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_speed", lfoSpeedKnob_);
    lfoDepthAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_depth", lfoDepthKnob_);

    // ── Master ────────────────────────────────────────────────────────────────
    setupKnob (outputGainKnob_, outputGainLbl_, "GAIN");
    outputGainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "output_gain", outputGainKnob_);

    // Wire up preset selector changes without inheriting ComboBox::Listener
    presetSelector_.onChange = [this] { onPresetSelected(); };

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

// ── paint() ───────────────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::paintSection (juce::Graphics& g,
                                              juce::Rectangle<int> rect,
                                              const juce::String& title,
                                              juce::Colour accent) const
{
    auto sb = rect.toFloat().reduced (2.f);

    // Panel body
    g.setColour (PM0Col::panel());
    g.fillRoundedRectangle (sb, 4.f);

    // Top accent strip
    auto strip = sb.withHeight (20.f);
    g.setColour (accent.withAlpha (0.12f));
    g.fillRoundedRectangle (strip, 4.f);
    // Fill the bottom corners of the strip so it blends flat into the panel
    g.fillRect (strip.withTrimmedTop (4.f));

    // Border (bottom + sides brighter for bevel effect)
    g.setColour (PM0Col::panelBorder());
    g.drawRoundedRectangle (sb.reduced (0.5f), 4.f, 0.8f);

    // Top edge highlight
    g.setColour (juce::Colour (0xFF505858));
    g.drawLine (sb.getX() + 6.f, sb.getY() + 0.5f,
                sb.getRight() - 6.f, sb.getY() + 0.5f, 0.7f);

    // Bottom shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (sb.getX() + 6.f, sb.getBottom() - 0.5f,
                sb.getRight() - 6.f, sb.getBottom() - 0.5f, 0.7f);

    // Section title
    g.setColour (accent);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.f).withStyle ("Bold")));
    g.drawText (title, rect.withHeight (20).reduced (8, 0),
                juce::Justification::centredLeft, false);
}

void PM0AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Editor background
    g.fillAll (PM0Col::bg());

    // Preset bar background
    g.setColour (juce::Colour (0xFF141614));
    g.fillRect (0, 0, getWidth(), 50);
    g.setColour (PM0Col::panelBorder());
    g.drawLine (0.f, 49.5f, (float)getWidth(), 49.5f, 0.8f);

    // Section panels
    paintSection (g, oscSect_,    "OSCILLATOR",   PM0Col::osc());
    paintSection (g, filterSect_, "FILTER",        PM0Col::filter());
    paintSection (g, volEnvSect_, "VOLUME ENV",    PM0Col::volEnv());
    paintSection (g, fEnvSect_,   "FILTER ENV",    PM0Col::fEnv());
    paintSection (g, lfoSect_,    "LFO",           PM0Col::lfo());
    paintSection (g, masterSect_, "MASTER",        PM0Col::master());

    // Output meter (right of master section)
    auto meterArea = masterSect_.withTrimmedBottom (masterSect_.getHeight() / 2).reduced (6, 4);
    g.setColour (PM0Col::trackArc());
    g.fillRect (meterArea);

    g.setColour (PM0Col::panelBorder());
    g.drawRect (meterArea);

    float fill = juce::jmap (displayOutputPeak_, -60.f, 0.f, 0.f, (float) meterArea.getHeight());
    int   fillH = juce::jlimit (0, meterArea.getHeight(), (int) fill);
    if (fillH > 0)
    {
        auto fillRect = meterArea.withTop (meterArea.getBottom() - fillH);
        // Colour: green → amber → red depending on level
        juce::Colour meterCol = displayOutputPeak_ > -6.f ? juce::Colour (0xFFE84020)
                              : displayOutputPeak_ > -18.f ? PM0Col::valueArc()
                                                            : PM0Col::ledOn();
        g.setColour (meterCol.withAlpha (0.8f));
        g.fillRect (fillRect);
    }
}

// ── resized() ─────────────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ── Preset bar ────────────────────────────────────────────────────────────
    {
        auto bar = bounds.removeFromTop (50).reduced (8, 8);
        presetLabel_.setBounds   (bar.removeFromLeft (60));
        bar.removeFromLeft (4);
        presetSelector_.setBounds (bar.removeFromLeft (300));
        bar.removeFromLeft (8);
        saveAsButton_.setBounds  (bar.removeFromLeft (90));
        bar.removeFromLeft (4);
        deleteButton_.setBounds  (bar.removeFromLeft (80));
    }

    // ── Section grid ──────────────────────────────────────────────────────────
    // Widths: Osc 165 | Filter 165 | VolEnv 265 | FiltEnv 265 | LFO 165 | Master rest
    auto content = bounds.reduced (6);

    oscSect_    = content.removeFromLeft (165);
    filterSect_ = content.removeFromLeft (165);
    volEnvSect_ = content.removeFromLeft (265);
    fEnvSect_   = content.removeFromLeft (265);
    lfoSect_    = content.removeFromLeft (165);
    masterSect_ = content; // remaining

    // ── Helper lambdas ────────────────────────────────────────────────────────
    const int kHeaderH = 22;  // section title strip height
    const int kPad     = 6;   // inner padding
    const int kBtnH    = 25;  // LED button height
    const int kGap     = 4;   // gap between items
    const int kKnobH   = 60;  // rotary knob height
    const int kTbH     = 16;  // textbox height under knob
    const int kLblH    = 13;  // parameter name label height
    [[maybe_unused]] const int kRowH = kKnobH + kTbH + kLblH + kGap;

    // Returns the content rect of a section (below header, inset by padding)
    auto contentOf = [&] (juce::Rectangle<int> sect) {
        return sect.reduced (kPad).withTrimmedTop (kHeaderH + kGap);
    };

    // Places a knob + label in a column (removes from top of `col`)
    auto placeKnob = [&] (juce::Rectangle<int>& col, juce::Slider& k, juce::Label& l) {
        auto slot = col.removeFromTop (kKnobH + kTbH);
        k.setBounds (slot.reduced (4, 0));
        l.setBounds (col.removeFromTop (kLblH));
        col.removeFromTop (kGap);
    };

    // ── OSCILLATOR ────────────────────────────────────────────────────────────
    {
        auto area = contentOf (oscSect_);

        // 4 waveform LED buttons stacked vertically
        for (auto& btn : oscWaveformBtns_)
        {
            btn.setBounds (area.removeFromTop (kBtnH));
            area.removeFromTop (kGap);
        }
        area.removeFromTop (kGap * 2);

        // 2 knobs side-by-side: Tune | Detune
        auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
        int halfW = row.getWidth() / 2;
        auto c1 = row.removeFromLeft (halfW);
        placeKnob (c1, oscTuneKnob_,   oscTuneLbl_);
        placeKnob (row, oscDetuneKnob_, oscDetuneLbl_);
    }

    // ── FILTER ────────────────────────────────────────────────────────────────
    {
        auto area = contentOf (filterSect_);

        // 3 mode LED buttons stacked
        for (auto& btn : filterModeBtns_)
        {
            btn.setBounds (area.removeFromTop (kBtnH));
            area.removeFromTop (kGap);
        }
        area.removeFromTop (kGap * 2);

        // 2 knobs side-by-side: Cutoff | Resonance
        auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
        int halfW = row.getWidth() / 2;
        auto c1 = row.removeFromLeft (halfW);
        placeKnob (c1, filterCutoffKnob_,    filterCutoffLbl_);
        placeKnob (row, filterResonanceKnob_, filterResonanceLbl_);
    }

    // ── VOLUME ENVELOPE ───────────────────────────────────────────────────────
    {
        auto area = contentOf (volEnvSect_);
        int halfW = area.getWidth() / 2;

        // Row 1: Attack | Decay
        {
            auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
            auto c1 = row.removeFromLeft (halfW);
            placeKnob (c1, envAttackKnob_, envAttackLbl_);
            placeKnob (row, envDecayKnob_,  envDecayLbl_);
        }
        area.removeFromTop (kGap);

        // Row 2: Sustain | Release
        {
            auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
            auto c1 = row.removeFromLeft (halfW);
            placeKnob (c1, envSustainKnob_, envSustainLbl_);
            placeKnob (row, envReleaseKnob_, envReleaseLbl_);
        }
        area.removeFromTop (kGap * 2);

        // SUSTAIN toggle button (centered)
        envSustainBtn_.setBounds (area.removeFromTop (kBtnH)
                                       .withSizeKeepingCentre (120, kBtnH));
    }

    // ── FILTER ENVELOPE ───────────────────────────────────────────────────────
    {
        auto area = contentOf (fEnvSect_);
        int halfW = area.getWidth() / 2;

        // Row 1: Attack | Decay
        {
            auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
            auto c1 = row.removeFromLeft (halfW);
            placeKnob (c1, fenvAttackKnob_, fenvAttackLbl_);
            placeKnob (row, fenvDecayKnob_,  fenvDecayLbl_);
        }
        area.removeFromTop (kGap);

        // Row 2: Sustain | Release
        {
            auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
            auto c1 = row.removeFromLeft (halfW);
            placeKnob (c1, fenvSustainKnob_, fenvSustainLbl_);
            placeKnob (row, fenvReleaseKnob_, fenvReleaseLbl_);
        }
        area.removeFromTop (kGap * 2);

        // Row 3: Depth knob | SUSTAIN toggle
        {
            auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
            auto c1 = row.removeFromLeft (halfW);
            placeKnob (c1, envFilterModKnob_, envFilterModLbl_);
            // Sustain button vertically centred in right half
            fenvSustainBtn_.setBounds (row.withSizeKeepingCentre (120, kBtnH));
        }
    }

    // ── LFO ───────────────────────────────────────────────────────────────────
    {
        auto area = contentOf (lfoSect_);

        // 3 target LED buttons stacked vertically
        for (auto& btn : lfoTargetBtns_)
        {
            btn.setBounds (area.removeFromTop (kBtnH));
            area.removeFromTop (kGap);
        }
        area.removeFromTop (kGap * 2);

        // 2 knobs side-by-side: Speed | Depth
        auto row = area.removeFromTop (kKnobH + kTbH + kLblH + kGap);
        int halfW = row.getWidth() / 2;
        auto c1 = row.removeFromLeft (halfW);
        placeKnob (c1, lfoSpeedKnob_, lfoSpeedLbl_);
        placeKnob (row, lfoDepthKnob_, lfoDepthLbl_);
    }

    // ── MASTER ────────────────────────────────────────────────────────────────
    {
        auto area = contentOf (masterSect_);
        // Gain knob centred
        int kw = juce::jmin (area.getWidth() - 8, 70);
        auto knobSlot = area.removeFromTop (kKnobH + kTbH);
        outputGainKnob_.setBounds (knobSlot.withSizeKeepingCentre (kw, kKnobH + kTbH));
        outputGainLbl_.setBounds  (area.removeFromTop (kLblH));
    }
}

// ── timerCallback() — sync radio groups + meter ───────────────────────────────

void PM0AudioProcessorEditor::timerCallback()
{
    // Output meter
    float newPeak = proc_.outputPeakDb.load (std::memory_order_relaxed);
    if (std::abs (newPeak - displayOutputPeak_) > 0.5f)
    {
        displayOutputPeak_ = newPeak;
        repaint (masterSect_);
    }

    // Waveform buttons
    int waveform = (int) *proc_.apvts.getRawParameterValue ("osc_waveform");
    for (int i = 0; i < 4; ++i)
        oscWaveformBtns_[i].setToggleState (i == waveform, juce::dontSendNotification);

    // Filter mode buttons
    int filterMode = (int) *proc_.apvts.getRawParameterValue ("filter_mode");
    for (int i = 0; i < 3; ++i)
        filterModeBtns_[i].setToggleState (i == filterMode, juce::dontSendNotification);

    // LFO target buttons
    int lfoTarget = (int) *proc_.apvts.getRawParameterValue ("lfo_target");
    for (int i = 0; i < 3; ++i)
        lfoTargetBtns_[i].setToggleState (i == lfoTarget, juce::dontSendNotification);
}

// ── buttonClicked() ───────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::buttonClicked (juce::Button* btn)
{
    // Preset actions
    if (btn == &saveAsButton_) { onSaveAsPressed(); return; }
    if (btn == &deleteButton_) { onDeletePressed();  return; }

    // Radio groups — set the APVTS choice parameter to the clicked button's index
    auto setChoice = [&] (const juce::String& paramId, int idx, int total)
    {
        auto* param = proc_.apvts.getParameter (paramId);
        if (param)
            param->setValueNotifyingHost (
                static_cast<float> (idx) / static_cast<float> (juce::jmax (1, total - 1)));
    };

    for (int i = 0; i < 4; ++i)
        if (btn == &oscWaveformBtns_[i]) { setChoice ("osc_waveform", i, 4); return; }

    for (int i = 0; i < 3; ++i)
        if (btn == &filterModeBtns_[i])  { setChoice ("filter_mode",  i, 3); return; }

    for (int i = 0; i < 3; ++i)
        if (btn == &lfoTargetBtns_[i])   { setChoice ("lfo_target",   i, 3); return; }

    // envSustainBtn_ and fenvSustainBtn_ are handled by ButtonAttachment — no action needed here.
}

// ── Preset management ─────────────────────────────────────────────────────────

void PM0AudioProcessorEditor::updatePresetList()
{
    presetSelector_.clear (juce::dontSendNotification);
    auto* pm = proc_.getPresetManager();
    if (!pm) return;

    auto presets = pm->getPresetList();
    for (size_t i = 0; i < presets.size(); ++i)
        presetSelector_.addItem (presets[i].name, static_cast<int> (i + 1));

    int idx = pm->getCurrentPresetIndex();
    presetSelector_.setSelectedItemIndex (idx, juce::dontSendNotification);

    bool isFactory = (idx >= 0 && idx < (int) presets.size())
                     ? presets[(size_t) idx].isFactory : true;
    deleteButton_.setEnabled (!isFactory);
}

void PM0AudioProcessorEditor::onPresetSelected()
{
    int idx = presetSelector_.getSelectedItemIndex();
    if (idx >= 0) proc_.setCurrentProgram (idx);

    auto* pm = proc_.getPresetManager();
    if (!pm) return;
    auto presets = pm->getPresetList();
    bool isFactory = (idx >= 0 && idx < (int) presets.size())
                     ? presets[(size_t) idx].isFactory : true;
    deleteButton_.setEnabled (!isFactory);
}

void PM0AudioProcessorEditor::onSaveAsPressed()
{
    auto* w = new juce::AlertWindow ("Save Preset As", "Enter preset name:",
                                     juce::AlertWindow::NoIcon);
    w->addTextEditor ("name", "", "Preset name:");
    w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PM0AudioProcessorEditor> safe (this);
    w->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe, w] (int result)
        {
            if (result == 1 && safe != nullptr)
            {
                auto name = w->getTextEditorContents ("name").trim();
                if (name.isNotEmpty())
                {
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
    if (idx < 0 || idx >= (int) presets.size()) return;
    if (presets[(size_t) idx].isFactory) return;

    auto name = presets[(size_t) idx].name;
    juce::Component::SafePointer<PM0AudioProcessorEditor> safe (this);

    juce::AlertWindow::showOkCancelBox (
        juce::AlertWindow::WarningIcon, "Delete Preset",
        "Delete preset '" + name + "'?", "Delete", "Cancel", nullptr,
        juce::ModalCallbackFunction::create ([safe, idx] (int result)
        {
            if (result == 1 && safe != nullptr)
            {
                auto* pm2 = safe->proc_.getPresetManager();
                if (pm2 && pm2->deletePreset (idx)) safe->updatePresetList();
            }
        }));
}
