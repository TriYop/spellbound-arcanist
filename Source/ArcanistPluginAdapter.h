#pragma once
#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class ArcanistPluginAdapter : public Plugin
{
public:
    ArcanistPluginAdapter() : Plugin(1, 0, 0) {}

protected:
    const char* getLabel() const override { return "Arcanist"; }
    const char* getDescription() const override { return "Soft pads and drones synthesizer"; }
    const char* getMaker() const override { return "Spellbound"; }
    const char* getLicense() const override { return "https://spellbound.audio/plugins/arcanist#license"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

    void initParameter(uint32_t, Parameter&) override {}
    float getParameterValue(uint32_t) const override { return 0.0f; }
    void setParameterValue(uint32_t, float) override {}

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent*, uint32_t) override
    {
        // Stub: silence. Task 4 replaces this with real voice rendering.
        std::memset(outputs[0], 0, sizeof(float) * frames);
        std::memset(outputs[1], 0, sizeof(float) * frames);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistPluginAdapter)
};

END_NAMESPACE_DISTRHO
