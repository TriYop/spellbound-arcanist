#pragma once
#include "audioplugins/common/hui/dgl/Button.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui
{

// Composes N Common::hui::dgl::Button instances into a mutually-exclusive
// selector -- Common's Button is a plain momentary click with no persistent
// "selected" visual state of its own, so this class owns that state and
// re-palettes the active button on every change. Local to Arcanist per
// Common's own design note: adapter/composition code lives in the
// consuming plugin until proven shared by a second/third consumer.
class RadioButtonGroup
{
public:
    RadioButtonGroup(DGL_NAMESPACE::NanoTopLevelWidget* parent,
                      std::vector<std::string> labels,
                      audioplugins::common::hui::dgl::ButtonPalette activePalette,
                      audioplugins::common::hui::dgl::ButtonPalette inactivePalette)
        : activePalette_(activePalette), inactivePalette_(inactivePalette)
    {
        for (auto& label : labels)
        {
            auto button = std::make_unique<audioplugins::common::hui::dgl::Button>(parent);
            button->setLabel(label.c_str());
            const int index = static_cast<int>(buttons_.size());
            button->onClick = [this, index]() { select(index); };
            buttons_.push_back(std::move(button));
        }
        applyPalettes();
    }

    audioplugins::common::hui::dgl::Button& buttonAt(int i) { return *buttons_[static_cast<size_t>(i)]; }
    int count() const { return static_cast<int>(buttons_.size()); }

    // Host-driven / programmatic change. Does NOT invoke onSelected.
    void setSelectedIndex(int index)
    {
        if (index < 0 || index >= count()) return;
        selectedIndex_ = index;
        applyPalettes();
    }
    int getSelectedIndex() const noexcept { return selectedIndex_; }

    std::function<void(int)> onSelected;

private:
    void select(int index)
    {
        selectedIndex_ = index;
        applyPalettes();
        if (onSelected) onSelected(index);
    }

    void applyPalettes()
    {
        for (int i = 0; i < count(); ++i)
            buttons_[static_cast<size_t>(i)]->setPalette(i == selectedIndex_ ? activePalette_ : inactivePalette_);
    }

    std::vector<std::unique_ptr<audioplugins::common::hui::dgl::Button>> buttons_;
    audioplugins::common::hui::dgl::ButtonPalette activePalette_, inactivePalette_;
    int selectedIndex_ = 0;
};

} // namespace ui
