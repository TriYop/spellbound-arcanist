#include "ArcanistUI.h"
#include "ArcanistPluginAdapter.h"
#include "FactoryPresets.h"

#include <algorithm>
#include <cstdlib>
#include <string>

START_NAMESPACE_DISTRHO

namespace {

// hui::Colour (plain RGBA, framework-free) -> DGL's NanoVG-backed Color,
// same conversion helper as PugilistUI.cpp's toDglColor().
DGL_NAMESPACE::Color toDglColor(const hui::Colour& c) noexcept
{
    return DGL_NAMESPACE::Color(static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b),
                                 static_cast<float>(c.a) / 255.f);
}

// ArcanistCol palette, ported from Source/_juce_reference/UI/ArcanistLookAndFeel.h.
const hui::dgl::RotaryKnobPalette kKnobPalette = {
    /* track        */ {0x13, 0x15, 0x13, 0xff},
    /* valueArc     */ {0xe8, 0x88, 0x14, 0xff},
    /* valueArcGlow */ {0xe8, 0xa8, 0x44, 0xff},
    /* knobTop      */ {0x4e, 0x52, 0x48, 0xff},
    /* knobBottom   */ {0x25, 0x28, 0x25, 0xff},
    /* knobRim      */ {0x5c, 0x60, 0x58, 0xff},
};
const hui::dgl::ToggleSwitchPalette kTogglePalette = {
    /* bezel      */ {0x0a, 0x1a, 0x0a, 0xff},
    /* track      */ {0x1a, 0x20, 0x20, 0xff},
    /* activeGlow */ {0x22, 0xe8, 0x40, 0xff},
    /* thumbTop   */ {0x4e, 0x52, 0x48, 0xff},
    /* thumbBottom*/ {0x25, 0x28, 0x25, 0xff},
    /* thumbRim   */ {0x5c, 0x60, 0x58, 0xff},
};
const hui::dgl::ButtonPalette kRadioActivePalette = {
    /* background         */ {0x2a, 0x30, 0x30, 0xff},
    /* backgroundDisabled */ {0x1a, 0x20, 0x20, 0xff},
    /* border             */ {0x5c, 0x60, 0x58, 0xff},
    /* text               */ {0xcc, 0xc8, 0xb4, 0xff},
    /* textDisabled       */ {0x4c, 0x50, 0x4a, 0xff},
};
const hui::dgl::ButtonPalette kRadioInactivePalette = {
    /* background         */ {0x1a, 0x20, 0x20, 0xff},
    /* backgroundDisabled */ {0x1a, 0x20, 0x20, 0xff},
    /* border             */ {0x32, 0x3a, 0x32, 0xff},
    /* text               */ {0x4c, 0x50, 0x4a, 0xff},
    /* textDisabled       */ {0x4c, 0x50, 0x4a, 0xff},
};
const hui::dgl::ButtonPalette kBarButtonPalette = {
    /* background         */ {0x1e, 0x22, 0x20, 0xff},
    /* backgroundDisabled */ {0x16, 0x18, 0x17, 0xff},
    /* border             */ {0x32, 0x3a, 0x32, 0xff},
    /* text               */ {0xcc, 0xc8, 0xb4, 0xff},
    /* textDisabled       */ {0x4c, 0x50, 0x4a, 0xff},
};
const hui::dgl::VuMeterPalette kMeterPalette = {
    /* background */ {0x11, 0x13, 0x11, 0xff},
    /* border     */ {0x32, 0x3a, 0x32, 0xff},
    /* fillLow    */ {0x22, 0xaa, 0x44, 0xff},
    /* fillHigh   */ {0xe8, 0x88, 0x14, 0xff},
    /* peakLine   */ {0xff, 0xff, 0xff, 0xff},
};
const hui::dgl::PresetSelectorPalette kPresetSelectorPalette = {
    /* closedBackground */ {0x1e, 0x22, 0x20, 0xff},
    /* listBackground   */ {0x14, 0x17, 0x15, 0xff},
    /* border           */ {0x32, 0x3a, 0x32, 0xff},
    /* text             */ {0xcc, 0xc8, 0xb4, 0xff},
    /* textFactory      */ {0x8a, 0x8f, 0x84, 0xff},
    /* rowHighlight     */ {0xe8, 0x88, 0x14, 0x40},
};
const hui::dgl::MidiKeyboardPalette kKeyboardPalette = {
    /* whiteKey           */ {0xe8, 0xe6, 0xe0, 0xff},
    /* blackKey           */ {0x1a, 0x18, 0x20, 0xff},
    /* keyBorder          */ {0x10, 0x10, 0x14, 0xff},
    /* pressedOverlay     */ {0xe8, 0x88, 0x14, 0xff},
    /* octaveButton       */ {0x1e, 0x22, 0x20, 0xff},
    /* octaveButtonBorder */ {0x32, 0x3a, 0x32, 0xff},
    /* octaveButtonGlyph  */ {0xcc, 0xc8, 0xb4, 0xff},
};

// Background/panel/text tokens, ported from ArcanistCol.
constexpr hui::Colour kBgColor{0x18, 0x1b, 0x18, 0xff};
constexpr hui::Colour kPanelColor{0x1e, 0x22, 0x20, 0xff};
constexpr hui::Colour kPanelBorderColor{0x32, 0x3a, 0x32, 0xff};
constexpr hui::Colour kTextPrimary{0xcc, 0xc8, 0xb4, 0xff};
constexpr hui::Colour kTextDim{0x88, 0x80, 0x78, 0xff};

// Section accent colours, ported from ArcanistCol::osc()/filter()/volEnv()/
// fEnv()/lfo()/master(). Row 2 (Osc 2 chain) reuses the same accents as the
// analogous Row 1 section -- the JUCE original had no separate accent set
// for the mirrored chain either.
constexpr hui::Colour kAccentOsc{0xe8, 0xcc, 0x60, 0xff};
constexpr hui::Colour kAccentFilter{0x60, 0xc8, 0xe8, 0xff};
constexpr hui::Colour kAccentVolEnv{0x80, 0xe8, 0x60, 0xff};
constexpr hui::Colour kAccentFEnv{0xb0, 0x60, 0xe8, 0xff};
constexpr hui::Colour kAccentLfo{0xe8, 0x80, 0x40, 0xff};
constexpr hui::Colour kAccentMaster{0xd8, 0xd4, 0xc0, 0xff};

// Choice-parameter label sets, transcribed from the JUCE-era
// PluginEditor.h's LedButton arrays.
const std::vector<std::string> kOscWaveformLabels  = {"SINE", "TRI", "SAW", "SQR"};
const std::vector<std::string> kFilterModeLabels   = {"LP", "BP", "HP"};
const std::vector<std::string> kLfoTargetLabels    = {"FILTER", "AMP", "PITCH"};
const std::vector<std::string> kOsc2WaveformLabels = {"SINE", "TRI", "SAW", "SQR", "NOISE"};
const std::vector<std::string> kOsc2MultLabels     = {"x0.5", "x1", "x2", "x4"};
const std::vector<std::string> kOsc2MixModeLabels  = {"SUM", "AM", "FM", "RING"};

// Short knob/switch caption per continuous/boolean parameter, transcribed
// verbatim from the JUCE-era PluginEditor.cpp's setupKnob()/LedButton
// construction calls (RotaryKnob/ToggleSwitch draw no text of their own --
// see RotaryKnob.h/ToggleSwitch.h -- so ArcanistUI draws these itself in
// onNanoDisplay()). Left as "" for Choice-kind slots: RadioButtonGroup's
// buttons render their own labels (see kOscWaveformLabels etc. above).
const char* const kShortLabel[kArcanistParamCount] = {
    /* kParamOscWaveform     */ "",
    /* kParamOscTune         */ "TUNE",
    /* kParamOscDetune       */ "DETUNE",
    /* kParamFilterMode      */ "",
    /* kParamFilterCutoff    */ "CUTOFF",
    /* kParamFilterResonance */ "RESO",
    /* kParamEnvAttack       */ "ATTACK",
    /* kParamEnvDecay        */ "DECAY",
    /* kParamEnvSustain      */ "SUSTAIN",
    /* kParamEnvRelease      */ "RELEASE",
    /* kParamEnvFilterMod    */ "DEPTH",
    /* kParamEnvSustainOn    */ "SUSTAIN",
    /* kParamFEnvAttack      */ "ATTACK",
    /* kParamFEnvDecay       */ "DECAY",
    /* kParamFEnvSustain     */ "SUSTAIN",
    /* kParamFEnvRelease     */ "RELEASE",
    /* kParamFEnvSustainOn   */ "SUSTAIN",
    /* kParamLfoTarget       */ "",
    /* kParamLfoSpeed        */ "SPEED",
    /* kParamLfoDepth        */ "DEPTH",
    /* kParamOutputGain      */ "GAIN",
    /* kParamOsc2On          */ "OSC 2 ON",
    /* kParamOsc2Waveform    */ "",
    /* kParamOsc2Mult        */ "",
    /* kParamOsc2Phase       */ "PHASE",
    /* kParamOsc2MixMode     */ "",
    /* kParamOsc2MixDepth    */ "DEPTH",
    /* kParamOsc2EnvOn       */ "ENV ON",
    /* kParamOsc2EnvAttack   */ "ATTACK",
    /* kParamOsc2EnvDecay    */ "DECAY",
    /* kParamOsc2EnvSustain  */ "SUSTAIN",
    /* kParamOsc2EnvRelease  */ "RELEASE",
    /* kParamOsc2EnvSustainOn*/ "SUSTAIN",
    /* kParamOsc2FltOn       */ "FILT ON",
    /* kParamOsc2FltCutoff   */ "CUTOFF",
    /* kParamOsc2FltResonance*/ "RESO",
    /* kParamOsc2FltMode     */ "",
    /* kParamOsc2FEnvAttack  */ "ATTACK",
    /* kParamOsc2FEnvDecay   */ "DECAY",
    /* kParamOsc2FEnvSustain */ "SUSTAIN",
    /* kParamOsc2FEnvRelease */ "RELEASE",
    /* kParamOsc2FEnvSustainOn*/"SUSTAIN",
    /* kParamOsc2FEnvDepth   */ "DEPTH",
};
static_assert(sizeof(kShortLabel) / sizeof(kShortLabel[0]) == kArcanistParamCount,
              "kShortLabel must have one entry per ArcanistParameters value");

std::string arcanistUserPresetsDirectory()
{
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.config/Arcanist/presets";
}

} // namespace

ArcanistUI::ArcanistUI()
    : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    , presetBrowser_(arcanistFactoryPresets(), arcanistUserPresetsDirectory(), "com.spellbound.arcanist")
{
    setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

    // Registers the font fontFace(NANOVG_DEJAVU_SANS_TTF) refers to in
    // onNanoDisplay() -- without this call every text() draw is silently a
    // no-op (see Hex's/Pugilist's UI constructor for the same call).
    loadSharedResources();

    // ── Continuous parameters -> RotaryKnob, boolean -> ToggleSwitch,
    //    choice -> RadioButtonGroup. Positions/sizes are assigned in
    //    layoutWidgets(), called once below after every widget exists.
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
    {
        const auto& spec = kArcanistParamSpecs[i];
        switch (spec.kind)
        {
        case ArcanistParamKind::Continuous:
        {
            auto knob = std::make_unique<hui::dgl::RotaryKnob>(this);
            knob->setPalette(kKnobPalette);
            knob->setRange(spec.min, spec.max);
            knob->setDefaultValue(spec.defaultValue);
            knob->setValue(spec.defaultValue);
            knob->onDragStateChanged = [this, i](bool started) { editParameter(i, started); };
            knob->onValueChanged = [this, i](float v) { setParameterValue(i, v); };
            knobs_[i] = std::move(knob);
            break;
        }
        case ArcanistParamKind::Boolean:
        {
            auto sw = std::make_unique<hui::dgl::ToggleSwitch>(this);
            sw->setPalette(kTogglePalette);
            sw->setPosition(spec.defaultValue > 0.5f ? 1 : 0);
            sw->onDragStateChanged = [this, i](bool started) { editParameter(i, started); };
            sw->onPositionChanged = [this, i](int pos) { setParameterValue(i, pos > 0 ? 1.0f : 0.0f); };
            switches_[i] = std::move(sw);
            break;
        }
        case ArcanistParamKind::Choice:
        {
            const std::vector<std::string>* labels =
                  i == kParamOscWaveform  ? &kOscWaveformLabels
                : i == kParamFilterMode   ? &kFilterModeLabels
                : i == kParamLfoTarget    ? &kLfoTargetLabels
                : i == kParamOsc2Waveform ? &kOsc2WaveformLabels
                : i == kParamOsc2Mult     ? &kOsc2MultLabels
                : i == kParamOsc2MixMode  ? &kOsc2MixModeLabels
                : &kFilterModeLabels; // kParamOsc2FltMode reuses LP/BP/HP

            auto group = std::make_unique<ui::RadioButtonGroup>(this, *labels, kRadioActivePalette, kRadioInactivePalette);
            group->setSelectedIndex(static_cast<int>(spec.defaultValue));
            group->onSelected = [this, i](int idx) {
                editParameter(i, true);
                setParameterValue(i, static_cast<float>(idx));
                editParameter(i, false);
            };
            radioGroups_[i] = std::move(group);
            break;
        }
        }
    }

    // ── Output meter ──────────────────────────────────────────────────────
    outputMeter_ = std::make_unique<hui::dgl::VuMeter>(this);
    outputMeter_->setSize(20, 200);
    outputMeter_->setPalette(kMeterPalette);

    // ── Presets bar ───────────────────────────────────────────────────────
    presetSelector_ = std::make_unique<hui::dgl::PresetSelector>(this);
    presetSelector_->setPalette(kPresetSelectorPalette);
    presetSelector_->setClosedSize(260, 22);
    presetSelector_->onIndexSelected = [this](int index) {
        if (const auto* preset = presetBrowser_.selectIndex(index))
        {
            applyPreset(*preset);
            refreshPresetControls();
        }
    };

    saveButton_ = std::make_unique<hui::dgl::Button>(this);
    saveButton_->setPalette(kBarButtonPalette);
    saveButton_->setLabel("SAVE AS");
    saveButton_->onClick = [this]() {
        FileBrowserOptions options;
        options.saving = true;
        options.defaultName = "New Preset.xml";
        options.title = "Save Arcanist Preset";
        const std::string dir = arcanistUserPresetsDirectory();
        options.startDir = dir.c_str();
        openFileBrowser(options);
    };

    deleteButton_ = std::make_unique<hui::dgl::Button>(this);
    deleteButton_->setPalette(kBarButtonPalette);
    deleteButton_->setLabel("DELETE");
    deleteButton_->onClick = [this]() {
        if (presetBrowser_.deleteCurrent()) refreshPresetControls();
    };

    // ── Embedded MIDI keyboard ────────────────────────────────────────────
    // 3 octaves starting at MIDI 48 (C3): pad/drone chords are typically
    // voiced from around middle C upward (Vangelis/Enya-style sustained
    // chords), unlike Pugilist's fixed low GM percussion notes (35-51,
    // hence its C1 start) -- Arcanist is fully chromatic and polyphonic, so
    // no note-zone tinting is needed (setZones() is skipped entirely).
    keyboard_ = std::make_unique<hui::dgl::MidiKeyboard>(this, 3);
    keyboard_->setPalette(kKeyboardPalette);
    keyboard_->setSize(static_cast<uint>(DISTRHO_UI_DEFAULT_WIDTH - 12), 78);
    keyboard_->setAbsolutePos(6, DISTRHO_UI_DEFAULT_HEIGHT - 78 - 6);
    keyboard_->setOctaveShift(48);
    keyboard_->onNote = [this](const hui::keyboard::NoteEvent& e) {
        sendNote(0, e.note, e.isNoteOn ? e.velocity : 0);
    };

    refreshPresetControls();
    layoutWidgets();
}

void ArcanistUI::layoutWidgets()
{
    // Pure geometry, ported section-by-section from the JUCE-era
    // PluginEditor.cpp's resized() (six Row-1 sections: OSC/FILTER/VOLENV/
    // FENV/LFO/MASTER, five Row-2 sections: OSC2/MIX2/ENV2/FLT2/FENV2, plus
    // the preset bar at the top and the keyboard strip already positioned
    // in the constructor above). Section widths below are copied verbatim
    // from resized()'s content1/content2 removeFromLeft() calls -- the
    // JUCE editor's window was already sized to fit exactly this content
    // (1250x860, same as DISTRHO_UI_DEFAULT_WIDTH/HEIGHT), so the pixel
    // values carry over unchanged.
    constexpr float kHeaderH = 22.f, kPad = 6.f, kBtnH = 22.f, kGap = 3.f;
    constexpr float kKnobSize = 46.f, kLblH = 12.f, kTopBarH = 50.f;
    constexpr float kToggleW = 36.f, kToggleH = 18.f;

    Rect bounds{0.f, 0.f, static_cast<float>(DISTRHO_UI_DEFAULT_WIDTH), static_cast<float>(DISTRHO_UI_DEFAULT_HEIGHT)};

    // ── Preset bar ───────────────────────────────────────────────────────
    {
        Rect bar = bounds.removeFromTop(kTopBarH).reduced(8.f, 8.f);
        const Rect selRect = bar.removeFromLeft(260.f);
        presetSelector_->setAbsolutePos(static_cast<int>(selRect.x), static_cast<int>(selRect.y));
        bar.removeFromLeft(8.f);
        const Rect saveRect = bar.removeFromLeft(90.f);
        saveButton_->setSize(90, static_cast<uint>(saveRect.h));
        saveButton_->setAbsolutePos(static_cast<int>(saveRect.x), static_cast<int>(saveRect.y));
        bar.removeFromLeft(4.f);
        const Rect delRect = bar.removeFromLeft(80.f);
        deleteButton_->setSize(80, static_cast<uint>(delRect.h));
        deleteButton_->setAbsolutePos(static_cast<int>(delRect.x), static_cast<int>(delRect.y));
    }

    const float halfH = bounds.h * 0.5f;
    Rect row1 = bounds.removeFromTop(halfH);
    Rect row2 = bounds;

    Rect content1 = row1.reduced(kPad, kPad);
    Rect content2 = row2.reduced(kPad, kPad);

    const Rect oscSect    = content1.removeFromLeft(180.f);
    const Rect filterSect = content1.removeFromLeft(180.f);
    const Rect volEnvSect = content1.removeFromLeft(268.f);
    const Rect fEnvSect   = content1.removeFromLeft(268.f);
    const Rect lfoSect    = content1.removeFromLeft(175.f);
    const Rect masterSect = content1;

    const Rect osc2Sect  = content2.removeFromLeft(255.f);
    const Rect mix2Sect  = content2.removeFromLeft(160.f);
    const Rect env2Sect  = content2.removeFromLeft(265.f);
    const Rect flt2Sect  = content2.removeFromLeft(175.f);
    const Rect fenv2Sect = content2;

    sections_ = {
        {oscSect,    "OSCILLATOR",   kAccentOsc},
        {filterSect, "FILTER",       kAccentFilter},
        {volEnvSect, "VOL ENV",      kAccentVolEnv},
        {fEnvSect,   "FILTER ENV",   kAccentFEnv},
        {lfoSect,    "LFO",          kAccentLfo},
        {masterSect, "MASTER",       kAccentMaster},
        {osc2Sect,   "OSC 2",        kAccentOsc},
        {mix2Sect,   "MIX 2",        kAccentLfo},
        {env2Sect,   "VOL ENV 2",    kAccentVolEnv},
        {flt2Sect,   "FILTER 2",     kAccentFilter},
        {fenv2Sect,  "FILTER ENV 2", kAccentFEnv},
    };

    auto contentOf = [&](Rect sect) { return sect.reduced(kPad, kPad).withTrimmedTop(kHeaderH + kGap); };

    auto placeRadio = [&](Rect& area, uint32_t param) {
        auto& group = *radioGroups_[param];
        for (int i = 0; i < group.count(); ++i)
        {
            const Rect row = area.removeFromTop(kBtnH);
            group.buttonAt(i).setSize(static_cast<uint>(row.w), static_cast<uint>(row.h));
            group.buttonAt(i).setAbsolutePos(static_cast<int>(row.x), static_cast<int>(row.y));
            area.removeFromTop(kGap);
        }
    };

    auto placeKnob = [&](Rect& col, uint32_t param) {
        const Rect slot = col.removeFromTop(kKnobSize + kLblH + kGap);
        const float kw = std::min(slot.w - 4.f, kKnobSize);
        const Rect knobRect = Rect{slot.x, slot.y, slot.w, kKnobSize}.withSizeKeepingCentre(kw, kw);
        knobs_[param]->setSize(static_cast<uint>(kw), static_cast<uint>(kw));
        knobs_[param]->setAbsolutePos(static_cast<int>(knobRect.x), static_cast<int>(knobRect.y));
        labelRects_[param] = Rect{slot.x, slot.y + kKnobSize + 1.f, slot.w, kLblH};
    };

    auto placeToggle = [&](Rect& area, uint32_t param) {
        const Rect row = area.removeFromTop(kBtnH).withSizeKeepingCentre(kToggleW, kToggleH);
        switches_[param]->setSize(static_cast<uint>(kToggleW), static_cast<uint>(kToggleH));
        switches_[param]->setAbsolutePos(static_cast<int>(row.x), static_cast<int>(row.y));
        labelRects_[param] = Rect{row.x + row.w + 4.f, row.y, 100.f, kToggleH};
    };

    // Places a toggle to the left of the remaining half-row, keeping its
    // label rect readable rather than off the edge of a half-width column
    // (used where a toggle shares a row with a knob, e.g. FILTER ENV's
    // DEPTH+SUSTAIN row).
    auto placeToggleInline = [&](Rect area, uint32_t param) {
        const Rect row = area.withSizeKeepingCentre(kToggleW, kToggleH);
        switches_[param]->setSize(static_cast<uint>(kToggleW), static_cast<uint>(kToggleH));
        switches_[param]->setAbsolutePos(static_cast<int>(row.x), static_cast<int>(row.y));
        labelRects_[param] = Rect{row.x + row.w + 4.f, row.y, area.w - kToggleW - 4.f, kToggleH};
    };

    // ── OSCILLATOR 1 ─────────────────────────────────────────────────────
    {
        Rect area = contentOf(oscSect);
        placeRadio(area, kParamOscWaveform);
        area.removeFromTop(kGap);
        const float half = area.w * 0.5f;
        Rect c1 = area.removeFromLeft(half);
        placeKnob(c1, kParamOscTune);
        placeKnob(area, kParamOscDetune);
    }

    // ── FILTER 1 ─────────────────────────────────────────────────────────
    {
        Rect area = contentOf(filterSect);
        placeRadio(area, kParamFilterMode);
        area.removeFromTop(kGap);
        const float half = area.w * 0.5f;
        Rect c1 = area.removeFromLeft(half);
        placeKnob(c1, kParamFilterCutoff);
        placeKnob(area, kParamFilterResonance);
    }

    // ── VOLUME ENV 1 ─────────────────────────────────────────────────────
    {
        Rect area = contentOf(volEnvSect);
        const float half = area.w * 0.5f;
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamEnvAttack);
            placeKnob(row, kParamEnvDecay);
        }
        area.removeFromTop(kGap);
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamEnvSustain);
            placeKnob(row, kParamEnvRelease);
        }
        area.removeFromTop(kGap);
        placeToggle(area, kParamEnvSustainOn);
    }

    // ── FILTER ENV 1 ─────────────────────────────────────────────────────
    {
        Rect area = contentOf(fEnvSect);
        const float half = area.w * 0.5f;
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamFEnvAttack);
            placeKnob(row, kParamFEnvDecay);
        }
        area.removeFromTop(kGap);
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamFEnvSustain);
            placeKnob(row, kParamFEnvRelease);
        }
        area.removeFromTop(kGap);
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamEnvFilterMod);
            placeToggleInline(row, kParamFEnvSustainOn);
        }
    }

    // ── LFO ──────────────────────────────────────────────────────────────
    {
        Rect area = contentOf(lfoSect);
        placeRadio(area, kParamLfoTarget);
        area.removeFromTop(kGap);
        const float half = area.w * 0.5f;
        Rect c1 = area.removeFromLeft(half);
        placeKnob(c1, kParamLfoSpeed);
        placeKnob(area, kParamLfoDepth);
    }

    // ── MASTER ───────────────────────────────────────────────────────────
    {
        Rect area = contentOf(masterSect);
        placeKnob(area, kParamOutputGain);
        area.removeFromTop(kGap);
        constexpr float meterW = 20.f, meterH = 200.f;
        const float mx = area.x + (area.w - meterW) * 0.5f;
        const float my = area.y + std::max(0.f, (area.h - meterH) * 0.5f);
        outputMeter_->setAbsolutePos(static_cast<int>(mx), static_cast<int>(my));
    }

    // ── OSC 2 ────────────────────────────────────────────────────────────
    {
        Rect area = contentOf(osc2Sect);
        placeToggle(area, kParamOsc2On);
        area.removeFromTop(kGap);
        const float half = area.w * 0.5f;
        Rect leftCol = area.removeFromLeft(half);
        Rect rightCol = area;
        placeRadio(leftCol, kParamOsc2Waveform);
        placeRadio(rightCol, kParamOsc2Mult);
        rightCol.removeFromTop(kGap);
        placeKnob(rightCol, kParamOsc2Phase);
    }

    // ── MIX MODE ─────────────────────────────────────────────────────────
    {
        Rect area = contentOf(mix2Sect);
        placeRadio(area, kParamOsc2MixMode);
        area.removeFromTop(kGap);
        placeKnob(area, kParamOsc2MixDepth);
    }

    // ── VOL ENV 2 ────────────────────────────────────────────────────────
    {
        Rect area = contentOf(env2Sect);
        placeToggle(area, kParamOsc2EnvOn);
        area.removeFromTop(kGap);
        const float half = area.w * 0.5f;
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamOsc2EnvAttack);
            placeKnob(row, kParamOsc2EnvDecay);
        }
        area.removeFromTop(kGap);
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamOsc2EnvSustain);
            placeKnob(row, kParamOsc2EnvRelease);
        }
        area.removeFromTop(kGap);
        placeToggle(area, kParamOsc2EnvSustainOn);
    }

    // ── FILTER 2 ─────────────────────────────────────────────────────────
    {
        Rect area = contentOf(flt2Sect);
        placeToggle(area, kParamOsc2FltOn);
        area.removeFromTop(kGap);
        placeRadio(area, kParamOsc2FltMode);
        area.removeFromTop(kGap);
        const float half = area.w * 0.5f;
        Rect c1 = area.removeFromLeft(half);
        placeKnob(c1, kParamOsc2FltCutoff);
        placeKnob(area, kParamOsc2FltResonance);
    }

    // ── FILT ENV 2 ───────────────────────────────────────────────────────
    {
        Rect area = contentOf(fenv2Sect);
        if (area.w > 340.f) area = area.withWidth(340.f);
        const float half = area.w * 0.5f;
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamOsc2FEnvAttack);
            placeKnob(row, kParamOsc2FEnvDecay);
        }
        area.removeFromTop(kGap);
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamOsc2FEnvSustain);
            placeKnob(row, kParamOsc2FEnvRelease);
        }
        area.removeFromTop(kGap);
        {
            Rect row = area.removeFromTop(kKnobSize + kLblH + kGap);
            Rect c1 = row.removeFromLeft(half);
            placeKnob(c1, kParamOsc2FEnvDepth);
            placeToggleInline(row, kParamOsc2FEnvSustainOn);
        }
    }
}

void ArcanistUI::parameterChanged(const uint32_t index, const float value)
{
    if (index >= kArcanistParamCount) return;
    setKnobOrSwitch(index, value);
}

void ArcanistUI::setKnobOrSwitch(const uint32_t index, const float value)
{
    switch (kArcanistParamSpecs[index].kind)
    {
    case ArcanistParamKind::Continuous: if (knobs_[index]) knobs_[index]->setValue(value); break;
    case ArcanistParamKind::Boolean:    if (switches_[index]) switches_[index]->setPosition(value > 0.5f ? 1 : 0); break;
    case ArcanistParamKind::Choice:     if (radioGroups_[index]) radioGroups_[index]->setSelectedIndex(static_cast<int>(value)); break;
    }
}

void ArcanistUI::onNanoDisplay()
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    // 1. Background ----------------------------------------------------
    beginPath();
    rect(0.0f, 0.0f, w, h);
    fillColor(toDglColor(kBgColor)); // ArcanistCol::bg()
    fill();
    closePath();

    fontFace(NANOVG_DEJAVU_SANS_TTF);

    // 2. Preset bar label -----------------------------------------------
    fontSize(11.0f);
    textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    fillColor(toDglColor(kTextPrimary));
    text(8.0f, 25.0f, "PRESET", nullptr);

    // 3. Section panels + headers ----------------------------------------
    for (const auto& s : sections_)
    {
        beginPath();
        roundedRect(s.rect.x, s.rect.y, s.rect.w, s.rect.h, 4.0f);
        fillColor(toDglColor(kPanelColor));
        fill();
        closePath();

        beginPath();
        roundedRect(s.rect.x + 0.5f, s.rect.y + 0.5f, s.rect.w - 1.0f, s.rect.h - 1.0f, 4.0f);
        strokeColor(toDglColor(kPanelBorderColor));
        strokeWidth(1.0f);
        stroke();
        closePath();

        fontSize(11.0f);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(toDglColor(s.accent));
        text(s.rect.x + 8.0f, s.rect.y + 11.0f, s.title, nullptr);
    }

    // 4. Continuous knob + boolean switch labels --------------------------
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
    {
        const auto kind = kArcanistParamSpecs[i].kind;
        if (kind == ArcanistParamKind::Choice) continue;
        const Rect& r = labelRects_[i];
        if (r.w <= 0.0f) continue;

        fontSize(7.5f);
        fillColor(toDglColor(kind == ArcanistParamKind::Boolean ? kTextDim : kTextPrimary));
        if (kind == ArcanistParamKind::Boolean)
        {
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            text(r.x, r.y + r.h * 0.5f, kShortLabel[i], nullptr);
        }
        else
        {
            textAlign(ALIGN_CENTER | ALIGN_TOP);
            text(r.x + r.w * 0.5f, r.y, kShortLabel[i], nullptr);
        }
    }
}

void ArcanistUI::uiFileBrowserSelected(const char* filename)
{
    if (filename == nullptr) return;
    std::string path(filename);
    const size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    const size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    if (presetBrowser_.saveAs(base, captureCurrentParameters()))
        refreshPresetControls();
}

void ArcanistUI::applyPreset(const audioplugins::common::presets::Preset& preset)
{
    for (const auto& pv : preset.parameters)
    {
        for (uint32_t i = 0; i < kArcanistParamCount; ++i)
        {
            if (pv.id != kArcanistParamSpecs[i].id) continue;
            setKnobOrSwitch(i, pv.value);
            editParameter(i, true);
            setParameterValue(i, pv.value);
            editParameter(i, false);
            break;
        }
    }
    // Ported from the JUCE-era PresetManager -> setCurrentProgram() ->
    // allNotesOffPending_ path: silence + retrigger on preset change.
    if (auto* adapter = static_cast<ArcanistPluginAdapter*>(getPluginInstancePointer()))
        adapter->requestAllNotesOff();
}

std::vector<audioplugins::common::presets::ParameterValue> ArcanistUI::captureCurrentParameters() const
{
    std::vector<audioplugins::common::presets::ParameterValue> result;
    result.reserve(kArcanistParamCount);
    for (uint32_t i = 0; i < kArcanistParamCount; ++i)
    {
        float value = kArcanistParamSpecs[i].defaultValue;
        switch (kArcanistParamSpecs[i].kind)
        {
        case ArcanistParamKind::Continuous: if (knobs_[i]) value = knobs_[i]->getValue(); break;
        case ArcanistParamKind::Boolean:    if (switches_[i]) value = static_cast<float>(switches_[i]->getPosition()); break;
        case ArcanistParamKind::Choice:     if (radioGroups_[i]) value = static_cast<float>(radioGroups_[i]->getSelectedIndex()); break;
        }
        result.push_back({kArcanistParamSpecs[i].id, value});
    }
    return result;
}

void ArcanistUI::refreshPresetControls()
{
    presetSelector_->setEntries(presetBrowser_.getEntries());
    presetSelector_->setCurrentIndex(presetBrowser_.getCurrentIndex());

    const auto entries = presetBrowser_.getEntries();
    const int idx = presetBrowser_.getCurrentIndex();
    const bool isFactory = (idx >= 0 && static_cast<size_t>(idx) < entries.size()) ? entries[static_cast<size_t>(idx)].isFactory : true;
    deleteButton_->setEnabled(!isFactory);
}

UI* createUI() { return new ArcanistUI(); }

END_NAMESPACE_DISTRHO
