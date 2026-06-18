#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/PM0LookAndFeel.h"
#include "UI/LedButton.h"

class PM0AudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer,
      private juce::Button::Listener
{
public:
    explicit PM0AudioProcessorEditor (PM0AudioProcessor&);
    ~PM0AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // ── Internal helpers ──────────────────────────────────────────────────────
    void timerCallback() override;
    void buttonClicked (juce::Button*) override;

    void updatePresetList();
    void onPresetSelected();
    void onSaveAsPressed();
    void onDeletePressed();

    void setupKnob (juce::Slider& knob, juce::Label& lbl, const juce::String& labelText);

    // Draws one section panel (background + border + accent top-strip + title).
    void paintSection (juce::Graphics& g,
                       juce::Rectangle<int> rect,
                       const juce::String& title,
                       juce::Colour accent) const;

    // ── Processor reference ───────────────────────────────────────────────────
    PM0AudioProcessor& proc_;
    PM0LookAndFeel     laf_;

    // ── Preset bar ────────────────────────────────────────────────────────────
    juce::Label      presetLabel_   { {}, "PRESET" };
    juce::ComboBox   presetSelector_;
    juce::TextButton saveAsButton_  { "SAVE AS" };
    juce::TextButton deleteButton_  { "DELETE" };

    // ── Oscillator ────────────────────────────────────────────────────────────
    LedButton oscWaveformBtns_[4] {
        LedButton { "SINE" },  LedButton { "TRI" },
        LedButton { "SAW"  },  LedButton { "SQR" }
    };
    juce::Slider oscTuneKnob_, oscDetuneKnob_;
    juce::Label  oscTuneLbl_,  oscDetuneLbl_;

    // ── Filter ────────────────────────────────────────────────────────────────
    LedButton filterModeBtns_[3] {
        LedButton { "LP" }, LedButton { "BP" }, LedButton { "HP" }
    };
    juce::Slider filterCutoffKnob_, filterResonanceKnob_;
    juce::Label  filterCutoffLbl_,  filterResonanceLbl_;

    // ── Volume envelope ───────────────────────────────────────────────────────
    juce::Slider envAttackKnob_, envDecayKnob_, envSustainKnob_, envReleaseKnob_;
    juce::Label  envAttackLbl_,  envDecayLbl_,  envSustainLbl_,  envReleaseLbl_;
    LedButton    envSustainBtn_ { "SUSTAIN", PM0Col::volEnv() };

    // ── Filter envelope ───────────────────────────────────────────────────────
    juce::Slider fenvAttackKnob_, fenvDecayKnob_, fenvSustainKnob_, fenvReleaseKnob_, envFilterModKnob_;
    juce::Label  fenvAttackLbl_,  fenvDecayLbl_,  fenvSustainLbl_,  fenvReleaseLbl_,  envFilterModLbl_;
    LedButton    fenvSustainBtn_ { "SUSTAIN", PM0Col::fEnv() };

    // ── LFO ───────────────────────────────────────────────────────────────────
    LedButton lfoTargetBtns_[3] {
        LedButton { "FILTER" }, LedButton { "AMP" }, LedButton { "PITCH" }
    };
    juce::Slider lfoSpeedKnob_, lfoDepthKnob_;
    juce::Label  lfoSpeedLbl_,  lfoDepthLbl_;

    // ── Master ────────────────────────────────────────────────────────────────
    juce::Slider outputGainKnob_;
    juce::Label  outputGainLbl_;

    float displayOutputPeak_ { -100.f };

    // ── Section layout rects (set in resized, used in paint) ─────────────────
    juce::Rectangle<int> oscSect_, filterSect_, volEnvSect_, fEnvSect_, lfoSect_, masterSect_;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

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
