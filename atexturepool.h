#pragma once

#include "atextureatlas.h"

#include <unordered_map>
#include <string>

namespace almondnamespace {
    struct TexturePool {
        std::unordered_map<std::string, int> textureMap;

        int get_texture(const std::string& name, almondnamespace::TextureAtlas& atlas) {
            if (textureMap.find(name) == textureMap.end()) {
                int textureId = atlas.add_texture(name, {}, 128, 128); // Placeholder for texture data
                textureMap[name] = textureId;
            }
            return textureMap[name];
        }
    };
}
