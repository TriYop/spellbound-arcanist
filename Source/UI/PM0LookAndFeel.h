#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ── Shared colour constants ────────────────────────────────────────────────────
struct PM0Col
{
    static juce::Colour bg()           { return juce::Colour (0xFF181B18); }  // editor background
    static juce::Colour panel()        { return juce::Colour (0xFF1E2220); }  // section panel face
    static juce::Colour panelBorder()  { return juce::Colour (0xFF323A32); }  // section border
    static juce::Colour knobTop()      { return juce::Colour (0xFF4E5248); }  // knob gradient top
    static juce::Colour knobBottom()   { return juce::Colour (0xFF252825); }  // knob gradient bottom
    static juce::Colour knobRim()      { return juce::Colour (0xFF5C6058); }  // knob rim highlight
    static juce::Colour valueArc()     { return juce::Colour (0xFFE88814); }  // value arc (warm amber)
    static juce::Colour trackArc()     { return juce::Colour (0xFF131513); }  // full-range track
    static juce::Colour ledOn()        { return juce::Colour (0xFF22E840); }  // LED active (green)
    static juce::Colour ledOff()       { return juce::Colour (0xFF0A1A0A); }  // LED inactive
    static juce::Colour textPrimary()  { return juce::Colour (0xFFCCC8B4); }  // main label text
    static juce::Colour textDim()      { return juce::Colour (0xFF4C504A); }  // disabled/dim text
    static juce::Colour valueText()    { return juce::Colour (0xFF888078); }  // textbox value
    static juce::Colour tbBg()         { return juce::Colour (0xFF111311); }  // textbox background

    // Section accent colours (for header labels and borders)
    static juce::Colour osc()          { return juce::Colour (0xFFE8CC60); }  // warm gold
    static juce::Colour filter()       { return juce::Colour (0xFF60C8E8); }  // cyan
    static juce::Colour volEnv()       { return juce::Colour (0xFF80E860); }  // green
    static juce::Colour fEnv()         { return juce::Colour (0xFFB060E8); }  // purple
    static juce::Colour lfo()          { return juce::Colour (0xFFE88040); }  // orange
    static juce::Colour master()       { return juce::Colour (0xFFD8D4C0); }  // cream
};

// ── Custom look-and-feel ───────────────────────────────────────────────────────
class PM0LookAndFeel : public juce::LookAndFeel_V4
{
public:
    PM0LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};
