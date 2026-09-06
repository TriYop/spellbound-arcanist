#pragma once
#include "DistrhoUI.hpp"

START_NAMESPACE_DISTRHO

class ArcanistUI : public UI
{
public:
    ArcanistUI() : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT) {}

protected:
    void parameterChanged(uint32_t, float) override {}

    void onNanoDisplay() override
    {
        beginPath();
        rect(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
        fillColor(Color(24, 27, 24, 255)); // ArcanistCol::bg() 0xFF181B18, see Task 6
        fill();
        closePath();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArcanistUI)
};

END_NAMESPACE_DISTRHO
