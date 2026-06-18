#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PM0LookAndFeel.h"

// ── LedButton ─────────────────────────────────────────────────────────────────
// A TextButton rendered as a hardware LED push-button:
//   [●  LABEL TEXT]
// LED is green when toggled on, dark when off.
// LED colour can be overridden per-instance for accent-colour buttons (e.g.
// the ADSR/ADR sustain toggles).
class LedButton : public juce::TextButton
{
public:
    explicit LedButton (const juce::String& label,
                        juce::Colour led = PM0Col::ledOn())
        : juce::TextButton (label), ledColour_ (led) {}

    void setLedColour (juce::Colour c) { ledColour_ = c; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        bool on   = getToggleState();
        bool over = isMouseOver();
        bool down = isMouseButtonDown();

        // ── Background ────────────────────────────────────────────────────
        juce::Colour bgCol = on   ? juce::Colour (0xFF2A3030)
                           : over ? juce::Colour (0xFF222828)
                                  : juce::Colour (0xFF1A2020);
        if (down) bgCol = bgCol.brighter (0.08f);
        g.setColour (bgCol);
        g.fillRoundedRectangle (bounds, 3.f);

        // ── Border ────────────────────────────────────────────────────────
        juce::Colour borderCol = on ? PM0Col::panelBorder().brighter (0.3f)
                                    : PM0Col::panelBorder();
        g.setColour (borderCol);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.f, 0.8f);

        // ── LED circle ────────────────────────────────────────────────────
        const float ledD  = 7.f;
        const float ledX  = 7.f;
        const float ledY  = (bounds.getHeight() - ledD) * 0.5f;
        auto ledRect = juce::Rectangle<float> (ledX, ledY, ledD, ledD);

        if (on)
        {
            // Outer glow
            g.setColour (ledColour_.withAlpha (0.22f));
            g.fillEllipse (ledRect.expanded (3.f));
            // LED body
            g.setColour (ledColour_);
            g.fillEllipse (ledRect);
            // Specular highlight
            g.setColour (juce::Colours::white.withAlpha (0.45f));
            g.fillEllipse (ledRect.withSize (ledD * 0.38f, ledD * 0.38f)
                                   .translated (ledD * 0.14f, ledD * 0.1f));
        }
        else
        {
            g.setColour (PM0Col::ledOff());
            g.fillEllipse (ledRect);
            g.setColour (juce::Colour (0xFF1A2A1A));
            g.drawEllipse (ledRect.expanded (0.3f), 0.5f);
        }

        // ── Label text ────────────────────────────────────────────────────
        float textX = ledX + ledD + 6.f;
        g.setColour (on ? PM0Col::textPrimary() : PM0Col::textDim().brighter (0.4f));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.5f).withStyle ("Bold")));
        g.drawText (getButtonText(),
                    juce::Rectangle<float> (textX, 0.f,
                                            bounds.getWidth() - textX - 4.f,
                                            bounds.getHeight()),
                    juce::Justification::centredLeft, true);
    }

private:
    juce::Colour ledColour_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedButton)
};
