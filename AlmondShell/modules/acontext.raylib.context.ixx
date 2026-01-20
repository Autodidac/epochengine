/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝
 *
 *   AlmondShell – Raylib Context (Portable)
 **************************************************************/

module;

#include <include/aengine.config.hpp> // for ALMOND_USING Macros

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif

// Win32 has BOOL CloseWindow(HWND). Raylib has void CloseWindow(void).
// Prevent the collision in this TU by temporarily renaming Win32's symbol name during header include.
#   define CloseWindow CloseWindow_Win32
#   include <Windows.h>
#   undef CloseWindow

#   include <wingdi.h> // HGLRC + wgl*
#endif

#include <iostream>

export module acontext.raylib.context;

import <algorithm>;
import <cmath>;
import <cstdint>;
import <functional>;
import <memory>;
import <string>;
import <string_view>;
import <thread>;
import <utility>;

import aengine.core.context;
import aengine.core.commandline;
import aatlas.manager;

import acontext.raylib.state;
import acontext.raylib.textures;
import acontext.raylib.renderer;
import acontext.raylib.input;
import acontext.raylib.api;

#if defined(ALMOND_USING_RAYLIB)

namespace almondnamespace::raylibcontext
{
    using NativeWindowHandle = void*;

    inline std::string& title_storage()
    {
        static std::string s_title = "AlmondShell";
        return s_title;
    }

#if defined(_WIN32)
    namespace detail
    {
        [[nodiscard]] inline HGLRC current_context() noexcept { return ::wglGetCurrentContext(); }
        [[nodiscard]] inline HDC   current_dc() noexcept { return ::wglGetCurrentDC(); }

        inline bool make_current(HDC dc, HGLRC rc) noexcept
        {
            return ::wglMakeCurrent(dc, rc) != FALSE;
        }

        inline void clear_current() noexcept
        {
            (void)::wglMakeCurrent(nullptr, nullptr);
        }

        [[nodiscard]] inline bool contexts_match(HDC a_dc, HGLRC a_rc, HDC b_dc, HGLRC b_rc) noexcept
        {
            return a_dc == b_dc && a_rc == b_rc;
        }

        // Debug helper: verify the multiplexer has made the raylib context current
        inline void debug_expect_raylib_current(const almondnamespace::raylibstate::RaylibState& st, const char* where)
        {
#if defined(_DEBUG)
            const auto dc = current_dc();
            const auto rc = current_context();
            if (dc != st.hdc || rc != st.hglrc)
            {
                std::cerr << "[Raylib] WARNING: raylib context not current at " << where
                    << " (current dc/rc != raylib dc/rc)\n";
            }
#else
            (void)st; (void)where;
#endif
        }

    }
#endif

    namespace detail
    {
        inline void ensure_offscreen_target(almondnamespace::raylibstate::RaylibState& st, unsigned w, unsigned h)
        {
            const unsigned clampedW = (std::max)(1u, w);
            const unsigned clampedH = (std::max)(1u, h);

            if (st.offscreen.id != 0 &&
                st.offscreenWidth == clampedW &&
                st.offscreenHeight == clampedH)
            {
                return;
            }

            if (st.offscreen.id != 0)
                almondnamespace::raylib_api::unload_render_texture(st.offscreen);

            st.offscreen = almondnamespace::raylib_api::load_render_texture(
                static_cast<int>(clampedW),
                static_cast<int>(clampedH));
            st.offscreenWidth = clampedW;
            st.offscreenHeight = clampedH;
        }
    }

    export inline bool raylib_initialize(
        std::shared_ptr<core::Context> ctx,
        NativeWindowHandle parent = nullptr,
        unsigned width = 0,
        unsigned height = 0,
        std::function<void(int, int)> resizeCallback = nullptr,
        std::string title = {})
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;

        if (width == 0)  width = static_cast<unsigned>(core::cli::window_width);
        if (height == 0) height = static_cast<unsigned>(core::cli::window_height);

        st.width = (std::max)(1u, width);
        st.height = (std::max)(1u, height);

        if (!title.empty())
            title_storage() = std::move(title);

        // Prevent re-initializing raylib (it initializes global state once).
        if (st.running)
        {
            return true;
        }

#if defined(_WIN32)
        // Capture whatever context the multiplexer/docking currently has bound.
        const HDC   previousDC = detail::current_dc();
        const HGLRC previousContext = detail::current_context();
#endif

        st.owner_ctx = ctx.get();
        st.owner_thread = std::this_thread::get_id();
        st.userResize = std::move(resizeCallback);

#if defined(_WIN32)
        st.hwnd = static_cast<HWND>(parent ? parent : (ctx ? ctx->hwnd : nullptr));
        st.hdc = ctx ? ctx->hdc : detail::current_dc();
        st.hglrc = ctx ? ctx->hglrc : detail::current_context();
        st.ownsDC = false;

        if (!st.hdc || !st.hglrc)
        {
            std::cerr << "[Raylib] Missing host OpenGL context for Raylib initialization.\n";
            st.running = false;
            st.cleanupIssued = false;
            return false;
        }
#else
        st.hwnd = parent ? parent : (ctx ? ctx->hwnd : nullptr);
#endif

#if defined(_WIN32)
        if (st.hdc && st.hglrc &&
            !detail::contexts_match(previousDC, previousContext, st.hdc, st.hglrc))
        {
            (void)detail::make_current(st.hdc, st.hglrc);
        }
#endif

        almondnamespace::raylib_api::set_config_flags(
            static_cast<unsigned>(almondnamespace::raylib_api::flag_msaa_4x_hint));

        almondnamespace::raylib_api::init_window(
            static_cast<int>(st.width),
            static_cast<int>(st.height),
            title_storage().c_str());

        almondnamespace::raylib_api::set_target_fps(0);

        st.onResize = [](int w, int h)
        {
            auto& state = almondnamespace::raylibstate::s_raylibstate;
            const int clampedW = (std::max)(1, w);
            const int clampedH = (std::max)(1, h);
            state.width = static_cast<unsigned>(clampedW);
            state.height = static_cast<unsigned>(clampedH);

            almondnamespace::raylib_api::set_window_size(clampedW, clampedH);

            (void)raylib_make_current();
            detail::ensure_offscreen_target(state, state.width, state.height);
#if defined(_WIN32)
            detail::debug_expect_raylib_current(state, "raylib_resize");
#endif

            if (state.userResize)
                state.userResize(clampedW, clampedH);
        };

        detail::ensure_offscreen_target(st, st.width, st.height);

#if defined(_WIN32)
        // Restore previous GL binding for the dock/multiplexer host.
        if (!detail::contexts_match(previousDC, previousContext, st.hdc, st.hglrc))
        {
            if (previousDC && previousContext)
                (void)detail::make_current(previousDC, previousContext);
            else
                detail::clear_current();
        }
#endif

        if (ctx)
            ctx->onResize = st.onResize;

        st.running = true;
        st.cleanupIssued = false;

        almondnamespace::atlasmanager::register_backend_uploader(
            almondnamespace::core::ContextType::RayLib,
            almondnamespace::raylibtextures::ensure_uploaded);

        return true;
    }

    export inline bool raylib_make_current() noexcept
    {
#if defined(_WIN32)
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.hdc || !st.hglrc)
            return false;

        return detail::make_current(st.hdc, st.hglrc);
#else
        return true;
#endif
    }

    export inline void raylib_process()
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

        if (almondnamespace::raylib_api::window_should_close())
        {
            st.running = false;
            if (auto cur = almondnamespace::core::get_current_render_context(); cur && cur->windowData)
                cur->windowData->running = false;
            else if (st.owner_ctx && st.owner_ctx->windowData)
                st.owner_ctx->windowData->running = false;
            return;
        }

#if defined(_WIN32)
        // Expect the multiplexer to have activated this backend before calling process.
        detail::debug_expect_raylib_current(st, "raylib_process");
#endif
    }

    export inline void raylib_clear(float r, float g, float b, float /*a*/)
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

#if defined(_WIN32)
        // Reality check: with multi-backend + queued commands, the current context can drift.
        // Raylib requires its own context current for BeginDrawing/EndDrawing. If multiplexer
        // didn't activate it (or it was stolen), heal here.
        (void)raylib_make_current();
        detail::debug_expect_raylib_current(st, "raylib_clear");
#endif

        if (st.offscreen.id == 0)
            return;

        almondnamespace::raylib_api::begin_texture_mode(st.offscreen);
        almondnamespace::raylib_api::clear_background(
            almondnamespace::raylib_api::Color{
                static_cast<unsigned char>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
                static_cast<unsigned char>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
                static_cast<unsigned char>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
                255
            });
    }

    export inline void raylib_present()
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

#if defined(_WIN32)
        (void)raylib_make_current();
        detail::debug_expect_raylib_current(st, "raylib_present");
#endif

        if (st.offscreen.id == 0)
            return;

        almondnamespace::raylib_api::end_texture_mode();

        almondnamespace::raylib_api::begin_drawing();

        const auto& tex = st.offscreen.texture;
        const almondnamespace::raylib_api::Rectangle src{
            0.0f,
            0.0f,
            static_cast<float>(tex.width),
            -static_cast<float>(tex.height)
        };
        const almondnamespace::raylib_api::Rectangle dst{
            0.0f,
            0.0f,
            static_cast<float>(st.width),
            static_cast<float>(st.height)
        };

        almondnamespace::raylib_api::draw_texture_pro(
            tex,
            src,
            dst,
            almondnamespace::raylib_api::Vector2{ 0.0f, 0.0f },
            0.0f,
            almondnamespace::raylib_api::white);

        almondnamespace::raylib_api::end_drawing();
    }

    export inline void raylib_cleanup(std::shared_ptr<core::Context>)
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running || st.cleanupIssued)
            return;

        st.cleanupIssued = true;

        almondnamespace::atlasmanager::unregister_backend_uploader(
            almondnamespace::core::ContextType::RayLib);

#if defined(_WIN32)
        // Cleanup should be called when raylib backend is active; don't steal context here.
        detail::debug_expect_raylib_current(st, "raylib_cleanup");
#endif

        almondnamespace::raylibtextures::shutdown_current_context_backend();
        if (st.offscreen.id != 0)
            almondnamespace::raylib_api::unload_render_texture(st.offscreen);
        almondnamespace::raylib_api::close_window();

#if defined(_WIN32)
        if (st.ownsDC && st.hdc && st.hwnd)
            ::ReleaseDC(st.hwnd, st.hdc);
#endif

        st = {};
    }

    export inline void raylib_set_window_title(std::string_view title)
    {
        title_storage().assign(title.begin(), title.end());
        almondnamespace::raylib_api::set_window_title(title_storage().c_str());
    }

    export inline int raylib_get_width()
    {
        return static_cast<int>(almondnamespace::raylibstate::s_raylibstate.width);
    }

    export inline int raylib_get_height()
    {
        return static_cast<int>(almondnamespace::raylibstate::s_raylibstate.height);
    }

    export inline almondnamespace::raylib_api::Vector2 raylib_get_mouse_position()
    {
        return almondnamespace::raylib_api::get_mouse_position();
    }

    export inline NativeWindowHandle raylib_get_native_window()
    {
        return almondnamespace::raylibstate::s_raylibstate.hwnd;
    }
}

#endif // ALMOND_USING_RAYLIB
