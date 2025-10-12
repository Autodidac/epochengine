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

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>

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
        std::function<void(int, int)> onResize = nullptr)
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
        InitWindow(static_cast<int>(s_raylibstate.width),
            static_cast<int>(s_raylibstate.height),
            "Raylib Window");

        // Mirror raylib’s native handles
        s_raylibstate.hwnd = (HWND)GetWindowHandle();
        s_raylibstate.hdc = GetDC(s_raylibstate.hwnd);
        s_raylibstate.glContext = wglGetCurrentContext();

        // Keep ctx in sync with actual handles (parity with OpenGL path)
        if (ctx) {
            ctx->hwnd = s_raylibstate.hwnd;
            ctx->hdc = s_raylibstate.hdc;
            ctx->hglrc = s_raylibstate.glContext;
            ctx->width = static_cast<int>(s_raylibstate.width);
            ctx->height = static_cast<int>(s_raylibstate.height);
        }

        // If docking into a parent, reparent + hard resize both sides once
        if (s_raylibstate.parent) {
            SetParent(s_raylibstate.hwnd, s_raylibstate.parent);
            ShowWindow(s_raylibstate.hwnd, SW_SHOW);

            LONG_PTR style = GetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= WS_CHILD | WS_VISIBLE;
            SetWindowLongPtr(s_raylibstate.hwnd, GWL_STYLE, style);

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
        const unsigned int prevW = s_raylibstate.width;
        const unsigned int prevH = s_raylibstate.height;

        unsigned int obsW = prevW;
        unsigned int obsH = prevH;

        const int rw = GetScreenWidth();
        const int rh = GetScreenHeight();
        if (rw > 0) obsW = static_cast<unsigned int>(rw);
        if (rh > 0) obsH = static_cast<unsigned int>(rh);

        if (obsW != prevW || obsH != prevH) {
            // Do NOT push SetWindowSize here; we only mirror and notify
            dispatch_resize(ctx, obsW, obsH, /*updateRaylibWindow=*/false, /*notifyClient=*/true);
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
