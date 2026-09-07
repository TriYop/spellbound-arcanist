#pragma once
#include "DistrhoUI.hpp"
#include "ArcanistParams.h"
#include "UI/RadioButtonGroup.h"
#include "audioplugins/common/hui/dgl/RotaryKnob.h"
#include "audioplugins/common/hui/dgl/ToggleSwitch.h"
#include "audioplugins/common/hui/dgl/VuMeter.h"
#include "audioplugins/common/hui/dgl/PresetSelector.h"
#include "audioplugins/common/hui/dgl/Button.h"
#include "audioplugins/common/hui/dgl/MidiKeyboard.h"
#include "audioplugins/common/presets/PresetBrowser.h"
#include <array>
#include <memory>
#include <vector>

START_NAMESPACE_DISTRHO
namespace hui = audioplugins::common::hui;

class ArcanistUI : public UI
{
public:
    ArcanistUI();

protected:
    void parameterChanged(uint32_t index, float value) override;
    void onNanoDisplay() override;
    void uiFileBrowserSelected(const char* filename) override;

private:
    void applyPreset(const audioplugins::common::presets::Preset& preset);
    std::vector<audioplugins::common::presets::ParameterValue> captureCurrentParameters() const;
    void refreshPresetControls();
    void setKnobOrSwitch(uint32_t paramIndex, float value); // dispatches by ArcanistParamKind

    // Pure-geometry helper -- a minimal JUCE-Rectangle-alike so the fixed
    // (non-resizable, DISTRHO_UI_USER_RESIZABLE 0) layout below can be
    // ported section-by-section from the JUCE-era PluginEditor.cpp's
    // resized() using the same "carve off a strip" idiom, without pulling
    // in a JUCE dependency for a handful of rectangle operations.
    struct Rect
    {
        float x = 0.f, y = 0.f, w = 0.f, h = 0.f;

        Rect removeFromTop(float amount) noexcept
        {
            amount = amount < h ? amount : h;
            const Rect top{x, y, w, amount};
            y += amount; h -= amount;
            return top;
        }
        Rect removeFromLeft(float amount) noexcept
        {
            amount = amount < w ? amount : w;
            const Rect left{x, y, amount, h};
            x += amount; w -= amount;
            return left;
        }
        Rect reduced(float dx, float dy) const noexcept { return Rect{x + dx, y + dy, w - 2.f * dx, h - 2.f * dy}; }
        Rect reduced(float d) const noexcept { return reduced(d, d); }
        Rect withTrimmedTop(float amount) const noexcept { return Rect{x, y + amount, w, h - amount}; }
        Rect withWidth(float newW) const noexcept { return Rect{x, y, newW, h}; }
        Rect withSizeKeepingCentre(float nw, float nh) const noexcept
        {
            return Rect{x + (w - nw) * 0.5f, y + (h - nh) * 0.5f, nw, nh};
        }
    };

    struct SectionInfo
    {
        Rect rect;
        const char* title;
        hui::Colour accent;
    };

    // Populates knobs_/switches_/radioGroups_ absolute positions and
    // sections_/labelRects_ (for onNanoDisplay's headers/knob labels).
    // Ported section-by-section from the JUCE-era PluginEditor.cpp's
    // resized() -- pure geometry, ported as-is rather than re-derived.
    void layoutWidgets();

    // One entry per continuous parameter (kArcanistParamCount-sized, only
    // Continuous-kind slots populated) / boolean / choice -- indexed
    // directly by ArcanistParameters, avoiding 43 hand-named members.
    std::array<std::unique_ptr<hui::dgl::RotaryKnob>, kArcanistParamCount> knobs_;
    std::array<std::unique_ptr<hui::dgl::ToggleSwitch>, kArcanistParamCount> switches_;
    std::array<std::unique_ptr<ui::RadioButtonGroup>, kArcanistParamCount> radioGroups_;

    // Label rect for every Continuous/Boolean parameter (drawn by
    // onNanoDisplay -- RotaryKnob/ToggleSwitch draw no text of their own).
    // Choice parameters need no entry: RadioButtonGroup's buttons draw
    // their own labels.
    std::array<Rect, kArcanistParamCount> labelRects_{};

    std::vector<SectionInfo> sections_;

    std::unique_ptr<hui::dgl::VuMeter> outputMeter_;
    std::unique_ptr<hui::dgl::MidiKeyboard> keyboard_;

    audioplugins::common::presets::PresetBrowser presetBrowser_;
    std::unique_ptr<hui::dgl::PresetSelector> presetSelector_;
    std::unique_ptr<hui::dgl::Button> saveButton_, deleteButton_;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistUI)
};

END_NAMESPACE_DISTRHO
