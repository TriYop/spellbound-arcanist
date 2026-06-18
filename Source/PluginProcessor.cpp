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
    {
        allNotesOffPending = true;
        presetManager_->loadPreset (index);
    }
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

    // On preset change: release all held notes so the old sound fades to silence
    if (allNotesOffPending.exchange (false))
        for (auto& voice : voices_)
            voice.noteOff();

    // Update all voice parameters from APVTS
    float oscTune = *apvts.getRawParameterValue ("osc_tune");
    float oscDetune = *apvts.getRawParameterValue ("osc_detune");
    float filterCutoff = *apvts.getRawParameterValue ("filter_cutoff");
    float filterResonance = *apvts.getRawParameterValue ("filter_resonance");
    float envAttack = *apvts.getRawParameterValue ("env_attack");
    float envDecay = *apvts.getRawParameterValue ("env_decay");
    float envSustain = *apvts.getRawParameterValue ("env_sustain");
    float envRelease = *apvts.getRawParameterValue ("env_release");
    float envFilterMod     = *apvts.getRawParameterValue ("env_filter_mod");
    bool  envSustainOn     = *apvts.getRawParameterValue ("env_sustain_on") > 0.5f;
    float fenvAttack       = *apvts.getRawParameterValue ("fenv_attack");
    float fenvDecay        = *apvts.getRawParameterValue ("fenv_decay");
    float fenvSustain      = *apvts.getRawParameterValue ("fenv_sustain");
    float fenvRelease      = *apvts.getRawParameterValue ("fenv_release");
    bool  fenvSustainOn    = *apvts.getRawParameterValue ("fenv_sustain_on") > 0.5f;
    float lfoSpeed = *apvts.getRawParameterValue ("lfo_speed");
    float lfoDepth = *apvts.getRawParameterValue ("lfo_depth");
    float outputGain = *apvts.getRawParameterValue ("output_gain");
    auto waveform = static_cast<Oscillator::Waveform> (
        static_cast<int> (*apvts.getRawParameterValue ("osc_waveform")));
    auto filterMode = static_cast<Filter::Mode> (
        static_cast<int> (*apvts.getRawParameterValue ("filter_mode")));
    int lfoTarget = static_cast<int> (*apvts.getRawParameterValue ("lfo_target"));

    // ── Osc 2 params ────────────────────────────────────────────────────────────
    bool  osc2On        = *apvts.getRawParameterValue ("osc2_on")         > 0.5f;
    int   osc2Waveform  = static_cast<int> (*apvts.getRawParameterValue ("osc2_waveform"));
    int   osc2Mult      = static_cast<int> (*apvts.getRawParameterValue ("osc2_mult"));
    float osc2Phase     = *apvts.getRawParameterValue ("osc2_phase");
    int   osc2MixModeI  = static_cast<int> (*apvts.getRawParameterValue ("osc2_mix_mode"));
    float osc2MixDepth  = *apvts.getRawParameterValue ("osc2_mix_depth");
    bool  osc2EnvOn     = *apvts.getRawParameterValue ("osc2_env_on")     > 0.5f;
    float osc2EnvAtk    = *apvts.getRawParameterValue ("osc2_env_attack");
    float osc2EnvDec    = *apvts.getRawParameterValue ("osc2_env_decay");
    float osc2EnvSus    = *apvts.getRawParameterValue ("osc2_env_sustain");
    float osc2EnvRel    = *apvts.getRawParameterValue ("osc2_env_release");
    bool  osc2EnvSusOn  = *apvts.getRawParameterValue ("osc2_env_sus_on") > 0.5f;
    bool  osc2FltOn     = *apvts.getRawParameterValue ("osc2_flt_on")     > 0.5f;
    float osc2FltCut    = *apvts.getRawParameterValue ("osc2_flt_cutoff");
    float osc2FltReso   = *apvts.getRawParameterValue ("osc2_flt_reso");
    int   osc2FltModeI  = static_cast<int> (*apvts.getRawParameterValue ("osc2_flt_mode"));
    float osc2FenvAtk   = *apvts.getRawParameterValue ("osc2_fenv_atk");
    float osc2FenvDec   = *apvts.getRawParameterValue ("osc2_fenv_dec");
    float osc2FenvSus   = *apvts.getRawParameterValue ("osc2_fenv_sus");
    float osc2FenvRel   = *apvts.getRawParameterValue ("osc2_fenv_rel");
    bool  osc2FenvSusOn = *apvts.getRawParameterValue ("osc2_fenv_sus_on") > 0.5f;
    float osc2FenvDepth = *apvts.getRawParameterValue ("osc2_fenv_depth");

    auto osc2MixModeE  = static_cast<Voice::MixMode>       (osc2MixModeI);
    auto osc2FltModeE  = static_cast<Filter::Mode>         (osc2FltModeI);
    auto osc2WaveformE = static_cast<Oscillator::Waveform> (osc2Waveform);

    for (auto& voice : voices_)
    {
        voice.setWaveform (waveform);
        voice.setOscillatorTune (oscTune);
        voice.setOscillatorDetune (oscDetune);
        voice.setFilterMode (filterMode);
        voice.setFilterCutoff (filterCutoff);
        voice.setFilterResonance (filterResonance);
        voice.setEnvelopeAttack (envAttack);
        voice.setEnvelopeDecay (envDecay);
        voice.setEnvelopeSustain (envSustain);
        voice.setEnvelopeRelease (envRelease);
        voice.setEnvelopeFilterMod (envFilterMod);
        voice.setEnvelopeSustainEnabled (envSustainOn);
        voice.setFEnvAttack  (fenvAttack);
        voice.setFEnvDecay   (fenvDecay);
        voice.setFEnvSustain (fenvSustain);
        voice.setFEnvRelease (fenvRelease);
        voice.setFEnvSustainEnabled (fenvSustainOn);
        voice.setLFOTarget (lfoTarget);
        voice.setLFOSpeed (lfoSpeed);
        voice.setLFODepth (lfoDepth);
        voice.setOutputGain (outputGain);

        // Osc 2 chain
        voice.setOsc2Enabled            (osc2On);
        voice.setOsc2Waveform           (osc2WaveformE);
        voice.setOsc2Mult               (osc2Mult);
        voice.setOsc2Phase              (osc2Phase);
        voice.setOsc2MixMode            (osc2MixModeE);
        voice.setOsc2MixDepth           (osc2MixDepth);
        voice.setOsc2EnvEnabled         (osc2EnvOn);
        voice.setOsc2EnvAttack          (osc2EnvAtk);
        voice.setOsc2EnvDecay           (osc2EnvDec);
        voice.setOsc2EnvSustain         (osc2EnvSus);
        voice.setOsc2EnvRelease         (osc2EnvRel);
        voice.setOsc2EnvSustainEnabled  (osc2EnvSusOn);
        voice.setOsc2FilterEnabled      (osc2FltOn);
        voice.setOsc2FilterCutoff       (osc2FltCut);
        voice.setOsc2FilterResonance    (osc2FltReso);
        voice.setOsc2FilterMode         (osc2FltModeE);
        voice.setOsc2FEnvAttack         (osc2FenvAtk);
        voice.setOsc2FEnvDecay          (osc2FenvDec);
        voice.setOsc2FEnvSustain        (osc2FenvSus);
        voice.setOsc2FEnvRelease        (osc2FenvRel);
        voice.setOsc2FEnvSustainEnabled (osc2FenvSusOn);
        voice.setOsc2FEnvDepth          (osc2FenvDepth);
    }

    // Process MIDI events and route to voices
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            // Priority 1: re-use the voice already playing this note (cuts release tail)
            // Priority 2: any free (inactive) voice
            // Priority 3: steal the oldest active voice
            int sameNoteVoice = -1;
            int freeVoice     = -1;
            int oldestVoice   = -1;
            float oldestTime  = -1.f;

            for (size_t i = 0; i < voices_.size(); ++i)
            {
                if (voices_[i].isActive() && voices_[i].getNoteNumber() == msg.getNoteNumber())
                {
                    sameNoteVoice = static_cast<int> (i);
                    break;
                }
                if (!voices_[i].isActive() && freeVoice < 0)
                    freeVoice = static_cast<int> (i);
                if (voices_[i].isActive() && voices_[i].getTimeSinceNoteOn() > oldestTime)
                {
                    oldestTime  = voices_[i].getTimeSinceNoteOn();
                    oldestVoice = static_cast<int> (i);
                }
            }

            int voiceIndex = (sameNoteVoice >= 0) ? sameNoteVoice
                           : (freeVoice     >= 0) ? freeVoice
                                                   : oldestVoice;
            if (voiceIndex >= 0)
                voices_[static_cast<size_t> (voiceIndex)].noteOn (msg.getNoteNumber(), msg.getVelocity() / 127.f);
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
    params.push_back (std::make_unique<AudioParameterChoice>
        ("filter_mode", "Filter Mode",
         StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("filter_cutoff", "Filter Cutoff", 20.f, 20000.f, 4000.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("filter_resonance", "Filter Resonance", 0.f, 1.f, 0.f));

    // Envelope parameters
    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_attack", "Attack Time", 0.001f, 5.f, 0.1f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_decay", "Decay Time", 0.f, 5.f, 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_sustain", "Sustain Level", 0.f, 1.f, 0.8f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_release", "Release Time", 0.5f, 10.f, 3.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("env_filter_mod", "Filter Envelope Modulation", -100.f, 100.f, 0.f));

    // Sustain bypass (ADSR ↔ ADR)
    params.push_back (std::make_unique<AudioParameterBool>
        ("env_sustain_on", "Amp Sustain On", true));

    // Filter envelope (dedicated ADSR for filter cutoff)
    params.push_back (std::make_unique<AudioParameterFloat>
        ("fenv_attack",  "Filter Env Attack",  0.001f, 5.f,  0.001f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("fenv_decay",   "Filter Env Decay",   0.f,    5.f,  1.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("fenv_sustain", "Filter Env Sustain", 0.f,    1.f,  0.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("fenv_release", "Filter Env Release", 0.001f, 10.f, 3.f));

    params.push_back (std::make_unique<AudioParameterBool>
        ("fenv_sustain_on", "Filter Sustain On", true));

    // LFO parameters
    params.push_back (std::make_unique<AudioParameterChoice>
        ("lfo_target", "LFO Target",
         StringArray { "Filter", "Amplitude", "Pitch" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("lfo_speed", "LFO Speed", 0.1f, 10.f, 0.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("lfo_depth", "LFO Depth", 0.f, 100.f, 0.f));

    // Master parameters
    params.push_back (std::make_unique<AudioParameterFloat>
        ("output_gain", "Output Gain", -24.f, 12.f, 0.f));

    // ── Oscillator 2 chain ─────────────────────────────────────────────────────
    params.push_back (std::make_unique<AudioParameterBool>
        ("osc2_on", "Osc 2 On", false));

    params.push_back (std::make_unique<AudioParameterChoice>
        ("osc2_waveform", "Osc 2 Waveform",
         StringArray { "Sine", "Triangle", "Sawtooth", "Square", "Noise" }, 2));

    params.push_back (std::make_unique<AudioParameterChoice>
        ("osc2_mult", "Osc 2 Multiplier",
         StringArray { "0.5x", "1x", "2x", "4x" }, 1));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_phase", "Osc 2 Phase", 0.f, 360.f, 0.f));

    params.push_back (std::make_unique<AudioParameterChoice>
        ("osc2_mix_mode", "Osc 2 Mix Mode",
         StringArray { "SUM", "AM", "FM", "RING" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_mix_depth", "Osc 2 Mix Depth", 0.f, 100.f, 50.f));

    // Vol envelope 2
    params.push_back (std::make_unique<AudioParameterBool>
        ("osc2_env_on", "Osc 2 Env On", false));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_env_attack",  "Osc 2 Env Attack",  0.001f, 5.f,  0.1f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_env_decay",   "Osc 2 Env Decay",   0.f,    5.f,  0.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_env_sustain", "Osc 2 Env Sustain", 0.f,    1.f,  0.8f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_env_release", "Osc 2 Env Release", 0.001f, 10.f, 3.f));

    params.push_back (std::make_unique<AudioParameterBool>
        ("osc2_env_sus_on", "Osc 2 Env Sustain On", true));

    // Filter 2
    params.push_back (std::make_unique<AudioParameterBool>
        ("osc2_flt_on", "Osc 2 Filter On", false));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_flt_cutoff", "Osc 2 Filter Cutoff", 20.f, 20000.f, 4000.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_flt_reso", "Osc 2 Filter Resonance", 0.f, 1.f, 0.f));

    params.push_back (std::make_unique<AudioParameterChoice>
        ("osc2_flt_mode", "Osc 2 Filter Mode",
         StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));

    // Filter envelope 2
    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_fenv_atk", "Osc 2 FEnv Attack",  0.001f, 5.f,   0.001f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_fenv_dec", "Osc 2 FEnv Decay",   0.f,    5.f,   1.5f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_fenv_sus", "Osc 2 FEnv Sustain", 0.f,    1.f,   0.f));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_fenv_rel", "Osc 2 FEnv Release", 0.001f, 10.f,  3.f));

    params.push_back (std::make_unique<AudioParameterBool>
        ("osc2_fenv_sus_on", "Osc 2 FEnv Sustain On", true));

    params.push_back (std::make_unique<AudioParameterFloat>
        ("osc2_fenv_depth", "Osc 2 FEnv Depth", -100.f, 100.f, 0.f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PM0AudioProcessor();
}
