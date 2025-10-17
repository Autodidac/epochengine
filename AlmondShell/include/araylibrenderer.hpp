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
 // araylibrenderer.hpp
#pragma once

#include "aplatform.hpp"        // Must be first
#include "aengineconfig.hpp"

#if defined(ALMOND_USING_RAYLIB)

#include "araylibtextures.hpp"
#include "aspritehandle.hpp"
#include "aatlastexture.hpp"
#include "araylibstate.hpp"

#include <algorithm>
#include <iostream>
#include <span>

#include <raylib.h> // must be included (you had it commented out)

// ---------- DPI helpers ----------
namespace almondnamespace::raylibcontext
{
    inline float ui_scale_x() noexcept {
#if !defined(RAYLIB_NO_WINDOW)
        if (IsWindowReady()) {
            const Vector2 dpi = GetWindowScaleDPI();           // e.g. {1.5, 1.5}
            if (dpi.x > 0.f) return dpi.x;
        }
#endif
        return 1.f;
    }

    inline float ui_scale_y() noexcept {
#if !defined(RAYLIB_NO_WINDOW)
        if (IsWindowReady()) {
            const Vector2 dpi = GetWindowScaleDPI();
            if (dpi.y > 0.f) return dpi.y;
        }
#endif
        return 1.f;
    }

    // Call this after init and whenever size/DPI can change
    inline void sync_input_scaling() noexcept {
#if !defined(RAYLIB_NO_WINDOW)
        if (!IsWindowReady()) return;
        SetMouseScale(ui_scale_x(), ui_scale_y());
#endif
    }

    struct RendererContext
    {
        enum class RenderMode { SingleTexture, TextureAtlas };
        RenderMode mode = RenderMode::TextureAtlas;
    };

    inline RendererContext raylib_renderer{};

    inline void begin_frame()
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Viewport should be set by your context loop using GetRenderWidth/Height().
        // (Do it there to avoid GL include churn here.)
    }

    inline void end_frame()
    {
        EndDrawing();
    }

    // Draw sprite in either normalized (0..1) or logical pixels (pre-DPI) space.
    inline void draw_sprite(SpriteHandle handle,
        std::span<const TextureAtlas* const> atlases,
        float x, float y, float width, float height) noexcept
    {
        if (!handle.is_valid()) return;

        const int a = (int)handle.atlasIndex;
        const int i = (int)handle.localIndex;
        if (a < 0 || a >= (int)atlases.size()) return;

        const TextureAtlas* atlas = atlases[a];
        if (!atlas) return;

        AtlasRegion r{};
        if (!atlas->try_get_entry_info(i, r)) return;

        // Ensure GPU upload
        almondnamespace::raylibtextures::ensure_uploaded(*atlas);
        auto it = almondnamespace::raylibtextures::raylib_gpu_atlases.find(atlas);
        if (it == almondnamespace::raylibtextures::raylib_gpu_atlases.end() || it->second.texture.id == 0) return;

        const Texture2D& tex = it->second.texture;

        // Source rect in atlas pixels
        Raylib_Rectangle src{ (float)r.x, (float)r.y, (float)r.width, (float)r.height };

        const int rw = std::max(1, GetRenderWidth());
        const int rh = std::max(1, GetRenderHeight());
        const float sx = ui_scale_x(); // DPI scale for logical pixels
        const float sy = ui_scale_y();

        // Consider rect "normalized" if any dimension is 0<..<=1 or coords are in [0..1]
        const bool normalized =
            (width > 0.f && width <= 1.f) ||
            (height > 0.f && height <= 1.f) ||
            ((x >= 0.f && x <= 1.f) && (y >= 0.f && y <= 1.f));

        float px, py, pw, ph;
        if (normalized) {
            // 0..1 -> framebuffer pixels
            px = x * rw;
            py = y * rh;
            pw = (width > 0.f ? width * rw : (float)r.width);
            ph = (height > 0.f ? height * rh : (float)r.height);
        }
        else {
            // logical pixels -> DPI-scaled pixels
            px = x * sx;
            py = y * sy;
            pw = (width > 0.f ? width * sx : (float)r.width * sx);
            ph = (height > 0.f ? height * sy : (float)r.height * sy);
        }

        pw = std::max(pw, 1.0f);
        ph = std::max(ph, 1.0f);

        Raylib_Rectangle dst{ px, py, pw, ph };
        DrawTexturePro(tex, src, dst, Vector2{ 0,0 }, 0.0f, WHITE);
    }

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
