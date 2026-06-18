#include "PluginProcessor.h"
#include "PluginEditor.h"

PM0AudioProcessor::PM0AudioProcessor()
    : AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "Parameters", createParameterLayout())
    , presetManager_ (std::make_unique<PresetManager> (apvts))
{
}

PM0AudioProcessor::~PM0AudioProcessor()
{
}

const juce::String PM0AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PM0AudioProcessor::acceptsMidi() const
{
    return true;
}

bool PM0AudioProcessor::producesMidi() const
{
    return false;
}

bool PM0AudioProcessor::isMidiEffect() const
{
    return false;
}

double PM0AudioProcessor::getTailLengthSeconds() const
{
    return 3.0; // Allow tail time for sustaining pads
}

int PM0AudioProcessor::getNumPrograms()
{
    if (presetManager_)
    {
        auto presets = presetManager_->getPresetList();
        return static_cast<int> (presets.size());
    }
    return 24; // Minimum: factory presets
}

int PM0AudioProcessor::getCurrentProgram()
{
    if (presetManager_)
        return presetManager_->getCurrentPresetIndex();
    return 0;
}

void PM0AudioProcessor::setCurrentProgram (int index)
{
    if (presetManager_)
        presetManager_->loadPreset (index);
}

const juce::String PM0AudioProcessor::getProgramName (int index)
{
    if (presetManager_)
    {
        auto presets = presetManager_->getPresetList();
        if (index >= 0 && index < static_cast<int> (presets.size()))
            return presets[index].name;
    }
    return {};
}

void PM0AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    if (presetManager_)
        presetManager_->renamePreset (index, newName);
}

void PM0AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voices_.clear();
    voices_.resize (16); // Polyphony of 16

    for (auto& voice : voices_)
        voice.prepare (sampleRate, samplesPerBlock, getTailLengthSeconds());
}

void PM0AudioProcessor::releaseResources()
{
    voices_.clear();
}

bool PM0AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PM0AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update all voice parameters from APVTS
    float oscTune = *apvts.getRawParameterValue ("osc_tune");
    float oscDetune = *apvts.getRawParameterValue ("osc_detune");
    float filterCutoff = *apvts.getRawParameterValue ("filter_cutoff");
    float filterResonance = *apvts.getRawParameterValue ("filter_resonance");
    float envAttack = *apvts.getRawParameterValue ("env_attack");
    float envDecay = *apvts.getRawParameterValue ("env_decay");
    float envSustain = *apvts.getRawParameterValue ("env_sustain");
    float envRelease = *apvts.getRawParameterValue ("env_release");
    float envFilterMod = *apvts.getRawParameterValue ("env_filter_mod");
    float lfoSpeed = *apvts.getRawParameterValue ("lfo_speed");
    float lfoDepth = *apvts.getRawParameterValue ("lfo_depth");
    float outputGain = *apvts.getRawParameterValue ("output_gain");
    auto waveform = static_cast<Oscillator::Waveform> (
        static_cast<int> (*apvts.getRawParameterValue ("osc_waveform")));

    for (auto& voice : voices_)
    {
        voice.setWaveform (waveform);
        voice.setOscillatorTune (oscTune);
        voice.setOscillatorDetune (oscDetune);
        voice.setFilterCutoff (filterCutoff);
        voice.setFilterResonance (filterResonance);
        voice.setEnvelopeAttack (envAttack);
        voice.setEnvelopeDecay (envDecay);
        voice.setEnvelopeSustain (envSustain);
        voice.setEnvelopeRelease (envRelease);
        voice.setEnvelopeFilterMod (envFilterMod);
        voice.setLFOSpeed (lfoSpeed);
        voice.setLFODepth (lfoDepth);
        voice.setOutputGain (outputGain);
    }

    // Process MIDI events and route to voices
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            // Find the next available voice (active but oldest, or an inactive one)
            int voiceIndex = -1;
            float oldestTime = -1.f;

            for (size_t i = 0; i < voices_.size(); ++i)
            {
                if (!voices_[i].isActive())
                {
                    voiceIndex = static_cast<int> (i);
                    break;
                }
                else if (voices_[i].getTimeSinceNoteOn() > oldestTime)
                {
                    oldestTime = voices_[i].getTimeSinceNoteOn();
                    voiceIndex = static_cast<int> (i);
                }
            }

            if (voiceIndex >= 0)
            {
                auto& voice = voices_[static_cast<size_t> (voiceIndex)];
                voice.noteOn (msg.getNoteNumber(), msg.getVelocity() / 127.f);
            }
        }
        else if (msg.isNoteOff())
        {
            for (auto& voice : voices_)
                if (voice.getNoteNumber() == msg.getNoteNumber())
                    voice.noteOff();
        }
    }

    // Render all voices
    buffer.clear();

    for (auto& voice : voices_)
    {
        voice.process (buffer);
    }

    // Measure output peak
    float peakLevel = 0.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peakLevel = std::max (peakLevel, buffer.getRMSLevel (ch, 0, buffer.getNumSamples()));

    outputPeakDb.store (juce::Decibels::gainToDecibels (peakLevel, -100.f), std::memory_order_relaxed);
}

bool PM0AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PM0AudioProcessor::createEditor()
{
    return new PM0AudioProcessorEditor (*this);
}

void PM0AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.state;
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PM0AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout PM0AudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Oscillator parameters
    params.push_back (std::make_unique<AudioParameterChoice>
        ("osc_waveform", "Waveform",
         StringArray { "Sine", "Triangle", "Sawtooth", "Square" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc_tune", "Oscillator Tune", -24.f, 24.f, 0.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc_detune", "Oscillator Detune", -50.f, 50.f, 0.f));

    // Filter parameters
    params.push_back (std::make_unique<AudioParameterFloat>
        ("filter_cutoff", "Filter Cutoff", 20.f, 20000.f, 4000.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("filter_resonance", "Filter Resonance", 0.f, 1.f, 0.f));

    // Envelope parameters
    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_attack", "Attack Time", 0.01f, 5.f, 0.1f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_decay", "Decay Time", 0.f, 5.f, 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_sustain", "Sustain Level", 0.f, 1.f, 0.8f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_release", "Release Time", 0.5f, 10.f, 3.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_filter_mod", "Filter Envelope Modulation", -100.f, 100.f, 0.f));

    // LFO parameters
    params.push_back (std::make_unique<AudioParameterFloat>
        ("lfo_speed", "LFO Speed", 0.1f, 10.f, 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("lfo_depth", "LFO Depth", 0.f, 100.f, 0.f));

    // Master parameters
    params.push_back (std::make_unique<AudioParameterFloat>
        ("output_gain", "Output Gain", -24.f, 12.f, 0.f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PM0AudioProcessor();
}
