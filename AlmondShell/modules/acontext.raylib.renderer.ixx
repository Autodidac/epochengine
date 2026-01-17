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
module;

#include "aengine.config.hpp"

#if defined(ALMOND_USING_RAYLIB)
#include <raylib.h>
#endif

export module acontext.raylib.renderer;

import <algorithm>;
import <cstdint>;
import <span>;

import acontext.raylib.state;     // ::almondnamespace::raylibstate::get_last_viewport_fit
import acontext.raylib.textures;  // ::almondnamespace::raylibtextures::ensure_uploaded / try_get_texture
import aatlas.texture;
import aspritehandle;

#if defined(ALMOND_USING_RAYLIB)

namespace almondnamespace::raylibcontext
{
    inline void begin_frame()
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }

    inline void end_frame()
    {
        EndDrawing();
    }

    inline void draw_sprite(
        SpriteHandle handle,
        std::span<const TextureAtlas* const> atlases,
        float x, float y, float width, float height) noexcept
    {
        if (!handle.is_valid())
            return;

        const int a = static_cast<int>(handle.atlasIndex);
        const int i = static_cast<int>(handle.localIndex);
        if (a < 0 || a >= static_cast<int>(atlases.size()))
            return;

        const TextureAtlas* atlas = atlases[static_cast<std::size_t>(a)];
        if (!atlas)
            return;

        AtlasRegion r{};
        if (!atlas->try_get_entry_info(i, r))
            return;

        // IMPORTANT: fully qualified from inside this namespace.
        ::almondnamespace::raylibtextures::ensure_uploaded(*atlas);
        const Texture2D* texPtr = ::almondnamespace::raylibtextures::try_get_texture(*atlas);
        if (!texPtr || texPtr->id == 0)
            return;

        const Texture2D& tex = *texPtr;

        const Raylib_Rectangle src{
            static_cast<float>(r.x),
            static_cast<float>(r.y),
            static_cast<float>(r.width),
            static_cast<float>(r.height)
        };

        const auto fit = ::almondnamespace::raylibstate::get_last_viewport_fit();

        const float viewportScale = (fit.scale > 0.0f) ? fit.scale : 1.0f;
        const float designWidth = static_cast<float>((std::max)(1, fit.refW));
        const float designHeight = static_cast<float>((std::max)(1, fit.refH));

        Vector2 offset{ 0.0f, 0.0f };
#if !defined(RAYLIB_NO_WINDOW)
        if (IsWindowReady())
            offset = GetRenderOffset();
#endif

        const float baseOffsetX = offset.x + static_cast<float>(fit.vpX);
        const float baseOffsetY = offset.y + static_cast<float>(fit.vpY);

        const bool normalized =
            (x >= 0.f && x <= 1.f && y >= 0.f && y <= 1.f) &&
            ((width <= 1.f && width >= 0.f) || width <= 0.f) &&
            ((height <= 1.f && height >= 0.f) || height <= 0.f);

        float px{}, py{}, pw{}, ph{};
        if (normalized)
        {
            px = baseOffsetX + x * designWidth * viewportScale;
            py = baseOffsetY + y * designHeight * viewportScale;

            const float scaledW = (width > 0.f) ? (width * designWidth * viewportScale)
                : (static_cast<float>(r.width) * viewportScale);
            const float scaledH = (height > 0.f) ? (height * designHeight * viewportScale)
                : (static_cast<float>(r.height) * viewportScale);

            pw = (std::max)(scaledW, 1.0f);
            ph = (std::max)(scaledH, 1.0f);
        }
        else
        {
            px = baseOffsetX + x * viewportScale;
            py = baseOffsetY + y * viewportScale;

            const float scaledW = (width > 0.f) ? (width * viewportScale)
                : (static_cast<float>(r.width) * viewportScale);
            const float scaledH = (height > 0.f) ? (height * viewportScale)
                : (static_cast<float>(r.height) * viewportScale);

            pw = (std::max)(scaledW, 1.0f);
            ph = (std::max)(scaledH, 1.0f);
        }

        const Raylib_Rectangle dst{ px, py, pw, ph };
        DrawTexturePro(tex, src, dst, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
    }
}

#endif // ALMOND_USING_RAYLIB
