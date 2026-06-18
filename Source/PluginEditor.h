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

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback () override;
    void buttonClicked (juce::Button*) override;

    void updatePresetList ();
    void onPresetSelected ();
    void onSaveAsPressed  ();
    void onDeletePressed  ();

    void setupKnob (juce::Slider& knob, juce::Label& lbl, const juce::String& labelText);

    void paintSection (juce::Graphics&, juce::Rectangle<int>, const juce::String& title,
                       juce::Colour accent, bool dimmed = false) const;

    // ── Processor reference ───────────────────────────────────────────────────
    PM0AudioProcessor& proc_;
    PM0LookAndFeel     laf_;

    // ── Preset bar ────────────────────────────────────────────────────────────
    juce::Label      presetLabel_  { {}, "PRESET" };
    juce::ComboBox   presetSelector_;
    juce::TextButton saveAsButton_ { "SAVE AS" };
    juce::TextButton deleteButton_ { "DELETE"  };

    // ══════════════════════════════════════════════════════════════════════════
    //  ROW 1 — Osc 1 chain
    // ══════════════════════════════════════════════════════════════════════════

    // Oscillator 1
    LedButton oscWaveformBtns_[4] {
        LedButton{"SINE"}, LedButton{"TRI"}, LedButton{"SAW"}, LedButton{"SQR"}
    };
    juce::Slider oscTuneKnob_, oscDetuneKnob_;
    juce::Label  oscTuneLbl_,  oscDetuneLbl_;

    // Filter 1
    LedButton filterModeBtns_[3] {
        LedButton{"LP"}, LedButton{"BP"}, LedButton{"HP"}
    };
    juce::Slider filterCutoffKnob_, filterResonanceKnob_;
    juce::Label  filterCutoffLbl_,  filterResonanceLbl_;

    // Volume envelope 1
    juce::Slider envAttackKnob_, envDecayKnob_, envSustainKnob_, envReleaseKnob_;
    juce::Label  envAttackLbl_,  envDecayLbl_,  envSustainLbl_,  envReleaseLbl_;
    LedButton    envSustainBtn_ { "SUSTAIN", PM0Col::volEnv() };

    // Filter envelope 1
    juce::Slider fenvAttackKnob_, fenvDecayKnob_, fenvSustainKnob_, fenvReleaseKnob_, envFilterModKnob_;
    juce::Label  fenvAttackLbl_,  fenvDecayLbl_,  fenvSustainLbl_,  fenvReleaseLbl_,  envFilterModLbl_;
    LedButton    fenvSustainBtn_ { "SUSTAIN", PM0Col::fEnv() };

    // LFO
    LedButton lfoTargetBtns_[3] {
        LedButton{"FILTER"}, LedButton{"AMP"}, LedButton{"PITCH"}
    };
    juce::Slider lfoSpeedKnob_, lfoDepthKnob_;
    juce::Label  lfoSpeedLbl_,  lfoDepthLbl_;

    // Master
    juce::Slider outputGainKnob_;
    juce::Label  outputGainLbl_;
    float        displayOutputPeak_ { -100.f };

    // ══════════════════════════════════════════════════════════════════════════
    //  ROW 2 — Osc 2 chain
    // ══════════════════════════════════════════════════════════════════════════

    // Osc 2 oscillator
    LedButton osc2OnBtn_           { "OSC 2 ON",  PM0Col::osc() };  // master bypass
    LedButton osc2WaveformBtns_[5] {
        LedButton{"SINE"}, LedButton{"TRI"}, LedButton{"SAW"}, LedButton{"SQR"}, LedButton{"NOISE"}
    };
    LedButton osc2MultBtns_[4] {
        LedButton{"x0.5"}, LedButton{"x1"}, LedButton{"x2"}, LedButton{"x4"}
    };
    juce::Slider osc2PhaseKnob_;
    juce::Label  osc2PhaseLbl_;

    // Mix mode
    LedButton osc2MixModeBtns_[4] {
        LedButton{"SUM"}, LedButton{"AM"}, LedButton{"FM"}, LedButton{"RING"}
    };
    juce::Slider osc2MixDepthKnob_;
    juce::Label  osc2MixDepthLbl_;

    // Vol envelope 2
    LedButton    osc2EnvOnBtn_    { "ENV ON",  PM0Col::volEnv() };
    juce::Slider osc2EnvAtkKnob_, osc2EnvDecKnob_, osc2EnvSusKnob_, osc2EnvRelKnob_;
    juce::Label  osc2EnvAtkLbl_,  osc2EnvDecLbl_,  osc2EnvSusLbl_,  osc2EnvRelLbl_;
    LedButton    osc2EnvSusBtn_   { "SUSTAIN", PM0Col::volEnv() };

    // Filter 2
    LedButton osc2FltOnBtn_      { "FILT ON", PM0Col::filter() };
    LedButton osc2FltModeBtns_[3] {
        LedButton{"LP"}, LedButton{"BP"}, LedButton{"HP"}
    };
    juce::Slider osc2FltCutKnob_, osc2FltResoKnob_;
    juce::Label  osc2FltCutLbl_,  osc2FltResoLbl_;

    // Filter envelope 2
    juce::Slider osc2FenvAtkKnob_, osc2FenvDecKnob_, osc2FenvSusKnob_, osc2FenvRelKnob_, osc2FenvDepthKnob_;
    juce::Label  osc2FenvAtkLbl_,  osc2FenvDecLbl_,  osc2FenvSusLbl_,  osc2FenvRelLbl_,  osc2FenvDepthLbl_;
    LedButton    osc2FenvSusBtn_  { "SUSTAIN", PM0Col::fEnv() };

    // ── Section layout rects ──────────────────────────────────────────────────
    // Row 1
    juce::Rectangle<int> oscSect_, filterSect_, volEnvSect_, fEnvSect_, lfoSect_, masterSect_;
    // Row 2
    juce::Rectangle<int> osc2Sect_, mix2Sect_, env2Sect_, flt2Sect_, fenv2Sect_;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Row 1 attachments
    std::unique_ptr<SliderAttachment>
        oscTuneAtt_, oscDetuneAtt_,
        filterCutoffAtt_, filterResonanceAtt_,
        envAttackAtt_, envDecayAtt_, envSustainAtt_, envReleaseAtt_,
        fenvAttackAtt_, fenvDecayAtt_, fenvSustainAtt_, fenvReleaseAtt_, envFilterModAtt_,
        lfoSpeedAtt_, lfoDepthAtt_,
        outputGainAtt_;
    std::unique_ptr<ButtonAttachment> envSustainBtnAtt_, fenvSustainBtnAtt_;

    // Row 2 attachments
    std::unique_ptr<ButtonAttachment>
        osc2OnAtt_,
        osc2EnvOnAtt_, osc2EnvSusAtt_,
        osc2FltOnAtt_,
        osc2FenvSusAtt_;
    std::unique_ptr<SliderAttachment>
        osc2PhaseAtt_, osc2MixDepthAtt_,
        osc2EnvAtkAtt_, osc2EnvDecAtt_, osc2EnvSusKnobAtt_, osc2EnvRelAtt_,
        osc2FltCutAtt_, osc2FltResoAtt_,
        osc2FenvAtkAtt_, osc2FenvDecAtt_, osc2FenvSusKnobAtt_, osc2FenvRelAtt_, osc2FenvDepthAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PM0AudioProcessorEditor)
};
