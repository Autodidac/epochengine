#pragma once

#include "atexture.h"

#include <vector>
#include <memory>

namespace almondnamespace {
    struct TextureAtlas {
        int atlasWidth;
        int atlasHeight;
        std::vector<Texture> textures;

        TextureAtlas(int width, int height) : atlasWidth(width), atlasHeight(height) {}

        int add_texture(const std::string& textureName, const std::vector<unsigned char>& textureData, int width, int height) {
            int textureId = (int)textures.size();  // Use index as texture ID
            textures.push_back({ textureId, textureName, width, height });
            return textureId;
        }

        Texture get_texture(int id) const {
            return textures[id];
        }
    };
}