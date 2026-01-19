/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 **************************************************************/

module;

#include <include/aengine.config.hpp>

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
// Same collision avoidance as your context TU.
#   define CloseWindow CloseWindow_Win32
#   include <Windows.h>
#   undef CloseWindow
#   include <wingdi.h>
#endif

export module acontext.raylib.renderer;

import <algorithm>;
import <cstdint>;
import <span>;

import acontext.raylib.state;
import acontext.raylib.textures;
import aatlas.texture;
import aspritehandle;
import acontext.raylib.api;

#if defined(ALMOND_USING_RAYLIB)

namespace almondnamespace::raylibrenderer
{
#if defined(_WIN32)
    struct make_current_guard
    {
        HDC   prev_dc{};
        HGLRC prev_rc{};
        bool  changed{ false };

        make_current_guard(HDC dc, HGLRC rc) noexcept
        {
            prev_dc = ::wglGetCurrentDC();
            prev_rc = ::wglGetCurrentContext();

            if (dc && rc && (prev_dc != dc || prev_rc != rc))
                changed = (::wglMakeCurrent(dc, rc) != FALSE);
        }

        ~make_current_guard() noexcept
        {
            if (!changed) return;

            if (prev_dc && prev_rc)
                (void)::wglMakeCurrent(prev_dc, prev_rc);
            else
                (void)::wglMakeCurrent(nullptr, nullptr);
        }
    };
#endif

    // DO NOT call BeginDrawing/EndDrawing here.
    // The context layer owns frame boundaries; this renderer only issues draw calls.
    export inline void begin_frame() {}
    export inline void end_frame() {}

    export inline void draw_sprite(
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

        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

#if defined(_WIN32)
        // Hard requirement: raylib GL context must be current for uploads + draw calls.
        make_current_guard guard{ st.hdc, st.hglrc };
#endif

        // Upload (this will no-op if cached + correct version).
        almondnamespace::raylibtextures::ensure_uploaded(*atlas);

        const auto* texPtr = almondnamespace::raylibtextures::try_get_texture(*atlas);
        if (!texPtr || texPtr->id == 0)
            return;

        const auto& tex = *texPtr;

        const almondnamespace::raylib_api::Rectangle src{
            static_cast<float>(r.x),
            static_cast<float>(r.y),
            static_cast<float>(r.width),
            static_cast<float>(r.height)
        };

        const auto fit = almondnamespace::raylibstate::get_last_viewport_fit();

        const float viewportScale = (fit.scale > 0.0f) ? fit.scale : 1.0f;
        const float designWidth = static_cast<float>((std::max)(1, fit.refW));
        const float designHeight = static_cast<float>((std::max)(1, fit.refH));

        const float baseOffsetX = static_cast<float>(fit.vpX);
        const float baseOffsetY = static_cast<float>(fit.vpY);

        const bool normalized =
            (x >= 0.f && x <= 1.f && y >= 0.f && y <= 1.f) &&
            ((width <= 1.f && width >= 0.f) || width <= 0.f) &&
            ((height <= 1.f && height >= 0.f) || height <= 0.f);

        float px{}, py{}, pw{}, ph{};
        if (normalized)
        {
            px = baseOffsetX + x * designWidth * viewportScale;
            py = baseOffsetY + y * designHeight * viewportScale;

            const float scaledW = (width > 0.f)
                ? (width * designWidth * viewportScale)
                : (static_cast<float>(r.width) * viewportScale);

            const float scaledH = (height > 0.f)
                ? (height * designHeight * viewportScale)
                : (static_cast<float>(r.height) * viewportScale);

            pw = (std::max)(scaledW, 1.0f);
            ph = (std::max)(scaledH, 1.0f);
        }
        else
        {
            px = baseOffsetX + x * viewportScale;
            py = baseOffsetY + y * viewportScale;

            const float scaledW = (width > 0.f)
                ? (width * viewportScale)
                : (static_cast<float>(r.width) * viewportScale);

            const float scaledH = (height > 0.f)
                ? (height * viewportScale)
                : (static_cast<float>(r.height) * viewportScale);

            pw = (std::max)(scaledW, 1.0f);
            ph = (std::max)(scaledH, 1.0f);
        }

        const almondnamespace::raylib_api::Rectangle dst{ px, py, pw, ph };

        almondnamespace::raylib_api::draw_texture_pro(
            tex,
            src,
            dst,
            almondnamespace::raylib_api::Vector2{ 0.0f, 0.0f },
            0.0f,
            almondnamespace::raylib_api::white);
    }
}

#endif // ALMOND_USING_RAYLIB
