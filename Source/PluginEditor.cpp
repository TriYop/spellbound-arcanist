#include "PluginEditor.h"

PM0AudioProcessorEditor::PM0AudioProcessorEditor (PM0AudioProcessor& p)
    : AudioProcessorEditor (&p), proc_ (p)
{
    setSize (1000, 500);

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

    // Oscillator
    addAndMakeVisible (oscTuneKnob_);
    oscTuneKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    oscTuneKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    oscTuneAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc_tune", oscTuneKnob_);

    addAndMakeVisible (oscTuneLbl_);
    oscTuneLbl_.setText ("Tune", juce::dontSendNotification);
    oscTuneLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (oscDetuneKnob_);
    oscDetuneKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    oscDetuneKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    oscDetuneAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "osc_detune", oscDetuneKnob_);

    addAndMakeVisible (oscDetuneLbl_);
    oscDetuneLbl_.setText ("Detune", juce::dontSendNotification);
    oscDetuneLbl_.setJustificationType (juce::Justification::centred);

    // Filter
    addAndMakeVisible (filterCutoffKnob_);
    filterCutoffKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    filterCutoffKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    filterCutoffAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "filter_cutoff", filterCutoffKnob_);

    addAndMakeVisible (filterCutoffLbl_);
    filterCutoffLbl_.setText ("Cutoff", juce::dontSendNotification);
    filterCutoffLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (filterResonanceKnob_);
    filterResonanceKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    filterResonanceKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    filterResonanceAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "filter_resonance", filterResonanceKnob_);

    addAndMakeVisible (filterResonanceLbl_);
    filterResonanceLbl_.setText ("Resonance", juce::dontSendNotification);
    filterResonanceLbl_.setJustificationType (juce::Justification::centred);

    // Envelope
    addAndMakeVisible (envAttackKnob_);
    envAttackKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    envAttackKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    envAttackAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_attack", envAttackKnob_);

    addAndMakeVisible (envAttackLbl_);
    envAttackLbl_.setText ("Attack", juce::dontSendNotification);
    envAttackLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (envDecayKnob_);
    envDecayKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    envDecayKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    envDecayAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_decay", envDecayKnob_);

    addAndMakeVisible (envDecayLbl_);
    envDecayLbl_.setText ("Decay", juce::dontSendNotification);
    envDecayLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (envSustainKnob_);
    envSustainKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    envSustainKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    envSustainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_sustain", envSustainKnob_);

    addAndMakeVisible (envSustainLbl_);
    envSustainLbl_.setText ("Sustain", juce::dontSendNotification);
    envSustainLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (envReleaseKnob_);
    envReleaseKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    envReleaseKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    envReleaseAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_release", envReleaseKnob_);

    addAndMakeVisible (envReleaseLbl_);
    envReleaseLbl_.setText ("Release", juce::dontSendNotification);
    envReleaseLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (envFilterModKnob_);
    envFilterModKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    envFilterModKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    envFilterModAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "env_filter_mod", envFilterModKnob_);

    addAndMakeVisible (envFilterModLbl_);
    envFilterModLbl_.setText ("Filter Mod", juce::dontSendNotification);
    envFilterModLbl_.setJustificationType (juce::Justification::centred);

    // LFO
    addAndMakeVisible (lfoSpeedKnob_);
    lfoSpeedKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lfoSpeedKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    lfoSpeedAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_speed", lfoSpeedKnob_);

    addAndMakeVisible (lfoSpeedLbl_);
    lfoSpeedLbl_.setText ("LFO Speed", juce::dontSendNotification);
    lfoSpeedLbl_.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (lfoDepthKnob_);
    lfoDepthKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lfoDepthKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    lfoDepthAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "lfo_depth", lfoDepthKnob_);

    addAndMakeVisible (lfoDepthLbl_);
    lfoDepthLbl_.setText ("LFO Depth", juce::dontSendNotification);
    lfoDepthLbl_.setJustificationType (juce::Justification::centred);

    // Master
    addAndMakeVisible (outputGainKnob_);
    outputGainKnob_.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainKnob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    outputGainAtt_ = std::make_unique<SliderAttachment> (proc_.apvts, "output_gain", outputGainKnob_);

    addAndMakeVisible (outputGainLbl_);
    outputGainLbl_.setText ("Output Gain", juce::dontSendNotification);
    outputGainLbl_.setJustificationType (juce::Justification::centred);

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
}

void PM0AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Preset selector bar at top (50px)
    auto presetArea = bounds.removeFromTop (50).reduced (5);
    presetLabel_.setBounds (presetArea.removeFromLeft (60));
    presetSelector_.setBounds (presetArea.removeFromLeft (300));
    presetArea.removeFromLeft (8); // gap
    saveAsButton_.setBounds (presetArea.removeFromLeft (100));
    presetArea.removeFromLeft (4); // gap
    deleteButton_.setBounds (presetArea.removeFromLeft (80));

    // Parameter controls in remaining space
    auto area = bounds.reduced (10);
    int knobSize = 70;
    int labelHeight = 18;

    // Oscillator group (top left)
    auto oscArea = area.removeFromLeft (180).reduced (5);
    oscArea.removeFromTop (5);
    oscTuneKnob_.setBounds (oscArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    oscTuneLbl_.setBounds (oscArea.removeFromTop (labelHeight));
    oscDetuneKnob_.setBounds (oscArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    oscDetuneLbl_.setBounds (oscArea.removeFromTop (labelHeight));

    // Filter group
    auto filterArea = area.removeFromLeft (180).reduced (5);
    filterArea.removeFromTop (5);
    filterCutoffKnob_.setBounds (filterArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    filterCutoffLbl_.setBounds (filterArea.removeFromTop (labelHeight));
    filterResonanceKnob_.setBounds (filterArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    filterResonanceLbl_.setBounds (filterArea.removeFromTop (labelHeight));

    // Envelope group (2x3 grid)
    auto envArea = area.removeFromLeft (280).reduced (5);
    envArea.removeFromTop (5);

    auto row1 = envArea.removeFromTop (knobSize + labelHeight + 15);
    auto col1 = row1.removeFromLeft (90);
    auto col2 = row1.removeFromLeft (90);
    auto col3 = row1;

    envAttackKnob_.setBounds (col1.removeFromTop (knobSize + labelHeight + 10).reduced (3));
    envAttackLbl_.setBounds (col1.removeFromTop (labelHeight));

    envDecayKnob_.setBounds (col2.removeFromTop (knobSize + labelHeight + 10).reduced (3));
    envDecayLbl_.setBounds (col2.removeFromTop (labelHeight));

    envSustainKnob_.setBounds (col3.removeFromTop (knobSize + labelHeight + 10).reduced (3));
    envSustainLbl_.setBounds (col3.removeFromTop (labelHeight));

    auto row2 = envArea.removeFromTop (knobSize + labelHeight + 15);
    col1 = row2.removeFromLeft (90);
    col2 = row2.removeFromLeft (90);

    envReleaseKnob_.setBounds (col1.removeFromTop (knobSize + labelHeight + 10).reduced (3));
    envReleaseLbl_.setBounds (col1.removeFromTop (labelHeight));

    envFilterModKnob_.setBounds (col2.removeFromTop (knobSize + labelHeight + 10).reduced (3));
    envFilterModLbl_.setBounds (col2.removeFromTop (labelHeight));

    // LFO group
    auto lfoArea = area.removeFromLeft (180).reduced (5);
    lfoArea.removeFromTop (5);
    lfoSpeedKnob_.setBounds (lfoArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    lfoSpeedLbl_.setBounds (lfoArea.removeFromTop (labelHeight));
    lfoDepthKnob_.setBounds (lfoArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    lfoDepthLbl_.setBounds (lfoArea.removeFromTop (labelHeight));

    // Master (right side with meter)
    auto masterArea = area;
    outputGainKnob_.setBounds (masterArea.removeFromTop (knobSize + labelHeight + 10).reduced (5));
    outputGainLbl_.setBounds (masterArea.removeFromTop (labelHeight));
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

    // Disable delete for factory presets (indices 0-23)
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

    // Refresh delete button enable state
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
    // AlertWindow::runModalLoop() is not permitted in JUCE 8 plugins.
    // Use enterModalState with an async callback instead.
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
        true /* deleteWhenDismissed */);
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

    // Guard: never delete factory presets (belt-and-suspenders; button should be disabled)
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
