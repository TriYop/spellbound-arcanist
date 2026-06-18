#include "PM0LookAndFeel.h"
#include <cmath>

PM0LookAndFeel::PM0LookAndFeel()
{
    // Knob text boxes
    setColour (juce::Slider::textBoxTextColourId,        PM0Col::valueText());
    setColour (juce::Slider::textBoxBackgroundColourId,  PM0Col::tbBg());
    setColour (juce::Slider::textBoxOutlineColourId,     PM0Col::panelBorder());
    setColour (juce::Slider::textBoxHighlightColourId,   PM0Col::valueArc().withAlpha (0.4f));

    // Labels
    setColour (juce::Label::textColourId,                PM0Col::textPrimary());
    setColour (juce::Label::backgroundColourId,          juce::Colours::transparentBlack);

    // Text editor (inline edit of textbox)
    setColour (juce::TextEditor::backgroundColourId,     PM0Col::tbBg());
    setColour (juce::TextEditor::textColourId,           PM0Col::valueText());
    setColour (juce::TextEditor::outlineColourId,        PM0Col::valueArc().withAlpha (0.6f));
    setColour (juce::TextEditor::highlightColourId,      PM0Col::valueArc().withAlpha (0.3f));
    setColour (juce::TextEditor::highlightedTextColourId, PM0Col::textPrimary());
}

void PM0LookAndFeel::drawRotarySlider (juce::Graphics& g,
                                        int x, int y, int w, int h,
                                        float sliderPos,
                                        float startAngle, float endAngle,
                                        juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.f);
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    const float arcR    = radius;
    const float knobR   = radius - 5.f;
    const float angle   = startAngle + sliderPos * (endAngle - startAngle);
    const float arcStrokeFull  = 3.5f;
    const float arcStrokeValue = 2.5f;

    // ── Track arc (full range, dark) ────────────────────────────────────
    {
        juce::Path arc;
        arc.addArc (cx - arcR, cy - arcR, arcR * 2.f, arcR * 2.f,
                    startAngle, endAngle, true);
        g.setColour (PM0Col::trackArc());
        g.strokePath (arc, juce::PathStrokeType (arcStrokeFull,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Value arc (amber) ────────────────────────────────────────────────
    if (sliderPos > 0.001f)
    {
        juce::Path arc;
        arc.addArc (cx - arcR, cy - arcR, arcR * 2.f, arcR * 2.f,
                    startAngle, angle, true);
        g.setColour (PM0Col::valueArc());
        g.strokePath (arc, juce::PathStrokeType (arcStrokeValue,
                      juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Knob shadow ──────────────────────────────────────────────────────
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (cx - knobR + 1.f, cy - knobR + 2.5f, knobR * 2.f, knobR * 2.f);

    // ── Knob body (gradient) ─────────────────────────────────────────────
    {
        juce::ColourGradient grad (PM0Col::knobTop(), cx, cy - knobR,
                                   PM0Col::knobBottom(), cx, cy + knobR, false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - knobR, cy - knobR, knobR * 2.f, knobR * 2.f);
    }

    // ── Knob rim (top highlight) ─────────────────────────────────────────
    g.setColour (PM0Col::knobRim());
    g.drawEllipse (cx - knobR, cy - knobR, knobR * 2.f, knobR * 2.f, 0.7f);

    // ── Indicator line + tip dot ─────────────────────────────────────────
    {
        float s = std::sin (angle);
        float c = std::cos (angle);
        float startR = knobR * 0.22f;
        float endR   = knobR * 0.72f;

        g.setColour (PM0Col::valueArc());
        g.drawLine (cx + s * startR, cy - c * startR,
                    cx + s * endR,   cy - c * endR,  1.8f);
        g.fillEllipse (cx + s * endR - 2.5f, cy - c * endR - 2.5f, 5.f, 5.f);
    }
}

void PM0LookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.f : 0.5f;
        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (label.getFont());
        g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0),
                          label.getJustificationType(),
                          juce::jmax (1, (int)(label.getHeight() / label.getFont().getHeight())),
                          label.getMinimumHorizontalScale());
    }
}

juce::Label* PM0LookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (juce::Font (juce::FontOptions{}.withHeight (10.f)));
    return l;
}
