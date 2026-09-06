#include "ArcanistPluginAdapter.h"
#include <algorithm>
#include <cstring>

START_NAMESPACE_DISTRHO

ArcanistPluginAdapter::ArcanistPluginAdapter()
    : Plugin(kArcanistParamCount, 0, 0)
{
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
        values_[i] = kArcanistParamSpecs[i].defaultValue;
}

const char* ArcanistPluginAdapter::getLabel() const { return "Arcanist"; }
const char* ArcanistPluginAdapter::getDescription() const { return "Soft pads and drones synthesizer"; }
const char* ArcanistPluginAdapter::getMaker() const { return "Spellbound"; }
const char* ArcanistPluginAdapter::getLicense() const { return "https://spellbound.audio/plugins/arcanist#license"; }
uint32_t ArcanistPluginAdapter::getVersion() const { return d_version(0, 1, 0); }

void ArcanistPluginAdapter::initParameter(const uint32_t index, Parameter& parameter)
{
    if (index >= kArcanistParamCount) return;
    const auto& spec = kArcanistParamSpecs[index];

    parameter.hints = kParameterIsAutomatable;
    if (spec.kind == ArcanistParamKind::Boolean) parameter.hints |= kParameterIsBoolean;
    if (spec.kind == ArcanistParamKind::Choice)  parameter.hints |= kParameterIsInteger;

    parameter.name   = spec.name;
    parameter.symbol = spec.id;
    parameter.ranges.min = spec.min;
    parameter.ranges.max = spec.max;
    parameter.ranges.def = spec.defaultValue;
}

float ArcanistPluginAdapter::getParameterValue(const uint32_t index) const
{
    return index < kArcanistParamCount ? values_[index] : 0.0f;
}

void ArcanistPluginAdapter::setParameterValue(const uint32_t index, const float value)
{
    if (index < kArcanistParamCount) values_[index] = value;
}

void ArcanistPluginAdapter::activate()
{
    voices_.clear();
    voices_.resize(16);
    for (auto& v : voices_)
        v.prepare(getSampleRate(), static_cast<int>(getBufferSize()), 3.0);
}

void ArcanistPluginAdapter::applyParametersToVoice(Voice& voice) const
{
    // Ported verbatim from Source/_juce_reference/PluginProcessor.cpp's
    // processBlock() per-voice setter block, indexed by ArcanistParameters
    // instead of apvts.getRawParameterValue(id).
    voice.setWaveform(static_cast<Oscillator::Waveform>(static_cast<int>(values_[kParamOscWaveform])));
    voice.setOscillatorTune(values_[kParamOscTune]);
    voice.setOscillatorDetune(values_[kParamOscDetune]);
    voice.setFilterMode(static_cast<Filter::Mode>(static_cast<int>(values_[kParamFilterMode])));
    voice.setFilterCutoff(values_[kParamFilterCutoff]);
    voice.setFilterResonance(values_[kParamFilterResonance]);
    voice.setEnvelopeAttack(values_[kParamEnvAttack]);
    voice.setEnvelopeDecay(values_[kParamEnvDecay]);
    voice.setEnvelopeSustain(values_[kParamEnvSustain]);
    voice.setEnvelopeRelease(values_[kParamEnvRelease]);
    voice.setEnvelopeFilterMod(values_[kParamEnvFilterMod]);
    voice.setEnvelopeSustainEnabled(values_[kParamEnvSustainOn] > 0.5f);
    voice.setFEnvAttack(values_[kParamFEnvAttack]);
    voice.setFEnvDecay(values_[kParamFEnvDecay]);
    voice.setFEnvSustain(values_[kParamFEnvSustain]);
    voice.setFEnvRelease(values_[kParamFEnvRelease]);
    voice.setFEnvSustainEnabled(values_[kParamFEnvSustainOn] > 0.5f);
    voice.setLFOTarget(static_cast<int>(values_[kParamLfoTarget]));
    voice.setLFOSpeed(values_[kParamLfoSpeed]);
    voice.setLFODepth(values_[kParamLfoDepth]);
    voice.setOutputGain(values_[kParamOutputGain]);

    voice.setOsc2Enabled(values_[kParamOsc2On] > 0.5f);
    voice.setOsc2Waveform(static_cast<Oscillator::Waveform>(static_cast<int>(values_[kParamOsc2Waveform])));
    voice.setOsc2Mult(static_cast<int>(values_[kParamOsc2Mult]));
    voice.setOsc2Phase(values_[kParamOsc2Phase]);
    voice.setOsc2MixMode(static_cast<Voice::MixMode>(static_cast<int>(values_[kParamOsc2MixMode])));
    voice.setOsc2MixDepth(values_[kParamOsc2MixDepth]);
    voice.setOsc2EnvEnabled(values_[kParamOsc2EnvOn] > 0.5f);
    voice.setOsc2EnvAttack(values_[kParamOsc2EnvAttack]);
    voice.setOsc2EnvDecay(values_[kParamOsc2EnvDecay]);
    voice.setOsc2EnvSustain(values_[kParamOsc2EnvSustain]);
    voice.setOsc2EnvRelease(values_[kParamOsc2EnvRelease]);
    voice.setOsc2EnvSustainEnabled(values_[kParamOsc2EnvSustainOn] > 0.5f);
    voice.setOsc2FilterEnabled(values_[kParamOsc2FltOn] > 0.5f);
    voice.setOsc2FilterCutoff(values_[kParamOsc2FltCutoff]);
    voice.setOsc2FilterResonance(values_[kParamOsc2FltResonance]);
    voice.setOsc2FilterMode(static_cast<Filter::Mode>(static_cast<int>(values_[kParamOsc2FltMode])));
    voice.setOsc2FEnvAttack(values_[kParamOsc2FEnvAttack]);
    voice.setOsc2FEnvDecay(values_[kParamOsc2FEnvDecay]);
    voice.setOsc2FEnvSustain(values_[kParamOsc2FEnvSustain]);
    voice.setOsc2FEnvRelease(values_[kParamOsc2FEnvRelease]);
    voice.setOsc2FEnvSustainEnabled(values_[kParamOsc2FEnvSustainOn] > 0.5f);
    voice.setOsc2FEnvDepth(values_[kParamOsc2FEnvDepth]);
}

void ArcanistPluginAdapter::handleNoteOn(const int note, const float velocity)
{
    // Ported verbatim from PluginProcessor.cpp's MIDI loop: reuse the same
    // note's voice if it's still ringing (cuts the release tail cleanly),
    // else any free voice, else steal the oldest active voice.
    int sameNoteVoice = -1, freeVoice = -1, oldestVoice = -1;
    float oldestTime = -1.0f;

    for (size_t i = 0; i < voices_.size(); ++i)
    {
        if (voices_[i].isActive() && voices_[i].getNoteNumber() == note) { sameNoteVoice = static_cast<int>(i); break; }
        if (!voices_[i].isActive() && freeVoice < 0) freeVoice = static_cast<int>(i);
        if (voices_[i].isActive() && voices_[i].getTimeSinceNoteOn() > oldestTime)
        {
            oldestTime = voices_[i].getTimeSinceNoteOn();
            oldestVoice = static_cast<int>(i);
        }
    }

    const int voiceIndex = sameNoteVoice >= 0 ? sameNoteVoice : (freeVoice >= 0 ? freeVoice : oldestVoice);
    if (voiceIndex >= 0)
        voices_[static_cast<size_t>(voiceIndex)].noteOn(note, velocity);
}

void ArcanistPluginAdapter::handleNoteOff(const int note)
{
    for (auto& voice : voices_)
        if (voice.getNoteNumber() == note)
            voice.noteOff();
}

void ArcanistPluginAdapter::handleMidiEvent(const MidiEvent& ev)
{
    if (ev.size < 3) return;
    const uint8_t status = ev.data[0] & 0xF0;
    const uint8_t note = ev.data[1];
    const uint8_t velocity = ev.data[2];

    if (status == 0x90 && velocity > 0) handleNoteOn(note, static_cast<float>(velocity) / 127.0f);
    else if (status == 0x80 || (status == 0x90 && velocity == 0)) handleNoteOff(note);
}

void ArcanistPluginAdapter::run(const float**, float** outputs, const uint32_t frames,
                                  const MidiEvent* midiEvents, const uint32_t midiEventCount)
{
    // On a full preset reload (Task 5's ArcanistUI::applyPreset() calls
    // requestAllNotesOff()): silence every voice's tail over 0.1 s, then
    // hard-kill, matching the JUCE-era PluginProcessor's ramp exactly.
    if (allNotesOffPending_)
    {
        allNotesOffPending_ = false;
        for (auto& voice : voices_) voice.noteOff();
        fadeOutSamplesTotal_ = static_cast<int>(getSampleRate() * 0.1);
        fadeOutSamplesRemaining_ = fadeOutSamplesTotal_;
    }

    for (auto& voice : voices_) applyParametersToVoice(voice);

    for (uint32_t i = 0; i < midiEventCount; ++i) handleMidiEvent(midiEvents[i]);

    dsp::AudioBuffer out;
    float* channels[2] = { outputs[0], outputs[1] };
    out.wrapExternal(channels, 2, static_cast<int>(frames));
    out.clear();

    for (auto& voice : voices_) voice.process(out);

    if (fadeOutSamplesRemaining_ > 0)
    {
        const float totalF = static_cast<float>(fadeOutSamplesTotal_);
        for (uint32_t n = 0; n < frames; ++n)
        {
            const float gain = fadeOutSamplesRemaining_ > 0
                              ? static_cast<float>(fadeOutSamplesRemaining_--) / totalF : 0.0f;
            outputs[0][n] *= gain;
            outputs[1][n] *= gain;
        }
        if (fadeOutSamplesRemaining_ == 0)
            for (auto& voice : voices_) voice.forceStop();
    }
}

Plugin* createPlugin() { return new ArcanistPluginAdapter(); }

END_NAMESPACE_DISTRHO
