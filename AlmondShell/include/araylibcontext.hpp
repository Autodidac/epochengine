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
#include <exception>
#include <iostream>
#include <memory>
#include <string>

// Raylib last
#include <raylib.h>

namespace almondnamespace::raylibcontext
{
    // Usually at the start of your program, before window creation:
    static almondnamespace::contextwindow::WindowData* g_raylibwindowContext;

    // --- Helper: apply size to Win32 child + raylib/GLFW (after docking) ---
    inline void apply_native_resize(int w, int h, bool updateRaylibWindow)
    {
        const int W = std::max(1, w);
        const int H = std::max(1, h);

        if (s_raylibstate.hwnd) {
            // Make the child window fit the parent client area precisely
            ::SetWindowPos(s_raylibstate.hwnd, nullptr, 0, 0, W, H,
                SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }

#if !defined(RAYLIB_NO_WINDOW)
        // Also force GLFW/raylib to update its framebuffer size (critical after SetParent)
        if (updateRaylibWindow && ::IsWindowReady()) {
            ::SetWindowSize(W, H);
        }
#endif
    }

    inline void dispatch_resize(const std::shared_ptr<core::Context>& ctx,
        unsigned int width,
        unsigned int height,
        bool updateRaylibWindow,
        bool notifyClient = true)
    {
        unsigned int nextWidth = width;
        unsigned int nextHeight = height;
        bool nextUpdateWindow = updateRaylibWindow;
        bool nextNotifyClient = notifyClient;

        if (s_raylibstate.dispatchingResize)
        {
            s_raylibstate.pendingWidth = nextWidth;
            s_raylibstate.pendingHeight = nextHeight;
            s_raylibstate.pendingUpdateWindow = s_raylibstate.pendingUpdateWindow || nextUpdateWindow;
            s_raylibstate.pendingNotifyClient = s_raylibstate.pendingNotifyClient || nextNotifyClient;
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

            if (s_raylibstate.framebufferWidth == 0)
                s_raylibstate.framebufferWidth = safeWidth;
            if (s_raylibstate.framebufferHeight == 0)
                s_raylibstate.framebufferHeight = safeHeight;

            if (ctx) {
                ctx->width = static_cast<int>(safeWidth);
                ctx->height = static_cast<int>(safeHeight);
            }

            // Push to native window + framebuffer (both sides)
            apply_native_resize(static_cast<int>(safeWidth),
                static_cast<int>(safeHeight),
                /*updateRaylibWindow=*/nextUpdateWindow);

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

            if (!s_raylibstate.hasPendingResize) break;

            // Drain pending coalesced resize
            nextWidth = s_raylibstate.pendingWidth;
            nextHeight = s_raylibstate.pendingHeight;
            nextUpdateWindow = s_raylibstate.pendingUpdateWindow;
            nextNotifyClient = s_raylibstate.pendingNotifyClient;

            s_raylibstate.hasPendingResize = false;
            s_raylibstate.pendingUpdateWindow = false;
            s_raylibstate.pendingNotifyClient = false;
        }

        s_raylibstate.dispatchingResize = false;
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
        s_raylibstate.pendingWidth = 0;
        s_raylibstate.pendingHeight = 0;

        // Wire context-aware onResize into our dispatcher
        std::weak_ptr<core::Context> ctxWeak = ctx;
        s_raylibstate.onResize = [ctxWeak](int ww, int hh)
            {
                const int safeW = std::max(1, ww);
                const int safeH = std::max(1, hh);
                auto locked = ctxWeak.lock();
                dispatch_resize(locked,
                    static_cast<unsigned int>(safeW),
                    static_cast<unsigned int>(safeH),
                    /*updateRaylibWindow=*/false,  // avoid feedback loop; caller decides
                    /*notifyClient=*/false);
            };

        if (ctx) {
            ctx->onResize = s_raylibstate.onResize;
        }

        // Seed sizes into state (no native calls yet)
        dispatch_resize(ctx, clampedWidth, clampedHeight, /*updateRaylibWindow=*/false, /*notifyClient=*/false);

        static bool initialized = false;
        if (initialized) return true;
        initialized = true;

        auto& backend = almondnamespace::raylibtextures::get_raylib_backend();
        auto& rlState = backend.rlState;

        // If you need resizable window behavior, set before InitWindow:
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        if (windowTitle.empty()) {
            windowTitle = "Raylib Window";
        }
        InitWindow(static_cast<int>(s_raylibstate.width),
            static_cast<int>(s_raylibstate.height),
            windowTitle.c_str());
        SetWindowTitle(windowTitle.c_str());

        // Mirror raylib’s native handles
        s_raylibstate.hwnd = (HWND)GetWindowHandle();
        s_raylibstate.hdc = GetDC(s_raylibstate.hwnd);
        s_raylibstate.glContext = wglGetCurrentContext();

        const int initialRenderWidth = GetRenderWidth();
        const int initialRenderHeight = GetRenderHeight();
        if (initialRenderWidth > 0)
            s_raylibstate.framebufferWidth = static_cast<unsigned int>(initialRenderWidth);
        else if (s_raylibstate.framebufferWidth == 0)
            s_raylibstate.framebufferWidth = s_raylibstate.width;

        if (initialRenderHeight > 0)
            s_raylibstate.framebufferHeight = static_cast<unsigned int>(initialRenderHeight);
        else if (s_raylibstate.framebufferHeight == 0)
            s_raylibstate.framebufferHeight = s_raylibstate.height;

        Vector2 dpiScale = GetWindowScaleDPI();
        if (dpiScale.x > 0.f)
            s_raylibstate.dpiScaleX = dpiScale.x;
        else
            s_raylibstate.dpiScaleX = 1.0f;

        if (dpiScale.y > 0.f)
            s_raylibstate.dpiScaleY = dpiScale.y;
        else
            s_raylibstate.dpiScaleY = 1.0f;

        // Keep ctx in sync with actual handles (parity with OpenGL path)
        if (ctx) {
            ctx->hwnd = s_raylibstate.hwnd;
            ctx->hdc = s_raylibstate.hdc;
            ctx->hglrc = s_raylibstate.glContext;
            ctx->width = static_cast<int>(std::max(1u, s_raylibstate.framebufferWidth));
            ctx->height = static_cast<int>(std::max(1u, s_raylibstate.framebufferHeight));
        }

        // If docking into a parent, reparent + hard resize both sides once
        if (s_raylibstate.parent) {
            SetParent(s_raylibstate.hwnd, s_raylibstate.parent);
            ShowWindow(s_raylibstate.hwnd, SW_SHOW);

            LONG_PTR style = GetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= WS_CHILD | WS_VISIBLE;
            SetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE, style);

#if defined(_WIN32)
            almondnamespace::core::MakeDockable(s_raylibstate.hwnd, s_raylibstate.parent);
#endif

            if (!windowTitle.empty()) {
                const std::wstring wideTitle(windowTitle.begin(), windowTitle.end());
                SetWindowTextW(s_raylibstate.hwnd, wideTitle.c_str());
            }

            RECT client{};
            GetClientRect(s_raylibstate.parent, &client);
            const int pw = std::max<LONG>(1, client.right - client.left);
            const int ph = std::max<LONG>(1, client.bottom - client.top);

            // Force *both* Win32 child and GLFW framebuffer sizes
            apply_native_resize(pw, ph, /*updateRaylibWindow=*/true);

            // Sync back to state/ctx
            s_raylibstate.width = static_cast<unsigned int>(pw);
            s_raylibstate.height = static_cast<unsigned int>(ph);
            if (ctx) { ctx->width = pw; ctx->height = ph; }

            // Notify client once (optional)
            if (s_raylibstate.onResize) { s_raylibstate.onResize(pw, ph); }

            // Let parent layouts react if they care
            PostMessage(s_raylibstate.parent, WM_SIZE, 0, MAKELPARAM(pw, ph));
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
        const unsigned int prevWindowW = s_raylibstate.width;
        const unsigned int prevWindowH = s_raylibstate.height;

        unsigned int observedWindowW = prevWindowW;
        unsigned int observedWindowH = prevWindowH;

        const int screenW = GetScreenWidth();
        const int screenH = GetScreenHeight();
        if (screenW > 0) observedWindowW = static_cast<unsigned int>(screenW);
        if (screenH > 0) observedWindowH = static_cast<unsigned int>(screenH);

        if (observedWindowW != prevWindowW || observedWindowH != prevWindowH) {
            // Do NOT push SetWindowSize here; we only mirror and notify
            dispatch_resize(ctx, observedWindowW, observedWindowH, /*updateRaylibWindow=*/false, /*notifyClient=*/true);
        }

        unsigned int observedFramebufferW = s_raylibstate.framebufferWidth;
        unsigned int observedFramebufferH = s_raylibstate.framebufferHeight;

        const int renderW = GetRenderWidth();
        const int renderH = GetRenderHeight();
        if (renderW > 0)
            observedFramebufferW = static_cast<unsigned int>(renderW);
        else if (observedFramebufferW == 0)
            observedFramebufferW = observedWindowW;

        if (renderH > 0)
            observedFramebufferH = static_cast<unsigned int>(renderH);
        else if (observedFramebufferH == 0)
            observedFramebufferH = observedWindowH;

        s_raylibstate.framebufferWidth = observedFramebufferW;
        s_raylibstate.framebufferHeight = observedFramebufferH;
        s_raylibstate.screenWidth = static_cast<int>(observedWindowW);
        s_raylibstate.screenHeight = static_cast<int>(observedWindowH);

        Vector2 dpiScale = GetWindowScaleDPI();
        if (dpiScale.x > 0.f)
            s_raylibstate.dpiScaleX = dpiScale.x;
        else
            s_raylibstate.dpiScaleX = 1.0f;

        if (dpiScale.y > 0.f)
            s_raylibstate.dpiScaleY = dpiScale.y;
        else
            s_raylibstate.dpiScaleY = 1.0f;

        if (ctx) {
            ctx->width = static_cast<int>(observedFramebufferW);
            ctx->height = static_cast<int>(observedFramebufferH);
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

        // IMPORTANT: raylib CloseWindow() (no HWND overload)
        ::CloseWindow(s_raylibstate.hwnd);

        if (s_raylibstate.hdc && s_raylibstate.hwnd) {
            ::ReleaseDC(s_raylibstate.hwnd, s_raylibstate.hdc);
            s_raylibstate.hdc = nullptr;
        }

        s_raylibstate.running = false;
        s_raylibstate.framebufferWidth = 0;
        s_raylibstate.framebufferHeight = 0;
        s_raylibstate.dpiScaleX = 1.0f;
        s_raylibstate.dpiScaleY = 1.0f;

        if (ctx) {
            ctx->hwnd = nullptr;
            ctx->hdc = nullptr;
            ctx->hglrc = nullptr;
        }
    }

    // ──────────────────────────────────────────────
    // Helpers
    // ──────────────────────────────────────────────
    inline int  raylib_get_width()  noexcept {
        const unsigned int fb = s_raylibstate.framebufferWidth ? s_raylibstate.framebufferWidth : s_raylibstate.width;
        return static_cast<int>(std::max(1u, fb));
    }
    inline int  raylib_get_height() noexcept {
        const unsigned int fb = s_raylibstate.framebufferHeight ? s_raylibstate.framebufferHeight : s_raylibstate.height;
        return static_cast<int>(std::max(1u, fb));
    }
    inline void raylib_set_window_title(const std::string& title) { SetWindowTitle(title.c_str()); }
    inline bool RaylibIsRunning(std::shared_ptr<core::Context>) { return s_raylibstate.running; }

} // namespace almondnamespace::raylibcontext

#endif // ALMOND_USING_RAYLIB
