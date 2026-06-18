#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class PM0AudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer,
      private juce::ComboBox::Listener,
      private juce::Button::Listener
{
public:
    explicit PM0AudioProcessorEditor (PM0AudioProcessor&);
    ~PM0AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

    void updatePresetList();
    void onPresetSelected();
    void onSaveAsPressed();
    void onDeletePressed();

    PM0AudioProcessor& proc_;

    // Preset selector UI
    juce::Label    presetLabel_   { {}, "Preset:" };
    juce::ComboBox presetSelector_;
    juce::TextButton saveAsButton_ { "Save As..." };
    juce::TextButton deleteButton_ { "Delete" };

    // Oscillator controls
    juce::ComboBox oscWaveformBox_;
    juce::Label    oscWaveformLbl_;
    juce::Slider oscTuneKnob_, oscDetuneKnob_;
    juce::Label  oscTuneLbl_,  oscDetuneLbl_;

    // Filter controls
    juce::ComboBox filterModeBox_;
    juce::Label    filterModeLbl_;
    juce::Slider filterCutoffKnob_, filterResonanceKnob_;
    juce::Label  filterCutoffLbl_,  filterResonanceLbl_;

    // Volume envelope controls
    juce::Slider    envAttackKnob_, envDecayKnob_, envSustainKnob_, envReleaseKnob_;
    juce::Label     envAttackLbl_,  envDecayLbl_,  envSustainLbl_,  envReleaseLbl_;
    juce::TextButton envSustainBtn_ { "Sustain" };

    // Filter envelope controls
    juce::Slider    fenvAttackKnob_, fenvDecayKnob_, fenvSustainKnob_, fenvReleaseKnob_, envFilterModKnob_;
    juce::Label     fenvAttackLbl_,  fenvDecayLbl_,  fenvSustainLbl_,  fenvReleaseLbl_,  envFilterModLbl_;
    juce::TextButton fenvSustainBtn_ { "Sustain" };

    // LFO controls
    juce::ComboBox lfoTargetBox_;
    juce::Label    lfoTargetLbl_;
    juce::Slider lfoSpeedKnob_, lfoDepthKnob_;
    juce::Label  lfoSpeedLbl_,  lfoDepthLbl_;

    // Master controls
    juce::Slider outputGainKnob_;
    juce::Label  outputGainLbl_;

    float displayOutputPeak_ { -100.f };

    // APVTS attachments
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboBoxAttachment> oscWaveformAtt_, filterModeAtt_, lfoTargetAtt_;

    std::unique_ptr<SliderAttachment>
        oscTuneAtt_, oscDetuneAtt_,
        filterCutoffAtt_, filterResonanceAtt_,
        envAttackAtt_, envDecayAtt_, envSustainAtt_, envReleaseAtt_,
        fenvAttackAtt_, fenvDecayAtt_, fenvSustainAtt_, fenvReleaseAtt_, envFilterModAtt_,
        lfoSpeedAtt_, lfoDepthAtt_,
        outputGainAtt_;

    std::unique_ptr<ButtonAttachment> envSustainBtnAtt_, fenvSustainBtnAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PM0AudioProcessorEditor)
};
