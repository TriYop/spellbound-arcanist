#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Synthesis/Voice.h"
#include "PresetManager.h"
#include <vector>
#include <memory>

class PM0AudioProcessorEditor;

class PM0AudioProcessor final : public juce::AudioProcessor
{
public:
    PM0AudioProcessor();
    ~PM0AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi()  const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    PresetManager* getPresetManager() { return presetManager_.get(); }

    std::atomic<float> outputPeakDb       { -100.f };
    std::atomic<bool>  allNotesOffPending { false  };

    // 0.1 s output ramp applied on preset change; only touched on audio thread
    int fadeOutSamplesTotal_     = 0;
    int fadeOutSamplesRemaining_ = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::unique_ptr<PresetManager> presetManager_;
    std::vector<Voice> voices_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PM0AudioProcessor)
};
