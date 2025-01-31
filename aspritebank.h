#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

namespace almondnamespace {
    struct SpriteBank {
        std::unordered_map<std::string, std::tuple<int, float, float, float, float>> sprites;

        std::optional<std::tuple<int, float, float, float, float>> get_sprite(const std::string& spriteName) const {
            auto it = sprites.find(spriteName);
            if (it != sprites.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        void add_sprite(const std::string& spriteName, int textureId, float x, float y, float width, float height) {
            sprites[spriteName] = std::make_tuple(textureId, x, y, width, height);
        }

    };
}