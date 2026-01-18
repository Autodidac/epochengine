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
 **************************************************************/

module;

#include "aengine.config.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wingdi.h>
#endif

export module acontext.raylib.context;

import <algorithm>;
import <cmath>;
import <cstdint>;
import <functional>;
import <memory>;
import <string>;
import <string_view>;
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
#if defined(_WIN32)
    using NativeWindowHandle = HWND;
    using NativeDeviceContext = HDC;
    using NativeGlContext = HGLRC;
#else
    using NativeWindowHandle = void*;
    using NativeDeviceContext = void*;
    using NativeGlContext = void*;
#endif

    inline std::string& title_storage()
    {
        static std::string s_title = "AlmondShell";
        return s_title;
    }

#if defined(_WIN32)
    inline NativeWindowHandle get_hwnd_from_raylib() noexcept
    {
        return static_cast<HWND>(almondnamespace::raylib_api::get_window_handle());
    }

    inline bool make_current(NativeDeviceContext dc, NativeGlContext rc) noexcept
    {
        return ::wglMakeCurrent(dc, rc) == TRUE;
    }

    inline bool refresh_wgl_handles() noexcept
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;

        const HWND currentWindow = get_hwnd_from_raylib();
        const bool windowChanged = (currentWindow && currentWindow != st.hwnd);

        // Fast path: already have handles and context.
        if (!windowChanged && st.hdc && st.hglrc)
        {
            (void)make_current(st.hdc, st.hglrc);
            return true;
        }

        st.hwnd = currentWindow;
        st.hdc = st.hwnd ? ::GetDC(st.hwnd) : nullptr;

        // During docking transitions, current context can be temporarily null.
        // Keep the cached one if WGL reports null.
        HGLRC current = ::wglGetCurrentContext();
        if (!current)
            current = st.hglrc;

        st.hglrc = current;

        if (!st.hwnd || !st.hdc || !st.hglrc)
            return false;

        (void)make_current(st.hdc, st.hglrc);
        return true;
    }
#endif // _WIN32

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

        st.parent = parent;
        st.width = (std::max)(1u, width);
        st.height = (std::max)(1u, height);
        st.onResize = std::move(resizeCallback);

        if (!title.empty())
            title_storage() = std::move(title);

        almondnamespace::raylib_api::set_config_flags(
            static_cast<unsigned int>(almondnamespace::raylib_api::flag_msaa_4x_hint));
        almondnamespace::raylib_api::init_window(
            static_cast<int>(st.width),
            static_cast<int>(st.height),
            title_storage().c_str());
        almondnamespace::raylib_api::set_target_fps(0);

#if defined(_WIN32)
        st.hwnd = get_hwnd_from_raylib();
        st.hdc = st.hwnd ? ::GetDC(st.hwnd) : nullptr;
        st.hglrc = ::wglGetCurrentContext();

        if (ctx)
        {
            ctx->hwnd = st.hwnd;
            ctx->hdc = st.hdc;
            ctx->hglrc = st.hglrc;
        }
#else
        (void)ctx;
#endif

        st.running = true;
        st.cleanupIssued = false;

        almondnamespace::atlasmanager::register_backend_uploader(
            almondnamespace::core::ContextType::RayLib,
            almondnamespace::raylibtextures::ensure_uploaded);

        return true;
    }

    export inline void raylib_process()
    {
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

#if defined(_WIN32)
        (void)refresh_wgl_handles();
#endif

        const int w = almondnamespace::raylib_api::get_screen_width();
        const int h = almondnamespace::raylib_api::get_screen_height();
        if (w > 0 && h > 0 && (static_cast<unsigned>(w) != st.width || static_cast<unsigned>(h) != st.height))
        {
            st.width = static_cast<unsigned>(w);
            st.height = static_cast<unsigned>(h);

            if (st.onResize)
                st.onResize(w, h);
        }
    }

    export inline void raylib_clear(float r, float g, float b, float a)
    {
        (void)a;
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

        almondnamespace::raylib_api::begin_drawing();
        almondnamespace::raylib_api::clear_background(almondnamespace::raylib_api::Color{
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

        almondnamespace::raylib_api::end_drawing();
    }

    export inline void raylib_cleanup(std::shared_ptr<core::Context> ctx)
    {
        (void)ctx;
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running || st.cleanupIssued)
            return;

        st.cleanupIssued = true;

        // Stop new uploads first.
        almondnamespace::atlasmanager::unregister_backend_uploader(
            almondnamespace::core::ContextType::RayLib);

#if defined(_WIN32)
        const bool haveCurrent = refresh_wgl_handles();
#else
        const bool haveCurrent = true;
#endif

        // Only tear down GL resources if we can make the context current.
        if (haveCurrent)
            almondnamespace::raylibtextures::shutdown_current_context_backend();

        almondnamespace::raylib_api::close_window();

        st.running = false;
        st.hwnd = nullptr;
        st.hdc = nullptr;
        st.hglrc = nullptr;
        st.parent = nullptr;
        st.onResize = {};
        st.width = 0;
        st.height = 0;
        st.lastViewport = {};
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

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
