#include "PluginEditor.h"

static void setupKnob (juce::Slider& knob, juce::Label& lbl,
                       const juce::String& labelText)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    lbl.setText (labelText, juce::dontSendNotification);
    lbl.setJustificationType (juce::Justification::centred);
}

PM0AudioProcessorEditor::PM0AudioProcessorEditor (PM0AudioProcessor& p)
    : AudioProcessorEditor (&p), proc_ (p)
{
    setSize (1150, 500);

    // Preset selector bar
    addAndMakeVisible (presetLabel_);
    presetLabel_.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (presetSelector_);
    presetSelector_.addListener (this);

    addAndMakeVisible (saveAsButton_);
    saveAsButton_.addListener (this);

    addAndMakeVisible (deleteButton_);
    deleteButton_.addListener (this);

    updatePresetList();

    // Oscillator — waveform selector
    oscWaveformBox_.addItem ("Sine",     1);
    oscWaveformBox_.addItem ("Triangle", 2);
    oscWaveformBox_.addItem ("Sawtooth", 3);
    oscWaveformBox_.addItem ("Square",   4);
    addAndMakeVisible (oscWaveformBox_);
    oscWaveformAtt_ = std::make_unique<ComboBoxAttachment> (proc_.apvts, "osc_waveform", oscWaveformBox_);

    addAndMakeVisible (oscWaveformLbl_);
    oscWaveformLbl_.setText ("Waveform", juce::dontSendNotification);
    oscWaveformLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (oscTuneKnob_);    addAndMakeVisible (oscTuneLbl_);
    addAndMakeVisible (oscDetuneKnob_);  addAndMakeVisible (oscDetuneLbl_);
    setupKnob (oscTuneKnob_,   oscTuneLbl_,   "Tune");
    setupKnob (oscDetuneKnob_, oscDetuneLbl_, "Detune");
    oscTuneAtt_    = std::make_unique<SliderAttachment> (proc_.apvts, "osc_tune",    oscTuneKnob_);
    oscDetuneAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "osc_detune",  oscDetuneKnob_);

    // Filter — mode selector
    filterModeBox_.addItem ("Low Pass",  1);
    filterModeBox_.addItem ("Band Pass", 2);
    filterModeBox_.addItem ("High Pass", 3);
    addAndMakeVisible (filterModeBox_);
    filterModeAtt_ = std::make_unique<ComboBoxAttachment> (proc_.apvts, "filter_mode", filterModeBox_);

    addAndMakeVisible (filterModeLbl_);
    filterModeLbl_.setText ("Mode", juce::dontSendNotification);
    filterModeLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (filterCutoffKnob_);    addAndMakeVisible (filterCutoffLbl_);
    addAndMakeVisible (filterResonanceKnob_); addAndMakeVisible (filterResonanceLbl_);
    setupKnob (filterCutoffKnob_,    filterCutoffLbl_,    "Cutoff");
    setupKnob (filterResonanceKnob_, filterResonanceLbl_, "Resonance");
    filterCutoffAtt_    = std::make_unique<SliderAttachment> (proc_.apvts, "filter_cutoff",     filterCutoffKnob_);
    filterResonanceAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "filter_resonance",  filterResonanceKnob_);

    // Volume Envelope
    addAndMakeVisible (envAttackKnob_);   addAndMakeVisible (envAttackLbl_);
    addAndMakeVisible (envDecayKnob_);    addAndMakeVisible (envDecayLbl_);
    addAndMakeVisible (envSustainKnob_);  addAndMakeVisible (envSustainLbl_);
    addAndMakeVisible (envReleaseKnob_);  addAndMakeVisible (envReleaseLbl_);
    setupKnob (envAttackKnob_,  envAttackLbl_,  "Attack");
    setupKnob (envDecayKnob_,   envDecayLbl_,   "Decay");
    setupKnob (envSustainKnob_, envSustainLbl_, "Sustain");
    setupKnob (envReleaseKnob_, envReleaseLbl_, "Release");
    envAttackAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "env_attack",   envAttackKnob_);
    envDecayAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "env_decay",    envDecayKnob_);
    envSustainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_sustain",  envSustainKnob_);
    envReleaseAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_release",  envReleaseKnob_);

    envSustainBtn_.setClickingTogglesState (true);
    addAndMakeVisible (envSustainBtn_);
    envSustainBtnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "env_sustain_on", envSustainBtn_);

    // Filter Envelope
    addAndMakeVisible (fenvAttackKnob_);   addAndMakeVisible (fenvAttackLbl_);
    addAndMakeVisible (fenvDecayKnob_);    addAndMakeVisible (fenvDecayLbl_);
    addAndMakeVisible (fenvSustainKnob_);  addAndMakeVisible (fenvSustainLbl_);
    addAndMakeVisible (fenvReleaseKnob_);  addAndMakeVisible (fenvReleaseLbl_);
    addAndMakeVisible (envFilterModKnob_); addAndMakeVisible (envFilterModLbl_);
    setupKnob (fenvAttackKnob_,  fenvAttackLbl_,  "Attack");
    setupKnob (fenvDecayKnob_,   fenvDecayLbl_,   "Decay");
    setupKnob (fenvSustainKnob_, fenvSustainLbl_, "Sustain");
    setupKnob (fenvReleaseKnob_, fenvReleaseLbl_, "Release");
    setupKnob (envFilterModKnob_, envFilterModLbl_, "Depth");
    fenvAttackAtt_  = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_attack",     fenvAttackKnob_);
    fenvDecayAtt_   = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_decay",      fenvDecayKnob_);
    fenvSustainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_sustain",    fenvSustainKnob_);
    fenvReleaseAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "fenv_release",    fenvReleaseKnob_);
    envFilterModAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_filter_mod", envFilterModKnob_);

    fenvSustainBtn_.setClickingTogglesState (true);
    addAndMakeVisible (fenvSustainBtn_);
    fenvSustainBtnAtt_ = std::make_unique<ButtonAttachment> (proc_.apvts, "fenv_sustain_on", fenvSustainBtn_);

    // LFO — target selector
    lfoTargetBox_.addItem ("Filter",    1);
    lfoTargetBox_.addItem ("Amplitude", 2);
    lfoTargetBox_.addItem ("Pitch",     3);
    addAndMakeVisible (lfoTargetBox_);
    lfoTargetAtt_ = std::make_unique<ComboBoxAttachment> (proc_.apvts, "lfo_target", lfoTargetBox_);

    addAndMakeVisible (lfoTargetLbl_);
    lfoTargetLbl_.setText ("LFO Target", juce::dontSendNotification);
    lfoTargetLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (lfoSpeedKnob_); addAndMakeVisible (lfoSpeedLbl_);
    addAndMakeVisible (lfoDepthKnob_); addAndMakeVisible (lfoDepthLbl_);
    setupKnob (lfoSpeedKnob_, lfoSpeedLbl_, "LFO Speed");
    setupKnob (lfoDepthKnob_, lfoDepthLbl_, "LFO Depth");
    lfoSpeedAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_speed", lfoSpeedKnob_);
    lfoDepthAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_depth", lfoDepthKnob_);

    // Master
    addAndMakeVisible (outputGainKnob_); addAndMakeVisible (outputGainLbl_);
    setupKnob (outputGainKnob_, outputGainLbl_, "Output Gain");
    outputGainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "output_gain", outputGainKnob_);

    startTimer (30);
}

PM0AudioProcessorEditor::~PM0AudioProcessorEditor()
{
    stopTimer();
}

void PM0AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);

    // Draw output meter — positioned to the right, below the preset bar
    auto meterArea = getLocalBounds().withTrimmedTop (50).removeFromRight (50).reduced (5);
    g.setColour (juce::Colours::lightgrey);
    g.drawRect (meterArea);

    g.setColour (juce::Colours::green);
    float meterHeight = juce::jmap (displayOutputPeak_, -100.f, 0.f, 0.f, (float)meterArea.getHeight());
    g.fillRect (meterArea.withHeight ((int)meterHeight));

    g.setColour (juce::Colours::white);
    g.setFont (10);
    g.drawText (juce::String (displayOutputPeak_, 1), meterArea.reduced (2), juce::Justification::centred);

    // Section header labels
    g.setFont (11.f);
    g.setColour (juce::Colours::lightgrey.withAlpha (0.7f));
    auto bounds   = getLocalBounds().withTrimmedTop (50);
    auto area     = bounds.reduced (10);
    int  secW[6]  = { 160, 160, 270, 270, 160, 80 };
    const char* secNames[] = { "OSCILLATOR", "FILTER", "VOLUME ENV", "FILTER ENV", "LFO", "MASTER" };
    for (int i = 0; i < 6; ++i)
    {
        auto sec = area.removeFromLeft (secW[i]);
        g.drawText (secNames[i], sec.removeFromTop (14), juce::Justification::centred);
    }
}

void PM0AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Preset selector bar at top (50px)
    auto presetArea = bounds.removeFromTop (50).reduced (5);
    presetLabel_.setBounds (presetArea.removeFromLeft (60));
    presetSelector_.setBounds (presetArea.removeFromLeft (300));
    presetArea.removeFromLeft (8);
    saveAsButton_.setBounds (presetArea.removeFromLeft (100));
    presetArea.removeFromLeft (4);
    deleteButton_.setBounds (presetArea.removeFromLeft (80));

    // Parameter controls in remaining space
    auto area       = bounds.reduced (10);
    area.removeFromTop (14); // space for section headers painted in paint()
    int knobSize    = 70;
    int labelHeight = 18;
    int rowHeight   = knobSize + labelHeight + 12;
    int btnHeight   = 28;

    // Helper: lay out 3 knobs side-by-side in a strip
    auto placeKnobRow = [&] (juce::Rectangle<int> strip,
                             juce::Slider& k1, juce::Label& l1,
                             juce::Slider& k2, juce::Label& l2,
                             juce::Slider& k3, juce::Label& l3)
    {
        int colW = strip.getWidth() / 3;
        auto c1  = strip.removeFromLeft (colW);
        auto c2  = strip.removeFromLeft (colW);
        auto c3  = strip;
        k1.setBounds (c1.removeFromTop (rowHeight).reduced (3));
        l1.setBounds (c1.removeFromTop (labelHeight));
        k2.setBounds (c2.removeFromTop (rowHeight).reduced (3));
        l2.setBounds (c2.removeFromTop (labelHeight));
        k3.setBounds (c3.removeFromTop (rowHeight).reduced (3));
        l3.setBounds (c3.removeFromTop (labelHeight));
    };

    // ── Oscillator (160px) ──────────────────────────────────────────────
    auto oscArea = area.removeFromLeft (160).reduced (5);
    oscWaveformLbl_.setBounds (oscArea.removeFromTop (labelHeight));
    oscWaveformBox_.setBounds (oscArea.removeFromTop (24).reduced (2));
    oscArea.removeFromTop (6);
    oscTuneKnob_.setBounds   (oscArea.removeFromTop (rowHeight).reduced (3));
    oscTuneLbl_.setBounds    (oscArea.removeFromTop (labelHeight));
    oscDetuneKnob_.setBounds (oscArea.removeFromTop (rowHeight).reduced (3));
    oscDetuneLbl_.setBounds  (oscArea.removeFromTop (labelHeight));

    // ── Filter (160px) ──────────────────────────────────────────────────
    auto filterArea = area.removeFromLeft (160).reduced (5);
    filterModeLbl_.setBounds    (filterArea.removeFromTop (labelHeight));
    filterModeBox_.setBounds    (filterArea.removeFromTop (24).reduced (2));
    filterArea.removeFromTop (6);
    filterCutoffKnob_.setBounds    (filterArea.removeFromTop (rowHeight).reduced (3));
    filterCutoffLbl_.setBounds     (filterArea.removeFromTop (labelHeight));
    filterResonanceKnob_.setBounds (filterArea.removeFromTop (rowHeight).reduced (3));
    filterResonanceLbl_.setBounds  (filterArea.removeFromTop (labelHeight));

    // ── Volume Envelope (270px) ─────────────────────────────────────────
    // Row 1: Attack, Decay, Sustain
    // Row 2: Release, [spacer], [Sustain toggle button]
    {
        auto envArea = area.removeFromLeft (270).reduced (5);

        auto row1 = envArea.removeFromTop (rowHeight + labelHeight + 4);
        placeKnobRow (row1,
                      envAttackKnob_,  envAttackLbl_,
                      envDecayKnob_,   envDecayLbl_,
                      envSustainKnob_, envSustainLbl_);

        auto row2 = envArea.removeFromTop (rowHeight + labelHeight + 4);
        int  colW = row2.getWidth() / 3;
        auto c1   = row2.removeFromLeft (colW);
        row2.removeFromLeft (colW); // spacer
        auto c3   = row2;

        envReleaseKnob_.setBounds (c1.removeFromTop (rowHeight).reduced (3));
        envReleaseLbl_.setBounds  (c1.removeFromTop (labelHeight));

        // Sustain toggle button: centred in c3
        envSustainBtn_.setBounds (c3.withSizeKeepingCentre (80, btnHeight));
    }

    // ── Filter Envelope (270px) ─────────────────────────────────────────
    // Row 1: Attack, Decay, Sustain
    // Row 2: Release, Depth, [Sustain toggle button]
    {
        auto fenvArea = area.removeFromLeft (270).reduced (5);

        auto row1 = fenvArea.removeFromTop (rowHeight + labelHeight + 4);
        placeKnobRow (row1,
                      fenvAttackKnob_,  fenvAttackLbl_,
                      fenvDecayKnob_,   fenvDecayLbl_,
                      fenvSustainKnob_, fenvSustainLbl_);

        auto row2 = fenvArea.removeFromTop (rowHeight + labelHeight + 4);
        int  colW = row2.getWidth() / 3;
        auto c1   = row2.removeFromLeft (colW);
        auto c2   = row2.removeFromLeft (colW);
        auto c3   = row2;

        fenvReleaseKnob_.setBounds  (c1.removeFromTop (rowHeight).reduced (3));
        fenvReleaseLbl_.setBounds   (c1.removeFromTop (labelHeight));
        envFilterModKnob_.setBounds (c2.removeFromTop (rowHeight).reduced (3));
        envFilterModLbl_.setBounds  (c2.removeFromTop (labelHeight));

        // Sustain toggle button: centred in c3
        fenvSustainBtn_.setBounds (c3.withSizeKeepingCentre (80, btnHeight));
    }

    // ── LFO (160px) ─────────────────────────────────────────────────────
    auto lfoArea = area.removeFromLeft (160).reduced (5);
    lfoTargetLbl_.setBounds (lfoArea.removeFromTop (labelHeight));
    lfoTargetBox_.setBounds (lfoArea.removeFromTop (24).reduced (2));
    lfoArea.removeFromTop (6);
    lfoSpeedKnob_.setBounds (lfoArea.removeFromTop (rowHeight).reduced (3));
    lfoSpeedLbl_.setBounds  (lfoArea.removeFromTop (labelHeight));
    lfoDepthKnob_.setBounds (lfoArea.removeFromTop (rowHeight).reduced (3));
    lfoDepthLbl_.setBounds  (lfoArea.removeFromTop (labelHeight));

    // ── Master (remaining, minus 50px meter on far right) ────────────────
    auto masterArea = area;
    outputGainKnob_.setBounds (masterArea.removeFromTop (rowHeight).reduced (3));
    outputGainLbl_.setBounds  (masterArea.removeFromTop (labelHeight));
}

void PM0AudioProcessorEditor::timerCallback()
{
    auto newPeak = proc_.outputPeakDb.load (std::memory_order_relaxed);
    if (std::abs (newPeak - displayOutputPeak_) > 0.5f)
    {
        displayOutputPeak_ = newPeak;
        repaint();
    }
}

void PM0AudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetSelector_)
        onPresetSelected();
}

void PM0AudioProcessorEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == &saveAsButton_)
        onSaveAsPressed();
    else if (buttonThatWasClicked == &deleteButton_)
        onDeletePressed();
}

void PM0AudioProcessorEditor::updatePresetList()
{
    presetSelector_.clear (juce::dontSendNotification);

    auto* pm = proc_.getPresetManager();
    if (pm == nullptr)
        return;

    auto presets = pm->getPresetList();
    for (size_t i = 0; i < presets.size(); ++i)
        presetSelector_.addItem (presets[i].name, static_cast<int> (i + 1));

    int currentIndex = pm->getCurrentPresetIndex();
    presetSelector_.setSelectedItemIndex (currentIndex, juce::dontSendNotification);

    bool isFactory = (currentIndex >= 0 && currentIndex < static_cast<int> (presets.size()))
                     ? presets[static_cast<size_t> (currentIndex)].isFactory
                     : true;
    deleteButton_.setEnabled (!isFactory);
}

void PM0AudioProcessorEditor::onPresetSelected()
{
    int selectedIndex = presetSelector_.getSelectedItemIndex();
    if (selectedIndex >= 0)
        proc_.setCurrentProgram (selectedIndex);

    auto* pm = proc_.getPresetManager();
    if (pm == nullptr)
        return;

    auto presets = pm->getPresetList();
    bool isFactory = (selectedIndex >= 0 && selectedIndex < static_cast<int> (presets.size()))
                     ? presets[static_cast<size_t> (selectedIndex)].isFactory
                     : true;
    deleteButton_.setEnabled (!isFactory);
}

void PM0AudioProcessorEditor::onSaveAsPressed()
{
    auto* w = new juce::AlertWindow ("Save Preset As",
                                     "Enter preset name:",
                                     juce::AlertWindow::NoIcon);
    w->addTextEditor ("presetName", "", "Preset name:");
    w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PM0AudioProcessorEditor> safeThis (this);

    w->enterModalState (true,
        juce::ModalCallbackFunction::create ([safeThis, w] (int result)
        {
            if (result == 1 && safeThis != nullptr)
            {
                auto name = w->getTextEditorContents ("presetName").trim();
                if (name.isNotEmpty())
                {
                    auto* pm = safeThis->proc_.getPresetManager();
                    if (pm != nullptr && pm->savePreset (name))
                        safeThis->updatePresetList();
                }
            }
        }),
        true);
}

void PM0AudioProcessorEditor::onDeletePressed()
{
    auto* pm = proc_.getPresetManager();
    if (pm == nullptr)
        return;

    int currentIndex = pm->getCurrentPresetIndex();
    auto presets = pm->getPresetList();

    if (currentIndex < 0 || currentIndex >= static_cast<int> (presets.size()))
        return;

    if (presets[static_cast<size_t> (currentIndex)].isFactory)
        return;

    auto name = presets[static_cast<size_t> (currentIndex)].name;

    juce::Component::SafePointer<PM0AudioProcessorEditor> safeThis (this);

    juce::AlertWindow::showOkCancelBox (
        juce::AlertWindow::WarningIcon,
        "Delete Preset",
        "Delete preset '" + name + "'?",
        "Delete", "Cancel", nullptr,
        juce::ModalCallbackFunction::create ([safeThis, currentIndex] (int result)
        {
            if (result == 1 && safeThis != nullptr)
            {
                auto* pm2 = safeThis->proc_.getPresetManager();
                if (pm2 != nullptr && pm2->deletePreset (currentIndex))
                    safeThis->updatePresetList();
            }
        }));
}
