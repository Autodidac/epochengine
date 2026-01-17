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

import aatlas.manager;
import acontext.raylib.state;
import acontext.raylib.textures;
import acontext.raylib.renderer;
import acontext.raylib.input;

#if defined(ALMOND_USING_RAYLIB)

namespace almondnamespace::raylibcontext
{
#if defined(ALMOND_USING_RAYLIB)
    namespace
    {
        inline almondnamespace::raylibstate::GuiFitViewport compute_fit_viewport(
            int fbW, int fbH, int refW, int refH) noexcept
        {
            almondnamespace::raylibstate::GuiFitViewport r{};
            r.fbW = (std::max)(1, fbW);
            r.fbH = (std::max)(1, fbH);
            r.refW = (std::max)(1, refW);
            r.refH = (std::max)(1, refH);

            const float sx = static_cast<float>(r.fbW) / static_cast<float>(r.refW);
            const float sy = static_cast<float>(r.fbH) / static_cast<float>(r.refH);
            r.scale = (std::max)(0.0001f, (std::min)(sx, sy));

            r.vpW = (std::max)(1, static_cast<int>(std::lround(r.refW * r.scale)));
            r.vpH = (std::max)(1, static_cast<int>(std::lround(r.refH * r.scale)));
            r.vpX = (r.fbW - r.vpW) / 2;
            r.vpY = (r.fbH - r.vpH) / 2;
            return r;
        }

        inline std::pair<int, int> resolve_reference_size(
            const almondnamespace::raylibstate::RaylibState& st) noexcept
        {
            const bool widthOverride = core::cli::window_width_overridden
                && core::cli::window_width > 0;
            const bool heightOverride = core::cli::window_height_overridden
                && core::cli::window_height > 0;

            const unsigned fallbackW = (st.virtualWidth > 0u)
                ? st.virtualWidth
                : (st.designWidth > 0u ? st.designWidth : st.logicalWidth);
            const unsigned fallbackH = (st.virtualHeight > 0u)
                ? st.virtualHeight
                : (st.designHeight > 0u ? st.designHeight : st.logicalHeight);

            const int refW = widthOverride ? core::cli::window_width
                : static_cast<int>((std::max)(1u, fallbackW));
            const int refH = heightOverride ? core::cli::window_height
                : static_cast<int>((std::max)(1u, fallbackH));

            return { refW, refH };
        }

        inline void apply_mouse_viewport_mapping(
            const almondnamespace::raylibstate::GuiFitViewport& fit) noexcept
        {
#if !defined(RAYLIB_NO_WINDOW)
            if (!::IsWindowReady())
                return;

            const Vector2 renderOffset = ::GetRenderOffset();
            const int baseOffsetX = static_cast<int>(std::floor(renderOffset.x)) + fit.vpX;
            const int baseOffsetY = static_cast<int>(std::floor(renderOffset.y)) + fit.vpY;
            const float invScale = (fit.scale > 0.0f) ? (1.0f / fit.scale) : 1.0f;
            ::SetMouseOffset(-baseOffsetX, -baseOffsetY);
            ::SetMouseScale(invScale, invScale);
#endif
        }
    }
#endif

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
        auto& st = almondnamespace::raylibstate::s_raylibstate;

        if (width == 0)  width = static_cast<unsigned>(core::cli::window_width);
        if (height == 0) height = static_cast<unsigned>(core::cli::window_height);

        st.parent = parent;
        st.width = (std::max)(1u, width);
        st.height = (std::max)(1u, height);
        st.logicalWidth = st.width;
        st.logicalHeight = st.height;
        st.onResize = std::move(resizeCallback);

        if (!title.empty())
            title_storage() = std::move(title);

        ::SetConfigFlags(FLAG_MSAA_4X_HINT);
        ::InitWindow(static_cast<int>(st.width), static_cast<int>(st.height), title_storage().c_str());
        ::SetTargetFPS(0);

#if !defined(RAYLIB_NO_WINDOW)
        if (::IsWindowReady())
        {
            const int fbW = (std::max)(1, ::GetRenderWidth());
            const int fbH = (std::max)(1, ::GetRenderHeight());
            const int logicalW = (std::max)(1, ::GetScreenWidth());
            const int logicalH = (std::max)(1, ::GetScreenHeight());

            st.width = static_cast<unsigned>(fbW);
            st.height = static_cast<unsigned>(fbH);
            st.logicalWidth = static_cast<unsigned>(logicalW);
            st.logicalHeight = static_cast<unsigned>(logicalH);
        }
#endif

        if (st.designWidth == 0u)
        {
            st.designWidth = core::cli::window_width_overridden && core::cli::window_width > 0
                ? static_cast<unsigned>(core::cli::window_width)
                : st.logicalWidth;
        }
        if (st.designHeight == 0u)
        {
            st.designHeight = core::cli::window_height_overridden && core::cli::window_height > 0
                ? static_cast<unsigned>(core::cli::window_height)
                : st.logicalHeight;
        }
        if (st.virtualWidth == 0u)
            st.virtualWidth = st.designWidth;
        if (st.virtualHeight == 0u)
            st.virtualHeight = st.designHeight;

        {
            const auto [refW, refH] = resolve_reference_size(st);
            st.lastViewport = compute_fit_viewport(
                static_cast<int>((std::max)(1u, st.width)),
                static_cast<int>((std::max)(1u, st.height)),
                refW,
                refH);
            apply_mouse_viewport_mapping(st.lastViewport);
        }

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

        // Hook input here once you stabilize exported names in acontext.raylib.input
        // almondnamespace::raylibcontextinput::process();

        const int logicalW = (std::max)(1, ::GetScreenWidth());
        const int logicalH = (std::max)(1, ::GetScreenHeight());
        const int fbW = (std::max)(1, ::GetRenderWidth());
        const int fbH = (std::max)(1, ::GetRenderHeight());

        const bool logicalChanged =
            static_cast<unsigned>(logicalW) != st.logicalWidth
            || static_cast<unsigned>(logicalH) != st.logicalHeight;
        const bool framebufferChanged =
            static_cast<unsigned>(fbW) != st.width
            || static_cast<unsigned>(fbH) != st.height;

        if (logicalChanged)
        {
            st.logicalWidth = static_cast<unsigned>(logicalW);
            st.logicalHeight = static_cast<unsigned>(logicalH);
            if (st.onResize)
                st.onResize(logicalW, logicalH);
        }

        if (framebufferChanged)
        {
            st.width = static_cast<unsigned>(fbW);
            st.height = static_cast<unsigned>(fbH);
        }

        const auto [refW, refH] = resolve_reference_size(st);
        st.lastViewport = compute_fit_viewport(fbW, fbH, refW, refH);
        st.virtualWidth = static_cast<unsigned>(refW);
        st.virtualHeight = static_cast<unsigned>(refH);
        apply_mouse_viewport_mapping(st.lastViewport);

        const std::uintptr_t windowId = reinterpret_cast<std::uintptr_t>(st.hwnd);
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
            static_cast<std::int64_t>(st.logicalWidth),
            telemetry::RendererTelemetryTags{ almondnamespace::core::ContextType::RayLib, windowId, "logical_width" });
        telemetry::emit_gauge(
            "renderer.framebuffer.size",
            static_cast<std::int64_t>(st.logicalHeight),
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

        almondnamespace::atlasmanager::unregister_backend_uploader(
            almondnamespace::core::ContextType::RayLib);

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
        st.hglrc = nullptr;
        st.parent = nullptr;
        st.onResize = {};
        st.width = 0;
        st.height = 0;
        st.logicalWidth = 0;
        st.logicalHeight = 0;
        st.virtualWidth = 0;
        st.virtualHeight = 0;
        st.designWidth = 0;
        st.designHeight = 0;
        st.lastViewport = {};
    }

    export inline void raylib_set_window_title(std::string_view title)
    {
        title_storage().assign(title.begin(), title.end());
        ::SetWindowTitle(title_storage().c_str());
    }

    export inline int raylib_get_width()
    {
        const auto& st = almondnamespace::raylibstate::s_raylibstate;
        return static_cast<int>((std::max)(1u, st.logicalWidth));
    }

    export inline int raylib_get_height()
    {
        const auto& st = almondnamespace::raylibstate::s_raylibstate;
        return static_cast<int>((std::max)(1u, st.logicalHeight));
    }

    export inline ::Vector2 raylib_get_mouse_position()
    {
        return ::GetMousePosition();
    }

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
