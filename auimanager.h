#pragma once

#include "auibutton.h"
#include "aeventsystem.h"

#include <vector>
#include <memory>
#include <functional>

namespace almondnamespace {
    struct UIManager {
        std::vector<std::shared_ptr<UIButton>> buttons;
    };

    // Factory function to create a UIManager
    inline UIManager createUIManager() {
        return UIManager{ {} };
    }

    // Add a button to the UIManager
    inline void addButton(UIManager& manager, std::shared_ptr<UIButton> button) {
        manager.buttons.push_back(std::move(button));
    }

    // Update all buttons with events
    inline void updateUIManager(UIManager& manager, EventSystem& eventSystem) {
        eventSystem.PollEvents();
        for (auto& button : manager.buttons) {
            Event event;
            updateUIButton(*button, event);  // Update individual buttons
        }
    }

    // Render function placeholder
    /*
    inline void renderUIManager(const UIManager& manager, BasicRenderer& renderer) {
        for (const auto& button : manager.buttons) {
            renderUIButton(*button, renderer);
        }
    }
    */

} // namespace almond
