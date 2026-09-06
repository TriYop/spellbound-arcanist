#pragma once
#include "DistrhoPlugin.hpp"
#include "ArcanistParams.h"
#include "Synthesis/Voice.h"
#include "Synthesis/AudioBuffer.h"
#include <array>
#include <vector>

START_NAMESPACE_DISTRHO

/**
   DPF Plugin adapter for Spellbound Arcanist.

   Thin shim over a 16-voice pool of the existing framework-free Voice
   (Source/Synthesis/Voice.h): reads all 43 host parameters generically via
   kArcanistParamSpecs (Task 4 Step 2), applies them to every voice once per
   block (applyParametersToVoice(), ported verbatim from the JUCE-era
   PluginProcessor::processBlock()'s per-voice setter calls), dispatches
   DPF's MidiEvent array through the same same-note-reuse / free-voice /
   oldest-voice-steal algorithm the JUCE version used, and wraps DPF's raw
   float** run() output in a non-owning dsp::AudioBuffer (see
   AudioBuffer::wrapExternal(), Task 2) so Voice::process() needs no
   modification at all.
 */
class ArcanistPluginAdapter : public Plugin
{
public:
    ArcanistPluginAdapter();

protected:
    const char* getLabel() const override;
    const char* getDescription() const override;
    const char* getMaker() const override;
    const char* getLicense() const override;
    uint32_t getVersion() const override;

    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;

    void activate() override;
    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override;

private:
    void applyParametersToVoice(Voice& voice) const;
    void handleMidiEvent(const MidiEvent& ev);
    void handleNoteOn(int note, float velocity);
    void handleNoteOff(int note);

    std::array<float, kArcanistParamCount> values_;
    std::vector<Voice> voices_; // 16, matches JUCE-era polyphony

    // 0.1 s output ramp applied on a full-state reload (preset change via
    // ArcanistUI, see Task 5) -- ported from the JUCE-era
    // fadeOutSamplesTotal_/fadeOutSamplesRemaining_/allNotesOffPending.
    bool allNotesOffPending_ = false;
    int fadeOutSamplesTotal_ = 0;
    int fadeOutSamplesRemaining_ = 0;

public:
    // Called by ArcanistUI (Task 5) right after applying a preset's values,
    // so the audio thread starts the same silence-then-retrigger ramp the
    // JUCE-era PresetManager triggered via setCurrentProgram().
    void requestAllNotesOff() { allNotesOffPending_ = true; }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistPluginAdapter)
};

END_NAMESPACE_DISTRHO
