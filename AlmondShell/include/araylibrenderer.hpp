/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for Non-Commercial Purposes ONLY,          *
 *   without prior commercial licensing agreement.            *
 *                                                            *
 *   Redistribution Allowed with This Notice and              *
 *   LICENSE file. No obligation to disclose modifications.   *
 *                                                            *
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
#pragma once

#include "aengineconfig.hpp"

#if defined(ALMOND_USING_RAYLIB)

#include "araylibtextures.hpp"
#include "aspritehandle.hpp"
#include "aatlastexture.hpp"
#include "araylibstate.hpp"

#include <iostream>
//#undef Rectangle // Avoid conflict with raylib Rectangle
#include <algorithm>
#include <cmath>

#include <raylib.h> // Ensure this is included after platform-specific headers
namespace almondnamespace::raylibcontext
{
     struct Rect {
            float x;
            float y;
            float width;
            float height;

            // Conversion operator to raylib's Rectangle type  
            operator struct Raylib_Rectangle() const {
                return { x, y, width, height };
            }
        };
    struct RendererContext
    {
        enum class RenderMode {
            SingleTexture,
            TextureAtlas
        };

        RenderMode mode = RenderMode::TextureAtlas;
    };

    inline RendererContext raylib_renderer{};

    inline void begin_frame()
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }

    inline void end_frame()
    {
        EndDrawing();
    }

    inline void draw_sprite(SpriteHandle handle, std::span<const TextureAtlas* const> atlases, float x, float y, float width, float height) noexcept
    {
        if (!handle.is_valid()) {
            std::cerr << "[Raylib_DrawSprite] Invalid sprite handle.\n";
            return;
        }

        const int atlasIdx = static_cast<int>(handle.atlasIndex);
        const int localIdx = static_cast<int>(handle.localIndex);

        if (atlasIdx < 0 || atlasIdx >= static_cast<int>(atlases.size())) {
            std::cerr << "[Raylib_DrawSprite] Atlas index out of bounds: " << atlasIdx << '\n';
            return;
        }

        const TextureAtlas* atlas = atlases[atlasIdx];
        if (!atlas) {
            std::cerr << "[Raylib_DrawSprite] Null atlas pointer at index: " << atlasIdx << '\n';
            return;
        }

        AtlasRegion region{};
        if (!atlas->try_get_entry_info(localIdx, region)) {
            std::cerr << "[Raylib_DrawSprite] Sprite index out of bounds: " << localIdx << '\n';
            return;
        }

        almondnamespace::raylibtextures::ensure_uploaded(*atlas);

        auto texIt = almondnamespace::raylibtextures::raylib_gpu_atlases.find(atlas);
        if (texIt == almondnamespace::raylibtextures::raylib_gpu_atlases.end() || texIt->second.texture.id == 0) {
            std::cerr << "[Raylib_DrawSprite] Atlas not uploaded or invalid texture ID: " << atlas->name << "\n";
            return;
        }

        const Texture2D& texture = texIt->second.texture;
        Rect srcRect{
            static_cast<float>(region.x),
            static_cast<float>(region.y),
            static_cast<float>(region.width),
            static_cast<float>(region.height)
        };

        const unsigned int fbWidthStored = s_raylibstate.framebufferWidth ? s_raylibstate.framebufferWidth : s_raylibstate.width;
        const unsigned int fbHeightStored = s_raylibstate.framebufferHeight ? s_raylibstate.framebufferHeight : s_raylibstate.height;

        const float framebufferWidth = static_cast<float>(std::max(1u, fbWidthStored));
        const float framebufferHeight = static_cast<float>(std::max(1u, fbHeightStored));
        const float logicalWidth = static_cast<float>(std::max(1u, s_raylibstate.width));
        const float logicalHeight = static_cast<float>(std::max(1u, s_raylibstate.height));

        float scaleX = logicalWidth > 0.f ? framebufferWidth / logicalWidth : 1.0f;
        float scaleY = logicalHeight > 0.f ? framebufferHeight / logicalHeight : 1.0f;

        if (!std::isfinite(scaleX) || scaleX <= 0.f)
            scaleX = 1.0f;
        if (!std::isfinite(scaleY) || scaleY <= 0.f)
            scaleY = 1.0f;

        float drawX = x;
        float drawY = y;
        float drawWidth = width;
        float drawHeight = height;

        const bool widthNormalized = drawWidth > 0.f && drawWidth <= 1.f;
        const bool heightNormalized = drawHeight > 0.f && drawHeight <= 1.f;

        if (widthNormalized) {
            if (drawX >= 0.f && drawX <= 1.f)
                drawX *= framebufferWidth;
            drawWidth = std::max(drawWidth * framebufferWidth, 1.0f);
        } else {
            drawX *= scaleX;
            drawWidth *= scaleX;
        }

        if (heightNormalized) {
            if (drawY >= 0.f && drawY <= 1.f)
                drawY *= framebufferHeight;
            drawHeight = std::max(drawHeight * framebufferHeight, 1.0f);
        } else {
            drawY *= scaleY;
            drawHeight *= scaleY;
        }

        if (drawWidth <= 0.f)
            drawWidth = static_cast<float>(region.width);
        if (drawHeight <= 0.f)
            drawHeight = static_cast<float>(region.height);

        Rect destRect{
            drawX,
            drawY,
            drawWidth,
            drawHeight
        };

        Vector2 origin{ 0.0f, 0.0f };  // No rotation offset
        float rotation = 0.0f;

        DrawTexturePro(texture, srcRect, destRect, origin, rotation, WHITE);
    }

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
