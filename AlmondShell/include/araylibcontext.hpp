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
// araylibcontext.hpp
#pragma once

#include "aengineconfig.hpp"

#if defined(ALMOND_USING_RAYLIB)

#include "acontext.hpp"
#include "awindowdata.hpp"

#include "acontextwindow.hpp"
#include "arobusttime.hpp"
#include "aimageloader.hpp"
#include "araylibtextures.hpp" // AtlasGPU, gpu_atlases, ensure_uploaded
#include "araylibstate.hpp"    // brings in the actual inline s_raylibstate
#include "aatlasmanager.hpp"
#include "araylibrenderer.hpp" // RendererContext, RenderMode
#include "acommandline.hpp"    // cli::window_width, cli::window_height
#include "araylibcontextinput.hpp"

#if defined(_WIN32)
namespace almondnamespace::core { void MakeDockable(HWND hwnd, HWND parent); }
#endif

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

// Raylib last
#include <raylib.h>
#if !defined(RAYLIB_NO_WINDOW)
extern "C" void CloseWindow(void);
#endif


namespace almondnamespace::raylibcontext
{
    // Usually at the start of your program, before window creation:
    static almondnamespace::contextwindow::WindowData* g_raylibwindowContext;

    // --- Helper: apply size to Win32 child + raylib/GLFW (after docking) ---
    inline void apply_native_resize(int w, int h, bool updateNativeWindow, bool updateRaylibWindow)
    {
        const int W = std::max(1, w);
        const int H = std::max(1, h);

#if defined(_WIN32)
        if (updateNativeWindow && s_raylibstate.hwnd) {
            ::SetWindowPos(s_raylibstate.hwnd, nullptr, 0, 0, W, H,
                SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
#endif

#if !defined(RAYLIB_NO_WINDOW)
        if (updateRaylibWindow && ::IsWindowReady()) {
            // Convert physical (framebuffer) → logical for GLFW/raylib
            float dx = 1.0f, dy = 1.0f;
            const Vector2 dpi = ::GetWindowScaleDPI();
            if (dpi.x > 0.0f) dx = dpi.x;
            if (dpi.y > 0.0f) dy = dpi.y;

            const int logicalW = std::max(1, (int)std::lround((double)W / dx));
            const int logicalH = std::max(1, (int)std::lround((double)H / dy));

            ::SetWindowSize(logicalW, logicalH);
        }
#endif
    }

    inline std::pair<float, float> framebuffer_scale() noexcept
    {
#if !defined(RAYLIB_NO_WINDOW)
        if (!::IsWindowReady()) {
            return { 1.0f, 1.0f };
        }

        const int framebufferWidth = std::max(1, ::GetRenderWidth());
        const int framebufferHeight = std::max(1, ::GetRenderHeight());

        unsigned int logicalWidth = s_raylibstate.logicalWidth;
        unsigned int logicalHeight = s_raylibstate.logicalHeight;

        if (::IsWindowReady()) {
            const int screenWidth = ::GetScreenWidth();
            const int screenHeight = ::GetScreenHeight();
            if (screenWidth > 0) {
                logicalWidth = static_cast<unsigned int>(screenWidth);
            }
            if (screenHeight > 0) {
                logicalHeight = static_cast<unsigned int>(screenHeight);
            }

            const Vector2 dpi = ::GetWindowScaleDPI();
            if (dpi.x > 0.0f) {
                logicalWidth = static_cast<unsigned int>(std::max(1, (int)std::lround(static_cast<double>(framebufferWidth) / dpi.x)));
            }
            if (dpi.y > 0.0f) {
                logicalHeight = static_cast<unsigned int>(std::max(1, (int)std::lround(static_cast<double>(framebufferHeight) / dpi.y)));
            }
        }

        const float logicalWidthF = static_cast<float>(std::max(1u, logicalWidth));
        const float logicalHeightF = static_cast<float>(std::max(1u, logicalHeight));

        const float sx = static_cast<float>(framebufferWidth) / logicalWidthF;
        const float sy = static_cast<float>(framebufferHeight) / logicalHeightF;

        return { sx, sy };
#else
        return { 1.0f, 1.0f };
#endif
    }

    inline void update_mouse_scale()
    {
#if !defined(RAYLIB_NO_WINDOW)
        if (!::IsWindowReady()) return;
        const auto [sx, sy] = framebuffer_scale();
        ::SetMouseScale(sx, sy);
#endif
    }

    inline void dispatch_resize(const std::shared_ptr<core::Context>& ctx,
        unsigned int width,
        unsigned int height,
        bool updateRaylibWindow,
        bool notifyClient = true,
        bool skipNativeApply = false)
    {
        unsigned int nextWidth = width;
        unsigned int nextHeight = height;
        bool nextUpdateWindow = updateRaylibWindow;
        bool nextNotifyClient = notifyClient;
        bool nextSkipNativeApply = skipNativeApply;

        if (s_raylibstate.dispatchingResize)
        {
            s_raylibstate.pendingWidth = nextWidth;
            s_raylibstate.pendingHeight = nextHeight;
            s_raylibstate.pendingUpdateWindow = s_raylibstate.pendingUpdateWindow || nextUpdateWindow;
            s_raylibstate.pendingNotifyClient = s_raylibstate.pendingNotifyClient || nextNotifyClient;
            s_raylibstate.pendingSkipNativeApply = nextSkipNativeApply;
            s_raylibstate.hasPendingResize = true;
            return;
        }

        s_raylibstate.dispatchingResize = true;

        for (;;)
        {
            const unsigned int safeWidth = std::max(1u, nextWidth);
            const unsigned int safeHeight = std::max(1u, nextHeight);

            // Update state and ctx mirror
            s_raylibstate.width = safeWidth;
            s_raylibstate.height = safeHeight;

            unsigned int logicalWidth = s_raylibstate.logicalWidth;
            unsigned int logicalHeight = s_raylibstate.logicalHeight;

#if !defined(RAYLIB_NO_WINDOW)
            if (::IsWindowReady()) {
                const Vector2 dpi = ::GetWindowScaleDPI();
                if (dpi.x > 0.0f) {
                    logicalWidth = static_cast<unsigned int>(std::max(1, (int)std::lround(static_cast<double>(safeWidth) / dpi.x)));
                }
                else {
                    logicalWidth = safeWidth;
                }

                if (dpi.y > 0.0f) {
                    logicalHeight = static_cast<unsigned int>(std::max(1, (int)std::lround(static_cast<double>(safeHeight) / dpi.y)));
                }
                else {
                    logicalHeight = safeHeight;
                }

                const int screenWidth = ::GetScreenWidth();
                const int screenHeight = ::GetScreenHeight();
                if (screenWidth > 0) {
                    logicalWidth = static_cast<unsigned int>(screenWidth);
                }
                if (screenHeight > 0) {
                    logicalHeight = static_cast<unsigned int>(screenHeight);
                }
            }
            else {
                logicalWidth = safeWidth;
                logicalHeight = safeHeight;
            }
#else
            logicalWidth = safeWidth;
            logicalHeight = safeHeight;
#endif

            s_raylibstate.logicalWidth = std::max(1u, logicalWidth);
            s_raylibstate.logicalHeight = std::max(1u, logicalHeight);

            if (ctx) {
                ctx->width = static_cast<int>(safeWidth);
                ctx->height = static_cast<int>(safeHeight);
            }

            bool hasNativeParent = false;
#if defined(_WIN32)
            HWND observedParent = nullptr;
            if (s_raylibstate.hwnd) {
                observedParent = ::GetParent(s_raylibstate.hwnd);
                if (observedParent && observedParent != ::GetDesktopWindow()) {
                    hasNativeParent = true;
                }
                else {
                    observedParent = nullptr;
                }
                s_raylibstate.parent = observedParent;
            }
            else if (s_raylibstate.parent) {
                hasNativeParent = true;
            }
#endif

            const bool updateNativeWindow = !nextSkipNativeApply && hasNativeParent;
            const bool updateFramebuffer = !nextSkipNativeApply && (nextUpdateWindow || hasNativeParent);

            // Push to native window + framebuffer (both sides)
            if (!nextSkipNativeApply) {
                apply_native_resize(static_cast<int>(safeWidth),
                    static_cast<int>(safeHeight),
                    updateNativeWindow,
                    updateFramebuffer);
            }

            // Notify client
            if (nextNotifyClient && s_raylibstate.clientOnResize) {
                try {
                    s_raylibstate.clientOnResize(static_cast<int>(safeWidth),
                        static_cast<int>(safeHeight));
                }
                catch (const std::exception& e) {
                    std::cerr << "[Raylib] onResize client callback threw: " << e.what() << "\n";
                }
                catch (...) {
                    std::cerr << "[Raylib] onResize client callback threw unknown exception.\n";
                }
            }

            update_mouse_scale();

            if (!s_raylibstate.hasPendingResize) break;

            // Drain pending coalesced resize
            nextWidth = s_raylibstate.pendingWidth;
            nextHeight = s_raylibstate.pendingHeight;
            nextUpdateWindow = s_raylibstate.pendingUpdateWindow;
            nextNotifyClient = s_raylibstate.pendingNotifyClient;
            nextSkipNativeApply = s_raylibstate.pendingSkipNativeApply;

            s_raylibstate.hasPendingResize = false;
            s_raylibstate.pendingUpdateWindow = false;
            s_raylibstate.pendingNotifyClient = false;
            s_raylibstate.pendingSkipNativeApply = false;
        }

        s_raylibstate.dispatchingResize = false;
    }

    inline void sync_framebuffer_size(const std::shared_ptr<core::Context>& ctx,
        bool notifyClient)
    {
#if !defined(RAYLIB_NO_WINDOW)
        if (!::IsWindowReady()) return;

        const int framebufferWidth = ::GetRenderWidth();
        const int framebufferHeight = ::GetRenderHeight();

        if (framebufferWidth > 0 && framebufferHeight > 0) {
            dispatch_resize(ctx,
                static_cast<unsigned int>(framebufferWidth),
                static_cast<unsigned int>(framebufferHeight),
                /*updateRaylibWindow=*/false,
                notifyClient,
                /*skipNativeApply=*/true);
        }
#endif
    }

    // ──────────────────────────────────────────────
    // Initialize Raylib window and context
    // ──────────────────────────────────────────────
    inline bool raylib_initialize(std::shared_ptr<core::Context> ctx,
        HWND parentWnd = nullptr,
        unsigned int w = 800,
        unsigned int h = 600,
        std::function<void(int, int)> onResize = nullptr,
        std::string windowTitle = {})
    {
        const unsigned int clampedWidth = std::max(1u, w);
        const unsigned int clampedHeight = std::max(1u, h);

        s_raylibstate.clientOnResize = std::move(onResize);
        s_raylibstate.parent = parentWnd;
        s_raylibstate.dispatchingResize = false;
        s_raylibstate.hasPendingResize = false;
        s_raylibstate.pendingUpdateWindow = false;
        s_raylibstate.pendingNotifyClient = false;
        s_raylibstate.pendingSkipNativeApply = false;
        s_raylibstate.pendingWidth = 0;
        s_raylibstate.pendingHeight = 0;

        // Wire context-aware onResize into our dispatcher
        std::weak_ptr<core::Context> ctxWeak = ctx;
        s_raylibstate.onResize = [ctxWeak](int ww, int hh)
            {
                const int safeW = std::max(1, ww);
                const int safeH = std::max(1, hh);
                if (auto locked = ctxWeak.lock()) {
                    dispatch_resize(locked, (unsigned)safeW, (unsigned)safeH,
                        /*updateRaylibWindow=*/true,
                        /*notifyClient=*/false);
                }
            };
        if (ctx) ctx->onResize = s_raylibstate.onResize;

        // Seed sizes into state (no native calls yet)
        dispatch_resize(ctx, clampedWidth, clampedHeight, /*updateRaylibWindow=*/false, /*notifyClient=*/false);

        static bool initialized = false;
        if (initialized) return true;
        initialized = true;

        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        if (windowTitle.empty()) windowTitle = "Raylib Window";

        InitWindow((int)s_raylibstate.width, (int)s_raylibstate.height, windowTitle.c_str());
        SetWindowTitle(windowTitle.c_str());
        update_mouse_scale();
        sync_framebuffer_size(ctx, /*notifyClient=*/false);

        // Mirror raylib’s native handles
#if defined(_WIN32)
        s_raylibstate.hwnd = (HWND)GetWindowHandle();
        s_raylibstate.hdc = GetDC(s_raylibstate.hwnd);
        s_raylibstate.glContext = wglGetCurrentContext();
#endif

        if (ctx) {
            ctx->hwnd = s_raylibstate.hwnd;
            ctx->hdc = s_raylibstate.hdc;
            ctx->hglrc = s_raylibstate.glContext;
            ctx->width = (int)s_raylibstate.width;
            ctx->height = (int)s_raylibstate.height;
        }

        // If docking into a parent, reparent + hard resize both sides once
        if (s_raylibstate.parent) {
            ::SetParent(s_raylibstate.hwnd, s_raylibstate.parent);
            ::ShowWindow(s_raylibstate.hwnd, SW_SHOW);

            // Add clip styles for clean child docking
            LONG_PTR style = ::GetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
            ::SetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE, style);

            LONG_PTR ex = ::GetWindowLongPtr(s_raylibstate.hwnd, GWL_EXSTYLE);
            ex &= ~WS_EX_APPWINDOW;
            ::SetWindowLongPtr(s_raylibstate.hwnd, GWL_EXSTYLE, ex);

#if defined(_WIN32)
            almondnamespace::core::MakeDockable(s_raylibstate.hwnd, s_raylibstate.parent);
#endif

            if (!windowTitle.empty()) {
                const std::wstring wideTitle(windowTitle.begin(), windowTitle.end());
                ::SetWindowTextW(s_raylibstate.hwnd, wideTitle.c_str());
            }

            RECT client{};
            ::GetClientRect(s_raylibstate.parent, &client);
            const int pw = std::max<LONG>(1, client.right - client.left);  // physical
            const int ph = std::max<LONG>(1, client.bottom - client.top);   // physical

            // Fit child HWND (physical) + tell raylib/GLFW in logical units
            apply_native_resize(pw, ph,
                /*updateNativeWindow=*/true,
                /*updateRaylibWindow=*/true);

            // Sync back to state/ctx with physical size
            s_raylibstate.width = (unsigned)pw;
            s_raylibstate.height = (unsigned)ph;
            unsigned int logicalChildW = static_cast<unsigned int>(std::max<LONG>(1, pw));
            unsigned int logicalChildH = static_cast<unsigned int>(std::max<LONG>(1, ph));
#if !defined(RAYLIB_NO_WINDOW)
            const Vector2 childDpi = ::GetWindowScaleDPI();
            if (childDpi.x > 0.0f) {
                logicalChildW = static_cast<unsigned int>(std::max(1, (int)std::lround(static_cast<double>(pw) / childDpi.x)));
            }
            if (childDpi.y > 0.0f) {
                logicalChildH = static_cast<unsigned int>(std::max(1, (int)std::lround(static_cast<double>(ph) / childDpi.y)));
            }
#endif
            s_raylibstate.logicalWidth = std::max(1u, logicalChildW);
            s_raylibstate.logicalHeight = std::max(1u, logicalChildH);
            if (ctx) { ctx->width = pw; ctx->height = ph; }

            // Notify client once (optional)
            if (s_raylibstate.onResize) { s_raylibstate.onResize(pw, ph); }

            // Let parent layouts react if they care
            ::PostMessage(s_raylibstate.parent, WM_SIZE, 0, MAKELPARAM(pw, ph));

            // Final sync from actual framebuffer (handles any rounding)
            sync_framebuffer_size(ctx, /*notifyClient=*/true);
        }

        s_raylibstate.running = true;

        // Hook atlas uploads
        atlasmanager::register_backend_uploader(core::ContextType::RayLib,
            [](const TextureAtlas& atlas) {
                raylibtextures::ensure_uploaded(atlas);
            });

        return true;
    }

    inline void Raylib_CloseWindow() { /* no-op placeholder */ }

    // ──────────────────────────────────────────────
    // Per-frame event processing
    // ──────────────────────────────────────────────
    inline bool raylib_process(std::shared_ptr<core::Context> ctx, core::CommandQueue& queue)
    {
        if (!s_raylibstate.running || WindowShouldClose()) {
            s_raylibstate.running = false;
            return true;
        }

        atlasmanager::process_pending_uploads(core::ContextType::RayLib);

        // Observe current window size from raylib; coalesce through dispatcher
        const unsigned int prevW = s_raylibstate.width;
        const unsigned int prevH = s_raylibstate.height;

        unsigned int obsW = prevW;
        unsigned int obsH = prevH;

        const int rw = GetRenderWidth();
        const int rh = GetRenderHeight();
        if (rw > 0) obsW = static_cast<unsigned int>(rw);
        if (rh > 0) obsH = static_cast<unsigned int>(rh);

        if (obsW != prevW || obsH != prevH) {
            // Do NOT push SetWindowSize here; we only mirror and notify
            dispatch_resize(ctx, obsW, obsH,
                /*updateRaylibWindow=*/false,
                /*notifyClient=*/true,
                /*skipNativeApply=*/true);
        }

        if (!wglMakeCurrent(s_raylibstate.hdc, s_raylibstate.glContext)) {
            s_raylibstate.running = false;
            std::cerr << "[Raylib] Failed to make Raylib GL context current\n";
            return true;
        }

        // Animated bg proves frame is live and viewport matches
        static auto* bgTimer = almondnamespace::time::getTimer("menu", "bg_color");
        if (!bgTimer) bgTimer = &almondnamespace::time::createNamedTimer("menu", "bg_color");
        const double t = almondnamespace::time::elapsed(*bgTimer);

        const unsigned char r = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 1.0)) * 255);
        const unsigned char g = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 0.7 + 2.0)) * 255);
        const unsigned char b = static_cast<unsigned char>((0.5 + 0.5 * std::sin(t * 1.3 + 4.0)) * 255);

        // Correct order: Begin → Clear → drain → End
        BeginDrawing();
        ClearBackground(Color{ r, g, b, 255 });

        queue.drain();

        EndDrawing();

        // optional: wglMakeCurrent(nullptr, nullptr);
        return true; // continue running
    }

    // ──────────────────────────────────────────────
    // Clear / Present helpers (symmetry only)
    // ──────────────────────────────────────────────
    inline void raylib_clear()
    {
        BeginDrawing();
        ClearBackground(DARKPURPLE);
        EndDrawing();
    }

    inline void raylib_present() { /* EndDrawing() presents already */ }

    // ──────────────────────────────────────────────
    // Cleanup and shutdown
    // ──────────────────────────────────────────────
    inline void raylib_cleanup(std::shared_ptr<almondnamespace::core::Context>& ctx)
    {
        if (!s_raylibstate.running) return;

        // Detach current GL context first (avoid dangling current on destroy)
#if defined(_WIN32)
        if (s_raylibstate.hdc && s_raylibstate.glContext) {
            wglMakeCurrent(nullptr, nullptr);
        }
#endif

#if !defined(RAYLIB_NO_WINDOW)
        // Proper raylib shutdown (GLFW + GL + internal state)
        if (IsWindowReady()) {
            CloseWindow();  // raylib’s zero-arg API
        }
#endif

#if defined(_WIN32)
        // If re-parenting/styling kept the child HWND alive, force-destroy it.
        if (s_raylibstate.hwnd && ::IsWindow(s_raylibstate.hwnd)) {
            if (s_raylibstate.hdc) {
                ::ReleaseDC(s_raylibstate.hwnd, s_raylibstate.hdc);
                s_raylibstate.hdc = nullptr;
            }
            ::DestroyWindow(s_raylibstate.hwnd);
        }
        s_raylibstate.hwnd = nullptr;
        s_raylibstate.glContext = nullptr;
#endif

        s_raylibstate.running = false;

        if (ctx) {
            ctx->hwnd = nullptr;
            ctx->hdc = nullptr;
            ctx->hglrc = nullptr;
        }
    }
    // ──────────────────────────────────────────────
    // Helpers
    // ──────────────────────────────────────────────
    inline int  raylib_get_width()  noexcept { return static_cast<int>(s_raylibstate.width); }
    inline int  raylib_get_height() noexcept { return static_cast<int>(s_raylibstate.height); }
    inline void raylib_set_window_title(const std::string& title) { SetWindowTitle(title.c_str()); }
    inline bool RaylibIsRunning(std::shared_ptr<core::Context>) { return s_raylibstate.running; }

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
