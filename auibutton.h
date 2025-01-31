#pragma once

#include "aeventsystem.h"

#include <string>
#include <functional>

namespace almondnamespace {
    struct UIButton {
        float x, y, width, height;
        std::string label;
        std::function<void()> onClickCallback;
        bool isHovered = false;
        bool isPressed = false;
    };

    // Factory function to create a UIButton
    inline UIButton createUIButton(float x, float y, float width, float height, const std::string& label) {
        return UIButton{ x, y, width, height, label, nullptr, false, false };
    }

    // Set the callback for the button
    inline void setOnClick(UIButton& button, std::function<void()> callback) {
        button.onClickCallback = std::move(callback);
    }

    // Update the button state based on the event
    inline void updateUIButton(UIButton& button, const Event& event) {
        if (event.type == EventType::MouseMove) {
            button.isHovered = (event.x >= button.x && event.x <= button.x + button.width &&
                event.y >= button.y && event.y <= button.y + button.height);
        }

        if (event.type == EventType::MouseButtonClick && button.isHovered) {
            button.isPressed = true;
            if (button.onClickCallback) {
                button.onClickCallback();
            }
        }
    }

    // Render function placeholder (can integrate with your rendering system)
    /*
    inline void renderUIButton(const UIButton& button, BasicRenderer& renderer) {
        unsigned int color = button.isHovered ? 0x00FF00 : 0xFFFFFF;  // Green when hovered, white when normal
        renderer.DrawRect(button.x, button.y, button.width, button.height, color);
        renderer.DrawText(button.x + 10, button.y + 10, button.label, 0x000000);  // Text color black
    }
    */

} // namespace almond
