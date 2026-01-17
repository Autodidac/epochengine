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

#if defined(ALMOND_USING_RAYLIB)
#include <raylib.h>
#endif

export module acontext.raylib.context;

import <algorithm>;
import <cmath>;
import <cstdint>;
import <functional>;
import <memory>;
import <mutex>;
import <string>;
import <string_view>;
import <utility>;

import aengine.core.context;
import aengine.core.commandline;
import aengine.context.type;
import aengine.core.time;
import aengine.telemetry;

import acontext.raylib.state;
import acontext.raylib.textures;
import acontext.raylib.renderer;
import acontext.raylib.input;

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
        // raylib function, not Win32
        return static_cast<HWND>(::GetWindowHandle());
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

        if (!windowChanged && st.hdc && st.hglrc)
        {
            (void)make_current(st.hdc, st.hglrc);
            return true;
        }

        st.hwnd = currentWindow;
        st.hdc = st.hwnd ? ::GetDC(st.hwnd) : nullptr;
        st.hglrc = ::wglGetCurrentContext();

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
        (void)ctx;

        auto& st = almondnamespace::raylibstate::s_raylibstate;

        if (width == 0)  width = static_cast<unsigned>(core::cli::window_width);
        if (height == 0) height = static_cast<unsigned>(core::cli::window_height);

        st.parent = parent;
        st.width = width;
        st.height = height;
        st.onResize = std::move(resizeCallback);

        if (!title.empty())
            title_storage() = std::move(title);

        ::SetConfigFlags(FLAG_MSAA_4X_HINT);
        ::InitWindow(static_cast<int>(st.width), static_cast<int>(st.height), title_storage().c_str());
        ::SetTargetFPS(0);

#if defined(_WIN32)
        st.hwnd = get_hwnd_from_raylib();
        st.hdc = st.hwnd ? ::GetDC(st.hwnd) : nullptr;
        st.glContext = ::wglGetCurrentContext();
#endif

        st.running = true;
        st.cleanupIssued = false;
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

        // Hook input here once you stabilize exported names in acontext.raylib.input
        // almondnamespace::raylibcontextinput::process();

        const int w = ::GetScreenWidth();
        const int h = ::GetScreenHeight();
        if (w > 0 && h > 0 && (static_cast<unsigned>(w) != st.width || static_cast<unsigned>(h) != st.height))
        {
            st.width = static_cast<unsigned>(w);
            st.height = static_cast<unsigned>(h);

            if (st.onResize)
                st.onResize(w, h);
        }

        const std::uintptr_t windowId = reinterpret_cast<std::uintptr_t>(st.hwnd);
        const int fbW = ::GetRenderWidth();
        const int fbH = ::GetRenderHeight();
        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(fbW),
            telemetry::RendererTelemetryTags{ almondnamespace::core::ContextType::RayLib, windowId, "framebuffer_width" });
        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(fbH),
            telemetry::RendererTelemetryTags{ almondnamespace::core::ContextType::RayLib, windowId, "framebuffer_height" });
        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(st.width),
            telemetry::RendererTelemetryTags{ almondnamespace::core::ContextType::RayLib, windowId, "logical_width" });
        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(st.height),
            telemetry::RendererTelemetryTags{ almondnamespace::core::ContextType::RayLib, windowId, "logical_height" });
    }

    export inline void raylib_clear(float r, float g, float b, float a)
    {
        (void)a;
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running)
            return;

        almondnamespace::timing::reset(st.frameTimer);
        ::BeginDrawing();
        ::ClearBackground(::Color{
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

        auto ctx = core::get_current_render_context();
        const std::uintptr_t windowId = ctx && ctx->windowData
            ? reinterpret_cast<std::uintptr_t>(ctx->windowData->hwnd)
            : reinterpret_cast<std::uintptr_t>(st.hwnd);
        if (ctx && ctx->windowData)
        {
            std::size_t depth = 0;
            {
                std::scoped_lock lock(ctx->windowData->commandQueue.get_mutex());
                depth = ctx->windowData->commandQueue.get_queue().size();
            }
            telemetry::emit_gauge(
                "renderer.command_queue.depth",
                static_cast<std::int64_t>(depth),
                telemetry::RendererTelemetryTags{ ctx->type, windowId });
        }

        ::EndDrawing();
        const double frameMs = almondnamespace::timing::realElapsed(st.frameTimer) * 1000.0;
        telemetry::emit_histogram_ms(
            "renderer.frame.time_ms",
            frameMs,
            telemetry::RendererTelemetryTags{ core::ContextType::RayLib, windowId });
    }

    export inline void raylib_cleanup(std::shared_ptr<core::Context> ctx)
    {
        (void)ctx;
        auto& st = almondnamespace::raylibstate::s_raylibstate;
        if (!st.running || st.cleanupIssued)
            return;

        st.cleanupIssued = true;

        // If you added this exported function in acontext.raylib.textures, keep it.
        // Otherwise comment it out.
        almondnamespace::raylibtextures::shutdown_current_context_backend();

        // Avoid Win32 CloseWindow(HWND) collision:
#if defined(_WIN32)
#pragma push_macro("CloseWindow")
#undef CloseWindow
#endif

        ::CloseWindow(st.hwnd); // raylib CloseWindow()

#if defined(_WIN32)
#pragma pop_macro("CloseWindow")
#endif

        st.running = false;
        st.hwnd = nullptr;
        st.hdc = nullptr;
        st.glContext = nullptr;
        st.parent = nullptr;
        st.onResize = {};
        st.width = 0;
        st.height = 0;
        st.lastViewport = {};
    }

    export inline void raylib_set_window_title(std::string_view title)
    {
        title_storage().assign(title.begin(), title.end());
        ::SetWindowTitle(title_storage().c_str());
    }

    export inline int raylib_get_width()
    {
        return static_cast<int>(almondnamespace::raylibstate::s_raylibstate.width);
    }

    export inline int raylib_get_height()
    {
        return static_cast<int>(almondnamespace::raylibstate::s_raylibstate.height);
    }

    export inline ::Vector2 raylib_get_mouse_position()
    {
        return ::GetMousePosition();
    }

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
